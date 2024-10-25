## Compiler agnostic include files

This directory is to be added to the include path sent to non cc65 compilers, so that the main project files can continue to use <std*.h>, <string.h>, <conio.h> etc as available in CC65.

Platform specific wrappers should be provided (e.g. conio.c) in that platform's folder