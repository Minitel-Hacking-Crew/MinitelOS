#include "globals.h"
#include <cmath>

// ─── External globals ─────────────────────────────────────────────────────────
extern std::map<String, int>    shell_int_vars;
extern std::map<String, float>  shell_float_vars;
extern std::map<String, String> shell_string_vars;
extern std::map<String, bool>   shell_bool_vars;
extern std::map<String, String> shell_vars;
extern int    numCommands;
extern bool   shell_break_flag;
extern void   shell_println_wrapped(const String &text);
// write_to_file déclarée dans shell.h (inclus via globals.h)
extern String shell_abspath(const String &path);
extern String shell_output_buffer;
extern bool   shell_redirect_mode;
extern String shell_current_dir;
extern ShellCommand commands[];

// Script-level exit flag — stops entire script; reset on each shell_run call.
static bool shell_exit_flag   = false;
// Return flag — set by "return" inside a function; cleared by the call handler.
static bool shell_return_flag = false;
// Function registry — populated by the first pass of shell_run.
static std::map<String, std::vector<String>> shell_script_funcs;

// ─── Utilities ────────────────────────────────────────────────────────────────
float round2(float val) { return roundf(val * 100.0f) / 100.0f; }

// Substitute $var references, longest-first to avoid substring collisions.
static String subst_vars(const String &src) {
    if (shell_int_vars.empty() && shell_float_vars.empty() &&
        shell_string_vars.empty() && shell_bool_vars.empty()) return src;
    std::vector<std::pair<String, String>> pairs;
    for (auto &kv : shell_int_vars)
        pairs.push_back({"$" + kv.first, String(kv.second)});
    for (auto &kv : shell_float_vars) {
        char buf[16]; dtostrf(kv.second, 0, 2, buf);
        pairs.push_back({"$" + kv.first, String(buf)});
    }
    for (auto &kv : shell_string_vars)
        pairs.push_back({"$" + kv.first, kv.second});
    for (auto &kv : shell_bool_vars)
        pairs.push_back({"$" + kv.first, kv.second ? "true" : "false"});
    std::sort(pairs.begin(), pairs.end(),
              [](const std::pair<String,String> &a, const std::pair<String,String> &b){
                  return a.first.length() > b.first.length();
              });
    String result = src;
    for (auto &p : pairs) result.replace(p.first, p.second);
    return result;
}

// Substitute $var AND bare numeric-var names for arithmetic expressions.
// Bare names (without $) are substituted for int/float vars only, longest-first.
static String subst_vars_arith(const String &src) {
    String result = subst_vars(src);   // first pass: $var for all types
    std::vector<std::pair<String, String>> pairs;
    for (auto &kv : shell_int_vars)
        pairs.push_back({kv.first, String(kv.second)});
    for (auto &kv : shell_float_vars) {
        char buf[16]; dtostrf(kv.second, 0, 2, buf);
        pairs.push_back({kv.first, String(buf)});
    }
    std::sort(pairs.begin(), pairs.end(),
              [](const std::pair<String,String> &a, const std::pair<String,String> &b){
                  return a.first.length() > b.first.length();
              });
    for (auto &p : pairs) result.replace(p.first, p.second);
    return result;
}

// Resolve a single token as a variable name or literal value.
static String resolve_val(const String &v) {
    if (shell_int_vars.count(v))    return String(shell_int_vars.at(v));
    if (shell_float_vars.count(v))  { char buf[16]; dtostrf(shell_float_vars.at(v), 0, 2, buf); return String(buf); }
    if (shell_string_vars.count(v)) return shell_string_vars.at(v);
    if (shell_bool_vars.count(v))   return shell_bool_vars.at(v) ? "true" : "false";
    return v;
}

// ─── Arithmetic ───────────────────────────────────────────────────────────────
// Returns index of the first binary operator, skipping an optional leading sign.
static int find_op(const String &expr) {
    int start = (expr.length() > 0 && (expr[0] == '+' || expr[0] == '-')) ? 1 : 0;
    for (int i = start; i < (int)expr.length(); i++) {
        char c = expr[i];
        if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%') return i;
    }
    return -1;
}

static int eval_int_expr(const String &rawExpr) {
    String expr = subst_vars_arith(rawExpr);
    expr.trim();
    int opIdx = find_op(expr);
    if (opIdx < 0) return expr.toInt();
    char op   = expr[opIdx];
    int left  = expr.substring(0, opIdx).toInt();
    int right = expr.substring(opIdx + 1).toInt();
    switch (op) {
        case '+': return left + right;
        case '-': return left - right;
        case '*': return left * right;
        case '/': return right ? left / right : 0;
        case '%': return right ? left % right : 0;
    }
    return 0;
}

static float eval_float_expr(const String &rawExpr) {
    String expr = subst_vars_arith(rawExpr);
    expr.trim();
    int opIdx = find_op(expr);
    if (opIdx < 0) return expr.toFloat();
    char  op    = expr[opIdx];
    float left  = expr.substring(0, opIdx).toFloat();
    float right = expr.substring(opIdx + 1).toFloat();
    switch (op) {
        case '+': return left + right;
        case '-': return left - right;
        case '*': return left * right;
        case '/': return right != 0.0f ? left / right : 0.0f;
    }
    return 0.0f;
}

// ─── Condition evaluation ─────────────────────────────────────────────────────
// Format: "<left> <op> <right>"
// Int ops:    eq ne gt lt ge le
// String ops: seq sne
static bool eval_condition(const String &rawCond) {
    String cond = subst_vars(rawCond);   // $var substitution
    cond.trim();
    int s1 = cond.indexOf(' ');
    int s2 = cond.lastIndexOf(' ');
    if (s1 < 0 || s2 <= s1) return false;
    String left  = cond.substring(0, s1);
    String op    = cond.substring(s1 + 1, s2);
    String right = cond.substring(s2 + 1);
    left.trim(); op.trim(); right.trim();
    // Resolve bare variable names (backward compat: "while n lt 10")
    left  = resolve_val(left);
    right = resolve_val(right);
    if (op == "seq") return left  == right;
    if (op == "sne") return left  != right;
    int l = left.toInt();
    int r = right.toInt();
    if (op == "eq")  return l == r;
    if (op == "ne")  return l != r;
    if (op == "gt")  return l >  r;
    if (op == "lt")  return l <  r;
    if (op == "ge")  return l >= r;
    if (op == "le")  return l <= r;
    return false;
}

// ─── Command execution (with I/O redirect support) ────────────────────────────
void shell_eval_line(const String &line) {
    String proc = subst_vars(line);
    int appendIdx   = proc.indexOf(">>");
    bool appendMode = (appendIdx != -1);
    int redirectIdx = appendMode ? appendIdx : proc.indexOf('>');
    String redirectFile = "";
    String realCmd      = proc;
    if (redirectIdx != -1) {
        realCmd      = proc.substring(0, redirectIdx);
        redirectFile = proc.substring(redirectIdx + (appendMode ? 2 : 1));
        realCmd.trim(); redirectFile.trim();
    }
    int spaceIdx = realCmd.indexOf(' ');
    String cmd  = (spaceIdx == -1) ? realCmd : realCmd.substring(0, spaceIdx);
    String args = (spaceIdx == -1) ? "" : realCmd.substring(spaceIdx + 1);

    String prev_buf   = shell_output_buffer;
    bool   prev_redir = shell_redirect_mode;
    if (redirectFile.length() > 0 || prev_redir) {
        shell_output_buffer = ""; shell_redirect_mode = true;
    }
    bool found = false;
    for (int i = 0; i < numCommands; ++i) {
        if (cmd == commands[i].name) { commands[i].func(args); found = true; break; }
    }
    if (!found && realCmd.length() > 0)
        shell_println_wrapped("Commande inconnue : " + cmd);
    if (redirectFile.length() > 0) {
        write_to_file(shell_abspath(redirectFile), shell_output_buffer, appendMode, true);
        shell_redirect_mode = false;
    } else if (prev_redir) {
        prev_buf += shell_output_buffer;
        shell_output_buffer = prev_buf;
        shell_redirect_mode = true;
    }
}

// ─── Line processing ──────────────────────────────────────────────────────────
bool shell_process_line(const String &line) {
    String l = line;
    l.trim();
    if (l.length() == 0 || l.startsWith("#")) return true;
    if (l == "exit") { shell_exit_flag = true; return true; }

    // Typed declarations: int/float/string/bool <var> [= <expr>]
    if (l.startsWith("int ")    || l.startsWith("float ") ||
        l.startsWith("string ") || l.startsWith("bool ")) {
        int spIdx  = l.indexOf(' ');
        String type = l.substring(0, spIdx);
        String rest = l.substring(spIdx + 1);
        rest.trim();
        int eqIdx  = rest.indexOf('=');
        String var = (eqIdx != -1) ? rest.substring(0, eqIdx) : rest;
        String rhs = (eqIdx != -1) ? rest.substring(eqIdx + 1) : String("");
        var.trim(); rhs.trim();
        if (type == "int")
            shell_int_vars[var] = eval_int_expr(rhs);
        else if (type == "float")
            shell_float_vars[var] = round2(eval_float_expr(rhs));
        else if (type == "string") {
            rhs = subst_vars(rhs);
            if (rhs.startsWith("\"") && rhs.endsWith("\""))
                rhs = rhs.substring(1, rhs.length() - 1);
            shell_string_vars[var] = rhs;
        } else if (type == "bool") {
            String v = subst_vars(rhs);
            shell_bool_vars[var] = (v == "true" || v == "1");
        }
        return true;
    }

    // read <VAR> — sets string var from input (no-op in non-interactive mode)
    if (l.startsWith("read ")) {
        String var = l.substring(5); var.trim();
        shell_string_vars[var] = "";
        return true;
    }

    // Untyped assignment: <ident> [spaces] = [spaces] <rhs>
    int eqIdx = l.indexOf('=');
    if (eqIdx > 0) {
        String lhs = l.substring(0, eqIdx);
        lhs.trim();
        // Validate LHS is a pure identifier (alphanumeric + underscore, no spaces)
        bool isIdent = lhs.length() > 0;
        for (int j = 0; j < (int)lhs.length() && isIdent; j++) {
            char c = lhs[j];
            if (!isalnum((unsigned char)c) && c != '_') isIdent = false;
        }
        if (isIdent) {
            String var = lhs;
            String rhs = l.substring(eqIdx + 1); rhs.trim();
            if (shell_int_vars.count(var)) {
                shell_int_vars[var] = eval_int_expr(rhs); return true;
            }
            if (shell_float_vars.count(var)) {
                shell_float_vars[var] = round2(eval_float_expr(rhs)); return true;
            }
            if (shell_string_vars.count(var)) {
                rhs = subst_vars(rhs);
                if (rhs.startsWith("\"") && rhs.endsWith("\""))
                    rhs = rhs.substring(1, rhs.length() - 1);
                shell_string_vars[var] = rhs; return true;
            }
            if (shell_bool_vars.count(var)) {
                String v = subst_vars(rhs);
                shell_bool_vars[var] = (v == "true" || v == "1"); return true;
            }
            // Auto-type: use only $var substitution for type detection
            // (bare-name arith subst must not corrupt string literals)
            String sub = subst_vars(rhs); sub.trim();
            if (sub.startsWith("\"") && sub.endsWith("\"")) {
                // Quoted string literal
                shell_string_vars[var] = sub.substring(1, sub.length() - 1);
            } else if (sub == "true" || sub == "false") {
                shell_bool_vars[var] = (sub == "true");
            } else {
                // Numeric? Hint from first char of original rhs
                String rhsT = rhs; rhsT.trim();
                bool mightBeNum = rhsT.length() > 0 &&
                    (isdigit((unsigned char)rhsT[0]) || rhsT[0] == '$' ||
                     rhsT[0] == '-' || rhsT[0] == '+');
                if (mightBeNum && sub.indexOf('.') != -1)
                    shell_float_vars[var] = round2(eval_float_expr(rhs));
                else if (mightBeNum)
                    shell_int_vars[var] = eval_int_expr(rhs);
                else
                    shell_string_vars[var] = sub;
            }
            return true;
        }
    }

    shell_eval_line(l);
    return true;
}

// ─── Block executor (handles all control structures recursively) ───────────────
void shell_execute_block(const std::vector<String> &block) {
    for (size_t i = 0; i < block.size() && !shell_break_flag && !shell_exit_flag && !shell_return_flag; ++i) {
        String line = block[i]; line.trim();
        if (line.length() == 0 || line.startsWith("#")) continue;
        if (line == "break")  { shell_break_flag  = true; return; }
        if (line == "exit")   { shell_exit_flag   = true; return; }
        if (line == "return") { shell_return_flag = true; return; }

        // ── func <name> / endfunc — skip at runtime (collected by first pass) ─
        if (line.startsWith("func ")) {
            int depth = 1;
            while (++i < block.size()) {
                String bl = block[i]; bl.trim();
                if (bl.startsWith("func ")) depth++;
                if (bl == "endfunc" && --depth == 0) break;
            }
            continue;
        }

        // ── call <funcname> [arg1 arg2 ...] ───────────────────────────────────
        if (line.startsWith("call ")) {
            String rest = subst_vars(line.substring(5)); rest.trim();
            int sp = rest.indexOf(' ');
            String fname  = (sp == -1) ? rest : rest.substring(0, sp);
            String argstr = (sp == -1) ? String("") : rest.substring(sp + 1);
            argstr.trim();

            if (!shell_script_funcs.count(fname)) {
                shell_println_wrapped("Fonction inconnue : " + fname);
                continue;
            }

            // Parse args (basic quote support)
            std::vector<String> call_args;
            String cur = "";
            bool inQ = false;
            for (int ci = 0; ci < (int)argstr.length(); ++ci) {
                char c = argstr[ci];
                if (c == '"') { inQ = !inQ; continue; }
                if (c == ' ' && !inQ) {
                    if (cur.length() > 0) { call_args.push_back(cur); cur = ""; }
                } else {
                    cur += c;
                }
            }
            if (cur.length() > 0) call_args.push_back(cur);

            // Save $1..$9 and $argc from outer scope
            std::map<String, int>    savedArgInt;
            std::map<String, String> savedArgStr;
            for (int j = 1; j <= 9; ++j) {
                String k = String(j);
                if (shell_int_vars.count(k))    savedArgInt[k] = shell_int_vars.at(k);
                if (shell_string_vars.count(k)) savedArgStr[k] = shell_string_vars.at(k);
            }
            if (shell_int_vars.count("argc")) savedArgInt["argc"] = shell_int_vars.at("argc");

            // Set $argc and $1..$N for this call
            shell_int_vars["argc"] = (int)call_args.size();
            for (int j = 0; j < (int)call_args.size(); ++j) {
                String k = String(j + 1);
                String a = call_args[j];
                bool isNum = a.length() > 0 &&
                    (isdigit((unsigned char)a[0]) ||
                     (a[0] == '-' && a.length() > 1 && isdigit((unsigned char)a[1])));
                shell_string_vars.erase(k);
                shell_int_vars.erase(k);
                if (isNum) shell_int_vars[k]    = a.toInt();
                else       shell_string_vars[k] = a;
            }

            shell_execute_block(shell_script_funcs.at(fname));
            shell_return_flag = false;  // "return" stops the function, not the caller

            // Restore $1..$9 and $argc
            for (int j = 1; j <= (int)call_args.size(); ++j) {
                String k = String(j);
                shell_int_vars.erase(k);
                shell_string_vars.erase(k);
            }
            shell_int_vars.erase("argc");
            for (auto &kv : savedArgInt) shell_int_vars[kv.first]    = kv.second;
            for (auto &kv : savedArgStr) shell_string_vars[kv.first] = kv.second;
            continue;
        }

        // ── for <var> <start> <end> / endfor ──────────────────────────────────
        if (line.startsWith("for ")) {
            String rest = subst_vars(line.substring(4)); rest.trim();
            int s1 = rest.indexOf(' ');
            int s2 = rest.indexOf(' ', s1 + 1);
            if (s1 < 0 || s2 < 0) continue;
            String forVar   = rest.substring(0, s1);
            String startStr = rest.substring(s1 + 1, s2); startStr.trim();
            String endStr   = rest.substring(s2 + 1);      endStr.trim();
            int forStart = resolve_val(startStr).toInt();
            int forEnd   = resolve_val(endStr).toInt();
            std::vector<String> forBlock;
            int depth = 1;
            while (++i < block.size()) {
                String bl = block[i]; bl.trim();
                if (bl.startsWith("for "))  depth++;
                if (bl.startsWith("endfor")) { if (--depth == 0) break; }
                if (depth > 0) forBlock.push_back(bl);
            }
            for (int j = forStart; j <= forEnd && !shell_exit_flag && !shell_return_flag; ++j) {
                shell_int_vars[forVar] = j;
                shell_execute_block(forBlock);
                if (shell_break_flag) break;
            }
            shell_break_flag = false;
            continue;
        }

        // ── while <cond> / endwhile ───────────────────────────────────────────
        if (line.startsWith("while ")) {
            String cond = line.substring(6); cond.trim();
            std::vector<String> whileBlock;
            int depth = 1;
            while (++i < block.size()) {
                String bl = block[i]; bl.trim();
                if (bl.startsWith("while "))   depth++;
                if (bl.startsWith("endwhile")) { if (--depth == 0) break; }
                if (depth > 0) whileBlock.push_back(bl);
            }
            while (eval_condition(cond) && !shell_exit_flag && !shell_return_flag) {
                shell_execute_block(whileBlock);
                if (shell_break_flag) break;
            }
            shell_break_flag = false;
            continue;
        }

        // ── if <cond> / else / endif ──────────────────────────────────────────
        if (line.startsWith("if ")) {
            String cond = line.substring(3); cond.trim();
            std::vector<String> ifBlock, elseBlock;
            bool inElse = false;
            int depth = 1;
            while (++i < block.size()) {
                String bl = block[i]; bl.trim();
                if (bl.startsWith("if "))   depth++;
                if (bl.startsWith("endif")) { if (--depth == 0) break; }
                if (depth == 0) break;
                if (bl.startsWith("else") && depth == 1) { inElse = true; continue; }
                (inElse ? elseBlock : ifBlock).push_back(bl);
            }
            shell_execute_block(eval_condition(cond) ? ifBlock : elseBlock);
            continue;
        }

        if (line == "endfor" || line == "endwhile" || line == "endif" || line == "else")
            continue;

        shell_process_line(line);
    }
}

// ─── Script entry point ───────────────────────────────────────────────────────
void shell_run(const String &args) {
    String filename = shell_abspath(args); filename.trim();
    if (!LittleFS.exists(filename)) {
        shell_println_wrapped("Script introuvable : " + filename); return;
    }
    File file = LittleFS.open(filename, "r");
    if (!file) {
        shell_println_wrapped("Erreur ouverture : " + filename); return;
    }
    std::vector<String> raw;
    while (file.available()) {
        String l = file.readStringUntil('\n'); l.trim();
        raw.push_back(l);
    }
    file.close();

    // ── First pass: collect func definitions (functions usable before definition)
    shell_script_funcs.clear();
    std::vector<String> lines;
    for (size_t i = 0; i < raw.size(); ++i) {
        String l = raw[i]; l.trim();
        if (l.startsWith("func ")) {
            String fname = l.substring(5); fname.trim();
            std::vector<String> fbody;
            int depth = 1;
            while (++i < raw.size()) {
                String bl = raw[i]; bl.trim();
                if (bl.startsWith("func ")) depth++;
                if (bl == "endfunc" && --depth == 0) break;
                if (depth > 0) fbody.push_back(bl);
            }
            shell_script_funcs[fname] = fbody;
        } else {
            lines.push_back(raw[i]);
        }
    }

    shell_exit_flag   = false;
    shell_break_flag  = false;
    shell_return_flag = false;
    shell_execute_block(lines);
    shell_exit_flag   = false;
    shell_break_flag  = false;
    shell_return_flag = false;
    shell_script_funcs.clear();
}
