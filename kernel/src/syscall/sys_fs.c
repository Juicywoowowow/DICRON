#include "lib/string.h"
#include <dicron/fs.h>
#include <dicron/log.h>
#include <dicron/mem.h>
#include <dicron/process.h>
#include <dicron/syscall.h>
#include <dicron/uaccess.h>

#define WRITE_MAX (1024 * 1024)

#define READ_MAX (1024 * 1024)

long sys_read(long fd, long buf_addr, long count, long a3, long a4, long a5) {
  (void)a3;
  (void)a4;
  (void)a5;

  if (count < 0)
    return -EINVAL;
  if (count == 0)
    return 0;
  if (count > READ_MAX)
    count = READ_MAX;

  if (!uaccess_valid((void *)buf_addr, (size_t)count))
    return -EFAULT;

  struct file *f = fd_get((int)fd);
  if (!f)
    return -EBADF;
  uint32_t accmode = f->f_mode & (O_RDONLY | O_WRONLY | O_RDWR);
  if (accmode != O_RDONLY && accmode != O_RDWR)
    return -EBADF;

  if (!f->f_op || !f->f_op->read)
    return -EINVAL;

  long ret = f->f_op->read(f, (char *)buf_addr, (size_t)count);
  return ret;
}

long sys_write(long fd, long buf_addr, long count, long a3, long a4, long a5) {
  (void)a3;
  (void)a4;
  (void)a5;

  if (count < 0)
    return -EINVAL;
  if (count == 0)
    return 0;
  if (count > WRITE_MAX)
    count = WRITE_MAX;

  if (!uaccess_valid((const void *)buf_addr, (size_t)count)) {
    klog(KLOG_DEBUG, "sys_write: !uaccess_valid\n");
    return -EFAULT;
  }

  struct file *f = fd_get((int)fd);
  if (!f) {
    klog(KLOG_DEBUG, "sys_write: fd_get failed for %d\n", (int)fd);
    return -EBADF;
  }
  uint32_t waccmode = f->f_mode & (O_RDONLY | O_WRONLY | O_RDWR);
  if (waccmode != O_WRONLY && waccmode != O_RDWR) {
    klog(KLOG_DEBUG, "sys_write: mode bad\n");
    return -EBADF;
  }

  if (!f->f_op || !f->f_op->write) {
    return -EINVAL;
  }

  long ret = f->f_op->write(f, (const char *)buf_addr, (size_t)count);
  return ret;
}

long sys_open(long path_addr, long flags, long mode, long a3, long a4,
              long a5) {
  (void)a3;
  (void)a4;
  (void)a5;

  if (!uaccess_valid_string((const char *)path_addr, 4096))
    return -EFAULT;
  
  char kpath[4096];
  const char *u_path = (const char *)path_addr;
  size_t klimit = 4095;
  size_t i = 0;
  while (i < klimit && u_path[i] != '\0') {
    kpath[i] = u_path[i];
    i++;
  }
  kpath[i] = '\0';
  const char *path = kpath;

  struct inode *inode = vfs_namei(path);
  if (!inode) {
    if (flags & O_CREAT) {
      int ret = vfs_create(path, (int)mode);
      if (ret < 0)
        return ret;
      inode = vfs_namei(path);
    }
    if (!inode)
      return -ENOENT;
  }

  struct file *f = file_alloc();
  if (!f)
    return -ENFILE;

  f->f_inode = inode;
  f->f_op = inode->f_op;
  f->f_mode = (uint32_t)flags;
  f->f_pos = 0;
  if (flags & O_APPEND)
    f->f_pos = inode->size;

  int fd = fd_install(f);
  if (fd < 0) {
    file_put(f);
    return -EMFILE;
  }

  return fd;
}

long sys_close(long fd, long a1, long a2, long a3, long a4, long a5) {
  (void)a1;
  (void)a2;
  (void)a3;
  (void)a4;
  (void)a5;
  return fd_close((int)fd);
}

struct iovec {
  void *iov_base;
  size_t iov_len;
};

long sys_readv(long fd, long iov_addr, long iovcnt, long a3, long a4, long a5) {
  if (iovcnt < 0 || iovcnt > 128)
    return -EINVAL;
  if (iovcnt == 0)
    return 0;
  
  struct iovec kiov[128];
  if (copy_from_user(kiov, (void *)iov_addr, (size_t)iovcnt * sizeof(struct iovec)) < 0)
    return -EFAULT;

  long total = 0;
  for (long i = 0; i < iovcnt; i++) {
    long ret = sys_read(fd, (long)kiov[i].iov_base, (long)kiov[i].iov_len, a3, a4, a5);
    if (ret < 0) {
      if (total > 0)
        return total;
      return ret;
    }
    total += ret;
    if ((size_t)ret < kiov[i].iov_len)
      break; /* short read */
  }
  return total;
}

long sys_writev(long fd, long iov_addr, long iovcnt, long a3, long a4,
                long a5) {
  if (iovcnt < 0 || iovcnt > 128)
    return -EINVAL;
  if (iovcnt == 0)
    return 0;
  
  struct iovec kiov[128];
  if (copy_from_user(kiov, (void *)iov_addr, (size_t)iovcnt * sizeof(struct iovec)) < 0)
    return -EFAULT;

  long total = 0;
  for (long i = 0; i < iovcnt; i++) {
    long ret = sys_write(fd, (long)kiov[i].iov_base, (long)kiov[i].iov_len, a3, a4, a5);
    if (ret < 0) {
      if (total > 0)
        return total;
      return ret;
    }
    total += ret;
    if ((size_t)ret < kiov[i].iov_len)
      break; /* short write */
  }
  return total;
}

long sys_ioctl(long fd, long cmd, long arg, long a3, long a4, long a5) {
  (void)arg;
  (void)a3;
  (void)a4;
  (void)a5;
  struct file *f = fd_get((int)fd);
  if (!f)
    return -EBADF;

  /* 0x5401 is TCGETS, 0x5413 is TIOCGWINSZ
   * Faking success for standard TTY ioctls allows isatty() to return true */
  if ((cmd & 0xff00) == 0x5400) {
    return 0;
  }

  return -ENOTTY;
}

long sys_openat(long dirfd, long path_addr, long flags, long mode, long a4,
                long a5) {
  (void)a4;
  (void)a5;
  /* AT_FDCWD (-100) means use current working directory (treat as absolute) */
  (void)dirfd;
  return sys_open(path_addr, flags, mode, 0, 0, 0);
}
