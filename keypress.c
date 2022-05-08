#include "keypress.h"
#include "terminal.h"
#include "stdio.h"
#include "globals.h"

struct termpos posMouse;
static kbd_queue_t kbd_q[KBD_BUF_SIZE];
kbd_queue_t * kbd;
bool __global_keylogger_ready = false;
bool __global_keylogger_enable = true;

char * kp_arrow[] = {"UP", "DOWN", "RIGHT", "LEFT"};

int get_keypress(enum keypress * kp, char * cp)
{
    *kp = KP_UNDEF;
    *cp = getchar();
    if (*cp == *ESC)
    {
        *cp = getchar();
        if (*cp == '[')
        {
            *cp = getchar();
            switch (*cp)
            {
                case 'A':
                case 'B':
                case 'C':
                case 'D':
                    *kp = KP_ARROW;
                    return 1;
                case 'I':
                case 'O':
                    *kp = KP_FOCUS;
                    return 1;
                case 'M':
                    *cp = getchar();
                    *kp = (*cp == ' ') ? KP_CLICK : KP_IGNORE; // Mouse click ('#'=release)
                    posMouse.col = getchar() - 33;
                    posMouse.row = getchar() - 33;
                    return 1;
            }
        }
    }
    else if (('a' <= *cp && *cp <= 'z') || ('A' <= *cp && *cp <= 'Z'))
    {
        *kp = KP_CHAR;
        return 1;
    }
    else if (*cp == 10 || *cp == '\n' || *cp == '\r')
    {
        *kp = KP_IGNORE;
        return 1;
    }
    return 0;
}

void keylogger_reset()
{
    int i;

    __global_keylogger_ready = false;
    __global_keylogger_enable = false;
    for (i = 0; i < KBD_BUF_SIZE; i++)
    {
        kbd_q[i].kp = KP_UNDEF;
        kbd_q[i].c = 'U';
        kbd_q[i].consumed = true;
    }
    
    __global_keylogger_ready = true;
    __global_keylogger_enable = true;
}

void * keylogger(void * args)
{
    int i;
    char cp;
    enum keypress kp;
    kbd_queue_t * kq;

    // Reset keyboard buffer/queue
    keylogger_reset();

    // Record keyboard input
    i = 0;
    while (TUI_keepRunning && get_keypress(&kp, &cp))
    {
        if (__global_keylogger_enable)
        {
            kq = &kbd_q[i];
            kq->kp = kp;
            kq->c = cp;
            kq->consumed = false;
            i++; i%=KBD_BUF_SIZE;
        }
        else {
            usleep(100*1000);
        }
    }
}

int get_keypress_from_queue()
{
    static int i = 0;

    kbd = &kbd_q[i];
    if (!kbd_q[i].consumed)
    {
        kbd->consumed = true;
        i++; i%=KBD_BUF_SIZE;
        return 1;
    }
    else
    {
        usleep(10*1000);
    }
    return 0;
}
