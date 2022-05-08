#if !defined(KEYPRESS_H)
#define KEYPRESS_H

#include <stdbool.h>

enum keypress {
    KP_UNDEF,
    KP_IGNORE,
    KP_CHAR,
    KP_ARROW,
    KP_FOCUS,
    KP_CLICK
};

enum kp_arrow {
    KP_ARROW_UP,
    KP_ARROW_DOWN,
    KP_ARROW_RIGHT,
    KP_ARROW_LEFT
};

#define KBD_BUF_SIZE 16
typedef struct {
    enum keypress kp;
    char c;
    bool consumed;
} kbd_queue_t;

extern char * kp_arrow[];
extern struct termpos posMouse;
extern bool __global_keylogger_ready;
extern bool __global_keylogger_enable;
extern kbd_queue_t * kbd;

//int get_keypress(enum keypress * kp, char * cp);
void keylogger_reset();
void * keylogger(void * args);
int get_keypress_from_queue();

#endif // KEYPRESS_H
