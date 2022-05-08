#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "../ansi.h"
#include "../terminal.h"

#define BRUSH "▒"

int main(int argc, char const *argv[])
{
    unsigned char c, meta, col, row;
    int draw = 0;
    termios_enableRawMode();

    say(
        CSI "?25l"   // Hide cursor
        CSI "?1003h" // Track any mouse event
    );

    while (getchar() == '\e')
    {
        if (getchar() == '[')
        {
            c = getchar();
            if (c == 'M')
            {
                meta = getchar();
                col = getchar() - 0x20;
                row = getchar() - 0x20;
                if (meta == 'C') // Move
                {
                }
                else if (meta == ' ') // Click
                {
                    draw = 1;
                }
                else if (meta == '#') // Release
                {
                    draw = 0;
                }
                else
                {
                    // printf("'%c' at %d, %d\r\n", meta, row, col);
                }
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }

        if (draw)
        {
            printf(CSI "31m");
            printf(CSI "%d;%dH", row, col);
            printf(BRUSH);
            printf(CSI "0m");
            fflush(stdout);
        }
    }

    say(
        CSI "1000;1H" // Cursor at bottom left to preserve drawing
        CSI "?25h"    // Show cursor
        CSI "?1003l"  // Disable tracking of any mouse event
    );

    termios_disableRawMode();

    return 0;
}
