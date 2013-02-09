//
// syscalls.c -- Low level memory and I/O functions for newlib 
//
//  Nov, 2012
//  <andy@payne.org>
//

#include <freedom.h>
#include <sys/stat.h>
#include "common.h"

int _close(int file) { return -1; }
int _isatty(int file) { return 1; }
int _open(const char *name, int flags, int mode) { return -1; }

int _fstat(int file, struct stat *st) 
{
    st->st_mode = S_IFCHR;                  // Character device
    return 0;       
}

int _write(int file, char *p, int len)
{
    switch(file) {
     case 1:        return uart_write(p, len);              // stdout
     case 2:        return uart_write_err(p,len);           // stderr
     default:       return -1;
    }
}

int _read(int file, char *p, int len)
{
    return uart_read(p, len);
}

// ------------------------------------------------------------------------------------
// _sbrk(len) -- Allocate space on the heap
//
static char *heap_end = (char *) __heap_start;

char *_sbrk(int incr)
{
    heap_end += incr;                   // TODO:  check for collisions with the stack
    return (heap_end - incr);
}

// Signal handler (fault)
int _kill(int pid, int sig)
{
    fault(0b11111110);
    return -1;                          // Never gets here
}