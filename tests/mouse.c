#include <stdlib.h>
#include <stdio.h>

#include "../terminal.h"

int main(int argc, char const *argv[])
{
    unsigned char c, meta, row, col;

    termios_enableRawMode();

    printf("\e[?1000h"); // Enable mouse click detection

    puts("Click somewhere inside the screen");
    puts("Press any other key to end execution");
    while (getchar() == '\e')
    {
        if (getchar() == '[')
        {
            if (getchar() == 'M')
            {
                meta = getchar();
                col = getchar() - 33;
                row = getchar() - 33;
                if (meta == ' ') // Click
                {
                    printf("Clicked at %d, %d\r\n", row, col);
                }
                // '#' // Release
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
    }

    printf("\e[?1000l"); // Disable mouse click detection

    termios_disableRawMode();

    return 0;
}
