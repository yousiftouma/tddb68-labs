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
  switch (*(int*) (f->esp)) {
  	case SYS_HALT:
  		power_off();
  		break;
  	default:
  		printf("Unknown system call, %d", *(int*)f->esp);
  }

  thread_exit ();
}
