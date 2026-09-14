#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#define NOB_EXPERIMENTAL_DELETE_OLD
#include "thirdparty/nob.h"

#define BUILD_FOLDER "build/"
#define SRC_FOLDER "src/"

int main(int argc, char **argv) {
  GO_REBUILD_URSELF(argc, argv);

  if (!mkdir_if_not_exists(BUILD_FOLDER))
    return 1;
  Cmd cmd = {0};

  nob_cc(&cmd);
  nob_cc_flags(&cmd);
  nob_cc_output(&cmd, BUILD_FOLDER "azote");
  nob_cc_inputs(&cmd, SRC_FOLDER "main.c");
  // cmd_append(&cmd, "-O2");
  // cmd_append(&cmd, "-ggdb");

  cmd_append(&cmd, "-lm");
  cmd_append(&cmd, "-lncurses");
  cmd_append(&cmd, "-lX11", "-lXinerama");

  if (!nob_cmd_run(&cmd))
    return 1;

  return 0;
}
