/**
 * @file main.c
 * @brief Application entry point for the Chronos synthesizer TUI.
 *
 * Implements:
 * - Text Editor for Scripting
 * - Tracker Interface (Hex Grid)
 * - Oscilloscope/Spectral Visualizer
 * - Audio Engine Integration
 */

#define _CRT_SECURE_NO_WARNINGS
#define CR_IMPLEMENTATION
#include "chronos.h"

#define SS_ENGINE_IMPLEMENTATION
#include "ss.h"

#define MZN_TERM_IMPLEMENTATION
#include "cong.h"

#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

// --- Key Code Definitions ---
#define K_UP 1000
#define K_DOWN 1001
#define K_LEFT 1002
#define K_RIGHT 1003
#define K_HOME 1004
#define K_END 1005
#define K_DEL 1006
#define K_TAB 9
#define K_BACKSPACE 127
#define K_ENTER 13
#define K_ESC 27
#define K_PGUP 1007
#define K_PGDN 1008
#define K_CTRL_S 19
#define K_CTRL_O 15

// --- Constants ---
#define FFT_SIZE 256
#define WATERFALL_HISTORY 64
#define WATERFALL_SMOOTHING 0.8
#define M_PI 3.14159265358979323846

#ifndef MZN_ATTR_BOLD
#define MZN_ATTR_BOLD 0x08
#endif

// --- Globals ---
static int term_w = 80;
static int term_h = 24;
static int needs_resize = 1;

typedef struct {
  float r;
  float i;
} cpx;

// --- Platform Specific Input Handling ---
#ifndef _WIN32
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

static struct termios oldt, newt;

void set_non_blocking_input() {
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  newt.c_iflag &= ~(IXON | ICRNL);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
}

void restore_input() { tcsetattr(STDIN_FILENO, TCSANOW, &oldt); }

void handle_winch(int sig) {
  struct winsize w;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
    if (w.ws_col > 0 && w.ws_row > 0) {
      term_w = w.ws_col;
      term_h = w.ws_row;
      needs_resize = 1;
    }
  }
}

int get_key_evt() {
  unsigned char c;
  if (read(STDIN_FILENO, &c, 1) != 1)
    return -1;
  if (c == 27) {
    unsigned char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1)
      return K_ESC;
    if (read(STDIN_FILENO, &seq[1], 1) != 1)
      return K_ESC;
    if (seq[0] == '[') {
      if (seq[1] >= '0' && seq[1] <= '9') {
        if (read(STDIN_FILENO, &seq[2], 1) != 1)
          return K_ESC;
        if (seq[2] == '~') {
          switch (seq[1]) {
          case '1': return K_HOME;
          case '3': return K_DEL;
          case '4': return K_END;
          case '5': return K_PGUP;
          case '6': return K_PGDN;
          }
        }
      } else {
        switch (seq[1]) {
        case 'A': return K_UP;
        case 'B': return K_DOWN;
        case 'C': return K_RIGHT;
        case 'D': return K_LEFT;
        case 'H': return K_HOME;
        case 'F': return K_END;
        }
      }
    }
    return K_ESC;
  }
  if (c == 127 || c == 8) return K_BACKSPACE;
  if (c == 10 || c == 13) return K_ENTER;
  if (c == 9) return K_TAB;
  if (c == 19) return K_CTRL_S;
  if (c == 15) return K_CTRL_O;
  return c;
}
#else
#include <conio.h>
#include <windows.h>
static DWORD orig_console_mode;

void set_non_blocking_input() {
  HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
  GetConsoleMode(hIn, &orig_console_mode);
  DWORD mode = orig_console_mode;
  mode &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
  SetConsoleMode(hIn, mode);
}

void restore_input() {
  SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), orig_console_mode);
}

void check_resize() {
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
    int w = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    int h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    if (w != term_w || h != term_h) {
      term_w = w;
      term_h = h;
      needs_resize = 1;
    }
  }
}

int get_key_evt() {
  check_resize();
  if (_kbhit()) {
    int c = _getch();
    if (c == 0 || c == 0xE0) {
      c = _getch();
      switch (c) {
      case 72: return K_UP;
      case 80: return K_DOWN;
      case 75: return K_LEFT;
      case 77: return K_RIGHT;
      case 71: return K_HOME;
      case 79: return K_END;
      case 83: return K_DEL;
      case 73: return K_PGUP;
      case 81: return K_PGDN;
      }
    }
    if (c == 8) return K_BACKSPACE;
    if (c == 13) return K_ENTER;
    if (c == 27) return K_ESC;
    if (c == 9) return K_TAB;
    if (c == 19) return K_CTRL_S;
    if (c == 15) return K_CTRL_O;
    return c;
  }
  return -1;
}
#endif

// --- UI Theme & Structures ---
#define THEME_BG MZN_ATTR_BG_BLACK
#define THEME_FG MZN_ATTR_FG_WHITE
#define THEME_BORDER MZN_ATTR_FG_BLUE
#define THEME_BORDER_ACTIVE MZN_ATTR_FG_CYAN | MZN_ATTR_BOLD
#define THEME_TITLE MZN_ATTR_FG_YELLOW | MZN_ATTR_BOLD
#define THEME_ACCENT MZN_ATTR_FG_GREEN
#define THEME_WARN MZN_ATTR_FG_RED
#define THEME_MUTED MZN_ATTR_FG_MAGENTA

typedef struct {
  int x, y, w, h;
} Rect;

// --- Helper Functions ---

static void draw_panel(Rect r, const char *title, int active, int fill_char) {
  int attr_border = active ? THEME_BORDER_ACTIVE : THEME_BORDER;
  int attr_fill = THEME_BG | THEME_FG;

  if (r.x < 0 || r.y < 0 || r.x + r.w > term_w || r.y + r.h > term_h) return;

  for (int y = r.y; y < r.y + r.h; y++) {
    for (int x = r.x; x < r.x + r.w; x++) {
      mzn_term_set_char(x, y, fill_char, attr_fill);
    }
  }

  for (int i = 1; i < r.w - 1; i++) {
    mzn_term_set_char(r.x + i, r.y, '-', attr_border);
    mzn_term_set_char(r.x + i, r.y + r.h - 1, '-', attr_border);
  }
  for (int i = 1; i < r.h - 1; i++) {
    mzn_term_set_char(r.x, r.y + i, '|', attr_border);
    mzn_term_set_char(r.x + r.w - 1, r.y + i, '|', attr_border);
  }
  mzn_term_set_char(r.x, r.y, '+', attr_border);
  mzn_term_set_char(r.x + r.w - 1, r.y, '+', attr_border);
  mzn_term_set_char(r.x, r.y + r.h - 1, '+', attr_border);
  mzn_term_set_char(r.x + r.w - 1, r.y + r.h - 1, '+', attr_border);

  if (title && strlen(title) > 0) {
    int tx = r.x + 2;
    if (tx + (int)strlen(title) < r.x + r.w) {
      mzn_term_draw_string(tx, r.y, " ", attr_border);
      mzn_term_draw_string(tx + 1, r.y, title, THEME_TITLE);
      mzn_term_draw_string(tx + 1 + strlen(title), r.y, " ", attr_border);
    }
  }
}

static void draw_progress_v(int x, int y, int h, double val, int color_low, int color_high) {
  if (x < 0 || x >= term_w || y < 0 || y + h > term_h) return;
  int fill_h = (int)(val * h);
  if (fill_h > h) fill_h = h;
  for (int i = 0; i < h; i++) {
    int draw_y = y + (h - 1) - i;
    if (i < fill_h) {
      int col = (i > h * 0.8) ? color_high : color_low;
      mzn_term_set_char(x, draw_y, '#', col);
    } else {
      mzn_term_set_char(x, draw_y, '|', MZN_ATTR_FG_BLACK | MZN_ATTR_BG_BLACK | MZN_ATTR_BOLD);
    }
  }
}

// --- Application State ---
#define MAX_HISTORY 32
#define MAX_LINE_LEN 1024
#define MAX_LOG_LINES 512
#define WAVEFORM_BUFFER_SIZE 4800
#define EDITOR_MAX_LINES 2048
#define EDITOR_MAX_COL 256

static char cmd_history[MAX_HISTORY][MAX_LINE_LEN];
static int cmd_history_count = 0;
static int cmd_history_idx = 0;
static char log_history[MAX_LOG_LINES][MAX_LINE_LEN];
static int log_history_count = 0;
static char loaded_filename[MAX_LINE_LEN] = {0};
static time_t last_mod_time = 0;
static cr_engine *engine = NULL;

static char editor_lines[EDITOR_MAX_LINES][EDITOR_MAX_COL];
static int editor_line_count = 0;
static int ed_cx = 0, ed_cy = 0, ed_scroll = 0;
static char ed_msg[64] = {0};

static int popup_mode = 0;
static char popup_input_buf[64] = {0};
static int popup_input_pos = 0;

static FILE *rec_file = NULL;
static int is_recording = 0;
static long rec_bytes = 0;

static volatile double waveform_l[WAVEFORM_BUFFER_SIZE];
static volatile double waveform_r[WAVEFORM_BUFFER_SIZE];
static volatile int waveform_head = 0;
static volatile double cpu_load = 0.0;
static volatile double peak_l = 0.0, peak_r = 0.0;

// UI Modes: 0=Console, 1=Params, 2=Help, 3=Editor, 4=Tracker
static int ui_mode = 0;
static int vis_mode = 0;
static int selected_param_idx = 0;
static int log_scroll_offset = 0;
static int triggers[8] = {0};

// Tracker State
static int trk_cursor_x = 0; 
static int trk_cursor_y = 0; 
static int trk_cursor_ch = 0; 
static int trk_edit_mode = 0; 
static int trk_octave = 4;
static int trk_current_pat = 0;

// FFT State
static float waterfall_buf[WATERFALL_HISTORY][FFT_SIZE / 2];
static int waterfall_head = 0;
static float fft_in[FFT_SIZE];
static cpx fft_out[FFT_SIZE];
static int fft_idx = 0;

// --- FFT Implementation ---
static cpx cpx_add(cpx a, cpx b) { cpx r = {a.r + b.r, a.i + b.i}; return r; }
static cpx cpx_sub(cpx a, cpx b) { cpx r = {a.r - b.r, a.i - b.i}; return r; }
static cpx cpx_mul(cpx a, cpx b) { cpx r = {a.r * b.r - a.i * b.i, a.r * b.i + a.i * b.r}; return r; }
static float cpx_mag(cpx a) { return sqrtf(a.r * a.r + a.i * a.i); }

static void simple_fft(cpx *x, int n) {
  if (n <= 1) return;
  cpx even[n / 2];
  cpx odd[n / 2];
  for (int i = 0; i < n / 2; i++) {
    even[i] = x[2 * i];
    odd[i] = x[2 * i + 1];
  }
  simple_fft(even, n / 2);
  simple_fft(odd, n / 2);
  for (int k = 0; k < n / 2; k++) {
    float theta = -2.0f * M_PI * k / n;
    cpx t = {cosf(theta), sinf(theta)};
    cpx odd_term = cpx_mul(t, odd[k]);
    x[k] = cpx_add(even[k], odd_term);
    x[k + n / 2] = cpx_sub(even[k], odd_term);
  }
}

// --- Logging & IO ---

static void tui_log(const char *fmt, ...) {
  va_list args;
  char temp_buffer[MAX_LINE_LEN];
  va_start(args, fmt);
  vsnprintf(temp_buffer, MAX_LINE_LEN, fmt, args);
  va_end(args);
  if (log_history_count == MAX_LOG_LINES) {
    memmove(log_history[0], log_history[1], (MAX_LOG_LINES - 1) * MAX_LINE_LEN);
    log_history_count--;
  }
  strncpy(log_history[log_history_count], temp_buffer, MAX_LINE_LEN - 1);
  log_history[log_history_count++][MAX_LINE_LEN - 1] = '\0';
  log_scroll_offset = 0;
}

static void engine_log_cb(int level, const char *msg) {
  char prefix[16] = "";
  switch (level) {
  case CR_LOG_ERROR: strcpy(prefix, "ERR"); break;
  case CR_LOG_WARN: strcpy(prefix, "WRN"); break;
  case CR_LOG_INFO: strcpy(prefix, "INF"); break;
  case CR_LOG_DEBUG: strcpy(prefix, "DBG"); break;
  }
  tui_log("[%s] %s", prefix, msg);
}

static void editor_save_to_disk(const char *filename) {
  FILE *f = fopen(filename, "w");
  if (f) {
    for (int i = 0; i < editor_line_count; i++)
      fprintf(f, "%s\n", editor_lines[i]);
    fclose(f);

    // Auto-reload after save
    long sz;
    char *content;
    f = fopen(filename, "rb");
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    content = (char *)malloc(sz + 1);
    if (content) {
      fread(content, 1, sz, f);
      content[sz] = 0;
      fclose(f);
      cr_eval(engine, content, 1);
      free(content);
    }

    struct stat s;
    if (stat(filename, &s) == 0)
      last_mod_time = s.st_mtime;
    strcpy(ed_msg, "Saved & Reloaded!");
  } else {
    strcpy(ed_msg, "Write Error!");
  }
}

static int load_script(const char *filename) {
  FILE *f = fopen(filename, "rb");
  if (f) {
    long sz;
    char *content;
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    content = (char *)malloc(sz + 1);
    if (content) {
      struct stat file_stat;
      fread(content, 1, sz, f);
      content[sz] = 0;
      if (cr_eval(engine, content, 1)) {
        if (stat(filename, &file_stat) == 0)
          last_mod_time = file_stat.st_mtime;
        strncpy(loaded_filename, filename, MAX_LINE_LEN - 1);
        tui_log("Loaded '%s'", filename);
      } else
        tui_log("Load Failed");
      free(content);
    }
    fclose(f);
    return 1;
  }
  tui_log("File not found: '%s'", filename);
  return 0;
}

static void editor_load_file(const char *filename) {
  FILE *f = fopen(filename, "r");
  editor_line_count = 0;
  ed_cx = 0; ed_cy = 0; ed_scroll = 0;
  if (f) {
    char buf[EDITOR_MAX_COL];
    while (fgets(buf, EDITOR_MAX_COL, f) && editor_line_count < EDITOR_MAX_LINES) {
      int len = strlen(buf);
      while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
        buf[len - 1] = 0; len--;
      }
      strcpy(editor_lines[editor_line_count++], buf);
    }
    fclose(f);
  } else {
    editor_lines[0][0] = 0; editor_line_count = 1;
  }
}

// --- Popup Logic ---
static void open_popup_save_as() {
  popup_mode = 2;
  popup_input_buf[0] = 0;
  popup_input_pos = 0;
}

static void handle_popup_input(int key) {
  if (popup_mode == 1) { // New File?
    if (key == 'y' || key == 'Y' || key == K_ENTER) {
      ui_mode = 3;
      editor_lines[0][0] = 0; editor_line_count = 1;
      loaded_filename[0] = 0;
      popup_mode = 0;
    } else if (key == 'n' || key == 'N' || key == K_ESC) {
      exit(0);
    }
  } else if (popup_mode == 3) { // Overwrite?
    if (key == 'y' || key == 'Y' || key == K_ENTER) {
      strcpy(loaded_filename, popup_input_buf);
      editor_save_to_disk(loaded_filename);
      popup_mode = 0;
    } else if (key == 'n' || key == 'N' || key == K_ESC) {
      open_popup_save_as();
    }
  } else if (popup_mode == 2) { // Save As input
    if (key == K_ENTER) {
      if (popup_input_pos > 0) {
        FILE *f = fopen(popup_input_buf, "r");
        if (f) { fclose(f); popup_mode = 3; } 
        else {
          strcpy(loaded_filename, popup_input_buf);
          editor_save_to_disk(loaded_filename);
          popup_mode = 0;
        }
      } else popup_mode = 0;
    } else if (key == K_ESC) {
      popup_mode = 0;
    } else if (key == K_BACKSPACE) {
      if (popup_input_pos > 0) popup_input_buf[--popup_input_pos] = 0;
    } else if (key >= 32 && key <= 126 && popup_input_pos < 63) {
      popup_input_buf[popup_input_pos++] = (char)key;
      popup_input_buf[popup_input_pos] = 0;
    }
  }
}

static void draw_popup_window(int mode) {
  int w = 42, h = 9;
  int x = (term_w - w) / 2;
  int y = (term_h - h) / 2;
  if (x < 0) x = 0; if (y < 0) y = 0;

  Rect shadow = {x + 1, y + 1, w, h};
  draw_panel(shadow, "", 0, MZN_ATTR_BG_BLACK);
  int fill = MZN_ATTR_BG_BLUE | MZN_ATTR_FG_WHITE;
  
  // Fill background
  for(int j=0; j<h; j++) for(int i=0; i<w; i++) mzn_term_set_char(x+i, y+j, ' ', fill);
  
  // Draw Border
  int border = MZN_ATTR_FG_WHITE | MZN_ATTR_BOLD;
  for(int i=1; i<w-1; i++) { mzn_term_set_char(x+i, y, '-', border); mzn_term_set_char(x+i, y+h-1, '-', border); }
  for(int i=1; i<h-1; i++) { mzn_term_set_char(x, y+i, '|', border); mzn_term_set_char(x+w-1, y+i, '|', border); }
  
  int tx = x + 3, ty = y + 2;
  if (mode == 1) {
    mzn_term_draw_string(tx, ty, "NO FILE LOADED", fill | MZN_ATTR_BOLD);
    mzn_term_draw_string(tx, ty + 2, "Create New File? [Y/N]", fill | MZN_ATTR_FG_YELLOW);
  } else if (mode == 2) {
    mzn_term_draw_string(tx, ty, "SAVE AS:", fill | MZN_ATTR_BOLD);
    mzn_term_draw_string(tx, ty + 2, ">", fill | MZN_ATTR_FG_YELLOW);
    mzn_term_draw_string(tx + 2, ty + 2, popup_input_buf, fill | MZN_ATTR_BOLD);
    mzn_term_set_char(tx + 2 + popup_input_pos, ty + 2, '_', MZN_ATTR_BG_WHITE | MZN_ATTR_FG_BLACK);
  } else if (mode == 3) {
    mzn_term_draw_string(tx, ty, "FILE EXISTS!", fill | MZN_ATTR_BOLD);
    mzn_term_draw_string(tx, ty + 2, "Overwrite? [Y/N]", fill | MZN_ATTR_FG_YELLOW);
  }
}

// --- Audio Recording & Callback ---
static void write_wav_header(FILE *f, int sample_rate, int channels) {
  int data_sz = 0x7fffffff;
  int byte_rate = sample_rate * channels * 2;
  short block_align = channels * 2;
  fwrite("RIFF", 1, 4, f); fwrite(&data_sz, 4, 1, f);
  fwrite("WAVE", 1, 4, f); fwrite("fmt ", 1, 4, f);
  int subchunk1_size = 16; short audio_fmt = 1;
  fwrite(&subchunk1_size, 4, 1, f); fwrite(&audio_fmt, 2, 1, f);
  short chans = (short)channels; fwrite(&chans, 2, 1, f);
  fwrite(&sample_rate, 4, 1, f); fwrite(&byte_rate, 4, 1, f);
  fwrite(&block_align, 2, 1, f);
  short bits_per_sample = 16; fwrite(&bits_per_sample, 2, 1, f);
  fwrite("data", 1, 4, f); fwrite(&data_sz, 4, 1, f);
}

static void update_wav_header_size(FILE *f, long bytes) {
  if (!f) return;
  long total = bytes + 36;
  fseek(f, 4, SEEK_SET); fwrite(&total, 4, 1, f);
  fseek(f, 40, SEEK_SET); fwrite(&bytes, 4, 1, f);
}

static void toggle_recording() {
  if (is_recording) {
    if (rec_file) {
      update_wav_header_size(rec_file, rec_bytes);
      fclose(rec_file); rec_file = NULL;
    }
    is_recording = 0;
  } else {
    rec_file = fopen("output.wav", "wb");
    if (rec_file) {
      write_wav_header(rec_file, 48000, 2);
      rec_bytes = 0; is_recording = 1;
    }
  }
}

void audio_cb(short *buf, int frames, int sr, int ch, void *data) {
  int i;
  double pk_l = 0.0, pk_r = 0.0;
  short *rec_buf = NULL;
  if (is_recording && rec_file) rec_buf = (short *)malloc(frames * 2 * sizeof(short));

#ifdef _WIN32
  LARGE_INTEGER start, end, freq;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&start);
#else
  struct timeval start, end;
  gettimeofday(&start, NULL);
#endif

  for (i = 0; i < frames; i++) {
    // Critical: Update Tracker/Sequencer
    cr_tick(engine);
    
    double l = cr_process(engine, 0);
    double r = cr_process(engine, 1);
    
    // Clipping
    if (l > 1.0) l = 1.0; else if (l < -1.0) l = -1.0;
    if (r > 1.0) r = 1.0; else if (r < -1.0) r = -1.0;
    
    if (fabs(l) > pk_l) pk_l = fabs(l);
    if (fabs(r) > pk_r) pk_r = fabs(r);
    
    waveform_l[waveform_head] = l;
    waveform_r[waveform_head] = r;

    // FFT Processing
    double mono = (l + r) * 0.5;
    if (fft_idx < FFT_SIZE) {
      fft_in[fft_idx++] = (float)mono;
    } else {
      for (int k = 0; k < FFT_SIZE; k++) {
        fft_out[k].r = fft_in[k]; fft_out[k].i = 0.0f;
      }
      simple_fft(fft_out, FFT_SIZE);
      waterfall_head = (waterfall_head - 1 + WATERFALL_HISTORY) % WATERFALL_HISTORY;
      for (int k = 0; k < FFT_SIZE / 2; k++) {
        float mag = cpx_mag(fft_out[k]);
        float log_mag = log10f(mag + 1.0f) * 2.0f;
        waterfall_buf[waterfall_head][k] = (waterfall_buf[waterfall_head][k] * WATERFALL_SMOOTHING) + (log_mag * (1.0f - WATERFALL_SMOOTHING));
      }
      fft_idx = 0;
    }

    waveform_head = (waveform_head + 1) % WAVEFORM_BUFFER_SIZE;
    short sl = (short)(l * 32000.0);
    short sr_val = (short)(r * 32000.0);
    buf[i * 2] = sl; buf[i * 2 + 1] = sr_val;
    if (rec_buf) { rec_buf[i * 2] = sl; rec_buf[i * 2 + 1] = sr_val; }
  }

  if (rec_buf) {
    fwrite(rec_buf, 2, frames * 2, rec_file);
    rec_bytes += frames * 4;
    free(rec_buf);
  }
  peak_l = (pk_l > peak_l) ? pk_l : pk_l * 0.95;
  peak_r = (pk_r > peak_r) ? pk_r : peak_r * 0.95;

#ifdef _WIN32
  QueryPerformanceCounter(&end);
  double time_sec = (double)(end.QuadPart - start.QuadPart) / freq.QuadPart;
#else
  gettimeofday(&end, NULL);
  double time_sec = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
#endif
  cpu_load = (time_sec / ((double)frames / sr)) * 100.0;
}

// --- Parameter Management ---
typedef struct {
  char name[64];
  double value;
  int index;
  int input_const_node_idx;
} ui_param_t;

static int get_ui_params(cr_context *ctx, ui_param_t *out_params, int max_p) {
  int count = 0;
  for (int i = 0; i < ctx->var_count && count < max_p; i++) {
    // Skip internal tracker vars
    if (ctx->variables[i].name[0] == 't' && isdigit(ctx->variables[i].name[1]) && ctx->variables[i].name[2] == 0) continue;
    
    cr_node *n = &ctx->node_pool[ctx->variables[i].node_index];
    if (n->op_desc && strcmp(n->op_desc->name, "param") == 0) {
      strcpy(out_params[count].name, ctx->variables[i].name);
      out_params[count].index = ctx->variables[i].node_index;
      out_params[count].input_const_node_idx = n->inputs[0];
      out_params[count].value = ctx->node_pool[n->inputs[0]].value.as.f;
      count++;
    }
  }
  return count;
}

// --- Tracker View Implementation ---

static void note_to_str(unsigned char n, char *buf) {
  const char *notes[] = {"C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"};
  if (n == 0) strcpy(buf, "...");
  else if (n == 254) strcpy(buf, "==="); // Key Off
  else sprintf(buf, "%s%d", notes[(n % 12)], (n / 12) - 1);
}

// Dump pattern to log for Copy/Paste into script
static void dump_pattern_to_log(int pat_idx) {
    tui_log("Dumping Pat %d Ch %d to Log:", pat_idx, trk_cursor_ch);
    cr_pattern *p = &engine->active->tracker.patterns[pat_idx];
    char buf[64];
    tui_log("pattern %d, %d, \"", pat_idx, trk_cursor_ch);
    for(int r=0; r<CR_TRACK_ROWS; r+=4) { // Sample 4 rows just to show it works
         cr_track_cell *c = &p->rows[r][trk_cursor_ch];
         note_to_str(c->note, buf);
         tui_log("  %s %02X %02X %X %02X", buf, c->inst, c->vol, c->cmd, c->val);
    }
    tui_log("\"");
}

static void draw_tracker_view(Rect r) {
  draw_panel(r, "TRACKER", 1, ' ');
  
  // Header
  int ty = r.y + 1;
  char head[256];
  sprintf(head, "PAT:%02X  LEN:64  OCT:%d  EDIT:%s [P]rint", 
          trk_current_pat, trk_octave, trk_edit_mode ? "REC" : "OFF");
  mzn_term_draw_string(r.x + 2, ty++, head, THEME_TITLE);
  
  int view_rows = r.h - 3;
  int start_row = trk_cursor_y - (view_rows / 2);
  
  // Retrieve current pattern from engine
  cr_pattern *pat = &engine->active->tracker.patterns[trk_current_pat];

  for (int i = 0; i < view_rows; i++) {
    int row_idx = start_row + i;
    int draw_y = ty + i;
    
    if (row_idx < 0 || row_idx >= CR_TRACK_ROWS) continue;

    int is_curr = (row_idx == trk_cursor_y);
    int bg = is_curr ? (trk_edit_mode ? MZN_ATTR_BG_RED : MZN_ATTR_BG_BLUE) : MZN_ATTR_BG_BLACK;
    int fg = is_curr ? MZN_ATTR_FG_WHITE : MZN_ATTR_FG_CYAN;
    
    // Draw Row Number
    char num[8];
    sprintf(num, "%02X|", row_idx);
    mzn_term_draw_string(r.x + 1, draw_y, num, MZN_ATTR_FG_YELLOW | bg);
    
    // Draw Channels
    int x_off = 5;
    for (int ch = 0; ch < CR_TRACK_CHANNELS; ch++) {
       if (x_off + 12 > r.w) break;
       
       cr_track_cell *c = &pat->rows[row_idx][ch];
       char n_str[4], val_str[16];
       note_to_str(c->note, n_str);
       
       // Format: NOTE INS VOL CMD VAL
       sprintf(val_str, "%s %02X %02X %X %02X", n_str, c->inst, c->vol, c->cmd, c->val);
       
       if (is_curr && ch == trk_cursor_ch) {
          mzn_term_draw_string(r.x + x_off, draw_y, val_str, bg | fg);
       } else {
          mzn_term_draw_string(r.x + x_off, draw_y, val_str, bg | (ch % 2 ? MZN_ATTR_FG_WHITE : MZN_ATTR_FG_MAGENTA));
       }
       
       mzn_term_draw_string(r.x + x_off + 13, draw_y, "|", MZN_ATTR_FG_BLUE | bg);
       x_off += 15;
    }
  }
}

static void handle_tracker_input(int key) {
  cr_pattern *pat = &engine->active->tracker.patterns[trk_current_pat];
  cr_track_cell *cell = &pat->rows[trk_cursor_y][trk_cursor_ch];

  // Navigation
  if (key == K_UP) { if (trk_cursor_y > 0) trk_cursor_y--; }
  else if (key == K_DOWN) { if (trk_cursor_y < CR_TRACK_ROWS-1) trk_cursor_y++; }
  else if (key == K_LEFT) { 
      if (trk_cursor_ch > 0) trk_cursor_ch--; 
      else if (trk_cursor_ch == 0 && trk_current_pat > 0) {
          trk_current_pat--; trk_cursor_ch = CR_TRACK_CHANNELS-1;
      }
  }
  else if (key == K_RIGHT) { 
      if (trk_cursor_ch < CR_TRACK_CHANNELS-1) trk_cursor_ch++;
      else if (trk_cursor_ch == CR_TRACK_CHANNELS-1 && trk_current_pat < CR_MAX_PATTERNS-1) {
          trk_current_pat++; trk_cursor_ch = 0;
      }
  }
  else if (key == ' ') { trk_edit_mode = !trk_edit_mode; } // Toggle Rec
  else if (key == 'p' || key == 'P') { dump_pattern_to_log(trk_current_pat); }
  
  // Data Entry (Only in Rec Mode)
  if (trk_edit_mode) {
     int note = -1;
     // QWERTY Piano Mapping (Z=C4)
     char *keys = "zsxdcvgbhnjm,q2w3er5t6y7u";
     char *ptr = strchr(keys, tolower(key));
     if (ptr) {
        int semitone = ptr - keys;
        note = (trk_octave * 12) + semitone;
        cell->note = note;
        cell->inst = 1;
        cell->vol = 64;
        if (trk_cursor_y < CR_TRACK_ROWS-1) trk_cursor_y++;
     }
     else if (key == '.') { cell->note = 0; } // Clear
     else if (key == K_DEL) { cell->note = 254; } // Key Off
     
     // Sync to back buffer too to prevent flickering if context swap happens
     engine->back->tracker.patterns[trk_current_pat] = *pat;
  }
}

// --- Editor Input ---
static void editor_handle_input(int key) {
  ed_msg[0] = 0;
  if (key == K_ESC) { ui_mode = 0; return; }
  if (key == K_CTRL_O) { open_popup_save_as(); return; }
  if (key == K_CTRL_S) {
    if (loaded_filename[0]) editor_save_to_disk(loaded_filename);
    else open_popup_save_as();
    return;
  }
  if (key == K_UP) { if (ed_cy > 0) ed_cy--; } 
  else if (key == K_DOWN) { if (ed_cy < editor_line_count - 1) ed_cy++; } 
  else if (key == K_LEFT) { if (ed_cx > 0) ed_cx--; } 
  else if (key == K_RIGHT) { if (ed_cx < (int)strlen(editor_lines[ed_cy])) ed_cx++; }

  int len = strlen(editor_lines[ed_cy]);
  if (ed_cx > len) ed_cx = len;

  if (key == K_ENTER) {
    if (editor_line_count < EDITOR_MAX_LINES - 1) {
      for (int i = editor_line_count; i > ed_cy + 1; i--) strcpy(editor_lines[i], editor_lines[i - 1]);
      strcpy(editor_lines[ed_cy + 1], &editor_lines[ed_cy][ed_cx]);
      editor_lines[ed_cy][ed_cx] = 0;
      editor_line_count++; ed_cy++; ed_cx = 0;
    }
  } else if (key == K_BACKSPACE) {
    if (ed_cx > 0) {
      memmove(&editor_lines[ed_cy][ed_cx - 1], &editor_lines[ed_cy][ed_cx], len - ed_cx + 1);
      ed_cx--;
    } else if (ed_cy > 0) {
      int prev_len = strlen(editor_lines[ed_cy - 1]);
      if (prev_len + len < EDITOR_MAX_COL) {
        strcat(editor_lines[ed_cy - 1], editor_lines[ed_cy]);
        for (int i = ed_cy; i < editor_line_count - 1; i++) strcpy(editor_lines[i], editor_lines[i + 1]);
        editor_line_count--; ed_cy--; ed_cx = prev_len;
      }
    }
  } else if (key >= 32 && key <= 126) {
    if (len < EDITOR_MAX_COL - 1) {
      memmove(&editor_lines[ed_cy][ed_cx + 1], &editor_lines[ed_cy][ed_cx], len - ed_cx + 1);
      editor_lines[ed_cy][ed_cx] = (char)key;
      ed_cx++;
    }
  }
}

// --- Console Input ---
static void add_history(const char *line) {
  if (strlen(line) == 0) return;
  if (cmd_history_count > 0 && !strcmp(cmd_history[cmd_history_count - 1], line)) return;
  if (cmd_history_count < MAX_HISTORY) {
    strcpy(cmd_history[cmd_history_count++], line);
  } else {
    for (int i = 0; i < MAX_HISTORY - 1; i++) strcpy(cmd_history[i], cmd_history[i + 1]);
    strcpy(cmd_history[MAX_HISTORY - 1], line);
  }
  cmd_history_idx = cmd_history_count;
}

// --- Visualizers ---
static void draw_waterfall_3d(Rect r) {
  if (r.w <= 0 || r.h <= 0) return;
  int *horizon = (int *)malloc(term_w * sizeof(int));
  if (!horizon) return;
  for (int i = 0; i < term_w; i++) horizon[i] = r.y + r.h;

  for (int i = 0; i < WATERFALL_HISTORY; i++) {
    int buf_idx = (waterfall_head + i) % WATERFALL_HISTORY;
    int layer_y_base = (r.y + r.h - 2) - i;
    int layer_x_offset = i;
    if (layer_y_base < r.y) break;

    for (int bin = 0; bin < FFT_SIZE / 2; bin += 2) {
      int screen_x = r.x + layer_x_offset + (bin / 2);
      if (screen_x >= r.x && screen_x < r.x + r.w) {
        float val = waterfall_buf[buf_idx][bin];
        int height = (int)(val * 8.0f);
        int peak_y = layer_y_base - height;
        if (peak_y < horizon[screen_x]) {
          char c = (i == 0) ? '#' : (i < 5 ? '=' : '-');
          int color = (i == 0) ? MZN_ATTR_FG_WHITE : (i < 8 ? MZN_ATTR_FG_CYAN : MZN_ATTR_FG_BLUE);
          if (peak_y >= r.y) mzn_term_set_char(screen_x, peak_y, c, color);
          for (int y = peak_y + 1; y < horizon[screen_x]; y++) {
            if (y >= r.y && y < r.y + r.h) mzn_term_set_char(screen_x, y, ' ', MZN_ATTR_BG_BLACK);
          }
          horizon[screen_x] = peak_y;
        }
      }
    }
  }
  free(horizon);
}

// --- Main Drawing Function ---
static void draw_modern_ui(const char *buf, int pos) {
  if (needs_resize) { mzn_term_resize(term_w, term_h); needs_resize = 0; }
  mzn_term_clear_buffer(THEME_BG | THEME_FG);
  int w = term_w, h = term_h;

  // Top Bar
  mzn_term_draw_string(2, 0, " CHRONOS v1.6 ", MZN_ATTR_BG_BLUE | MZN_ATTR_FG_WHITE | MZN_ATTR_BOLD);
  char status[128];
  sprintf(status, " BPM: %.0f | CPU: %.1f%% ", engine->active->transport.bpm, cpu_load);
  mzn_term_draw_string(18, 0, status, MZN_ATTR_FG_CYAN);
  if (is_recording) mzn_term_draw_string(40, 0, " REC ", MZN_ATTR_BG_RED | MZN_ATTR_FG_WHITE | MZN_ATTR_BOLD);
  if (loaded_filename[0]) {
    char fn[64]; sprintf(fn, " FILE: %s ", loaded_filename);
    if ((int)strlen(fn) < w - 45) mzn_term_draw_string(w - strlen(fn) - 2, 0, fn, MZN_ATTR_FG_YELLOW);
  }

  // Layout Calculations
  int sidebar_w = (w < 80) ? (w < 50 ? 0 : 20) : 34;
  int main_w = w - sidebar_w;
  int footer_h = 3;
  int workspace_h = h - 2 - footer_h;
  int scope_h = (h / 3 < 10) ? 10 : h / 3;
  if (scope_h > workspace_h - 6) scope_h = workspace_h - 6;

  Rect r_main = {0, 1, main_w, workspace_h};
  Rect r_scope = {1, 2, main_w - 2, scope_h};
  Rect r_log = {1, 2 + scope_h, main_w - 2, workspace_h - (scope_h + 1)};
  Rect r_side = {main_w, 1, sidebar_w, workspace_h};
  Rect r_foot = {0, h - footer_h, w, footer_h};

  // --- View Dispatch ---
  if (ui_mode == 3) { // EDITOR
    draw_panel(r_main, "EDITOR", 1, ' ');
    int vh = r_main.h - 2;
    if (ed_cy < ed_scroll) ed_scroll = ed_cy;
    if (ed_cy >= ed_scroll + vh) ed_scroll = ed_cy - vh + 1;

    for (int i = 0; i < vh; i++) {
      int idx = ed_scroll + i;
      if (idx < editor_line_count) {
        char lnum[8]; sprintf(lnum, "%4d ", idx + 1);
        mzn_term_draw_string(r_main.x + 1, r_main.y + 1 + i, lnum, MZN_ATTR_BG_BLACK | MZN_ATTR_FG_MAGENTA);
        mzn_term_draw_string(r_main.x + 6, r_main.y + 1 + i, editor_lines[idx], THEME_FG);
      }
    }
    // Cursor
    int sx = r_main.x + 6 + ed_cx;
    int sy = r_main.y + 1 + (ed_cy - ed_scroll);
    char c = (ed_cx < (int)strlen(editor_lines[ed_cy])) ? editor_lines[ed_cy][ed_cx] : ' ';
    if (sx >= r_main.x && sx < r_main.x + r_main.w - 1 && sy >= r_main.y && sy < r_main.y + r_main.h - 1)
      mzn_term_set_char(sx, sy, c, MZN_ATTR_BG_WHITE | MZN_ATTR_FG_BLACK);
      
    if (ed_msg[0]) mzn_term_draw_string(r_main.x + r_main.w - strlen(ed_msg) - 2, r_main.y, ed_msg, THEME_ACCENT);

  } else if (ui_mode == 4) { // TRACKER
    draw_tracker_view(r_main);
  } else { // CONSOLE / DEFAULT
    draw_panel(r_main, "CONSOLE", ui_mode == 0, ' ');
    mzn_term_draw_string(r_scope.x + 1, r_scope.y - 1, "VISUALIZER", THEME_MUTED);

    if (vis_mode == 0) { // Scope
      int cy = r_scope.y + (r_scope.h / 2);
      for (int i = 0; i < r_scope.w; i++) {
        int idx = (waveform_head + i * (WAVEFORM_BUFFER_SIZE / r_scope.w)) % WAVEFORM_BUFFER_SIZE;
        int y_l = cy - (int)(waveform_l[idx] * 4);
        int y_r = cy - (int)(waveform_r[idx] * 4);
        if (y_l >= r_scope.y && y_l < r_scope.y + r_scope.h) mzn_term_set_char(r_scope.x + i, y_l, '*', MZN_ATTR_FG_GREEN);
        if (y_r != y_l && y_r >= r_scope.y && y_r < r_scope.y + r_scope.h) mzn_term_set_char(r_scope.x + i, y_r, '*', MZN_ATTR_FG_RED);
      }
    } else if (vis_mode == 1) { // XY
      int cx = r_scope.x + (r_scope.w / 2), cy = r_scope.y + (r_scope.h / 2);
      for (int i = 0; i < WAVEFORM_BUFFER_SIZE; i += 30) {
        int px = cx + (int)(waveform_l[i] * (int)(r_scope.w / 2 - 2));
        int py = cy - (int)(waveform_r[i] * (int)(r_scope.h / 2 - 1));
        if (px >= r_scope.x && px < r_scope.x + r_scope.w && py >= r_scope.y && py < r_scope.y + r_scope.h)
          mzn_term_set_char(px, py, '.', MZN_ATTR_FG_CYAN);
      }
    } else if (vis_mode == 2) { // 3D
      mzn_term_draw_string(r_scope.x + 1, r_scope.y, "SPECTRAL TERRAIN", THEME_MUTED);
      draw_waterfall_3d(r_scope);
    }

    // Logs
    for (int i = 1; i < r_main.w - 1; i++) mzn_term_set_char(r_main.x + i, r_log.y - 1, '-', THEME_BORDER);
    mzn_term_draw_string(r_log.x + 1, r_log.y - 1, "LOG", THEME_MUTED);

    int max_vis = r_log.h;
    int max_scroll = (log_history_count > max_vis) ? log_history_count - max_vis : 0;
    if (log_scroll_offset > max_scroll) log_scroll_offset = max_scroll;
    if (log_scroll_offset < 0) log_scroll_offset = 0;
    int start = max_scroll - log_scroll_offset;

    for (int i = 0; i < max_vis && start + i < log_history_count; i++) {
      char lbuf[1024]; strncpy(lbuf, log_history[start + i], r_log.w - 1); lbuf[r_log.w - 1] = 0;
      int attr = THEME_FG;
      if (strstr(lbuf, "[ERR]")) attr = MZN_ATTR_FG_RED;
      else if (strstr(lbuf, "[WRN]")) attr = MZN_ATTR_FG_YELLOW;
      else if (strstr(lbuf, "[INF]")) attr = MZN_ATTR_FG_CYAN;
      mzn_term_draw_string(r_log.x, r_log.y + i, lbuf, attr);
    }
  }

  // Sidebar
  if (sidebar_w > 0) {
    draw_panel(r_side, "CONTROLS", ui_mode == 1, ' ');
    int ty = r_side.y + 1;
    mzn_term_draw_string(r_side.x + 2, ty++, "TRIGGERS (1-8)", THEME_TITLE);
    for (int i = 0; i < 2; i++) {
      char tbuf[64];
      sprintf(tbuf, "%d:%s %d:%s %d:%s %d:%s", 
              i*4+1, triggers[i*4]? "ON " : " . ", i*4+2, triggers[i*4+1]? "ON " : " . ",
              i*4+3, triggers[i*4+2]? "ON " : " . ", i*4+4, triggers[i*4+3]? "ON " : " . ");
      mzn_term_draw_string(r_side.x + 2, ty++, tbuf, THEME_FG);
    }
    ty++;
    mzn_term_draw_string(r_side.x + 2, ty++, "OUTPUT", THEME_TITLE);
    draw_progress_v(r_side.x + 2, ty, 6, peak_l, MZN_ATTR_FG_GREEN, MZN_ATTR_FG_RED);
    draw_progress_v(r_side.x + 6, ty, 6, peak_r, MZN_ATTR_FG_GREEN, MZN_ATTR_FG_RED);
    ty += 8;

    mzn_term_draw_string(r_side.x + 2, ty++, "PARAMS (TAB)", THEME_TITLE);
    ui_param_t params[32];
    int p_count = get_ui_params(engine->active, params, 32);
    if (selected_param_idx >= p_count) selected_param_idx = (p_count > 0 ? p_count - 1 : 0);
    int max_params_h = r_side.h - (ty - r_side.y) - 1;
    int p_start = (selected_param_idx >= max_params_h) ? selected_param_idx - max_params_h + 1 : 0;

    for (int i = 0; i < max_params_h && (p_start + i) < p_count; i++) {
      int idx = p_start + i;
      int is_sel = (ui_mode == 1 && idx == selected_param_idx);
      char pbuf[64]; sprintf(pbuf, "%-14s %6.3f", params[idx].name, params[idx].value);
      mzn_term_draw_string(r_side.x + 2, ty++, pbuf, is_sel ? (MZN_ATTR_BG_CYAN | MZN_ATTR_FG_BLACK) : THEME_FG);
    }
  }

  // Footer
  for (int i = 0; i < w; i++) mzn_term_set_char(i, r_foot.y, '-', THEME_BORDER);
  if (ui_mode == 2) {
    mzn_term_draw_string(2, r_foot.y + 1, " HELP: E=Edit, R=Rec, V=Vis, TAB=View, ESC=Panic, Ctrl+S=Save ", MZN_ATTR_FG_YELLOW);
  } else {
    mzn_term_draw_string(2, r_foot.y + 1, (ui_mode == 0) ? "CMD > " : "    > ", (ui_mode == 0) ? MZN_ATTR_FG_GREEN : THEME_MUTED);
    if (ui_mode == 0) {
      mzn_term_draw_string(8, r_foot.y + 1, buf, MZN_ATTR_FG_WHITE);
      mzn_term_set_char(8 + pos, r_foot.y + 1, buf[pos] ? buf[pos] : ' ', MZN_ATTR_BG_WHITE | MZN_ATTR_FG_BLACK);
    } else {
       char *mode_str = (ui_mode == 1) ? "[PARAMS]" : (ui_mode == 3 ? "[EDITOR]" : "[TRACKER]");
       mzn_term_draw_string(8, r_foot.y + 1, mode_str, THEME_MUTED);
    }
  }
  if (popup_mode > 0) draw_popup_window(popup_mode);
  mzn_term_draw(); mzn_term_flip_buffer();
}

// --- Main Loop ---
int main(int argc, char **argv) {
  ss_audio_t *audio;
  char line_buf[MAX_LINE_LEN] = {0};
  int line_pos = 0;
  time_t last_check_time = 0;

#ifdef _WIN32
  check_resize();
#else
  struct winsize w;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) {
    term_w = w.ws_col; term_h = w.ws_row;
  }
  signal(SIGWINCH, handle_winch);
#endif

  mzn_term_init(THEME_BG | THEME_FG);
  mzn_term_resize(term_w, term_h);

  engine = cr_create_engine(48000);
  cr_set_log_callback(engine, engine_log_cb);
  cr_set_log_level(engine, CR_LOG_INFO);

  if (argc > 1) {
    load_script(argv[1]);
    editor_load_file(argv[1]);
  } else {
    popup_mode = 1; // Prompt new file
  }

  audio = ss_open_audio(audio_cb, NULL, 48000, 2, 1024, NULL);
  if (!audio) return 1;

  set_non_blocking_input();

  while (1) {
    int key;
    time_t now = time(NULL);

    // Auto-reload check
    if (ui_mode != 3 && popup_mode == 0 && loaded_filename[0] && now > last_check_time) {
      last_check_time = now;
      struct stat s;
      if (stat(loaded_filename, &s) == 0) {
        if (s.st_mtime > last_mod_time && now - s.st_mtime >= 1) load_script(loaded_filename);
      }
    }

    draw_modern_ui(line_buf, line_pos);
    key = get_key_evt();

    if (key != -1) {
      if (popup_mode > 0) { handle_popup_input(key); continue; }
      
      // Mode-Specific Handling
      if (ui_mode == 3) { editor_handle_input(key); continue; }
      if (ui_mode == 4) { 
          handle_tracker_input(key); 
          // Allow TAB to exit tracker
          if (key == K_TAB) { ui_mode = 0; continue; }
          // Allow E to switch to editor
          if (key == 'e' || key == 'E') { ui_mode = 3; continue; }
          continue; 
      }

      // Mode Cycle: Console(0) -> Params(1) -> Help(2) -> Tracker(4) -> Console(0)
      if (key == K_TAB) {
        ui_mode++;
        if (ui_mode == 3) ui_mode++; // Skip Editor in TAB cycle
        if (ui_mode > 4) ui_mode = 0;
        continue;
      }
      
      if (key == K_ESC) { cr_eval(engine, "out = 0", 0); tui_log("PANIC!"); continue; }

      // Global Shortcuts
      if ((key == 'e' || key == 'E') && ui_mode != 0) {
        if (loaded_filename[0] || editor_line_count > 0) {
          if (loaded_filename[0]) editor_load_file(loaded_filename);
          ui_mode = 3; continue;
        }
      }
      
      // Page Scrolling for Log
      if (key == K_PGUP) log_scroll_offset = (log_scroll_offset + 5 > MAX_LOG_LINES) ? MAX_LOG_LINES : log_scroll_offset + 5;
      if (key == K_PGDN) log_scroll_offset = (log_scroll_offset - 5 < 0) ? 0 : log_scroll_offset - 5;

      int typing = (ui_mode == 0 && line_pos > 0);
      if (!typing) {
        if (key == 'r' || key == 'R') { toggle_recording(); continue; }
        if (key == 'v' || key == 'V') { vis_mode = (vis_mode + 1) % 3; continue; }
        if (key >= '1' && key <= '8') {
          int t = key - '1'; triggers[t] = !triggers[t];
          char buf[64]; sprintf(buf, "t%d = %d", t+1, triggers[t]);
          cr_eval(engine, buf, 0); continue;
        }
      }

      // Params Mode Input
      if (ui_mode == 1) {
        ui_param_t params[32];
        int count = get_ui_params(engine->active, params, 32);
        if (key == K_UP && selected_param_idx > 0) selected_param_idx--;
        else if (key == K_DOWN && selected_param_idx < count - 1) selected_param_idx++;
        else if (key == K_LEFT || key == K_RIGHT) {
          if (count > 0 && selected_param_idx < count) {
            int nid = params[selected_param_idx].input_const_node_idx;
            cr_val *v = &engine->active->node_pool[nid].value;
            double change = (key == K_RIGHT) ? 0.01 : -0.01;
            if (v->type == CR_FLOAT) v->as.f += change; else v->as.i += (int)(change * 100);
          }
        }
      } 
      // Console Input
      else if (ui_mode == 0) {
        int len = strlen(line_buf);
        if (key == K_ENTER) {
          if (!strcmp(line_buf, "quit")) break;
          else if (strncmp(line_buf, "load ", 5) == 0) { load_script(line_buf + 5); editor_load_file(line_buf + 5); }
          else if (!strcmp(line_buf, "edit")) {
            if (loaded_filename[0]) editor_load_file(loaded_filename);
            ui_mode = 3;
          } else if (len > 0) {
            add_history(line_buf);
            if (cr_eval(engine, line_buf, 0)) tui_log("Executed."); else tui_log("Eval Error.");
          }
          line_buf[0] = 0; line_pos = 0;
        } else if (key == K_BACKSPACE && line_pos > 0) {
          memmove(line_buf + line_pos - 1, line_buf + line_pos, len - line_pos + 1); line_pos--;
        } else if (key == K_DEL && line_pos < len) {
          memmove(line_buf + line_pos, line_buf + line_pos + 1, len - line_pos);
        } else if (key == K_LEFT && line_pos > 0) line_pos--;
        else if (key == K_RIGHT && line_pos < len) line_pos++;
        else if (key == K_UP && cmd_history_count > 0) {
           if (cmd_history_idx > 0) cmd_history_idx--;
           strcpy(line_buf, cmd_history[cmd_history_idx]); line_pos = strlen(line_buf);
        } else if (key == K_DOWN) {
           if (cmd_history_idx < cmd_history_count - 1) {
             cmd_history_idx++; strcpy(line_buf, cmd_history[cmd_history_idx]); line_pos = strlen(line_buf);
           } else { line_buf[0] = 0; line_pos = 0; cmd_history_idx = cmd_history_count; }
        } else if (key >= 32 && key <= 126 && len < MAX_LINE_LEN - 1) {
          memmove(line_buf + line_pos + 1, line_buf + line_pos, len - line_pos + 1);
          line_buf[line_pos++] = (char)key;
        }
      }
    }
  }

  restore_input();
  if (rec_file) { update_wav_header_size(rec_file, rec_bytes); fclose(rec_file); }
  ss_close_audio(audio);
  cr_destroy_engine(engine);
  return 0;
}