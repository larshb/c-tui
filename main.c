#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <ctype.h>

#include "ansi.h"
#include "terminal.h"

static volatile bool inFocus = true;
static volatile bool keepRunning = true;

char exitReason[128] = "";



void intHandler(int dummy) {
    keepRunning = 0;
}

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

char * kp_arrow[] = {"UP", "DOWN", "RIGHT", "LEFT"};

struct termpos {
    unsigned char row;
    unsigned char col;
};
struct termpos termpos;

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
                    termpos.col = getchar() - 33;
                    termpos.row = getchar() - 33;
                    return 1;
            }
        }
    }
    else if (('a' <= *cp && *cp <= 'z') || ('A' <= *cp && *cp <= 'Z'))
    {
        *kp = KP_CHAR;
        return 1;
    }
    return 0;
}

#define KBD_BUF_SIZE 16
typedef struct {
    enum keypress kp;
    char c;
    bool consumed;
} kbd_queue_t;
kbd_queue_t kbd_q[KBD_BUF_SIZE];
kbd_queue_t * kbd;

bool __global_keylogger_ready = false;

void * keylogger(void * args)
{
    int i;
    char cp;
    enum keypress kp;
    kbd_queue_t * kbd;

    // Reset keyboard buffer/queue
    for (i = 0; i < KBD_BUF_SIZE; i++)
    {
        kbd_q[i].kp = KP_UNDEF;
        kbd_q[i].c = 'U';
        kbd_q[i].consumed = true;
    }
    i = 0;
    __global_keylogger_ready = true;

    // Record keyboard input
    while (keepRunning && get_keypress(&kp, &cp))
    {
        kbd = &kbd_q[i];
        kbd->kp = kp;
        kbd->c = cp;
        kbd->consumed = false;
        i++; i%=KBD_BUF_SIZE;
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
    return 0;
}

void setup()
{
    // Register signal interrupt handler
    signal(SIGINT, intHandler);

    // Setup RAW TTY
    termios_enableRawMode();
    
    say(
        CSI "?1049h" // Enable alternative buffer
        CSI "?1000h" // Detect mouse pointer
        CSI "?1004h" // Detect focus
        CSI "?25l"   // Hide cursor
        CSI "H"      // Position cursor top left
        CSI "2J"     // Clear entire screen
    );
}

void cleanup()
{
    termios_disableRawMode();
    say(
        CSI "?1049l" // Disable alternative buffer
        CSI "?25h"   // Show cursor
        CSI "?1000l" // Undetect mouse pointer
        CSI "?1004l" // Undetect focus
    );
}

#define TITLE "{Title placeholder}"

void position_cursor(int r, int c)
{
    printf(CSI "%d;%dH", r+1, c+1);
}

#define KNOB_DESC_SIZE 128
typedef struct {
    int r, c;
    int value;
    int width;
    char desc[KNOB_DESC_SIZE];
    int id;
    int (*get)(int); // id
    void (*set)(int, int); // id, value
} knob_t;

void knobs_position(int rows, int cols, int row_offset, knob_t * knobs, const int N_KNOBS)
{
    int i, r, c;
    int desc_width, desc_width_max=0;
    knob_t * k_p;

    // Figure out max width needed for every knob
    for (i = 0; i < N_KNOBS; i++)
    {
        desc_width = strlen(knobs[i].desc);
        if (desc_width_max < desc_width)
        {
            desc_width_max = desc_width;
        }
    }

    r = row_offset;
    c = 2;
    for (i = 0; i < N_KNOBS; i++)
    {
        k_p = &knobs[i];
        k_p->width = desc_width_max;
        if (cols < c + desc_width_max + 2)
        {
            r += 3;
            c = 2;
        }
        k_p->r = r;
        k_p->c = c;
        c += desc_width_max + 1;
    }
}

int knob_update(knob_t * knob)
{
    knob->value = knob->get(knob->id);
    return knob->value;
}

#define N_KNOBS 20
int dummy_data[N_KNOBS];
void dummy_set(int id, int value)
{
    dummy_data[id] = value;
}
int dummy_get(int id)
{
    return dummy_data[id];
}
int random_get(int id)
{
    return rand() % 255;
}

int file_dimensions(const char * path, ssize_t * rows, ssize_t * cols)
{
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t n;
    
    *rows = 0;
    *cols = 0;
    fp = fopen(path, "r");
    if (fp == NULL)
    {
        return 1;
    }
    while ((n = getline(&line, &len, fp)) != -1) {
        (*rows)++;
        // if (line[n-1] == '\n') n-=1;
        if (*cols < n) *cols = n;
    }
    fclose(fp);
    if (line) free(line);
    return 0;
}

void tui()
{
    int i, r, c;
    pthread_t thread;
    bool redraw = true;
    bool popup = true;
    struct winsize w, w_prev;
    char msg[128] = {0};

    // Set up help-menu
    const struct {
        char key[10];
        char desc[128];
    } help[] = {
        {"X", "E[x]it"},
        {"A", "Right[a]"},
        {"D", "Left[d]"},
        {"↑", "Increment[↑]"},
        {"↓", "Decrement[↓]"}
        /*,
        {'Q', "Menu left"},
        {'E', "Menu right"}*/
    };

    // Set up (dummy) knobs
    knob_t knobs[N_KNOBS];
    knob_t gauge[N_KNOBS]; // Read-only
    for (i = 0; i < N_KNOBS; i++)
    {
        knob_t * k_p = &knobs[i];
        knob_t * g_p = &gauge[i];
        memset(k_p->desc, 0, KNOB_DESC_SIZE);
        memset(g_p->desc, 0, KNOB_DESC_SIZE);
        sprintf(k_p->desc, "DAC-%d", i);
        sprintf(g_p->desc, "ADC-%d", i);
        k_p->id = i;
        g_p->id = i;
        k_p->set = dummy_set;
        g_p->set = NULL;
        k_p->get = dummy_get;
        g_p->get = random_get;
        k_p->set(k_p->id, i);
        knob_update(k_p);
        knob_update(g_p);
    }
    int knob_sel = 0;

    pthread_create(&thread, NULL, keylogger, NULL);

    while (!__global_keylogger_ready)
    {
        usleep(10*1000);
    }

    i = 0;
    while (keepRunning)
    {
        // Get terminal dimensions
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        if (w_prev.ws_col != w.ws_col || w_prev.ws_row != w.ws_row)
        {
            redraw = true;
            sprintf(msg, "[%d, %d]", w.ws_row, w.ws_col);
        }
        w_prev.ws_col = w.ws_col;
        w_prev.ws_row = w.ws_row;

        // if (popup) redraw = true;

        if (redraw)
        {
            redraw = false;

            knobs_position(w.ws_row, w.ws_col, 1, knobs, N_KNOBS);
            // knobs_position(w.ws_row, w.ws_col, 1+w.ws_row/2, gauge, N_KNOBS);

            position_cursor(w.ws_row-2, w.ws_col-2);
            say(
                CSI "2J"  // Clear screen
                CSI "H"   // Cursor top left
            );

            if (inFocus)
            {
                // Draw outer frame
                position_cursor(0, 0);
                printf("┌");
                for (i = 0; i < w.ws_col-2; i++) printf("─");
                printf("┐");
                for (i = 1; i < w.ws_row-1; i++)
                {
                    position_cursor(i, 0); printf("│");
                    position_cursor(i, w.ws_col); printf("│");
                }
                position_cursor(w.ws_row, 0);
                printf("└");
                for (i = 0; i < w.ws_col-2; i++) printf("─");
                printf("┘");

                // Print title
                position_cursor(0, 2);
                printf("%s", TITLE);
            }

            // // Divider
            // position_cursor(w.ws_row / 2, 0);
            // printf("├");
            // for (i = 0; i < w.ws_col-2; i++) printf("─");
            // printf("┤");
            // position_cursor(w.ws_row / 2, 2);
            // printf("{divider text}");

            // Print help
            #if 0 // Inside frame
            position_cursor(w.ws_row-3, 0);
            printf("├");
            for (i = 0; i < w.ws_col-2; i++) printf("─");
            printf("┤");
            position_cursor(w.ws_row-2, 1);
            for (i = 0; i < sizeof(help) / sizeof(*help); i++)
            {
                printf(" " CSI "7m %s " CSI "0m" " %s", help[i].key, help[i].desc);
            }
            #else // on top of frame
            position_cursor(w.ws_row-1, 2);
            for (i = 0; i < sizeof(help) / sizeof(*help); i++)
            {
                // printf(" ");
                printf("%s", help[i].desc);
                // printf(" ");
                // printf("[%s]", help[i].key);
                // printf(" ");
                printf(CSI "C"); // Cursor forward
            }
            #endif

            // Print knobs
            for (i = 0; i < N_KNOBS; i++)
            {
                knob_t * k_p = &knobs[i];
                position_cursor(k_p->r, k_p->c);
                if (i == knob_sel && inFocus) printf(CSI "7m");
                printf("%*d", k_p->width, knob_update(k_p));
                position_cursor(k_p->r+1, k_p->c);
                printf(CSI "0;2m" "%*s" CSI "0m", k_p->width, k_p->desc);
            }
        }

        // // Print gauges
        // for (i = 0; i < N_KNOBS; i++)
        // {
        //     knob_t * g_p = &gauge[i];
        //     position_cursor(g_p->r, g_p->c);
        //     printf("%*d", g_p->width, knob_update(g_p));
        //     position_cursor(g_p->r+1, g_p->c);
        //     printf(CSI "0;2m" "%*s" CSI "0m", g_p->width, g_p->desc);
        // }

        if (popup)
        {
            FILE * fp;
            char * line = NULL;
            size_t len = 0;
            ssize_t n;
            ssize_t rows, cols;
            const char * path = "popup.txt";
            file_dimensions(path, &rows, &cols);

            // Check boundaries
            if (!(rows <= (w.ws_row-4) && cols <= (w.ws_col-4)))
            {
                printf(CSI "H" "Terminal window too small");
                keepRunning = false;
            }
            else
            {
                r = (w.ws_row/2) - (rows/2) - 2;
                c = (w.ws_col/2) - (cols/2) - 2;
                position_cursor(r, c);
                printf("┌");
                for (i = 0; i < cols+2-2; i++) printf("─");
                printf("┐");


                fp = fopen(path, "r");
                if (fp == NULL)
                {
                    exit(EXIT_FAILURE);
                }
                while ((n = getline(&line, &len, fp)) != -1) {
                    position_cursor(++r, c); printf("│");
                    if (line[n-1] == '\n') {
                        line[n-1] = 0;
                        n -= 1;
                    }
                    printf("%s", line);
                    for (i = 0; i < cols-n; i++) printf(" ");
                    printf("│");
                }
                fclose(fp);
                position_cursor(++r, c);
                printf("└");
                for (i = 0; i < cols+2-2; i++) printf("─");
                printf("┘");
                if (line) free(line);
            }
        }
        
        // Flush terminal buffer
        fflush(stdout);

        if (keepRunning && get_keypress_from_queue())
        {
            if (popup)
            {
                popup = false;
                redraw = true;
            }
            knob_t * k_p = &knobs[knob_sel];
            if (kbd->kp == KP_CHAR)
            {
                switch (tolower(kbd->c))
                {
                    case 'x': keepRunning = false; break;
                    case 'a':
                        knob_sel -= 1;
                        if (knob_sel < 0) knob_sel += N_KNOBS;
                        redraw = true;
                        break;
                    case 'd':
                        knob_sel += 1;
                        knob_sel %= N_KNOBS;
                        redraw = true;
                        break;
                }
            }
            else if (kbd->kp == KP_ARROW && kbd->c == 'A') // up:increase
            {
                k_p->set(k_p->id, knob_update(k_p) + 1);
                redraw = true;
            }
            else if (kbd->kp == KP_ARROW && kbd->c == 'B') // down:decrease
            {
                k_p->set(k_p->id, knob_update(k_p) - 1);
                redraw = true;
            }
            else if (kbd->kp == KP_FOCUS)
            {
                inFocus = (kbd->c == 'I');
                sprintf(msg, "Focus changed to %s", inFocus?"in":"out");
                redraw = true;
            }
            else if (kbd->kp == KP_CLICK)
            {
                position_cursor(termpos.row, termpos.col);
                printf("✗");
                fflush(stdout);
            }
            else if (kbd->kp != KP_IGNORE)
            {
                sprintf(msg, "Unknown option %c [%d:%d]", kbd->c, kbd->kp, kbd->c);
            }
            
        }
        else
        {
            usleep(10*1000);
        }
        if (*msg != 0)
        {
            int temp_col = w.ws_col/2 - strlen(msg)/2;
            if (0 <= temp_col)
            {
                position_cursor(w.ws_row / 2, temp_col);
                printf(CSI "41m" "%s" CSI "0m", msg);
                memset(&msg, 0, sizeof(msg) / sizeof(*msg));
                // fflush(stdout);
                usleep(500*1000);
                redraw = true;
            }
            else
            {
                keepRunning = false;
                sprintf(exitReason, "Terminal window too narrow");
            }
        }
    }
}

int main(int argc, char *argv[]) {
    setup();
    tui();
    // pthread_join(thread, NULL);
    cleanup();
    if (*exitReason != 0) printf("%s\n", exitReason);
    // puts("Goodbye!");
    return 0;
}
