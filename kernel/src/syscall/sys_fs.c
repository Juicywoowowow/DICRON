#include "lib/string.h"
#include <dicron/fs.h>
#include <dicron/log.h>
#include <dicron/mem.h>
#include <dicron/process.h>
#include <dicron/syscall.h>
#include <dicron/uaccess.h>

#define WRITE_MAX (1024 * 1024)

long sys_read(long fd, long buf_addr, long count, long a3, long a4, long a5) {
  (void)a3;
  (void)a4;
  (void)a5;

  if (count < 0)
    return -EINVAL;
  if (count == 0)
    return 0;

  if (!uaccess_valid((void *)buf_addr, (size_t)count))
    return -EFAULT;

  struct file *f = fd_get((int)fd);
  if (!f)
    return -EBADF;
  if (!(f->f_mode & O_RDWR) && !(f->f_mode == O_RDONLY))
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

  if (count < 0) return -EINVAL;
  if (count == 0) return 0;
  if (count > WRITE_MAX) count = WRITE_MAX;

  if (!uaccess_valid((const void *)buf_addr, (size_t)count)) {
    klog(KLOG_INFO, "sys_write: !uaccess_valid\n");
    return -EFAULT;
  }

  struct file *f = fd_get((int)fd);
  if (!f) {
    klog(KLOG_INFO, "sys_write: fd_get failed for %d\n", (int)fd);
    return -EBADF;
  }
  if (!(f->f_mode & O_RDWR) && !(f->f_mode == O_WRONLY)) {
    klog(KLOG_INFO, "sys_write: mode bad\n");
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

  if (!uaccess_valid((const void *)path_addr, 1))
    return -EFAULT;
  /* Assume safe for now */
  const char *path = (const char *)path_addr;

  struct inode *inode = vfs_namei(path);
  if (!inode) {
    if (flags & O_CREAT) {
      /* Simplistic: assume root directory for now */
      if (!S_ISDIR(vfs_namei("/")->mode))
        return -ENOENT;
      int ret = vfs_namei("/")->i_op->create(vfs_namei("/"), path, (int)mode);
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
	if (iovcnt < 0 || iovcnt > 1024) return -EINVAL;
	if (iovcnt == 0) return 0;
	if (!uaccess_valid((void *)iov_addr, (size_t)iovcnt * sizeof(struct iovec)))
		return -EFAULT;
	struct iovec *iov = (struct iovec *)iov_addr;
	long total = 0;
	for (long i = 0; i < iovcnt; i++) {
		long ret = sys_read(fd, (long)iov[i].iov_base, (long)iov[i].iov_len, a3, a4, a5);
		if (ret < 0) {
			if (total > 0) return total;
			return ret;
		}
		total += ret;
		if ((size_t)ret < iov[i].iov_len) break; /* short read */
	}
	return total;
}

long sys_writev(long fd, long iov_addr, long iovcnt, long a3, long a4, long a5) {
	if (iovcnt < 0 || iovcnt > 1024) return -EINVAL;
	if (iovcnt == 0) return 0;
	if (!uaccess_valid((void *)iov_addr, (size_t)iovcnt * sizeof(struct iovec))) {
		return -EFAULT;
	}
	struct iovec *iov = (struct iovec *)iov_addr;
	long total = 0;
	for (long i = 0; i < iovcnt; i++) {
		long ret = sys_write(fd, (long)iov[i].iov_base, (long)iov[i].iov_len, a3, a4, a5);
		if (ret < 0) {
			if (total > 0) return total;
			return ret;
		}
		total += ret;
		if ((size_t)ret < iov[i].iov_len) break; /* short write */
	}
	return total;
}

long sys_ioctl(long fd, long cmd, long arg, long a3, long a4, long a5) {
	(void)cmd; (void)arg; (void)a3; (void)a4; (void)a5;
	struct file *f = fd_get((int)fd);
	if (!f) return -EBADF;
	/* We don't support TTY ioctls yet, musl handles ENOTTY smoothly */
	return -ENOTTY;
}
