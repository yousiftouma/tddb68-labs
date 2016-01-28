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
		}
  		break;
  	default:
  		printf("Unknown system call, %d", *(int*)f->esp);
  }

  thread_exit ();
}
