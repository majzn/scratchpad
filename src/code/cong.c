#include <stdio.h>
#include <stdlib.h>

#define MZN_TERM_IMPLEMENTATION
#include "cong.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(ms) usleep((ms) * 1000)
#define VK_ESCAPE 27
#define VK_RETURN 10
#endif

#define APP_COLOR_DEFAULT_BG (MZN_ATTR_BG_BLUE)
#define APP_COLOR_FRAME                                                        \
  (MZN_ATTR_FG_INTENSITY | MZN_ATTR_FG_WHITE | MZN_ATTR_BG_BLACK)
#define APP_COLOR_TEXT (MZN_ATTR_FG_WHITE | MZN_ATTR_BG_BLUE)
#define APP_COLOR_HIGHLIGHT                                                    \
  (MZN_ATTR_FG_BLACK | MZN_ATTR_BG_RED | MZN_ATTR_BG_INTENSITY)
#define APP_COLOR_PROGRESS (MZN_ATTR_FG_YELLOW | MZN_ATTR_BG_BLUE)

void draw_scene(int frame) {
  int w = MZN_SCREEN_WIDTH;
  int h = MZN_SCREEN_HEIGHT;
  int mid_x = w / 2;
  int mid_y = h / 2;

  mzn_term_clear_buffer(APP_COLOR_DEFAULT_BG);

  mzn_term_draw_hline(0, 0, w, '=', APP_COLOR_FRAME);
  mzn_term_draw_hline(0, h - 1, w, '=', APP_COLOR_FRAME);

  const char *title = "MZN_TERM.H Cross-Platform Graphics Demo";
  mzn_term_draw_string(mid_x - (int)strlen(title) / 2, 0, title,
                       APP_COLOR_FRAME);

  mzn_term_draw_string(2, 2, "Normal Text (FG White)", APP_COLOR_TEXT);
  mzn_term_draw_string(2, 4, "Error Text (FG Intense Red on BG Blue)",
                       MZN_ATTR_FG_INTENSITY | MZN_ATTR_FG_RED |
                           MZN_ATTR_BG_BLUE);
  mzn_term_draw_string(2, 6, "Highlight (FG Black on BG Intense Red)",
                       APP_COLOR_HIGHLIGHT);
  mzn_term_draw_string(2, 8, "Bright Green Text",
                       MZN_ATTR_FG_INTENSITY | MZN_ATTR_FG_GREEN |
                           APP_COLOR_DEFAULT_BG);

  char progress_bar[22];
  memset(progress_bar, '-', sizeof(progress_bar));
  progress_bar[0] = '[';
  progress_bar[sizeof(progress_bar) - 2] = ']';
  progress_bar[sizeof(progress_bar) - 1] = '\0';

  int bar_len = 20;
  int filled_len = (frame % bar_len) + 1;

  for (int i = 0; i < filled_len && i < bar_len; i++) {
    progress_bar[i + 1] = '#';
  }

  mzn_term_draw_string(mid_x - bar_len / 2 - 7, mid_y,
                       "Frame:", APP_COLOR_TEXT);
  mzn_term_draw_string(mid_x - bar_len / 2, mid_y, progress_bar,
                       APP_COLOR_PROGRESS);

  mzn_term_draw_string(
      2, h - 3,
#ifdef _WIN32
      "Running on Win32 (Windows Console API) | Press ESC to Exit"
#else
      "Running on POSIX (ncurses) | Press ESC or 'q' to Exit"
#endif
      ,
      APP_COLOR_TEXT);

  mzn_term_draw();
  mzn_term_flip_buffer();
}

int handle_input() {
#ifdef _WIN32
  HANDLE hConsoleInput = mzn_term_get_input_handle();
  INPUT_RECORD irBuffer[1];
  DWORD dwEventsRead;

  PeekConsoleInput(hConsoleInput, irBuffer, 1, &dwEventsRead);
  if (dwEventsRead > 0) {
    ReadConsoleInput(hConsoleInput, irBuffer, 1, &dwEventsRead);
    if (irBuffer[0].EventType == KEY_EVENT &&
        irBuffer[0].Event.KeyEvent.bKeyDown) {
      WORD vk = irBuffer[0].Event.KeyEvent.wVirtualKeyCode;
      if (vk == VK_ESCAPE) {
        return 0;
      }
    }
  }
  return 1;

#else
  int ch = getch();
  if (ch == VK_ESCAPE || ch == 'q' || ch == 'Q') {
    return 0;
  }
  return 1;
#endif
}

int main(void) {
  int frame_count = 0;
  int running = 1;

  mzn_term_init(MZN_ATTR_BG_BLACK);

  while (running) {
    draw_scene(frame_count++);
    running = handle_input();
    Sleep(50);
  }

#ifndef _WIN32
  endwin();
#endif

  return 0;
}
