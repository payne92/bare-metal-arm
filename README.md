Bare Metal Arm
==============

This is a "bare metal" runtime for the Freescale Freedom FRDM-KL25Z ARM development board ($12). It builds
builds with GCC ARM toolchain, with no other external dependencies. 

Quick start on Linux:
* Clone the repo: `git clone git@github.com:payne92/bare-metal-arm.git`
* Grab and unpack GCC ARM toolchain: `cd bare-metal-arm; make gcc-arm`
* 'make'

This will create a `demo.srec` image file to flash onto the development board.  (If you're using
the standard bootloader, plug SDA USB port to a host computer and then copy the .SREC file
to the FRDM-KL25Z block device.)  

If everything is working, the RGB LEB will be steady green.  You can access the USB SDA serial port (at 115,200 baud)
and see the accelerometer and touch input status.

References:
* Freescale Freedom FRDM-KL25Z: http://www.freescale.com/webapp/sps/site/prod_summary.jsp?code=FRDM-KL25Z
* GCC ARM toolchain:  https://launchpad.net/gcc-arm-embedded
* Newlib: http://sourceware.org/newlib/

Why do this?
------------

Most vendor provided toolchains are daunting. They're large and complicated because they support a wide
range of processors, libraries, and commerical C compilers.  These tools are usually laden with 
complex configuration tools, lots of source macros and #ifdef statements, and code that's been ported
down several generations of processors.

In contrast, this project is a small (<1,000 lines) simple, and clean bare metal framework that builds 
from the command line.  It has no external library or tool dependencies, only supports GCC, and has minimal use of
assembly. 
