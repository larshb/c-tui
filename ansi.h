#if !defined(ANSI_H)
#define ANSI_H

#include <string.h>

#define ESC "\e"
#define CSI ESC "["

#define say(s) write(STDOUT_FILENO, s, strlen(s));

#endif // ANSI_H
