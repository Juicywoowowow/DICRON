#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/mman.h>
#include <sys/time.h>

static int pass_count = 0;
static int fail_count = 0;

static void check(const char *name, int ok) {
  if (ok) {
    printf("  [PASS] %s\n", name);
    pass_count++;
  } else {
    printf("  [FAIL] %s\n", name);
    fail_count++;
  }
}

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  printf("\n");
  printf("==========================================\n");
  printf("        HELLO FROM DICRON + MUSL!         \n");
  printf("==========================================\n\n");

  /* --- getpid (already tested) --- */
  printf("[test] getpid\n");
  pid_t pid = getpid();
  printf("  PID = %d\n", pid);
  check("getpid returns > 0", pid > 0);

  /* --- malloc / brk (already tested) --- */
  printf("[test] malloc (brk)\n");
  char *buf = malloc(1024);
  check("malloc returned non-NULL", buf != NULL);
  if (buf) free(buf);

  /* --- uname --- */
  printf("[test] uname\n");
  struct utsname uts;
  int rc = uname(&uts);
  check("uname returns 0", rc == 0);
  if (rc == 0) {
    printf("  sysname  = %s\n", uts.sysname);
    printf("  release  = %s\n", uts.release);
    printf("  machine  = %s\n", uts.machine);
  }

  /* --- sched_yield --- */
  printf("[test] sched_yield\n");
  rc = sched_yield();
  check("sched_yield returns 0", rc == 0);

  /* --- mmap + munmap --- */
  printf("[test] mmap / munmap\n");
  size_t map_size = 4096;
  void *addr = mmap(NULL, map_size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  check("mmap returned valid pointer", addr != MAP_FAILED);
  if (addr != MAP_FAILED) {
    memset(addr, 0xAB, map_size);
    unsigned char *p = (unsigned char *)addr;
    int data_ok = (p[0] == 0xAB) && (p[4095] == 0xAB);
    check("write to mmap region", data_ok);
    rc = munmap(addr, map_size);
    check("munmap returns 0", rc == 0);
  }

  /* --- open / write / close / open / read / close --- */
  printf("[test] open / write / read / close\n");
  const char *path = "/dicron_test.txt";
  const char *msg = "DICRON VFS works!";
  size_t msg_len = strlen(msg);

  int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
  check("open(O_CREAT|O_WRONLY) >= 0", fd >= 0);
  if (fd >= 0) {
    ssize_t nw = write(fd, msg, msg_len);
    check("write returns correct length", (size_t)nw == msg_len);
    close(fd);

    char rbuf[64];
    memset(rbuf, 0, sizeof(rbuf));
    fd = open(path, O_RDONLY);
    check("open(O_RDONLY) >= 0", fd >= 0);
    if (fd >= 0) {
      ssize_t nr = read(fd, rbuf, sizeof(rbuf) - 1);
      check("read returns correct length", (size_t)nr == msg_len);
      check("read-back data matches", memcmp(rbuf, msg, msg_len) == 0);
      close(fd);
    }
  }

  /* --- gettimeofday --- */
  printf("[test] gettimeofday\n");
  struct timeval tv;
  rc = gettimeofday(&tv, NULL);
  check("gettimeofday returns 0", rc == 0);
  if (rc == 0) {
    printf("  tv_sec  = %ld\n", (long)tv.tv_sec);
    printf("  tv_usec = %ld\n", (long)tv.tv_usec);
  }

  /* --- multi-file write + read-back --- */
  printf("[test] multi-file write + read\n");
  const char *files[] = { "/file_a.txt", "/file_b.txt", "/file_c.txt" };
  const char *data[]  = { "Alpha", "Bravo", "Charlie" };
  int file_count = 3;

  for (int i = 0; i < file_count; i++) {
    fd = open(files[i], O_CREAT | O_WRONLY, 0644);
    if (fd >= 0) {
      write(fd, data[i], strlen(data[i]));
      close(fd);
    }
  }

  int multi_ok = 1;
  for (int i = 0; i < file_count; i++) {
    char rbuf2[64];
    memset(rbuf2, 0, sizeof(rbuf2));
    fd = open(files[i], O_RDONLY);
    if (fd < 0) { multi_ok = 0; continue; }
    ssize_t n = read(fd, rbuf2, sizeof(rbuf2) - 1);
    close(fd);
    if (n != (ssize_t)strlen(data[i]) ||
        memcmp(rbuf2, data[i], strlen(data[i])) != 0)
      multi_ok = 0;
  }
  check("3 files written and read back correctly", multi_ok);

  /* --- overwrite + re-read --- */
  printf("[test] overwrite + re-read\n");
  fd = open("/file_a.txt", O_WRONLY);
  if (fd >= 0) {
    const char *new_msg = "Overwritten!";
    write(fd, new_msg, strlen(new_msg));
    close(fd);

    char rbuf3[64];
    memset(rbuf3, 0, sizeof(rbuf3));
    fd = open("/file_a.txt", O_RDONLY);
    if (fd >= 0) {
      ssize_t n = read(fd, rbuf3, sizeof(rbuf3) - 1);
      close(fd);
      check("overwrite read-back matches", n > 0 &&
            memcmp(rbuf3, "Overwritten!", 12) == 0);
    } else {
      check("overwrite re-open", 0);
    }
  } else {
    check("overwrite open", 0);
  }

  /* --- summary --- */
  printf("\n==========================================\n");
  printf("  Results: %d passed, %d failed\n", pass_count, fail_count);
  printf("==========================================\n\n");

  return fail_count > 0 ? 1 : 0;
}
