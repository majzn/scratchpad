#ifndef TUI_CORE_H
#define TUI_CORE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <curses.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <unistd.h>
#endif

#define MZN_ATTR_FG_BLUE 0x01
#define MZN_ATTR_FG_GREEN 0x02
#define MZN_ATTR_FG_RED 0x04
#define MZN_ATTR_FG_INTENSITY 0x08
#define MZN_ATTR_BG_BLUE 0x10
#define MZN_ATTR_BG_GREEN 0x20
#define MZN_ATTR_BG_RED 0x40
#define MZN_ATTR_BG_INTENSITY 0x80
#define MZN_ATTR_BOLD MZN_ATTR_FG_INTENSITY

#define MZN_ATTR_FG_BLACK (0)
#define MZN_ATTR_FG_WHITE (MZN_ATTR_FG_RED | MZN_ATTR_FG_GREEN | MZN_ATTR_FG_BLUE)
#define MZN_ATTR_FG_YELLOW (MZN_ATTR_FG_RED | MZN_ATTR_FG_GREEN)
#define MZN_ATTR_FG_CYAN (MZN_ATTR_FG_BLUE | MZN_ATTR_FG_GREEN)
#define MZN_ATTR_FG_MAGENTA (MZN_ATTR_FG_RED | MZN_ATTR_FG_BLUE)

#define MZN_ATTR_BG_BLACK (0)
#define MZN_ATTR_BG_WHITE (MZN_ATTR_BG_RED | MZN_ATTR_BG_GREEN | MZN_ATTR_BG_BLUE)
#define MZN_ATTR_BG_YELLOW (MZN_ATTR_BG_RED | MZN_ATTR_BG_GREEN)
#define MZN_ATTR_BG_CYAN (MZN_ATTR_BG_BLUE | MZN_ATTR_BG_GREEN)
#define MZN_ATTR_BG_MAGENTA (MZN_ATTR_BG_RED | MZN_ATTR_BG_BLUE)

enum MZN_EventType {
    MZN_EVENT_NONE,
    MZN_EVENT_KEY_DOWN,
    MZN_EVENT_KEY_UP,
    MZN_EVENT_MOUSE_MOVE,
    MZN_EVENT_MOUSE_CLICK,
    MZN_EVENT_MOUSE_WHEEL,
    MZN_EVENT_RESIZE,
    MZN_EVENT_QUIT
};

enum MZN_KeyCode {
    MZN_KEY_UNKNOWN = 0,
    MZN_KEY_ENTER = 13,
    MZN_KEY_ESC = 27,
    MZN_KEY_BACKSPACE = 8,
    MZN_KEY_TAB = 9,
    MZN_KEY_LEFT = 256,
    MZN_KEY_RIGHT,
    MZN_KEY_UP,
    MZN_KEY_DOWN,
    MZN_KEY_HOME,
    MZN_KEY_END,
    MZN_KEY_INSERT,
    MZN_KEY_DELETE,
    MZN_KEY_PAGE_UP,
    MZN_KEY_PAGE_DOWN,
    MZN_KEY_F1,
    MZN_KEY_F2,
    MZN_KEY_F3,
    MZN_KEY_F4,
    MZN_KEY_F5,
    MZN_KEY_F6,
    MZN_KEY_F7,
    MZN_KEY_F8,
    MZN_KEY_F9,
    MZN_KEY_F10,
    MZN_KEY_F11,
    MZN_KEY_F12
};

typedef struct {
    enum MZN_EventType type;
    int key_code;
    char character;
    int x;
    int y;
    int w;
    int h;
    int ctrl;
    int alt;
    int shift;
    int button; 
    int wheel_delta;
} MZN_Event;

extern int MZN_SCREEN_WIDTH;
extern int MZN_SCREEN_HEIGHT;

void mzn_term_init(int initial_attr);
void mzn_term_shutdown(void);
void mzn_term_resize(int w, int h);
void mzn_term_clear_buffer(int background_attr);
void mzn_term_set_char(int x, int y, char c, int attr);
void mzn_term_draw_string(int x, int y, const char *s, int attr);
void mzn_term_draw_hline(int x, int y, int len, char c, int attr);
void mzn_term_draw_vline(int x, int y, int len, char c, int attr);
void mzn_term_draw_box(int x, int y, int w, int h, int attr);
void mzn_term_draw_fill(int x, int y, int w, int h, char c, int attr);
void mzn_term_draw(void);
void mzn_term_flip_buffer(void);
MZN_Event mzn_term_poll_event(void);
void mzn_term_sleep_ms(long ms);

#ifdef MZN_TERM_IMPLEMENTATION

int MZN_SCREEN_WIDTH = 80;
int MZN_SCREEN_HEIGHT = 24;

#ifdef _WIN32
static HANDLE hStdin_global;
static DWORD fdwSaveOldMode_global;
#endif

static struct {
#ifdef _WIN32
  HANDLE hScreenBuffers[2];
  int nActiveBuffer;
  CHAR_INFO *ciBuffer;
  COORD dwBufferSize;
  COORD dwBufferCoord;
  SMALL_RECT rcRegion;
  int current_bg_attr;
#else
  int current_bg_attr;
#endif
} MZN_TERM_GLOBAL;

#ifdef _WIN32

static int mzn_win_vkey_to_mzn_key(WORD vkey)
{
    switch (vkey)
    {
        case VK_LEFT: return MZN_KEY_LEFT;
        case VK_RIGHT: return MZN_KEY_RIGHT;
        case VK_UP: return MZN_KEY_UP;
        case VK_DOWN: return MZN_KEY_DOWN;
        case VK_HOME: return MZN_KEY_HOME;
        case VK_END: return MZN_KEY_END;
        case VK_INSERT: return MZN_KEY_INSERT;
        case VK_DELETE: return MZN_KEY_DELETE;
        case VK_PRIOR: return MZN_KEY_PAGE_UP;
        case VK_NEXT: return MZN_KEY_PAGE_DOWN;
        case VK_F1: return MZN_KEY_F1;
        case VK_F2: return MZN_KEY_F2;
        case VK_F3: return MZN_KEY_F3;
        case VK_F4: return MZN_KEY_F4;
        case VK_F5: return MZN_KEY_F5;
        case VK_F6: return MZN_KEY_F6;
        case VK_F7: return MZN_KEY_F7;
        case VK_F8: return MZN_KEY_F8;
        case VK_F9: return MZN_KEY_F9;
        case VK_F10: return MZN_KEY_F10;
        case VK_F11: return MZN_KEY_F11;
        case VK_F12: return MZN_KEY_F12;
        default: return 0;
    }
}

void mzn_term_init(int initial_attr) {
    DWORD fdwMode;

    hStdin_global = GetStdHandle(STD_INPUT_HANDLE);
    if (hStdin_global == INVALID_HANDLE_VALUE) {
        return;
    }
    if (! GetConsoleMode(hStdin_global, &fdwSaveOldMode_global) ) {
        return;
    }

    fdwMode = (ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT | ENABLE_EXTENDED_FLAGS) & ~ENABLE_QUICK_EDIT_MODE;
    if (! SetConsoleMode(hStdin_global, fdwMode) ) {
        return;
    }

    MZN_TERM_GLOBAL.hScreenBuffers[0] = CreateConsoleScreenBuffer(
        GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
        CONSOLE_TEXTMODE_BUFFER, NULL);
    MZN_TERM_GLOBAL.hScreenBuffers[1] = CreateConsoleScreenBuffer(
        GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
        CONSOLE_TEXTMODE_BUFFER, NULL);

    MZN_TERM_GLOBAL.nActiveBuffer = 0;
    SetConsoleActiveScreenBuffer(
        MZN_TERM_GLOBAL.hScreenBuffers[MZN_TERM_GLOBAL.nActiveBuffer]);

    MZN_TERM_GLOBAL.ciBuffer = NULL;
    MZN_TERM_GLOBAL.current_bg_attr = initial_attr;
    mzn_term_resize(MZN_SCREEN_WIDTH, MZN_SCREEN_HEIGHT);
}

void mzn_term_shutdown(void) {
    SetConsoleMode(hStdin_global, fdwSaveOldMode_global);
    if (MZN_TERM_GLOBAL.ciBuffer) free(MZN_TERM_GLOBAL.ciBuffer);
    CloseHandle(MZN_TERM_GLOBAL.hScreenBuffers[0]);
    CloseHandle(MZN_TERM_GLOBAL.hScreenBuffers[1]);
}

void mzn_term_resize(int w, int h) {
  if (w < 1) w = 1;
  if (h < 1) h = 1;

  if (MZN_TERM_GLOBAL.ciBuffer)
    free(MZN_TERM_GLOBAL.ciBuffer);

  MZN_SCREEN_WIDTH = w;
  MZN_SCREEN_HEIGHT = h;

  MZN_TERM_GLOBAL.ciBuffer = (CHAR_INFO *)malloc(w * h * sizeof(CHAR_INFO));

  MZN_TERM_GLOBAL.dwBufferSize.X = w;
  MZN_TERM_GLOBAL.dwBufferSize.Y = h;
  MZN_TERM_GLOBAL.dwBufferCoord.X = 0;
  MZN_TERM_GLOBAL.dwBufferCoord.Y = 0;
  MZN_TERM_GLOBAL.rcRegion.Left = 0;
  MZN_TERM_GLOBAL.rcRegion.Top = 0;
  MZN_TERM_GLOBAL.rcRegion.Right = w - 1;
  MZN_TERM_GLOBAL.rcRegion.Bottom = h - 1;

  SetConsoleScreenBufferSize(MZN_TERM_GLOBAL.hScreenBuffers[0], MZN_TERM_GLOBAL.dwBufferSize);
  SetConsoleScreenBufferSize(MZN_TERM_GLOBAL.hScreenBuffers[1], MZN_TERM_GLOBAL.dwBufferSize);
  
  SetConsoleWindowInfo(MZN_TERM_GLOBAL.hScreenBuffers[0], TRUE, &MZN_TERM_GLOBAL.rcRegion);
  SetConsoleWindowInfo(MZN_TERM_GLOBAL.hScreenBuffers[1], TRUE, &MZN_TERM_GLOBAL.rcRegion);

  {
      CONSOLE_CURSOR_INFO cursorInfo;
      cursorInfo.bVisible = FALSE;
      cursorInfo.dwSize = 1;
      SetConsoleCursorInfo(MZN_TERM_GLOBAL.hScreenBuffers[0], &cursorInfo);
      SetConsoleCursorInfo(MZN_TERM_GLOBAL.hScreenBuffers[1], &cursorInfo);
  }

  SetConsoleActiveScreenBuffer(
      MZN_TERM_GLOBAL.hScreenBuffers[MZN_TERM_GLOBAL.nActiveBuffer]);
  mzn_term_clear_buffer(MZN_TERM_GLOBAL.current_bg_attr);
}

void mzn_term_clear_buffer(int background_attr) {
  int i;
  int size = MZN_SCREEN_WIDTH * MZN_SCREEN_HEIGHT;
  MZN_TERM_GLOBAL.current_bg_attr = background_attr;
  if (!MZN_TERM_GLOBAL.ciBuffer)
    return;

  {
      CHAR_INFO fill;
      fill.Char.AsciiChar = ' ';
      fill.Attributes = (WORD)background_attr;

      for (i = 0; i < size; i++) {
        MZN_TERM_GLOBAL.ciBuffer[i] = fill;
      }
  }
}

void mzn_term_set_char(int x, int y, char c, int attr) {
  if (x >= 0 && x < MZN_SCREEN_WIDTH && y >= 0 && y < MZN_SCREEN_HEIGHT) {
    int i = y * MZN_SCREEN_WIDTH + x;
    if (MZN_TERM_GLOBAL.ciBuffer) {
      MZN_TERM_GLOBAL.ciBuffer[i].Char.AsciiChar = c;
      MZN_TERM_GLOBAL.ciBuffer[i].Attributes = (WORD)attr;
    }
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

void mzn_term_draw_vline(int x, int y, int len, char c, int attr) {
  int i;
  for (i = 0; i < len; i++) {
    mzn_term_set_char(x, y + i, c, attr);
  }
}

void mzn_term_draw_box(int x, int y, int w, int h, int attr) {
    if (w < 2 || h < 2) return;
    
    mzn_term_draw_hline(x + 1, y, w - 2, 196, attr);
    mzn_term_draw_hline(x + 1, y + h - 1, w - 2, 196, attr);
    mzn_term_draw_vline(x, y + 1, h - 2, 179, attr);
    mzn_term_draw_vline(x + w - 1, y + 1, h - 2, 179, attr);

    mzn_term_set_char(x, y, 218, attr);
    mzn_term_set_char(x + w - 1, y, 191, attr);
    mzn_term_set_char(x, y + h - 1, 192, attr);
    mzn_term_set_char(x + w - 1, y + h - 1, 217, attr);
}

void mzn_term_draw_fill(int x, int y, int w, int h, char c, int attr) {
    int i, j;
    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++) {
            mzn_term_set_char(x + i, y + j, c, attr);
        }
    }
}

void mzn_term_draw(void) {
  if (MZN_TERM_GLOBAL.ciBuffer) {
    WriteConsoleOutput(
        MZN_TERM_GLOBAL.hScreenBuffers[1 - MZN_TERM_GLOBAL.nActiveBuffer],
        MZN_TERM_GLOBAL.ciBuffer, MZN_TERM_GLOBAL.dwBufferSize,
        MZN_TERM_GLOBAL.dwBufferCoord, &MZN_TERM_GLOBAL.rcRegion);
  }
}

void mzn_term_flip_buffer(void) {
  SetConsoleActiveScreenBuffer(
      MZN_TERM_GLOBAL.hScreenBuffers[1 - MZN_TERM_GLOBAL.nActiveBuffer]);
  MZN_TERM_GLOBAL.nActiveBuffer = 1 - MZN_TERM_GLOBAL.nActiveBuffer;
}

MZN_Event mzn_term_poll_event(void) {
    DWORD cNumRead, i;
    INPUT_RECORD irInBuf[128];
    MZN_Event event = {MZN_EVENT_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    if (! ReadConsoleInput(hStdin_global, irInBuf, 128, &cNumRead) )
        return event;

    for (i = 0; i < cNumRead; i++)
    {
        switch(irInBuf[i].EventType)
        {
            case KEY_EVENT:
                if (irInBuf[i].Event.KeyEvent.bKeyDown) {
                    KEY_EVENT_RECORD ker = irInBuf[i].Event.KeyEvent;
                    event.type = MZN_EVENT_KEY_DOWN;
                    event.character = ker.uChar.AsciiChar;
                    event.key_code = ker.uChar.AsciiChar ? (int)ker.uChar.AsciiChar : mzn_win_vkey_to_mzn_key(ker.wVirtualKeyCode);
                    event.ctrl = (ker.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;
                    event.alt = (ker.dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) != 0;
                    event.shift = (ker.dwControlKeyState & SHIFT_PRESSED) != 0;

                    if (event.key_code == MZN_KEY_ESC || event.character == 'q' || event.character == 'Q') {
                        event.type = MZN_EVENT_QUIT;
                    } else if (event.key_code == 0) {
                        event.key_code = mzn_win_vkey_to_mzn_key(ker.wVirtualKeyCode);
                    }
                } else {
                    event.type = MZN_EVENT_KEY_UP;
                    event.character = irInBuf[i].Event.KeyEvent.uChar.AsciiChar;
                }
                if (event.type != MZN_EVENT_NONE) return event;
                break;

            case MOUSE_EVENT:
                {
                    MOUSE_EVENT_RECORD mer = irInBuf[i].Event.MouseEvent;
                    event.x = mer.dwMousePosition.X;
                    event.y = mer.dwMousePosition.Y;
                    event.ctrl = (mer.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;
                    event.alt = (mer.dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) != 0;
                    event.shift = (mer.dwControlKeyState & SHIFT_PRESSED) != 0;

                    if (mer.dwEventFlags == MOUSE_MOVED) {
                        event.type = MZN_EVENT_MOUSE_MOVE;
                        return event;
                    } else if (mer.dwEventFlags == 0 || mer.dwEventFlags == DOUBLE_CLICK) {
                        if (mer.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) {
                            event.type = MZN_EVENT_MOUSE_CLICK;
                            event.button = 1;
                            return event;
                        } else if (mer.dwButtonState & RIGHTMOST_BUTTON_PRESSED) {
                            event.type = MZN_EVENT_MOUSE_CLICK;
                            event.button = 2;
                            return event;
                        }
                    } else if (mer.dwEventFlags == MOUSE_WHEELED || mer.dwEventFlags == 4 /*MOUSE_HWHEELED*/) {
                        event.type = MZN_EVENT_MOUSE_WHEEL;
                        event.wheel_delta = (short)HIWORD(mer.dwButtonState) / WHEEL_DELTA;
                        return event;
                    }
                }
                break;

            case WINDOW_BUFFER_SIZE_EVENT:
                event.type = MZN_EVENT_RESIZE;
                event.w = irInBuf[i].Event.WindowBufferSizeEvent.dwSize.X;
                event.h = irInBuf[i].Event.WindowBufferSizeEvent.dwSize.Y;
                mzn_term_resize(event.w, event.h);
                return event;

            case FOCUS_EVENT:
            case MENU_EVENT:
                break;

            default:
                break;
        }
    }
    return event;
}

void mzn_term_sleep_ms(long ms) {
    Sleep((DWORD)ms);
}

#else

static attr_t mzn_term_apply_attr_posix(int attr) {
  attr_t nc_attr = A_NORMAL;

  if (attr & MZN_ATTR_FG_INTENSITY) {
    nc_attr |= A_BOLD;
  }

  {
      int fg_idx = (attr & 0x07);
      int bg_idx = (attr & 0x70) >> 4;
      int pair_idx = (bg_idx * 8) + fg_idx + 1;
      if (pair_idx > 64)
          pair_idx = 64;
      nc_attr |= COLOR_PAIR(pair_idx);
  }
  return nc_attr;
}

static int mzn_posix_key_to_mzn_key(int posix_key)
{
    switch (posix_key)
    {
        case KEY_LEFT: return MZN_KEY_LEFT;
        case KEY_RIGHT: return MZN_KEY_RIGHT;
        case KEY_UP: return MZN_KEY_UP;
        case KEY_DOWN: return MZN_KEY_DOWN;
        case KEY_HOME: return MZN_KEY_HOME;
        case KEY_END: return MZN_KEY_END;
        case KEY_IC: return MZN_KEY_INSERT;
        case KEY_DC: return MZN_KEY_DELETE;
        case KEY_PPAGE: return MZN_KEY_PAGE_UP;
        case KEY_NPAGE: return MZN_KEY_PAGE_DOWN;
        case KEY_F(1): return MZN_KEY_F1;
        case KEY_F(2): return MZN_KEY_F2;
        case KEY_F(3): return MZN_KEY_F3;
        case KEY_F(4): return MZN_KEY_F4;
        case KEY_F(5): return MZN_KEY_F5;
        case KEY_F(6): return MZN_KEY_F6;
        case KEY_F(7): return MZN_KEY_F7;
        case KEY_F(8): return MZN_KEY_F8;
        case KEY_F(9): return MZN_KEY_F9;
        case KEY_F(10): return MZN_KEY_F10;
        case KEY_F(11): return MZN_KEY_F11;
        case KEY_F(12): return MZN_KEY_F12;
        default: return 0;
    }
}

void mzn_term_init(int initial_attr) {
  if (initscr() == NULL) {
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
    {
        int bg_win_idx;
        int fg_win_idx;
        for (bg_win_idx = 0; bg_win_idx < 8; bg_win_idx++) {
            for (fg_win_idx = 0; fg_win_idx < 8; fg_win_idx++) {
                init_pair(pair_idx, nc_colors[fg_win_idx], nc_colors[bg_win_idx]);
                pair_idx++;
            }
        }
    }
  }

  MZN_TERM_GLOBAL.current_bg_attr = initial_attr;
  mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
  printf("\033[?1003h\n");
}

void mzn_term_shutdown(void) {
    printf("\033[?1003l\n");
    endwin();
}

void mzn_term_resize(int w, int h) {
  MZN_SCREEN_WIDTH = w;
  MZN_SCREEN_HEIGHT = h;
  resizeterm(h, w);
  mzn_term_clear_buffer(MZN_TERM_GLOBAL.current_bg_attr);
  refresh();
}

void mzn_term_clear_buffer(int background_attr) {
  MZN_TERM_GLOBAL.current_bg_attr = background_attr;
  {
      attr_t bg_attr = mzn_term_apply_attr_posix(background_attr);
      bkgd(bg_attr);
      erase();
  }
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
    {
        int i;
        for (i = 0; i < len; i++) {
            if (x + i < MZN_SCREEN_WIDTH) {
                mvaddch(y, x + i, c);
            }
        }
    }
    attroff(nc_attr);
  }
}

void mzn_term_draw_vline(int x, int y, int len, char c, int attr) {
  int i;
  for (i = 0; i < len; i++) {
    mzn_term_set_char(x, y + i, c, attr);
  }
}

void mzn_term_draw_box(int x, int y, int w, int h, int attr) {
    if (w < 2 || h < 2) return;
    
    mzn_term_draw_hline(x + 1, y, w - 2, ACS_HLINE, attr);
    mzn_term_draw_hline(x + 1, y + h - 1, w - 2, ACS_HLINE, attr);
    mzn_term_draw_vline(x, y + 1, h - 2, ACS_VLINE, attr);
    mzn_term_draw_vline(x + w - 1, y + 1, h - 2, ACS_VLINE, attr);

    mzn_term_set_char(x, y, ACS_ULCORNER, attr);
    mzn_term_set_char(x + w - 1, y, ACS_URCORNER, attr);
    mzn_term_set_char(x, y + h - 1, ACS_LLCORNER, attr);
    mzn_term_set_char(x + w - 1, y + h - 1, ACS_LRCORNER, attr);
}

void mzn_term_draw_fill(int x, int y, int w, int h, char c, int attr) {
    int i, j;
    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++) {
            mzn_term_set_char(x + i, y + j, c, attr);
        }
    }
}

void mzn_term_draw(void) {}

void mzn_term_flip_buffer(void) { refresh(); }

MZN_Event mzn_term_poll_event(void) {
    int ch;
    MEVENT event_ncurses;
    MZN_Event event = {MZN_EVENT_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    ch = getch();

    if (ch != ERR) {
        if (ch == 'q' || ch == MZN_KEY_ESC) {
            event.type = MZN_EVENT_QUIT;
            return event;
        } else if (ch == KEY_RESIZE) {
            getmaxyx(stdscr, MZN_SCREEN_HEIGHT, MZN_SCREEN_WIDTH);
            mzn_term_resize(MZN_SCREEN_WIDTH, MZN_SCREEN_HEIGHT);
            event.type = MZN_EVENT_RESIZE;
            event.w = MZN_SCREEN_WIDTH;
            event.h = MZN_SCREEN_HEIGHT;
            return event;
        } else if (ch == KEY_MOUSE) {
            if (getmouse(&event_ncurses) == OK) {
                event.x = event_ncurses.x;
                event.y = event_ncurses.y;
                event.shift = (event_ncurses.bstate & A_BUTTON_SHIFT) != 0;
                event.ctrl = (event_ncurses.bstate & A_BUTTON_CTRL) != 0;
                event.alt = (event_ncurses.bstate & A_BUTTON_ALT) != 0;

                if (event_ncurses.bstate & BUTTON1_CLICKED) {
                    event.type = MZN_EVENT_MOUSE_CLICK;
                    event.button = 1;
                } else if (event_ncurses.bstate & BUTTON2_CLICKED) {
                    event.type = MZN_EVENT_MOUSE_CLICK;
                    event.button = 2;
                } else if (event_ncurses.bstate & BUTTON3_CLICKED) {
                    event.type = MZN_EVENT_MOUSE_CLICK;
                    event.button = 3;
                } else if (event_ncurses.bstate & REPORT_MOUSE_POSITION) {
                    event.type = MZN_EVENT_MOUSE_MOVE;
                } else if (event_ncurses.bstate & BUTTON4_CLICKED) {
                    event.type = MZN_EVENT_MOUSE_WHEEL;
                    event.wheel_delta = 1;
                } else if (event_ncurses.bstate & BUTTON5_CLICKED) {
                    event.type = MZN_EVENT_MOUSE_WHEEL;
                    event.wheel_delta = -1;
                }
                if (event.type != MZN_EVENT_NONE) return event;
            }
        } else {
            event.type = MZN_EVENT_KEY_DOWN;
            if (ch < 256) {
                event.character = (char)ch;
                event.key_code = ch;
            } else {
                event.key_code = mzn_posix_key_to_mzn_key(ch);
            }

            if (event.key_code == 0 && (ch >= 256)) {
                 if (ch == KEY_BACKSPACE) event.key_code = MZN_KEY_BACKSPACE;
                 else if (ch == KEY_ENTER || ch == '\n' || ch == '\r') event.key_code = MZN_KEY_ENTER;
            }
            return event;
        }
    }
    return event;
}

void mzn_term_sleep_ms(long ms) {
    usleep(ms * 1000);
}
#endif
#endif
#endif