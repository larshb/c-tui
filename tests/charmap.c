#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <locale.h>

#include "../terminal.h"

unsigned int offset = 0x000;
unsigned int charsPerPage = 0x200;
unsigned int charsPerLine = 0x10;

int cleanChar(unsigned int c)
{
    if (
        (c < 0x20)
        || (0x80 <= c && c < 0x90)
    ) return '.';
    return c;
}

void printCharMap()
{
    unsigned int charIndex, charRow, charColumn;

    say(
        CSI "2J" // Clear screen
        CSI "H"  // Cursor home
    )

    for (charRow = 0; charRow < charsPerPage / charsPerLine; charRow++)
    {
        charIndex = offset + (charsPerLine * charRow);
        printf("U+%04X |", charIndex);
        for (charColumn = 0; charColumn < charsPerLine; charColumn++)
        {
            printf(" %lc", cleanChar(charIndex));
            charIndex++;
        }
        printf("\r\n");
    }
}

int main(int argc, char const *argv[])
{
    unsigned char c, meta, row, col;

    termios_enableRawMode();
    say(
        CSI "?25l"   // Hide cursor
        CSI "?1049h" // Enable alternate buffer
    )
    setlocale(LC_CTYPE, "");
    printCharMap();

    while (getchar() == '\e')
    {
        int refresh = 1;
        if (getchar() == '[')
        {
            c = getchar();
            if      (c == 'A') { /*    UP */ }
            else if (c == 'B') { /*  DOWN */ }
            else if (c == 'C') { /* RIGHT */ offset += charsPerPage; }
            else if (c == 'D') { /*  LEFT */ offset -= charsPerPage; }
            else { break; }
        }
        else
        {
            break;
        }

        while (offset < 0) offset += charsPerPage;
        printCharMap();
    }

    say(
        CSI "?25h"   // Show cursor
        CSI "?1049l" // Disable alternate buffer
    )
    termios_disableRawMode();

    return 0;
}
