#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <ctype.h>

#include "terminal.h"
#include "knobs.h"
#include "keypress.h"
#include "globals.h"

volatile bool TUI_keepRunning;
volatile bool TUI_inFocus = true;
volatile bool TUI_shrinkToFit = true;
kbd_queue_t * kbd;

char exitReason[128] = "";

void intHandler(int dummy) {
    TUI_keepRunning = 0;
}

void setup()
{
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
        CSI "?1000l" // Undetect mouse pointer
        CSI "?1004l" // Undetect focus
        CSI "?1049l" // Disable alternative buffer
        CSI "?25h"   // Show cursor
    );
}

#define TITLE "{Title placeholder}"
#define N_DUMMY_KNOBS 8
int dummy_data[N_DUMMY_KNOBS];
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
void dummy_init(knob_t knobs[], const int N_KNOBS) {
    int i;

    // knob_t knobs[N_KNOBS];
    // knob_t gauge[N_KNOBS]; // Read-only
    for (i = 0; i < N_KNOBS; i++)
    {
        knob_t * k_p = &knobs[i];
        // knob_t * g_p = &gauge[i];
        memset(k_p->desc, 0, KNOB_DESC_SIZE);
        // memset(g_p->desc, 0, KNOB_DESC_SIZE);
        sprintf(k_p->desc, "DUMMY-%d", i);
        // sprintf(g_p->desc, "ADC-%d", i);
        k_p->id = i;
        // g_p->id = i;
        k_p->set = dummy_set;
        // g_p->set = NULL; // Read only
        k_p->get = dummy_get;
        // g_p->get = random_get;
        k_p->set(k_p->id, i);
        knob_update(k_p);
        // knob_update(g_p);
    }
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

void printFile(const char path[])
{
    FILE * fp;
    char c;

    fp = fopen(path, "r");
    if (fp == NULL)
    {
        printf("Cannot open file \n");
        return;
    }

    c = fgetc(fp);
    while (c != EOF)
    {
        printf("%c", c);
        c = fgetc(fp);
    }

    fclose(fp);
}

void tui()
{
    int i, r, c;
    pthread_t thread;
    bool redraw = true;
    struct winsize win_curr, win_prev;
    struct termpos canvas, posBtnMaximize, posBtnClose;
    char msg[128] = {0};

    // Setup keylogger
    pthread_create(&thread, NULL, keylogger, NULL);
    while (!__global_keylogger_ready)
    {
        usleep(10*1000);
    }
    
    bool prompt=false;
    const char prompt_label[256] = "Run in shell";

    bool popup=false;
    const char popup_path[256] = "popup.txt";

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
    knob_t knobs[N_DUMMY_KNOBS];
    dummy_init(knobs, N_DUMMY_KNOBS);
    int knob_width = knobs_get_width(knobs, N_DUMMY_KNOBS);
    int canvas_max_width;
    int knob_sel = 0;

    i = 0;
    while (TUI_keepRunning)
    {
        // Get terminal dimensions
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &win_curr);
        if (TUI_shrinkToFit)
        {
            int knobs_fixed_width = 2+(knob_width+1)*8+1;
            if (knobs_fixed_width < win_curr.ws_col) win_curr.ws_col = knobs_fixed_width;
        }
        if (win_prev.ws_col != win_curr.ws_col || win_prev.ws_row != win_curr.ws_row)
        {
            redraw = true;
            canvas.row = win_curr.ws_row;
            canvas.col = win_curr.ws_col;
            posBtnClose.row = 0;
            posBtnClose.col = canvas.col-3;
            posBtnMaximize.row = 0;
            posBtnMaximize.col = canvas.col-5;
            if (!TUI_shrinkToFit) sprintf(msg, "[%d, %d]", canvas.row, canvas.col);
        }
        win_prev.ws_col = win_curr.ws_col;
        win_prev.ws_row = win_curr.ws_row;

        // if (popup) redraw = true;

        if (redraw)
        {
            redraw = false;

            knobs_position(canvas.row, canvas.col, 2, knob_width, knobs, N_DUMMY_KNOBS);
            // knobs_position(canvas.row, canvas.col, 1+canvas.row/2, gauge, N_KNOBS);

            if (TUI_shrinkToFit)
            {
                canvas.row = knobs[N_DUMMY_KNOBS-1].r + 3;
            }

            position_cursor(canvas.row-2, canvas.col-2);
            say(
                CSI "2J"  // Clear screen
                CSI "H"   // Cursor top left
            );

            if (TUI_inFocus)
            {
                // Draw outer frame
                position_cursor(0, 0);
                printf("┌");
                for (i = 0; i < canvas.col-2; i++) printf("─");
                printf("┐");
                for (i = 1; i < canvas.row-1; i++)
                {
                    position_cursor(i, 0); printf("│");
                    position_cursor(i, canvas.col-1); printf("│");
                }
                position_cursor(canvas.row-1, 0);
                printf("└");
                for (i = 0; i < canvas.col-2; i++) printf("─");
                printf("┘");

                // Add top buttons
                position_cursor(posBtnClose.row, posBtnClose.col);
                printf(CSI "0m" "☒" CSI "0m");
                position_cursor(posBtnMaximize.row, posBtnMaximize.col);
                printf(CSI "0m" "☐" CSI "0m");

                // Print title
                position_cursor(0, 2);
                printf("%s", TITLE);

                // // Divider
                // position_cursor(canvas.row / 2, 0);
                // printf("├");
                // for (i = 0; i < canvas.col-2; i++) printf("─");
                // printf("┤");
                // position_cursor(canvas.row / 2, 2);
                // printf("{divider text}");

                // Print help
                if (!TUI_shrinkToFit)
                {
                    position_cursor(canvas.row-1, 2);
                    for (i = 0; i < sizeof(help) / sizeof(*help); i++)
                    {
                        // printf(" ");
                        printf("%s", help[i].desc);
                        // printf(" ");
                        // printf("[%s]", help[i].key);
                        // printf(" ");
                        printf(CSI "C"); // Cursor forward
                    }
                }
            }

            // Print knobs
            for (i = 0; i < N_DUMMY_KNOBS; i++)
            {
                knob_t * k_p = &knobs[i];
                position_cursor(k_p->r, k_p->c);
                if (i == knob_sel && TUI_inFocus) printf(CSI "7m");
                printf("%*d", knob_width, knob_update(k_p));
                position_cursor(k_p->r+1, k_p->c);
                printf(CSI "0;2m" "%*s" CSI "0m", knob_width, k_p->desc);
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
        // fflush(stdout);
        // usleep(10*1000);

        if (popup)
        {
            FILE * fp;
            char * line = NULL;
            size_t len = 0;
            ssize_t n;
            ssize_t rows, cols;

            popup = false;
            file_dimensions(popup_path, &rows, &cols);

            // Check boundaries
            if (!(rows <= (win_curr.ws_row-3) && cols <= (win_curr.ws_col-3)))
            {
                sprintf(msg, "Terminal window too small for popup");
                // TUI_keepRunning = false;
            }
            else
            {
                r = (win_curr.ws_row/2) - (rows/2) - 2;
                c = (win_curr.ws_col/2) - (cols/2) - 2;
                position_cursor(r, c);
                printf("┌");
                for (i = 0; i < cols+2-2; i++) printf("─");
                printf("┐");


                fp = fopen(popup_path, "r");
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

            fflush(stdout);
            // while (get_keypress_from_queue()) { /* Flush residual keypresses */ }
            while (!get_keypress_from_queue()) { /* Wait for keypress */ }
        }

        fflush(stdout);

        if (prompt)
        {
            prompt = false;
            redraw = true;
            fflush(stdout);
            cleanup();
            printf("...\r\n");
            system("neofetch > log.txt");
            printFile("log.txt");
            printf("Press enter to continue");
            fflush(stdout);
            while (!get_keypress_from_queue()) usleep(10*1000);
            setup();
        }

        if (TUI_keepRunning && get_keypress_from_queue())
        {
            knob_t * k_p = &knobs[knob_sel];
            if (kbd->kp == KP_CHAR)
            {
                switch (tolower(kbd->c))
                {
                    case 'x': TUI_keepRunning = false; break;
                    case 'a':
                        knob_sel -= 1;
                        if (knob_sel < 0) knob_sel += N_DUMMY_KNOBS;
                        redraw = true;
                        break;
                    case 'd':
                        knob_sel += 1;
                        knob_sel %= N_DUMMY_KNOBS;
                        redraw = true;
                        break;
                    case 'p':
                        prompt = true;
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
                TUI_inFocus = (kbd->c == 'I');
                // sprintf(msg, "Focus changed to %s", TUI_inFocus?"in":"out");
                redraw = true;
            }
            else if (kbd->kp == KP_CLICK)
            {
                position_cursor(posMouse.row, posMouse.col);
                if (posMouse.row == posBtnClose.row && posMouse.col == posBtnClose.col)
                    TUI_keepRunning = false;
                else if (posMouse.row == posBtnMaximize.row && posMouse.col == posBtnMaximize.col)
                    TUI_shrinkToFit = !TUI_shrinkToFit;
                else
                    printf("✗");
            }
            else if (kbd->kp != KP_IGNORE)
            {
                sprintf(msg, "Unknown option %c [%d:%d]", kbd->c, kbd->kp, kbd->c);
            }
        }

        fflush(stdout);

        if (TUI_keepRunning && *msg != 0)
        {
            int temp_col = canvas.col/2 - strlen(msg)/2;
            if (0 <= temp_col)
            {
                position_cursor(canvas.row / 2, temp_col);
                printf(CSI "41m" "%s" CSI "0m", msg);
                memset(&msg, 0, sizeof(msg) / sizeof(*msg));
                fflush(stdout);
                // usleep(500*1000);
                redraw = true;
            }
            // else
            // {
            //     TUI_keepRunning = false;
            //     sprintf(exitReason, "Terminal window too narrow");
            // }
        }

        if (!redraw) usleep(10*1000);
    }
}

int main(int argc, char *argv[]) {
    // Register signal interrupt handler
    signal(SIGINT, intHandler);
    TUI_keepRunning = true;
    
    setup();
    tui();
    // pthread_join(thread, NULL);
    cleanup();
    if (*exitReason != 0) printf("%s\n", exitReason);
    // puts("Goodbye!");
    return 0;
}
