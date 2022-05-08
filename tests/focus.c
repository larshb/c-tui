#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>

struct termios termios_keep;

void termios_enableRawMode()
{
    tcgetattr(STDIN_FILENO, &termios_keep);
    struct termios raw = termios_keep;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

void termios_disableRawMode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios_keep);
}

int main(int argc, char const *argv[])
{
    unsigned char c;

    termios_enableRawMode();

    printf("\e[?1004hDetecting focus\n");
    while (c = getchar())
    {
        if (c == 0x1b) printf("ESC\n");
        else printf("0x%x '%c'\r\n", c, c);
        if (c == 'q') break;
    }

    termios_disableRawMode();

    return 0;
}
