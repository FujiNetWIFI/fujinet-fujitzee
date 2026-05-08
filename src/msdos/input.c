#ifdef __WATCOMC__

#include <dos.h>
#include "vars.h"
#include "../platform-specific/input.h"

unsigned char readJoystick()
{
    return 0;
}

/* kbhit() is supplied by Watcom's <conio.h> runtime; do not redefine it
 * here, or the int-vs-byte signature mismatch corrupts the AX return and
 * makes input look dead. */

/* Blocking key read - matches the cc65 cgetc semantics that the shared
 * fujitzee code expects (it calls bare cgetc() to wait for any key after
 * clearCommonInput()). The drain pattern `while (kbhit()) cgetc();` is
 * still safe because kbhit guarantees a key is buffered before cgetc runs. */
char cgetc(void)
{
    union REGS r;
    r.h.ah = 0x00;
    int86(0x16, &r, &r);
    if (r.h.al == 0)
        return (char)(r.h.ah | 0x80);
    return (char)r.h.al;
}

#endif /* __WATCOMC__ */
