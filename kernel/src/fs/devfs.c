#include <dicron/fs.h>
#include <dicron/mem.h>
#include <dicron/io.h>

static long console_write(struct file *f, const char *buf, size_t count)
{
	(void)f;
	for (size_t i = 0; i < count; i++)
		kio_putchar(buf[i]);
	return (long)count;
}

static long console_read(struct file *f, char *buf, size_t count)
{
	(void)f; (void)buf; (void)count;
	return 0; /* EOF for now until keyboard is hooked up */
}

static struct file_operations console_fops = {
	.read = console_read,
	.write = console_write,
	.ioctl = NULL,
	.release = NULL,
};

static struct inode console_inode;

void devfs_init(void)
{
	console_inode.ino = 0;
	console_inode.mode = S_IFCHR | 0666;
	console_inode.f_op = &console_fops;
}

struct file *devfs_get_console(void)
{
	struct file *f = file_alloc();
	if (f) {
		f->f_inode = &console_inode;
		f->f_op = &console_fops;
		f->f_mode = O_RDWR;
	}
	return f;
}
