#ifndef MZN_TERM_H
#define MZN_TERM_H

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>

#define MZN_ATTR_FG_BLUE FOREGROUND_BLUE
#define MZN_ATTR_FG_GREEN FOREGROUND_GREEN
#define MZN_ATTR_FG_RED FOREGROUND_RED
#define MZN_ATTR_FG_INTENSITY FOREGROUND_INTENSITY
#define MZN_ATTR_BG_BLUE BACKGROUND_BLUE
#define MZN_ATTR_BG_GREEN BACKGROUND_GREEN
#define MZN_ATTR_BG_RED BACKGROUND_RED
#define MZN_ATTR_BG_INTENSITY BACKGROUND_INTENSITY

#else
#include <curses.h>
#include <unistd.h>

#define MZN_ATTR_FG_BLUE 0x01
#define MZN_ATTR_FG_GREEN 0x02
#define MZN_ATTR_FG_RED 0x04
#define MZN_ATTR_FG_INTENSITY 0x08
#define MZN_ATTR_BG_BLUE 0x10
#define MZN_ATTR_BG_GREEN 0x20
#define MZN_ATTR_BG_RED 0x40
#define MZN_ATTR_BG_INTENSITY 0x80

#endif

#define MZN_ATTR_FG_BLACK (0)
#define MZN_ATTR_FG_WHITE                                                      \
  (MZN_ATTR_FG_RED | MZN_ATTR_FG_GREEN | MZN_ATTR_FG_BLUE)
#define MZN_ATTR_FG_YELLOW (MZN_ATTR_FG_RED | MZN_ATTR_FG_GREEN)
#define MZN_ATTR_FG_CYAN (MZN_ATTR_FG_BLUE | MZN_ATTR_FG_GREEN)
#define MZN_ATTR_FG_MAGENTA (MZN_ATTR_FG_RED | MZN_ATTR_FG_BLUE)

#define MZN_ATTR_BG_BLACK (0)
#define MZN_ATTR_BG_WHITE                                                      \
  (MZN_ATTR_BG_RED | MZN_ATTR_BG_GREEN | MZN_ATTR_BG_BLUE)
#define MZN_ATTR_BG_YELLOW (MZN_ATTR_BG_RED | MZN_ATTR_BG_GREEN)
#define MZN_ATTR_BG_CYAN (MZN_ATTR_BG_BLUE | MZN_ATTR_BG_GREEN)
#define MZN_ATTR_BG_MAGENTA (MZN_ATTR_BG_RED | MZN_ATTR_BG_BLUE)

#define MZN_SCREEN_WIDTH 120
#define MZN_SCREEN_HEIGHT 30
#define MZN_BUFFER_SIZE (MZN_SCREEN_WIDTH * MZN_SCREEN_HEIGHT)

void mzn_term_init(int initial_attr);
void mzn_term_clear_buffer(int background_attr);
void mzn_term_set_char(int x, int y, char c, int attr);
void mzn_term_draw_string(int x, int y, const char *s, int attr);
void mzn_term_draw_hline(int x, int y, int len, char c, int attr);
void mzn_term_draw(void);
void mzn_term_flip_buffer(void);

#ifdef _WIN32
HANDLE mzn_term_get_input_handle(void);
#endif

#ifdef MZN_TERM_IMPLEMENTATION

static struct {
#ifdef _WIN32
  HANDLE hConsoleInput;
  HANDLE hScreenBuffers[2];
  int nActiveBuffer;
  CHAR_INFO ciBuffer[MZN_BUFFER_SIZE];
  COORD dwBufferSize;
  COORD dwBufferCoord;
  SMALL_RECT rcRegion;
#endif
  int current_bg_attr;
} MZN_TERM_GLOBAL;

#ifdef _WIN32

void mzn_term_init(int initial_attr) {
  MZN_TERM_GLOBAL.hConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
  MZN_TERM_GLOBAL.hScreenBuffers[0] = CreateConsoleScreenBuffer(
      GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
      CONSOLE_TEXTMODE_BUFFER, NULL);
  MZN_TERM_GLOBAL.hScreenBuffers[1] = CreateConsoleScreenBuffer(
      GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
      CONSOLE_TEXTMODE_BUFFER, NULL);
  MZN_TERM_GLOBAL.nActiveBuffer = 0;
  SetConsoleActiveScreenBuffer(
      MZN_TERM_GLOBAL.hScreenBuffers[MZN_TERM_GLOBAL.nActiveBuffer]);

  MZN_TERM_GLOBAL.dwBufferSize.X = MZN_SCREEN_WIDTH;
  MZN_TERM_GLOBAL.dwBufferSize.Y = MZN_SCREEN_HEIGHT;
  MZN_TERM_GLOBAL.dwBufferCoord.X = 0;
  MZN_TERM_GLOBAL.dwBufferCoord.Y = 0;
  MZN_TERM_GLOBAL.rcRegion.Left = 0;
  MZN_TERM_GLOBAL.rcRegion.Top = 0;
  MZN_TERM_GLOBAL.rcRegion.Right = MZN_SCREEN_WIDTH - 1;
  MZN_TERM_GLOBAL.rcRegion.Bottom = MZN_SCREEN_HEIGHT - 1;

  MZN_TERM_GLOBAL.current_bg_attr = initial_attr;
  mzn_term_clear_buffer(initial_attr);
}

HANDLE mzn_term_get_input_handle(void) { return MZN_TERM_GLOBAL.hConsoleInput; }

void mzn_term_clear_buffer(int background_attr) {
  int i;
  MZN_TERM_GLOBAL.current_bg_attr = background_attr;
  for (i = 0; i < MZN_BUFFER_SIZE; i++) {
    MZN_TERM_GLOBAL.ciBuffer[i].Char.AsciiChar = ' ';
    MZN_TERM_GLOBAL.ciBuffer[i].Attributes = (WORD)background_attr;
  }
}

void mzn_term_set_char(int x, int y, char c, int attr) {
  if (x >= 0 && x < MZN_SCREEN_WIDTH && y >= 0 && y < MZN_SCREEN_HEIGHT) {
    int i = y * MZN_SCREEN_WIDTH + x;
    MZN_TERM_GLOBAL.ciBuffer[i].Char.AsciiChar = c;
    MZN_TERM_GLOBAL.ciBuffer[i].Attributes = (WORD)attr;
  }
}

void mzn_term_draw_string(int x, int y, const char *s, int attr) {
  int i;
  int len = (int)strlen(s);
  for (i = 0; i < len && x + i < MZN_SCREEN_WIDTH; i++) {
    mzn_term_set_char(x + i, y, s[i], attr);
  }
}

void mzn_term_draw_hline(int x, int y, int len, char c, int attr) {
  int i;
  for (i = 0; i < len; i++) {
    mzn_term_set_char(x + i, y, c, attr);
  }
}

void mzn_term_draw(void) {
  WriteConsoleOutput(
      MZN_TERM_GLOBAL.hScreenBuffers[1 - MZN_TERM_GLOBAL.nActiveBuffer],
      MZN_TERM_GLOBAL.ciBuffer, MZN_TERM_GLOBAL.dwBufferSize,
      MZN_TERM_GLOBAL.dwBufferCoord, &MZN_TERM_GLOBAL.rcRegion);
}

void mzn_term_flip_buffer(void) {
  SetConsoleActiveScreenBuffer(
      MZN_TERM_GLOBAL.hScreenBuffers[1 - MZN_TERM_GLOBAL.nActiveBuffer]);
  MZN_TERM_GLOBAL.nActiveBuffer = 1 - MZN_TERM_GLOBAL.nActiveBuffer;
}

#else

static attr_t mzn_term_apply_attr_posix(int attr) {
  attr_t nc_attr = A_NORMAL;

  if (attr & MZN_ATTR_FG_INTENSITY) {
    nc_attr |= A_BOLD;
  }

  int fg_idx = (attr & 0x07);
  int bg_idx = (attr & 0x70) >> 4;

  int pair_idx = (bg_idx * 8) + fg_idx + 1;

  if (pair_idx > 64)
    pair_idx = 64;

  nc_attr |= COLOR_PAIR(pair_idx);

  return nc_attr;
}

void mzn_term_init(int initial_attr) {
  if (initscr() == NULL) {
    fprintf(stderr, "Error initializing ncurses.\n");
    return;
  }

  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  nodelay(stdscr, TRUE);

  if (has_colors()) {
    start_color();

    short nc_colors[8] = {COLOR_BLACK,  COLOR_BLUE, COLOR_GREEN,
                          COLOR_CYAN,   COLOR_RED,  COLOR_MAGENTA,
                          COLOR_YELLOW, COLOR_WHITE};

    int pair_idx = 1;
    for (int bg_win_idx = 0; bg_win_idx < 8; bg_win_idx++) {
      for (int fg_win_idx = 0; fg_win_idx < 8; fg_win_idx++) {
        init_pair(pair_idx, nc_colors[fg_win_idx], nc_colors[bg_win_idx]);
        pair_idx++;
      }
    }
  }

  MZN_TERM_GLOBAL.current_bg_attr = initial_attr;
  mzn_term_clear_buffer(initial_attr);
}

void mzn_term_clear_buffer(int background_attr) {
  MZN_TERM_GLOBAL.current_bg_attr = background_attr;
  attr_t bg_attr = mzn_term_apply_attr_posix(background_attr);

  attrset(bg_attr);
  erase();
}

void mzn_term_set_char(int x, int y, char c, int attr) {
  if (x >= 0 && x < MZN_SCREEN_WIDTH && y >= 0 && y < MZN_SCREEN_HEIGHT) {
    attr_t nc_attr = mzn_term_apply_attr_posix(attr);

    attron(nc_attr);
    mvaddch(y, x, c);
    attroff(nc_attr);
  }
}

void mzn_term_draw_string(int x, int y, const char *s, int attr) {
  if (y >= 0 && y < MZN_SCREEN_HEIGHT) {
    attr_t nc_attr = mzn_term_apply_attr_posix(attr);

    attron(nc_attr);
    mvaddnstr(y, x, s, MZN_SCREEN_WIDTH - x);
    attroff(nc_attr);
  }
}

void mzn_term_draw_hline(int x, int y, int len, char c, int attr) {
  if (y >= 0 && y < MZN_SCREEN_HEIGHT) {
    attr_t nc_attr = mzn_term_apply_attr_posix(attr);

    attron(nc_attr);

    for (int i = 0; i < len; i++) {
      if (x + i < MZN_SCREEN_WIDTH) {
        mvaddch(y, x + i, c);
      }
    }

    attroff(nc_attr);
  }
}

void mzn_term_draw(void) {}

void mzn_term_flip_buffer(void) { refresh(); }

#endif

#endif

#endif
