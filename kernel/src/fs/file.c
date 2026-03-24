#include <dicron/fs.h>
#include <dicron/process.h>
#include <dicron/mem.h>
#include <dicron/log.h>
#include <stddef.h>

/* Allocates a file struct */
struct file *file_alloc(void)
{
	struct file *f = kzalloc(sizeof(struct file));
	if (f)
		f->f_count = 1;
	return f;
}

/* Increments reference count */
void file_get(struct file *f)
{
	if (f)
		f->f_count++; /* TODO: atomic */
}

/* Decrements reference count, frees if 0 */
void file_put(struct file *f)
{
	if (!f) return;
	f->f_count--;
	if (f->f_count <= 0) {
		if (f->f_op && f->f_op->release)
			f->f_op->release(f->f_inode, f);
		kfree(f);
	}
}

/* Gets the file from current process */
struct file *fd_get(int fd)
{
	if (fd < 0 || fd >= MAX_FDS) return NULL;
	struct process *proc = process_current();
	if (!proc) return NULL;
	return proc->fds[fd];
}

/* Installs a file into the first available FD */
int fd_install(struct file *f)
{
	struct process *proc = process_current();
	if (!proc) return -1;
	
	for (int i = 0; i < MAX_FDS; i++) {
		if (!proc->fds[i]) {
			proc->fds[i] = f;
			return i;
		}
	}
	return -1;
}

/* Closes a file descriptor */
int fd_close(int fd)
{
	if (fd < 0 || fd >= MAX_FDS) return -1;
	struct process *proc = process_current();
	if (!proc) return -1;
	
	struct file *f = proc->fds[fd];
	if (!f) return -1;
	
	proc->fds[fd] = NULL;
	file_put(f);
	return 0;
}

void process_fd_init(void *process_ptr)
{
	struct process *proc = (struct process *)process_ptr;
	for (int i = 0; i < MAX_FDS; i++)
		proc->fds[i] = NULL;
		
	struct file *console = devfs_get_console();
	if (console) {
		proc->fds[0] = console; file_get(console);
		proc->fds[1] = console; file_get(console);
		proc->fds[2] = console; file_get(console);
		file_put(console); /* drop original ref */
	}
}

void process_fd_cleanup(void *process_ptr)
{
	struct process *proc = (struct process *)process_ptr;
	for (int i = 0; i < MAX_FDS; i++) {
		if (proc->fds[i]) {
			file_put(proc->fds[i]);
			proc->fds[i] = NULL;
		}
	}
}
