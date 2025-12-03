#define MZN_TERM_IMPLEMENTATION
#include "cong.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#define MAX_LOG_LINES 10

char log_buffer[MAX_LOG_LINES][80];
int log_count = 0;

void log_event(const char *msg)
{
    int i;
    if (log_count < MAX_LOG_LINES) {
        log_count++;
    }
    for (i = 0; i < MAX_LOG_LINES - 1; i++) {
        strcpy(log_buffer[i], log_buffer[i + 1]);
    }
    strncpy(log_buffer[MAX_LOG_LINES - 1], msg, 79);
    log_buffer[MAX_LOG_LINES - 1][79] = '\0';
}

void draw_ui(int mouse_x, int mouse_y, int quit_flag)
{
    int i;
    char status_bar[80];
    char mouse_pos_str[32];
    int log_start_y = 5;
    
    mzn_term_clear_buffer(MZN_ATTR_BG_BLACK);

    mzn_term_draw_fill(1, 1, MZN_SCREEN_WIDTH - 2, 3, ' ', MZN_ATTR_BG_BLUE | MZN_ATTR_FG_WHITE);
    mzn_term_draw_string(2, 1, "C89 Interactive TUI Example", MZN_ATTR_FG_YELLOW | MZN_ATTR_BOLD);

    mzn_term_draw_box(1, 4, MZN_SCREEN_WIDTH - 2, MAX_LOG_LINES + 3, MZN_ATTR_FG_CYAN);
    mzn_term_draw_string(2, 5, "Event Log:", MZN_ATTR_FG_MAGENTA | MZN_ATTR_BOLD);

    for (i = 0; i < MAX_LOG_LINES; i++) {
        int log_index = i;
        if (log_index < log_count) {
            mzn_term_draw_string(3, 7 + i, log_buffer[log_index], MZN_ATTR_FG_WHITE);
        }
    }

    mzn_term_draw_hline(0, MZN_SCREEN_HEIGHT - 2, MZN_SCREEN_WIDTH, 196, MZN_ATTR_FG_GREEN);

    mzn_term_draw_string(2, 3, "Press 'q' or ESC to quit.", MZN_ATTR_FG_WHITE);

    sprintf(mouse_pos_str, "Mouse: (%d, %d)", mouse_x, mouse_y);
    mzn_term_draw_string(2, MZN_SCREEN_HEIGHT - 1, mouse_pos_str, MZN_ATTR_FG_CYAN);

    if (quit_flag) {
        strcpy(status_bar, "Application is shutting down...");
    } else {
        strcpy(status_bar, "Ready. Use mouse or keyboard.");
    }
    mzn_term_draw_string(MZN_SCREEN_WIDTH - strlen(status_bar) - 2, MZN_SCREEN_HEIGHT - 1, status_bar, MZN_ATTR_FG_GREEN | MZN_ATTR_BOLD);

    mzn_term_draw();
    mzn_term_flip_buffer();
}

int main(void)
{
    int running = 1;
    int mouse_x = 0;
    int mouse_y = 0;
    MZN_Event event;
    char msg[80];

    mzn_term_init(MZN_ATTR_FG_WHITE | MZN_ATTR_BG_BLACK);

    log_event("Application initialized.");

    while (running) {
        event = mzn_term_poll_event();

        switch (event.type) {
            case MZN_EVENT_QUIT:
                running = 0;
                break;
            case MZN_EVENT_KEY_DOWN:
                if (event.key_code >= MZN_KEY_LEFT) {
                    sprintf(msg, "Special Key Down: Code %d (Ctrl: %d, Alt: %d)", event.key_code, event.ctrl, event.alt);
                } else if (event.key_code >= 32 && event.key_code <= 126) {
                    sprintf(msg, "Char Key Down: '%c' (Ctrl: %d, Shift: %d)", event.character, event.ctrl, event.shift);
                } else {
                     sprintf(msg, "Control Key Down: Code %d", event.key_code);
                }
                log_event(msg);
                break;
            case MZN_EVENT_MOUSE_CLICK:
                mouse_x = event.x;
                mouse_y = event.y;
                sprintf(msg, "Mouse Click B%d: (%d, %d) (Ctrl: %d)", event.button, mouse_x, mouse_y, event.ctrl);
                log_event(msg);
                break;
            case MZN_EVENT_MOUSE_MOVE:
                mouse_x = event.x;
                mouse_y = event.y;
                /* Only log move if within 5 lines of the top to prevent log spam */
                if (mouse_y < 5) {
                    sprintf(msg, "Mouse Move: (%d, %d)", mouse_x, mouse_y);
                    log_event(msg);
                }
                break;
            case MZN_EVENT_MOUSE_WHEEL:
                sprintf(msg, "Mouse Wheel Delta: %d", event.wheel_delta);
                log_event(msg);
                break;
            case MZN_EVENT_RESIZE:
                sprintf(msg, "Resize: %d x %d", event.w, event.h);
                log_event(msg);
                break;
            case MZN_EVENT_NONE:
                mzn_term_sleep_ms(10);
                break;
            default:
                break;
        }

        draw_ui(mouse_x, mouse_y, !running);
    }

    mzn_term_shutdown();
    
    return 0;
}