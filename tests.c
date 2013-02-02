//
// tests.c -- Test cases
//
//  Copyright (c) 2012-2013 Andrew Payne <andy@payne.org>
//

#include <freedom.h>
#include "common.h"
#include <assert.h>

// Defined by linker
extern char __heap_start[];
extern char __StackTop[];
extern char __data_start__[];
extern char __data_end__[];
extern char __bss_start__[];
extern char __bss_end__[];
extern char __etext[];                // End of code/flash

extern char *_sbrk(int len);

static volatile int init_zero;
static volatile int init_value = 0xdeadbeef;
static const int const_value = 0x12345678;

// Run some basic test cases to make sure things are set up correctly
void tests(void)
{
    char i;
    char *stack, *heap_end;

    stack = &i;
    heap_end = _sbrk(0);

    // Check memory layout
    assert(__heap_start <= heap_end);
    assert(__heap_start >= __bss_end__);
    assert(heap_end < stack);
    assert(stack < __StackTop);

    assert((char *) &init_zero >= __bss_start__);
    assert((char *) &init_zero < __bss_end__);

    assert((char *) &init_value >= __data_start__);
    assert((char *) &init_value < __data_end__);

    assert((char *) &const_value < __etext);            // Stored in flash ROM

    // Check that variables are initialized properly
    assert(init_zero == 0);
    assert(init_value != 0);
    assert(init_value == 0xdeadbeef);
    assert(const_value != 0);
    assert(const_value == 0x12345678);
}