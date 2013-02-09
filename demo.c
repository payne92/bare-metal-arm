//
// demo.c -- Simple demonstration program
//
//  Copyright (c) 2012-2013 Andrew Payne <andy@payne.org>
//

#include <stdio.h>
#include "freedom.h"
#include "common.h"

extern char *_sbrk(int len);

// Main program
int main(void)
{
    char i;
    char *heap_end;
    
    // Initialize all modules
    uart_init(115200);
    accel_init();
    touch_init((1 << 9) | (1 << 10));       // Channels 9 and 10
    // usb_init();
    setvbuf(stdin, NULL, _IONBF, 0);        // No buffering

    // Run tests
    tests();
    delay(500);
    RGB_LED(0,100,0);                       // Green

    // Welcome banner
    iprintf("\r\n\r\n====== Freescale Freedom FRDM-LK25Z\r\n");
    iprintf("Built: %s %s\r\n\r\n", __DATE__, __TIME__);
    heap_end = _sbrk(0);
    iprintf("Heap:  %p to %p (%d bytes used)\r\n", __heap_start, heap_end, 
                heap_end - (char *)__heap_start);
    iprintf("Stack: %p to %p (%d bytes used)\r\n", &i, __StackTop, 
                (char *)__StackTop - &i);
    iprintf("%d bytes free\r\n", &i - heap_end);
    
    for(;;) {
        iprintf("monitor> ");
        getchar();
        iprintf("\r\n");
        iprintf("Inputs:  x=%5d   y=%5d   z=%5d ", accel_x(), accel_y(), accel_z());
        iprintf("touch=(%d,%d)\r\n", touch_data(9), touch_data(10));
        // usb_dump();
    }
}