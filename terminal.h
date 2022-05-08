#if !defined(TERMINAL_H)
#define TERMINAL_H

#include <unistd.h>
#include <termios.h>
#include <string.h>

#define ESC "\e"
#define CSI ESC "["

#define say(s) write(STDOUT_FILENO, s, strlen(s));

struct termios termios_keep;

#define termios_disableRawMode() \
{ \
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios_keep); \
}

#define termios_enableRawMode() \
{ \
    tcgetattr(STDIN_FILENO, &termios_keep); \
    struct termios raw = termios_keep; \
    raw.c_lflag &= ~(ECHO | ICANON); \
    tcsetattr(STDIN_FILENO, TCSANOW, &raw); \
}

#endif // TERMINAL_H
