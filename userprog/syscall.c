#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int syscall_nr = *(int*)(f->esp);
  void* arg_pr = (f->esp);
  switch (syscall_nr) {
  	
  	// System halt
  	case SYS_HALT :
  		power_off();
  		break;

  	case SYS_WRITE:

  		// Write to console
  		if (((int*)arg_pr)[1] == 1) {
  			const char* buf = ((char**)arg_pr)[2];
  			size_t buf_size = ((size_t*)arg_pr)[3];
  			putbuf(buf, buf_size);
  			f->eax = buf_size;
		}
  		break;

  	case SYS_CREATE:
  		;
  		bool success = false;
  		success = filesys_create(((char**)arg_pr)[1], ((int*)arg_pr)[2]);
  		f->eax = success;
  		break;

  	case SYS_OPEN:
      ;
      char* file_name = ((char**)arg_pr)[1];
      struct file *new_open = filesys_open(file_name);
      int fnd = bitmap_scan_and_flip (thread_current()->file_ids, 0, 1, 0);
      thread_current()->open_files[fnd] = new_open;
      f->eax = fnd;

  	default:
  		printf("Unknown system call, %d", *(int*)f->esp);
  }
  //thread_exit();
}
