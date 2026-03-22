/*  main.c  - main */

#include <xinu.h>

void sndA(void) {
  while (1) {
    fputs("A\n", CONSOLE);
  }
}
void sndB(void) {
  while (1) {
    fputs("B\n", CONSOLE);
  }
}

void main(void) {
  resume(create(sndA, 1024, 30, "process 1", 0));
  resume(create(sndB, 1024, 30, "process 2", 0));
}
