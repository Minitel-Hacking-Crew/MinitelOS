Import("env")
import os

sim_src_dir = os.path.join(env.subst("$PROJECT_DIR"), "sim", "src")
sim_build_dir = os.path.join("$BUILD_DIR", "sim_src")

# BuildLibrary compiles and creates a static lib that gets linked
env.BuildLibrary(sim_build_dir, sim_src_dir)
