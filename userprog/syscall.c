#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "threads/init.h"
#include "devices/input.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

/*
  Creates a new file with a specified name and size, returns true if successful,
  else false
*/
  bool syscall_create(void* arg_ptr);

/*
  Opens the file with the given name, and returns a file descriptor (fd),
  returns fd = -1 if no file with that name exists or if the user program
  has too many files opened allready
*/
int syscall_open(void* arg_ptr);

/*
  Closes the file with the given name if opened
*/
void syscall_close(void* arg_ptr);

/*
  Writes size bytes from the buffer to the open file with the given fd,
  if the fd = 1 then the buffer is written to the console. Returns the number
  of bytes sucessfully written
*/
int syscall_write(void* arg_ptr);

/*
  Reads size bytes from the file with the given fd, if fd = STIN_FILENO then
  we read from console instead. Returns the number of bytes actually read.
*/
int syscall_read(void* arg_ptr);

/*
  Creates a new child process executing the given command, with arguments.
  Return the process id (pid_t) of the child_process, -1 if not sucessful
*/
pid_t syscall_exec(void* arg_ptr);

/*
  Sleeps the calling thread until the child process with the given pid exits,
  or returns immidiatly if the child has already exited. Return -1 if no child
  process has the given pid
*/
int syscall_wait(void* arg_ptr);

/*
  Kills the user program by killing the thread and thereby realesing all allocated
  resources and closes all open files
*/
void syscall_exit(void* arg_ptr);

/*
  Halts the system and power down the system
*/
void syscall_halt(void);

/*
  Returns true if the given pointer is pointing to user space and the location
  is within a allocated page for the user process
*/
bool is_valid_ptr(void* ptr);

static void syscall_handler (struct intr_frame *f UNUSED) {

    void* arg_ptr = (f->esp);

    // Check that the stack pointer is valid
    if (!is_valid_ptr(arg_ptr)) {
      thread_exit();
    }

    int syscall_nr = *(int*)(f->esp);


    switch (syscall_nr) {	
      case SYS_HALT :
        syscall_halt();
        break;

      case SYS_WRITE:
        f->eax = syscall_write(arg_ptr);
        break;

      case SYS_CREATE:
        f->eax = syscall_create(arg_ptr);
        break;

      case SYS_OPEN:
        f->eax = syscall_open(arg_ptr);
        break;

      case SYS_READ:
        f->eax = syscall_read(arg_ptr);
        break;

      case SYS_CLOSE:
        syscall_close(arg_ptr);
        break;

      case SYS_EXEC:
        f->eax = syscall_exec(arg_ptr);
        break;

      case SYS_WAIT:
        f->eax = syscall_wait(arg_ptr);
        break;

      case SYS_EXIT:
        syscall_exit(arg_ptr);
        break;

      default:
        printf("Unknown system call, %d", syscall_nr);
    }
}

bool is_valid_ptr(void* ptr) {
  return is_user_vaddr(ptr) 
      && pagedir_get_page(thread_current()->pagedir, ptr) != NULL;
}

bool syscall_create(void* arg_ptr) {
  char* file_name = ((char**)arg_ptr)[1];
  int file_size = ((int*)arg_ptr)[2];
  return filesys_create(file_name, file_size);
}

int syscall_open(void* arg_ptr) {
  char* file_name = ((char**)arg_ptr)[1];
  struct file *new_open = filesys_open(file_name);

  // Check sucessful open
  if (new_open != NULL) {
    unsigned int fnd = bitmap_scan_and_flip(thread_current()->file_ids, 0, 1, 0);
    if (fnd != BITMAP_ERROR) { // Not enough space
      thread_current()->open_files[fnd] = new_open;
      return fnd;
    }
  }
  return -1;
}

void syscall_close(void* arg_ptr) {
  int fd = ((int*)arg_ptr)[1];
  if (fd < 128 && fd > 1 && bitmap_test(thread_current()->file_ids, fd)) {
    file_close(thread_current()->open_files[fd]);
  }
  bitmap_set(thread_current()->file_ids, fd, 0);
}

int syscall_write(void* arg_ptr) {
  int bytes_written = 0;

  int fd = ((int*)arg_ptr)[1];
  void *buf = ((void**)arg_ptr)[2];
  int size = ((int*)arg_ptr)[3];

  // Write to console
  if (fd == STDOUT_FILENO) {
    const char* char_buf = (char*)buf;
    putbuf(char_buf, size);
    bytes_written = size;
  } 
  else {
    if (fd < 128 && fd > 1 && bitmap_test(thread_current()->file_ids, fd)) {
      struct file* file_to_write = thread_current()->open_files[fd];
      bytes_written = file_write(file_to_write, buf, size);
    }
  }
  return bytes_written;  
}

int syscall_read(void* arg_ptr) {
  int fd = ((int*)arg_ptr)[1];
  void *buf = ((void**)arg_ptr)[2];
  int size = ((int*)arg_ptr)[3];

  int bytes_read = -1;

  // Read from console if STDIN_FILENO
  if (fd == STDIN_FILENO) {
    bytes_read = 0;
    char* char_buf = (char*)buf;
    while (bytes_read < size) {
      char_buf[bytes_read] = (char)input_getc();
      bytes_read++;
    }
  }
  else {
    if (fd < 128 && fd > 1 && bitmap_test(thread_current()->file_ids, fd)) {
      struct file* file_to_read = thread_current()->open_files[fd];
      bytes_read = file_read(file_to_read, buf, size);
    }
  }
  return bytes_read;
}

pid_t syscall_exec(void* arg_ptr) {
  char* cmd = ((char**)arg_ptr)[1];
  return (pid_t) process_execute(cmd);
}

void syscall_exit(void* arg_ptr) {

  int exit_code = ((int*) arg_ptr)[1];
  struct child_status* my_status = thread_current()->my_status;
  
  // I am child, set exit code
  if (my_status != NULL) {
    lock_acquire(&my_status->lock);
    my_status->exit_code = exit_code;
    lock_release(&my_status->lock);
  }
  thread_exit(); // Kill thread and free resources
}

int syscall_wait(void* arg_ptr) {
  pid_t child_pid = ((pid_t*) arg_ptr)[1];
  return process_wait(child_pid);
}

void syscall_halt() {
  power_off();
}