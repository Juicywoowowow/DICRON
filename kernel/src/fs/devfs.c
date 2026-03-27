#include <dicron/fs.h>
#include <dicron/io.h>
#include <dicron/mem.h>


static long console_write(struct file *f, const char *buf, size_t count) {
  (void)f;
  for (size_t i = 0; i < count; i++)
    kio_putchar(buf[i]);
  return (long)count;
}

static long console_read(struct file *f, char *buf, size_t count) {
  (void)f;
  if (count == 0)
    return 0;

  size_t read_bytes = 0;
  while (read_bytes < count) {
    int c = kio_getchar();
    if (c < 0) {
      if (read_bytes > 0)
        break;
      return c; /* Error or EOF signal */
    }

    if (c == '\b' || c == 0x7F) {
      if (read_bytes > 0) {
        read_bytes--;
        kio_puts("\b \b"); /* erase on screen */
      }
      continue;
    }

    /* Echo character back to the screen */
    kio_putchar(c);

    /* Map carriage return to newline */
    if (c == '\r') {
      c = '\n';
      kio_putchar('\n');
    }

    buf[read_bytes++] = (char)c;

    /* Return immediately after a newline (line-buffered behavior) */
    if (c == '\n' || c == '\004') { /* ^D EOF */
      if (c == '\004')
        read_bytes--;
      break;
    }
  }

  return (long)read_bytes;
}

static struct file_operations console_fops = {
    .read = console_read,
    .write = console_write,
    .ioctl = NULL,
    .release = NULL,
};

static struct inode console_inode;

void devfs_init(void) {
  console_inode.ino = 0;
  console_inode.mode = S_IFCHR | 0666;
  console_inode.f_op = &console_fops;
}

struct file *devfs_get_console(void) {
  struct file *f = file_alloc();
  if (f) {
    f->f_inode = &console_inode;
    f->f_op = &console_fops;
    f->f_mode = O_RDWR;
  }
  return f;
}
