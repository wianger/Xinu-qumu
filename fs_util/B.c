#include "userlib.h"

int main(int argc, char *argv[]) {
  char msg[] = "Lab6-B wrote this text.\n";

  if (argc < 2) {
    u2023202316_printf("B: need target file\n");
    return -1;
  }
  u2023202316_writefile(argv[1], msg, sizeof(msg) - 1);
  u2023202316_printf("B: wrote %d bytes to %s\n", sizeof(msg) - 1, argv[1]);
  return 0;
}
