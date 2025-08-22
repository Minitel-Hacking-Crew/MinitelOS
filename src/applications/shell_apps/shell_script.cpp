#include "globals.h"

// Dépendances globales externes
extern std::map<String, int> shell_int_vars;
extern std::map<String, float> shell_float_vars;
extern std::map<String, String> shell_string_vars;
extern std::map<String, bool> shell_bool_vars;
extern std::map<String, String> shell_vars;
extern int numCommands;
extern bool shell_break_flag;
extern void shell_println_wrapped(const String &text);
extern void write_to_file(const String &filename, const String &content, bool append);
extern String shell_abspath(const String &path);
extern String shell_output_buffer;
extern bool shell_redirect_mode;
extern String shell_current_dir;
extern float round2(float);
extern ShellCommand commands[];

void shell_run(const String &args)
{
    String filename = shell_abspath(args);
    filename.trim();
    if (!LittleFS.exists(filename))
    {
        shell_println_wrapped("Script introuvable : " + filename);
        return;
    }
    File file = LittleFS.open(filename, "r");
    if (!file)
    {
        shell_println_wrapped("Erreur ouverture : " + filename);
        return;
    }
    while (file.available())
    {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() == 0 || line.startsWith("#"))
            continue;
        if (line.startsWith("for "))
        {
            String rest = line.substring(4);
            rest.trim();
            int s1 = rest.indexOf(' ');
            int s2 = rest.indexOf(' ', s1 + 1);
            if (s1 < 0 || s2 < 0)
                continue;
            String var = rest.substring(0, s1);
            int start = rest.substring(s1 + 1, s2).toInt();
            int end = rest.substring(s2 + 1).toInt();
            std::vector<String> forBlock;
            int depth = 1;
            while (file.available())
            {
                String bline = file.readStringUntil('\n');
                bline.trim();
                if (bline.startsWith("for "))
                    depth++;
                if (bline.startsWith("endfor"))
                    depth--;
                if (depth == 0)
                    break;
                forBlock.push_back(bline);
            }
            for (int i = start; i <= end; ++i)
            {
                shell_int_vars[var] = i;
                shell_execute_block(forBlock);
                if (shell_break_flag)
                    break;
            }
            shell_break_flag = false;
            continue;
        }
        if (line.startsWith("while "))
        {
            String cond = line.substring(6);
            cond.trim();
            std::vector<String> whileBlock;
            int depth = 1;
            while (file.available())
            {
                String bline = file.readStringUntil('\n');
                bline.trim();
                if (bline.startsWith("while "))
                    depth++;
                if (bline.startsWith("endwhile"))
                    depth--;
                if (depth == 0)
                    break;
                whileBlock.push_back(bline);
            }
            while (true)
            {
                bool condResult = false;
                int firstSpace = cond.indexOf(' ');
                int lastSpace = cond.lastIndexOf(' ');
                if (firstSpace > 0 && lastSpace > firstSpace)
                {
                    String var = cond.substring(0, firstSpace);
                    String op = cond.substring(firstSpace + 1, lastSpace);
                    String val = cond.substring(lastSpace + 1);
                    var.trim();
                    op.trim();
                    val.trim();
                    int varInt = shell_int_vars.count(var) ? shell_int_vars[var] : 0;
                    int valInt = val.toInt();
                    if (op == "eq")
                        condResult = (varInt == valInt);
                    else if (op == "gt")
                        condResult = (varInt > valInt);
                    else if (op == "lt")
                        condResult = (varInt < valInt);
                }
                if (!condResult || shell_break_flag)
                    break;
                shell_execute_block(whileBlock);
                if (shell_break_flag)
                    break;
            }
            shell_break_flag = false;
            continue;
        }
        if (line.startsWith("if "))
        {
            String cond = line.substring(3);
            cond.trim();
            std::vector<String> ifBlock, elseBlock;
            bool inElse = false;
            int depth = 1;
            while (file.available())
            {
                String bline = file.readStringUntil('\n');
                bline.trim();
                if (bline.startsWith("if "))
                    depth++;
                if (bline.startsWith("endif"))
                    depth--;
                if (depth == 0)
                    break;
                if (bline.startsWith("else") && depth == 1)
                {
                    inElse = true;
                    continue;
                }
                if (inElse)
                    elseBlock.push_back(bline);
                else
                    ifBlock.push_back(bline);
            }
            bool condResult = false;
            int firstSpace = cond.indexOf(' ');
            int lastSpace = cond.lastIndexOf(' ');
            if (firstSpace > 0 && lastSpace > firstSpace)
            {
                String var = cond.substring(0, firstSpace);
                String op = cond.substring(firstSpace + 1, lastSpace);
                String val = cond.substring(lastSpace + 1);
                var.trim();
                op.trim();
                val.trim();
                int varInt = shell_int_vars.count(var) ? shell_int_vars[var] : 0;
                int valInt = val.toInt();
                if (op == "eq")
                    condResult = (varInt == valInt);
                else if (op == "gt")
                    condResult = (varInt > valInt);
                else if (op == "lt")
                    condResult = (varInt < valInt);
            }
            shell_execute_block(condResult ? ifBlock : elseBlock);
            continue;
        }
        if (line == "endfor" || line == "endwhile" || line == "endif" || line == "else")
            continue;
        shell_process_line(line);
    }
    file.close();
    // if (!shell_redirect_mode)
    //     minitel.println();
}

void shell_eval_line(const String &line)
{
    String processedLine = line;
    for (auto &kv : shell_int_vars)
        processedLine.replace("$" + kv.first, String(kv.second));
    for (auto &kv : shell_float_vars)
    {
        char buf[16];
        dtostrf(kv.second, 0, 2, buf);
        processedLine.replace("$" + kv.first, String(buf));
    }
    for (auto &kv : shell_string_vars)
        processedLine.replace("$" + kv.first, kv.second);
    for (auto &kv : shell_bool_vars)
        processedLine.replace("$" + kv.first, kv.second ? "true" : "false");
    int redirectIdx = -1;
    int appendIdx = processedLine.indexOf(">>");
    bool appendMode = false;
    if (appendIdx != -1)
    {
        redirectIdx = appendIdx;
        appendMode = true;
    }
    else
    {
        redirectIdx = processedLine.indexOf('>');
    }
    String redirectFile = "";
    String realCmdLine = processedLine;
    if (redirectIdx != -1)
    {
        realCmdLine = processedLine.substring(0, redirectIdx);
        redirectFile = processedLine.substring(redirectIdx + (appendMode ? 2 : 1));
        realCmdLine.trim();
        redirectFile.trim();
    }
    int spaceIdx = realCmdLine.indexOf(' ');
    String cmd = (spaceIdx == -1) ? realCmdLine : realCmdLine.substring(0, spaceIdx);
    String args = (spaceIdx == -1) ? "" : realCmdLine.substring(spaceIdx + 1);
    String prev_output_buffer = shell_output_buffer;
    bool prev_redirect_mode = shell_redirect_mode;
    String line_output_buffer = "";
    if (redirectFile.length() > 0 || prev_redirect_mode)
    {
        shell_output_buffer = "";
        shell_redirect_mode = true;
    }
    bool found = false;
    for (int i = 0; i < numCommands; ++i)
    {
        if (cmd == commands[i].name)
        {
            commands[i].func(args);
            found = true;
            break;
        }
    }
    if (!found && realCmdLine.length() > 0)
    {
        shell_println_wrapped("Commande inconnue : " + cmd);
    }
    if (redirectFile.length() > 0)
    {
        String absRedirectFile = shell_abspath(redirectFile);
        write_to_file(absRedirectFile, shell_output_buffer, appendMode);
        shell_redirect_mode = false;
    }
    else if (prev_redirect_mode)
    {
        prev_output_buffer += shell_output_buffer;
        shell_output_buffer = prev_output_buffer;
        shell_redirect_mode = true;
    }
}
bool shell_process_line(const String &line)
{
    String l = line;
    l.trim();
    if (l.length() == 0 || l.startsWith("#"))
        return true;
    if (l.startsWith("int ") || l.startsWith("float ") || l.startsWith("string ") || l.startsWith("bool "))
    {
        int eqIdx = l.indexOf('=');
        String type, var, val;
        if (eqIdx != -1)
        {
            type = l.substring(0, l.indexOf(' '));
            var = l.substring(l.indexOf(' ') + 1, eqIdx);
            val = l.substring(eqIdx + 1);
        }
        else
        {
            type = l.substring(0, l.indexOf(' '));
            var = l.substring(l.indexOf(' ') + 1);
            val = "";
        }
        var.trim();
        val.trim();
        if (type == "int")
            shell_int_vars[var] = val.toInt();
        else if (type == "float")
            shell_float_vars[var] = round2(val.toFloat());
        else if (type == "string")
        {
            if (val.startsWith("\"") && val.endsWith("\""))
                val = val.substring(1, val.length() - 1);
            shell_string_vars[var] = val;
        }
        else if (type == "bool")
            shell_bool_vars[var] = (val == "true" || val == "1");
        return true;
    }
    int eqIdx = l.indexOf('=');
    if (eqIdx > 0)
    {
        String var = l.substring(0, eqIdx);
        String expr = l.substring(eqIdx + 1);
        var.trim();
        expr.trim();
        for (auto &kv : shell_int_vars)
            expr.replace(kv.first, String(kv.second));
        for (auto &kv : shell_float_vars)
        {
            char buf[16];
            dtostrf(kv.second, 0, 2, buf);
            expr.replace(kv.first, String(buf));
        }
        for (auto &kv : shell_string_vars)
            expr.replace(kv.first, kv.second);
        for (auto &kv : shell_bool_vars)
            expr.replace(kv.first, kv.second ? "true" : "false");
        if (shell_int_vars.count(var))
        {
            int res = shell_int_vars[var];
            char op = 0;
            int opIdx = -1;
            for (int i = 0; i < expr.length(); ++i)
            {
                if (expr[i] == '+' || expr[i] == '-' || expr[i] == '*' || expr[i] == '/' || expr[i] == '%')
                {
                    op = expr[i];
                    opIdx = i;
                    break;
                }
            }
            if (opIdx != -1)
            {
                int right = expr.substring(opIdx + 1).toInt();
                switch (op)
                {
                case '+':
                    res += right;
                    break;
                case '-':
                    res -= right;
                    break;
                case '*':
                    res *= right;
                    break;
                case '/':
                    if (right != 0)
                        res /= right;
                    break;
                case '%':
                    if (right != 0)
                        res %= right;
                    break;
                }
                shell_int_vars[var] = res;
            }
            else
            {
                shell_int_vars[var] = expr.toInt();
            }
            return true;
        }
        if (shell_float_vars.count(var))
        {
            float res = shell_float_vars[var];
            char op = 0;
            int opIdx = -1;
            for (int i = 0; i < expr.length(); ++i)
            {
                if (expr[i] == '+' || expr[i] == '-' || expr[i] == '*' || expr[i] == '/')
                {
                    op = expr[i];
                    opIdx = i;
                    break;
                }
            }
            if (opIdx != -1)
            {
                float right = expr.substring(opIdx + 1).toFloat();
                switch (op)
                {
                case '+':
                    res += right;
                    break;
                case '-':
                    res -= right;
                    break;
                case '*':
                    res *= right;
                    break;
                case '/':
                    if (right != 0.0f)
                        res /= right;
                    break;
                }
                shell_float_vars[var] = round2(res);
            }
            else
            {
                shell_float_vars[var] = round2(expr.toFloat());
            }
            return true;
        }
        if (shell_string_vars.count(var))
        {
            String v = expr;
            v.trim();
            int plusIdx = v.indexOf('+');
            if (plusIdx > 0)
            {
                String left = v.substring(0, plusIdx);
                String right = v.substring(plusIdx + 1);
                left.trim();
                right.trim();
                String leftVal = shell_string_vars.count(left) ? shell_string_vars[left] : left;
                if (right.startsWith("\"") && right.endsWith("\""))
                    right = right.substring(1, right.length() - 1);
                shell_string_vars[var] = leftVal + right;
            }
            else
            {
                bool isCmd = false;
                for (int i = 0; i < numCommands; ++i)
                {
                    if (v == commands[i].name)
                    {
                        isCmd = true;
                        shell_output_buffer = "";
                        shell_redirect_mode = true;
                        commands[i].func("");
                        shell_string_vars[var] = shell_output_buffer;
                        shell_redirect_mode = false;
                        break;
                    }
                }
                if (!isCmd)
                {
                    if (v.startsWith("\"") && v.endsWith("\""))
                        v = v.substring(1, v.length() - 1);
                    shell_string_vars[var] = v;
                }
            }
            return true;
        }
        if (shell_bool_vars.count(var))
        {
            bool v = (expr == "true" || expr == "1");
            shell_bool_vars[var] = v;
            return true;
        }
    }
    shell_eval_line(l);
    return true;
}
void shell_execute_block(const std::vector<String> &block)
{
    for (size_t i = 0; i < block.size(); ++i)
    {
        String line = block[i];
        line.trim();
        if (line.length() == 0 || line.startsWith("#"))
            continue;
        if (line.startsWith("for "))
        {
            String rest = line.substring(4);
            rest.trim();
            int s1 = rest.indexOf(' ');
            int s2 = rest.indexOf(' ', s1 + 1);
            if (s1 < 0 || s2 < 0)
                continue;
            String var = rest.substring(0, s1);
            int start = rest.substring(s1 + 1, s2).toInt();
            int end = rest.substring(s2 + 1).toInt();
            std::vector<String> forBlock;
            int depth = 1;
            while (++i < block.size())
            {
                String bline = block[i];
                bline.trim();
                if (bline.startsWith("for "))
                    depth++;
                if (bline.startsWith("endfor"))
                    depth--;
                if (depth == 0)
                    break;
                forBlock.push_back(bline);
            }
            for (int j = start; j <= end; ++j)
            {
                shell_int_vars[var] = j;
                shell_execute_block(forBlock);
                if (shell_break_flag)
                    break;
            }
            shell_break_flag = false;
            continue;
        }
        if (line.startsWith("while "))
        {
            String cond = line.substring(6);
            cond.trim();
            std::vector<String> whileBlock;
            int depth = 1;
            while (++i < block.size())
            {
                String bline = block[i];
                bline.trim();
                if (bline.startsWith("while "))
                    depth++;
                if (bline.startsWith("endwhile"))
                    depth--;
                if (depth == 0)
                    break;
                whileBlock.push_back(bline);
            }
            while (true)
            {
                bool condResult = false;
                int firstSpace = cond.indexOf(' ');
                int lastSpace = cond.lastIndexOf(' ');
                if (firstSpace > 0 && lastSpace > firstSpace)
                {
                    String var = cond.substring(0, firstSpace);
                    String op = cond.substring(firstSpace + 1, lastSpace);
                    String val = cond.substring(lastSpace + 1);
                    var.trim();
                    op.trim();
                    val.trim();
                    int varInt = shell_int_vars.count(var) ? shell_int_vars[var] : 0;
                    int valInt = val.toInt();
                    if (op == "eq")
                        condResult = (varInt == valInt);
                    else if (op == "gt")
                        condResult = (varInt > valInt);
                    else if (op == "lt")
                        condResult = (varInt < valInt);
                }
                if (!condResult || shell_break_flag)
                    break;
                shell_execute_block(whileBlock);
                if (shell_break_flag)
                    break;
            }
            shell_break_flag = false;
            continue;
        }
        if (line.startsWith("if "))
        {
            String cond = line.substring(3);
            cond.trim();
            std::vector<String> ifBlock, elseBlock;
            bool inElse = false;
            int depth = 1;
            while (++i < block.size())
            {
                String bline = block[i];
                bline.trim();
                if (bline.startsWith("if "))
                    depth++;
                if (bline.startsWith("endif"))
                    depth--;
                if (depth == 0)
                    break;
                if (bline.startsWith("else") && depth == 1)
                {
                    inElse = true;
                    continue;
                }
                if (inElse)
                    elseBlock.push_back(bline);
                else
                    ifBlock.push_back(bline);
            }
            bool condResult = false;
            int firstSpace = cond.indexOf(' ');
            int lastSpace = cond.lastIndexOf(' ');
            if (firstSpace > 0 && lastSpace > firstSpace)
            {
                String var = cond.substring(0, firstSpace);
                String op = cond.substring(firstSpace + 1, lastSpace);
                String val = cond.substring(lastSpace + 1);
                var.trim();
                op.trim();
                val.trim();
                int varInt = shell_int_vars.count(var) ? shell_int_vars[var] : 0;
                int valInt = val.toInt();
                if (op == "eq")
                    condResult = (varInt == valInt);
                else if (op == "gt")
                    condResult = (varInt > valInt);
                else if (op == "lt")
                    condResult = (varInt < valInt);
            }
            shell_execute_block(condResult ? ifBlock : elseBlock);
            continue;
        }
        if (line == "endfor" || line == "endwhile" || line == "endif" || line == "else")
            continue;
        shell_process_line(line);
        if (shell_break_flag)
            break;
    }
}
float round2(float val)
{
    return roundf(val * 100.0f) / 100.0f;
}