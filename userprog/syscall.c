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
      ;
      int fd = ((int*)arg_pr)[1];
      void *buf = ((void**)arg_pr)[2];
      int size = ((int*)arg_pr)[3];

  		// Write to console
  		if (fd == STDOUT_FILENO) {
  			const char* char_buf = ((char**)arg_pr)[2];
  			putbuf(char_buf, size);
  			f->eax = size;
		  }
      else {
        //if (bitmap_test(thread_current()->file_ids, fd));
        //TODO make sure to handle non opened files
        //printf("Writing files");
        f->eax = size;
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
      
      // Failed to open
      if (new_open != NULL) {
        int fnd = bitmap_scan_and_flip (thread_current()->file_ids, 0, 1, 0);
        thread_current()->open_files[fnd] = new_open;
        f->eax = fnd;
      }
      else {
        f->eax = -1;
      }

      break;

    case SYS_READ:
      ;
      int fd_r = ((int*)arg_pr)[1];
      void *buf_r = ((void**)arg_pr)[2];
      int size_r = ((int*)arg_pr)[3];
      int bytes_read_r = -1;

      // Read from console if STDIN_FILENO
      if (fd_r == STDIN_FILENO) {
        bytes_read_r = 0;
        char* char_buf_r = (char*)buf_r;
        while (bytes_read_r < size_r) {
          char_buf_r[bytes_read_r] = (char)input_getc();
          bytes_read_r++;
        }
        f->eax = bytes_read_r;
      }
      else {
        if (fd_r < 128 && bitmap_test(thread_current()->file_ids, fd_r)) {
          struct file* file_to_read = thread_current()->open_files[fd_r];
          bytes_read_r = file_read(file_to_read, buf_r, size_r);
        }
        f->eax = bytes_read_r;
      }
      break;

    case SYS_CLOSE:
      ;
      int fd_close = ((int*)arg_pr)[1];
      bitmap_set(thread_current()->file_ids, fd_close, 0);
      break;

  	default:
  		printf("Unknown system call, %d", *(int*)f->esp);
  }
  //thread_exit();
}
