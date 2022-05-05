#if !defined(ANSI_H)
#define ANSI_H

#include <string.h>

#define ESC "\033"
#define CSI ESC "["

#define ANSI_HIGH "h"
#define ANSI_LOW "l"

#define ANSI_CLEAR CSI "2J"
#define ANSI_CURSOR_TOP_LEFT CSI "h"

#define ANSI_ALTBUF CSI "?1049"
#define ANSI_ALTBUF_ON ANSI_ALTBUF ANSI_HIGH
#define ANSI_ALTBUF_OFF ANSI_ALTBUF ANSI_LOW

#define say(s) write(STDOUT_FILENO, s, strlen(s));

#endif // ANSI_H

