#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  printf("\n");
  printf("==========================================\n");
  printf("        HELLO FROM DICRON + MUSL!         \n");
  printf("==========================================\n\n");

  printf("PID: %d\n", getpid());

  char *buf = malloc(1024);
  if (buf) {
    printf("Dynamic Memory Allocation Works! (ptr=%p)\n", buf);
    free(buf);
  }

  printf("Standard I/O and vector sys_writev successful.\n");

  return 0;
}
