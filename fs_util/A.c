#include "userlib.h"

#define K2023202316_ID "2023202316"
#define K2023202316_NAME "wangyihang"

int main(int argc, char *argv[]) {
  int32 x;
  int32 i;

  x = 2023202316;
  u2023202316_printf("A: &x=0x%08X, &main=0x%08X\n", &x, &main);
  for (i = 0; i < argc; i++) {
    u2023202316_printf("arg-%d: %s\n", i, argv[i]);
  }
  u2023202316_printf("%s %s\n", K2023202316_ID, K2023202316_NAME);
  return 0;
}
