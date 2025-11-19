
#ifndef MIO_H
#define MIO_H

/* @PLATFORM *****************************************************************/

#include <float.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef MIO_NO_DEBUG
#define MIO_ASSERT(expr) ((void)0)
#else
#define MIO_ASSERT(expr)                                                      \
  ((expr) ? (void)0                                                           \
          : (fprintf(                                                         \
                 stderr, "Assertion failed: %s, file %s, line %d\n", #expr,   \
                 __FILE__, __LINE__),                                         \
             abort()))
#endif

typedef signed char int8;
typedef signed short int16;
typedef signed int int32;
typedef signed __int64 int64;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned __int64 uint64;
typedef float real32;
typedef double real64;

#define SYS_MAX_KEYS 256
#define SYS_MAX_MOUSE_BUTTONS 5
#define SYS_AUDIO_BUFFER_COUNT 2
#define SYS_AUDIO_DEFAULT_SAMPLE_RATE 44100
#define SYS_AUDIO_DEFAULT_CHANNELS 2
#define SYS_AUDIO_DEFAULT_BITS_PER_SAMPLE 16

typedef uint8 SYSRET;

typedef struct {
  int32 width, height;
} mioSystemSize;

typedef struct {
  int32 x, y;
} mioSystemPoint;

typedef struct {
  int32 sr;
  int32 ch;
  int32 bps;
  int32 bpf;
  int32 bufferFrames;
} mioSystemAudioFormat;

typedef void (*PFSYSTEMAUDIOCB)(void *ud, float *out, int32 frames);

SYSRET
sys_init(const char *title, int32 width, int32 height, SYSRET borderless);
SYSRET sys_process_messages(void);
void sys_present(void);
void sys_shutdown(void);
void sys_log_perf(void);
void sys_quit(void);

real64 sys_get_time(void);
real64 sys_get_delta_time(void);
mioSystemSize sys_get_window_size(void);
void sys_sleep(uint32 ms);
void sys_set_target_framerate(int32 fps);
void sys_minimize(void);
void sys_maximize(void);
void sys_restore(void);
void sys_toggle_fullscreen(void);

SYSRET sys_key_down(int32 key);
SYSRET sys_key_pressed(int32 key);
SYSRET sys_key_released(int32 key);

SYSRET sys_mouse_down(int32 btn);
SYSRET sys_mouse_pressed(int32 btn);
SYSRET sys_mouse_released(int32 btn);
mioSystemPoint sys_mouse_position(void);
int32 sys_mouse_wheel(void);

SYSRET sys_init_audio(int32 sr, int32 ch, PFSYSTEMAUDIOCB cb, void *ud);
void sys_shutdown_audio(void);
SYSRET sys_is_audio_playing(void);
mioSystemAudioFormat sys_get_audio_format(void);
void sys_beep(int32 freq, int32 dur);

SYSRET sys_file_exists(const char *fp);
uint8 *sys_load_file(const char *fp, int32 *sz);
SYSRET sys_save_file(const char *fp, uint8 *data, int32 sz);
void sys_free_file(uint8 *data);

void *sys_alloc(int32 sz);
void sys_free(void *ptr);
void *sys_realloc(void *ptr, int32 sz);

#if defined(_WIN32)

typedef enum mioVirtualKeys {
  KEY_LBUTTON = 0x01,
  KEY_RBUTTON = 0x02,
  KEY_CANCEL = 0x03,
  KEY_MBUTTON = 0x04,
  KEY_XBUTTON1 = 0x05,
  KEY_XBUTTON2 = 0x06,
  KEY_BACK = 0x08,
  KEY_TAB = 0x09,
  KEY_CLEAR = 0x0C,
  KEY_RETURN = 0x0D,
  KEY_SHIFT = 0x10,
  KEY_CONTROL = 0x11,
  KEY_MENU = 0x12,
  KEY_PAUSE = 0x13,
  KEY_CAPITAL = 0x14,
  KEY_KANA = 0x15,
  KEY_HANGUL = 0x15,
  KEY_JUNJA = 0x17,
  KEY_FINAL = 0x18,
  KEY_HANJA = 0x19,
  KEY_KANJI = 0x19,
  KEY_ESCAPE = 0x1B,
  KEY_CONVERT = 0x1C,
  KEY_NONCONVERT = 0x1D,
  KEY_ACCEPT = 0x1E,
  KEY_MODECHANGE = 0x1F,
  KEY_SPACE = 0x20,
  KEY_PRIOR = 0x21,
  KEY_NEXT = 0x22,
  KEY_END = 0x23,
  KEY_HOME = 0x24,
  KEY_LEFT = 0x25,
  KEY_UP = 0x26,
  KEY_RIGHT = 0x27,
  KEY_DOWN = 0x28,
  KEY_SELECT = 0x29,
  KEY_PRINT = 0x2A,
  KEY_RUN = 0x2B,
  KEY_SNAPSHOT = 0x2C,
  KEY_INSERT = 0x2D,
  KEY_DELETE = 0x2E,
  KEY_HELP = 0x2F,
  KEY_0 = 0x30,
  KEY_1 = 0x31,
  KEY_2 = 0x32,
  KEY_3 = 0x33,
  KEY_4 = 0x34,
  KEY_5 = 0x35,
  KEY_6 = 0x36,
  KEY_7 = 0x37,
  KEY_8 = 0x38,
  KEY_9 = 0x39,
  KEY_A = 0x41,
  KEY_B = 0x42,
  KEY_C = 0x43,
  KEY_D = 0x44,
  KEY_E = 0x45,
  KEY_F = 0x46,
  KEY_G = 0x47,
  KEY_H = 0x48,
  KEY_I = 0x49,
  KEY_J = 0x4A,
  KEY_K = 0x4B,
  KEY_L = 0x4C,
  KEY_M = 0x4D,
  KEY_N = 0x4E,
  KEY_O = 0x4F,
  KEY_P = 0x50,
  KEY_Q = 0x51,
  KEY_R = 0x52,
  KEY_S = 0x53,
  KEY_T = 0x54,
  KEY_U = 0x55,
  KEY_V = 0x56,
  KEY_W = 0x57,
  KEY_X = 0x58,
  KEY_Y = 0x59,
  KEY_Z = 0x5A,
  KEY_LWIN = 0x5B,
  KEY_RWIN = 0x5C,
  KEY_APPS = 0x5D,
  KEY_SLEEP = 0x5F,
  KEY_NUMPAD0 = 0x60,
  KEY_NUMPAD1 = 0x61,
  KEY_NUMPAD2 = 0x62,
  KEY_NUMPAD3 = 0x63,
  KEY_NUMPAD4 = 0x64,
  KEY_NUMPAD5 = 0x65,
  KEY_NUMPAD6 = 0x66,
  KEY_NUMPAD7 = 0x67,
  KEY_NUMPAD8 = 0x68,
  KEY_NUMPAD9 = 0x69,
  KEY_MULTIPLY = 0x6A,
  KEY_ADD = 0x6B,
  KEY_SEPARATOR = 0x6C,
  KEY_SUBTRACT = 0x6D,
  KEY_DECIMAL = 0x6E,
  KEY_DIVIDE = 0x6F,
  KEY_F1 = 0x70,
  KEY_F2 = 0x71,
  KEY_F3 = 0x72,
  KEY_F4 = 0x73,
  KEY_F5 = 0x74,
  KEY_F6 = 0x75,
  KEY_F7 = 0x76,
  KEY_F8 = 0x77,
  KEY_F9 = 0x78,
  KEY_F10 = 0x79,
  KEY_F11 = 0x7A,
  KEY_F12 = 0x7B,
  KEY_F13 = 0x7C,
  KEY_F14 = 0x7D,
  KEY_F15 = 0x7E,
  KEY_F16 = 0x7F,
  KEY_F17 = 0x80,
  KEY_F18 = 0x81,
  KEY_F19 = 0x82,
  KEY_F20 = 0x83,
  KEY_F21 = 0x84,
  KEY_F22 = 0x85,
  KEY_F23 = 0x86,
  KEY_F24 = 0x87,
  KEY_NUMLOCK = 0x90,
  KEY_SCROLL = 0x91,
  KEY_LSHIFT = 0xA0,
  KEY_RSHIFT = 0xA1,
  KEY_LCONTROL = 0xA2,
  KEY_RCONTROL = 0xA3,
  KEY_LMENU = 0xA4,
  KEY_RMENU = 0xA5,
  KEY_BROWSER_BACK = 0xA6,
  KEY_BROWSER_FORWARD = 0xA7,
  KEY_BROWSER_REFRESH = 0xA8,
  KEY_BROWSER_STOP = 0xA9,
  KEY_BROWSER_SEARCH = 0xAA,
  KEY_BROWSER_FAVORITES = 0xAB,
  KEY_BROWSER_HOME = 0xAC,
  KEY_VOLUME_MUTE = 0xAD,
  KEY_VOLUME_DOWN = 0xAE,
  KEY_VOLUME_UP = 0xAF,
  KEY_MEDIA_NEXT_TRACK = 0xB0,
  KEY_MEDIA_PREV_TRACK = 0xB1,
  KEY_MEDIA_STOP = 0xB2,
  KEY_MEDIA_PLAY_PAUSE = 0xB3,
  KEY_LAUNCH_MAIL = 0xB4,
  KEY_LAUNCH_MEDIA_SELECT = 0xB5,
  KEY_LAUNCH_APP1 = 0xB6,
  KEY_LAUNCH_APP2 = 0xB7,
  KEY_OEM_1 = 0xBA,
  KEY_OEM_PLUS = 0xBB,
  KEY_OEM_COMMA = 0xBC,
  KEY_OEM_MINUS = 0xBD,
  KEY_OEM_PERIOD = 0xBE,
  KEY_OEM_2 = 0xBF,
  KEY_OEM_3 = 0xC0,
  KEY_OEM_4 = 0xDB,
  KEY_OEM_5 = 0xDC,
  KEY_OEM_6 = 0xDD,
  KEY_OEM_7 = 0xDE,
  KEY_OEM_8 = 0xDF,
  KEY_OEM_102 = 0xE2,
  KEY_PROCESSKEY = 0xE5,
  KEY_PACKET = 0xE7,
  KEY_ATTN = 0xF6,
  KEY_CRSEL = 0xF7,
  KEY_EXSEL = 0xF8,
  KEY_EREOF = 0xF9,
  KEY_PLAY = 0xFA,
  KEY_ZOOM = 0xFB,
  KEY_NONAME = 0xFC,
  KEY_PA1 = 0xFD,
  KEY_OEM_CLEAR = 0xFE
} mioVirtualKeys;

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define INITGUID
#include <audioclient.h>
#include <immintrin.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <mmsystem.h>
#include <windows.h>
#include <windowsx.h>
#include <emmintrin.h>

#undef TRUE
#undef FALSE
#define TRUE 1
#define FALSE 0
#define MIO_ERROR -1
#define MIO_GLOBAL static

#define MIO_SET_BIT(x, n) ((x) | (1U << (n)))
#define MIO_CLEAR_BIT(x, n) ((x) & ~(1U << (n)))
#define MIO_TOGGLE_BIT(x, n) ((x) ^ (1U << (n)))
#define MIO_TEST_BIT(x, n) (((x) >> (n)) & 1U)
#define MIO_IS_BIT_SET(x, n) (((x) & (1U << (n))) != 0)
#define MIO_IS_BIT_CLEAR(x, n) (((x) & (1U << (n))) == 0)

#define MIO_SET_BITS(x, mask) ((x) | (mask))
#define MIO_CLEAR_BITS(x, mask) ((x) & ~(mask))
#define MIO_TOGGLE_BITS(x, mask) ((x) ^ (mask))
#define MIO_TEST_BITS(x, mask) ((x) & (mask))

#define MIO_ALIGN_UP(x, align) (((x) + (align) - 1) & ~((align) - 1))
#define MIO_ALIGN_DOWN(x, align) ((x) & ~((align) - 1))
#define MIO_IS_ALIGNED(x, align) (((x) & ((align) - 1)) == 0)

#define MIO_PI 3.14159265358979323846
#define MIO_TAU 6.2831853071795862
#define MIO_EU 2.7182818284590452

#define MIO_INT8_MIN -128
#define MIO_INT8_MAX 127
#define MIO_UINT8_MAX 255

#define MIO_INT16_MIN -32768
#define MIO_INT16_MAX 32767
#define MIO_UINT16_MAX 65535

#define MIO_INT32_MIN -2147483648
#define MIO_INT32_MAX 2147483647
#define MIO_UINT32_MAX 4294967295U

#define MIO_INT64_MIN -9223372036854775808LL
#define MIO_INT64_MAX 9223372036854775807LL
#define MIO_UINT64_MAX 18446744073709551615ULL

#define MIO_REAL32_MIN 1.17549435e-38F
#define MIO_REAL32_MAX 3.40282347e+38F

#define MIO_REAL64_MIN 2.2250738585072014e-308
#define MIO_REAL64_MAX 1.7976931348623157e+308

#define MIO_SWAP(x, y, temp)                                                  \
  {                                                                           \
    temp = x;                                                                 \
    x = y;                                                                    \
    y = temp;                                                                 \
  }

#define MIO_RGBA(r, g, b, a)                                                  \
  (uint32)(                                                                   \
      (((uint8)(a)) << 24) |                                                  \
      (((uint8)((r)) << 16) | (((uint8)(g)) << 8) | (((uint8)(b)))))

#define MIO_MIN(a, b) ((a) < (b) ? (a) : (b))
#define MIO_MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIO_CLAMP(v, min, max) MIO_MIN(MIO_MAX(v, min), max)
#define MIO_ABS(a) ((a) < 0 ? -(a) : (a))
#define MIO_SIGN(a) ((a) > 0 ? 1 : ((a) < 0 ? -1 : 0))
#define MIO_FABS(__x) ((__x) < 0 ? -(__x) : (__x))
#define MIO_FLOOR(x)                                                          \
  ((uint32)(x) - (((x) < 0) && (MIO_FABS((x) - (uint32)(x)) > DBL_EPSILON)))
#define MIO_CEIL(x)                                                           \
  ((uint32)(x) + (((x) > 0) && (MIO_FABS((x) - (uint32)(x)) > DBL_EPSILON)))

#define MIO_DOT2(g, x, y) ((real64)g[0] * (x) + (real64)g[1] * (y))
#define MIO_DOT3(g, x, y, z)                                                  \
  ((real64)g[0] * (x) + (real64)g[1] * (y) + (real64)g[2] * (z))
#define MIO_DOT4(g, x, y, z, w)                                               \
  ((real64)g[0] * (x) + (real64)g[1] * (y) + (real64)g[2] * (z) +             \
   (real64)g[3] * (w))
#define MIO_DOT2A(ax, ay, x, y) ((real64)(ax) * (x) + (real64)(ay) * (y))
#define MIO_DOT3A(ax, ay, az, x, y, z)                                        \
  ((real64)(ax) * (x) + (real64)(ay) * (y) + (real64)(az) * (z))
#define MIO_DOT4A(ax, ay, az, aw, x, y, z, w)                                 \
  ((real64)(ax) * (x) + (real64)(ay) * (y) + (real64)(az) * (z) +             \
   (real64)(aw) * (w))

#define MIO_MIX(a, b, t) (((1.0 - (t)) * (a)) + (t) * (b))

#define MIO_DEG_TO_RAD(d) ((d) * (MIO_PI / 180.0))
#define MIO_RAD_TO_DEG(r) ((r) * (180.0 / MIO_PI))

#define MIO_LERP(a, b, t) ((a) + ((b) - (a)) * (t))
#define MIO_SMOOTHSTEP(t) ((t) * (t) * (3.0 - 2.0 * (t)))
#define MIO_SMOOTHERSTEP(t)                                                   \
  ((t) * (t) * (t) * ((t) * ((t) * 6.0 - 15.0) + 10.0))

#define MIO_EPSILON_F 1e-6F
#define MIO_EPSILON_D 1e-9
#define MIO_F_EQUAL(a, b) (MIO_ABS((a) - (b)) < MIO_EPSILON_F)
#define MIO_D_EQUAL(a, b) (MIO_ABS((a) - (b)) < MIO_EPSILON_D)

#define MIO_COL_SET_A(color, a) (((color) & 0x00FFFFFF) | ((a) << 24))
#define MIO_COL_SET_R(color, r) (((color) & 0xFF00FFFF) | ((r) << 0))
#define MIO_COL_SET_G(color, g) (((color) & 0xFFFF00FF) | ((g) << 8))
#define MIO_COL_SET_B(color, b) (((color) & 0xFFFFFF00) | ((b) << 16))
#define MIO_COL_GET_A(color) (((color) >> 24) & 0xFF)
#define MIO_COL_GET_R(color) (((color) >> 16) & 0xFF)
#define MIO_COL_GET_G(color) (((color) >> 8) & 0xFF)
#define MIO_COL_GET_B(color) (((color) >> 0) & 0xFF)

#define MIO_WORLD_TO_SCREEN_X(wx, sw, camx, scale)                            \
  (((wx) - (camx)) * (scale) + (sw / 2.0))
#define MIO_WORLD_TO_SCREEN_Y(wy, sh, camy, scale)                            \
  (((wy) - (camy)) * (scale) + (sh / 2.0))

#define MIO_SCREEN_TO_WORLD_X(sx, sw, camx, scale)                            \
  (((sx) - (sw / 2.0)) / (scale) + (camx))
#define MIO_SCREEN_TO_WORLD_Y(sy, sh, camy, scale)                            \
  (((sy) - (sh / 2.0)) / (scale) + (camy))

#define MIO_3D_DEPTH_TEST 0x0001
#define MIO_3D_AFFINE_MAP 0x0002
#define MIO_3D_VERTEX_SNAP 0x0004
#define MIO_3D_SHADE_FLAT 0x0008
#define MIO_3D_WIREFRAME 0x0010
#define MIO_3D_SOLID 0x0020
#define MIO_3D_TEXTURE 0x0040
#define MIO_3D_CULL_BACKFACE 0x0080
#define MIO_3D_CULL_FRUSTUM 0x0100
#define MIO_3D_CULL_BEHIND 0x0200
#define MIO_3D_CLIP_FRUSTUM 0x0400
#define MIO_3D_CENTER_POINTS 0x0800
#define MIO_3D_NORMALS 0x1000

typedef struct {
  int32 x, y, width, height;
} mioSystemRect;

typedef struct {
  SYSRET init;
  SYSRET wasapi;
  IMMDeviceEnumerator *de;
  IMMDevice *dev;
  IAudioClient *ac;
  IAudioRenderClient *rc;
  HANDLE ev;
  HANDLE th;
  WAVEFORMATEX *fmt;
  HWAVEOUT wo;
  WAVEHDR wh[SYS_AUDIO_BUFFER_COUNT];
  int16 *wb[SYS_AUDIO_BUFFER_COUNT];
  real32 *wb_f[SYS_AUDIO_BUFFER_COUNT];
  mioSystemAudioFormat afmt;
  PFSYSTEMAUDIOCB cb;
  void *ud;
  SYSRET stop;
  void *audioBuffer;
} mioSystemAudioState;

typedef struct {
  HWND hwnd;
  HDC hdc;
  SYSRET running;
  SYSRET isFullscreen;
  SYSRET isMinimized;
  SYSRET keys[SYS_MAX_KEYS];
  SYSRET kp[SYS_MAX_KEYS];
  SYSRET kr[SYS_MAX_KEYS];
  SYSRET mb[SYS_MAX_MOUSE_BUTTONS];
  SYSRET mp[SYS_MAX_MOUSE_BUTTONS];
  SYSRET mr[SYS_MAX_MOUSE_BUTTONS];
  mioSystemPoint mpos;
  int32 mwd;
  mioSystemSize ws;
  mioSystemSize cs;
  mioSystemRect pre_fs;
  uint32 *fb;
  int32 fb_w, fb_h;
  LARGE_INTEGER freq;
  LARGE_INTEGER st;
  LARGE_INTEGER lt;
  real64 target_dt;
  real64 dtAcc;
  real64 fpsCount;
  uint32 fpsUpdateCount;
  mioSystemAudioState audio;
} mioSystemState;

typedef struct {
  real32 x1;
  real32 y1;
  real32 x2;
  real32 y2;
} mioRect;

typedef struct {
  real32 x, y;
} mioVec2;

typedef struct {
  real32 x, y, z;
} mioVec3;

typedef struct {
  real32 x, y, z, w;
} mioVec4;

typedef struct {
  real32 x, y, z, w;
} mioQuat;

typedef struct {
  real32 m[4];
} mioMat2;

typedef struct {
  real32 m[9];
} mioMat3;

typedef struct {
  real32 m[16];
} mioMat4;

typedef struct {
  mioVec4 pos;
  mioVec3 normal;
  mioVec2 uv;
} mioVertex;

typedef struct {
  mioVertex *vertices;
  int32 *indices;
  int32 vertexCount;
  int32 indexCount;
  mioVec3 boundsMin;
  mioVec3 boundsMax;
} mioMesh;

typedef struct {
  uint32 *data;
  uint32 width;
  uint32 height;
} mioTexture;

typedef struct {
  mioVec3 pos;
  real32 fov;
  real32 yaw;
  real32 pitch;
  real32 width;
  real32 height;
  real32 nearPlane;
  real32 farPlane;
  real32 moveSpeed;
  real32 lookSpeed;
  real32 zoom;
  mioMat4 projection;
} mioCamera;

typedef struct {
  int32 width;
  int32 height;
  mioRect clip;
  uint32 flags2D;
  uint32 flags3D;
  uint32 *colourData;
  real32 *depthData;
  size_t maxFramebufferSize;
  size_t maxDataSize;
  real32 sunX;
  real32 sunY;
  real32 sunZ;
  real32 sunFactor;
  real32 ambient;
	int32 resolution;
	mioCamera camera;
} mioRenderContext;

typedef struct {
  void *state;
  int32 (*init)(void *);
  int32 (*update)(void *, real64);
  int32 (*draw)(void *);
  int32 (*shutdown)(void *);
  mioRenderContext render;
  uint32 colour;
} mioApp;

MIO_GLOBAL mioApp G_APP = {0};
MIO_GLOBAL mioSystemState G_SYS = {0};
MIO_GLOBAL SYSRET G_VERBOSE_MODE = 1;
MIO_GLOBAL real64 G_LAST_DEBUG_TIME = 0.0;

MIO_GLOBAL LRESULT CALLBACK
sys_wnd_proc(HWND hw, UINT msg, WPARAM wp, LPARAM lp);

mioRect mio_rect(real32 x1, real32 y1, real32 x2, real32 y2) {
  mioRect ret;
  ret.x1 = x1;
  ret.y1 = y1;
  ret.x2 = x2;
  ret.y2 = y2;
  return ret;
}

mioRect mio_rect_clip(mioRect a, mioRect b) {
  return mio_rect(
      a.x1 > b.x1   ? a.x1
      : b.x1 < a.x2 ? b.x1
                    : a.x2,
      a.y1 > b.y1   ? a.y1
      : b.y1 < a.y2 ? b.y1
                    : a.y2,
      a.x2 < b.x2   ? a.x2
      : b.x2 > a.x1 ? b.x2
                    : a.x1,
      a.y2 < b.y2   ? a.y2
      : b.y2 > a.y1 ? b.y2
                    : a.y1);
}

int32 mio_rect_bounds(mioRect r, int32 x, int32 y) {
  return x >= r.x1 && y >= r.y1 && x < r.x2 && y < r.y2;
}

mioRenderContext mio_render_context(void *memory, size_t size, int32 depth, int32 res) {
  mioRenderContext ctx;
  uint8 *mem = memory ? (uint8 *)memory : NULL;
  memset(&ctx, 0, sizeof(ctx));
  if (memory) {
    memset(memory, 0, size);
    ctx.colourData = memory;
    ctx.maxDataSize = size;
    ctx.maxFramebufferSize = depth ? size / 2 : size;
    ctx.depthData = depth ? (real32 *)(mem + ctx.maxFramebufferSize) : NULL;
  }
	ctx.resolution = res;
  ctx.sunX = 0.4f;
  ctx.sunY = 0.2;
  ctx.sunZ = -0.2;
  ctx.sunFactor = 1.0f;
  ctx.ambient = 0.04;
  ctx.flags3D = MIO_3D_SOLID | MIO_3D_TEXTURE | MIO_3D_SHADE_FLAT |
                MIO_3D_CULL_BACKFACE | MIO_3D_CULL_BEHIND |
                MIO_3D_CULL_FRUSTUM | MIO_3D_CLIP_FRUSTUM | MIO_3D_DEPTH_TEST;
	return ctx;
}

int32 mio_render_context_resize(int32 w, int32 h) {
  mioRenderContext *ctx = &G_APP.render;
  ctx->width = w;
  ctx->height = h;
  ctx->clip = mio_rect(0, 0, (real32)w, (real32)h);
  G_SYS.fb = ctx->colourData;
  G_SYS.fb_w = w;
  G_SYS.fb_h = h;
  return TRUE;
}

MIO_GLOBAL void sys_log(const char *format, ...) {
  va_list args;
  if (G_VERBOSE_MODE) {
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
  }
}

MIO_GLOBAL void sys_clear_input(void) {
  int32 i;
  for (i = 0; i < SYS_MAX_KEYS; i++) {
    G_SYS.kp[i] = 0;
    G_SYS.kr[i] = 0;
  }
  for (i = 0; i < SYS_MAX_MOUSE_BUTTONS; i++) {
    G_SYS.mp[i] = 0;
    G_SYS.mr[i] = 0;
  }
  G_SYS.mwd = 0;
}

MIO_GLOBAL DWORD WINAPI sys_wasapi_thread(LPVOID p) {
  HRESULT hr;
  uint32 bfc, pad, avail;
  uint8 *data;
  real32 *fb;
  int32 i;
  (void)p;
  hr = G_SYS.audio.ac->lpVtbl->GetBufferSize(G_SYS.audio.ac, &bfc);
  if (FAILED(hr)) { return 1; }
  fb = (real32 *)sys_alloc(bfc * G_SYS.audio.afmt.ch * sizeof(real32));
  if (!fb) { return 1; }
  while (!G_SYS.audio.stop) {
    WaitForSingleObject(G_SYS.audio.ev, INFINITE);
    if (G_SYS.audio.stop) { break; }
    hr = G_SYS.audio.ac->lpVtbl->GetCurrentPadding(G_SYS.audio.ac, &pad);
    if (FAILED(hr)) { break; }
    avail = bfc - pad;
    if (avail > 0) {
      hr = G_SYS.audio.rc->lpVtbl->GetBuffer(G_SYS.audio.rc, avail, &data);
      if (FAILED(hr)) { break; }
      if (G_SYS.audio.cb) {
        G_SYS.audio.cb(G_SYS.audio.ud, fb, (int32)avail);
        if (G_SYS.audio.fmt->wBitsPerSample == 16) {
          int16 *sd = (int16 *)data;
          for (i = 0; i < (int32)(avail * G_SYS.audio.afmt.ch); i++) {
            real32 s = fb[i];
            if (s > 1.0f) { s = 1.0f; }
            if (s < -1.0f) { s = -1.0f; }
            sd[i] = (int16)(s * 32767.0f);
          }
        } else if (G_SYS.audio.fmt->wBitsPerSample == 32) {
          real32 *fd = (real32 *)data;
          memcpy(fd, fb, avail * G_SYS.audio.afmt.ch * sizeof(real32));
        }
      } else {
        memset(data, 0, avail * G_SYS.audio.afmt.bpf);
      }
      hr = G_SYS.audio.rc->lpVtbl->ReleaseBuffer(G_SYS.audio.rc, avail, 0);
      if (FAILED(hr)) { break; }
    }
  }
  sys_free(fb);
  return 0;
}

MIO_GLOBAL void CALLBACK sys_waveout_cb(
    HWAVEOUT hwo, UINT msg, DWORD_PTR di, DWORD_PTR dp1, DWORD_PTR dp2) {
  mioSystemAudioState *audio;
  int32 index;
  WAVEHDR *h;
  real32 *fb;
  int32 i;
  (void)hwo;
  (void)dp2;
  if (msg != WOM_DONE || G_SYS.audio.stop) { return; }
  h = (WAVEHDR *)dp1;
  audio = (mioSystemAudioState *)di;
  index = h - audio->wh;
  fb = audio->wb_f[index];

  if (audio->cb) {
    audio->cb(audio->ud, fb, audio->afmt.bufferFrames);
  } else {
    memset(fb, 0, audio->afmt.bufferFrames * audio->afmt.ch * sizeof(real32));
  }
  for (i = 0; i < audio->afmt.bufferFrames * audio->afmt.ch; i++) {
    real32 s = fb[i];
    if (s > 1.0f) { s = 1.0f; }
    if (s < -1.0f) { s = -1.0f; }
    ((int16 *)h->lpData)[i] = (int16)(s * 32767.0f);
  }
  waveOutWrite(G_SYS.audio.wo, h, sizeof(WAVEHDR));
}

MIO_GLOBAL SYSRET sys_init_wasapi(mioSystemAudioFormat *fmt) {
  HRESULT hr;
  REFERENCE_TIME bd = 200000;
  uint32 bfc;
  sys_log("Attempting WASAPI initialization...\n");
  hr = CoCreateInstance(
      &CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator,
      (void **)&G_SYS.audio.de);
  if (FAILED(hr)) {
    sys_log("Failed to create IMMDeviceEnumerator.\n");
    return 0;
  }
  hr = G_SYS.audio.de->lpVtbl->GetDefaultAudioEndpoint(
      G_SYS.audio.de, eRender, eConsole, &G_SYS.audio.dev);
  if (FAILED(hr)) {
    sys_log("Failed to get default audio endpoint.\n");
    return 0;
  }
  hr = G_SYS.audio.dev->lpVtbl->Activate(
      G_SYS.audio.dev, &IID_IAudioClient, CLSCTX_ALL, NULL,
      (void **)&G_SYS.audio.ac);
  if (FAILED(hr)) {
    sys_log("Failed to activate IAudioClient.\n");
    return 0;
  }
  hr = G_SYS.audio.ac->lpVtbl->GetMixFormat(G_SYS.audio.ac, &G_SYS.audio.fmt);
  if (FAILED(hr)) {
    sys_log("Failed to get mix format.\n");
    return 0;
  }
  fmt->sr = G_SYS.audio.fmt->nSamplesPerSec;
  fmt->ch = G_SYS.audio.fmt->nChannels;
  fmt->bps = G_SYS.audio.fmt->wBitsPerSample;
  fmt->bpf = G_SYS.audio.fmt->nBlockAlign;
  sys_log(
      "WASAPI mix format: sample rate=%d, channels=%d, "
      "bits per sample=%d.\n",
      fmt->sr, fmt->ch, fmt->bps);
  hr = G_SYS.audio.ac->lpVtbl->Initialize(
      G_SYS.audio.ac, AUDCLNT_SHAREMODE_SHARED,
      AUDCLNT_STREAMFLAGS_EVENTCALLBACK, bd, 0, G_SYS.audio.fmt, NULL);
  if (FAILED(hr)) {
    sys_log("Failed to initialize IAudioClient.\n");
    return 0;
  }
  hr = G_SYS.audio.ac->lpVtbl->GetBufferSize(G_SYS.audio.ac, &bfc);
  if (FAILED(hr)) {
    sys_log("Failed to get buffer size.\n");
    return 0;
  }
  fmt->bufferFrames = bfc;
  sys_log("WASAPI buffer size: %d frames.\n", fmt->bufferFrames);
  G_SYS.audio.ev = CreateEvent(NULL, FALSE, FALSE, NULL);
  if (!G_SYS.audio.ev) {
    sys_log("Failed to create event.\n");
    return 0;
  }
  hr = G_SYS.audio.ac->lpVtbl->SetEventHandle(G_SYS.audio.ac, G_SYS.audio.ev);
  if (FAILED(hr)) {
    sys_log("Failed to set event handle.\n");
    return 0;
  }
  hr = G_SYS.audio.ac->lpVtbl->GetService(
      G_SYS.audio.ac, &IID_IAudioRenderClient, (void **)&G_SYS.audio.rc);
  if (FAILED(hr)) {
    sys_log("Failed to get IAudioRenderClient service.\n");
    return 0;
  }
  G_SYS.audio.th = CreateThread(NULL, 0, sys_wasapi_thread, NULL, 0, NULL);
  if (!G_SYS.audio.th) {
    sys_log("Failed to create WASAPI thread.\n");
    return 0;
  }
  hr = G_SYS.audio.ac->lpVtbl->Start(G_SYS.audio.ac);
  if (FAILED(hr)) {
    sys_log("Failed to start audio client.\n");
    return 0;
  }
  return 1;
}

MIO_GLOBAL void sys_shutdown_wasapi(void) {
  sys_log("Shutting down WASAPI...\n");
  G_SYS.audio.stop = 1;
  SetEvent(G_SYS.audio.ev);
  if (G_SYS.audio.ac) { G_SYS.audio.ac->lpVtbl->Stop(G_SYS.audio.ac); }
  if (G_SYS.audio.th) {
    WaitForSingleObject(G_SYS.audio.th, 5000);
    CloseHandle(G_SYS.audio.th);
    G_SYS.audio.th = NULL;
  }
  if (G_SYS.audio.ev) {
    CloseHandle(G_SYS.audio.ev);
    G_SYS.audio.ev = NULL;
  }
  if (G_SYS.audio.rc) {
    G_SYS.audio.rc->lpVtbl->Release(G_SYS.audio.rc);
    G_SYS.audio.rc = NULL;
  }
  if (G_SYS.audio.ac) {
    G_SYS.audio.ac->lpVtbl->Release(G_SYS.audio.ac);
    G_SYS.audio.ac = NULL;
  }
  if (G_SYS.audio.fmt) {
    CoTaskMemFree(G_SYS.audio.fmt);
    G_SYS.audio.fmt = NULL;
  }
  if (G_SYS.audio.dev) {
    G_SYS.audio.dev->lpVtbl->Release(G_SYS.audio.dev);
    G_SYS.audio.dev = NULL;
  }
  if (G_SYS.audio.de) {
    G_SYS.audio.de->lpVtbl->Release(G_SYS.audio.de);
    G_SYS.audio.de = NULL;
  }
  sys_log("WASAPI shutdown complete.\n");
}

MIO_GLOBAL SYSRET sys_init_waveout(mioSystemAudioFormat *fmt) {
  WAVEFORMATEX wfx;
  MMRESULT r;
  int32 i, j;
  sys_log("Attempting WaveOut initialization...\n");
  wfx.wFormatTag = WAVE_FORMAT_PCM;
  wfx.nChannels = (WORD)fmt->ch;
  wfx.nSamplesPerSec = (DWORD)fmt->sr;
  wfx.wBitsPerSample = (WORD)fmt->bps;
  wfx.nBlockAlign = wfx.nChannels * (wfx.wBitsPerSample / 8);
  wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
  wfx.cbSize = 0;
  fmt->bpf = wfx.nBlockAlign;
  fmt->bufferFrames = fmt->sr / 10;
  sys_log(
      "WaveOut format: sample rate=%d, channels=%d, "
      "bits per sample=%d, buffer frames=%d.\n",
      fmt->sr, fmt->ch, fmt->bps, fmt->bufferFrames);
  r = waveOutOpen(
      &G_SYS.audio.wo, WAVE_MAPPER, &wfx, (DWORD_PTR)sys_waveout_cb,
      (DWORD_PTR)&G_SYS.audio, CALLBACK_FUNCTION);
  if (r != MMSYSERR_NOERROR) {
    sys_log("Failed to open WaveOut device. Error code: %d.\n", r);
    return 0;
  }
  G_SYS.audio.audioBuffer = sys_alloc(
      SYS_AUDIO_BUFFER_COUNT * (fmt->bufferFrames * fmt->bpf) +
      SYS_AUDIO_BUFFER_COUNT * (fmt->bufferFrames * fmt->ch * sizeof(real32)));
  if (!G_SYS.audio.audioBuffer) {
    sys_log("Failed to allocate audio buffer memory.\n");
    waveOutClose(G_SYS.audio.wo);
    return 0;
  }

  for (i = 0; i < SYS_AUDIO_BUFFER_COUNT; i++) {
    int32 bsz = fmt->bufferFrames * fmt->bpf;
    G_SYS.audio.wb[i] = (int16 *)((char *)G_SYS.audio.audioBuffer + i * bsz);
    G_SYS.audio.wb_f[i] =
        (real32 *)((char *)G_SYS.audio.audioBuffer +
                   SYS_AUDIO_BUFFER_COUNT * bsz +
                   i * (fmt->bufferFrames * fmt->ch * sizeof(real32)));

    memset(&G_SYS.audio.wh[i], 0, sizeof(WAVEHDR));
    G_SYS.audio.wh[i].lpData = (LPSTR)G_SYS.audio.wb[i];
    G_SYS.audio.wh[i].dwBufferLength = (DWORD)bsz;
    G_SYS.audio.wh[i].dwFlags = 0;
    waveOutPrepareHeader(G_SYS.audio.wo, &G_SYS.audio.wh[i], sizeof(WAVEHDR));
  }

  for (i = 0; i < SYS_AUDIO_BUFFER_COUNT; i++) {
    if (G_SYS.audio.cb) {
      G_SYS.audio.cb(G_SYS.audio.ud, G_SYS.audio.wb_f[i], fmt->bufferFrames);
    } else {
      memset(
          G_SYS.audio.wb_f[i], 0,
          fmt->bufferFrames * fmt->ch * sizeof(real32));
    }
    for (j = 0; j < fmt->bufferFrames * fmt->ch; j++) {
      real32 s = G_SYS.audio.wb_f[i][j];
      if (s > 1.0f) { s = 1.0f; }
      if (s < -1.0f) { s = -1.0f; }
      ((int16 *)G_SYS.audio.wh[i].lpData)[j] = (int16)(s * 32767.0f);
    }
    waveOutWrite(G_SYS.audio.wo, &G_SYS.audio.wh[i], sizeof(WAVEHDR));
  }
  return 1;
}

MIO_GLOBAL void sys_shutdown_waveout(void) {
  int32 i;
  sys_log("Shutting down WaveOut...\n");
  if (G_SYS.audio.wo) {
    waveOutReset(G_SYS.audio.wo);
    for (i = 0; i < SYS_AUDIO_BUFFER_COUNT; i++) {
      if (G_SYS.audio.wh[i].dwFlags & WHDR_PREPARED) {
        waveOutUnprepareHeader(
            G_SYS.audio.wo, &G_SYS.audio.wh[i], sizeof(WAVEHDR));
      }
    }
    waveOutClose(G_SYS.audio.wo);
    G_SYS.audio.wo = NULL;
  }
  if (G_SYS.audio.audioBuffer) {
    sys_free(G_SYS.audio.audioBuffer);
    G_SYS.audio.audioBuffer = NULL;
  }
  sys_log("WaveOut shutdown complete.\n");
}

MIO_GLOBAL int32 sys_set_timer(uint32 milis) {
  SetTimer(G_SYS.hwnd, 1, milis, NULL);
  return TRUE;
}

MIO_GLOBAL LRESULT CALLBACK
sys_wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  switch (msg) {
  case WM_NCCALCSIZE: {
    LPRECT client_rect = (LPRECT)lp;
    if (wp == TRUE) {
      client_rect->left += 0;
      client_rect->top += 0;
      client_rect->right -= 0;
      client_rect->bottom -= 0;
      return 0;
    }
  } break;
  case WM_NCHITTEST: {
    const int32 bt = 4;
    const int32 th = 30;
    POINT pt;
    RECT window_rect;
    int32 x;
    int32 y;
    int32 w;
    int32 h;
    pt.x = GET_X_LPARAM(lp);
    pt.y = GET_Y_LPARAM(lp);
    GetWindowRect(hwnd, &window_rect);
    x = pt.x - window_rect.left;
    y = pt.y - window_rect.top;
    w = window_rect.right - window_rect.left;
    h = window_rect.bottom - window_rect.top;
    if (x < bt && y < bt) { return HTTOPLEFT; }
    if (x > w - bt && y < bt) { return HTTOPRIGHT; }
    if (x < bt && y > h - bt) { return HTBOTTOMLEFT; }
    if (x > w - bt && y > h - bt) { return HTBOTTOMRIGHT; }
    if (x < bt) { return HTLEFT; }
    if (x > w - bt) { return HTRIGHT; }
    if (y < bt) { return HTTOP; }
    if (y > h - bt) { return HTBOTTOM; }
    if (y < th) { return HTCAPTION; }
    return HTCLIENT;
  }
  case WM_PAINT: {
    BITMAPINFO bmi;
    mioSystemSize wndSz = sys_get_window_size();
    HDC hdc = GetDC(hwnd);
    memset(&bmi, 0, sizeof(BITMAPINFO));
    if (G_APP.render.colourData) {
      G_APP.render.width = wndSz.width/G_APP.render.resolution;
      G_APP.render.height = wndSz.height/G_APP.render.resolution;
      G_SYS.fb_w = G_APP.render.width;
      G_SYS.fb_h = G_APP.render.height;
      if (G_APP.draw) { G_APP.draw(G_APP.state); }
    }
    if (G_SYS.fb) {
      bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
      bmi.bmiHeader.biWidth = G_SYS.fb_w;
      bmi.bmiHeader.biHeight = -G_SYS.fb_h;
      bmi.bmiHeader.biPlanes = 1;
      bmi.bmiHeader.biBitCount = 32;
      bmi.bmiHeader.biCompression = BI_RGB;
      StretchDIBits(
          hdc, 0, 0, G_SYS.cs.width, G_SYS.cs.height, 0, 0, G_SYS.fb_w,
          G_SYS.fb_h, G_SYS.fb, &bmi, DIB_RGB_COLORS, SRCCOPY);
    }
    ReleaseDC(hwnd, hdc);
  } break;
  case WM_SIZE:
    if (wp == SIZE_MINIMIZED) {
      G_SYS.isMinimized = 1;
    } else {
      G_SYS.isMinimized = 0;
      G_SYS.cs.width = LOWORD(lp);
      G_SYS.cs.height = HIWORD(lp);
    }
    mio_render_context_resize(G_SYS.cs.width, G_SYS.cs.height);
    break;
  case WM_SIZING:
    break;
  case WM_ENTERSIZEMOVE:
    break;
  case WM_ERASEBKGND:
    return 1;
  case WM_EXITSIZEMOVE:
    InvalidateRect(G_SYS.hwnd, NULL, FALSE);
    break;
  case WM_CLOSE:
    G_SYS.running = 0;
    return 0;
  case WM_KEYDOWN:
    if (wp < SYS_MAX_KEYS) {
      if (!G_SYS.keys[wp]) { G_SYS.kp[wp] = 1; }
      G_SYS.keys[wp] = 1;
    }
    break;
  case WM_KEYUP:
    if (wp < SYS_MAX_KEYS) {
      G_SYS.kr[wp] = 1;
      G_SYS.keys[wp] = 0;
    }
    break;
  case WM_LBUTTONDOWN:
    if (!G_SYS.mb[0]) { G_SYS.mp[0] = 1; }
    G_SYS.mb[0] = 1;
    break;
  case WM_LBUTTONUP:
    G_SYS.mr[0] = 1;
    G_SYS.mb[0] = 0;
    break;
  case WM_RBUTTONDOWN:
    if (!G_SYS.mb[1]) { G_SYS.mp[1] = 1; }
    G_SYS.mb[1] = 1;
    break;
  case WM_RBUTTONUP:
    G_SYS.mr[1] = 1;
    G_SYS.mb[1] = 0;
    break;
  case WM_MBUTTONDOWN:
    if (!G_SYS.mb[2]) { G_SYS.mp[2] = 1; }
    G_SYS.mb[2] = 1;
    break;
  case WM_MBUTTONUP:
    G_SYS.mr[2] = 1;
    G_SYS.mb[2] = 0;
    break;
  case WM_MOUSEMOVE:
    G_SYS.mpos.x = GET_X_LPARAM(lp);
    G_SYS.mpos.y = GET_Y_LPARAM(lp);
    break;
  case WM_MOUSEWHEEL:
    G_SYS.mwd = GET_WHEEL_DELTA_WPARAM(wp) / WHEEL_DELTA;
    break;
  }
  return DefWindowProc(hwnd, msg, wp, lp);
}

real64 sys_get_time(void) {
  LARGE_INTEGER cur;
  QueryPerformanceCounter(&cur);
  return (real64)(cur.QuadPart - G_SYS.st.QuadPart) /
         (real64)G_SYS.freq.QuadPart;
}

real64 sys_get_delta_time(void) {
  LARGE_INTEGER cur;
  real64 dt;
  QueryPerformanceCounter(&cur);
  dt =
      (real64)(cur.QuadPart - G_SYS.lt.QuadPart) / (real64)G_SYS.freq.QuadPart;
  G_SYS.lt = cur;
  return dt;
}

SYSRET
sys_init(const char *title, int32 width, int32 height, SYSRET borderless) {
  WNDCLASSA wc;
  RECT rect;
  DWORD dwStyle;
  memset(&wc, 0, sizeof(WNDCLASSA));
  memset(&G_SYS, 0, sizeof(G_SYS));
  sys_log("Initializing system...\n");
  QueryPerformanceFrequency(&G_SYS.freq);
  QueryPerformanceCounter(&G_SYS.st);
  G_SYS.lt = G_SYS.st;
  G_LAST_DEBUG_TIME = sys_get_time();
  sys_log("Setting up window class...\n");
  wc.lpfnWndProc = sys_wnd_proc;
  wc.hInstance = GetModuleHandle(NULL);
  wc.lpszClassName = "PlatformWindow";
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
  wc.style = CS_HREDRAW | CS_VREDRAW;
  if (!RegisterClassA(&wc)) {
    sys_log("Failed to register window class.\n");
    return 0;
  }
  if (borderless) {
    dwStyle = WS_POPUP | WS_THICKFRAME;
    G_SYS.ws.width = width;
    G_SYS.ws.height = height;
    G_SYS.cs.width = width;
    G_SYS.cs.height = height;
  } else {
    dwStyle = WS_OVERLAPPEDWINDOW;
    sys_log(
        "Calculating window size for client area %dx%d...\n", width, height);
    rect.left = 0;
    rect.top = 0;
    rect.right = width;
    rect.bottom = height;
    AdjustWindowRect(&rect, dwStyle, FALSE);
    G_SYS.ws.width = rect.right - rect.left;
    G_SYS.ws.height = rect.bottom - rect.top;
    G_SYS.cs.width = width;
    G_SYS.cs.height = height;
  }
  sys_log(
      "Creating window of size %dx%d...\n", G_SYS.ws.width, G_SYS.ws.height);
  G_SYS.hwnd = CreateWindowA(
      "PlatformWindow", title ? title : "Application", dwStyle, CW_USEDEFAULT,
      CW_USEDEFAULT, G_SYS.ws.width, G_SYS.ws.height, NULL, NULL,
      GetModuleHandle(NULL), NULL);
  if (!G_SYS.hwnd) {
    sys_log("Failed to create window.\n");
    return 0;
  }
  G_SYS.hdc = GetDC(G_SYS.hwnd);
  if (!G_SYS.hdc) {
    sys_log("Failed to get device context.\n");
    return 0;
  }
  ShowWindow(G_SYS.hwnd, SW_SHOW);
  UpdateWindow(G_SYS.hwnd);
  G_SYS.running = 1;
  sys_log("System initialization successful.\n");
  return 1;
}

SYSRET sys_process_messages(void) {
  MSG msg;
  sys_clear_input();
  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return G_SYS.running;
}

void sys_present(void) { InvalidateRect(G_SYS.hwnd, NULL, FALSE); }

void sys_shutdown(void) {
  sys_log("Shutting down system...\n");
  sys_shutdown_audio();
  if (G_SYS.hdc) {
    ReleaseDC(G_SYS.hwnd, G_SYS.hdc);
    G_SYS.hdc = NULL;
  }
  if (G_SYS.hwnd) {
    DestroyWindow(G_SYS.hwnd);
    G_SYS.hwnd = NULL;
  }
  UnregisterClassA("PlatformWindow", GetModuleHandle(NULL));
  sys_log("System shutdown complete.\n");
}

void sys_log_perf(void) {
  real64 current_time = sys_get_time();
  G_SYS.fpsCount++;
  if (current_time - G_LAST_DEBUG_TIME >= 1.0) {
    sys_log(
        "FPS (U): %d FPS (R): %d | Window Size: %dx%d | Mouse Pos: %d, %d\n",
        G_SYS.fpsUpdateCount, (uint32)G_SYS.fpsCount, G_SYS.cs.width,
        G_SYS.cs.height, G_SYS.mpos.x, G_SYS.mpos.y);
    G_LAST_DEBUG_TIME = current_time;
    G_SYS.fpsCount = 0;
    G_SYS.fpsUpdateCount = 0;
  }
}

void sys_quit(void) {
  sys_log("Quit requested.\n");
  G_SYS.running = 0;
}

mioSystemSize sys_get_window_size(void) { return G_SYS.cs; }

void sys_sleep(uint32 ms) { Sleep(ms); }

void sys_set_target_framerate(int32 fps) {
  if (fps > 0) {
    G_SYS.target_dt = 1.0 / (real64)fps;
  } else {
    G_SYS.target_dt = 0;
  }
}

void sys_minimize(void) { ShowWindow(G_SYS.hwnd, SW_MINIMIZE); }

void sys_maximize(void) { ShowWindow(G_SYS.hwnd, SW_MAXIMIZE); }

void sys_restore(void) { ShowWindow(G_SYS.hwnd, SW_RESTORE); }

void sys_toggle_fullscreen(void) {
  RECT rect;
  MONITORINFO mi;
  memset(&mi, 0, sizeof(MONITORINFO));
  mi.cbSize = sizeof(mi);
  if (G_SYS.isFullscreen) {
    SetWindowLongPtr(G_SYS.hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
    SetWindowPos(
        G_SYS.hwnd, HWND_NOTOPMOST, G_SYS.pre_fs.x, G_SYS.pre_fs.y,
        G_SYS.pre_fs.width, G_SYS.pre_fs.height,
        SWP_FRAMECHANGED | SWP_NOACTIVATE);
    G_SYS.isFullscreen = 0;
  } else {
    GetWindowRect(G_SYS.hwnd, &rect);
    G_SYS.pre_fs.x = rect.left;
    G_SYS.pre_fs.y = rect.top;
    G_SYS.pre_fs.width = rect.right - rect.left;
    G_SYS.pre_fs.height = rect.bottom - rect.top;
    GetMonitorInfo(
        MonitorFromWindow(G_SYS.hwnd, MONITOR_DEFAULTTOPRIMARY), &mi);
    SetWindowLongPtr(G_SYS.hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
    SetWindowPos(
        G_SYS.hwnd, HWND_TOPMOST, mi.rcMonitor.left, mi.rcMonitor.top,
        mi.rcMonitor.right - mi.rcMonitor.left,
        mi.rcMonitor.bottom - mi.rcMonitor.top,
        SWP_FRAMECHANGED | SWP_NOACTIVATE);
    G_SYS.isFullscreen = 1;
  }
}

SYSRET sys_key_down(int32 key) {
  if (key < 0 || key >= SYS_MAX_KEYS) { return 0; }
  return G_SYS.keys[key];
}

SYSRET sys_key_pressed(int32 key) {
  if (key < 0 || key >= SYS_MAX_KEYS) { return 0; }
  return G_SYS.kp[key];
}

SYSRET sys_key_released(int32 key) {
  if (key < 0 || key >= SYS_MAX_KEYS) { return 0; }
  return G_SYS.kr[key];
}

SYSRET sys_mouse_down(int32 btn) {
  if (btn < 0 || btn >= SYS_MAX_MOUSE_BUTTONS) { return 0; }
  return G_SYS.mb[btn];
}

SYSRET sys_mouse_pressed(int32 btn) {
  if (btn < 0 || btn >= SYS_MAX_MOUSE_BUTTONS) { return 0; }
  return G_SYS.mp[btn];
}

SYSRET sys_mouse_released(int32 btn) {
  if (btn < 0 || btn >= SYS_MAX_MOUSE_BUTTONS) { return 0; }
  return G_SYS.mr[btn];
}

mioSystemPoint sys_mouse_position(void) { return G_SYS.mpos; }

int32 sys_mouse_wheel(void) { return G_SYS.mwd; }

SYSRET sys_init_audio(int32 sr, int32 ch, PFSYSTEMAUDIOCB cb, void *ud) {
  mioSystemAudioFormat fmt;
  HRESULT hr;
  if (G_SYS.audio.init) {
    sys_log("Audio already initialized, shutting down first.\n");
    sys_shutdown_audio();
  }
  sys_log("Initializing audio with sample rate %d, channels %d...\n", sr, ch);
  fmt.sr = sr > 0 ? sr : SYS_AUDIO_DEFAULT_SAMPLE_RATE;
  fmt.ch = ch > 0 ? ch : SYS_AUDIO_DEFAULT_CHANNELS;
  fmt.bps = SYS_AUDIO_DEFAULT_BITS_PER_SAMPLE;
  fmt.bpf = fmt.ch * (fmt.bps / 8);
  G_SYS.audio.cb = cb;
  G_SYS.audio.ud = ud;
  G_SYS.audio.afmt = fmt;
  G_SYS.audio.stop = 0;
  hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
  if (SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE) {
    sys_log("Attempting WASAPI initialization...\n");
    if (sys_init_wasapi(&fmt)) {
      G_SYS.audio.wasapi = 1;
      G_SYS.audio.init = 1;
      sys_log("WASAPI initialization successful.\n");
      return 1;
    }
    sys_log("WASAPI initialization failed.\n");
  }
  sys_log("Attempting WaveOut initialization...\n");
  if (sys_init_waveout(&fmt)) {
    G_SYS.audio.wasapi = 0;
    G_SYS.audio.init = 1;
    sys_log("WaveOut initialization successful.\n");
    return 1;
  }
  sys_log("Audio initialization failed.\n");
  return 0;
}

void sys_shutdown_audio(void) {
  if (!G_SYS.audio.init) { return; }
  sys_log("Shutting down audio system...\n");
  G_SYS.audio.stop = 1;
  if (G_SYS.audio.wasapi) {
    sys_shutdown_wasapi();
  } else {
    sys_shutdown_waveout();
  }
  memset(&G_SYS.audio, 0, sizeof(mioSystemAudioState));
  CoUninitialize();
  sys_log("Audio shutdown complete.\n");
}

SYSRET sys_is_audio_playing(void) {
  return G_SYS.audio.init && !G_SYS.audio.stop;
}

mioSystemAudioFormat sys_get_audio_format(void) { return G_SYS.audio.afmt; }

void sys_beep(int32 freq, int32 dur) { Beep((DWORD)freq, (DWORD)dur); }

SYSRET sys_file_exists(const char *fp) {
  DWORD attr = GetFileAttributesA(fp);
  return (
      attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

uint8 *sys_load_file(const char *fp, int32 *sz) {
  HANDLE f;
  DWORD fsz;
  DWORD br;
  uint8 *buf;
  sys_log("Loading file: %s\n", fp);
  f = CreateFileA(
      fp, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL, NULL);
  if (f == INVALID_HANDLE_VALUE) {
    sys_log("Failed to open file: %s\n", fp);
    if (sz) { *sz = 0; }
    return NULL;
  }
  fsz = GetFileSize(f, NULL);
  if (fsz == INVALID_FILE_SIZE) {
    sys_log("Failed to get file size for: %s\n", fp);
    CloseHandle(f);
    if (sz) { *sz = 0; }
    return NULL;
  }
  buf = (uint8 *)sys_alloc((int32)fsz + 1);
  if (!buf) {
    sys_log("Memory allocation failed for file: %s\n", fp);
    CloseHandle(f);
    if (sz) { *sz = 0; }
    return NULL;
  }
  if (!ReadFile(f, buf, fsz, &br, NULL) || br != fsz) {
    sys_log("Failed to read file: %s\n", fp);
    sys_free(buf);
    CloseHandle(f);
    if (sz) { *sz = 0; }
    return NULL;
  }
  buf[fsz] = 0;
  CloseHandle(f);
  if (sz) { *sz = (int32)fsz; }
  sys_log("Successfully loaded file: %s, size: %d bytes\n", fp, *sz);
  return buf;
}

SYSRET sys_save_file(const char *fp, uint8 *data, int32 sz) {
  HANDLE f;
  DWORD bw;
  SYSRET ret = 1;
  sys_log("Saving file: %s, size: %d bytes\n", fp, sz);
  f = CreateFileA(
      fp, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (f == INVALID_HANDLE_VALUE) {
    sys_log("Failed to create file for saving: %s\n", fp);
    return 0;
  }
  if (!WriteFile(f, data, (DWORD)sz, &bw, NULL) || bw != (DWORD)sz) {
    sys_log("Failed to write to file: %s\n", fp);
    ret = 0;
  }
  CloseHandle(f);
  if (!ret) {
    if (DeleteFileA(fp)) {
      sys_log("Successfully deleted partially written file: %s\n", fp);
    } else {
      sys_log(
          "Failed to delete partially written file: %s. Error: %lu\n", fp,
          GetLastError());
    }
  }
  return ret;
}

void sys_free_file(uint8 *data) {
  if (data) {
    sys_free(data);
    sys_log("Freed file memory.\n");
  }
}

void *sys_alloc(int32 sz) {
  size_t total_size = (size_t)sz + sizeof(int32);
  void *ptr =
      VirtualAlloc(NULL, total_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (ptr) {
    *((int32 *)ptr) = sz;
    return (void *)((char *)ptr + sizeof(int32));
  }
  return NULL;
}

void sys_free(void *ptr) {
  if (ptr) {
    void *base_ptr = (void *)((char *)ptr - sizeof(int32));
    VirtualFree(base_ptr, 0, MEM_RELEASE);
  }
}

void *sys_realloc(void *ptr, int32 sz) {
  void *new_ptr;
  int32 old_sz;
  if (ptr == NULL) { return sys_alloc(sz); }
  if (sz == 0) {
    sys_free(ptr);
    return NULL;
  }
  old_sz = *((int32 *)((char *)ptr - sizeof(int32)));
  new_ptr = sys_alloc(sz);
  if (new_ptr) {
    int32 copy_sz = old_sz < sz ? old_sz : sz;
    memcpy(new_ptr, ptr, (size_t)copy_sz);
    sys_free(ptr);
  }
  return new_ptr;
}

#elif defined(__linux__)
#error "Linux support not implemented"
#else
#error "Unsupported platform"
#endif

/* @LIBRARY ******************************************************************/

/* @DRAWING ******************************************************************/

#include "math.h"

#define MIO_DRAW_PIXEL(x, y)                                                  \
  G_APP.render.colourData[(x) + (y) * G_APP.render.width] = G_APP.colour;

#define MIO_DRAW_DEPTH(x, y, d)                                               \
  G_APP.render.depthData[(x) + (y) * G_APP.render.width] = d;

#define MIO_LINE_CLIP_EPSILON 1e-10f

static const int32 G_BAYER_MATRIX_8[8][8] = {
    {0, 32, 8, 40, 2, 34, 10, 42},  {48, 16, 56, 24, 50, 18, 58, 26},
    {12, 44, 4, 36, 14, 46, 6, 38}, {60, 28, 52, 20, 62, 30, 54, 22},
    {3, 35, 11, 43, 1, 33, 9, 41},  {51, 19, 59, 27, 49, 17, 25, 57},
    {15, 47, 7, 39, 13, 45, 5, 37}, {63, 31, 55, 23, 61, 29, 53, 21}};

uint32 mio_set_colour(uint32 colour) {
  G_APP.colour = colour;
  return colour;
}

uint32 mio_alpha_blend_full(uint32 fg, uint32 bg) {
  real32 fgA, fgR, fgG, fgB;
  real32 bgA, bgR, bgG, bgB;
  real32 outA, outR, outG, outB;
  fgA = ((fg >> 24) & 0xFF) / 255.0f;
  fgR = ((fg >> 16) & 0xFF) / 255.0f;
  fgG = ((fg >> 8) & 0xFF) / 255.0f;
  fgB = ((fg >> 0) & 0xFF) / 255.0f;
  bgA = ((bg >> 24) & 0xFF) / 255.0f;
  bgR = ((bg >> 16) & 0xFF) / 255.0f;
  bgG = ((bg >> 8) & 0xFF) / 255.0f;
  bgB = ((bg >> 0) & 0xFF) / 255.0f;
  outA = fgA + bgA * (1.0f - fgA);
  if (outA < 0.000001f) { return 0x00000000; }
  outR = fgR * fgA + bgR * bgA * (1.0f - fgA);
  outG = fgG * fgA + bgG * bgA * (1.0f - fgA);
  outB = fgB * fgA + bgB * bgA * (1.0f - fgA);
  outA = outA * 255.0f;
  outR = outR * 255.0f;
  outG = outG * 255.0f;
  outB = outB * 255.0f;
  if (outA > 255.0f) { outA = 255.0f; }
  if (outA < 0.0f) { outA = 0.0f; }
  if (outR > 255.0f) { outR = 255.0f; }
  if (outR < 0.0f) { outR = 0.0f; }
  if (outG > 255.0f) { outG = 255.0f; }
  if (outG < 0.0f) { outG = 0.0f; }
  if (outB > 255.0f) { outB = 255.0f; }
  if (outB < 0.0f) { outB = 0.0f; }
  return ((uint32)outA << 24) | ((uint32)outR << 16) | ((uint32)outG << 8) |
         (uint32)outB;
}

uint32 mio_alpha_blend(uint32 fg, uint32 bg) {
  real32 fgA, fgR, fgG, fgB;
  real32 bgR, bgG, bgB;
  real32 outR, outG, outB;
  uint8 outA;
  fgA = ((fg >> 24) & 0xFF) / 255.0f;
  fgR = ((fg >> 16) & 0xFF) / 255.0f;
  fgG = ((fg >> 8) & 0xFF) / 255.0f;
  fgB = ((fg >> 0) & 0xFF) / 255.0f;
  bgR = ((bg >> 16) & 0xFF) / 255.0f;
  bgG = ((bg >> 8) & 0xFF) / 255.0f;
  bgB = ((bg >> 0) & 0xFF) / 255.0f;
  outR = fgR * fgA + bgR * (1.0f - fgA);
  outG = fgG * fgA + bgG * (1.0f - fgA);
  outB = fgB * fgA + bgB * (1.0f - fgA);
  outR = outR * 255.0f;
  outG = outG * 255.0f;
  outB = outB * 255.0f;
  if (outR > 255.0f) { outR = 255.0f; }
  if (outR < 0.0f) { outR = 0.0f; }
  if (outG > 255.0f) { outG = 255.0f; }
  if (outG < 0.0f) { outG = 0.0f; }
  if (outB > 255.0f) { outB = 255.0f; }
  if (outB < 0.0f) { outB = 0.0f; }
  outA = 255;
  return ((uint32)outA << 24) | ((uint32)outR << 16) | ((uint32)outG << 8) |
         (uint32)outB;
}

uint32 mio_alpha_blend_simd(uint32 fg, uint32 bg) {
  __m128 bgp, fgp;
  __m128 bgpp, fgpp;
  __m128 tempAlphas;
  __m128 invFg;
  __m128 fgResult, bgResult;
  __m128 result;
  uint8 outR, outG, outB, outA;
  real32 out[4];
  bgp = _mm_set_ps(
      (float)((bg >> 24) & 0xFF), (float)((bg >> 0) & 0xFF),
      (float)((bg >> 8) & 0xFF), (float)((bg >> 16) & 0xFF));
  fgp = _mm_set_ps(
      (float)((fg >> 24) & 0xFF), (float)((fg >> 0) & 0xFF),
      (float)((fg >> 8) & 0xFF), (float)((fg >> 16) & 0xFF));
  bgpp = _mm_div_ps(bgp, _mm_set1_ps(255.0f));
  fgpp = _mm_div_ps(fgp, _mm_set1_ps(255.0f));
  tempAlphas = _mm_shuffle_ps(fgpp, fgpp, _MM_SHUFFLE(3, 3, 3, 3));
  invFg = _mm_sub_ps(_mm_set1_ps(1.0f), tempAlphas);
  fgResult = _mm_mul_ps(tempAlphas, fgpp);
  bgResult = _mm_mul_ps(invFg, bgpp);
  result = _mm_add_ps(fgResult, bgResult);
  result = _mm_mul_ps(result, _mm_set1_ps(255.0f));
  result = _mm_max_ps(result, _mm_setzero_ps());
  result = _mm_min_ps(result, _mm_set1_ps(255.0f));
  _mm_storeu_ps(out, result);
  outR = (uint8)out[0];
  outG = (uint8)out[1];
  outB = (uint8)out[2];
  outA = 255;
  return (outA << 24) | (outR << 16) | (outG << 8) | outB;
}

uint32 mio_rgba_lerp(uint32 c1, uint32 c2, real32 t) {
  uint32 r1, g1, b1, a1;
  uint32 r2, g2, b2, a2;
  uint32 r, g, b, a;
  r1 = (c1 >> 16) & 0xFF;
  g1 = (c1 >> 8) & 0xFF;
  b1 = (c1 >> 0) & 0xFF;
  a1 = (c1 >> 24) & 0xFF;
  r2 = (c2 >> 16) & 0xFF;
  g2 = (c2 >> 8) & 0xFF;
  b2 = (c2 >> 0) & 0xFF;
  a2 = (c2 >> 24) & 0xFF;
  r = (uint32)((real32)r1 + t * ((real32)r2 - (real32)r1));
  g = (uint32)((real32)g1 + t * ((real32)g2 - (real32)g1));
  b = (uint32)((real32)b1 + t * ((real32)b2 - (real32)b1));
  a = (uint32)((real32)a1 + t * ((real32)a2 - (real32)a1));
  return (a << 24) | (r << 16) | (g << 8) | b;
}

int32 mio_blend_pixel(int32 x, int32 y) {
  uint32 index = x + y * G_APP.render.width;
  uint32 bg = G_APP.render.colourData[index];
  G_APP.render.colourData[index] = mio_alpha_blend(G_APP.colour, bg);
  return TRUE;
}

int32 mio_blend_pixel_full(int32 x, int32 y) {
  uint32 index = x + y * G_APP.render.width;
  uint32 bg = G_APP.render.colourData[index];
  G_APP.render.colourData[index] = mio_alpha_blend_full(G_APP.colour, bg);
  return TRUE;
}

int32 mio_blend_pixel_fast(int32 x, int32 y) {
  uint32 index = x + y * G_APP.render.width;
  uint32 bg = G_APP.render.colourData[index];
  G_APP.render.colourData[index] = mio_alpha_blend_simd(G_APP.colour, bg);
  return TRUE;
}

int32 mio_draw_clear(void) {
  int32 i;
  mioRenderContext *ctx = &G_APP.render;
  uint32 *p = ctx->colourData;
  if (G_SYS.fb && G_SYS.fb_w > 0 && G_SYS.fb_h > 0) {
    for (i = 0; i < G_SYS.fb_w * G_SYS.fb_h; i++) {
      *p = G_APP.colour;
      p++;
    }
  }
  return TRUE;
}

int32 mio_line_clip(
    mioRect clip, real32 *x0, real32 *y0, real32 *x1, real32 *y1) {
  int32 i;
  real32 t;
  real32 p[4], q[4];
  real32 xmin = (real32)clip.x1;
  real32 ymin = (real32)clip.y1;
  real32 xmax = (real32)clip.x2;
  real32 ymax = (real32)clip.y2;
  real32 dx = *x1 - *x0;
  real32 dy = *y1 - *y0;
  real32 u1 = 0.0f;
  real32 u2 = 1.0f;
  real32 origX0 = *x0;
  real32 origY0 = *y0;
  p[0] = -dx;
  q[0] = origX0 - xmin;
  p[1] = dx;
  q[1] = xmax - origX0;
  p[2] = -dy;
  q[2] = origY0 - ymin;
  p[3] = dy;
  q[3] = ymax - origY0;
  for (i = 0; i < 4; ++i) {
    if (MIO_FABS(p[i]) < MIO_LINE_CLIP_EPSILON) {
      if (q[i] < 0.0f) { return FALSE; }
    } else {
      t = q[i] / p[i];
      if (p[i] < 0.0f) {
        if (t > u2) { return FALSE; }
        if (t > u1) { u1 = t; }
      } else {
        if (t < u1) { return FALSE; }
        if (t < u2) { u2 = t; }
      }
    }
  }
  *x0 = origX0 + u1 * dx;
  *y0 = origY0 + u1 * dy;
  *x1 = origX0 + u2 * dx;
  *y1 = origY0 + u2 * dy;
  return TRUE;
}

int32 mio_draw_line(real32 x1, real32 y1, real32 x2, real32 y2) {
  int32 i, xi, yi;
  real32 dx, dy, steps, xInc, yInc, x, y;
  int32 width = G_APP.render.width;
  uint32 colour = G_APP.colour;
  uint32 *pixels = G_APP.render.colourData;
  mioRect clip = mio_rect(0, 0, G_APP.render.width, G_APP.render.height);
  if (!mio_line_clip(clip, &x1, &y1, &x2, &y2)) { return FALSE; }
  dx = x2 - x1;
  dy = y2 - y1;
  steps = MIO_FABS(dx) > MIO_FABS(dy) ? MIO_FABS(dx) : MIO_FABS(dy);
  if (steps < 1.0f) { steps = 1.0f; }
  xInc = dx / steps;
  yInc = dy / steps;
  x = x1;
  y = y1;
  if (MIO_COL_GET_A(colour) < 255) {
    for (i = 0; i <= (int32)steps; ++i) {
      xi = (int32)(x + 0.5f);
      yi = (int32)(y + 0.5f);
      mio_blend_pixel(xi, yi);
      x += xInc;
      y += yInc;
    }
  } else {
    for (i = 0; i <= (int32)steps; ++i) {
      xi = (int32)(x + 0.5f);
      yi = (int32)(y + 0.5f);
      pixels[yi * width + xi] = colour;
      x += xInc;
      y += yInc;
    }
  }

  return TRUE;
}

int32 mio_draw_quad_fill(
    real32 x0, real32 y0, real32 x1, real32 y1, real32 x2, real32 y2,
    real32 x3, real32 y3) {
  int32 y, x;
  int32 i, j, cnt;
  real32 yi, xi, yj, xj;
  real32 minY, maxY, xMin, xMax;
  real32 vx[4];
  real32 vy[4];
  uint32 *data = G_APP.render.colourData;
  int32 width = G_APP.render.width;
  uint32 col = G_APP.colour;
  real32 inters[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  vx[0] = x0;
  vx[1] = x1;
  vx[2] = x2;
  vx[3] = x3;

  vy[0] = y0;
  vy[1] = y1;
  vy[2] = y2;
  vy[3] = y3;

  minY = vy[0];
  maxY = vy[0];

  for (i = 1; i < 4; i++) {
    if (vy[i] < minY) { minY = vy[i]; }
    if (vy[i] > maxY) { maxY = vy[i]; }
  }
  for (y = (int32)(ceil(minY)); y <= (int32)(MIO_FLOOR(maxY)); y++) {
    inters[0] = 0.0f;
    inters[1] = 0.0f;
    inters[2] = 0.0f;
    inters[3] = 0.0f;
    cnt = 0;
    for (i = 0, j = 1; i < 4; j = (++i + 1) % 4) {
      yi = vy[i];
      yj = vy[j];
      if ((yi <= y && yj > y) || (yj <= y && yi > y)) {
        xi = vx[i];
        xj = vx[j];
				inters[cnt++] = xi + (y - yi) * (xj - xi) / (yj - yi);
      }
    }
    if (cnt < 2) { continue; }
    xMin = inters[0];
    xMax = inters[0];
    for (i = 1; i < cnt; i++) {
      if (inters[i] < xMin) { xMin = inters[i]; }
      if (inters[i] > xMax) { xMax = inters[i]; }
    }
    if (xMin < 0) { xMin = 0; }
    if (xMax >= width) { xMax = width - 1; }
    if (y >= 0 && y < (int32)G_APP.render.height) {
      if (MIO_COL_GET_A(col) < 255) {
        for (x = (int32)(ceil(xMin)); x <= (int32)(MIO_FLOOR(xMax)); x++) {
          if (x >= 0 && x < width) { mio_blend_pixel(x, y); }
        }
      } else {
        for (x = (int32)(ceil(xMin)); x <= (int32)(MIO_FLOOR(xMax)); x++) {
          if (x >= 0 && x < width) { data[x + y * width] = col; }
        }
      }
    }
  }

  return TRUE;
}

int32 mio_draw_quad(
    real32 x0, real32 y0, real32 x1, real32 y1, real32 x2, real32 y2,
    real32 x3, real32 y3) {
  mio_draw_line(x0, y0, x1, y1);
  mio_draw_line(x1, y1, x2, y2);
  mio_draw_line(x2, y2, x3, y3);
  mio_draw_line(x3, y3, x0, y0);
  return TRUE;
}

int32 mio_draw_circle(int32 xc, int32 yc, real32 r) {
  real32 radius;
  real32 dtheta;
  real32 cos_dt;
  real32 sin_dt;
  real32 cx, cy;
  real32 tx;
  real32 fx, fy;
  int32 i, n;
  int32 ix, iy;
  uint32 *data = G_APP.render.colourData;
  int32 width = G_APP.render.width;
  int32 height = G_APP.render.height;
  uint32 col = G_APP.colour;
  mioRect clip = G_APP.render.clip;
  if (r < 0.0f) { return FALSE; }
  if (r < 0.5f && xc >= 0 && xc < width && yc >= 0 && yc < height) {
    if (MIO_COL_GET_A(col) < 255) {
      mio_blend_pixel(xc, yc);
    } else {
      data[xc + yc * width] = col;
    }
    return TRUE;
  }
  if (xc + (int32)r <= clip.x1) { return FALSE; }
  if (xc - (int32)r >= clip.x2) { return FALSE; }
  if (yc + (int32)r <= clip.y1) { return FALSE; }
  if (yc - (int32)r >= clip.y2) { return FALSE; }
  radius = r;
  n = (int32)(ceil(2.0f * (real32)MIO_PI * radius));
  dtheta = (real32)(2.0f * MIO_PI / (real32)n);
  cos_dt = cos(dtheta);
  sin_dt = sin(dtheta);
  cx = 1.0f;
  cy = 0.0f;
  if (MIO_COL_GET_A(col) < 255) {
    if (cx - (int32)r < clip.x1 || cx + (int32)r >= clip.x2 ||
        cy - (int32)r < clip.y1 || cy + (int32)r >= clip.y2) {
      for (i = 0; i < n; i++) {
        fx = (real32)xc + cx * radius;
        fy = (real32)yc + cy * radius;
        ix = (int32)(fx + 0.5f);
        iy = (int32)(fy + 0.5f);
        if (ix >= clip.x1 && ix < clip.x2 && iy >= clip.y1 && iy < clip.y2) {
          mio_blend_pixel(ix, iy);
        }
        tx = cx * cos_dt - cy * sin_dt;
        cy = cx * sin_dt + cy * cos_dt;
        cx = tx;
      }
    } else {
      for (i = 0; i < n; i++) {
        fx = (real32)xc + cx * radius;
        fy = (real32)yc + cy * radius;
        ix = (int32)(fx + 0.5f);
        iy = (int32)(fy + 0.5f);
        mio_blend_pixel(ix, iy);
        tx = cx * cos_dt - cy * sin_dt;
        cy = cx * sin_dt + cy * cos_dt;
        cx = tx;
      }
    }
  } else {
    if (cx - (int32)r < clip.x1 || cx + (int32)r >= clip.x2 ||
        cy - (int32)r < clip.y1 || cy + (int32)r >= clip.y2) {
      for (i = 0; i < n; i++) {
        fx = (real32)xc + cx * radius;
        fy = (real32)yc + cy * radius;
        ix = (int32)(fx + 0.5f);
        iy = (int32)(fy + 0.5f);
        if (ix >= clip.x1 && ix < clip.x2 && iy >= clip.y1 && iy < clip.y2) {
          data[ix + iy * width] = col;
        }
        tx = cx * cos_dt - cy * sin_dt;
        cy = cx * sin_dt + cy * cos_dt;
        cx = tx;
      }
    } else {
      for (i = 0; i < n; i++) {
        fx = (real32)xc + cx * radius;
        fy = (real32)yc + cy * radius;
        ix = (int32)(fx + 0.5f);
        iy = (int32)(fy + 0.5f);
        data[ix + iy * width] = col;
        tx = cx * cos_dt - cy * sin_dt;
        cy = cx * sin_dt + cy * cos_dt;
        cx = tx;
      }
    }
  }

  return TRUE;
}

int32 mio_draw_circle_thick(int32 xc, int32 yc, real32 r, real32 thickness) {
  real32 outerRadius = r;
  real32 innerRadius = r - thickness;
  real32 outerR2, innerR2;
  real32 dy, dx;
  int32 y0, y1, x0, x1;
  int32 y, x;
  real32 distSq, maxDx;
  uint32 *data = G_APP.render.colourData;
  int32 width = G_APP.render.width;
  uint32 col = G_APP.colour;
  mioRect clip = G_APP.render.clip;
  if (thickness <= 0.0f || r < 0.0f) { return FALSE; }
  if (innerRadius < 0) { innerRadius = 0; }
  outerR2 = outerRadius * outerRadius;
  innerR2 = innerRadius * innerRadius;
  y0 = (int32)(ceil((real32)yc - outerRadius));
  y1 = (int32)(MIO_FLOOR((real32)yc + outerRadius));
  if (MIO_COL_GET_A(col) < 255) {
    for (y = y0; y <= y1; y++) {
      if (y < clip.y1 || y >= clip.y2) { continue; }
      dy = (real32)y - (real32)yc;
      maxDx = sqrt(outerR2 - dy * dy);
      x0 = (int32)(ceil((real32)xc - maxDx));
      x1 = (int32)(MIO_FLOOR((real32)xc + maxDx));
      for (x = x0; x <= x1; x++) {
        if (x < clip.x1 || x >= clip.x2) { continue; }
        dx = (real32)x - (real32)xc;
        distSq = dx * dx + dy * dy;
        if (distSq <= outerR2 && distSq >= innerR2) { mio_blend_pixel(x, y); }
      }
    }
  } else {
    for (y = y0; y <= y1; y++) {
      if (y < clip.y1 || y >= clip.y2) { continue; }
      dy = (real32)y - (real32)yc;
      maxDx = sqrt(outerR2 - dy * dy);
      x0 = (int32)(ceil((real32)xc - maxDx));
      x1 = (int32)(MIO_FLOOR((real32)xc + maxDx));
      for (x = x0; x <= x1; x++) {
        if (x < clip.x1 || x >= clip.x2) { continue; }
        dx = (real32)x - (real32)xc;
        distSq = dx * dx + dy * dy;
        if (distSq <= outerR2 && distSq >= innerR2) {
          data[x + y * width] = col;
        }
      }
    }
  }
  return TRUE;
}

int32 mio_draw_circle_fill(int32 xc, int32 yc, real32 r) {
  real32 radius;
  real32 r2;
  real32 dy, dx;
  int32 y0, y1;
  int32 x0, x1;
  int32 y, x;
  uint32 *data = G_APP.render.colourData;
  int32 width = G_APP.render.width;
  int32 height = G_APP.render.height;
  uint32 col = G_APP.colour;
  mioRect clip = G_APP.render.clip;
  if (r < 0.0f) { return FALSE; }
  if (r < 0.5f && xc >= 0 && xc < width && yc >= 0 && yc < height) {
    data[xc + yc * width] = col;
    return TRUE;
  }
  if (xc + (int32)r <= clip.x1) { return FALSE; }
  if (xc - (int32)r >= clip.x2) { return FALSE; }
  if (yc + (int32)r <= clip.y1) { return FALSE; }
  if (yc - (int32)r >= clip.y2) { return FALSE; }
  radius = r;
  r2 = radius * radius;
  y0 = (int32)(ceil((real32)yc - radius));
  y1 = (int32)(MIO_FLOOR((real32)yc + radius));
  if (MIO_COL_GET_A(col) < 255) {
    if (xc - (int32)r <= clip.x1 || xc + (int32)r >= clip.x2 ||
        yc - (int32)r <= clip.y1 || yc + (int32)r >= clip.y2) {
      for (y = y0; y <= y1; y++) {
        dy = (real32)y - (real32)yc;
        dx = sqrt(r2 - dy * dy);
        x0 = (int32)(ceil((real32)xc - dx));
        x1 = (int32)(MIO_FLOOR((real32)xc + dx));
        for (x = x0; x <= x1; x++) {
          if (x >= clip.x1 && x < clip.x2 && y >= clip.y1 && y < clip.y2) {
            mio_blend_pixel(x, y);
          }
        }
      }
    } else {
      for (y = y0; y <= y1; y++) {
        dy = (real32)y - (real32)yc;
        dx = sqrt(r2 - dy * dy);
        x0 = (int32)(ceil((real32)xc - dx));
        x1 = (int32)(MIO_FLOOR((real32)xc + dx));
        for (x = x0; x <= x1; x++) { mio_blend_pixel(x, y); }
      }
    }
  } else {
    if (xc - (int32)r <= clip.x1 || xc + (int32)r >= clip.x2 ||
        yc - (int32)r <= clip.y1 || yc + (int32)r >= clip.y2) {
      for (y = y0; y <= y1; y++) {
        dy = (real32)y - (real32)yc;
        dx = sqrt(r2 - dy * dy);
        x0 = (int32)(ceil((real32)xc - dx));
        x1 = (int32)(MIO_FLOOR((real32)xc + dx));
        for (x = x0; x <= x1; x++) {
          if (x >= clip.x1 && x < clip.x2 && y >= clip.y1 && y < clip.y2) {
            data[x + y * width] = col;
          }
        }
      }
    } else {
      for (y = y0; y <= y1; y++) {
        dy = (real32)y - (real32)yc;
        dx = sqrt(r2 - dy * dy);
        x0 = (int32)(ceil((real32)xc - dx));
        x1 = (int32)(MIO_FLOOR((real32)xc + dx));
        for (x = x0; x <= x1; x++) { data[x + y * width] = col; }
      }
    }
  }
  return TRUE;
}

int32 mio_draw_line_thick(
    real32 x1, real32 y1, real32 x2, real32 y2, real32 t, int32 capped) {
  real32 dx, dy, length, half, nx, ny, ox, oy;
  real32 fx0, fy0, fx1, fy1, fx2, fy2, fx3, fy3;
  real32 thickness = t;
  if (thickness < 2.0f) {
    mio_draw_line(x1, y1, x2, y2);
  } else {
    dx = x2 - x1;
    dy = y2 - y1;
    length = sqrt(dx * dx + dy * dy);
    half = thickness * 0.5f;
    nx = -dy / length;
    ny = dx / length;
    ox = nx * half;
    oy = ny * half;
    fx0 = x1 + ox;
    fy0 = y1 + oy;
    fx1 = x2 + ox;
    fy1 = y2 + oy;
    fx2 = x2 - ox;
    fy2 = y2 - oy;
    fx3 = x1 - ox;
    fy3 = y1 - oy;
    if (capped) {
      mio_draw_circle_fill((int32)(x1 + 0.5f), (int32)(y1 + 0.5f), half);
      mio_draw_circle_fill((int32)(x2 + 0.5f), (int32)(y2 + 0.5f), half);
    }
    mio_draw_quad_fill(fx0, fy0, fx1, fy1, fx2, fy2, fx3, fy3);
  }
  return TRUE;
}

int32 mio_draw_rect(int32 x1, int32 y1, int32 x2, int32 y2) {
  mio_draw_line(x1, y1, x2, y1);
  mio_draw_line(x1, y2, x2, y2);
  mio_draw_line(x1, y1, x1, y2);
  mio_draw_line(x2, y1, x2, y2);
  return TRUE;
}

int32 mio_draw_rect_thick(int32 x1, int32 y1, int32 x2, int32 y2, int32 t) {
  while (t-- > 0) { mio_draw_rect(x1 + t, y1 + t, x2 - t, y2 - t); }
  return TRUE;
}

int32 mio_draw_rect_fill(int32 x1, int32 y1, int32 x2, int32 y2) {
  int32 x, y;
  mioRect clipped;
  uint32 *data = G_APP.render.colourData;
  uint32 width = G_APP.render.width;
  uint32 col = G_APP.colour;
  mioRect clip = G_APP.render.clip;
  int32 maxX = MIO_MAX(x1, x2);
  int32 maxY = MIO_MAX(y1, y2);
  int32 minX = MIO_MIN(x1, x2);
  int32 minY = MIO_MIN(y1, y2);
  x1 = minX;
  x2 = maxX;
  y1 = minY;
  y2 = maxY;
  clipped = mio_rect_clip(clip, mio_rect(x1, y1, x2, y2));
  if (MIO_COL_GET_A(col) < 255) {
    for (y = clipped.y1; y < clipped.y2; y++) {
      for (x = clipped.x1; x < clipped.x2; x++) { mio_blend_pixel(x, y); }
    }
  } else {
    for (y = clipped.y1; y < clipped.y2; y++) {
      for (x = clipped.x1; x < clipped.x2; x++) { data[x + y * width] = col; }
    }
  }
  return TRUE;
}

int32 mio_draw_triangle(
    real32 x0, real32 y0, real32 x1, real32 y1, real32 x2, real32 y2) {
  mio_draw_line(x0, y0, x1, y1);
  mio_draw_line(x1, y1, x2, y2);
  mio_draw_line(x2, y2, x0, y0);
  return TRUE;
}

int32 mio_draw_triangle_fill(
    real32 x0, real32 y0, real32 x1, real32 y1, real32 x2, real32 y2) {
  real32 vx[3], vy[3];
  real32 minY, maxY;
  int32 i, j, y, x, cnt;
  real32 xi, yi, xj, yj;
  real32 xMin, xMax;
  uint32 *data = G_APP.render.colourData;
  int32 width = G_APP.render.width;
  uint32 col = G_APP.colour;
  real32 inters[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  vx[0] = x0;
  vy[0] = y0;
  vx[1] = x1;
  vy[1] = y1;
  vx[2] = x2;
  vy[2] = y2;
  minY = vy[0];
  maxY = vy[0];
  for (i = 1; i < 3; i++) {
    if (vy[i] < minY) { minY = vy[i]; }
    if (vy[i] > maxY) { maxY = vy[i]; }
  }
  for (y = (int32)(ceil(minY)); y <= (int32)(MIO_FLOOR(maxY)); y++) {
    cnt = 0;
    for (i = 0, j = 1; i < 3; j = (++i + 1) % 3) {
      yi = vy[i];
      yj = vy[j];
      if ((yi <= y && yj > y) || (yj <= y && yi > y)) {
        xi = vx[i];
        xj = vx[j];
        inters[cnt++] = xi + (y - yi) * (xj - xi) / (yj - yi);
			}
    }
    if (cnt < 2) { continue; }
    xMin = inters[0];
    xMax = inters[0];
    for (i = 1; i < cnt; i++) {
      if (inters[i] < xMin) { xMin = inters[i]; }
      if (inters[i] > xMax) { xMax = inters[i]; }
    }
    if (xMin < 0) { xMin = 0; }
    if (xMax >= width) { xMax = width - 1; }
    if (y >= 0 && y < (int32)G_APP.render.height) {
      if (MIO_COL_GET_A(col) < 255) {
        for (x = (int32)(ceil(xMin)); x <= (int32)(MIO_FLOOR(xMax)); x++) {
          if (x >= 0 && x < width) { mio_blend_pixel(x, y); }
        }
      } else {
        for (x = (int32)(ceil(xMin)); x <= (int32)(MIO_FLOOR(xMax)); x++) {
          if (x >= 0 && x < width) { data[x + y * width] = col; }
        }
      }
    }
  }
  return TRUE;
}

int32 mio_draw_ellipse(int32 xc, int32 yc, real32 rx, real32 ry) {
  int32 x, y;
  real32 dx, dy;
  real32 rx2 = rx * rx;
  real32 ry2 = ry * ry;
  int32 clip_x1 = (int32)G_APP.render.clip.x1;
  int32 clip_y1 = (int32)G_APP.render.clip.y1;
  int32 clip_x2 = (int32)G_APP.render.clip.x2;
  int32 clip_y2 = (int32)G_APP.render.clip.y2;
  uint32 *data = G_APP.render.colourData;
  int32 width = G_APP.render.width;
  uint32 col = G_APP.colour;
  if (rx < 0.0f || ry < 0.0f || !data) { return FALSE; }
  if (MIO_COL_GET_A(col) < 255) {
    for (y = (int32)(yc - ry); y <= (int32)(yc + ry); y++) {
      if (y < clip_y1 || y >= clip_y2) { continue; }
      dy = (real32)y - (real32)yc;
      dx = rx * sqrt(1.0f - (dy * dy / ry2));
      if (dx < 0) { dx = 0; }
      x = (int32)(xc + dx);
      if (x >= clip_x1 && x < clip_x2) { mio_blend_pixel(x, y); }
      x = (int32)(xc - dx);
      if (x >= clip_x1 && x < clip_x2) { mio_blend_pixel(x, y); }
    }
    for (x = (int32)(xc - rx); x <= (int32)(xc + rx); x++) {
      if (x < clip_x1 || x >= clip_x2) { continue; }
      dx = (real32)x - (real32)xc;
      dy = ry * sqrt(1.0f - (dx * dx / rx2));
      if (dy < 0) { dy = 0; }
      y = (int32)(yc + dy);
      if (y >= clip_y1 && y < clip_y2) { mio_blend_pixel(x, y); }
      y = (int32)(yc - dy);
      if (y >= clip_y1 && y < clip_y2) { mio_blend_pixel(x, y); }
    }
  } else {
    for (y = (int32)(yc - ry); y <= (int32)(yc + ry); y++) {
      if (y < clip_y1 || y >= clip_y2) { continue; }
      dy = (real32)y - (real32)yc;
      dx = rx * sqrt(1.0f - (dy * dy / ry2));
      if (dx < 0) { dx = 0; }
      x = (int32)(xc + dx);
      if (x >= clip_x1 && x < clip_x2) { data[x + y * width] = col; }
      x = (int32)(xc - dx);
      if (x >= clip_x1 && x < clip_x2) { data[x + y * width] = col; }
    }
    for (x = (int32)(xc - rx); x <= (int32)(xc + rx); x++) {
      if (x < clip_x1 || x >= clip_x2) { continue; }
      dx = (real32)x - (real32)xc;
      dy = ry * sqrt(1.0f - (dx * dx / rx2));
      if (dy < 0) { dy = 0; }
      y = (int32)(yc + dy);
      if (y >= clip_y1 && y < clip_y2) { data[x + y * width] = col; }
      y = (int32)(yc - dy);
      if (y >= clip_y1 && y < clip_y2) { data[x + y * width] = col; }
    }
  }
  return TRUE;
}

int32 mio_draw_rounded_rect(int32 x, int32 y, int32 w, int32 h, real32 r) {
  int32 minX, minY, maxX, maxY;
  real32 dx, dy;
  int32 i, j;
  real32 r2 = r * r;
  uint32 *data = G_APP.render.colourData;
  int32 width = G_APP.render.width;
  uint32 col = G_APP.colour;
  mioRect clip = G_APP.render.clip;
  if (w < 0 || h < 0 || !data) { return FALSE; }
  minX = MIO_MIN(x, x + w);
  maxX = MIO_MAX(x, x + w);
  minY = MIO_MIN(y, y + h);
  maxY = MIO_MAX(y, y + h);
  for (i = minY; i < maxY; i++) {
    for (j = minX; j < maxX; j++) {
      if (j < clip.x1 || j >= clip.x2 || i < clip.y1 || i >= clip.y2) {
        continue;
      }
      dx = 0, dy = 0;
      if (j < minX + r && i < minY + r) {
        dx = (real32)j - (minX + r);
        dy = (real32)i - (minY + r);
      } else if (j > maxX - r - 1 && i < minY + r) {
        dx = (real32)j - (maxX - r - 1);
        dy = (real32)i - (minY + r);
      } else if (j < minX + r && i > maxY - r - 1) {
        dx = (real32)j - (minX + r);
        dy = (real32)i - (maxY - r - 1);
      } else if (j > maxX - r - 1 && i > maxY - r - 1) {
        dx = (real32)j - (maxX - r - 1);
        dy = (real32)i - (maxY - r - 1);
      }
      if (MIO_COL_GET_A(col) < 255) {
        if (dx * dx + dy * dy <= r2) {
          mio_blend_pixel(j, i);
        } else if (j >= minX + r && j <= maxX - r - 1) {
          if (i == minY || i == maxY - 1) { mio_blend_pixel(j, i); }
        } else if (i >= minY + r && i <= maxY - r - 1) {
          if (j == minX || j == maxX - 1) { mio_blend_pixel(j, i); }
        }
      } else {
        if (dx * dx + dy * dy <= r2) {
          data[j + i * width] = col;
        } else if (j >= minX + r && j <= maxX - r - 1) {
          if (i == minY || i == maxY - 1) { data[j + i * width] = col; }
        } else if (i >= minY + r && i <= maxY - r - 1) {
          if (j == minX || j == maxX - 1) { data[j + i * width] = col; }
        }
      }
    }
  }
  return TRUE;
}

int32 mio_draw_polygon(
    int32 num_vertices, real32 *vertices_x, real32 *vertices_y) {
  int32 i;
  if (num_vertices < 2) { return FALSE; }
  for (i = 0; i < num_vertices - 1; i++) {
    mio_draw_line(
        vertices_x[i], vertices_y[i], vertices_x[i + 1], vertices_y[i + 1]);
  }
  mio_draw_line(
      vertices_x[num_vertices - 1], vertices_y[num_vertices - 1],
      vertices_x[0], vertices_y[0]);
  return TRUE;
}

int32 mio_draw_polygon_fill(
    int32 num_vertices, real32 *vertices_x, real32 *vertices_y) {
  real32 minY, maxY;
  int32 i, j, y, x, cnt, start_x, end_x;
  real32 xi, yi, xj, yj;
  uint32 *data = G_APP.render.colourData;
  int32 width = G_APP.render.width;
  uint32 col = G_APP.colour;
  real32 inters[256];
  if (num_vertices < 3 || num_vertices > 256 || !data) { return FALSE; }
  minY = vertices_y[0];
  maxY = vertices_y[0];
  for (i = 1; i < num_vertices; i++) {
    if (vertices_y[i] < minY) { minY = vertices_y[i]; }
    if (vertices_y[i] > maxY) { maxY = vertices_y[i]; }
  }
  for (y = (int32)(ceil(minY)); y <= (int32)(MIO_FLOOR(maxY)); y++) {
    cnt = 0;
    if (cnt >= 256) { break; }
    for (i = 0; i < num_vertices; i++) {
      j = (i + 1) % num_vertices;
      xi = vertices_x[i];
      yi = vertices_y[i];
      xj = vertices_x[j];
      yj = vertices_y[j];
      if ((yi <= y && yj > y) || (yj <= y && yi > y)) {
        if (MIO_FABS(yj - yi) > MIO_EPSILON_F) {
          inters[cnt++] = xi + (y - yi) * (xj - xi) / (yj - yi);
        } else {
          inters[cnt++] = xi;
          inters[cnt++] = xj;
        }
      }
    }
    for (i = 0; i < cnt - 1; i++) {
      for (j = 0; j < cnt - i - 1; j++) {
        if (inters[j] > inters[j + 1]) {
          real32 temp = inters[j];
          inters[j] = inters[j + 1];
          inters[j + 1] = temp;
        }
      }
    }
    for (i = 0; i < cnt; i += 2) {
      start_x = (int32)(ceil(inters[i]));
      end_x = (int32)(MIO_FLOOR(inters[i + 1]));
      if (y >= (int32)G_APP.render.clip.y1 &&
          y < (int32)G_APP.render.clip.y2) {
        if (MIO_COL_GET_A(col) < 255) {
          for (x = start_x; x <= end_x; x++) {
            if (x >= (int32)G_APP.render.clip.x1 &&
                x < (int32)G_APP.render.clip.x2) {
              mio_blend_pixel(x, y);
            }
          }
        } else {
          for (x = start_x; x <= end_x; x++) {
            if (x >= (int32)G_APP.render.clip.x1 &&
                x < (int32)G_APP.render.clip.x2) {
              data[x + y * width] = col;
            }
          }
        }
      }
    }
  }
  return TRUE;
}

int32 mio_draw_ellipse_fill(int32 xc, int32 yc, real32 rx, real32 ry) {
  int32 x, y;
  real32 rx2 = rx * rx;
  real32 ry2 = ry * ry;
  real32 dx, dy;
  uint32 *data = G_APP.render.colourData;
  int32 width = G_APP.render.width;
  uint32 col = G_APP.colour;
  mioRect clip = G_APP.render.clip;
  if (rx < 0.0f || ry < 0.0f) { return FALSE; }
  if (MIO_COL_GET_A(col) < 255) {
    for (y = (int32)(yc - ry); y <= (int32)(yc + ry); y++) {
      if (y < clip.y1 || y >= clip.y2) { continue; }
      for (x = (int32)(xc - rx); x <= (int32)(xc + rx); x++) {
        if (x < clip.x1 || x >= clip.x2) { continue; }
        dx = (real32)x - (real32)xc;
        dy = (real32)y - (real32)yc;
        if ((dx * dx / rx2) + (dy * dy / ry2) <= 1.0f) {
          mio_blend_pixel(x, y);
        }
      }
    }
  } else {
    for (y = (int32)(yc - ry); y <= (int32)(yc + ry); y++) {
      if (y < clip.y1 || y >= clip.y2) { continue; }
      for (x = (int32)(xc - rx); x <= (int32)(xc + rx); x++) {
        if (x < clip.x1 || x >= clip.x2) { continue; }
        dx = (real32)x - (real32)xc;
        dy = (real32)y - (real32)yc;
        if ((dx * dx / rx2) + (dy * dy / ry2) <= 1.0f) {
          data[x + y * width] = col;
        }
      }
    }
  }
  return TRUE;
}

void mio_draw_checkerboard(
    int32 x, int32 y, int32 width, int32 height, uint32 col1, uint32 col2,
    int32 cell_size) {
  uint32 *pixel;
  int32 cx, cy, cell_row, cell_col;
  int32 clip_x1 = (int32)G_APP.render.clip.x1;
  int32 clip_y1 = (int32)G_APP.render.clip.y1;
  int32 clip_x2 = (int32)G_APP.render.clip.x2;
  int32 clip_y2 = (int32)G_APP.render.clip.y2;
  int32 draw_x1 = MIO_MAX(x, clip_x1);
  int32 draw_y1 = MIO_MAX(y, clip_y1);
  int32 draw_x2 = MIO_MIN(x + width, clip_x2);
  int32 draw_y2 = MIO_MIN(y + height, clip_y2);
  if (G_APP.render.colourData == NULL) { return; }
  if (cell_size <= 0) { cell_size = 1; }
  if (draw_x1 >= draw_x2 || draw_y1 >= draw_y2) { return; }
  for (cy = draw_y1; cy < draw_y2; cy++) {
    for (cx = draw_x1; cx < draw_x2; cx++) {
      pixel = G_APP.render.colourData + (cy * G_SYS.fb_w) + cx;
      cell_row = (cy - y) / cell_size;
      cell_col = (cx - x) / cell_size;
      if ((cell_row + cell_col) % 2 == 0) {
        *pixel = col1;
      } else {
        *pixel = col2;
      }
    }
  }
}

int32 mio_draw_gradient(
    int32 x1, int32 y1, int32 x2, int32 y2, uint32 c1, uint32 c2,
    int32 vertical) {
  int32 x, y;
  real32 t;
  int32 minX, minY, maxX, maxY;
  mioRect clipped;
  uint32 *data;
  uint32 width;
  uint32 col;
  mioRect clip;
  data = G_APP.render.colourData;
  width = G_APP.render.width;
  clip = G_APP.render.clip;
  minX = MIO_MIN(x1, x2);
  maxX = MIO_MAX(x1, x2);
  minY = MIO_MIN(y1, y2);
  maxY = MIO_MAX(y1, y2);
  clipped = mio_rect_clip(clip, mio_rect(minX, minY, maxX, maxY));
  if (vertical) {
    for (y = (int32)clipped.y1; y < (int32)clipped.y2; y++) {
      t = ((real32)y - (real32)minY) / (real32)(maxY - minY);
      if (t < 0.0f) { t = 0.0f; }
      if (t > 1.0f) { t = 1.0f; }
      col = mio_rgba_lerp(c1, c2, t);
      for (x = (int32)clipped.x1; x < (int32)clipped.x2; x++) {
        data[x + y * width] = col;
      }
    }
  } else {
    for (x = (int32)clipped.x1; x < (int32)clipped.x2; x++) {
      t = ((real32)x - (real32)minX) / (real32)(maxX - minX);
      if (t < 0.0f) { t = 0.0f; }
      if (t > 1.0f) { t = 1.0f; }
      col = mio_rgba_lerp(c1, c2, t);
      for (y = (int32)clipped.y1; y < (int32)clipped.y2; y++) {
        data[x + y * width] = col;
      }
    }
  }
  return 1;
}

void mio_draw_quantize(int32 x1, int32 y1, int32 x2, int32 y2, int32 levels) {
  int32 x, y;
  uint32 col;
  uint8 r, g, b;
  for (y = y1; y < y2; y++) {
    for (x = x1; x < x2; x++) {
      col = G_APP.render.colourData[x + y * G_APP.render.width];
      r = (MIO_COL_GET_R(col) / levels) * levels;
      g = (MIO_COL_GET_G(col) / levels) * levels;
      b = (MIO_COL_GET_B(col) / levels) * levels;
      G_APP.render.colourData[x + y * G_APP.render.width] =
          MIO_RGBA(r, g, b, 255);
    }
  }
  return;
}

void mio_draw_dither(uint32 *pixels, int32 w, int32 h, int32 levels) {
  int32 x, y;
  real32 step;
  real32 threshNorm;
  uint32 col;
  real32 r, g, b;
  real32 lum, dithLum, newLum;
  real32 ratio, maxOrig, maxScaled;
  uint8 newR, newG, newB, lumVal;
  step = 255.0f / (real32)(levels - 1);
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      threshNorm = (real32)G_BAYER_MATRIX_8[y & 7][x & 7] / 64.0f;
      col = pixels[x + y * w];
      r = (real32)((col >> 16) & 0xFF);
      g = (real32)((col >> 8) & 0xFF);
      b = (real32)((col >> 0) & 0xFF);
      lum = r * 0.299f + g * 0.587f + b * 0.114f;
      dithLum = lum + (threshNorm - 0.5f) * step;
      newLum =
          MIO_CLAMP(MIO_FLOOR(dithLum / step + 0.5f) * step, 0.0f, 255.0f);
      if (lum > 1.0f) {
        ratio = newLum / lum;
        maxOrig = MIO_MAX(MIO_MAX(r, g), b);
        maxScaled = maxOrig * ratio;
        if (maxScaled > 255.0f) { ratio *= 255.0f / maxScaled; }
        newR = (uint8)MIO_CLAMP(r * ratio, 0.0f, 255.0f);
        newG = (uint8)MIO_CLAMP(g * ratio, 0.0f, 255.0f);
        newB = (uint8)MIO_CLAMP(b * ratio, 0.0f, 255.0f);
        pixels[x + y * w] = 0xFF000000 | (newR << 16) | (newG << 8) | newB;
      } else {
        lumVal = (uint8)newLum;
        pixels[x + y * w] =
            0xFF000000 | (lumVal << 16) | (lumVal << 8) | lumVal;
      }
    }
  }
}

void mio_draw_dither_perceptual(
    uint32 *pixels, int32 w, int32 h, int32 levels) {
  real32 step;
  int32 x, y;
  real32 rWeight, gWeight, bWeight;
  real32 baseThresh;
  uint32 col;
  real32 r, g, b;
  real32 rThresh, gThresh, bThresh;
  real32 dithR, dithG, dithB;
  uint8 newR, newG, newB;
  step = 255.0f / (real32)(levels - 1);
  rWeight = 1.0f;
  gWeight = 0.7f;
  bWeight = 1.2f;
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      baseThresh = (real32)G_BAYER_MATRIX_8[y & 7][x & 7] / 64.0f - 0.5f;
      col = pixels[x + y * w];
      r = (real32)((col >> 16) & 0xFF);
      g = (real32)((col >> 8) & 0xFF);
      b = (real32)((col >> 0) & 0xFF);
      rThresh = baseThresh * step * rWeight;
      gThresh = baseThresh * step * gWeight;
      bThresh = baseThresh * step * bWeight;
      dithR = r + rThresh;
      dithG = g + gThresh;
      dithB = b + bThresh;
      newR = (uint8)MIO_CLAMP(
          MIO_FLOOR(dithR / step + 0.5f) * step, 0.0f, 255.0f);
      newG = (uint8)MIO_CLAMP(
          MIO_FLOOR(dithG / step + 0.5f) * step, 0.0f, 255.0f);
      newB = (uint8)MIO_CLAMP(
          MIO_FLOOR(dithB / step + 0.5f) * step, 0.0f, 255.0f);
      pixels[x + y * w] = 0xFF000000 | (newR << 16) | (newG << 8) | newB;
    }
  }
}

/* @VECTOR MATH **************************************************************/

#define MIO_DEG2RAD (real32)(M_PI / 180.0)
#define MIO_RAD2DEG (real32)(180.0 / M_PI)
#define MIO_EPSILON 1e-6f

mioVec2 mio_vec2(real32 x, real32 y) {
  mioVec2 v;
  v.x = x;
  v.y = y;
  return v;
}

mioVec3 mio_vec3(real32 x, real32 y, real32 z) {
  mioVec3 v;
  v.x = x;
  v.y = y;
  v.z = z;
  return v;
}

mioVec4 mio_vec4(real32 x, real32 y, real32 z, real32 w) {
  mioVec4 v;
  v.x = x;
  v.y = y;
  v.z = z;
  v.w = w;
  return v;
}

mioQuat mio_quat(real32 x, real32 y, real32 z, real32 w) {
  mioQuat q;
  q.x = x;
  q.y = y;
  q.z = z;
  q.w = w;
  return q;
}

mioVec2 mio_vec2_add(mioVec2 a, mioVec2 b) {
  return mio_vec2(a.x + b.x, a.y + b.y);
}

mioVec2 mio_vec2_sub(mioVec2 a, mioVec2 b) {
  return mio_vec2(a.x - b.x, a.y - b.y);
}

mioVec2 mio_vec2_mul(mioVec2 v, real32 s) {
  return mio_vec2(v.x * s, v.y * s);
}

mioVec2 mio_vec2_div(mioVec2 v, real32 s) {
  return mio_vec2(v.x / s, v.y / s);
}

real32 mio_vec2_dot(mioVec2 a, mioVec2 b) { return a.x * b.x + a.y * b.y; }

real32 mio_vec2_length_sq(mioVec2 v) { return v.x * v.x + v.y * v.y; }

real32 mio_vec2_length(mioVec2 v) {
  return (real32)sqrt(mio_vec2_length_sq(v));
}

mioVec2 mio_vec2_normalize(mioVec2 v) {
  real32 len = mio_vec2_length(v);
  if (len > MIO_EPSILON) { return mio_vec2_div(v, len); }
  return mio_vec2(0.0f, 0.0f);
}

real32 mio_vec2_distance(mioVec2 a, mioVec2 b) {
  return mio_vec2_length(mio_vec2_sub(a, b));
}

mioVec2 mio_vec2_rotate(mioVec2 v, real32 angle) {
  real32 c = (real32)cos(angle);
  real32 s = (real32)sin(angle);
  return mio_vec2(v.x * c - v.y * s, v.x * s + v.y * c);
}

mioVec2 mio_vec2_perp(mioVec2 v) { return mio_vec2(-v.y, v.x); }

real32 mio_vec2_cross(mioVec2 a, mioVec2 b) { return a.x * b.y - a.y * b.x; }

mioVec3 mio_vec3_add(mioVec3 a, mioVec3 b) {
  return mio_vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

mioVec3 mio_vec3_sub(mioVec3 a, mioVec3 b) {
  return mio_vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

mioVec3 mio_vec3_scale(mioVec3 v, real32 s) {
  return mio_vec3(v.x * s, v.y * s, v.z * s);
}

mioVec3 mio_vec3_div(mioVec3 v, real32 s) {
  return mio_vec3(v.x / s, v.y / s, v.z / s);
}

real32 mio_vec3_dot(mioVec3 a, mioVec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

mioVec3 mio_vec3_cross(mioVec3 a, mioVec3 b) {
  return mio_vec3(
      a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

real32 mio_vec3_length_sq(mioVec3 v) {
  return v.x * v.x + v.y * v.y + v.z * v.z;
}

real32 mio_vec3_length(mioVec3 v) {
  return (real32)sqrt(mio_vec3_length_sq(v));
}

mioVec3 mio_vec3_normalize(mioVec3 v) {
  real32 len = mio_vec3_length(v);
  if (len > MIO_EPSILON) { return mio_vec3_div(v, len); }
  return mio_vec3(0.0f, 0.0f, 0.0f);
}

real32 mio_vec3_distance(mioVec3 a, mioVec3 b) {
  return mio_vec3_length(mio_vec3_sub(a, b));
}

mioVec3 mio_vec3_reflect(mioVec3 v, mioVec3 n) {
  real32 d = 2.0f * mio_vec3_dot(v, n);
  return mio_vec3_sub(v, mio_vec3_scale(n, d));
}

mioVec3 mio_vec3_project(mioVec3 v, mioVec3 onto) {
  real32 d = mio_vec3_dot(onto, onto);
  if (d > MIO_EPSILON) {
    real32 dp = mio_vec3_dot(v, onto);
    return mio_vec3_scale(onto, dp / d);
  }
  return mio_vec3(0.0f, 0.0f, 0.0f);
}

mioVec3 mio_vec3_reject(mioVec3 v, mioVec3 from) {
  return mio_vec3_sub(v, mio_vec3_project(v, from));
}

mioVec4 mio_vec4_add(mioVec4 a, mioVec4 b) {
  return mio_vec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

mioVec4 mio_vec4_sub(mioVec4 a, mioVec4 b) {
  return mio_vec4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

mioVec4 mio_vec4_mul(mioVec4 v, real32 s) {
  return mio_vec4(v.x * s, v.y * s, v.z * s, v.w * s);
}

mioVec4 mio_vec4_div(mioVec4 v, real32 s) {
  return mio_vec4(v.x / s, v.y / s, v.z / s, v.w / s);
}

real32 mio_vec4_dot(mioVec4 a, mioVec4 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

real32 mio_vec4_length_sq(mioVec4 v) {
  return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
}

real32 mio_vec4_length(mioVec4 v) {
  return (real32)sqrt(mio_vec4_length_sq(v));
}

mioVec4 mio_vec4_normalize(mioVec4 v) {
  real32 len = mio_vec4_length(v);
  if (len > MIO_EPSILON) { return mio_vec4_div(v, len); }
  return mio_vec4(0.0f, 0.0f, 0.0f, 0.0f);
}

mioMat2 mio_mat2_identity(void) {
  mioMat2 m;
  m.m[0] = 1.0f;
  m.m[2] = 0.0f;
  m.m[1] = 0.0f;
  m.m[3] = 1.0f;
  return m;
}

mioMat2 mio_mat2_mul(mioMat2 a, mioMat2 b) {
  mioMat2 result;
  result.m[0] = a.m[0] * b.m[0] + a.m[2] * b.m[1];
  result.m[1] = a.m[1] * b.m[0] + a.m[3] * b.m[1];
  result.m[2] = a.m[0] * b.m[2] + a.m[2] * b.m[3];
  result.m[3] = a.m[1] * b.m[2] + a.m[3] * b.m[3];
  return result;
}

mioVec2 mio_mat2_mul_vec2(mioMat2 m, mioVec2 v) {
  return mio_vec2(m.m[0] * v.x + m.m[2] * v.y, m.m[1] * v.x + m.m[3] * v.y);
}

real32 mio_mat2_determinant(mioMat2 m) {
  return m.m[0] * m.m[3] - m.m[2] * m.m[1];
}

mioMat2 mio_mat2_inverse(mioMat2 m) {
  mioMat2 result;
  real32 det = mio_mat2_determinant(m);
  real32 invDet;
  if (fabs(det) < MIO_EPSILON) { return mio_mat2_identity(); }
  invDet = 1.0f / det;
  result.m[0] = m.m[3] * invDet;
  result.m[1] = -m.m[1] * invDet;
  result.m[2] = -m.m[2] * invDet;
  result.m[3] = m.m[0] * invDet;
  return result;
}

mioMat2 mio_mat2_transpose(mioMat2 m) {
  mioMat2 result;
  result.m[0] = m.m[0];
  result.m[1] = m.m[2];
  result.m[2] = m.m[1];
  result.m[3] = m.m[3];
  return result;
}

mioMat3 mio_mat3_identity(void) {
  mioMat3 m;
  m.m[0] = 1.0f;
  m.m[3] = 0.0f;
  m.m[6] = 0.0f;
  m.m[1] = 0.0f;
  m.m[4] = 1.0f;
  m.m[7] = 0.0f;
  m.m[2] = 0.0f;
  m.m[5] = 0.0f;
  m.m[8] = 1.0f;
  return m;
}

mioMat3 mio_mat3_mul_scalar(mioMat3 m, real32 s) {
  m.m[0] *= s;
  m.m[4] *= s;
  m.m[8] *= s;
  return m;
}

mioMat3 mio_mat3_scale(mioVec3 v) {
  mioMat3 m = mio_mat3_identity();
  m.m[0] = v.x;
  m.m[4] = v.y;
  m.m[8] = v.z;
  return m;
}

mioMat3 mio_mat3_mul(mioMat3 a, mioMat3 b) {
  mioMat3 result;
  int i, j, k;
  for (i = 0; i < 3; i++) {
    for (j = 0; j < 3; j++) {
      result.m[i + j * 3] = 0.0f;
      for (k = 0; k < 3; k++) {
        result.m[i + j * 3] += a.m[i + k * 3] * b.m[k + j * 3];
      }
    }
  }
  return result;
}

mioVec3 mio_mat3_mul_vec3(mioMat3 m, mioVec3 v) {
  return mio_vec3(
      m.m[0] * v.x + m.m[3] * v.y + m.m[6] * v.z,
      m.m[1] * v.x + m.m[4] * v.y + m.m[7] * v.z,
      m.m[2] * v.x + m.m[5] * v.y + m.m[8] * v.z);
}

real32 mio_mat3_determinant(mioMat3 m) {
  return m.m[0] * (m.m[4] * m.m[8] - m.m[7] * m.m[5]) -
         m.m[3] * (m.m[1] * m.m[8] - m.m[7] * m.m[2]) +
         m.m[6] * (m.m[1] * m.m[5] - m.m[4] * m.m[2]);
}

mioMat3 mio_mat3_inverse(mioMat3 m) {
  mioMat3 result;
  real32 det = mio_mat3_determinant(m);
  real32 invDet;
  if (fabs(det) < MIO_EPSILON) { return mio_mat3_identity(); }
  invDet = 1.0f / det;
  result.m[0] = (m.m[4] * m.m[8] - m.m[7] * m.m[5]) * invDet;
  result.m[1] = (m.m[7] * m.m[2] - m.m[1] * m.m[8]) * invDet;
  result.m[2] = (m.m[1] * m.m[5] - m.m[4] * m.m[2]) * invDet;
  result.m[3] = (m.m[6] * m.m[5] - m.m[3] * m.m[8]) * invDet;
  result.m[4] = (m.m[0] * m.m[8] - m.m[6] * m.m[2]) * invDet;
  result.m[5] = (m.m[3] * m.m[2] - m.m[0] * m.m[5]) * invDet;
  result.m[6] = (m.m[3] * m.m[7] - m.m[6] * m.m[4]) * invDet;
  result.m[7] = (m.m[6] * m.m[1] - m.m[0] * m.m[7]) * invDet;
  result.m[8] = (m.m[0] * m.m[4] - m.m[3] * m.m[1]) * invDet;
  return result;
}

mioMat3 mio_mat3_transpose(mioMat3 m) {
  mioMat3 result;
  result.m[0] = m.m[0];
  result.m[3] = m.m[1];
  result.m[6] = m.m[2];
  result.m[1] = m.m[3];
  result.m[4] = m.m[4];
  result.m[7] = m.m[5];
  result.m[2] = m.m[6];
  result.m[5] = m.m[7];
  result.m[8] = m.m[8];
  return result;
}

mioMat4 mio_mat4_identity(void) {
  mioMat4 m;
  memset(m.m, 0, sizeof(m.m));
  m.m[0] = 1.0f;
  m.m[5] = 1.0f;
  m.m[10] = 1.0f;
  m.m[15] = 1.0f;
  return m;
}

mioMat4 mio_mat4_mul(mioMat4 a, mioMat4 b) {
  mioMat4 result;
  int i, j, k;
  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) {
      result.m[i + j * 4] = 0.0f;
      for (k = 0; k < 4; k++) {
        result.m[i + j * 4] += a.m[i + k * 4] * b.m[k + j * 4];
      }
    }
  }
  return result;
}

mioVec4 mio_mat4_mul_vec4(mioMat4 m, mioVec4 v) {
  return mio_vec4(
      m.m[0] * v.x + m.m[4] * v.y + m.m[8] * v.z + m.m[12] * v.w,
      m.m[1] * v.x + m.m[5] * v.y + m.m[9] * v.z + m.m[13] * v.w,
      m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z + m.m[14] * v.w,
      m.m[3] * v.x + m.m[7] * v.y + m.m[11] * v.z + m.m[15] * v.w);
}

mioVec3 mio_mat4_mul_point(mioMat4 m, mioVec3 v) {
  mioVec4 result = mio_mat4_mul_vec4(m, mio_vec4(v.x, v.y, v.z, 1.0f));
  if (fabs(result.w) > MIO_EPSILON) {
    return mio_vec3(
        result.x / result.w, result.y / result.w, result.z / result.w);
  }
  return mio_vec3(result.x, result.y, result.z);
}

mioVec3 mio_mat4_mul_direction(mioMat4 m, mioVec3 v) {
  mioVec4 result = mio_mat4_mul_vec4(m, mio_vec4(v.x, v.y, v.z, 0.0f));
  return mio_vec3(result.x, result.y, result.z);
}

mioMat4 mio_mat4_transpose(mioMat4 m) {
  mioMat4 result;
  int i, j;
  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) { result.m[i + j * 4] = m.m[j + i * 4]; }
  }
  return result;
}

real32 mio_mat4_determinant(mioMat4 m) {
  real32 det;
  real32 a = m.m[10] * m.m[15] - m.m[14] * m.m[11];
  real32 b = m.m[6] * m.m[15] - m.m[14] * m.m[7];
  real32 c = m.m[6] * m.m[11] - m.m[10] * m.m[7];
  real32 d = m.m[2] * m.m[15] - m.m[14] * m.m[3];
  real32 e = m.m[2] * m.m[11] - m.m[10] * m.m[3];
  real32 f = m.m[2] * m.m[7] - m.m[6] * m.m[3];

  det = m.m[0] * (m.m[5] * a - m.m[9] * b + m.m[13] * c) -
        m.m[4] * (m.m[1] * a - m.m[9] * d + m.m[13] * e) +
        m.m[8] * (m.m[1] * b - m.m[5] * d + m.m[13] * f) -
        m.m[12] * (m.m[1] * c - m.m[5] * e + m.m[9] * f);

  return det;
}

mioMat4 mio_mat4_inverse(mioMat4 m) {
  mioMat4 result;
  real32 det;
  real32 invDet;
  real32 a = m.m[10] * m.m[15] - m.m[14] * m.m[11];
  real32 b = m.m[6] * m.m[15] - m.m[14] * m.m[7];
  real32 c = m.m[6] * m.m[11] - m.m[10] * m.m[7];
  real32 d = m.m[2] * m.m[15] - m.m[14] * m.m[3];
  real32 e = m.m[2] * m.m[11] - m.m[10] * m.m[3];
  real32 f = m.m[2] * m.m[7] - m.m[6] * m.m[3];

  result.m[0] = m.m[5] * a - m.m[9] * b + m.m[13] * c;
  result.m[4] = -(m.m[4] * a - m.m[8] * b + m.m[12] * c);
  result.m[8] = m.m[4] * (m.m[9] * m.m[15] - m.m[13] * m.m[11]) -
                m.m[8] * (m.m[5] * m.m[15] - m.m[13] * m.m[7]) +
                m.m[12] * (m.m[5] * m.m[11] - m.m[9] * m.m[7]);
  result.m[12] =
      -(m.m[4] * (m.m[9] * m.m[14] - m.m[13] * m.m[10]) -
        m.m[8] * (m.m[5] * m.m[14] - m.m[13] * m.m[6]) +
        m.m[12] * (m.m[5] * m.m[10] - m.m[9] * m.m[6]));

  result.m[1] = -(m.m[1] * a - m.m[9] * d + m.m[13] * e);
  result.m[5] = m.m[0] * a - m.m[8] * d + m.m[12] * e;
  result.m[9] =
      -(m.m[0] * (m.m[9] * m.m[15] - m.m[13] * m.m[11]) -
        m.m[8] * (m.m[1] * m.m[15] - m.m[13] * m.m[3]) +
        m.m[12] * (m.m[1] * m.m[11] - m.m[9] * m.m[3]));
  result.m[13] = m.m[0] * (m.m[9] * m.m[14] - m.m[13] * m.m[10]) -
                 m.m[8] * (m.m[1] * m.m[14] - m.m[13] * m.m[2]) +
                 m.m[12] * (m.m[1] * m.m[10] - m.m[9] * m.m[2]);

  result.m[2] = m.m[1] * b - m.m[5] * d + m.m[13] * f;
  result.m[6] = -(m.m[0] * b - m.m[4] * d + m.m[12] * f);
  result.m[10] = m.m[0] * (m.m[5] * m.m[15] - m.m[13] * m.m[7]) -
                 m.m[4] * (m.m[1] * m.m[15] - m.m[13] * m.m[3]) +
                 m.m[12] * (m.m[1] * m.m[7] - m.m[5] * m.m[3]);
  result.m[14] =
      -(m.m[0] * (m.m[5] * m.m[14] - m.m[13] * m.m[6]) -
        m.m[4] * (m.m[1] * m.m[14] - m.m[13] * m.m[2]) +
        m.m[12] * (m.m[1] * m.m[6] - m.m[5] * m.m[2]));

  result.m[3] = -(m.m[1] * c - m.m[5] * e + m.m[9] * f);
  result.m[7] = m.m[0] * c - m.m[4] * e + m.m[8] * f;
  result.m[11] =
      -(m.m[0] * (m.m[5] * m.m[11] - m.m[9] * m.m[7]) -
        m.m[4] * (m.m[1] * m.m[11] - m.m[9] * m.m[3]) +
        m.m[8] * (m.m[1] * m.m[7] - m.m[5] * m.m[3]));
  result.m[15] = m.m[0] * (m.m[5] * m.m[10] - m.m[9] * m.m[6]) -
                 m.m[4] * (m.m[1] * m.m[10] - m.m[9] * m.m[2]) +
                 m.m[8] * (m.m[1] * m.m[6] - m.m[5] * m.m[2]);

  det = m.m[0] * result.m[0] + m.m[4] * result.m[1] + m.m[8] * result.m[2] +
        m.m[12] * result.m[3];

  if (fabs(det) < MIO_EPSILON) { return mio_mat4_identity(); }

  invDet = 1.0f / det;
  {
    int i;
    for (i = 0; i < 16; i++) { result.m[i] *= invDet; }
  }
  return result;
}

mioMat4 mio_mat4_translate(mioVec3 v) {
  mioMat4 m = mio_mat4_identity();
  m.m[12] = v.x;
  m.m[13] = v.y;
  m.m[14] = v.z;
  return m;
}

mioMat4 mio_mat4_scale(mioVec3 v) {
  mioMat4 m = mio_mat4_identity();
  m.m[0] = v.x;
  m.m[5] = v.y;
  m.m[10] = v.z;
  return m;
}

mioMat4 mio_mat4_rotate_x(real32 angle) {
  mioMat4 m = mio_mat4_identity();
  real32 c = (real32)cos(angle);
  real32 s = (real32)sin(angle);
  m.m[5] = c;
  m.m[6] = s;
  m.m[9] = -s;
  m.m[10] = c;
  return m;
}

mioMat4 mio_mat4_rotate_y(real32 angle) {
  mioMat4 m = mio_mat4_identity();
  real32 c = (real32)cos(angle);
  real32 s = (real32)sin(angle);
  m.m[0] = c;
  m.m[2] = -s;
  m.m[8] = s;
  m.m[10] = c;
  return m;
}

mioMat4 mio_mat4_rotate_z(real32 angle) {
  mioMat4 m = mio_mat4_identity();
  real32 c = (real32)cos(angle);
  real32 s = (real32)sin(angle);
  m.m[0] = c;
  m.m[1] = s;
  m.m[4] = -s;
  m.m[5] = c;
  return m;
}

mioMat4 mio_mat4_rotate(mioVec3 axis, real32 angle) {
  mioMat4 m;
  real32 c = (real32)cos(angle);
  real32 s = (real32)sin(angle);
  real32 t = 1.0f - c;
  mioVec3 a = mio_vec3_normalize(axis);

  m.m[0] = c + a.x * a.x * t;
  m.m[1] = a.y * a.x * t + a.z * s;
  m.m[2] = a.z * a.x * t - a.y * s;
  m.m[3] = 0.0f;

  m.m[4] = a.x * a.y * t - a.z * s;
  m.m[5] = c + a.y * a.y * t;
  m.m[6] = a.z * a.y * t + a.x * s;
  m.m[7] = 0.0f;

  m.m[8] = a.x * a.z * t + a.y * s;
  m.m[9] = a.y * a.z * t - a.x * s;
  m.m[10] = c + a.z * a.z * t;
  m.m[11] = 0.0f;

  m.m[12] = 0.0f;
  m.m[13] = 0.0f;
  m.m[14] = 0.0f;
  m.m[15] = 1.0f;

  return m;
}

mioMat4 mio_mat4_look_at(mioVec3 eye, mioVec3 center, mioVec3 up) {
  mioMat4 m;
  mioVec3 f = mio_vec3_normalize(mio_vec3_sub(center, eye));
  mioVec3 s = mio_vec3_normalize(mio_vec3_cross(f, up));
  mioVec3 u = mio_vec3_cross(s, f);

  m.m[0] = s.x;
  m.m[1] = u.x;
  m.m[2] = -f.x;
  m.m[3] = 0.0f;

  m.m[4] = s.y;
  m.m[5] = u.y;
  m.m[6] = -f.y;
  m.m[7] = 0.0f;

  m.m[8] = s.z;
  m.m[9] = u.z;
  m.m[10] = -f.z;
  m.m[11] = 0.0f;

  m.m[12] = -mio_vec3_dot(s, eye);
  m.m[13] = -mio_vec3_dot(u, eye);
  m.m[14] = mio_vec3_dot(f, eye);
  m.m[15] = 1.0f;

  return m;
}

mioMat4 mio_mat4_perspective(
    real32 fovy, real32 aspect, real32 nearPlane, real32 farPlane) {
  mioMat4 m;
  real32 tanHalfFovy = (real32)tan(fovy / 2.0f);

  memset(m.m, 0, sizeof(m.m));

  m.m[0] = 1.0f / (aspect * tanHalfFovy);
  m.m[5] = 1.0f / tanHalfFovy;
  m.m[10] = -(farPlane + nearPlane) / (farPlane - nearPlane);
  m.m[11] = -1.0f;
  m.m[14] = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);

  return m;
}

mioMat4 mio_mat4_ortho(
    real32 left, real32 right, real32 bottom, real32 top, real32 nearPlane,
    real32 farPlane) {
  mioMat4 m = mio_mat4_identity();

  m.m[0] = 2.0f / (right - left);
  m.m[5] = 2.0f / (top - bottom);
  m.m[10] = -2.0f / (farPlane - nearPlane);
  m.m[12] = -(right + left) / (right - left);
  m.m[13] = -(top + bottom) / (top - bottom);
  m.m[14] = -(farPlane + nearPlane) / (farPlane - nearPlane);

  return m;
}

mioMat4 mio_mat4_frustum(
    real32 left, real32 right, real32 bottom, real32 top, real32 nearPlane,
    real32 farPlane) {
  mioMat4 m;

  memset(m.m, 0, sizeof(m.m));

  m.m[0] = (2.0f * nearPlane) / (right - left);
  m.m[5] = (2.0f * nearPlane) / (top - bottom);
  m.m[8] = (right + left) / (right - left);
  m.m[9] = (top + bottom) / (top - bottom);
  m.m[10] = -(farPlane + nearPlane) / (farPlane - nearPlane);
  m.m[11] = -1.0f;
  m.m[14] = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);

  return m;
}

mioQuat mio_quat_identity(void) { return mio_quat(0.0f, 0.0f, 0.0f, 1.0f); }

mioQuat mio_quat_axis_angle(mioVec3 axis, real32 angle) {
  real32 halfAngle = angle * 0.5f;
  real32 s = (real32)sin(halfAngle);
  mioVec3 a = mio_vec3_normalize(axis);
  return mio_quat(a.x * s, a.y * s, a.z * s, (real32)cos(halfAngle));
}

mioQuat mio_quat_from_euler(real32 pitch, real32 yaw, real32 roll) {
  real32 cy = (real32)cos(yaw * 0.5f);
  real32 sy = (real32)sin(yaw * 0.5f);
  real32 cp = (real32)cos(pitch * 0.5f);
  real32 sp = (real32)sin(pitch * 0.5f);
  real32 cr = (real32)cos(roll * 0.5f);
  real32 sr = (real32)sin(roll * 0.5f);

  return mio_quat(
      sr * cp * cy - cr * sp * sy, cr * sp * cy + sr * cp * sy,
      cr * cp * sy - sr * sp * cy, cr * cp * cy + sr * sp * sy);
}

mioVec3 mio_quat_to_euler(mioQuat q) {
  mioVec3 result;
  real32 sinrCosp = 2.0f * (q.w * q.x + q.y * q.z);
  real32 cosrCosp = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
  result.x = (real32)atan2(sinrCosp, cosrCosp);

  {
    real32 sinp = 2.0f * (q.w * q.y - q.z * q.x);
    if (fabs(sinp) >= 1.0f) {
      result.y = (real32)copysign(MIO_PI / 2.0, sinp);
    } else {
      result.y = (real32)asin(sinp);
    }
  }
  {
    real32 sinyCosp = 2.0f * (q.w * q.z + q.x * q.y);
    real32 cosyCosp = 1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    result.z = (real32)atan2(sinyCosp, cosyCosp);
  }

  return result;
}

mioQuat mio_quat_mul(mioQuat a, mioQuat b) {
  return mio_quat(
      a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
      a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
      a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
      a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z);
}

real32 mio_quat_dot(mioQuat a, mioQuat b) {
  return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

real32 mio_quat_length_sq(mioQuat q) {
  return q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
}

real32 mio_quat_length(mioQuat q) {
  return (real32)sqrt(mio_quat_length_sq(q));
}

mioQuat mio_quat_normalize(mioQuat q) {
  real32 len = mio_quat_length(q);
  if (len > MIO_EPSILON) {
    real32 invLen = 1.0f / len;
    return mio_quat(q.x * invLen, q.y * invLen, q.z * invLen, q.w * invLen);
  }
  return mio_quat_identity();
}

mioQuat mio_quat_conjugate(mioQuat q) {
  return mio_quat(-q.x, -q.y, -q.z, q.w);
}

mioQuat mio_quat_inverse(mioQuat q) {
  real32 lenSq = mio_quat_length_sq(q);
  if (lenSq > MIO_EPSILON) {
    real32 invLenSq = 1.0f / lenSq;
    mioQuat conj = mio_quat_conjugate(q);
    return mio_quat(
        conj.x * invLenSq, conj.y * invLenSq, conj.z * invLenSq,
        conj.w * invLenSq);
  }
  return mio_quat_identity();
}

mioVec3 mio_quat_rotate_vec3(mioQuat q, mioVec3 v) {
  mioVec3 qv = mio_vec3(q.x, q.y, q.z);
  mioVec3 uv = mio_vec3_cross(qv, v);
  mioVec3 uuv = mio_vec3_cross(qv, uv);
  uv = mio_vec3_scale(uv, 2.0f * q.w);
  uuv = mio_vec3_scale(uuv, 2.0f);
  return mio_vec3_add(v, mio_vec3_add(uv, uuv));
}

mioQuat mio_quat_slerp(mioQuat a, mioQuat b, real32 t) {
  real32 cosHalfTheta = mio_quat_dot(a, b);
  mioQuat bAdjusted = b;

  if (cosHalfTheta < 0.0f) {
    bAdjusted = mio_quat(-b.x, -b.y, -b.z, -b.w);
    cosHalfTheta = -cosHalfTheta;
  }

  if (cosHalfTheta > 0.9995f) {
    return mio_quat(
        a.x + t * (bAdjusted.x - a.x), a.y + t * (bAdjusted.y - a.y),
        a.z + t * (bAdjusted.z - a.z), a.w + t * (bAdjusted.w - a.w));
  }

  {
    real32 halfTheta = (real32)acos(cosHalfTheta);
    real32 sinHalfTheta = (real32)sqrt(1.0f - cosHalfTheta * cosHalfTheta);
    real32 ratioA, ratioB;

    if (fabs(sinHalfTheta) < MIO_EPSILON) {
      return mio_quat(
          a.x * 0.5f + bAdjusted.x * 0.5f, a.y * 0.5f + bAdjusted.y * 0.5f,
          a.z * 0.5f + bAdjusted.z * 0.5f, a.w * 0.5f + bAdjusted.w * 0.5f);
    }

    ratioA = (real32)sin((1.0f - t) * halfTheta) / sinHalfTheta;
    ratioB = (real32)sin(t * halfTheta) / sinHalfTheta;

    return mio_quat(
        a.x * ratioA + bAdjusted.x * ratioB,
        a.y * ratioA + bAdjusted.y * ratioB,
        a.z * ratioA + bAdjusted.z * ratioB,
        a.w * ratioA + bAdjusted.w * ratioB);
  }
}

mioQuat mio_quat_nlerp(mioQuat a, mioQuat b, real32 t) {
  real32 cosHalfTheta = mio_quat_dot(a, b);
  mioQuat bAdjusted = b;

  if (cosHalfTheta < 0.0f) { bAdjusted = mio_quat(-b.x, -b.y, -b.z, -b.w); }

  return mio_quat_normalize(mio_quat(
      a.x + t * (bAdjusted.x - a.x), a.y + t * (bAdjusted.y - a.y),
      a.z + t * (bAdjusted.z - a.z), a.w + t * (bAdjusted.w - a.w)));
}

mioMat4 mio_quat_to_mat4(mioQuat q) {
  mioMat4 m;
  real32 xx = q.x * q.x;
  real32 yy = q.y * q.y;
  real32 zz = q.z * q.z;
  real32 xy = q.x * q.y;
  real32 xz = q.x * q.z;
  real32 yz = q.y * q.z;
  real32 wx = q.w * q.x;
  real32 wy = q.w * q.y;
  real32 wz = q.w * q.z;

  m.m[0] = 1.0f - 2.0f * (yy + zz);
  m.m[1] = 2.0f * (xy + wz);
  m.m[2] = 2.0f * (xz - wy);
  m.m[3] = 0.0f;

  m.m[4] = 2.0f * (xy - wz);
  m.m[5] = 1.0f - 2.0f * (xx + zz);
  m.m[6] = 2.0f * (yz + wx);
  m.m[7] = 0.0f;

  m.m[8] = 2.0f * (xz + wy);
  m.m[9] = 2.0f * (yz - wx);
  m.m[10] = 1.0f - 2.0f * (xx + yy);
  m.m[11] = 0.0f;

  m.m[12] = 0.0f;
  m.m[13] = 0.0f;
  m.m[14] = 0.0f;
  m.m[15] = 1.0f;

  return m;
}

mioQuat mio_mat4_to_quat(mioMat4 m) {
  mioQuat q;
  real32 trace = m.m[0] + m.m[5] + m.m[10];

  if (trace > 0.0f) {
    real32 s = (real32)sqrt(trace + 1.0f) * 2.0f;
    q.w = 0.25f * s;
    q.x = (m.m[6] - m.m[9]) / s;
    q.y = (m.m[8] - m.m[2]) / s;
    q.z = (m.m[1] - m.m[4]) / s;
  } else if (m.m[0] > m.m[5] && m.m[0] > m.m[10]) {
    real32 s = (real32)sqrt(1.0f + m.m[0] - m.m[5] - m.m[10]) * 2.0f;
    q.w = (m.m[6] - m.m[9]) / s;
    q.x = 0.25f * s;
    q.y = (m.m[4] + m.m[1]) / s;
    q.z = (m.m[8] + m.m[2]) / s;
  } else if (m.m[5] > m.m[10]) {
    real32 s = (real32)sqrt(1.0f + m.m[5] - m.m[0] - m.m[10]) * 2.0f;
    q.w = (m.m[8] - m.m[2]) / s;
    q.x = (m.m[4] + m.m[1]) / s;
    q.y = 0.25f * s;
    q.z = (m.m[9] + m.m[6]) / s;
  } else {
    real32 s = (real32)sqrt(1.0f + m.m[10] - m.m[0] - m.m[5]) * 2.0f;
    q.w = (m.m[1] - m.m[4]) / s;
    q.x = (m.m[8] + m.m[2]) / s;
    q.y = (m.m[9] + m.m[6]) / s;
    q.z = 0.25f * s;
  }

  return mio_quat_normalize(q);
}

mioQuat mio_quat_look_rotation(mioVec3 forward, mioVec3 up) {
  mioVec3 f = mio_vec3_normalize(forward);
  mioVec3 r = mio_vec3_normalize(mio_vec3_cross(up, f));
  mioVec3 u = mio_vec3_cross(f, r);
  mioMat4 m = mio_mat4_identity();

  m.m[0] = r.x;
  m.m[4] = r.y;
  m.m[8] = r.z;
  m.m[1] = u.x;
  m.m[5] = u.y;
  m.m[9] = u.z;
  m.m[2] = f.x;
  m.m[6] = f.y;
  m.m[10] = f.z;

  return mio_mat4_to_quat(m);
}

real32 mio_clamp(real32 value, real32 minVal, real32 maxVal) {
  if (value < minVal) { return minVal; }
  if (value > maxVal) { return maxVal; }
  return value;
}

real32 mio_min(real32 a, real32 b) { return a < b ? a : b; }

real32 mio_max(real32 a, real32 b) { return a > b ? a : b; }

real32 mio_sign(real32 value) {
  if (value > 0.0f) { return 1.0f; }
  if (value < 0.0f) { return -1.0f; }
  return 0.0f;
}

int mio_vec3_equal(mioVec3 a, mioVec3 b, real32 epsilon) {
  return fabs(a.x - b.x) < epsilon && fabs(a.y - b.y) < epsilon &&
         fabs(a.z - b.z) < epsilon;
}

mioVec3 mio_vec3_min(mioVec3 a, mioVec3 b) {
  return mio_vec3(mio_min(a.x, b.x), mio_min(a.y, b.y), mio_min(a.z, b.z));
}

mioVec3 mio_vec3_max(mioVec3 a, mioVec3 b) {
  return mio_vec3(mio_max(a.x, b.x), mio_max(a.y, b.y), mio_max(a.z, b.z));
}

mioVec3 mio_vec3_clamp(mioVec3 v, mioVec3 minVal, mioVec3 maxVal) {
  return mio_vec3(
      mio_clamp(v.x, minVal.x, maxVal.x), mio_clamp(v.y, minVal.y, maxVal.y),
      mio_clamp(v.z, minVal.z, maxVal.z));
}

mioMat3 mio_mat3_from_quat(mioQuat q) {
  mioMat3 m;
  real32 xx = q.x * q.x;
  real32 yy = q.y * q.y;
  real32 zz = q.z * q.z;
  real32 xy = q.x * q.y;
  real32 xz = q.x * q.z;
  real32 yz = q.y * q.z;
  real32 wx = q.w * q.x;
  real32 wy = q.w * q.y;
  real32 wz = q.w * q.z;

  m.m[0] = 1.0f - 2.0f * (yy + zz);
  m.m[1] = 2.0f * (xy + wz);
  m.m[2] = 2.0f * (xz - wy);

  m.m[3] = 2.0f * (xy - wz);
  m.m[4] = 1.0f - 2.0f * (xx + zz);
  m.m[5] = 2.0f * (yz + wx);

  m.m[6] = 2.0f * (xz + wy);
  m.m[7] = 2.0f * (yz - wx);
  m.m[8] = 1.0f - 2.0f * (xx + yy);

  return m;
}

mioMat4
mio_mat4_from_traf(mioVec3 translation, mioQuat rotation, mioVec3 scale) {
  mioMat4 t = mio_mat4_translate(translation);
  mioMat4 r = mio_quat_to_mat4(rotation);
  mioMat4 s = mio_mat4_scale(scale);
  return mio_mat4_mul(t, mio_mat4_mul(r, s));
}

real32 mio_angle_between_vectors(mioVec3 a, mioVec3 b) {
  real32 dot = mio_vec3_dot(mio_vec3_normalize(a), mio_vec3_normalize(b));
  return (real32)acos(mio_clamp(dot, -1.0f, 1.0f));
}

int32 mio_util_vec3_compare(const void *key1, const void *key2) {
  const mioVec3 *v1 = (const mioVec3 *)key1;
  const mioVec3 *v2 = (const mioVec3 *)key2;
  if (MIO_FABS(v1->x - v2->x) > MIO_EPSILON_F) { return 0; }
  if (MIO_FABS(v1->y - v2->y) > MIO_EPSILON_F) { return 0; }
  if (MIO_FABS(v1->z - v2->z) > MIO_EPSILON_F) { return 0; }
  return 1;
}

mioVec3
mio_vec3_barycentric(mioVec3 a, mioVec3 b, mioVec3 c, real32 u, real32 v) {
  real32 w = 1.0f - u - v;
  return mio_vec3(
      w * a.x + u * b.x + v * c.x, w * a.y + u * b.y + v * c.y,
      w * a.z + u * b.z + v * c.z);
}

mioMat4 mio_mat4_decompose_translation(mioMat4 m) {
  mioMat4 result = m;
  result.m[12] = 0.0f;
  result.m[13] = 0.0f;
  result.m[14] = 0.0f;
  return result;
}

mioVec3 mio_mat4_get_translation(mioMat4 m) {
  return mio_vec3(m.m[12], m.m[13], m.m[14]);
}

mioVec3 mio_mat4_get_scale(mioMat4 m) {
  mioVec3 col0 = mio_vec3(m.m[0], m.m[1], m.m[2]);
  mioVec3 col1 = mio_vec3(m.m[4], m.m[5], m.m[6]);
  mioVec3 col2 = mio_vec3(m.m[8], m.m[9], m.m[10]);
  return mio_vec3(
      mio_vec3_length(col0), mio_vec3_length(col1), mio_vec3_length(col2));
}

mioMat4 mio_mat4_remove_scale(mioMat4 m) {
  mioVec3 scale = mio_mat4_get_scale(m);
  mioMat4 result = m;

  if (fabs(scale.x) > MIO_EPSILON) {
    result.m[0] /= scale.x;
    result.m[1] /= scale.x;
    result.m[2] /= scale.x;
  }
  if (fabs(scale.y) > MIO_EPSILON) {
    result.m[4] /= scale.y;
    result.m[5] /= scale.y;
    result.m[6] /= scale.y;
  }
  if (fabs(scale.z) > MIO_EPSILON) {
    result.m[8] /= scale.z;
    result.m[9] /= scale.z;
    result.m[10] /= scale.z;
  }

  return result;
}

mioMat3 mio_mat4_to_mat3(mioMat4 m) {
  mioMat3 result;
  result.m[0] = m.m[0];
  result.m[3] = m.m[4];
  result.m[6] = m.m[8];
  result.m[1] = m.m[1];
  result.m[4] = m.m[5];
  result.m[7] = m.m[9];
  result.m[2] = m.m[2];
  result.m[5] = m.m[6];
  result.m[8] = m.m[10];
  return result;
}

mioMat4 mio_mat3_to_mat4(mioMat3 m) {
  mioMat4 result = mio_mat4_identity();
  result.m[0] = m.m[0];
  result.m[4] = m.m[3];
  result.m[8] = m.m[6];
  result.m[1] = m.m[1];
  result.m[5] = m.m[4];
  result.m[9] = m.m[7];
  result.m[2] = m.m[2];
  result.m[6] = m.m[5];
  result.m[10] = m.m[8];
  return result;
}

mioVec3 mio_mat4_get_right(mioMat4 m) {
  return mio_vec3(m.m[0], m.m[1], m.m[2]);
}

mioVec3 mio_mat4_get_up(mioMat4 m) { return mio_vec3(m.m[4], m.m[5], m.m[6]); }

mioVec3 mio_mat4_get_forward(mioMat4 m) {
  return mio_vec3(-m.m[8], -m.m[9], -m.m[10]);
}

mioMat4 mio_mat4_shadow(mioVec4 light, mioVec4 plane) {
  mioMat4 result;
  real32 dot = plane.x * light.x + plane.y * light.y + plane.z * light.z +
               plane.w * light.w;
  result.m[0] = dot - light.x * plane.x;
  result.m[1] = -light.y * plane.x;
  result.m[2] = -light.z * plane.x;
  result.m[3] = -light.w * plane.x;
  result.m[4] = -light.x * plane.y;
  result.m[5] = dot - light.y * plane.y;
  result.m[6] = -light.z * plane.y;
  result.m[7] = -light.w * plane.y;
  result.m[8] = -light.x * plane.z;
  result.m[9] = -light.y * plane.z;
  result.m[10] = dot - light.z * plane.z;
  result.m[11] = -light.w * plane.z;
  result.m[12] = -light.x * plane.w;
  result.m[13] = -light.y * plane.w;
  result.m[14] = -light.z * plane.w;
  result.m[15] = dot - light.w * plane.w;
  return result;
}

mioMat4 mio_mat4_reflection(mioVec4 plane) {
  mioMat4 result;

  result.m[0] = 1.0f - 2.0f * plane.x * plane.x;
  result.m[1] = -2.0f * plane.x * plane.y;
  result.m[2] = -2.0f * plane.x * plane.z;
  result.m[3] = 0.0f;

  result.m[4] = -2.0f * plane.y * plane.x;
  result.m[5] = 1.0f - 2.0f * plane.y * plane.y;
  result.m[6] = -2.0f * plane.y * plane.z;
  result.m[7] = 0.0f;

  result.m[8] = -2.0f * plane.z * plane.x;
  result.m[9] = -2.0f * plane.z * plane.y;
  result.m[10] = 1.0f - 2.0f * plane.z * plane.z;
  result.m[11] = 0.0f;

  result.m[12] = -2.0f * plane.w * plane.x;
  result.m[13] = -2.0f * plane.w * plane.y;
  result.m[14] = -2.0f * plane.w * plane.z;
  result.m[15] = 1.0f;

  return result;
}

mioVec3 mio_unproject(
    mioVec3 screenPos, mioMat4 view, mioMat4 projection, real32 viewportX,
    real32 viewportY, real32 viewportWidth, real32 viewportHeight) {
  mioMat4 viewProj = mio_mat4_mul(projection, view);
  mioMat4 invViewProj = mio_mat4_inverse(viewProj);
  mioVec4 normalized;
  mioVec4 result;

  normalized.x = (screenPos.x - viewportX) / viewportWidth * 2.0f - 1.0f;
  normalized.y = (screenPos.y - viewportY) / viewportHeight * 2.0f - 1.0f;
  normalized.z = screenPos.z * 2.0f - 1.0f;
  normalized.w = 1.0f;

  result = mio_mat4_mul_vec4(invViewProj, normalized);

  if (fabs(result.w) > MIO_EPSILON) {
    return mio_vec3(
        result.x / result.w, result.y / result.w, result.z / result.w);
  }

  return mio_vec3(result.x, result.y, result.z);
}

mioVec3 mio_project(
    mioVec3 worldPos, mioMat4 view, mioMat4 projection, real32 viewportX,
    real32 viewportY, real32 viewportWidth, real32 viewportHeight) {
  mioMat4 viewProj = mio_mat4_mul(projection, view);
  mioVec4 clipSpace = mio_mat4_mul_vec4(
      viewProj, mio_vec4(worldPos.x, worldPos.y, worldPos.z, 1.0f));
  mioVec3 result;

  if (fabs(clipSpace.w) > MIO_EPSILON) {
    clipSpace.x /= clipSpace.w;
    clipSpace.y /= clipSpace.w;
    clipSpace.z /= clipSpace.w;
  }

  result.x = (clipSpace.x * 0.5f + 0.5f) * viewportWidth + viewportX;
  result.y = (clipSpace.y * 0.5f + 0.5f) * viewportHeight + viewportY;
  result.z = clipSpace.z * 0.5f + 0.5f;

  return result;
}

real32 mio_vec3_triple_product(mioVec3 a, mioVec3 b, mioVec3 c) {
  return mio_vec3_dot(a, mio_vec3_cross(b, c));
}

mioVec3 mio_vec3_orthonormalize(mioVec3 normal, mioVec3 tangent) {
  tangent = mio_vec3_sub(tangent, mio_vec3_project(tangent, normal));
  return mio_vec3_normalize(tangent);
}

void mio_orthonormalize_basis(
    mioVec3 *normal, mioVec3 *tangent, mioVec3 *bitangent) {
  *normal = mio_vec3_normalize(*normal);
  *tangent = mio_vec3_orthonormalize(*normal, *tangent);
  *bitangent = mio_vec3_cross(*normal, *tangent);
}

mioVec2 mio_vec2_abs(mioVec2 v) {
  return mio_vec2((real32)fabs(v.x), (real32)fabs(v.y));
}

mioVec3 mio_vec3_abs(mioVec3 v) {
  return mio_vec3((real32)fabs(v.x), (real32)fabs(v.y), (real32)fabs(v.z));
}

mioVec4 mio_vec4_abs(mioVec4 v) {
  return mio_vec4(
      (real32)fabs(v.x), (real32)fabs(v.y), (real32)fabs(v.z),
      (real32)fabs(v.w));
}

mioVec2 mio_vec2_floor(mioVec2 v) {
  return mio_vec2((real32)floor(v.x), (real32)floor(v.y));
}

mioVec3 mio_vec3_floor(mioVec3 v) {
  return mio_vec3((real32)floor(v.x), (real32)floor(v.y), (real32)floor(v.z));
}

mioVec2 mio_vec2_ceil(mioVec2 v) {
  return mio_vec2((real32)ceil(v.x), (real32)ceil(v.y));
}

mioVec3 mio_vec3_ceil(mioVec3 v) {
  return mio_vec3((real32)ceil(v.x), (real32)ceil(v.y), (real32)ceil(v.z));
}

mioVec2 mio_vec2_round(mioVec2 v) {
  return mio_vec2((real32)floor(v.x + 0.5f), (real32)floor(v.y + 0.5f));
}

mioVec3 mio_vec3_round(mioVec3 v) {
  return mio_vec3(
      (real32)floor(v.x + 0.5f), (real32)floor(v.y + 0.5f),
      (real32)floor(v.z + 0.5f));
}

mioVec2 mio_vec2_fract(mioVec2 v) {
  return mio_vec2(v.x - (real32)floor(v.x), v.y - (real32)floor(v.y));
}

mioVec3 mio_vec3_fract(mioVec3 v) {
  return mio_vec3(
      v.x - (real32)floor(v.x), v.y - (real32)floor(v.y),
      v.z - (real32)floor(v.z));
}

mioVec2 mio_vec2_sign(mioVec2 v) {
  return mio_vec2(mio_sign(v.x), mio_sign(v.y));
}

mioVec3 mio_vec3_sign(mioVec3 v) {
  return mio_vec3(mio_sign(v.x), mio_sign(v.y), mio_sign(v.z));
}

mioVec2 mio_vec2_min_components(mioVec2 a, mioVec2 b) {
  return mio_vec2(mio_min(a.x, b.x), mio_min(a.y, b.y));
}

mioVec3 mio_vec3_min_components(mioVec3 a, mioVec3 b) {
  return mio_vec3(mio_min(a.x, b.x), mio_min(a.y, b.y), mio_min(a.z, b.z));
}

mioVec2 mio_vec2_max_components(mioVec2 a, mioVec2 b) {
  return mio_vec2(mio_max(a.x, b.x), mio_max(a.y, b.y));
}

mioVec3 mio_vec3_max_components(mioVec3 a, mioVec3 b) {
  return mio_vec3(mio_max(a.x, b.x), mio_max(a.y, b.y), mio_max(a.z, b.z));
}

real32 mio_vec2_min_component(mioVec2 v) { return mio_min(v.x, v.y); }

real32 mio_vec3_min_component(mioVec3 v) {
  return mio_min(mio_min(v.x, v.y), v.z);
}

real32 mio_vec2_max_component(mioVec2 v) { return mio_max(v.x, v.y); }

real32 mio_vec3_max_component(mioVec3 v) {
  return mio_max(mio_max(v.x, v.y), v.z);
}

mioVec2 mio_vec2_negate(mioVec2 v) { return mio_vec2(-v.x, -v.y); }

mioVec3 mio_vec3_negate(mioVec3 v) { return mio_vec3(-v.x, -v.y, -v.z); }

mioVec4 mio_vec4_negate(mioVec4 v) { return mio_vec4(-v.x, -v.y, -v.z, -v.w); }

int mio_vec2_equal(mioVec2 a, mioVec2 b, real32 epsilon) {
  return fabs(a.x - b.x) < epsilon && fabs(a.y - b.y) < epsilon;
}

int mio_vec4_equal(mioVec4 a, mioVec4 b, real32 epsilon) {
  return fabs(a.x - b.x) < epsilon && fabs(a.y - b.y) < epsilon &&
         fabs(a.z - b.z) < epsilon && fabs(a.w - b.w) < epsilon;
}

int mio_quat_equal(mioQuat a, mioQuat b, real32 epsilon) {
  return fabs(a.x - b.x) < epsilon && fabs(a.y - b.y) < epsilon &&
         fabs(a.z - b.z) < epsilon && fabs(a.w - b.w) < epsilon;
}

mioVec3 mio_vec3_pow(mioVec3 v, real32 exp) {
  return mio_vec3(
      (real32)pow(v.x, exp), (real32)pow(v.y, exp), (real32)pow(v.z, exp));
}

mioVec3 mio_vec3_exp(mioVec3 v) {
  return mio_vec3((real32)exp(v.x), (real32)exp(v.y), (real32)exp(v.z));
}

mioVec3 mio_vec3_sqrt(mioVec3 v) {
  return mio_vec3((real32)sqrt(v.x), (real32)sqrt(v.y), (real32)sqrt(v.z));
}

mioVec3 mio_vec3_reciprocal(mioVec3 v) {
  return mio_vec3(
      fabs(v.x) > MIO_EPSILON ? 1.0f / v.x : 0.0f,
      fabs(v.y) > MIO_EPSILON ? 1.0f / v.y : 0.0f,
      fabs(v.z) > MIO_EPSILON ? 1.0f / v.z : 0.0f);
}

mioQuat mio_quat_add(mioQuat a, mioQuat b) {
  return mio_quat(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

mioQuat mio_quat_scale(mioQuat q, real32 s) {
  return mio_quat(q.x * s, q.y * s, q.z * s, q.w * s);
}

mioQuat mio_quat_exp(mioQuat q) {
  real32 angle = (real32)sqrt(q.x * q.x + q.y * q.y + q.z * q.z);
  real32 sinAngle;

  if (angle < MIO_EPSILON) {
    return mio_quat(q.x, q.y, q.z, (real32)exp(q.w));
  }

  sinAngle = (real32)sin(angle);
  {
    real32 k = sinAngle / angle;
    real32 expW = (real32)exp(q.w);
    return mio_quat(
        q.x * k * expW, q.y * k * expW, q.z * k * expW,
        (real32)cos(angle) * expW);
  }
}

mioQuat mio_quat_len(mioQuat q) {
  real32 len = (real32)sqrt(q.x * q.x + q.y * q.y + q.z * q.z);
  real32 qlen = mio_quat_length(q);

  if (len < MIO_EPSILON) {
    return mio_quat(0.0f, 0.0f, 0.0f, (real32)log(qlen));
  }

  {
    real32 angle = (real32)atan2(len, q.w);
    real32 k = angle / len;
    return mio_quat(q.x * k, q.y * k, q.z * k, (real32)log(qlen));
  }
}

mioQuat mio_quat_pow(mioQuat q, real32 exp) {
  mioQuat ln = mio_quat_len(q);
  return mio_quat_exp(mio_quat_scale(ln, exp));
}

mioQuat
mio_quat_squad(mioQuat q0, mioQuat q1, mioQuat q2, mioQuat q3, real32 t) {
  mioQuat c0 = mio_quat_slerp(q0, q3, t);
  mioQuat c1 = mio_quat_slerp(q1, q2, t);
  return mio_quat_slerp(c0, c1, 2.0f * t * (1.0f - t));
}

mioQuat mio_quat_from_to_rotation(mioVec3 from, mioVec3 to) {
  mioVec3 f = mio_vec3_normalize(from);
  mioVec3 t = mio_vec3_normalize(to);
  real32 dot = mio_vec3_dot(f, t);

  if (dot > 0.999999f) { return mio_quat_identity(); }

  if (dot < -0.999999f) {
    mioVec3 axis = mio_vec3_cross(mio_vec3(1.0f, 0.0f, 0.0f), f);
    if (mio_vec3_length_sq(axis) < MIO_EPSILON) {
      axis = mio_vec3_cross(mio_vec3(0.0f, 1.0f, 0.0f), f);
    }
    axis = mio_vec3_normalize(axis);
    return mio_quat_axis_angle(axis, (real32)MIO_PI);
  }

  {
    mioVec3 axis = mio_vec3_cross(f, t);
    real32 s = (real32)sqrt((1.0f + dot) * 2.0f);
    real32 invS = 1.0f / s;
    return mio_quat(axis.x * invS, axis.y * invS, axis.z * invS, s * 0.5f);
  }
}

mioVec3 mio_quat_get_axis(mioQuat q) {
  real32 s = (real32)sqrt(1.0f - q.w * q.w);
  if (s < MIO_EPSILON) { return mio_vec3(1.0f, 0.0f, 0.0f); }
  return mio_vec3(q.x / s, q.y / s, q.z / s);
}

real32 mio_quat_get_angle(mioQuat q) {
  return 2.0f * (real32)acos(mio_clamp(q.w, -1.0f, 1.0f));
}

real32 mio_quat_angle_between(mioQuat a, mioQuat b) {
  real32 dot = mio_quat_dot(a, b);
  return (real32)acos(mio_clamp(fabs(dot), 0.0f, 1.0f)) * 2.0f;
}

mioMat4 mio_mat4_skew(real32 angleX, real32 angleY) {
  mioMat4 m = mio_mat4_identity();
  m.m[4] = (real32)tan(angleX);
  m.m[8] = (real32)tan(angleY);
  return m;
}

mioMat4
mio_mat4_from_rotation_translation(mioQuat rotation, mioVec3 translation) {
  mioMat4 m = mio_quat_to_mat4(rotation);
  m.m[12] = translation.x;
  m.m[13] = translation.y;
  m.m[14] = translation.z;
  return m;
}

void mio_mat4_decompose(
    mioMat4 m, mioVec3 *translation, mioQuat *rotation, mioVec3 *scale) {
  mioMat4 rotMat;
  *translation = mio_mat4_get_translation(m);
  *scale = mio_mat4_get_scale(m);
  rotMat = mio_mat4_remove_scale(m);
  *rotation = mio_mat4_to_quat(rotMat);
}

int mio_mat4_is_identity(mioMat4 m, real32 epsilon) {
  return fabs(m.m[0] - 1.0f) < epsilon && fabs(m.m[1]) < epsilon &&
         fabs(m.m[2]) < epsilon && fabs(m.m[3]) < epsilon &&
         fabs(m.m[4]) < epsilon && fabs(m.m[5] - 1.0f) < epsilon &&
         fabs(m.m[6]) < epsilon && fabs(m.m[7]) < epsilon &&
         fabs(m.m[8]) < epsilon && fabs(m.m[9]) < epsilon &&
         fabs(m.m[10] - 1.0f) < epsilon && fabs(m.m[11]) < epsilon &&
         fabs(m.m[12]) < epsilon && fabs(m.m[13]) < epsilon &&
         fabs(m.m[14]) < epsilon && fabs(m.m[15] - 1.0f) < epsilon;
}

real32 mio_lerp(real32 a, real32 b, real32 t) { return a + (b - a) * t; }

real32 mio_lerp_clamped(real32 a, real32 b, real32 t) {
  if (t < 0.0f) { t = 0.0f; }
  if (t > 1.0f) { t = 1.0f; }
  return a + (b - a) * t;
}

real32 mio_lerp_precise(real32 a, real32 b, real32 t) {
  return (1.0f - t) * a + t * b;
}

mioVec2 mio_vec2_lerp(mioVec2 a, mioVec2 b, real32 t) {
  return mio_vec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
}

mioVec2 mio_vec2_lerp_clamped(mioVec2 a, mioVec2 b, real32 t) {
  if (t < 0.0f) { t = 0.0f; }
  if (t > 1.0f) { t = 1.0f; }
  return mio_vec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
}

mioVec2 mio_vec2_lerp_precise(mioVec2 a, mioVec2 b, real32 t) {
  return mio_vec2((1.0f - t) * a.x + t * b.x, (1.0f - t) * a.y + t * b.y);
}

mioVec3 mio_vec3_lerp(mioVec3 a, mioVec3 b, real32 t) {
  return mio_vec3(
      a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
}

mioVec3 mio_vec3_lerp_clamped(mioVec3 a, mioVec3 b, real32 t) {
  if (t < 0.0f) { t = 0.0f; }
  if (t > 1.0f) { t = 1.0f; }
  return mio_vec3(
      a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
}

mioVec3 mio_vec3_lerp_precise(mioVec3 a, mioVec3 b, real32 t) {
  return mio_vec3(
      (1.0f - t) * a.x + t * b.x, (1.0f - t) * a.y + t * b.y,
      (1.0f - t) * a.z + t * b.z);
}

mioVec4 mio_vec4_lerp(mioVec4 a, mioVec4 b, real32 t) {
  return mio_vec4(
      a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t,
      a.w + (b.w - a.w) * t);
}

mioVec4 mio_vec4_lerp_clamped(mioVec4 a, mioVec4 b, real32 t) {
  if (t < 0.0f) { t = 0.0f; }
  if (t > 1.0f) { t = 1.0f; }
  return mio_vec4(
      a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t,
      a.w + (b.w - a.w) * t);
}

mioVec4 mio_vec4_lerp_precise(mioVec4 a, mioVec4 b, real32 t) {
  return mio_vec4(
      (1.0f - t) * a.x + t * b.x, (1.0f - t) * a.y + t * b.y,
      (1.0f - t) * a.z + t * b.z, (1.0f - t) * a.w + t * b.w);
}

mioQuat mio_quat_lerp(mioQuat a, mioQuat b, real32 t) {
  return mio_quat(
      a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t,
      a.w + (b.w - a.w) * t);
}

mioQuat mio_quat_lerp_clamped(mioQuat a, mioQuat b, real32 t) {
  if (t < 0.0f) { t = 0.0f; }
  if (t > 1.0f) { t = 1.0f; }
  return mio_quat(
      a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t,
      a.w + (b.w - a.w) * t);
}

mioQuat mio_quat_lerp_precise(mioQuat a, mioQuat b, real32 t) {
  return mio_quat(
      (1.0f - t) * a.x + t * b.x, (1.0f - t) * a.y + t * b.y,
      (1.0f - t) * a.z + t * b.z, (1.0f - t) * a.w + t * b.w);
}

mioMat2 mio_mat2_lerp(mioMat2 a, mioMat2 b, real32 t) {
  mioMat2 result;
  int i;
  for (i = 0; i < 4; i++) { result.m[i] = a.m[i] + (b.m[i] - a.m[i]) * t; }
  return result;
}

mioMat2 mio_mat2_lerp_clamped(mioMat2 a, mioMat2 b, real32 t) {
  mioMat2 result;
  int i;
  if (t < 0.0f) { t = 0.0f; }
  if (t > 1.0f) { t = 1.0f; }
  for (i = 0; i < 4; i++) { result.m[i] = a.m[i] + (b.m[i] - a.m[i]) * t; }
  return result;
}

mioMat2 mio_mat2_lerp_precise(mioMat2 a, mioMat2 b, real32 t) {
  mioMat2 result;
  int i;
  for (i = 0; i < 4; i++) { result.m[i] = (1.0f - t) * a.m[i] + t * b.m[i]; }
  return result;
}

mioMat3 mio_mat3_lerp(mioMat3 a, mioMat3 b, real32 t) {
  mioMat3 result;
  int i;
  for (i = 0; i < 9; i++) { result.m[i] = a.m[i] + (b.m[i] - a.m[i]) * t; }
  return result;
}

mioMat3 mio_mat3_lerp_clamped(mioMat3 a, mioMat3 b, real32 t) {
  mioMat3 result;
  int i;
  if (t < 0.0f) { t = 0.0f; }
  if (t > 1.0f) { t = 1.0f; }
  for (i = 0; i < 9; i++) { result.m[i] = a.m[i] + (b.m[i] - a.m[i]) * t; }
  return result;
}

mioMat3 mio_mat3_lerp_precise(mioMat3 a, mioMat3 b, real32 t) {
  mioMat3 result;
  int i;
  for (i = 0; i < 9; i++) { result.m[i] = (1.0f - t) * a.m[i] + t * b.m[i]; }
  return result;
}

mioMat4 mio_mat4_lerp(mioMat4 a, mioMat4 b, real32 t) {
  mioMat4 result;
  int i;
  for (i = 0; i < 16; i++) { result.m[i] = a.m[i] + (b.m[i] - a.m[i]) * t; }
  return result;
}

mioMat4 mio_mat4_lerp_clamped(mioMat4 a, mioMat4 b, real32 t) {
  mioMat4 result;
  int i;
  if (t < 0.0f) { t = 0.0f; }
  if (t > 1.0f) { t = 1.0f; }
  for (i = 0; i < 16; i++) { result.m[i] = a.m[i] + (b.m[i] - a.m[i]) * t; }
  return result;
}

mioMat4 mio_mat4_lerp_precise(mioMat4 a, mioMat4 b, real32 t) {
  mioMat4 result;
  int i;
  for (i = 0; i < 16; i++) { result.m[i] = (1.0f - t) * a.m[i] + t * b.m[i]; }
  return result;
}

mioVec3 mio_vec3_slerp(mioVec3 a, mioVec3 b, real32 t) {
  real32 dot = mio_vec3_dot(mio_vec3_normalize(a), mio_vec3_normalize(b));
  dot = mio_clamp(dot, -1.0f, 1.0f);

  {
    real32 theta = (real32)acos(dot) * t;
    mioVec3 relative =
        mio_vec3_normalize(mio_vec3_sub(b, mio_vec3_scale(a, dot)));
    return mio_vec3_add(
        mio_vec3_scale(a, (real32)cos(theta)),
        mio_vec3_scale(relative, (real32)sin(theta)));
  }
}

real32 mio_smoothstep(real32 edge0, real32 edge1, real32 x) {
  real32 t = mio_clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}

real32 mio_smootherstep(real32 edge0, real32 edge1, real32 x) {
  real32 t = mio_clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
  return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

real32 mio_inverse_lerp(real32 a, real32 b, real32 value) {
  if (fabs(b - a) < MIO_EPSILON) { return 0.0f; }
  return (value - a) / (b - a);
}

real32 mio_remap(
    real32 value, real32 fromMin, real32 fromMax, real32 toMin, real32 toMax) {
  real32 t = mio_inverse_lerp(fromMin, fromMax, value);
  return mio_lerp(toMin, toMax, t);
}

real32 mio_move_towards(real32 current, real32 target, real32 maxDelta) {
  real32 delta = target - current;
  if (fabs(delta) <= maxDelta) { return target; }
  return current + mio_sign(delta) * maxDelta;
}

real32 mio_repeat(real32 t, real32 length) {
  return mio_clamp(t - (real32)floor(t / length) * length, 0.0f, length);
}

real32 mio_ping_pong(real32 t, real32 length) {
  t = mio_repeat(t, length * 2.0f);
  return length - fabs(t - length);
}

real32 mio_delta_angle(real32 current, real32 target) {
  real32 delta = mio_repeat(target - current, 360.0f);
  if (delta > 180.0f) { delta -= 360.0f; }
  return delta;
}

real32 mio_lerp_angle(real32 a, real32 b, real32 t) {
  real32 delta = mio_repeat(b - a, 360.0f);
  if (delta > 180.0f) { delta -= 360.0f; }
  return a + delta * mio_clamp(t, 0.0f, 1.0f);
}

real32 mio_move_towards_angle(real32 current, real32 target, real32 maxDelta) {
  real32 deltaAngle = mio_delta_angle(current, target);
  if (-maxDelta < deltaAngle && deltaAngle < maxDelta) { return target; }
  target = current + deltaAngle;
  return mio_move_towards(current, target, maxDelta);
}

mioVec2 mio_vec2_lerp_unclamped(mioVec2 a, mioVec2 b, real32 t) {
  return mio_vec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
}

mioVec2 mio_vec2_inverse_lerp(mioVec2 a, mioVec2 b, mioVec2 value) {
  return mio_vec2(
      mio_inverse_lerp(a.x, b.x, value.x),
      mio_inverse_lerp(a.y, b.y, value.y));
}

mioVec2 mio_vec2_move_towards(
    mioVec2 current, mioVec2 target, real32 maxDistanceDelta) {
  mioVec2 delta = mio_vec2_sub(target, current);
  real32 distSq = mio_vec2_length_sq(delta);
  if (distSq == 0.0f || (maxDistanceDelta >= 0.0f &&
                         distSq <= maxDistanceDelta * maxDistanceDelta)) {
    return target;
  }
  {
    real32 dist = (real32)sqrt(distSq);
    return mio_vec2_add(current, mio_vec2_mul(delta, maxDistanceDelta / dist));
  }
}

mioVec2 mio_vec2_reflect(mioVec2 v, mioVec2 normal) {
  real32 dot = mio_vec2_dot(v, normal);
  return mio_vec2_sub(v, mio_vec2_mul(normal, 2.0f * dot));
}

mioVec2 mio_vec2_project(mioVec2 v, mioVec2 onto) {
  real32 d = mio_vec2_dot(onto, onto);
  if (d > MIO_EPSILON) {
    real32 dp = mio_vec2_dot(v, onto);
    return mio_vec2_mul(onto, dp / d);
  }
  return mio_vec2(0.0f, 0.0f);
}

mioVec2 mio_vec2_scale(mioVec2 a, mioVec2 b) {
  return mio_vec2(a.x * b.x, a.y * b.y);
}

mioVec2 mio_vec2_clamp_magnitude(mioVec2 v, real32 maxLength) {
  real32 lenSq = mio_vec2_length_sq(v);
  if (lenSq > maxLength * maxLength) {
    real32 len = (real32)sqrt(lenSq);
    return mio_vec2_mul(v, maxLength / len);
  }
  return v;
}

real32 mio_vec2_angle(mioVec2 from, mioVec2 to) {
  real32 denominator =
      (real32)sqrt(mio_vec2_length_sq(from) * mio_vec2_length_sq(to));
  if (denominator < MIO_EPSILON) { return 0.0f; }
  {
    real32 dot = mio_clamp(mio_vec2_dot(from, to) / denominator, -1.0f, 1.0f);
    return (real32)acos(dot);
  }
}

real32 mio_vec2_signed_angle(mioVec2 from, mioVec2 to) {
  real32 unsignedAngle = mio_vec2_angle(from, to);
  real32 sign = mio_sign(from.x * to.y - from.y * to.x);
  return unsignedAngle * sign;
}

mioVec2
mio_vec2_bezier_quadratic(mioVec2 p0, mioVec2 p1, mioVec2 p2, real32 t) {
  real32 u = 1.0f - t;
  real32 tt = t * t;
  real32 uu = u * u;
  return mio_vec2(
      uu * p0.x + 2.0f * u * t * p1.x + tt * p2.x,
      uu * p0.y + 2.0f * u * t * p1.y + tt * p2.y);
}

mioVec2 mio_vec2_bezier_cubic(
    mioVec2 p0, mioVec2 p1, mioVec2 p2, mioVec2 p3, real32 t) {
  real32 u = 1.0f - t;
  real32 tt = t * t;
  real32 uu = u * u;
  real32 uuu = uu * u;
  real32 ttt = tt * t;
  return mio_vec2(
      uuu * p0.x + 3.0f * uu * t * p1.x + 3.0f * u * tt * p2.x + ttt * p3.x,
      uuu * p0.y + 3.0f * uu * t * p1.y + 3.0f * u * tt * p2.y + ttt * p3.y);
}

mioVec2 mio_vec2_catmull_rom(
    mioVec2 p0, mioVec2 p1, mioVec2 p2, mioVec2 p3, real32 t) {
  real32 t2 = t * t;
  real32 t3 = t2 * t;
  return mio_vec2(
      0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * t +
              (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 +
              (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3),
      0.5f * ((2.0f * p1.y) + (-p0.y + p2.y) * t +
              (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 +
              (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3));
}

mioVec3 mio_vec3_lerp_unclamped(mioVec3 a, mioVec3 b, real32 t) {
  return mio_vec3(
      a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
}

mioVec3 mio_vec3_inverse_lerp(mioVec3 a, mioVec3 b, mioVec3 value) {
  return mio_vec3(
      mio_inverse_lerp(a.x, b.x, value.x), mio_inverse_lerp(a.y, b.y, value.y),
      mio_inverse_lerp(a.z, b.z, value.z));
}

mioVec3 mio_vec3_move_towards(
    mioVec3 current, mioVec3 target, real32 maxDistanceDelta) {
  mioVec3 delta = mio_vec3_sub(target, current);
  real32 distSq = mio_vec3_length_sq(delta);
  if (distSq == 0.0f || (maxDistanceDelta >= 0.0f &&
                         distSq <= maxDistanceDelta * maxDistanceDelta)) {
    return target;
  }
  {
    real32 dist = (real32)sqrt(distSq);
    return mio_vec3_add(current, mio_vec3_scale(delta, maxDistanceDelta / dist));
  }
}

mioVec3 mio_vec3_mul(mioVec3 a, mioVec3 b) {
  return mio_vec3(a.x * b.x, a.y * b.y, a.z * b.z);
}

mioVec3 mio_vec3_clamp_magnitude(mioVec3 v, real32 maxLength) {
  real32 lenSq = mio_vec3_length_sq(v);
  if (lenSq > maxLength * maxLength) {
    real32 len = (real32)sqrt(lenSq);
    return mio_vec3_scale(v, maxLength / len);
  }
  return v;
}

mioVec3 mio_vec3_rotate_around(
    mioVec3 point, mioVec3 pivot, mioVec3 axis, real32 angle) {
  mioQuat rotation = mio_quat_axis_angle(axis, angle);
  mioVec3 direction = mio_vec3_sub(point, pivot);
  direction = mio_quat_rotate_vec3(rotation, direction);
  return mio_vec3_add(direction, pivot);
}

mioVec3 mio_vec3_rotate_towards(
    mioVec3 current, mioVec3 target, real32 maxRadiansDelta,
    real32 maxMagnitudeDelta) {
  real32 currentMag = mio_vec3_length(current);
  real32 targetMag = mio_vec3_length(target);
  real32 newMag;

  if (currentMag < MIO_EPSILON || targetMag < MIO_EPSILON) {
    return mio_vec3_move_towards(current, target, maxMagnitudeDelta);
  }

  {
    mioVec3 currentNorm = mio_vec3_div(current, currentMag);
    mioVec3 targetNorm = mio_vec3_div(target, targetMag);
    real32 dot = mio_vec3_dot(currentNorm, targetNorm);

    if (dot > 0.9999f) {
      return mio_vec3_move_towards(current, target, maxMagnitudeDelta);
    }

    if (dot < -0.9999f) {
      mioVec3 axis = mio_vec3_cross(mio_vec3(0.0f, 1.0f, 0.0f), currentNorm);
      if (mio_vec3_length_sq(axis) < MIO_EPSILON) {
        axis = mio_vec3_cross(mio_vec3(1.0f, 0.0f, 0.0f), currentNorm);
      }
      axis = mio_vec3_normalize(axis);
      {
        mioQuat rotation = mio_quat_axis_angle(axis, maxRadiansDelta);
        mioVec3 rotated = mio_quat_rotate_vec3(rotation, currentNorm);
        newMag = mio_move_towards(currentMag, targetMag, maxMagnitudeDelta);
        return mio_vec3_scale(rotated, newMag);
      }
    }

    {
      real32 angle = (real32)acos(dot);
      real32 actualAngle = mio_min(maxRadiansDelta, angle);
      mioVec3 axis =
          mio_vec3_normalize(mio_vec3_cross(currentNorm, targetNorm));
      mioQuat rotation = mio_quat_axis_angle(axis, actualAngle);
      mioVec3 rotated = mio_quat_rotate_vec3(rotation, currentNorm);
      newMag = mio_move_towards(currentMag, targetMag, maxMagnitudeDelta);
      return mio_vec3_scale(rotated, newMag);
    }
  }
}

mioVec3
mio_vec3_bezier_quadratic(mioVec3 p0, mioVec3 p1, mioVec3 p2, real32 t) {
  real32 u = 1.0f - t;
  real32 tt = t * t;
  real32 uu = u * u;
  return mio_vec3(
      uu * p0.x + 2.0f * u * t * p1.x + tt * p2.x,
      uu * p0.y + 2.0f * u * t * p1.y + tt * p2.y,
      uu * p0.z + 2.0f * u * t * p1.z + tt * p2.z);
}

mioVec3 mio_vec3_bezier_cubic(
    mioVec3 p0, mioVec3 p1, mioVec3 p2, mioVec3 p3, real32 t) {
  real32 u = 1.0f - t;
  real32 tt = t * t;
  real32 uu = u * u;
  real32 uuu = uu * u;
  real32 ttt = tt * t;
  return mio_vec3(
      uuu * p0.x + 3.0f * uu * t * p1.x + 3.0f * u * tt * p2.x + ttt * p3.x,
      uuu * p0.y + 3.0f * uu * t * p1.y + 3.0f * u * tt * p2.y + ttt * p3.y,
      uuu * p0.z + 3.0f * uu * t * p1.z + 3.0f * u * tt * p2.z + ttt * p3.z);
}

mioVec3 mio_vec3_catmull_rom(
    mioVec3 p0, mioVec3 p1, mioVec3 p2, mioVec3 p3, real32 t) {
  real32 t2 = t * t;
  real32 t3 = t2 * t;
  return mio_vec3(
      0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * t +
              (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 +
              (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3),
      0.5f * ((2.0f * p1.y) + (-p0.y + p2.y) * t +
              (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 +
              (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3),
      0.5f * ((2.0f * p1.z) + (-p0.z + p2.z) * t +
              (2.0f * p0.z - 5.0f * p1.z + 4.0f * p2.z - p3.z) * t2 +
              (-p0.z + 3.0f * p1.z - 3.0f * p2.z + p3.z) * t3));
}

mioVec3
mio_vec3_hermite(mioVec3 p0, mioVec3 t0, mioVec3 p1, mioVec3 t1, real32 t) {
  real32 t2 = t * t;
  real32 t3 = t2 * t;
  real32 h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
  real32 h10 = t3 - 2.0f * t2 + t;
  real32 h01 = -2.0f * t3 + 3.0f * t2;
  real32 h11 = t3 - t2;

  return mio_vec3(
      h00 * p0.x + h10 * t0.x + h01 * p1.x + h11 * t1.x,
      h00 * p0.y + h10 * t0.y + h01 * p1.y + h11 * t1.y,
      h00 * p0.z + h10 * t0.z + h01 * p1.z + h11 * t1.z);
}

/* @3D DRAWING ***************************************************************/

#define MAX_CLIPPED_VERTS 16

typedef struct {
  mioVec4 vertices[MAX_CLIPPED_VERTS];
  int32 count;
} mioClippedFace;

mioVertex mio_vertex(mioVec3 pos) {
  mioVertex ret;
  ret.pos = mio_vec4(pos.x, pos.y, pos.z, 1.0f);
  ret.uv.x = 0.0f;
  ret.uv.y = 0.0f;
  ret.normal = mio_vec3(0.0f, 0.0f, 0.0f);
  return ret;
}

uint32 mio_3d_helper_set_bits(uint32 target, uint32 mask, SYSRET enable) {
  if (enable) {
    return target | mask;
  } else {
    return target & ~mask;
  }
}

uint32 mio_3d_helper_toggle_bits(uint32 target, uint32 mask) {
  return target ^ mask;
}

uint32 mio_3d_set_flag(uint32 flag, SYSRET enable) {
  G_APP.render.flags3D =
      mio_3d_helper_set_bits(G_APP.render.flags3D, flag, enable);
  return G_APP.render.flags3D;
}

uint32 mio_3d_toggle_flag(uint32 flag) {
  G_APP.render.flags3D = mio_3d_helper_toggle_bits(G_APP.render.flags3D, flag);
  return G_APP.render.flags3D;
}

SYSRET mio_3d_is_flag_set(uint32 flag) {
  return (G_APP.render.flags3D & flag) != 0;
}

SYSRET mio_3d_toggle_depth_test(SYSRET enable) {
  return mio_3d_set_flag(MIO_3D_DEPTH_TEST, enable);
}

SYSRET mio_3d_toggle_wireframe(SYSRET enable) {
  if (enable) {
    mio_3d_set_flag(MIO_3D_SOLID, FALSE);
    mio_3d_set_flag(MIO_3D_TEXTURE, FALSE);
    mio_3d_set_flag(MIO_3D_NORMALS, FALSE);
  }
  return mio_3d_set_flag(MIO_3D_WIREFRAME, enable);
}

SYSRET mio_3d_toggle_solid(SYSRET enable) {
  if (enable) { mio_3d_set_flag(MIO_3D_WIREFRAME, FALSE); }
  return mio_3d_set_flag(MIO_3D_SOLID, enable);
}

SYSRET mio_3d_toggle_texture(SYSRET enable) {
  if (enable) { mio_3d_set_flag(MIO_3D_WIREFRAME, FALSE); }
  return mio_3d_set_flag(MIO_3D_TEXTURE, enable);
}

SYSRET mio_3d_toggle_backface_culling(SYSRET enable) {
  return mio_3d_set_flag(MIO_3D_CULL_BACKFACE, enable);
}

SYSRET mio_3d_toggle_flat_shading(SYSRET enable) {
  return mio_3d_set_flag(MIO_3D_SHADE_FLAT, enable);
}

SYSRET mio_3d_toggle_normals_view(SYSRET enable) {
  if (enable) {
    mio_3d_set_flag(MIO_3D_SOLID, FALSE);
    mio_3d_set_flag(MIO_3D_TEXTURE, FALSE);
    mio_3d_set_flag(MIO_3D_WIREFRAME, FALSE);
  }
  return mio_3d_set_flag(MIO_3D_NORMALS, enable);
}

mioCamera mio_3d_camera(
    uint32 proj, real32 fov, real32 n, real32 f, real32 vm, real32 vr) {
  mioCamera cam;
  memset(&cam, 0, sizeof(cam));
  cam.fov = fov;
  cam.nearPlane = n;
  cam.farPlane = f;
  cam.moveSpeed = vm;
  cam.lookSpeed = vr;
  cam.zoom = 0.0f;
  cam.projection = mio_mat4_identity();
  return cam;
}

int32 mio_3d_camera_update(
    mioCamera *camera, uint32 width, uint32 height, uint32 proj, real32 dt) {
  mioVec3 forward, right, up;
  real32 cosYaw, sinYaw, cosPitch, sinPitch;
  real32 aspect = (real32)height / (real32)width;
  real32 speed;

  if (camera->zoom < MIO_EPSILON) { camera->zoom = 1.0f; }
  camera->pitch =
      MIO_CLAMP(camera->pitch, MIO_DEG_TO_RAD(-89.0f), MIO_DEG_TO_RAD(89.0f));

  cosYaw = (real32)cos(camera->yaw);
  sinYaw = (real32)sin(camera->yaw);
  cosPitch = (real32)cos(camera->pitch);
  sinPitch = (real32)sin(camera->pitch);

  forward.x = sinYaw * -cosPitch;
  forward.y = sinPitch;
  forward.z = cosYaw * -cosPitch;

  right.x = cosYaw;
  right.y = 0.0f;
  right.z = -sinYaw;

  up = mio_vec3(0.0f, 1.0f, 0.0f);
  speed = camera->moveSpeed * dt;

  if (sys_key_down(KEY_W)) {
    mioVec3 moveDir = forward;
    if (!proj) {
      moveDir.y = 0.0f;
      moveDir = mio_vec3_normalize(moveDir);
    }
    camera->pos = mio_vec3_add(camera->pos, mio_vec3_scale(moveDir, speed));
  }
  if (sys_key_down(KEY_S)) {
    mioVec3 moveDir = forward;
    if (!proj) {
      moveDir.y = 0.0f;
      moveDir = mio_vec3_normalize(moveDir);
    }
    camera->pos = mio_vec3_sub(camera->pos, mio_vec3_scale(moveDir, speed));
  }
  if (sys_key_down(KEY_A)) {
    camera->pos = mio_vec3_sub(camera->pos, mio_vec3_scale(right, speed));
  }
  if (sys_key_down(KEY_D)) {
    camera->pos = mio_vec3_add(camera->pos, mio_vec3_scale(right, speed));
  }

  if (sys_key_down(KEY_R)) {
    camera->pos = mio_vec3_add(camera->pos, mio_vec3_scale(up, speed));
  }
  if (sys_key_down(KEY_F)) {
    camera->pos = mio_vec3_sub(camera->pos, mio_vec3_scale(up, speed));
  }

  if (sys_key_down(KEY_LEFT)) { camera->yaw += camera->lookSpeed * dt; }
  if (sys_key_down(KEY_DOWN)) { camera->pitch += camera->lookSpeed * dt; }
  if (sys_key_down(KEY_RIGHT)) { camera->yaw -= camera->lookSpeed * dt; }
  if (sys_key_down(KEY_UP)) { camera->pitch -= camera->lookSpeed * dt; }

  if (sys_key_down(KEY_Q)) { camera->zoom *= 1.0f - dt * 2.0f; }
  if (sys_key_down(KEY_E)) { camera->zoom *= 1.0f + dt * 2.0f; }
  camera->zoom = MIO_CLAMP(camera->zoom, 0.01f, 100.0f);

  camera->width = (real32)width;
  camera->height = (real32)height;

  if (proj) {
    real32 newFov = MIO_CLAMP(
        camera->fov * camera->zoom, MIO_DEG_TO_RAD(1.0f),
        MIO_DEG_TO_RAD(179.0f));
    camera->projection = mio_mat4_perspective(
        newFov, camera->width / camera->height, camera->nearPlane,
        camera->farPlane);
  } else {
    camera->projection = mio_mat4_ortho(
        -camera->zoom, camera->zoom, aspect * camera->zoom,
        -aspect * camera->zoom, -camera->farPlane, camera->farPlane);
  }
  return TRUE;
}

void mio_3d_clear_depth(void) {
  int32 i;
  for (i = 0; i < G_APP.render.width * G_APP.render.height; i++) {
    G_APP.render.depthData[i] = 1.0f;
  }
}

mioVertex mio_vertex_lerp(mioVertex v1, mioVertex v2, real32 t) {
  mioVertex res;
  res.pos.x = v1.pos.x + t * (v2.pos.x - v1.pos.x);
  res.pos.y = v1.pos.y + t * (v2.pos.y - v1.pos.y);
  res.pos.z = v1.pos.z + t * (v2.pos.z - v1.pos.z);
  res.pos.w = v1.pos.w + t * (v2.pos.w - v1.pos.w);
  res.uv.x = v1.uv.x + t * (v2.uv.x - v1.uv.x);
  res.uv.y = v1.uv.y + t * (v2.uv.y - v1.uv.y);
  return res;
}

int32 mio_clip_polygon(mioVertex *in, int32 inCount, mioVertex *out) {
  mioVertex s;
  mioVertex p;
  mioVertex tempVerts[MAX_CLIPPED_VERTS];
  mioVertex *inPtr = in;
  mioVertex *outPtr = out;
  int32 outCount;
  int32 planeIdx, j;
  real32 sVal, pVal, t;
  inPtr = in;
  outPtr = out;
  for (planeIdx = 0; planeIdx < 6; planeIdx++) {
    outCount = 0;
    s = inPtr[inCount - 1];
    for (j = 0; j < inCount; j++) {
      p = inPtr[j];
      sVal = (planeIdx == 0)   ? (s.pos.x + s.pos.w)
             : (planeIdx == 1) ? (-s.pos.x + s.pos.w)
             : (planeIdx == 2) ? (-s.pos.y + s.pos.w)
             : (planeIdx == 3) ? (s.pos.y + s.pos.w)
             : (planeIdx == 4) ? (s.pos.z + s.pos.w)
                               : (-s.pos.z + s.pos.w);
      pVal = (planeIdx == 0)   ? (p.pos.x + p.pos.w)
             : (planeIdx == 1) ? (-p.pos.x + p.pos.w)
             : (planeIdx == 2) ? (-p.pos.y + p.pos.w)
             : (planeIdx == 3) ? (p.pos.y + p.pos.w)
             : (planeIdx == 4) ? (p.pos.z + p.pos.w)
                               : (-p.pos.z + p.pos.w);
      if (pVal >= 0.0f) {
        if (sVal < 0.0f) {
          t = sVal / (sVal - pVal);
          outPtr[outCount++] = mio_vertex_lerp(s, p, t);
        }
        outPtr[outCount++] = p;
      } else if (sVal >= 0.0f) {
        t = sVal / (sVal - pVal);
        outPtr[outCount++] = mio_vertex_lerp(s, p, t);
      }
      s = p;
    }

    if (outCount == 0) { return 0; }
    for (j = 0; j < outCount; j++) { tempVerts[j] = outPtr[j]; }

    inPtr = tempVerts;
    inCount = outCount;
  }
  for (j = 0; j < outCount; j++) { out[j] = inPtr[j]; }
  return outCount;
}

MIO_GLOBAL SYSRET mio_cull_mesh(mioMesh *mesh, mioMat4 mvp) {
  int32 outsideCount;
  int32 i;
  mioVec4 corners[8];
  corners[0] =
      mio_vec4(mesh->boundsMin.x, mesh->boundsMin.y, mesh->boundsMin.z, 1.0f);
  corners[1] =
      mio_vec4(mesh->boundsMax.x, mesh->boundsMin.y, mesh->boundsMin.z, 1.0f);
  corners[2] =
      mio_vec4(mesh->boundsMin.x, mesh->boundsMax.y, mesh->boundsMin.z, 1.0f);
  corners[3] =
      mio_vec4(mesh->boundsMax.x, mesh->boundsMax.y, mesh->boundsMin.z, 1.0f);
  corners[4] =
      mio_vec4(mesh->boundsMin.x, mesh->boundsMin.y, mesh->boundsMax.z, 1.0f);
  corners[5] =
      mio_vec4(mesh->boundsMax.x, mesh->boundsMin.y, mesh->boundsMax.z, 1.0f);
  corners[6] =
      mio_vec4(mesh->boundsMin.x, mesh->boundsMax.y, mesh->boundsMax.z, 1.0f);
  corners[7] =
      mio_vec4(mesh->boundsMax.x, mesh->boundsMax.y, mesh->boundsMax.z, 1.0f);
  for (i = 0; i < 8; i++) { corners[i] = mio_mat4_mul_vec4(mvp, corners[i]); }
  outsideCount = 0;
  for (i = 0; i < 8; i++) {
    if (corners[i].x > corners[i].w) { outsideCount++; }
  }
  if (outsideCount == 8) { return TRUE; }
  outsideCount = 0;
  for (i = 0; i < 8; i++) {
    if (corners[i].x < -corners[i].w) { outsideCount++; }
  }
  if (outsideCount == 8) { return TRUE; }
  outsideCount = 0;
  for (i = 0; i < 8; i++) {
    if (corners[i].y > corners[i].w) { outsideCount++; }
  }
  if (outsideCount == 8) { return TRUE; }
  outsideCount = 0;
  for (i = 0; i < 8; i++) {
    if (corners[i].y < -corners[i].w) { outsideCount++; }
  }
  if (outsideCount == 8) { return TRUE; }
  outsideCount = 0;
  for (i = 0; i < 8; i++) {
    if (corners[i].z > corners[i].w) { outsideCount++; }
  }
  if (outsideCount == 8) { return TRUE; }
  outsideCount = 0;
  for (i = 0; i < 8; i++) {
    if (corners[i].z < -corners[i].w) { outsideCount++; }
  }
  if (outsideCount == 8) { return TRUE; }
  return FALSE;
}

mioMesh *mio_create_heightmap_mesh(
    real32 *heightmap_data, int32 heightmap_width, int32 heightmap_height,
    real32 size, real32 height_scale) {
  int32 i, j, k;
  int32 vertex_count = heightmap_width * heightmap_height;
  int32 index_count = (heightmap_width - 1) * (heightmap_height - 1) * 6;
  mioMesh *mesh = (mioMesh *)sys_alloc(sizeof(mioMesh));
  mioVertex *vertices =
      (mioVertex *)sys_alloc(sizeof(mioVertex) * vertex_count);
  int32 *indices = (int32 *)sys_alloc(sizeof(int32) * index_count);
  real32 half_size = size / 2.0f;
  real32 step_x = size / (real32)(heightmap_width - 1);
  real32 step_z = size / (real32)(heightmap_height - 1);
  if (!mesh || !vertices || !indices) {
    if (mesh) { sys_free(mesh); }
    if (vertices) { sys_free(vertices); }
    if (indices) { sys_free(indices); }
    return NULL;
  }
  k = 0;
  for (i = 0; i < heightmap_height; i++) {
    for (j = 0; j < heightmap_width; j++) {
      real32 x = (real32)j * step_x - half_size;
      real32 z = (real32)i * step_z - half_size;
      real32 y = (heightmap_data[j + i * heightmap_width]) * height_scale;
      vertices[k].pos = mio_vec4(x, y, z, 1.0f);
      vertices[k].uv = mio_vec2(
          (real32)j / (real32)(heightmap_width - 1),
          (real32)i / (real32)(heightmap_height - 1));
      k++;
    }
  }
  k = 0;
  for (i = 0; i < heightmap_height - 1; i++) {
    for (j = 0; j < heightmap_width - 1; j++) {
      int32 v0 = i * heightmap_width + j;
      int32 v1 = v0 + 1;
      int32 v2 = (i + 1) * heightmap_width + j;
      int32 v3 = v2 + 1;
      indices[k++] = v0;
      indices[k++] = v2;
      indices[k++] = v1;
      indices[k++] = v1;
      indices[k++] = v2;
      indices[k++] = v3;
    }
  }
  mesh->vertices = vertices;
  mesh->indices = indices;
  mesh->vertexCount = vertex_count;
  mesh->indexCount = index_count;
  return mesh;
}

mioMesh mio_create_plane_mesh(real32 width, real32 height) {
  mioMesh plane;
  mioVertex *vertices;
  int32 *indices;
  vertices = (mioVertex *)sys_alloc(sizeof(mioVertex) * 4);
  indices = (int32 *)sys_alloc(sizeof(int32) * 6);

  vertices[0].pos.x = -width / 2.0f;
  vertices[0].pos.y = 0.0f;
  vertices[0].pos.z = -height / 2.0f;
  vertices[0].pos.w = 1.0f;
  vertices[0].uv.x = 0.0f;
  vertices[0].uv.y = 0.0f;

  vertices[1].pos.x = width / 2.0f;
  vertices[1].pos.y = 0.0f;
  vertices[1].pos.z = -height / 2.0f;
  vertices[1].pos.w = 1.0f;
  vertices[1].uv.x = 1.0f;
  vertices[1].uv.y = 0.0f;

  vertices[2].pos.x = width / 2.0f;
  vertices[2].pos.y = 0.0f;
  vertices[2].pos.z = height / 2.0f;
  vertices[2].pos.w = 1.0f;
  vertices[2].uv.x = 1.0f;
  vertices[2].uv.y = 1.0f;

  vertices[3].pos.x = -width / 2.0f;
  vertices[3].pos.y = 0.0f;
  vertices[3].pos.z = height / 2.0f;
  vertices[3].pos.w = 1.0f;
  vertices[3].uv.x = 0.0f;
  vertices[3].uv.y = 1.0f;

  indices[0] = 1;
  indices[1] = 0;
  indices[2] = 2;
  indices[3] = 2;
  indices[4] = 0;
  indices[5] = 3;

  plane.vertices = vertices;
  plane.indices = indices;
  plane.vertexCount = 4;
  plane.indexCount = 6;
  return plane;
}

mioVec2 mio_vertex_to_screen_coordinates(
    mioVertex *vertex, mioMat4 modelViewProjectionMatrix, int32 screenWidth,
    int32 screenHeight) {
  mioVec4 clippedPos =
      mio_mat4_mul_vec4(modelViewProjectionMatrix, vertex->pos);
  if (clippedPos.w != 0.0f) {
    real32 ndcX = clippedPos.x / clippedPos.w;
    real32 ndcY = clippedPos.y / clippedPos.w;
    mioVec2 screenCoords;
    screenCoords.x = (ndcX + 1.0f) * 0.5f * screenWidth;
    screenCoords.y = (1.0f - ndcY) * 0.5f * screenHeight;
    return screenCoords;
  } else {
    mioVec2 screenCoords;
    screenCoords.x = -1.0f;
    screenCoords.y = -1.0f;
    return screenCoords;
  }
}

mioMesh mio_create_sphere_mesh(real32 radius, int32 slices, int32 stacks) {
  mioMesh sphere;
  mioVertex *vertices;
  int32 *indices;
  int32 i, j, k;
  real32 phi, theta;
  real32 sin_phi, cos_phi, sin_theta, cos_theta;
  int32 vertex_count = (slices + 1) * (stacks + 1);
  int32 index_count = slices * stacks * 6;
  vertices = (mioVertex *)sys_alloc(sizeof(mioVertex) * vertex_count);
  indices = (int32 *)sys_alloc(sizeof(int32) * index_count);
  k = 0;
  for (i = 0; i <= stacks; i++) {
    phi = (real32)i / (real32)stacks * (real32)MIO_PI;
    sin_phi = sin(phi);
    cos_phi = cos(phi);
    for (j = 0; j <= slices; j++) {
      theta = (real32)j / (real32)slices * (real32)MIO_TAU;
      sin_theta = sin(theta);
      cos_theta = cos(theta);
      vertices[k].pos.x = radius * cos_theta * sin_phi;
      vertices[k].pos.y = radius * cos_phi;
      vertices[k].pos.z = radius * sin_theta * sin_phi;
      vertices[k].pos.w = 1.0f;
      vertices[k].uv.x = (real32)j / (real32)slices;
      vertices[k].uv.y = (real32)i / (real32)stacks;
      k++;
    }
  }
  k = 0;
  for (i = 0; i < stacks; i++) {
    for (j = 0; j < slices; j++) {
      indices[k++] = i * (slices + 1) + j;
      indices[k++] = (i + 1) * (slices + 1) + j + 1;
      indices[k++] = (i + 1) * (slices + 1) + j;
      indices[k++] = i * (slices + 1) + j;
      indices[k++] = i * (slices + 1) + j + 1;
      indices[k++] = (i + 1) * (slices + 1) + j + 1;
    }
  }
  sphere.vertices = vertices;
  sphere.indices = indices;
  sphere.vertexCount = vertex_count;
  sphere.indexCount = index_count;
  return sphere;
}

mioMesh mio_create_pyramid_mesh(real32 base_width, real32 height) {
  mioMesh pyramid;
  mioVertex *vertices;
  int32 *indices;
  vertices = (mioVertex *)sys_alloc(sizeof(mioVertex) * 5);
  indices = (int32 *)sys_alloc(sizeof(int32) * 18);
  vertices[0].pos.x = -base_width / 2.0f;
  vertices[0].pos.y = -height / 2.0f;
  vertices[0].pos.z = -base_width / 2.0f;
  vertices[0].pos.w = 1.0f;
  vertices[0].uv.x = 0.0f;
  vertices[0].uv.y = 1.0f;
  vertices[1].pos.x = base_width / 2.0f;
  vertices[1].pos.y = -height / 2.0f;
  vertices[1].pos.z = -base_width / 2.0f;
  vertices[1].pos.w = 1.0f;
  vertices[1].uv.x = 1.0f;
  vertices[1].uv.y = 1.0f;
  vertices[2].pos.x = base_width / 2.0f;
  vertices[2].pos.y = -height / 2.0f;
  vertices[2].pos.z = base_width / 2.0f;
  vertices[2].pos.w = 1.0f;
  vertices[2].uv.x = 1.0f;
  vertices[2].uv.y = 0.0f;
  vertices[3].pos.x = -base_width / 2.0f;
  vertices[3].pos.y = -height / 2.0f;
  vertices[3].pos.z = base_width / 2.0f;
  vertices[3].pos.w = 1.0f;
  vertices[3].uv.x = 0.0f;
  vertices[3].uv.y = 0.0f;
  vertices[4].pos.x = 0.0f;
  vertices[4].pos.y = height / 2.0f;
  vertices[4].pos.z = 0.0f;
  vertices[4].pos.w = 1.0f;
  vertices[4].uv.x = 0.5f;
  vertices[4].uv.y = 0.5f;
  indices[0] = 0;
  indices[1] = 1;
  indices[2] = 2;
  indices[3] = 0;
  indices[4] = 2;
  indices[5] = 3;
  indices[6] = 0;
  indices[7] = 4;
  indices[8] = 1;
  indices[9] = 1;
  indices[10] = 4;
  indices[11] = 2;
  indices[12] = 2;
  indices[13] = 4;
  indices[14] = 3;
  indices[15] = 3;
  indices[16] = 4;
  indices[17] = 0;
  pyramid.vertices = vertices;
  pyramid.indices = indices;
  pyramid.vertexCount = 5;
  pyramid.indexCount = 18;
  return pyramid;
}

mioMesh mio_create_cube_mesh(void) {
  mioMesh cube;
  mioVertex *vertices;
  int32 *indices;

  vertices = (mioVertex *)sys_alloc(sizeof(mioVertex) * 24);
  indices = (int32 *)sys_alloc(sizeof(int32) * 36);

  vertices[0].pos.x = -1.0f;
  vertices[0].pos.y = 1.0f;
  vertices[0].pos.z = -1.0f;
  vertices[0].pos.w = 1.0f;
  vertices[0].uv.x = 0.0f;
  vertices[0].uv.y = 0.0f;

  vertices[1].pos.x = 1.0f;
  vertices[1].pos.y = 1.0f;
  vertices[1].pos.z = -1.0f;
  vertices[1].pos.w = 1.0f;
  vertices[1].uv.x = 1.0f;
  vertices[1].uv.y = 0.0f;

  vertices[2].pos.x = 1.0f;
  vertices[2].pos.y = -1.0f;
  vertices[2].pos.z = -1.0f;
  vertices[2].pos.w = 1.0f;
  vertices[2].uv.x = 1.0f;
  vertices[2].uv.y = 1.0f;

  vertices[3].pos.x = -1.0f;
  vertices[3].pos.y = -1.0f;
  vertices[3].pos.z = -1.0f;
  vertices[3].pos.w = 1.0f;
  vertices[3].uv.x = 0.0f;
  vertices[3].uv.y = 1.0f;

  vertices[4].pos.x = -1.0f;
  vertices[4].pos.y = 1.0f;
  vertices[4].pos.z = 1.0f;
  vertices[4].pos.w = 1.0f;
  vertices[4].uv.x = 0.0f;
  vertices[4].uv.y = 0.0f;

  vertices[5].pos.x = 1.0f;
  vertices[5].pos.y = 1.0f;
  vertices[5].pos.z = 1.0f;
  vertices[5].pos.w = 1.0f;
  vertices[5].uv.x = 1.0f;
  vertices[5].uv.y = 0.0f;

  vertices[6].pos.x = 1.0f;
  vertices[6].pos.y = 1.0f;
  vertices[6].pos.z = -1.0f;
  vertices[6].pos.w = 1.0f;
  vertices[6].uv.x = 1.0f;
  vertices[6].uv.y = 1.0f;

  vertices[7].pos.x = -1.0f;
  vertices[7].pos.y = 1.0f;
  vertices[7].pos.z = -1.0f;
  vertices[7].pos.w = 1.0f;
  vertices[7].uv.x = 0.0f;
  vertices[7].uv.y = 1.0f;

  vertices[8].pos.x = -1.0f;
  vertices[8].pos.y = -1.0f;
  vertices[8].pos.z = -1.0f;
  vertices[8].pos.w = 1.0f;
  vertices[8].uv.x = 0.0f;
  vertices[8].uv.y = 0.0f;

  vertices[9].pos.x = 1.0f;
  vertices[9].pos.y = -1.0f;
  vertices[9].pos.z = -1.0f;
  vertices[9].pos.w = 1.0f;
  vertices[9].uv.x = 1.0f;
  vertices[9].uv.y = 0.0f;

  vertices[10].pos.x = 1.0f;
  vertices[10].pos.y = -1.0f;
  vertices[10].pos.z = 1.0f;
  vertices[10].pos.w = 1.0f;
  vertices[10].uv.x = 1.0f;
  vertices[10].uv.y = 1.0f;

  vertices[11].pos.x = -1.0f;
  vertices[11].pos.y = -1.0f;
  vertices[11].pos.z = 1.0f;
  vertices[11].pos.w = 1.0f;
  vertices[11].uv.x = 0.0f;
  vertices[11].uv.y = 1.0f;

  vertices[12].pos.x = 1.0f;
  vertices[12].pos.y = 1.0f;
  vertices[12].pos.z = -1.0f;
  vertices[12].pos.w = 1.0f;
  vertices[12].uv.x = 0.0f;
  vertices[12].uv.y = 0.0f;

  vertices[13].pos.x = 1.0f;
  vertices[13].pos.y = 1.0f;
  vertices[13].pos.z = 1.0f;
  vertices[13].pos.w = 1.0f;
  vertices[13].uv.x = 1.0f;
  vertices[13].uv.y = 0.0f;

  vertices[14].pos.x = 1.0f;
  vertices[14].pos.y = -1.0f;
  vertices[14].pos.z = 1.0f;
  vertices[14].pos.w = 1.0f;
  vertices[14].uv.x = 1.0f;
  vertices[14].uv.y = 1.0f;

  vertices[15].pos.x = 1.0f;
  vertices[15].pos.y = -1.0f;
  vertices[15].pos.z = -1.0f;
  vertices[15].pos.w = 1.0f;
  vertices[15].uv.x = 0.0f;
  vertices[15].uv.y = 1.0f;

  vertices[16].pos.x = -1.0f;
  vertices[16].pos.y = 1.0f;
  vertices[16].pos.z = 1.0f;
  vertices[16].pos.w = 1.0f;
  vertices[16].uv.x = 0.0f;
  vertices[16].uv.y = 0.0f;

  vertices[17].pos.x = -1.0f;
  vertices[17].pos.y = 1.0f;
  vertices[17].pos.z = -1.0f;
  vertices[17].pos.w = 1.0f;
  vertices[17].uv.x = 1.0f;
  vertices[17].uv.y = 0.0f;

  vertices[18].pos.x = -1.0f;
  vertices[18].pos.y = -1.0f;
  vertices[18].pos.z = -1.0f;
  vertices[18].pos.w = 1.0f;
  vertices[18].uv.x = 1.0f;
  vertices[18].uv.y = 1.0f;

  vertices[19].pos.x = -1.0f;
  vertices[19].pos.y = -1.0f;
  vertices[19].pos.z = 1.0f;
  vertices[19].pos.w = 1.0f;
  vertices[19].uv.x = 0.0f;
  vertices[19].uv.y = 1.0f;

  vertices[20].pos.x = 1.0f;
  vertices[20].pos.y = 1.0f;
  vertices[20].pos.z = 1.0f;
  vertices[20].pos.w = 1.0f;
  vertices[20].uv.x = 0.0f;
  vertices[20].uv.y = 0.0f;

  vertices[21].pos.x = -1.0f;
  vertices[21].pos.y = 1.0f;
  vertices[21].pos.z = 1.0f;
  vertices[21].pos.w = 1.0f;
  vertices[21].uv.x = 1.0f;
  vertices[21].uv.y = 0.0f;

  vertices[22].pos.x = -1.0f;
  vertices[22].pos.y = -1.0f;
  vertices[22].pos.z = 1.0f;
  vertices[22].pos.w = 1.0f;
  vertices[22].uv.x = 1.0f;
  vertices[22].uv.y = 1.0f;

  vertices[23].pos.x = 1.0f;
  vertices[23].pos.y = -1.0f;
  vertices[23].pos.z = 1.0f;
  vertices[23].pos.w = 1.0f;
  vertices[23].uv.x = 0.0f;
  vertices[23].uv.y = 1.0f;

  indices[0] = 0;
  indices[1] = 1;
  indices[2] = 2;
  indices[3] = 0;
  indices[4] = 2;
  indices[5] = 3;
  indices[6] = 4;
  indices[7] = 5;
  indices[8] = 6;
  indices[9] = 4;
  indices[10] = 6;
  indices[11] = 7;
  indices[12] = 8;
  indices[13] = 9;
  indices[14] = 10;
  indices[15] = 8;
  indices[16] = 10;
  indices[17] = 11;
  indices[18] = 12;
  indices[19] = 13;
  indices[20] = 14;
  indices[21] = 12;
  indices[22] = 14;
  indices[23] = 15;
  indices[24] = 16;
  indices[25] = 17;
  indices[26] = 18;
  indices[27] = 16;
  indices[28] = 18;
  indices[29] = 19;
  indices[30] = 20;
  indices[31] = 21;
  indices[32] = 22;
  indices[33] = 20;
  indices[34] = 22;
  indices[35] = 23;

  cube.vertices = vertices;
  cube.indices = indices;
  cube.vertexCount = 24;
  cube.indexCount = 36;
  return cube;
}

void mio_3d_get_bounds(mioMesh *mesh, mioVec3 *min, mioVec3 *max) {
  int32 i;
  mioVec4 vert;
  min->x = MIO_REAL32_MAX;
  min->y = MIO_REAL32_MAX;
  min->z = MIO_REAL32_MAX;
  max->x = MIO_REAL32_MIN;
  max->y = MIO_REAL32_MIN;
  max->z = MIO_REAL32_MIN;
  if (mesh && mesh->vertices) {
    for (i = 0; i < mesh->vertexCount; i++) {
      vert = mesh->vertices[i].pos;
      min->x = MIO_MIN(min->x, vert.x);
      min->y = MIO_MIN(min->y, vert.y);
      min->z = MIO_MIN(min->z, vert.z);
      max->x = MIO_MAX(max->x, vert.x);
      max->y = MIO_MAX(max->y, vert.y);
      max->z = MIO_MAX(max->z, vert.z);
    }
  } else {
    *min = mio_vec3(-1.0f, -1.0f, -1.0f);
    *max = mio_vec3(1.0f, 1.0f, 1.0f);
  }
}

int32 mio_3d_draw_triangle_span_solid(
    int32 row, real32 sx, real32 ex, real32 texSW, real32 texEW) {
  int32 i;
  int32 indexStart = (int32)sx;
  int32 indexEnd = (int32)ex;
  real32 delta = (ex - sx);
  real32 texStepW = (texEW - texSW) / delta;
  real32 texW = texSW;
  uint32 index = indexStart + row * G_APP.render.width;
  uint32 *pixel = G_APP.render.colourData + index;
  real32 *depth = G_APP.render.depthData + index;
  uint32 col = G_APP.colour;
  real32 invW = 1.0f - texW;
  for (i = indexStart; i < indexEnd; i++) {
    invW = 1.0f - texW;
    if (invW < *depth) {
      *pixel = col;
      *depth = invW;
    }
    depth++;
    pixel++;
    texW += texStepW;
  }
  return TRUE;
}

int32 mio_3d_draw_triangle_span_texture(
    int32 row, real32 sx, real32 ex, real32 texSU, real32 texSV, real32 texSW,
    real32 texEU, real32 texEV, real32 texEW, real32 lightValue,
    uint32 *texture, uint32 tW, uint32 tH) {
  int32 i;
  int32 tx, ty;
  real32 corrU, corrV;
  int32 dw = G_APP.render.width;
  int32 dh = G_APP.render.height;
  int32 indexStart = (int32)sx;
  int32 indexEnd = (int32)ex;
  uint32 index = indexStart + row * G_APP.render.width;
  real32 delta = 1.0f / (ex - sx);
  real32 texStepW = (texEW - texSW) * delta;
  real32 texStepX = (texEU - texSU) * delta;
  real32 texStepY = (texEV - texSV) * delta;
  real32 texU = texSU;
  real32 texV = texSV;
  real32 texW = texSW;
  real32 texWidth = (real32)(tW - 1);
  real32 texHeight = (real32)(tH - 1);
  uint8 *texBytes;
  uint8 *pixelBytes;
  uint32 *pixel = NULL;
  real32 *depth = NULL;
  real32 dval = 1.0f - texW;
  real32 invW = 0.0f;
  uint8 r, g, b;
  if (row < 0 || row >= dh) { return TRUE; }
  if (indexStart < 0 && indexEnd < 0) { return TRUE; }
  if (indexStart >= dw && indexEnd >= dw) { return TRUE; }
  if (G_APP.render.flags3D & MIO_3D_AFFINE_MAP) {
    for (i = indexStart; i < indexEnd; i++) {
      dval = 1.0f - texW;
      if (dval < G_APP.render.depthData[i + row * G_APP.render.width]) {
        tx = (int32)(texU * texWidth);
        ty = (int32)(texV * texHeight);
        texBytes = (uint8 *)&texture[tx + ty * tW];
        r = (uint8)((real32) * (texBytes + 2) * lightValue);
        g = (uint8)((real32) * (texBytes + 1) * lightValue);
        b = (uint8)((real32) * (texBytes + 0)) * lightValue;
        G_APP.render.colourData[i + row * G_APP.render.width] =
            MIO_RGBA(r, g, b, 255);
        G_APP.render.depthData[i + row * G_APP.render.width] = dval;
      }
      texU += texStepX;
      texV += texStepY;
      texW += texStepW;
    }
  } else {
    pixel = G_APP.render.colourData + index;
    depth = G_APP.render.depthData + index;
    for (i = indexStart; i < indexEnd; i++) {
      dval = 1.0f - texW;
      if (dval < *depth) {
        invW = 1.0 / texW;
        corrU = texU * invW;
        corrV = texV * invW;
        tx = (int32)(corrU * texWidth);
        ty = (int32)(corrV * texHeight);
        texBytes = (uint8 *)&texture[tx + ty * tW];
        pixelBytes = (uint8 *)pixel;
        *(pixelBytes++) = (uint8)(((real32) * (texBytes + 0)) * lightValue);
        *(pixelBytes++) = (uint8)(((real32) * (texBytes + 1)) * lightValue);
        *(pixelBytes++) = (uint8)(((real32) * (texBytes + 2)) * lightValue);
        *depth = dval;
      }
      depth++;
      pixel++;
      texU += texStepX;
      texV += texStepY;
      texW += texStepW;
    }
  }
  return TRUE;
}

int32 mio_3d_draw_triangle_solid(
    real32 lightness, real32 x1, real32 y1, real32 x2, real32 y2, real32 x3,
    real32 y3, real32 w1, real32 w2, real32 w3) {
  real32 absDy1, absDy2;
  real32 dx1, dy1, dw1;
  real32 dx2, dy2, dw2;
  real32 swapTemp;
  real32 sx, ex;
  int32 i;
  real32 imy1;
  real32 imy2;
  int32 iy1, iy2, iy3;
  real32 texSU = 0.0f, texSV = 0.0f, texSW = 0.0f;
  real32 texEU = 0.0f, texEV = 0.0f, texEW = 0.0f;
  real32 stepDAX = 0, stepDBX = 0;
  real32 stepDW1 = 0, stepDW2 = 0;
  x1 += 0.5f;
  x2 += 0.5f;
  x3 += 0.5f;
  y1 += 0.5f;
  y2 += 0.5f;
  y3 += 0.5f;
  if (y2 < y1) {
    MIO_SWAP(x1, x2, swapTemp);
    MIO_SWAP(y1, y2, swapTemp);
    MIO_SWAP(w1, w2, swapTemp);
  }
  if (y3 < y1) {
    MIO_SWAP(x1, x3, swapTemp);
    MIO_SWAP(y1, y3, swapTemp);
    MIO_SWAP(w1, w3, swapTemp);
  }
  if (y3 < y2) {
    MIO_SWAP(x2, x3, swapTemp);
    MIO_SWAP(y2, y3, swapTemp);
    MIO_SWAP(w2, w3, swapTemp);
  }
  iy1 = (int32)(y1);
  iy2 = (int32)(y2);
  iy3 = (int32)(y3);
  dx1 = x2 - x1;
  dy1 = y2 - y1;
  dw1 = w2 - w1;
  dx2 = x3 - x1;
  dy2 = y3 - y1;
  dw2 = w3 - w1;
  absDy1 = 1.0f / MIO_FABS(dy1);
  absDy2 = 1.0f / MIO_FABS(dy2);
  if (dy1) {
    stepDAX = dx1 * absDy1;
    stepDW1 = dw1 * absDy1;
  }
  if (dy2) {
    stepDBX = dx2 * absDy2;
    stepDW2 = dw2 * absDy2;
  }
  if (dy1) {
    for (i = iy1 + 1; i <= y2; i++) {
      imy1 = (real32)(i)-y1;
      sx = (x1 + imy1 * stepDAX);
      ex = (x1 + imy1 * stepDBX);
      texSW = w1 + imy1 * stepDW1;
      texEW = w1 + imy1 * stepDW2;
      if (sx > ex) {
        MIO_SWAP(sx, ex, swapTemp);
        MIO_SWAP(texSU, texEU, swapTemp);
        MIO_SWAP(texSV, texEV, swapTemp);
        MIO_SWAP(texSW, texEW, swapTemp);
      }
      mio_3d_draw_triangle_span_solid(i - 1, sx, ex, texSW, texEW);
    }
  }
  dx1 = x3 - x2;
  dy1 = y3 - y2;
  dw1 = w3 - w2;
  absDy1 = 1.0f / MIO_FABS(dy1);
  absDy2 = 1.0f / MIO_FABS(dy2);
  if (dy1) {
    stepDAX = dx1 * absDy1;
    stepDW1 = dw1 * absDy1;
  }
  if (dy2) { stepDBX = dx2 * absDy2; }
  if (dy1) {
    for (i = iy2 + 1; i <= iy3; i++) {
      imy1 = (real32)(i)-y1;
      imy2 = (real32)(i)-y2;
      sx = (x2 + imy2 * stepDAX);
      ex = (x1 + imy1 * stepDBX);
      texSW = w2 + imy2 * stepDW1;
      texEW = w1 + imy1 * stepDW2;
      if (sx > ex) {
        MIO_SWAP(sx, ex, swapTemp);
        MIO_SWAP(texSU, texEU, swapTemp);
        MIO_SWAP(texSV, texEV, swapTemp);
        MIO_SWAP(texSW, texEW, swapTemp);
      }
      mio_3d_draw_triangle_span_solid(i - 1, sx, ex, texSW, texEW);
    }
  }
  return TRUE;
}

int32 mio_3d_draw_triangle_texture(
    uint32 *texture, uint32 tW, uint32 tH, real32 lightness, real32 x1,
    real32 y1, real32 x2, real32 y2, real32 x3, real32 y3, real32 u1,
    real32 v1, real32 w1, real32 u2, real32 v2, real32 w2, real32 u3,
    real32 v3, real32 w3) {
  real32 absDy1, absDy2;
  real32 dx1, dy1, du1, dv1, dw1;
  real32 dx2, dy2, du2, dv2, dw2;
  real32 swapTemp;
  real32 sx, ex;
  int32 i;
  real32 imy1;
  real32 imy2;
  int32 iy1, iy2, iy3;
  real32 texSU = 0.0f, texSV = 0.0f, texSW = 0.0f;
  real32 texEU = 0.0f, texEV = 0.0f, texEW = 0.0f;
  real32 stepDAX = 0, stepDBX = 0, stepDU1 = 0;
  real32 stepDV1 = 0, stepDW1 = 0, stepDU2 = 0, stepDV2 = 0, stepDW2 = 0;
  x1 += 0.5f;
  x2 += 0.5f;
  x3 += 0.5f;
  y1 += 0.5f;
  y2 += 0.5f;
  y3 += 0.5f;
  if (G_APP.render.flags3D & MIO_3D_AFFINE_MAP) {
    u1 = u1 / w1;
    v1 = v1 / w1;
    u2 = u2 / w2;
    v2 = v2 / w2;
    u3 = u3 / w3;
    v3 = v3 / w3;
  }
  if (y2 < y1) {
    MIO_SWAP(x1, x2, swapTemp);
    MIO_SWAP(y1, y2, swapTemp);
    MIO_SWAP(u1, u2, swapTemp);
    MIO_SWAP(v1, v2, swapTemp);
    MIO_SWAP(w1, w2, swapTemp);
  }
  if (y3 < y1) {
    MIO_SWAP(x1, x3, swapTemp);
    MIO_SWAP(y1, y3, swapTemp);
    MIO_SWAP(u1, u3, swapTemp);
    MIO_SWAP(v1, v3, swapTemp);
    MIO_SWAP(w1, w3, swapTemp);
  }
  if (y3 < y2) {
    MIO_SWAP(x2, x3, swapTemp);
    MIO_SWAP(y2, y3, swapTemp);
    MIO_SWAP(u2, u3, swapTemp);
    MIO_SWAP(v2, v3, swapTemp);
    MIO_SWAP(w2, w3, swapTemp);
  }
  iy1 = (int32)(y1);
  iy2 = (int32)(y2);
  iy3 = (int32)(y3);
  dx1 = x2 - x1;
  dy1 = y2 - y1;
  dv1 = v2 - v1;
  du1 = u2 - u1;
  dw1 = w2 - w1;
  dx2 = x3 - x1;
  dy2 = y3 - y1;
  dv2 = v3 - v1;
  du2 = u3 - u1;
  dw2 = w3 - w1;
  absDy1 = 1.0f / MIO_FABS(dy1);
  absDy2 = 1.0f / MIO_FABS(dy2);
  if (dy1) {
    stepDAX = dx1 * absDy1;
    stepDU1 = du1 * absDy1;
    stepDV1 = dv1 * absDy1;
    stepDW1 = dw1 * absDy1;
  }
  if (dy2) {
    stepDBX = dx2 * absDy2;
    stepDU2 = du2 * absDy2;
    stepDV2 = dv2 * absDy2;
    stepDW2 = dw2 * absDy2;
  }
  if (dy1) {
    for (i = iy1 + 1; i <= y2; i++) {
      imy1 = (real32)(i)-y1;
      sx = (x1 + imy1 * stepDAX);
      ex = (x1 + imy1 * stepDBX);
      if (texture) {
        texSU = u1 + imy1 * stepDU1;
        texSV = v1 + imy1 * stepDV1;
        texEU = u1 + imy1 * stepDU2;
        texEV = v1 + imy1 * stepDV2;
      }
      texSW = w1 + imy1 * stepDW1;
      texEW = w1 + imy1 * stepDW2;

      if (sx > ex) {
        MIO_SWAP(sx, ex, swapTemp);
        MIO_SWAP(texSU, texEU, swapTemp);
        MIO_SWAP(texSV, texEV, swapTemp);
        MIO_SWAP(texSW, texEW, swapTemp);
      }
      mio_3d_draw_triangle_span_texture(
          i - 1, sx, ex, texSU, texSV, texSW, texEU, texEV, texEW, lightness,
          texture, tW, tH);
    }
  }
  stepDU1 = 0;
  stepDV1 = 0;
  dx1 = x3 - x2;
  dy1 = y3 - y2;
  dv1 = v3 - v2;
  du1 = u3 - u2;
  dw1 = w3 - w2;
  absDy1 = 1.0f / MIO_FABS(dy1);
  absDy2 = 1.0f / MIO_FABS(dy2);
  if (dy1) {
    stepDAX = dx1 * absDy1;
    stepDU1 = du1 * absDy1;
    stepDV1 = dv1 * absDy1;
    stepDW1 = dw1 * absDy1;
  }
  if (dy2) { stepDBX = dx2 * absDy2; }
  if (dy1) {
    for (i = iy2 + 1; i <= iy3; i++) {
      imy1 = (real32)(i)-y1;
      imy2 = (real32)(i)-y2;
      sx = (x2 + imy2 * stepDAX);
      ex = (x1 + imy1 * stepDBX);
      if (texture) {
        texSU = u2 + imy2 * stepDU1;
        texSV = v2 + imy2 * stepDV1;
        texEU = u1 + imy1 * stepDU2;
        texEV = v1 + imy1 * stepDV2;
      }
      texSW = w2 + imy2 * stepDW1;
      texEW = w1 + imy1 * stepDW2;
      if (sx > ex) {
        MIO_SWAP(sx, ex, swapTemp);
        MIO_SWAP(texSU, texEU, swapTemp);
        MIO_SWAP(texSV, texEV, swapTemp);
        MIO_SWAP(texSW, texEW, swapTemp);
      }
      mio_3d_draw_triangle_span_texture(
          i - 1, sx, ex, texSU, texSV, texSW, texEU, texEV, texEW, lightness,
          texture, tW, tH);
    }
  }
  return TRUE;
}

void mio_3d_draw_vertex_line(mioVertex v1, mioVertex v2) {
  int32 w = G_APP.render.width;
  int32 h = G_APP.render.height;
  int32 x1 = (int32)((v1.pos.x / v1.pos.w + 1.0f) * 0.5f * w);
  int32 y1 = (int32)((1.0f - (v1.pos.y / v1.pos.w + 1.0f) * 0.5f) * h);
  int32 x2 = (int32)((v2.pos.x / v2.pos.w + 1.0f) * 0.5f * w);
  int32 y2 = (int32)((1.0f - (v2.pos.y / v2.pos.w + 1.0f) * 0.5f) * h);
  real32 zNdcV1 = v1.pos.z / v1.pos.w;
  real32 zNdcV2 = v2.pos.z / v2.pos.w;
  int32 dx = abs(x2 - x1);
  int32 dy = abs(y2 - y1);
  int32 sx = (x1 < x2) ? 1 : -1;
  int32 sy = (y1 < y2) ? 1 : -1;
  int32 err = dx - dy;
  int32 i, e2;
  for (i = 0;; i++) {
    real32 t = (real32)i / (real32)((dx > dy) ? dx : dy);
    real32 zNdcInterpolated = zNdcV1 + t * (zNdcV2 - zNdcV1);
    if (x1 >= 0 && x1 < w && y1 >= 0 && y1 < h) {
      int32 index = y1 * w + x1;
      if (G_APP.render.flags3D & MIO_3D_DEPTH_TEST) {
        if (zNdcInterpolated < G_APP.render.depthData[index]) {
          MIO_DRAW_PIXEL(x1, y1);
          G_APP.render.depthData[index] = zNdcInterpolated;
        }
      } else {
        MIO_DRAW_PIXEL(x1, y1);
      }
    }
    if (x1 == x2 && y1 == y2) { break; }
    e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x1 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y1 += sy;
    }
  }
}

void mio_3d_draw_vertex_triangle(
    mioVertex v1, mioVertex v2, mioVertex v3, uint32 *texture, uint32 tW,
    uint32 tH, real32 light, uint32 ortho) {
  real32 vsf = 2.0f;
  int32 w = G_APP.render.width;
  int32 h = G_APP.render.height;
  int32 off = 0;
  real32 x1f = (v1.pos.x / v1.pos.w + 1.0f) * 0.5f * w - off;
  real32 y1f = (1.0f - (v1.pos.y / v1.pos.w + 1.0f) * 0.5f) * h;
  real32 x2f = (v2.pos.x / v2.pos.w + 1.0f) * 0.5f * w - off;
  real32 y2f = (1.0f - (v2.pos.y / v2.pos.w + 1.0f) * 0.5f) * h;
  real32 x3f = (v3.pos.x / v3.pos.w + 1.0f) * 0.5f * w - off;
  real32 y3f = (1.0f - (v3.pos.y / v3.pos.w + 1.0f) * 0.5f) * h;
  if (G_APP.render.flags3D & MIO_3D_VERTEX_SNAP) {
    x1f = ((real32)((int32)((x1f + 0.5f) / vsf))) * vsf;
    y1f = ((real32)((int32)((y1f + 0.5f) / vsf))) * vsf;
    x2f = ((real32)((int32)((x2f + 0.5f) / vsf))) * vsf;
    y2f = ((real32)((int32)((y2f + 0.5f) / vsf))) * vsf;
    x3f = ((real32)((int32)((x3f + 0.5f) / vsf))) * vsf;
    y3f = ((real32)((int32)((y3f + 0.5f) / vsf))) * vsf;
  }
  if (texture) {
    mio_3d_draw_triangle_texture(
        texture, tW, tH, light, x1f, y1f, x2f, y2f, x3f, y3f,
        v1.uv.x / v1.pos.w, v1.uv.y / v1.pos.w, 1.0f / v1.pos.w,
        v2.uv.x / v2.pos.w, v2.uv.y / v2.pos.w, 1.0f / v2.pos.w,
        v3.uv.x / v3.pos.w, v3.uv.y / v3.pos.w, 1.0f / v3.pos.w);
  } else {
    if (ortho) {
      if (G_APP.render.flags3D & MIO_3D_DEPTH_TEST) {
        mio_3d_draw_triangle_solid(
            light, x1f, y1f, x2f, y2f, x3f, y3f, 1.0f - v1.pos.z,
            1.0f - v2.pos.z, 1.0f - v3.pos.z);
      } else {
        mio_3d_draw_triangle_solid(
            light, x1f, y1f, x2f, y2f, x3f, y3f, 1.0f, 1.0f, 1.0f);
      }
    } else {
      if (G_APP.render.flags3D & MIO_3D_DEPTH_TEST) {
        mio_3d_draw_triangle_solid(
            light, x1f, y1f, x2f, y2f, x3f, y3f, 1.0f / v1.pos.w,
            1.0f / v2.pos.w, 1.0f / v3.pos.w);
      } else {
        mio_3d_draw_triangle_solid(
            light, x1f, y1f, x2f, y2f, x3f, y3f, 1.0f, 1.0f, 1.0f);
      }
    }
  }
}

void mio_3d_draw_mesh(
    mioCamera *cam, mioMesh *mesh, mioMat4 modelWorldMatrix, uint32 *texture,
    uint32 tW, uint32 tH, uint32 ortho) {
  int32 i;
  int32 j;
  uint32 origCol;
  mioVec3 sunDir;
  mioMat4 projectionMatrix;
  mioMat4 viewMatrix;
  mioMat4 viewProjectionMatrix;
  mioMat4 modelViewProjectionMatrix;
  mioMat4 modelViewMatrix;
  mioVertex transformedVerts[3];
  mioClippedFace clippedFace;
  mioVec3 v1;
  mioVec3 v2;
  mioVec3 v3;
  mioVec3 normalVecFace;
  mioVec3 viewVec;
  mioVec4 p1_4;
  mioVec4 p2_4;
  mioVec4 p3_4;
  mioVec4 normalVec4;
  mioVec4 transformedNormal4;
  mioVec3 p1;
  mioVec3 p2;
  mioVec3 p3;
  mioVec3 faceNormal;
  mioVec3 transformedNormal;
  mioVec3 normalizedTransformedNormal;
  int32 behind_near_plane_count = 0;
  real32 lightIntensityFace = 1.0f;
  mioMat4 camera_rot = mio_mat4_identity();
  mioMat4 camera_pos = mio_mat4_identity();
  origCol = G_APP.colour;
  camera_rot = mio_mat4_mul(mio_mat4_rotate_y(-cam->yaw), camera_rot);
  camera_rot = mio_mat4_mul(mio_mat4_rotate_x(-cam->pitch), camera_rot);
  camera_pos = mio_mat4_translate(mio_vec3_scale(cam->pos, -1.0f));
  viewMatrix = mio_mat4_mul(camera_rot, camera_pos);
  mio_3d_get_bounds(mesh, &mesh->boundsMin, &mesh->boundsMax);
  projectionMatrix = cam->projection;
  modelViewMatrix = mio_mat4_mul(viewMatrix, modelWorldMatrix);
  viewProjectionMatrix = mio_mat4_mul(projectionMatrix, viewMatrix);
  modelViewProjectionMatrix =
      mio_mat4_mul(viewProjectionMatrix, modelWorldMatrix);
  if ((G_APP.render.flags3D & MIO_3D_CULL_FRUSTUM) &&
      mio_cull_mesh(mesh, modelViewProjectionMatrix)) {
    return;
  }
  for (i = 0; i < mesh->indexCount / 3; i++) {
    behind_near_plane_count = 0;
    for (j = 0; j < 3; j++) {
      transformedVerts[j] = mesh->vertices[mesh->indices[i * 3 + j]];
      transformedVerts[j].pos =
          mio_mat4_mul_vec4(modelViewMatrix, transformedVerts[j].pos);
      if (transformedVerts[j].pos.z > 0.0f) { behind_near_plane_count++; }
    }
    if ((G_APP.render.flags3D & MIO_3D_CULL_BEHIND) &&
        behind_near_plane_count >= 3) {
      continue;
    }
    v1.x = transformedVerts[0].pos.x;
    v1.y = transformedVerts[0].pos.y;
    v1.z = transformedVerts[0].pos.z;
    v2.x = transformedVerts[1].pos.x;
    v2.y = transformedVerts[1].pos.y;
    v2.z = transformedVerts[1].pos.z;
    v3.x = transformedVerts[2].pos.x;
    v3.y = transformedVerts[2].pos.y;
    v3.z = transformedVerts[2].pos.z;
    normalVecFace = mio_vec3_cross(mio_vec3_sub(v2, v1), mio_vec3_sub(v3, v1));
    viewVec = mio_vec3(0.0f, 0.0f, 0.0f);
    if (ortho) {
      viewVec = mio_vec3(0.0f, 0.0f, 1.0f);
    } else {
      viewVec = mio_vec3_scale(v1, -1.0f);
    }
    if ((G_APP.render.flags3D & MIO_3D_CULL_BACKFACE) &&
        (mio_vec3_dot(normalVecFace, viewVec) < 0)) {
      continue;
    }
    if (G_APP.render.flags3D & MIO_3D_SHADE_FLAT) {
      p1_4 = mesh->vertices[mesh->indices[i * 3 + 0]].pos;
      p2_4 = mesh->vertices[mesh->indices[i * 3 + 1]].pos;
      p3_4 = mesh->vertices[mesh->indices[i * 3 + 2]].pos;
      p1.x = p1_4.x;
      p1.y = p1_4.y;
      p1.z = p1_4.z;
      p2.x = p2_4.x;
      p2.y = p2_4.y;
      p2.z = p2_4.z;
      p3.x = p3_4.x;
      p3.y = p3_4.y;
      p3.z = p3_4.z;
      faceNormal = mio_vec3_normalize(
          mio_vec3_cross(mio_vec3_sub(p2, p1), mio_vec3_sub(p3, p1)));
      normalVec4.x = faceNormal.x;
      normalVec4.y = faceNormal.y;
      normalVec4.z = faceNormal.z;
      normalVec4.w = 0.0f;
      transformedNormal4 = mio_mat4_mul_vec4(modelWorldMatrix, normalVec4);
      transformedNormal.x = transformedNormal4.x;
      transformedNormal.y = transformedNormal4.y;
      transformedNormal.z = transformedNormal4.z;
      normalizedTransformedNormal = mio_vec3_normalize(transformedNormal);
      sunDir =
          mio_vec3(G_APP.render.sunX, G_APP.render.sunY, G_APP.render.sunZ);
      lightIntensityFace = mio_vec3_dot(normalizedTransformedNormal, sunDir);
      lightIntensityFace = lightIntensityFace * 0.5f + 0.5f;
      lightIntensityFace = G_APP.render.ambient +
                           lightIntensityFace * (1.0f - G_APP.render.ambient);
      if (G_APP.render.flags3D & MIO_3D_NORMALS) {
        mio_set_colour(MIO_RGBA(
            (uint8)(normalizedTransformedNormal.x * 128.0f + 128),
            (uint8)(normalizedTransformedNormal.y * 128.0f + 128),
            (uint8)(normalizedTransformedNormal.z * 128.0f + 128), 255));
      } else {
        mio_set_colour(MIO_RGBA(
            (uint8)((real32)MIO_COL_GET_R(origCol) * lightIntensityFace),
            (uint8)((real32)MIO_COL_GET_G(origCol) * lightIntensityFace),
            (uint8)((real32)MIO_COL_GET_B(origCol) * lightIntensityFace),
            255));
      }
    }
    for (j = 0; j < 3; j++) {
      transformedVerts[j].pos =
          mio_mat4_mul_vec4(projectionMatrix, transformedVerts[j].pos);
    }
    clippedFace.count = mio_clip_polygon(
        transformedVerts, 3, (mioVertex *)clippedFace.vertices);
    if (clippedFace.count < 3) { continue; }
    if (G_APP.render.flags3D & MIO_3D_WIREFRAME) {
      for (j = 0; j < clippedFace.count; j++) {
        mio_3d_draw_vertex_line(
            ((mioVertex *)clippedFace.vertices)[j],
            ((mioVertex *)clippedFace.vertices)[(j + 1) % clippedFace.count]);
      }
    } else if (
        G_APP.render.flags3D & MIO_3D_SOLID ||
        G_APP.render.flags3D & MIO_3D_TEXTURE) {
      for (j = 0; j < clippedFace.count - 2; j++) {
        mio_3d_draw_vertex_triangle(
            ((mioVertex *)clippedFace.vertices)[0],
            ((mioVertex *)clippedFace.vertices)[j + 1],
            ((mioVertex *)clippedFace.vertices)[j + 2], texture, tW, tH,
            lightIntensityFace, ortho);
      }
    }
  }
  mio_set_colour(origCol);
}

void mio_3d_draw_mesh_ex(
    mioCamera *cam, mioMesh *mesh, mioVec3 pos, mioVec3 rot, uint32 *texture,
    uint32 tW, uint32 tH, uint32 ortho) {
  int32 i, j;
  uint32 origCol;
  mioVec3 sunDir;
  mioMat4 projectionMatrix, viewMatrix;
  mioMat4 viewProjectionMatrix;
  mioMat4 modelViewProjectionMatrix;
  mioMat4 modelViewMatrix;
  mioMat4 modelWorldMatrix;
  mioVertex transformedVerts[3];
  mioClippedFace clippedFace;
  mioVec3 v1, v2, v3, normalVecFace, viewVec;
  mioVec4 p1_4, p2_4, p3_4, normalVec4, transformedNormal4;
  mioVec3 p1, p2, p3, faceNormal, transformedNormal;
  mioVec3 normalizedTransformedNormal;
  int32 behind_near_plane_count = 0;
  real32 lightIntensityFace = 1.0f;
  real32 posInv = ortho ? -1.0f : -1.0f;
  mioMat4 camera_rot = mio_mat4_identity();
  mioMat4 camera_pos = mio_mat4_identity();
  origCol = G_APP.colour;
  camera_rot = mio_mat4_mul(mio_mat4_rotate_y(-cam->yaw), camera_rot);
  camera_rot = mio_mat4_mul(mio_mat4_rotate_x(-cam->pitch), camera_rot);
  camera_pos = mio_mat4_translate(mio_vec3_scale(cam->pos, posInv));
  viewMatrix = mio_mat4_mul(camera_rot, camera_pos);
  modelWorldMatrix = mio_mat4_identity();
  modelWorldMatrix = mio_mat4_mul(modelWorldMatrix, mio_mat4_rotate_x(rot.x));
  modelWorldMatrix = mio_mat4_mul(modelWorldMatrix, mio_mat4_rotate_y(rot.y));
  modelWorldMatrix = mio_mat4_mul(modelWorldMatrix, mio_mat4_rotate_z(rot.z));
  modelWorldMatrix = mio_mat4_mul(modelWorldMatrix, mio_mat4_translate(pos));
  mio_3d_get_bounds(mesh, &mesh->boundsMin, &mesh->boundsMax);
  projectionMatrix = cam->projection;
  modelViewMatrix = mio_mat4_mul(viewMatrix, modelWorldMatrix);
  viewProjectionMatrix = mio_mat4_mul(projectionMatrix, viewMatrix);
  modelViewProjectionMatrix =
      mio_mat4_mul(viewProjectionMatrix, modelWorldMatrix);
  if ((G_APP.render.flags3D & MIO_3D_CULL_FRUSTUM) &&
      mio_cull_mesh(mesh, modelViewProjectionMatrix)) {
    return;
  }
  for (i = 0; i < mesh->indexCount / 3; i++) {
    behind_near_plane_count = 0;
    for (j = 0; j < 3; j++) {
      transformedVerts[j] = mesh->vertices[mesh->indices[i * 3 + j]];
      transformedVerts[j].pos =
          mio_mat4_mul_vec4(modelViewMatrix, transformedVerts[j].pos);
      if (transformedVerts[j].pos.z > 0.0f) { behind_near_plane_count++; }
    }
    if (!ortho && behind_near_plane_count >= 3) { continue; }
    v1.x = transformedVerts[0].pos.x;
    v1.y = transformedVerts[0].pos.y;
    v1.z = transformedVerts[0].pos.z;
    v2.x = transformedVerts[1].pos.x;
    v2.y = transformedVerts[1].pos.y;
    v2.z = transformedVerts[1].pos.z;
    v3.x = transformedVerts[2].pos.x;
    v3.y = transformedVerts[2].pos.y;
    v3.z = transformedVerts[2].pos.z;
    normalVecFace = mio_vec3_cross(mio_vec3_sub(v2, v1), mio_vec3_sub(v3, v1));
    viewVec = mio_vec3(0.0f, 0.0f, 0.0f);
    if (ortho) {
      viewVec = mio_vec3(0.0f, 0.0f, 1.0f);
    } else {
      viewVec = mio_vec3_scale(v1, -1.0f);
    }
    if ((G_APP.render.flags3D & MIO_3D_CULL_BACKFACE) &&
        (mio_vec3_dot(normalVecFace, viewVec) < 0)) {
      continue;
    }
    if (G_APP.render.flags3D & MIO_3D_SHADE_FLAT) {
      p1_4 = mesh->vertices[mesh->indices[i * 3 + 0]].pos;
      p2_4 = mesh->vertices[mesh->indices[i * 3 + 1]].pos;
      p3_4 = mesh->vertices[mesh->indices[i * 3 + 2]].pos;
      p1.x = p1_4.x;
      p1.y = p1_4.y;
      p1.z = p1_4.z;
      p2.x = p2_4.x;
      p2.y = p2_4.y;
      p2.z = p2_4.z;
      p3.x = p3_4.x;
      p3.y = p3_4.y;
      p3.z = p3_4.z;
      faceNormal = mio_vec3_normalize(
          mio_vec3_cross(mio_vec3_sub(p2, p1), mio_vec3_sub(p3, p1)));
      normalVec4.x = faceNormal.x;
      normalVec4.y = faceNormal.y;
      normalVec4.z = faceNormal.z;
      normalVec4.w = 0.0f;
      transformedNormal4 = mio_mat4_mul_vec4(modelWorldMatrix, normalVec4);
      transformedNormal.x = transformedNormal4.x;
      transformedNormal.y = transformedNormal4.y;
      transformedNormal.z = transformedNormal4.z;
      normalizedTransformedNormal = mio_vec3_normalize(transformedNormal);
      sunDir =
          mio_vec3(G_APP.render.sunX, G_APP.render.sunY, G_APP.render.sunZ);
      lightIntensityFace = mio_vec3_dot(normalizedTransformedNormal, sunDir);
      lightIntensityFace = lightIntensityFace * 0.5f + 0.5f;
      lightIntensityFace = G_APP.render.ambient +
                           lightIntensityFace * (1.0f - G_APP.render.ambient);
      if (G_APP.render.flags3D & MIO_3D_NORMALS) {
        mio_set_colour(MIO_RGBA(
            (uint8)(normalizedTransformedNormal.x * 128.0f + 128),
            (uint8)(normalizedTransformedNormal.y * 128.0f + 128),
            (uint8)(normalizedTransformedNormal.z * 128.0f + 128), 255));
      } else {
        mio_set_colour(MIO_RGBA(
            (uint8)((real32)MIO_COL_GET_R(origCol) * lightIntensityFace),
            (uint8)((real32)MIO_COL_GET_G(origCol) * lightIntensityFace),
            (uint8)((real32)MIO_COL_GET_B(origCol) * lightIntensityFace),
            255));
      }
    }
    for (j = 0; j < 3; j++) {
      transformedVerts[j].pos =
          mio_mat4_mul_vec4(projectionMatrix, transformedVerts[j].pos);
    }
    clippedFace.count = mio_clip_polygon(
        transformedVerts, 3, (mioVertex *)clippedFace.vertices);
    if (clippedFace.count < 3) { continue; }
    if (G_APP.render.flags3D & MIO_3D_WIREFRAME) {
      for (j = 0; j < clippedFace.count; j++) {
        mio_3d_draw_vertex_line(
            ((mioVertex *)clippedFace.vertices)[j],
            ((mioVertex *)clippedFace.vertices)[(j + 1) % clippedFace.count]);
      }
    } else if (
        G_APP.render.flags3D & MIO_3D_SOLID ||
        G_APP.render.flags3D & MIO_3D_TEXTURE) {
      for (j = 0; j < clippedFace.count - 2; j++) {
        mio_3d_draw_vertex_triangle(
            ((mioVertex *)clippedFace.vertices)[0],
            ((mioVertex *)clippedFace.vertices)[j + 1],
            ((mioVertex *)clippedFace.vertices)[j + 2], texture, tW, tH,
            lightIntensityFace, ortho);
      }
    }
  }
  mio_set_colour(origCol);
}

int32 mio_3d_fog(real32 start, real32 end) {
  int32 x, y;
  real32 depth;
  real32 distance;
  real32 fog_factor;
  real32 fog_r = (real32)MIO_COL_GET_R(G_APP.colour);
  real32 fog_g = (real32)MIO_COL_GET_G(G_APP.colour);
  real32 fog_b = (real32)MIO_COL_GET_B(G_APP.colour);
  uint32 *pixels = G_APP.render.colourData;
  real32 *depths = G_APP.render.depthData;
  int32 width = G_APP.render.width;
  int32 height = G_APP.render.height;
  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++) {
      depth = depths[y * width + x];
      if (depth == 1.0f) { continue; }
      distance = 1.0f / (1.0f - depth);
      if (distance > end) {
        fog_factor = 1.0f;
      } else if (distance < start) {
        fog_factor = 0.0f;
      } else {
        fog_factor = (distance - start) / (end - start);
      }
      if (fog_factor > 0.0f) {
        uint32 old_color = pixels[y * width + x];
        real32 r = ((old_color >> 16) & 0xFF) / 255.0f;
        real32 g = ((old_color >> 8) & 0xFF) / 255.0f;
        real32 b = ((old_color >> 0) & 0xFF) / 255.0f;
        real32 new_r = (1.0f - fog_factor) * r + fog_factor * (fog_r / 255.0f);
        real32 new_g = (1.0f - fog_factor) * g + fog_factor * (fog_g / 255.0f);
        real32 new_b = (1.0f - fog_factor) * b + fog_factor * (fog_b / 255.0f);
        pixels[y * width + x] =
            MIO_RGBA(new_r * 255.0f, new_g * 255.0f, new_b * 255.0f, 255);
      }
    }
  }
  return TRUE;
}

int32 mio_draw_arrow_line(
    real32 x1, real32 y1, real32 x2, real32 y2, real32 arrow_screen_len) {
  real32 dx = x2 - x1;
  real32 dy = y2 - y1;
  real32 len = sqrt(dx * dx + dy * dy);
  real32 arrow_angle = MIO_DEG_TO_RAD(30.0f);
  real32 rot_x1, rot_y1, rot_x2, rot_y2;
  mio_draw_line(x1, y1, x2, y2);
  if (len > 0) {
    dx /= len;
    dy /= len;
    rot_x1 = dx * cos(arrow_angle) - dy * sin(arrow_angle);
    rot_y1 = dx * sin(arrow_angle) + dy * cos(arrow_angle);
    rot_x2 = dx * cos(-arrow_angle) - dy * sin(-arrow_angle);
    rot_y2 = dx * sin(-arrow_angle) + dy * cos(-arrow_angle);
    mio_draw_line(
        x2, y2, x2 - rot_x1 * arrow_screen_len,
        y2 - rot_y1 * arrow_screen_len);
    mio_draw_line(
        x2, y2, x2 - rot_x2 * arrow_screen_len,
        y2 - rot_y2 * arrow_screen_len);
  }
  return TRUE;
}

int32 mio_3d_draw_grid(
    mioCamera *cam, int32 axis_step, int32 grid_size, real32 max_dist,
    uint32 color, uint32 follow_camera, real32 near_fade_dist,
    real32 near_steepness, real32 far_steepness, real32 min_alpha,
    real32 max_alpha, uint32 draw_axes, mioMat4 transformMatrix) {
  int32 i;
  mioMat4 viewProjectionMatrix;
  mioMat4 camera_rot;
  mioMat4 camera_pos;
  mioMat4 modelMatrix;
  mioMat4 compensationMatrix;
  mioVec4 p1_4, p2_4;
  real32 w, h, screen_x1, screen_y1, screen_x2, screen_y2;
  real32 t;
  uint8 r, g, b, a;
  real32 alpha_factor, scaled_alpha;
  real32 dist;
  mioVec3 cam_pos_3d;
  mioMat4 modelViewProjectionMatrix;
  real32 grid_offset_x, grid_offset_z;
  real32 line_pos_x, line_pos_z;
  real32 j;
  mioVec3 seg_start_3d;
  mioVec3 seg_end_3d;
  mioVec3 seg_mid_3d;
  real32 axis_len, arrow_screen_len;
  mioVec3 origin_3d, x_axis_3d, y_axis_3d, z_axis_3d;
  mioVec3 offset_vec;
  real32 ndcX1, ndcY1, ndcX2, ndcY2;

  const real32 CLIP_EPSILON = MIO_EPSILON_F * 100.0f;
  const real32 NDC_CLIP_MAX = 10.0f;

  w = cam->width;
  h = cam->height;
  grid_offset_x = 0.0f;
  grid_offset_z = 0.0f;

  if (follow_camera) {
    grid_offset_x = (real32)floor((real64)cam->pos.x / (real64)axis_step) *
                    (real32)axis_step;
    grid_offset_z = (real32)floor((real64)cam->pos.z / (real64)axis_step) *
                    (real32)axis_step;

    offset_vec = mio_vec3(grid_offset_x, 0.0f, grid_offset_z);
    compensationMatrix = mio_mat4_translate(offset_vec);

    cam_pos_3d = mio_vec3_sub(cam->pos, offset_vec);
  } else {
    compensationMatrix = mio_mat4_identity();
    cam_pos_3d = mio_vec3(cam->pos.x, cam->pos.y, cam->pos.z);
  }

  camera_rot = mio_mat4_mul(mio_mat4_rotate_y(-cam->yaw), mio_mat4_identity());
  camera_rot = mio_mat4_mul(mio_mat4_rotate_x(-cam->pitch), camera_rot);
  camera_pos = mio_mat4_translate(mio_vec3_scale(cam->pos, -1.0f));
  viewProjectionMatrix =
      mio_mat4_mul(cam->projection, mio_mat4_mul(camera_rot, camera_pos));

  modelMatrix = transformMatrix;
  if (follow_camera) {
    modelMatrix = mio_mat4_mul(compensationMatrix, modelMatrix);
  }
  modelViewProjectionMatrix = mio_mat4_mul(viewProjectionMatrix, modelMatrix);

  r = (uint8)MIO_COL_GET_R(color);
  g = (uint8)MIO_COL_GET_G(color);
  b = (uint8)MIO_COL_GET_B(color);

  for (i = -grid_size * axis_step; i <= grid_size * axis_step;
       i += axis_step) {
    line_pos_x = (real32)i;
    for (j = -grid_size * axis_step; j < grid_size * axis_step;
         j += axis_step) {
      seg_start_3d = mio_vec3(line_pos_x, 0.0f, j);
      seg_end_3d = mio_vec3(line_pos_x, 0.0f, j + (real32)axis_step);

      seg_mid_3d = mio_vec3(
          (seg_start_3d.x + seg_end_3d.x) / 2.0f,
          (seg_start_3d.y + seg_end_3d.y) / 2.0f,
          (seg_start_3d.z + seg_end_3d.z) / 2.0f);

      dist = mio_vec3_distance(cam_pos_3d, seg_mid_3d);

      if (dist < near_fade_dist) {
        alpha_factor = (real32)pow(dist / near_fade_dist, near_steepness);
        scaled_alpha = (alpha_factor * (max_alpha - min_alpha)) + min_alpha;
      } else {
        alpha_factor =
            1.0f - (real32)pow(
                       ((dist - near_fade_dist) / (max_dist - near_fade_dist)),
                       far_steepness);
        scaled_alpha = alpha_factor * max_alpha;
      }
      scaled_alpha = MIO_CLAMP(scaled_alpha, 0.0f, 1.0f);
      a = (uint8)(scaled_alpha * 255.0f);
      mio_set_colour(MIO_RGBA(r, g, b, a));

      p1_4 = mio_mat4_mul_vec4(
          modelViewProjectionMatrix,
          mio_vec4(seg_start_3d.x, seg_start_3d.y, seg_start_3d.z, 1.0f));
      p2_4 = mio_mat4_mul_vec4(
          modelViewProjectionMatrix,
          mio_vec4(seg_end_3d.x, seg_end_3d.y, seg_end_3d.z, 1.0f));

      {
        uint32 in1 = p1_4.w > CLIP_EPSILON;
        uint32 in2 = p2_4.w > CLIP_EPSILON;

        if (!in1 && !in2) {
          continue;
        } else if (!in1 && in2) {
          t = (CLIP_EPSILON - p1_4.w) / (p2_4.w - p1_4.w);
          p1_4 = mio_vec4_lerp(p1_4, p2_4, t);
        } else if (in1 && !in2) {
          t = (CLIP_EPSILON - p2_4.w) / (p1_4.w - p2_4.w);
          p2_4 = mio_vec4_lerp(p2_4, p1_4, t);
        }
      }

      ndcX1 = p1_4.x / p1_4.w;
      ndcY1 = p1_4.y / p1_4.w;
      ndcX2 = p2_4.x / p2_4.w;
      ndcY2 = p2_4.y / p2_4.w;

      if (MIO_ABS(ndcX1) > NDC_CLIP_MAX && MIO_ABS(ndcX2) > NDC_CLIP_MAX &&
          MIO_ABS(ndcY1) > NDC_CLIP_MAX && MIO_ABS(ndcY2) > NDC_CLIP_MAX) {
        continue;
      }

      screen_x1 = (ndcX1 + 1.0f) * 0.5f * w;
      screen_y1 = (1.0f - (ndcY1 + 1.0f) * 0.5f) * h;
      screen_x2 = (ndcX2 + 1.0f) * 0.5f * w;
      screen_y2 = (1.0f - (ndcY2 + 1.0f) * 0.5f) * h;
      mio_draw_line(screen_x1, screen_y1, screen_x2, screen_y2);
    }
  }

  for (i = -grid_size * axis_step; i <= grid_size * axis_step;
       i += axis_step) {
    line_pos_z = (real32)i;
    for (j = -grid_size * axis_step; j < grid_size * axis_step;
         j += axis_step) {
      seg_start_3d = mio_vec3(j, 0.0f, line_pos_z);
      seg_end_3d = mio_vec3(j + (real32)axis_step, 0.0f, line_pos_z);

      seg_mid_3d = mio_vec3(
          (seg_start_3d.x + seg_end_3d.x) / 2.0f,
          (seg_start_3d.y + seg_end_3d.y) / 2.0f,
          (seg_start_3d.z + seg_end_3d.z) / 2.0f);

      dist = mio_vec3_distance(cam_pos_3d, seg_mid_3d);

      if (dist < near_fade_dist) {
        alpha_factor = (real32)pow(dist / near_fade_dist, near_steepness);
        scaled_alpha = (alpha_factor * (max_alpha - min_alpha)) + min_alpha;
      } else {
        alpha_factor =
            1.0f - (real32)pow(
                       ((dist - near_fade_dist) / (max_dist - near_fade_dist)),
                       far_steepness);
        scaled_alpha = alpha_factor * max_alpha;
      }
      scaled_alpha = MIO_CLAMP(scaled_alpha, 0.0f, 1.0f);
      a = (uint8)(scaled_alpha * 255.0f);
      mio_set_colour(MIO_RGBA(r, g, b, a));

      p1_4 = mio_mat4_mul_vec4(
          modelViewProjectionMatrix,
          mio_vec4(seg_start_3d.x, seg_start_3d.y, seg_start_3d.z, 1.0f));
      p2_4 = mio_mat4_mul_vec4(
          modelViewProjectionMatrix,
          mio_vec4(seg_end_3d.x, seg_end_3d.y, seg_end_3d.z, 1.0f));

      {
        uint32 in1 = p1_4.w > CLIP_EPSILON;
        uint32 in2 = p2_4.w > CLIP_EPSILON;

        if (!in1 && !in2) {
          continue;
        } else if (!in1 && in2) {
          t = (CLIP_EPSILON - p1_4.w) / (p2_4.w - p1_4.w);
          p1_4 = mio_vec4_lerp(p1_4, p2_4, t);
        } else if (in1 && !in2) {
          t = (CLIP_EPSILON - p2_4.w) / (p1_4.w - p2_4.w);
          p2_4 = mio_vec4_lerp(p2_4, p1_4, t);
        }
      }

      ndcX1 = p1_4.x / p1_4.w;
      ndcY1 = p1_4.y / p1_4.w;
      ndcX2 = p2_4.x / p2_4.w;
      ndcY2 = p2_4.y / p2_4.w;

      if (MIO_ABS(ndcX1) > NDC_CLIP_MAX && MIO_ABS(ndcX2) > NDC_CLIP_MAX &&
          MIO_ABS(ndcY1) > NDC_CLIP_MAX && MIO_ABS(ndcY2) > NDC_CLIP_MAX) {
        continue;
      }

      screen_x1 = (ndcX1 + 1.0f) * 0.5f * w;
      screen_y1 = (1.0f - (ndcY1 + 1.0f) * 0.5f) * h;
      screen_x2 = (ndcX2 + 1.0f) * 0.5f * w;
      screen_y2 = (1.0f - (ndcY2 + 1.0f) * 0.5f) * h;
      mio_draw_line(screen_x1, screen_y1, screen_x2, screen_y2);
    }
  }

  if (draw_axes) {
    real32 axes_coords[3][2][3];

    axis_len = 5.0f;
    arrow_screen_len = 10.0f;
    origin_3d = mio_vec3(0.0f, 0.0f, 0.0f);
    x_axis_3d = mio_vec3(axis_len, 0.0f, 0.0f);
    y_axis_3d = mio_vec3(0.0f, axis_len, 0.0f);
    z_axis_3d = mio_vec3(0.0f, 0.0f, axis_len);

    axes_coords[0][0][0] = origin_3d.x;
    axes_coords[0][0][1] = origin_3d.y;
    axes_coords[0][0][2] = origin_3d.z;
    axes_coords[0][1][0] = x_axis_3d.x;
    axes_coords[0][1][1] = x_axis_3d.y;
    axes_coords[0][1][2] = x_axis_3d.z;

    axes_coords[1][0][0] = origin_3d.x;
    axes_coords[1][0][1] = origin_3d.y;
    axes_coords[1][0][2] = origin_3d.z;
    axes_coords[1][1][0] = y_axis_3d.x;
    axes_coords[1][1][1] = y_axis_3d.y;
    axes_coords[1][1][2] = y_axis_3d.z;

    axes_coords[2][0][0] = origin_3d.x;
    axes_coords[2][0][1] = origin_3d.y;
    axes_coords[2][0][2] = origin_3d.z;
    axes_coords[2][1][0] = z_axis_3d.x;
    axes_coords[2][1][1] = z_axis_3d.y;
    axes_coords[2][1][2] = z_axis_3d.z;

    for (i = 0; i < 3; i++) {
      uint32 color_axis = MIO_RGBA(255, 0, 0, (uint8)(max_alpha * 255.0f));
      if (i == 1) {
        color_axis = MIO_RGBA(0, 255, 0, (uint8)(max_alpha * 255.0f));
      }
      if (i == 2) {
        color_axis = MIO_RGBA(255, 255, 0, (uint8)(max_alpha * 255.0f));
      }
      mio_set_colour(color_axis);

      p1_4 = mio_mat4_mul_vec4(
          modelViewProjectionMatrix,
          mio_vec4(
              axes_coords[i][0][0], axes_coords[i][0][1], axes_coords[i][0][2],
              1.0f));
      p2_4 = mio_mat4_mul_vec4(
          modelViewProjectionMatrix,
          mio_vec4(
              axes_coords[i][1][0], axes_coords[i][1][1], axes_coords[i][1][2],
              1.0f));

      {
        uint32 in1 = p1_4.w > CLIP_EPSILON;
        uint32 in2 = p2_4.w > CLIP_EPSILON;

        if (!in1 && !in2) {
          continue;
        } else if (!in1 && in2) {
          t = (CLIP_EPSILON - p1_4.w) / (p2_4.w - p1_4.w);
          p1_4 = mio_vec4_lerp(p1_4, p2_4, t);
        } else if (in1 && !in2) {
          t = (CLIP_EPSILON - p2_4.w) / (p1_4.w - p2_4.w);
          p2_4 = mio_vec4_lerp(p2_4, p1_4, t);
        }
      }

      ndcX1 = p1_4.x / p1_4.w;
      ndcY1 = p1_4.y / p1_4.w;
      ndcX2 = p2_4.x / p2_4.w;
      ndcY2 = p2_4.y / p2_4.w;

      if (MIO_ABS(ndcX1) > NDC_CLIP_MAX && MIO_ABS(ndcX2) > NDC_CLIP_MAX &&
          MIO_ABS(ndcY1) > NDC_CLIP_MAX && MIO_ABS(ndcY2) > NDC_CLIP_MAX) {
        continue;
      }

      screen_x1 = (ndcX1 + 1.0f) * 0.5f * w;
      screen_y1 = (1.0f - (ndcY1 + 1.0f) * 0.5f) * h;
      screen_x2 = (ndcX2 + 1.0f) * 0.5f * w;
      screen_y2 = (1.0f - (ndcY2 + 1.0f) * 0.5f) * h;

      mio_draw_arrow_line(
          screen_x1, screen_y1, screen_x2, screen_y2, arrow_screen_len);
    }
  }
  return TRUE;
}

int32 mio_3d_draw_vector(
    mioCamera *cam, mioVec3 pos, mioVec3 orient, real32 scale, uint32 color,
    uint32 draw_arrow, real32 near_fade_dist, real32 near_steepness,
    real32 far_steepness, real32 min_alpha, real32 max_alpha) {
  real32 w = cam->width;
  real32 h = cam->height;
  real32 screen_x1, screen_y1, screen_x2, screen_y2;
  real32 dist, alpha_factor, scaled_alpha;
  uint8 r, g, b, a;
  mioMat4 viewProjectionMatrix;
  mioMat4 camera_rot, camera_pos;
  mioVec4 p1_4, p2_4;
  mioVec3 end_point_3d;
  real32 near_w, t;
  camera_rot = mio_mat4_mul(mio_mat4_rotate_y(-cam->yaw), mio_mat4_identity());
  camera_rot = mio_mat4_mul(mio_mat4_rotate_x(-cam->pitch), camera_rot);
  camera_pos = mio_mat4_translate(mio_vec3_scale(cam->pos, -1.0f));
  viewProjectionMatrix =
      mio_mat4_mul(cam->projection, mio_mat4_mul(camera_rot, camera_pos));
  end_point_3d = mio_vec3_add(pos, mio_vec3_scale(orient, scale));
  dist = mio_vec3_distance(cam->pos, pos);
  if (dist < near_fade_dist) {
    alpha_factor = (real32)pow(dist / near_fade_dist, near_steepness);
    scaled_alpha = (alpha_factor * (max_alpha - min_alpha)) + min_alpha;
  } else {
    alpha_factor =
        1.0f - (real32)pow(
                   ((dist - near_fade_dist) / (100.0f - near_fade_dist)),
                   far_steepness);
    scaled_alpha = alpha_factor * max_alpha;
  }
  scaled_alpha = MIO_CLAMP(scaled_alpha, 0.0f, 1.0f);
  a = (uint8)(scaled_alpha * 255.0f);
  r = (uint8)MIO_COL_GET_R(color);
  g = (uint8)MIO_COL_GET_G(color);
  b = (uint8)MIO_COL_GET_B(color);
  mio_set_colour(MIO_RGBA(r, g, b, a));
  p1_4 = mio_mat4_mul_vec4(
      viewProjectionMatrix, mio_vec4(pos.x, pos.y, pos.z, 1.0f));
  p2_4 = mio_mat4_mul_vec4(
      viewProjectionMatrix,
      mio_vec4(end_point_3d.x, end_point_3d.y, end_point_3d.z, 1.0f));
  near_w = cam->nearPlane;
  t = 0.0f;
  if (p1_4.w <= 0.0f && p2_4.w > 0.0f) {
    t = (near_w - p1_4.w) / (p2_4.w - p1_4.w);
    p1_4 = mio_vec4_lerp(p1_4, p2_4, t);
  } else if (p2_4.w <= 0.0f && p1_4.w > 0.0f) {
    t = (near_w - p2_4.w) / (p1_4.w - p2_4.w);
    p2_4 = mio_vec4_lerp(p2_4, p1_4, t);
  } else if (p1_4.w <= 0.0f && p2_4.w <= 0.0f) {
    return TRUE;
  }
  screen_x1 = (p1_4.x / p1_4.w + 1.0f) * 0.5f * w;
  screen_y1 = (1.0f - (p1_4.y / p1_4.w + 1.0f) * 0.5f) * h;
  screen_x2 = (p2_4.x / p2_4.w + 1.0f) * 0.5f * w;
  screen_y2 = (1.0f - (p2_4.y / p2_4.w + 1.0f) * 0.5f) * h;
  if (draw_arrow) {
    mio_draw_arrow_line(screen_x1, screen_y1, screen_x2, screen_y2, 10.0f);
  } else {
    mio_draw_line(screen_x1, screen_y1, screen_x2, screen_y2);
  }
  return TRUE;
}

/* @NOISE ********************************************************************/

MIO_GLOBAL int64 G_NOISE_PERM[512] = {
    151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,
    225, 140, 36,  103, 30,  69,  142, 8,   99,  37,  240, 21,  10,  23,  190,
    6,   148, 247, 120, 234, 75,  0,   26,  197, 62,  94,  252, 219, 203, 117,
    35,  11,  32,  57,  177, 33,  88,  237, 149, 56,  87,  174, 20,  125, 136,
    171, 168, 68,  175, 74,  165, 71,  134, 139, 48,  27,  166, 77,  146, 158,
    231, 83,  111, 229, 122, 60,  211, 133, 230, 220, 105, 92,  41,  55,  46,
    245, 40,  244, 102, 143, 54,  65,  25,  63,  161, 1,   216, 80,  73,  209,
    76,  132, 187, 208, 89,  18,  169, 200, 196, 135, 130, 116, 188, 159, 86,
    164, 100, 109, 198, 173, 186, 3,   64,  52,  217, 226, 250, 124, 123, 5,
    202, 38,  147, 118, 126, 255, 82,  85,  212, 207, 206, 59,  227, 47,  16,
    58,  17,  182, 189, 28,  42,  223, 183, 170, 213, 119, 248, 152, 2,   44,
    154, 163, 70,  221, 153, 101, 155, 167, 43,  172, 9,   129, 22,  39,  253,
    19,  98,  108, 110, 79,  113, 224, 232, 178, 185, 112, 104, 218, 246, 97,
    228, 251, 34,  242, 193, 238, 210, 144, 12,  191, 179, 162, 241, 81,  51,
    145, 235, 249, 14,  239, 107, 49,  192, 214, 31,  181, 199, 106, 157, 184,
    84,  204, 176, 115, 121, 50,  45,  127, 4,   150, 254, 138, 236, 205, 93,
    222, 114, 67,  29,  24,  72,  243, 141, 128, 195, 78,  66,  215, 61,  156,
    180, 151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233,
    7,   225, 140, 36,  103, 30,  69,  142, 8,   99,  37,  240, 21,  10,  23,
    190, 6,   148, 247, 120, 234, 75,  0,   26,  197, 62,  94,  252, 219, 203,
    117, 35,  11,  32,  57,  177, 33,  88,  237, 149, 56,  87,  174, 20,  125,
    136, 171, 168, 68,  175, 74,  165, 71,  134, 139, 48,  27,  166, 77,  146,
    158, 231, 83,  111, 229, 122, 60,  211, 133, 230, 220, 105, 92,  41,  55,
    46,  245, 40,  244, 102, 143, 54,  65,  25,  63,  161, 1,   216, 80,  73,
    209, 76,  132, 187, 208, 89,  18,  169, 200, 196, 135, 130, 116, 188, 159,
    86,  164, 100, 109, 198, 173, 186, 3,   64,  52,  217, 226, 250, 124, 123,
    5,   202, 38,  147, 118, 126, 255, 82,  85,  212, 207, 206, 59,  227, 47,
    16,  58,  17,  182, 189, 28,  42,  223, 183, 170, 213, 119, 248, 152, 2,
    44,  154, 163, 70,  221, 153, 101, 155, 167, 43,  172, 9,   129, 22,  39,
    253, 19,  98,  108, 110, 79,  113, 224, 232, 178, 185, 112, 104, 218, 246,
    97,  228, 251, 34,  242, 193, 238, 210, 144, 12,  191, 179, 162, 241, 81,
    51,  145, 235, 249, 14,  239, 107, 49,  192, 214, 31,  181, 199, 106, 157,
    184, 84,  204, 176, 115, 121, 50,  45,  127, 4,   150, 254, 138, 236, 205,
    93,  222, 114, 67,  29,  24,  72,  243, 141, 128, 195, 78,  66,  215, 61,
    156, 180};

MIO_GLOBAL int64 G_NOISE_GRAD[12][3] = {
    {1, 1, 0},  {-1, 1, 0},  {1, -1, 0}, {-1, -1, 0}, {1, 0, 1},  {-1, 0, 1},
    {1, 0, -1}, {-1, 0, -1}, {0, 1, 1},  {0, -1, 1},  {0, 1, -1}, {0, -1, -1}};

MIO_GLOBAL uint32 g_randCounter = 0;

uint32 MIO_RAND16(uint32 range) {
  uint32 ret = G_NOISE_PERM[g_randCounter % 512];
  g_randCounter += G_NOISE_PERM[ret % 512];
  ret += G_NOISE_PERM[g_randCounter % 512];
  g_randCounter += G_NOISE_PERM[ret % 512];
  ret = G_NOISE_PERM[g_randCounter % 512];
  g_randCounter += G_NOISE_PERM[ret % 512];
  ret += G_NOISE_PERM[g_randCounter % 512];
  g_randCounter += G_NOISE_PERM[ret % 512];
  return ret % range;
}

typedef real64 (*PFNOISEPROC2D)(real64 x, real64 y);
typedef real64 (*PFNOISEPROC3D)(real64 x, real64 y, real64 z);

#define MIO_SQRT_3 1.732050807568877
#define MIO_SIMPLEX_F2 (0.5 * (MIO_SQRT_3 - 1.0))
#define MIO_SIMPLEX_G2 ((3.0 - MIO_SQRT_3) / 6.0)
#define MIO_SIMPLEX_F3 (1.0 / 3.0)
#define MIO_SIMPLEX_G3 (1.0 / 6.0)
#define MIO_SIMPLEX_G2x2 (MIO_SIMPLEX_G2 * 2.0)
#define MIO_SIMPLEX_G3x2 (MIO_SIMPLEX_G3 * 2.0)
#define MIO_SIMPLEX_G3x3 (MIO_SIMPLEX_G3 * 3.0)

real32 mio_noise_simplex_normalize(real64 n) {
  return (real32)((n * (1.0f / 0.34543f)) * 0.5f + 0.5f);
}

real64 mio_noise_simplex_2d(real64 xin, real64 yin) {
  real64 n0, n1, n2, x1, y1, x2, y2, t0, t1, t2;
  int64 i1, j1, ii, jj, gi0, gi1, gi2;
  real64 s = (xin + yin) * MIO_SIMPLEX_F2;
  int64 i = MIO_FLOOR(xin + s);
  int64 j = MIO_FLOOR(yin + s);
  real64 t = (real64)(i + j) * MIO_SIMPLEX_G2;
  real64 xd0 = (real64)i - t;
  real64 yd0 = (real64)j - t;
  real64 x0 = xin - xd0;
  real64 y0 = yin - yd0;
  if (x0 > y0) {
    i1 = 1;
    j1 = 0;
  } else {
    i1 = 0;
    j1 = 1;
  }
  x1 = x0 - i1 + MIO_SIMPLEX_G2;
  y1 = y0 - j1 + MIO_SIMPLEX_G2;
  x2 = x0 - 1.0 + MIO_SIMPLEX_G2x2;
  y2 = y0 - 1.0 + MIO_SIMPLEX_G2x2;
  ii = i & 255;
  jj = j & 255;
  gi0 = G_NOISE_PERM[ii + G_NOISE_PERM[jj]] % 12;
  gi1 = G_NOISE_PERM[ii + i1 + G_NOISE_PERM[jj + j1]] % 12;
  gi2 = G_NOISE_PERM[ii + 1 + G_NOISE_PERM[jj + 1]] % 12;
  t0 = 0.5 - x0 * x0 - y0 * y0;
  t1 = 0.5 - x1 * x1 - y1 * y1;
  t2 = 0.5 - x2 * x2 - y2 * y2;
  if (t0 < 0.0) {
    n0 = 0.0;
  } else {
    t0 *= t0;
    n0 = t0 * t0 * MIO_DOT2(G_NOISE_GRAD[gi0], x0, y0);
  }
  if (t1 < 0.0) {
    n1 = 0.0;
  } else {
    t1 *= t1;
    n1 = t1 * t1 * MIO_DOT2(G_NOISE_GRAD[gi1], x1, y1);
  }
  if (t2 < 0) {
    n2 = 0.0;
  } else {
    t2 *= t2;
    n2 = t2 * t2 * MIO_DOT2(G_NOISE_GRAD[gi2], x2, y2);
  }
  return MIO_MIN(MIO_MAX(((35.0729517397) * (n0 + n1 + n2)), -0.5), 0.5);
}

real64 mio_noise_simplex_3d(real64 xin, real64 yin, real64 zin) {
  real64 n0, n1, n2, n3, t0, t1, t2, t3;
  real64 x1, y1, z1, x2, y2, z2, x3, y3, z3;
  int64 i1, j1, k1;
  int64 i2, j2, k2;
  int64 ii, jj, kk, gi0, gi1, gi2, gi3;
  real64 xf = xin;
  real64 yf = yin;
  real64 zf = zin;
  real64 s = (real64)(xf + yf + zf) * MIO_SIMPLEX_F3;
  int64 i = MIO_FLOOR(xf + s);
  int64 j = MIO_FLOOR(yf + s);
  int64 k = MIO_FLOOR(zf + s);
  real64 t = (real64)(i + j + k) * MIO_SIMPLEX_G3;
  real64 xd0 = (real64)i - t;
  real64 yd0 = (real64)j - t;
  real64 zd0 = (real64)k - t;
  real64 x0 = xf - xd0;
  real64 y0 = yf - yd0;
  real64 z0 = zf - zd0;
  if (x0 >= y0) {
    if (y0 >= z0) {
      i1 = 1;
      j1 = 0;
      k1 = 0;
      i2 = 1;
      j2 = 1;
      k2 = 0;
    } else if (x0 >= z0) {
      i1 = 1;
      j1 = 0;
      k1 = 0;
      i2 = 1;
      j2 = 0;
      k2 = 1;
    } else {
      i1 = 0;
      j1 = 0;
      k1 = 1;
      i2 = 1;
      j2 = 0;
      k2 = 1;
    }
  } else {
    if (y0 < z0) {
      i1 = 0;
      j1 = 0;
      k1 = 1;
      i2 = 0;
      j2 = 1;
      k2 = 1;
    } else if (x0 < z0) {
      i1 = 0;
      j1 = 1;
      k1 = 0;
      i2 = 0;
      j2 = 1;
      k2 = 1;
    } else {
      i1 = 0;
      j1 = 1;
      k1 = 0;
      i2 = 1;
      j2 = 1;
      k2 = 0;
    }
  }
  x1 = x0 - (real64)i1 + MIO_SIMPLEX_G3;
  y1 = y0 - (real64)j1 + MIO_SIMPLEX_G3;
  z1 = z0 - (real64)k1 + MIO_SIMPLEX_G3;
  x2 = x0 - (real64)i2 + MIO_SIMPLEX_G3x2;
  y2 = y0 - (real64)j2 + MIO_SIMPLEX_G3x2;
  z2 = z0 - (real64)k2 + MIO_SIMPLEX_G3x2;
  x3 = x0 - 1.0 + MIO_SIMPLEX_G3x3;
  y3 = y0 - 1.0 + MIO_SIMPLEX_G3x3;
  z3 = z0 - 1.0 + MIO_SIMPLEX_G3x3;
  ii = i & 255;
  jj = j & 255;
  kk = k & 255;
  gi0 = G_NOISE_PERM[ii + G_NOISE_PERM[jj + G_NOISE_PERM[kk]]] % 12;
  gi1 = G_NOISE_PERM[ii + i1 + G_NOISE_PERM[jj + j1 + G_NOISE_PERM[kk + k1]]] %
        12;
  gi2 = G_NOISE_PERM[ii + i2 + G_NOISE_PERM[jj + j2 + G_NOISE_PERM[kk + k2]]] %
        12;
  gi3 =
      G_NOISE_PERM[ii + 1 + G_NOISE_PERM[jj + 1 + G_NOISE_PERM[kk + 1]]] % 12;
  t0 = 0.5 - x0 * x0 - y0 * y0 - z0 * z0;
  if (t0 < 0.0) {
    n0 = 0.0;
  } else {
    t0 *= t0;
    n0 = t0 * t0 * MIO_DOT3(G_NOISE_GRAD[gi0], x0, y0, z0);
  }
  t1 = 0.5 - x1 * x1 - y1 * y1 - z1 * z1;
  if (t1 < 0.0) {
    n1 = 0.0;
  } else {
    t1 *= t1;
    n1 = t1 * t1 * MIO_DOT3(G_NOISE_GRAD[gi1], x1, y1, z1);
  }
  t2 = 0.5 - x2 * x2 - y2 * y2 - z2 * z2;
  if (t2 < 0.0) {
    n2 = 0.0;
  } else {
    t2 *= t2;
    n2 = t2 * t2 * MIO_DOT3(G_NOISE_GRAD[gi2], x2, y2, z2);
  }
  t3 = 0.5 - x3 * x3 - y3 * y3 - z3 * z3;
  if (t3 < 0.0) {
    n3 = 0.0;
  } else {
    t3 *= t3;
    n3 = t3 * t3 * MIO_DOT3(G_NOISE_GRAD[gi3], x3, y3, z3);
  }
  return MIO_MIN(MIO_MAX(((38.440839548) * (n0 + n1 + n2 + n3)), -0.5), 0.5);
}

#define M1 1597334677U /* 1719413*929 */
#define M2 3812015801U /* 140473*2467*11 */

real64 mio_noise_hash_fast(uint32 x, uint32 y) {
  x *= M1;
  y *= M2;
  return (real64)((x ^ y) * M1) * (1.0 / (real64)0xffffffffU);
}

real64 mio_noise_value_2d(real64 x, real64 y) {
  real64 ix = (real64)MIO_FLOOR(x);
  real64 iy = (real64)MIO_FLOOR(y);
  real64 fx = (x - ix);
  real64 fy = (y - iy);
  real64 a = mio_noise_hash_fast(ix, iy);
  real64 b = mio_noise_hash_fast(ix + 1.0, iy);
  real64 c = mio_noise_hash_fast(ix, iy + 1.0);
  real64 d = mio_noise_hash_fast(ix + 1.0, iy + 1.0);
  real64 ux = (fx * fx * (3.0 - 2.0 * fx));
  real64 uy = (fy * fy * (3.0 - 2.0 * fy));
  return MIO_LERP(a, b, ux) + (c - a) * uy * (1.0 - ux) + (d - b) * ux * uy;
}

real64 mio_noise_octave_2d(
    PFNOISEPROC2D noiseProc, real64 x, real64 y, real64 scale,
    real64 persistance, int64 octaves) {
  real64 ret;
  real64 fx, fy;
  real64 freq = 1.0;
  real64 pers = 1.0;
  real64 acc = 0.0;
  int32 i;
  ret = 0.0;
  for (i = 0; i < octaves; i++) {
    fx = (x * scale) * freq;
    fy = (y * scale) * freq;
    ret += noiseProc(fx, fy) * pers;
    acc += pers;
    freq *= 2.0;
    pers *= persistance;
  }
  return ret / acc;
}

real32 mio_noise_octave_3d(
    PFNOISEPROC3D noiseProc, real64 x, real64 y, real64 z, real64 scale,
    real64 persistance, int32 octaves) {
  real64 ret;
  real64 fx, fy, fz;
  real64 freq = 1.0;
  real64 pers = 1.0;
  real64 acc = 0.0;
  int32 i;
  ret = 0.0;
  for (i = 0; i < octaves; i++) {
    fx = (x * scale) * freq;
    fy = (y * scale) * freq;
    fz = (z * scale) * freq;
    ret += noiseProc(fx, fy, fz) * pers;
    acc += pers;
    freq *= 2.0;
    pers *= persistance;
  }
  return ret / acc;
}

real32 mio_noise_domain_warp_2d(
    PFNOISEPROC2D proc, real32 x, real32 y, real32 dx, real32 dy, real32 warp,
    real32 scale, real32 pers, int32 noiseOctaves, int32 warpOctaves) {
  real64 dwx = mio_noise_octave_2d(proc, x, y, scale, pers, warpOctaves);
  real64 dwy =
      mio_noise_octave_2d(proc, x + dx, y + dy, scale, pers, warpOctaves);
  return (real32)mio_noise_octave_2d(
      proc, x + warp * dwx, y + warp * dwy, scale, pers, noiseOctaves);
}

real32 mio_noise_domain_warp_3d(
    PFNOISEPROC3D proc, real32 x, real32 y, real32 z, real32 dx, real32 dy,
    real32 dz, real32 warp, real32 scale, real32 pers, int32 octaves) {
  real64 dwx = mio_noise_octave_3d(proc, x, y, z, scale, pers, octaves);
  real64 dwy =
      mio_noise_octave_3d(proc, x + dx, y + dy, z, scale, pers, octaves);
  return (real32)mio_noise_octave_3d(
      proc, x + warp * dwx, y + warp * dwy, z, scale, pers, octaves);
}

/* @PHYSICS ******************************************************************/

/* @EXTRA ********************************************************************/

MIO_GLOBAL mioVec3 G_3D_START_HIT;

int32 mio_draw_heightmap(
    int32 ix, int32 iy, int32 width, int32 height, real32 *values) {
  int32 x, y;
  int32 index;
  real32 val;
  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++) {
      index = (x) + (height - y) * width;
      val = values[index] * 0.5f + 0.5f;
      mio_set_colour(MIO_RGBA(val * 255, val * 255, val * 255, 255));
      if (x + ix < G_APP.render.width && x + ix >= 0 &&
          y + iy < G_APP.render.height && y + iy >= 0) {
        MIO_DRAW_PIXEL(x + ix, y + iy);
      }
    }
  }
  return TRUE;
}

static mioVec3 mio_calculate_translation_hit(
    mioCamera *cam, mioVec3 rayOrigin, mioVec3 rayDir, mioVec3 planeNormal,
    mioVec3 planePoint) {
  real32 t;
  real32 denom = mio_vec3_dot(rayDir, planeNormal);
  real32 planeD = -mio_vec3_dot(planeNormal, planePoint);
  mioVec3 currentHitPosWorld = mio_vec3(0.0f, 0.0f, 0.0f);

  if (fabs(denom) > MIO_EPSILON_F) {
    t = -(mio_vec3_dot(rayOrigin, planeNormal) + planeD) / denom;

    if (t > 0.0f) {
      currentHitPosWorld = mio_vec3_add(rayOrigin, mio_vec3_scale(rayDir, t));
    }
  } else {
    if (fabs(planeNormal.y) < 0.1f) {
      mioVec3 fallbackNormal = mio_vec3(0.0f, 1.0f, 0.0f);
      real32 fallbackD = -mio_vec3_dot(fallbackNormal, G_3D_START_HIT);
      real32 fallbackDenom = mio_vec3_dot(rayDir, fallbackNormal);

      if (fabs(fallbackDenom) > MIO_EPSILON_F) {
        t = -(mio_vec3_dot(rayOrigin, fallbackNormal) + fallbackD) /
            fallbackDenom;
        if (t > 0.0f) {
          currentHitPosWorld =
              mio_vec3_add(rayOrigin, mio_vec3_scale(rayDir, t));
        }
      }
    }
  }
  return currentHitPosWorld;
}

static void
mio_get_mouse_ray(mioCamera *cam, mioVec3 *rayOrigin, mioVec3 *rayDir) {
  mioMat4 camera_rot = mio_mat4_identity();
  mioMat4 camera_pos = mio_mat4_identity();
  mioMat4 viewMatrix;
  mioMat4 projectionMatrix;
  mioSystemPoint mousePos;
  mioVec3 nearPoint, farPoint;

  camera_rot = mio_mat4_mul(mio_mat4_rotate_y(-cam->yaw), camera_rot);
  camera_rot = mio_mat4_mul(mio_mat4_rotate_x(-cam->pitch), camera_rot);
  camera_pos = mio_mat4_translate(mio_vec3_scale(cam->pos, -1.0f));
  viewMatrix = mio_mat4_mul(camera_rot, camera_pos);

  projectionMatrix = cam->projection;
  mousePos = sys_mouse_position();

  mousePos.y = (int32)cam->height - mousePos.y;

  nearPoint = mio_vec3((real32)mousePos.x, (real32)mousePos.y, 0.0f);
  farPoint = mio_vec3((real32)mousePos.x, (real32)mousePos.y, 1.0f);

  *rayOrigin = mio_unproject(
      nearPoint, viewMatrix, projectionMatrix, 0.0f, 0.0f, cam->width,
      cam->height);
  farPoint = mio_unproject(
      farPoint, viewMatrix, projectionMatrix, 0.0f, 0.0f, cam->width,
      cam->height);

  *rayDir = mio_vec3_normalize(mio_vec3_sub(farPoint, *rayOrigin));
}

static int32 mio_ray_intersect_triangle(
    mioVec3 orig, mioVec3 dir, mioVec3 v0, mioVec3 v1, mioVec3 v2,
    real32 *t_out) {
  mioVec3 edge1, edge2, tvec, pvec, qvec;
  real32 det, inv_det, t, u, v;

  edge1 = mio_vec3_sub(v1, v0);
  edge2 = mio_vec3_sub(v2, v0);
  pvec = mio_vec3_cross(dir, edge2);
  det = mio_vec3_dot(edge1, pvec);

  if (det > -MIO_EPSILON_F && det < MIO_EPSILON_F) { return 0; }
  inv_det = 1.0f / det;
  tvec = mio_vec3_sub(orig, v0);
  u = mio_vec3_dot(tvec, pvec) * inv_det;
  if (u < 0.0f || u > 1.0f) { return 0; }
  qvec = mio_vec3_cross(tvec, edge1);
  v = mio_vec3_dot(dir, qvec) * inv_det;
  if (v < 0.0f || u + v > 1.0f) { return 0; }
  t = mio_vec3_dot(edge2, qvec) * inv_det;
  if (t > MIO_EPSILON_F) {
    *t_out = t;
    return 1;
  }
  return 0;
}

static int32 mio_ray_intersect_sphere(
    mioVec3 orig, mioVec3 dir, mioVec3 center, real32 radius, real32 *t_out) {
  mioVec3 oc = mio_vec3_sub(orig, center);
  real32 a = mio_vec3_dot(dir, dir);
  real32 b = 2.0f * mio_vec3_dot(oc, dir);
  real32 c = mio_vec3_dot(oc, oc) - radius * radius;
  real32 discriminant = b * b - 4 * a * c;

  if (discriminant < 0) {
    return 0;
  } else {
    real32 sqrt_disc = (real32)sqrt(discriminant);
    real32 t1 = (-b - sqrt_disc) / (2.0f * a);
    real32 t2 = (-b + sqrt_disc) / (2.0f * a);

    if (t1 > MIO_EPSILON_F) {
      *t_out = t1;
      return 1;
    }
    if (t2 > MIO_EPSILON_F) {
      *t_out = t2;
      return 1;
    }
    return 0;
  }
}

static real32 mio_ray_intersect_model(
    mioVec3 rayOrigin, mioVec3 rayDir, mioMat4 modelMat, mioMesh *mesh) {
  real32 t = 0.0f;
  real32 closestT = MIO_REAL32_MAX;
  int32 i;

  for (i = 0; i < mesh->indexCount; i += 3) {
    mioVec3 v0, v1, v2;
    v0 = mio_mat4_mul_point(
        modelMat, mio_vec3(
                      mesh->vertices[mesh->indices[i + 0]].pos.x,
                      mesh->vertices[mesh->indices[i + 0]].pos.y,
                      mesh->vertices[mesh->indices[i + 0]].pos.z));
    v1 = mio_mat4_mul_point(
        modelMat, mio_vec3(
                      mesh->vertices[mesh->indices[i + 1]].pos.x,
                      mesh->vertices[mesh->indices[i + 1]].pos.y,
                      mesh->vertices[mesh->indices[i + 1]].pos.z));
    v2 = mio_mat4_mul_point(
        modelMat, mio_vec3(
                      mesh->vertices[mesh->indices[i + 2]].pos.x,
                      mesh->vertices[mesh->indices[i + 2]].pos.y,
                      mesh->vertices[mesh->indices[i + 2]].pos.z));

    if (mio_ray_intersect_triangle(rayOrigin, rayDir, v0, v1, v2, &t)) {
      if (t < closestT) { closestT = t; }
    }
  }
  return closestT;
}

static mioVec3 get_cursor_world_intersection(
    mioCamera *cam, mioMesh *meshes, mioMat4 *mats, int32 count) {
  int32 i;
  real32 minT = MIO_REAL32_MAX;
  mioVec3 rayOrigin, rayDir;
  real32 t = 0.0f;
  real32 closestT = MIO_REAL32_MAX;
  mioVec3 hitPos;

  mio_get_mouse_ray(cam, &rayOrigin, &rayDir);
  for (i = 0; i < count; i++) {
    closestT = mio_ray_intersect_model(rayOrigin, rayDir, mats[i], &meshes[i]);
    minT = MIO_MIN(minT, closestT);
  }
  if (closestT < MIO_REAL32_MAX) {
    hitPos = mio_vec3_add(rayOrigin, mio_vec3_scale(rayDir, minT));
    return hitPos;
  }
  if (fabs(rayDir.y) > MIO_EPSILON_F) {
    t = -rayOrigin.y / rayDir.y;
    if (t > 0) {
      hitPos = mio_vec3_add(rayOrigin, mio_vec3_scale(rayDir, t));
      return hitPos;
    }
  }

  return mio_vec3(0.0f, -100.0f, 0.0f);
}

MIO_GLOBAL mioMesh MIO_DEFAULT_SPHERE_MESH;
MIO_GLOBAL mioMesh MIO_DEFAULT_CUBE_MESH;
MIO_GLOBAL mioMesh MIO_DEFAULT_TETRA_MESH;
MIO_GLOBAL mioMesh MIO_DEFAULT_QUAD_MESH;
MIO_GLOBAL mioMesh MIO_DEFAULT_TRIANGLE_MESH;
MIO_GLOBAL mioMesh MIO_DEFAULT_SPHERE_MESH_LOW;

static int32 mio_3d_meshes_init(void) {
  MIO_DEFAULT_CUBE_MESH = mio_create_cube_mesh();
  MIO_DEFAULT_SPHERE_MESH = mio_create_sphere_mesh(1.0f, 12, 12);
  MIO_DEFAULT_TETRA_MESH = mio_create_sphere_mesh(1.0f, 12, 12);
  MIO_DEFAULT_SPHERE_MESH_LOW = mio_create_sphere_mesh(1.0f, 6, 6);
  MIO_DEFAULT_QUAD_MESH = mio_create_plane_mesh(1.0f, 1.0f);
  MIO_DEFAULT_TRIANGLE_MESH = mio_create_cube_mesh();
  return TRUE;
}

static int32 mio_draw_sphere_at_cursor_xz(
    mioCamera *cam, mioMesh *meshes, mioMat4 *mats, int32 count, real32 radius,
    uint32 color) {
  mioVec3 cursorWorldPos;
  mioMat4 sphereMat;
  mioVec3 scale;
  cursorWorldPos = get_cursor_world_intersection(cam, meshes, mats, count);
  scale = mio_vec3(radius, radius, radius);

  sphereMat = mio_mat4_mul(
      mio_mat4_translate(mio_vec3(
          cursorWorldPos.x, cursorWorldPos.y + radius, cursorWorldPos.z)),
      mio_mat4_scale(scale));

  mio_set_colour(color);
  mio_3d_draw_mesh(
      cam, &MIO_DEFAULT_SPHERE_MESH, sphereMat, NULL, 0, 0, FALSE);
  return 1;
}

/* @GIZMO ********************************************************************/

#define GIZMO_MODE_TRANSLATE 0
#define GIZMO_MODE_ROTATE 1
#define GIZMO_MODE_SCALE 2
#define GIZMO_AXIS_NONE 0
#define GIZMO_AXIS_X 1
#define GIZMO_AXIS_Y 2
#define GIZMO_AXIS_Z 3
#define GIZMO_AXIS_CENTER 4

#define MIO_MAX_MESHES 64

typedef struct {
  mioVec3 *pos;
  mioQuat *quat;
  mioVec3 *scale;
  int32 mode;
  uint32 isTargeted;
} mioGizmoTarget;

typedef struct {
  real32 pickingScale;
  real32 visualLength;
  real32 handleThickness;
  real32 handleSphereRadius;
  real32 rotationRadius;
  real32 ringThickness;
  real32 handleTolerance;
  real32 innerRadiusSq;
  real32 outerRadiusSq;
} mioGizmoParams;

typedef struct {
  mioVec3 dragStartPos;
  mioQuat dragStartQuat;
  mioVec3 dragStartScale;
  mioVec3 dragStartHit;
  mioVec3 dragPlaneNormal;
  real32 dragStartProjDist;

  int32 isDragging;
  int32 activeAxis;
  int32 isFirstFrame;
  mioGizmoTarget target;

  mioMesh cubeMesh;
  mioMesh sphereMesh;
  mioGizmoParams params;
  uint32 isInitialized;
} mioGizmo;

typedef struct {
  mioVec3 pos;
  mioQuat rot;
  mioVec3 scale;
} mioTransform;

typedef struct {
  int32 hitAxis;
  real32 hitT;
} GizmoHitResult;

static mioVec3
mio_apply_translation_snapping(mioVec3 position, real32 snapIncrement) {
  if (snapIncrement > 0.0f) {
    position.x = (real32)round(position.x / snapIncrement) * snapIncrement;
    position.y = (real32)round(position.y / snapIncrement) * snapIncrement;
    position.z = (real32)round(position.z / snapIncrement) * snapIncrement;
  }
  return position;
}

static int32 mio_pick_gizmo_handle_internal(
    mioGizmo *g, mioVec3 rayOrigin, mioVec3 rayDir, mioMat4 handleMat,
    mioMesh *mesh, int32 axisToCheck, real32 *gizmoHitT) {
  real32 t_hit;

  t_hit = mio_ray_intersect_model(rayOrigin, rayDir, handleMat, mesh);
  if (t_hit < MIO_REAL32_MAX && t_hit < *gizmoHitT) {
    *gizmoHitT = t_hit;
    g->activeAxis = axisToCheck;
    return 1;
  }
  return 0;
}

static void mio_handle_gizmo_selection_translate_scale(
    mioGizmo *g, mioVec3 localRayOrigin, mioVec3 localRayDir,
    real32 *gizmoHitT) {
  mioGizmoParams *p;
  mioVec3 axes[] = {
      mio_vec3(1.0f, 0.0f, 0.0f), mio_vec3(0.0f, 1.0f, 0.0f),
      mio_vec3(0.0f, 0.0f, 1.0f)};
  int32 gizmoAxes[] = {GIZMO_AXIS_X, GIZMO_AXIS_Y, GIZMO_AXIS_Z};
  int32 i;
  mioMat4 centerMat;
  mioMat4 handleMat;
  mioVec3 localEndPos;
  mioMesh *meshToUse;
  real32 handleSize;

  p = &g->params;

  if (g->target.mode == GIZMO_MODE_TRANSLATE) {
    centerMat = mio_mat4_scale(mio_vec3(
        p->handleSphereRadius * 1.5f, p->handleSphereRadius * 1.5f,
        p->handleSphereRadius * 1.5f));
    mio_pick_gizmo_handle_internal(
        g, localRayOrigin, localRayDir, centerMat, &g->sphereMesh,
        GIZMO_AXIS_CENTER, gizmoHitT);
  }

  for (i = 0; i < 3; i++) {
    localEndPos = mio_vec3_scale(axes[i], p->visualLength);
    meshToUse = (g->target.mode == GIZMO_MODE_TRANSLATE) ? &g->sphereMesh
                                                         : &g->cubeMesh;
    handleSize = (g->target.mode == GIZMO_MODE_TRANSLATE)
                     ? p->handleSphereRadius
                     : p->handleThickness;

    handleMat = mio_mat4_translate(localEndPos);
    if (g->target.mode == GIZMO_MODE_SCALE) {
      handleMat = mio_mat4_mul(handleMat, mio_quat_to_mat4(*g->target.quat));
    }
    handleMat = mio_mat4_mul(
        handleMat,
        mio_mat4_scale(mio_vec3(handleSize, handleSize, handleSize)));

    mio_pick_gizmo_handle_internal(
        g, localRayOrigin, localRayDir, handleMat, meshToUse, gizmoAxes[i],
        gizmoHitT);
  }
}

static void mio_handle_gizmo_selection_rotate(
    mioGizmo *g, mioCamera *cam, mioVec3 rayOrigin, mioVec3 rayDir,
    mioQuat rotQuat, real32 *gizmoHitT) {
  mioGizmoParams *p;
  mioVec3 axes[] = {
      mio_vec3(1.0f, 0.0f, 0.0f), mio_vec3(0.0f, 1.0f, 0.0f),
      mio_vec3(0.0f, 0.0f, 1.0f)};
  int32 gizmoAxes[] = {GIZMO_AXIS_X, GIZMO_AXIS_Y, GIZMO_AXIS_Z};
  int32 i;
  real32 handleDotRadius;
  real32 handleAngleOffsets[] = {
      MIO_DEG_TO_RAD(-45.0f), MIO_DEG_TO_RAD(45.0f), MIO_DEG_TO_RAD(45.0f)};
  mioVec3 base;
  mioVec3 localHandleRotated;
  mioVec3 worldHandlePos;
  real32 t_sphere;
  mioVec3 localAxis;
  mioMat4 startRotMat;
  mioMat4 startGizmoTransform;
  mioVec3 worldAxis;
  mioVec3 intersection;
  real32 distSq;
  real32 currentT;

  p = &g->params;
  handleDotRadius = p->handleSphereRadius * 1.2f;
  for (i = 0; i < 3; i++) {
    base = (mio_vec3_equal(axes[i], mio_vec3(1.0f, 0.0f, 0.0f), MIO_EPSILON_F))
               ? mio_vec3(0.0f, p->rotationRadius, 0.0f)
               : mio_vec3(p->rotationRadius, 0.0f, 0.0f);
    localHandleRotated = mio_quat_rotate_vec3(
        mio_quat_axis_angle(axes[i], handleAngleOffsets[i]), base);

    worldHandlePos = mio_vec3_add(
        *g->target.pos, mio_quat_rotate_vec3(rotQuat, localHandleRotated));
    t_sphere = 0.0f;

    if (mio_ray_intersect_sphere(
            rayOrigin, rayDir, worldHandlePos, handleDotRadius, &t_sphere) &&
        t_sphere < *gizmoHitT) {
      *gizmoHitT = t_sphere;
      g->activeAxis = gizmoAxes[i];
    }
  }

  if (g->activeAxis == GIZMO_AXIS_NONE) {
    for (i = 0; i < 3; i++) {
      localAxis = axes[i];
      startRotMat = mio_quat_to_mat4(*g->target.quat);
      startGizmoTransform =
          mio_mat4_mul(mio_mat4_translate(*g->target.pos), startRotMat);
      worldAxis = mio_vec3_normalize(
          mio_mat4_mul_direction(startGizmoTransform, localAxis));
      intersection = mio_calculate_translation_hit(
          cam, rayOrigin, rayDir, worldAxis, *g->target.pos);

      if (mio_vec3_length_sq(intersection) > MIO_EPSILON_F) {
        distSq =
            mio_vec3_length_sq(mio_vec3_sub(intersection, *g->target.pos));

        if (distSq >= p->innerRadiusSq && distSq <= p->outerRadiusSq) {
          currentT = mio_vec3_distance(rayOrigin, intersection);
          if (currentT < *gizmoHitT) {
            *gizmoHitT = currentT;
            g->activeAxis = gizmoAxes[i];
          }
        }
      }
    }
  }
}

static mioVec3 mio_gizmo_get_axis_vec(int32 axis) {
  return (axis == GIZMO_AXIS_X)   ? mio_vec3(1.0f, 0.0f, 0.0f)
         : (axis == GIZMO_AXIS_Y) ? mio_vec3(0.0f, 1.0f, 0.0f)
         : (axis == GIZMO_AXIS_Z) ? mio_vec3(0.0f, 0.0f, 1.0f)
                                  : mio_vec3(0.0f, 0.0f, 0.0f);
}

static mioGizmoParams mio_gizmo_calc_params(real32 maxDim) {
  mioGizmoParams p;
  const real32 F = 0.333f, L_R = 1.5f, MIN_S = 0.5f, MIN_L_R = 2.0f;
  const real32 H_R = 0.2f, V_R = 0.8f, P_M = 1.5f;

  p.pickingScale = MIO_MAX(maxDim * F, MIN_S);
  p.visualLength =
      MIO_MAX(maxDim + L_R * p.pickingScale, MIN_L_R * p.pickingScale);
  p.handleThickness = p.pickingScale * H_R;
  p.rotationRadius = p.visualLength * 0.8f;
  p.ringThickness = p.handleThickness * V_R;
  p.handleTolerance = p.ringThickness * P_M;
  p.handleSphereRadius = p.handleThickness;
  p.innerRadiusSq = (p.rotationRadius - p.handleTolerance) *
                    (p.rotationRadius - p.handleTolerance);
  p.outerRadiusSq = (p.rotationRadius + p.handleTolerance) *
                    (p.rotationRadius + p.handleTolerance);
  return p;
}

static mioVec3 mio_calculate_drag_plane_normal(
    mioVec3 worldGizmoAxis, mioVec3 cameraPos, mioVec3 dragStartHit) {
  mioVec3 camToGiz;
  mioVec3 normal;

  camToGiz = mio_vec3_normalize(mio_vec3_sub(dragStartHit, cameraPos));
  normal = mio_vec3_cross(worldGizmoAxis, camToGiz);

  if (mio_vec3_length_sq(normal) < MIO_EPSILON_F) {
    normal = (fabs(worldGizmoAxis.y) < 0.9f)
                 ? mio_vec3_cross(worldGizmoAxis, mio_vec3(0.0f, 1.0f, 0.0f))
                 : mio_vec3_cross(worldGizmoAxis, mio_vec3(1.0f, 0.0f, 0.0f));
  }
  return mio_vec3_normalize(mio_vec3_cross(worldGizmoAxis, normal));
}

static void mio_gizmo_core_init(mioGizmo *g) {
  g->cubeMesh = MIO_DEFAULT_CUBE_MESH;
  g->sphereMesh = MIO_DEFAULT_SPHERE_MESH_LOW;
  g->params = mio_gizmo_calc_params(1.0f);
  g->isDragging = 0;
  g->activeAxis = GIZMO_AXIS_NONE;
  g->target.isTargeted = 0;
  g->isInitialized = 1;
}

static void mio_gizmo_setup_drag(
    mioGizmo *g, mioCamera *cam, mioVec3 rayOrigin, mioVec3 rayDir,
    real32 gizmoHitT) {
  mioVec3 hitPos;
  mioVec3 localAxis;
  mioVec3 worldAxis;
  mioVec3 toHit;
  mioVec3 projectedHit;
  mioVec3 localProjectedHit;
  mioMat4 startRotMat;
  mioMat4 startGizmoTransform;

  hitPos = mio_vec3_add(rayOrigin, mio_vec3_scale(rayDir, gizmoHitT));

  g->isDragging = 1;
  g->isFirstFrame = 1;
  g->dragStartHit = hitPos;
  g->dragStartPos = *g->target.pos;
  g->dragStartQuat = *g->target.quat;
  g->dragStartScale = *g->target.scale;

  localAxis = mio_gizmo_get_axis_vec(g->activeAxis);
  startRotMat = mio_quat_to_mat4(g->dragStartQuat);
  startGizmoTransform =
      mio_mat4_mul(mio_mat4_translate(g->dragStartPos), startRotMat);
  worldAxis = mio_vec3_normalize(
      mio_mat4_mul_direction(startGizmoTransform, localAxis));

  if (g->target.mode == GIZMO_MODE_TRANSLATE ||
      g->target.mode == GIZMO_MODE_SCALE) {
    g->dragPlaneNormal = (g->activeAxis != GIZMO_AXIS_CENTER)
                             ? mio_calculate_drag_plane_normal(
                                   worldAxis, cam->pos, g->dragStartHit)
                             : mio_vec3(0.0f, 1.0f, 0.0f);

    if (g->target.mode == GIZMO_MODE_SCALE) {
      g->dragStartProjDist = mio_vec3_dot(
          mio_vec3_sub(g->dragStartHit, g->dragStartPos), worldAxis);
      if (g->dragStartProjDist < MIO_EPSILON_F) {
        g->dragStartProjDist = MIO_EPSILON_F * 100.0f;
      }
    }
  } else {
    g->dragPlaneNormal = worldAxis;
    toHit = mio_vec3_sub(hitPos, *g->target.pos);
    projectedHit = mio_vec3_reject(toHit, worldAxis);
    localProjectedHit = mio_mat4_mul_direction(
        mio_mat4_remove_scale(mio_mat4_inverse(startGizmoTransform)),
        projectedHit);

    if (g->activeAxis == GIZMO_AXIS_X) {
      g->dragStartQuat = mio_quat_axis_angle(
          mio_vec3(1, 0, 0),
          (real32)atan2(localProjectedHit.z, localProjectedHit.y));
    } else if (g->activeAxis == GIZMO_AXIS_Y) {
      g->dragStartQuat = mio_quat_axis_angle(
          mio_vec3(0, 1, 0),
          (real32)atan2(localProjectedHit.x, localProjectedHit.z));
    } else if (g->activeAxis == GIZMO_AXIS_Z) {
      g->dragStartQuat = mio_quat_axis_angle(
          mio_vec3(0, 0, 1),
          (real32)atan2(localProjectedHit.y, localProjectedHit.x));
    }
    g->dragStartQuat = *g->target.quat;
  }
}

static real32 mio_gizmo_get_snap_increment(int32 mode) {
  real32 snapIncrement = 0.0f;

  snapIncrement =
      (sys_key_down(KEY_CONTROL))
          ? ((mode == GIZMO_MODE_ROTATE) ? MIO_DEG_TO_RAD(15.0f) : 1.0f)
          : 0.0f;
  return snapIncrement;
}

static void mio_gizmo_update_translate(
    mioGizmo *g, mioCamera *cam, mioVec3 rayOrigin, mioVec3 rayDir,
    mioMat4 startGizmoTransform, real32 snapIncrement) {
  mioVec3 localAxis;
  mioVec3 currentHitPosWorld;
  mioVec3 worldDelta;
  mioVec3 worldGizmoAxis;
  real32 projectionDistance;

  localAxis = mio_gizmo_get_axis_vec(g->activeAxis);
  currentHitPosWorld = mio_calculate_translation_hit(
      cam, rayOrigin, rayDir, g->dragPlaneNormal, g->dragStartHit);
  worldDelta = mio_vec3_sub(currentHitPosWorld, g->dragStartHit);

  if (g->activeAxis != GIZMO_AXIS_CENTER) {
    worldGizmoAxis = mio_vec3_normalize(
        mio_mat4_mul_direction(startGizmoTransform, localAxis));
    projectionDistance = mio_vec3_dot(worldDelta, worldGizmoAxis);
    *g->target.pos = mio_vec3_add(
        g->dragStartPos, mio_vec3_scale(worldGizmoAxis, projectionDistance));
  } else {
    g->target.pos->x = g->dragStartPos.x + worldDelta.x;
    g->target.pos->z = g->dragStartPos.z + worldDelta.z;
  }
  *g->target.pos =
      mio_apply_translation_snapping(*g->target.pos, snapIncrement);
}

static void mio_gizmo_update_rotate(
    mioGizmo *g, mioCamera *cam, mioVec3 rayOrigin, mioVec3 rayDir,
    mioMat4 startGizmoTransform, real32 snapIncrement) {
  mioVec3 localAxis;
  mioVec3 worldAxis;
  mioVec3 newHitPosWorld;
  mioVec3 startVector;
  mioVec3 currentVector;
  mioVec3 nStart;
  mioVec3 nCurrent;
  real32 angle;
  real32 sign;
  real32 angleDelta;

  localAxis = mio_gizmo_get_axis_vec(g->activeAxis);
  worldAxis = mio_vec3_normalize(
      mio_mat4_mul_direction(startGizmoTransform, localAxis));
  newHitPosWorld = mio_calculate_translation_hit(
      cam, rayOrigin, rayDir, g->dragPlaneNormal, g->dragStartPos);

  if (mio_vec3_length_sq(newHitPosWorld) > MIO_EPSILON_F) {
    startVector = mio_vec3_sub(g->dragStartHit, g->dragStartPos);
    currentVector = mio_vec3_sub(newHitPosWorld, g->dragStartPos);

    nStart = mio_vec3_normalize(mio_vec3_reject(startVector, worldAxis));
    nCurrent = mio_vec3_normalize(mio_vec3_reject(currentVector, worldAxis));

    if (mio_vec3_length_sq(nStart) > MIO_EPSILON_F &&
        mio_vec3_length_sq(nCurrent) > MIO_EPSILON_F) {
      angle =
          (real32)acos(MIO_CLAMP(mio_vec3_dot(nStart, nCurrent), -1.0f, 1.0f));
      sign = mio_vec3_dot(mio_vec3_cross(nStart, nCurrent), worldAxis);
      angleDelta = angle * mio_sign(sign);

      if (snapIncrement > 0.0f) {
        angleDelta = (real32)round(angleDelta / snapIncrement) * snapIncrement;
      }

      *g->target.quat = mio_quat_normalize(mio_quat_mul(
          mio_quat_axis_angle(worldAxis, angleDelta), g->dragStartQuat));
    }
  }
}

static void mio_gizmo_update_scale(
    mioGizmo *g, mioCamera *cam, mioVec3 rayOrigin, mioVec3 rayDir,
    mioMat4 startGizmoTransform, real32 snapIncrement) {
  mioVec3 localAxis;
  mioVec3 worldGizmoAxis;
  mioVec3 currentHitPosWorld;
  real32 currentProjDist;
  real32 scaleRatio;
  real32 originalScale;
  real32 newScale;
  real32 *targetScaleComponent;

  localAxis = mio_gizmo_get_axis_vec(g->activeAxis);
  worldGizmoAxis = mio_mat4_mul_direction(startGizmoTransform, localAxis);
  currentHitPosWorld = mio_calculate_translation_hit(
      cam, rayOrigin, rayDir, g->dragPlaneNormal, g->dragStartHit);

  if (mio_vec3_length_sq(currentHitPosWorld) > MIO_EPSILON_F &&
      g->dragStartProjDist > MIO_EPSILON_F) {
    currentProjDist = mio_vec3_dot(
                          mio_vec3_sub(currentHitPosWorld, g->dragStartHit),
                          worldGizmoAxis) +
                      g->dragStartProjDist;
    scaleRatio = currentProjDist / g->dragStartProjDist;
    targetScaleComponent = (real32 *)g->target.scale;

    if (g->activeAxis == GIZMO_AXIS_X) {
      originalScale = g->dragStartScale.x;
      targetScaleComponent = &g->target.scale->x;
    } else if (g->activeAxis == GIZMO_AXIS_Y) {
      originalScale = g->dragStartScale.y;
      targetScaleComponent = &g->target.scale->y;
    } else {
      originalScale = g->dragStartScale.z;
      targetScaleComponent = &g->target.scale->z;
    }

    if (targetScaleComponent) {
      newScale = originalScale * scaleRatio;
      if (snapIncrement > 0.0f) {
        newScale = (real32)round(newScale / snapIncrement) * snapIncrement;
      }
      *targetScaleComponent = MIO_MAX(newScale, 0.1f);
    }
  }
}

static void mio_gizmo_handle_drag(
    mioGizmo *g, mioCamera *cam, mioVec3 rayOrigin, mioVec3 rayDir,
    real64 dt) {
  real32 snapIncrement;
  mioMat4 startRotMat;
  mioMat4 startGizmoTransform;

  snapIncrement = mio_gizmo_get_snap_increment(g->target.mode);
  startRotMat = mio_quat_to_mat4(g->dragStartQuat);
  startGizmoTransform =
      mio_mat4_mul(mio_mat4_translate(g->dragStartPos), startRotMat);

  if (g->isFirstFrame) {
    g->isFirstFrame = 0;
    g->dragStartHit = mio_calculate_translation_hit(
        cam, rayOrigin, rayDir, g->dragPlaneNormal, g->dragStartPos);
    return;
  }

  if (g->target.mode == GIZMO_MODE_TRANSLATE) {
    mio_gizmo_update_translate(
        g, cam, rayOrigin, rayDir, startGizmoTransform, snapIncrement);
  } else if (g->target.mode == GIZMO_MODE_ROTATE) {
    mio_gizmo_update_rotate(
        g, cam, rayOrigin, rayDir, startGizmoTransform, snapIncrement);
  } else if (g->target.mode == GIZMO_MODE_SCALE) {
    mio_gizmo_update_scale(
        g, cam, rayOrigin, rayDir, startGizmoTransform, snapIncrement);
  }
}

static void mio_gizmo_draw_translate_scale(
    mioGizmo *g, mioCamera *cam, mioVec3 pos, mioQuat rotQuat,
    mioMat4 rotMat) {
  mioGizmoParams *p;
  int32 i;
  mioVec3 axes[] = {
      mio_vec3(1.0f, 0.0f, 0.0f), mio_vec3(0.0f, 1.0f, 0.0f),
      mio_vec3(0.0f, 0.0f, 1.0f)};
  uint32 colors[] = {
      MIO_RGBA(255, 0, 0, 255), MIO_RGBA(0, 255, 0, 255),
      MIO_RGBA(0, 0, 255, 255)};
  int32 gizmoAxes[] = {GIZMO_AXIS_X, GIZMO_AXIS_Y, GIZMO_AXIS_Z};
  mioMat4 currentGizmoTransform;
  int32 mode;
  mioVec3 localEndPos;
  mioVec3 worldEndPos;
  uint32 drawColor;
  mioMat4 endMat;

  p = &g->params;
  currentGizmoTransform = mio_mat4_mul(mio_mat4_translate(pos), rotMat);
  mode = g->target.mode;

  for (i = 0; i < 3; i++) {
    localEndPos = mio_vec3_scale(axes[i], p->visualLength);
    worldEndPos = mio_mat4_mul_point(currentGizmoTransform, localEndPos);
    drawColor = (g->activeAxis == gizmoAxes[i]) ? MIO_RGBA(255, 255, 0, 255)
                                                : colors[i];

    mio_3d_draw_vector(
        cam, pos, mio_vec3_sub(worldEndPos, pos), 1.0f, drawColor, FALSE, 0.0f,
        1.0f, 1.0f, 1.0f, 1.0f);

    mio_set_colour(drawColor);
    if (mode == GIZMO_MODE_TRANSLATE) {
      endMat = mio_mat4_mul(
          mio_mat4_translate(worldEndPos),
          mio_mat4_scale(mio_vec3(
              p->handleSphereRadius, p->handleSphereRadius,
              p->handleSphereRadius)));
      mio_3d_draw_mesh(cam, &g->sphereMesh, endMat, NULL, 0, 0, FALSE);
    } else {
      endMat = mio_mat4_mul(
          mio_mat4_translate(worldEndPos),
          mio_mat4_mul(
              rotMat, mio_mat4_scale(mio_vec3(
                          p->handleThickness, p->handleThickness,
                          p->handleThickness))));
      mio_3d_draw_mesh(cam, &g->cubeMesh, endMat, NULL, 0, 0, FALSE);
    }
  }

  if (mode == GIZMO_MODE_TRANSLATE) {
    uint32 centerColor;
    mioMat4 centerMat;

    centerColor = (g->activeAxis == GIZMO_AXIS_CENTER)
                      ? MIO_RGBA(255, 255, 0, 255)
                      : MIO_RGBA(255, 255, 255, 255);
    mio_set_colour(centerColor);
    centerMat = mio_mat4_mul(
        mio_mat4_translate(pos),
        mio_mat4_scale(mio_vec3(
            p->handleSphereRadius * 1.5f, p->handleSphereRadius * 1.5f,
            p->handleSphereRadius * 1.5f)));
    mio_3d_draw_mesh(cam, &g->sphereMesh, centerMat, NULL, 0, 0, FALSE);
  }
}

static void mio_gizmo_draw_rotation_ring(
    mioGizmo *g, mioCamera *cam, mioMat4 transform, mioVec3 axis,
    uint32 color) {
  mioGizmoParams *p;
  int32 segments = 24;
  real32 angleStep;
  int32 j;
  real32 notchLen;
  real32 innerRadius;
  mioVec3 baseVector;
  mioVec3 rotated;
  mioVec3 nextRotated;
  mioVec3 start;
  mioVec3 end;
  real32 notchAngle;
  mioVec3 baseVectorInner;
  mioVec3 baseVectorOuter;
  mioVec3 rotatedInner;
  mioVec3 rotatedOuter;
  mioVec3 worldStart;
  mioVec3 worldEnd;

  p = &g->params;
  angleStep = MIO_TAU / segments;
  notchLen = p->ringThickness * 0.4f;
  innerRadius = p->rotationRadius - p->ringThickness * 0.5f;

  baseVector =
      (mio_vec3_equal(axis, mio_vec3(1.0f, 0.0f, 0.0f), MIO_EPSILON_F))
          ? mio_vec3(0.0f, p->rotationRadius, 0.0f)
          : mio_vec3(p->rotationRadius, 0.0f, 0.0f);

  mio_set_colour(color);
  for (j = 0; j < segments; j++) {
    rotated = mio_quat_rotate_vec3(
        mio_quat_axis_angle(axis, j * angleStep), baseVector);
    nextRotated = mio_quat_rotate_vec3(
        mio_quat_axis_angle(axis, (j + 1) * angleStep), baseVector);

    start = mio_mat4_mul_point(transform, rotated);
    end = mio_mat4_mul_point(transform, nextRotated);

    mio_3d_draw_vector(
        cam, start, mio_vec3_sub(end, start), 1.0f, color, FALSE, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f);
  }

  mio_set_colour(MIO_RGBA(255, 255, 255, 255));
  for (j = 0; j < segments; j++) {
    notchAngle = MIO_DEG_TO_RAD(j * (360.0f / (real32)segments));

    if (mio_vec3_equal(axis, mio_vec3(1.0f, 0.0f, 0.0f), MIO_EPSILON_F)) {
      baseVectorInner = mio_vec3(0.0f, innerRadius, 0.0f);
      baseVectorOuter = mio_vec3(0.0f, innerRadius + notchLen, 0.0f);
    } else {
      baseVectorInner = mio_vec3(innerRadius, 0.0f, 0.0f);
      baseVectorOuter = mio_vec3(innerRadius + notchLen, 0.0f, 0.0f);
    }

    rotatedInner = mio_quat_rotate_vec3(
        mio_quat_axis_angle(axis, notchAngle), baseVectorInner);
    rotatedOuter = mio_quat_rotate_vec3(
        mio_quat_axis_angle(axis, notchAngle), baseVectorOuter);
    worldStart = mio_mat4_mul_point(transform, rotatedInner);
    worldEnd = mio_mat4_mul_point(transform, rotatedOuter);

    mio_3d_draw_vector(
        cam, worldStart, mio_vec3_sub(worldEnd, worldStart), 1.0f, color,
        FALSE, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f);
  }
}

static void mio_gizmo_draw_rotate(
    mioGizmo *g, mioCamera *cam, mioVec3 pos, mioQuat rotQuat,
    mioMat4 rotMat) {
  mioGizmoParams *p;
  int32 i;
  mioMat4 currentGizmoTransform;
  real32 handleDotRadius;

  mioVec3 axes[] = {
      mio_vec3(1.0f, 0.0f, 0.0f), mio_vec3(0.0f, 1.0f, 0.0f),
      mio_vec3(0.0f, 0.0f, 1.0f)};
  uint32 colors[] = {
      MIO_RGBA(255, 0, 0, 255), MIO_RGBA(0, 255, 0, 255),
      MIO_RGBA(0, 0, 255, 255)};
  int32 gizmoAxes[] = {GIZMO_AXIS_X, GIZMO_AXIS_Y, GIZMO_AXIS_Z};
  real32 handleAngleOffsets[] = {
      MIO_DEG_TO_RAD(-45.0f), MIO_DEG_TO_RAD(45.0f), MIO_DEG_TO_RAD(45.0f)};
  uint32 color;
  mioVec3 base;
  mioVec3 localHandleRotated;
  mioVec3 worldHandlePos;
  mioMat4 handleMat;

  p = &g->params;
  currentGizmoTransform = mio_mat4_mul(mio_mat4_translate(pos), rotMat);
  handleDotRadius = p->handleSphereRadius * 1.2f;

  for (i = 0; i < 3; i++) {
    color = (g->activeAxis == gizmoAxes[i]) ? MIO_RGBA(255, 255, 0, 255)
                                            : colors[i];

    mio_gizmo_draw_rotation_ring(
        g, cam, currentGizmoTransform, axes[i], color);
    {
      base =
          (mio_vec3_equal(axes[i], mio_vec3(1.0f, 0.0f, 0.0f), MIO_EPSILON_F))
              ? mio_vec3(0.0f, p->rotationRadius, 0.0f)
              : mio_vec3(p->rotationRadius, 0.0f, 0.0f);
      localHandleRotated = mio_quat_rotate_vec3(
          mio_quat_axis_angle(axes[i], handleAngleOffsets[i]), base);

      worldHandlePos =
          mio_vec3_add(pos, mio_quat_rotate_vec3(rotQuat, localHandleRotated));
      handleMat = mio_mat4_mul(
          mio_mat4_translate(worldHandlePos),
          mio_mat4_scale(
              mio_vec3(handleDotRadius, handleDotRadius, handleDotRadius)));

      mio_set_colour(color);
      mio_3d_draw_mesh(cam, &g->sphereMesh, handleMat, NULL, 0, 0, FALSE);
    }
  }
}

static GizmoHitResult mio_check_gizmo_hit(
    mioGizmo *g, mioCamera *cam, mioVec3 rayOrigin, mioVec3 rayDir) {
  mioMat4 gizmoTransform;
  mioMat4 invGizmoTransform;
  mioVec3 localRayOrigin;
  mioVec3 localRayDir;
  real32 gizmoHitT = MIO_REAL32_MAX;
  GizmoHitResult result = {GIZMO_AXIS_NONE, MIO_REAL32_MAX};
  int32 axis_temp;

  if (!g->target.isTargeted) { return result; }

  gizmoTransform = mio_mat4_mul(
      mio_mat4_translate(*g->target.pos), mio_quat_to_mat4(*g->target.quat));
  invGizmoTransform = mio_mat4_inverse(gizmoTransform);
  localRayOrigin = mio_mat4_mul_point(invGizmoTransform, rayOrigin);
  localRayDir = mio_mat4_mul_direction(invGizmoTransform, rayDir);

  axis_temp = g->activeAxis;
  g->activeAxis = GIZMO_AXIS_NONE;

  if (g->target.mode == GIZMO_MODE_TRANSLATE ||
      g->target.mode == GIZMO_MODE_SCALE) {
    mio_handle_gizmo_selection_translate_scale(
        g, localRayOrigin, localRayDir, &gizmoHitT);
  } else if (g->target.mode == GIZMO_MODE_ROTATE) {
    mio_handle_gizmo_selection_rotate(
        g, cam, rayOrigin, rayDir, *g->target.quat, &gizmoHitT);
  }

  if (g->activeAxis != GIZMO_AXIS_NONE) {
    result.hitAxis = g->activeAxis;
    result.hitT = gizmoHitT;
  }

  g->activeAxis = axis_temp;
  return result;
}

static void mio_gizmo_logic_update(
    mioGizmo *g, mioCamera *cam, mioVec3 rayOrigin, mioVec3 rayDir,
    real64 dt) {
  mioMat4 gizmoTransform;
  mioMat4 invGizmoTransform;
  mioVec3 localRayOrigin;
  mioVec3 localRayDir;
  real32 gizmoHitT = MIO_REAL32_MAX;

  gizmoTransform = mio_mat4_mul(
      mio_mat4_translate(*g->target.pos), mio_quat_to_mat4(*g->target.quat));
  invGizmoTransform = mio_mat4_inverse(gizmoTransform);
  localRayOrigin = mio_mat4_mul_point(invGizmoTransform, rayOrigin);
  localRayDir = mio_mat4_mul_direction(invGizmoTransform, rayDir);

  if (sys_key_pressed(KEY_1)) {
    g->target.mode = GIZMO_MODE_TRANSLATE;
    g->activeAxis = GIZMO_AXIS_NONE;
  }
  if (sys_key_pressed(KEY_2)) {
    g->target.mode = GIZMO_MODE_ROTATE;
    g->activeAxis = GIZMO_AXIS_NONE;
  }
  if (sys_key_pressed(KEY_3)) {
    g->target.mode = GIZMO_MODE_SCALE;
    g->activeAxis = GIZMO_AXIS_NONE;
  }

  if (sys_mouse_pressed(0) && !g->isDragging) {
    g->activeAxis = GIZMO_AXIS_NONE;
    if (g->target.mode == GIZMO_MODE_TRANSLATE ||
        g->target.mode == GIZMO_MODE_SCALE) {
      mio_handle_gizmo_selection_translate_scale(
          g, localRayOrigin, localRayDir, &gizmoHitT);
    } else if (g->target.mode == GIZMO_MODE_ROTATE) {
      mio_handle_gizmo_selection_rotate(
          g, cam, rayOrigin, rayDir, *g->target.quat, &gizmoHitT);
    }

    if (g->activeAxis != GIZMO_AXIS_NONE) {
      mio_gizmo_setup_drag(g, cam, rayOrigin, rayDir, gizmoHitT);
    }
  } else if (sys_mouse_down(0) && g->isDragging) {
    mio_gizmo_handle_drag(g, cam, rayOrigin, rayDir, dt);
  } else if (sys_mouse_released(0) && g->isDragging) {
    g->isDragging = 0;
    g->isFirstFrame = 0;
    g->activeAxis = GIZMO_AXIS_NONE;
  }
}

static void mio_gizmo_do_draw(mioGizmo *g, mioCamera *cam) {
  mioVec3 pos;
  mioQuat rotQuat;
  mioMat4 rotMat;

  pos = *g->target.pos;
  rotQuat = *g->target.quat;
  rotMat = mio_quat_to_mat4(rotQuat);

  mio_3d_toggle_depth_test(FALSE);

  if (g->target.mode == GIZMO_MODE_TRANSLATE ||
      g->target.mode == GIZMO_MODE_SCALE) {
    mio_gizmo_draw_translate_scale(g, cam, pos, rotQuat, rotMat);
  } else if (g->target.mode == GIZMO_MODE_ROTATE) {
    mio_gizmo_draw_rotate(g, cam, pos, rotQuat, rotMat);
  }

  mio_set_colour(MIO_RGBA(255, 255, 255, 255));
  mio_3d_toggle_depth_test(TRUE);
}

void mio_gizmo_attach(
    mioGizmo *g, mioCamera *cam, mioTransform *target, int32 mode, real64 dt) {
  mioVec3 rayOrigin;
  mioVec3 rayDir;

  if (!g->isInitialized) { mio_gizmo_core_init(g); }

  g->target.pos = &target->pos;
  g->target.quat = &target->rot;
  g->target.scale = &target->scale;
  g->target.mode = mode;
  g->target.isTargeted = 1;

  mio_get_mouse_ray(cam, &rayOrigin, &rayDir);
  mio_gizmo_logic_update(g, cam, rayOrigin, rayDir, dt);
}

void mio_gizmo_render_if_attached(
    mioGizmo *g, mioCamera *cam, mioMesh *allMeshes, mioMat4 *allMats,
    uint32 meshCount) {
  if (g->target.isTargeted) {
    mio_gizmo_do_draw(g, cam);

    if (!g->isDragging && g->activeAxis == GIZMO_AXIS_NONE) {
      mio_draw_sphere_at_cursor_xz(
          cam, allMeshes, allMats, meshCount, 0.2f, 0xFFFFFFFF);
    }
  }
  g->target.isTargeted = 0;
}

int32 mio_select_object_from_world(
    mioCamera *cam, mioMesh *meshes, mioTransform *transforms, uint32 count,
    int32 currentSelection, int32 isGizmoDragging, int32 isGizmoHit) {
  mioVec3 rayOrigin;
  mioVec3 rayDir;
  real32 closestT = MIO_REAL32_MAX;
  int32 selectedIndex = 0;
  uint32 i;
  mioTransform *t;
  mioMat4 objMat;
  real32 currentT;

  if (!sys_mouse_pressed(0)) { return currentSelection; }

  if (isGizmoDragging || isGizmoHit) { return currentSelection; }

  mio_get_mouse_ray(cam, &rayOrigin, &rayDir);

  for (i = 0; i < count; i++) {
    t = &transforms[i];
    objMat = mio_mat4_mul(
        mio_mat4_translate(t->pos),
        mio_mat4_mul(mio_quat_to_mat4(t->rot), mio_mat4_scale(t->scale)));

    currentT = mio_ray_intersect_model(rayOrigin, rayDir, objMat, &meshes[i]);

    if (currentT < MIO_REAL32_MAX && currentT < closestT) {
      closestT = currentT;
      selectedIndex = (int32)i + 1;
    }
  }

  if (selectedIndex > 0) {
    return (currentSelection == selectedIndex) ? currentSelection
                                               : selectedIndex;
  } else {
    return 0;
  }
}

void mio_mesh_calculate_normals(mioMesh *mesh) {
  int32 i;
  mioVec3 v0, v1, v2, edge1, edge2, normal;

  for (i = 0; i < mesh->vertexCount; i++) {
    mesh->vertices[i].normal = mio_vec3(0.0f, 0.0f, 0.0f);
  }

  for (i = 0; i < mesh->indexCount; i += 3) {
    v0 = mio_vec3(
        mesh->vertices[mesh->indices[i]].pos.x,
        mesh->vertices[mesh->indices[i]].pos.y,
        mesh->vertices[mesh->indices[i]].pos.z);
    v1 = mio_vec3(
        mesh->vertices[mesh->indices[i + 1]].pos.x,
        mesh->vertices[mesh->indices[i + 1]].pos.y,
        mesh->vertices[mesh->indices[i + 1]].pos.z);
    v2 = mio_vec3(
        mesh->vertices[mesh->indices[i + 2]].pos.x,
        mesh->vertices[mesh->indices[i + 2]].pos.y,
        mesh->vertices[mesh->indices[i + 2]].pos.z);

    edge1 = mio_vec3_sub(v1, v0);
    edge2 = mio_vec3_sub(v2, v0);
    normal = mio_vec3_normalize(mio_vec3_cross(edge1, edge2));

    mesh->vertices[mesh->indices[i]].normal =
        mio_vec3_add(mesh->vertices[mesh->indices[i]].normal, normal);
    mesh->vertices[mesh->indices[i + 1]].normal =
        mio_vec3_add(mesh->vertices[mesh->indices[i + 1]].normal, normal);
    mesh->vertices[mesh->indices[i + 2]].normal =
        mio_vec3_add(mesh->vertices[mesh->indices[i + 2]].normal, normal);
  }

  for (i = 0; i < mesh->vertexCount; i++) {
    mesh->vertices[i].normal = mio_vec3_normalize(mesh->vertices[i].normal);
  }
}

int32 mio_project_to_screen(
    mioCamera *cam, mioMat4 mvp, mioVec3 worldPos, mioVec2 *screenPos) {
  mioVec4 clipPos = mio_mat4_mul_vec4(
      mvp, mio_vec4(worldPos.x, worldPos.y, worldPos.z, 1.0f));
  if (clipPos.w < cam->nearPlane) { return FALSE; }
  screenPos->x = (clipPos.x / clipPos.w + 1.0f) * 0.5f * cam->width;
  screenPos->y = (1.0f - (clipPos.y / clipPos.w + 1.0f) * 0.5f) * cam->height;
  return TRUE;
}

void mio_3d_draw_point(
    mioCamera *cam, mioMat4 mvp, mioVec3 worldPos, real32 size) {
  mioVec2 screenPos;
  if (mio_project_to_screen(cam, mvp, worldPos, &screenPos)) {
    mio_draw_circle_fill((int32)screenPos.x, (int32)screenPos.y, size);
  }
}

static int clip_line_axis(real32 numerator, real32 denominator, real32* t0, real32* t1) {
    real32 t;
    if (fabs(denominator) < MIO_EPSILON_F) {
        return numerator > 0;
    }

    t = numerator / denominator;

    if (denominator > 0) {
        if (t > *t1) return 1;
        *t0 = MIO_MAX(*t0, t);
    } else {
        if (t < *t0) return 1;
        *t1 = MIO_MIN(*t1, t);
    }

    if (*t0 > *t1) {
        return 1;
    }

    return 0;
}

void mio_3d_draw_line(mioCamera *cam, mioMat4 mvp, mioVec3 start, mioVec3 end, real32 t) {
    mioVec4 clipped_p1, clipped_p2;
    mioVec2 screenStart, screenEnd;
    mioVec4 p1 = mio_mat4_mul_vec4(mvp, mio_vec4(start.x, start.y, start.z, 1.0f));
    mioVec4 p2 = mio_mat4_mul_vec4(mvp, mio_vec4(end.x, end.y, end.z, 1.0f));
    real32 t0 = 0.0f;
    real32 t1 = 1.0f;
    mioVec4 dp = mio_vec4_sub(p2, p1);
    if (clip_line_axis(p1.x - p1.w, dp.w - dp.x, &t0, &t1)) return;
    if (clip_line_axis(-p1.x - p1.w, dp.x + dp.w, &t0, &t1)) return;
    if (clip_line_axis(p1.y - p1.w, dp.w - dp.y, &t0, &t1)) return;
    if (clip_line_axis(-p1.y - p1.w, dp.y + dp.w, &t0, &t1)) return;
    if (clip_line_axis(p1.z - p1.w, dp.w - dp.z, &t0, &t1)) return;
    if (clip_line_axis(-p1.z - p1.w, dp.z + dp.w, &t0, &t1)) return;
    clipped_p1 = mio_vec4_add(p1, mio_vec4_mul(dp, t0));
    clipped_p2 = mio_vec4_add(p1, mio_vec4_mul(dp, t1));
    screenStart.x = (clipped_p1.x / clipped_p1.w + 1.0f) * 0.5f * cam->width;
    screenStart.y = (1.0f - (clipped_p1.y / clipped_p1.w + 1.0f) * 0.5f) * cam->height;
    screenEnd.x = (clipped_p2.x / clipped_p2.w + 1.0f) * 0.5f * cam->width;
    screenEnd.y = (1.0f - (clipped_p2.y / clipped_p2.w + 1.0f) * 0.5f) * cam->height;
    mio_draw_line_thick(screenStart.x, screenStart.y, screenEnd.x, screenEnd.y, t, TRUE);
}

void mio_3d_draw_triangle_immediate(
    mioCamera *cam, mioMat4 mvp,
		mioVec3 v0_world, mioVec3 v1_world, mioVec3 v2_world) {
  mioVertex verts[3];
  mioVertex clipped_verts[MAX_CLIPPED_VERTS];
  int clipped_count;
  int j;

  verts[0].pos = mio_mat4_mul_vec4(
      mvp, mio_vec4(v0_world.x, v0_world.y, v0_world.z, 1.0f));
  verts[1].pos = mio_mat4_mul_vec4(
      mvp, mio_vec4(v1_world.x, v1_world.y, v1_world.z, 1.0f));
  verts[2].pos = mio_mat4_mul_vec4(
      mvp, mio_vec4(v2_world.x, v2_world.y, v2_world.z, 1.0f));

  verts[0].uv = mio_vec2(0, 0);
  verts[1].uv = mio_vec2(0, 0);
  verts[2].uv = mio_vec2(0, 0);
  verts[0].normal = mio_vec3(0, 1, 0);
  verts[1].normal = mio_vec3(0, 1, 0);
  verts[2].normal = mio_vec3(0, 1, 0);

  clipped_count = mio_clip_polygon(verts, 3, clipped_verts);

  if (clipped_count >= 3) {
    for (j = 0; j < clipped_count - 2; j++) {
      mio_3d_draw_vertex_triangle(
          clipped_verts[0], clipped_verts[j + 1], clipped_verts[j + 2], NULL,
          0, 0, 1.0f, FALSE);
    }
  }
}

void mio_3d_draw_triangle(
    mioCamera *cam, mioMat4 mvp, mioVec3 v0, mioVec3 v1, mioVec3 v2,
    int32 fill, real32 t) {
  if (fill) {
    mio_3d_draw_triangle_immediate(cam, mvp, v0, v1, v2);
  } else {
    mio_3d_draw_line(cam, mvp, v0, v1, t);
    mio_3d_draw_line(cam, mvp, v1, v2, t);
    mio_3d_draw_line(cam, mvp, v2, v0, t);
  }
}

void mio_3d_draw_quad_lines(
    mioCamera *cam, mioMat4 mvp, mioVec3 v0, mioVec3 v1, mioVec3 v2,
    mioVec3 v3, real32 t) {
  mio_3d_draw_line(cam, mvp, v0, v1, t);
  mio_3d_draw_line(cam, mvp, v1, v2, t);
  mio_3d_draw_line(cam, mvp, v2, v3, t);
  mio_3d_draw_line(cam, mvp, v3, v0, t);
}

void mio_3d_draw_cube_lines(
    mioCamera *cam, mioMat4 modelMatrix, real32 size, real32 t) {
  mioVec3 v[8];
  real32 s = size * 0.5f;
  mioMat4 mvp, camera_pos;
  mioMat4 viewProjectionMatrix;
  mioMat4 camera_rot =
      mio_mat4_mul(mio_mat4_rotate_y(-cam->yaw), mio_mat4_identity());
  camera_rot = mio_mat4_mul(mio_mat4_rotate_x(-cam->pitch), camera_rot);
  camera_pos = mio_mat4_translate(mio_vec3_scale(cam->pos, -1.0f));
  viewProjectionMatrix =
      mio_mat4_mul(cam->projection, mio_mat4_mul(camera_rot, camera_pos));
  mvp = mio_mat4_mul(viewProjectionMatrix, modelMatrix);

  v[0] = mio_vec3(-s, -s, -s);
  v[1] = mio_vec3(s, -s, -s);
  v[2] = mio_vec3(s, s, -s);
  v[3] = mio_vec3(-s, s, -s);
  v[4] = mio_vec3(-s, -s, s);
  v[5] = mio_vec3(s, -s, s);
  v[6] = mio_vec3(s, s, s);
  v[7] = mio_vec3(-s, s, s);

  mio_3d_draw_quad_lines(cam, mvp, v[0], v[1], v[2], v[3], t);
  mio_3d_draw_quad_lines(cam, mvp, v[4], v[5], v[6], v[7], t);
  mio_3d_draw_line(cam, mvp, v[0], v[4], t);
  mio_3d_draw_line(cam, mvp, v[1], v[5], t);
  mio_3d_draw_line(cam, mvp, v[2], v[6], t);
  mio_3d_draw_line(cam, mvp, v[3], v[7], t);
}

void mio_3d_draw_sphere_lines(
    mioCamera *cam, mioVec3 center, real32 radius, int32 segments, real32 t) {
  int32 i;
  real32 angle, nextAngle;
  mioMat4 mvp, camera_pos;
  mioMat4 viewProjectionMatrix;
  mioMat4 camera_rot =
      mio_mat4_mul(mio_mat4_rotate_y(-cam->yaw), mio_mat4_identity());
  camera_rot = mio_mat4_mul(mio_mat4_rotate_x(-cam->pitch), camera_rot);
  camera_pos = mio_mat4_translate(mio_vec3_scale(cam->pos, -1.0f));
  viewProjectionMatrix =
      mio_mat4_mul(cam->projection, mio_mat4_mul(camera_rot, camera_pos));
  mvp = mio_mat4_mul(viewProjectionMatrix, mio_mat4_translate(center));

  for (i = 0; i < segments; i++) {
    angle = (real32)i / segments * MIO_TAU;
    nextAngle = (real32)(i + 1) / segments * MIO_TAU;
    mio_3d_draw_line(
        cam, mvp, mio_vec3(cos(angle) * radius, sin(angle) * radius, 0),
        mio_vec3(cos(nextAngle) * radius, sin(nextAngle) * radius, 0), t);
    mio_3d_draw_line(
        cam, mvp, mio_vec3(cos(angle) * radius, 0, sin(angle) * radius),
        mio_vec3(cos(nextAngle) * radius, 0, sin(nextAngle) * radius), t);
    mio_3d_draw_line(
        cam, mvp, mio_vec3(0, cos(angle) * radius, sin(angle) * radius),
        mio_vec3(0, cos(nextAngle) * radius, sin(nextAngle) * radius), t);
  }
}

void mio_3d_draw_aabb_lines(
    mioCamera *cam, mioMat4 mvp, mioVec3 min, mioVec3 max, real32 t) {
  mioVec3 corners[8];
  corners[0] = mio_vec3(min.x, min.y, min.z);
  corners[1] = mio_vec3(max.x, min.y, min.z);
  corners[2] = mio_vec3(max.x, max.y, min.z);
  corners[3] = mio_vec3(min.x, max.y, min.z);
  corners[4] = mio_vec3(min.x, min.y, max.z);
  corners[5] = mio_vec3(max.x, min.y, max.z);
  corners[6] = mio_vec3(max.x, max.y, max.z);
  corners[7] = mio_vec3(min.x, max.y, max.z);

  mio_3d_draw_line(cam, mvp, corners[0], corners[1], t);
  mio_3d_draw_line(cam, mvp, corners[1], corners[2], t);
  mio_3d_draw_line(cam, mvp, corners[2], corners[3], t);
  mio_3d_draw_line(cam, mvp, corners[3], corners[0], t);

  mio_3d_draw_line(cam, mvp, corners[4], corners[5], t);
  mio_3d_draw_line(cam, mvp, corners[5], corners[6], t);
  mio_3d_draw_line(cam, mvp, corners[6], corners[7], t);
  mio_3d_draw_line(cam, mvp, corners[7], corners[4], t);

  mio_3d_draw_line(cam, mvp, corners[0], corners[4], t);
  mio_3d_draw_line(cam, mvp, corners[1], corners[5], t);
  mio_3d_draw_line(cam, mvp, corners[2], corners[6], t);
  mio_3d_draw_line(cam, mvp, corners[3], corners[7], t);
}

void mio_3d_draw_mesh_aabb_lines(mioCamera *cam, mioMesh *mesh, mioMat4 modelMatrix, real32 t) {
    mioMat4 camera_rot1 = mio_mat4_mul(mio_mat4_rotate_y(-cam->yaw), mio_mat4_identity());
    mioMat4 camera_rot2 = mio_mat4_mul(mio_mat4_rotate_x(-cam->pitch), camera_rot1);
    mioMat4 camera_pos = mio_mat4_translate(mio_vec3_scale(cam->pos, -1.0f));
    mioMat4 viewProjectionMatrix = mio_mat4_mul(cam->projection, mio_mat4_mul(camera_rot2, camera_pos));
    mioMat4 mvp = viewProjectionMatrix;
    mioVec3 localMin = mesh->boundsMin;
    mioVec3 localMax = mesh->boundsMax;
    mioVec3 localCorners[8] = {
        mio_vec3(localMin.x, localMin.y, localMin.z),
        mio_vec3(localMax.x, localMin.y, localMin.z),
        mio_vec3(localMax.x, localMax.y, localMin.z),
        mio_vec3(localMin.x, localMax.y, localMin.z),
        mio_vec3(localMin.x, localMin.y, localMax.z),
        mio_vec3(localMax.x, localMin.y, localMax.z),
        mio_vec3(localMax.x, localMax.y, localMax.z),
        mio_vec3(localMin.x, localMax.y, localMax.z)
    };
    mioVec3 worldMin = mio_vec3(MIO_REAL32_MAX, MIO_REAL32_MAX, MIO_REAL32_MAX);
    mioVec3 worldMax = mio_vec3(MIO_REAL32_MIN, MIO_REAL32_MIN, MIO_REAL32_MIN);
    int32 i;
    for (i = 0; i < 8; ++i) {
        mioVec3 worldCorner = mio_mat4_mul_point(modelMatrix, localCorners[i]);
        worldMin = mio_vec3_min(worldMin, worldCorner);
        worldMax = mio_vec3_max(worldMax, worldCorner);
    }
    mio_3d_draw_aabb_lines(cam, mvp, worldMin, worldMax, t);
}

mioMat4 mio_get_camera_world_matrix(mioCamera* cam) {
    mioMat4 rot_y = mio_mat4_rotate_y(cam->yaw);
    mioMat4 rot_x = mio_mat4_rotate_x(cam->pitch);
    mioMat4 trans = mio_mat4_translate(cam->pos);
    return mio_mat4_mul(trans, mio_mat4_mul(rot_y, rot_x));
}

void mio_3d_draw_dynamic_aabb(mioCamera *cam, mioMesh *mesh, mioMat4 modelMatrix, real32 pad, real32 t) {
    mioMat4 camera_rot;
    mioMat4 camera_pos;
    mioMat4 viewProjectionMatrix;
    mioVec3 worldMin;
    mioVec3 worldMax;
    mioVec3 worldVertex;
    int32 i;

    if (!mesh || !mesh->vertices || mesh->vertexCount == 0) {
        return;
    }

    worldVertex = mio_mat4_mul_point(modelMatrix, mio_vec3(mesh->vertices[0].pos.x, mesh->vertices[0].pos.y, mesh->vertices[0].pos.z));
    worldMin = worldVertex;
    worldMax = worldVertex;

    for (i = 1; i < mesh->vertexCount; i++) {
        worldVertex = mio_mat4_mul_point(modelMatrix, mio_vec3(mesh->vertices[i].pos.x, mesh->vertices[i].pos.y, mesh->vertices[i].pos.z));
        worldMin.x = MIO_MIN(worldMin.x, worldVertex.x);
        worldMin.y = MIO_MIN(worldMin.y, worldVertex.y);
        worldMin.z = MIO_MIN(worldMin.z, worldVertex.z);
        worldMax.x = MIO_MAX(worldMax.x, worldVertex.x);
        worldMax.y = MIO_MAX(worldMax.y, worldVertex.y);
        worldMax.z = MIO_MAX(worldMax.z, worldVertex.z);
    }

    camera_rot = mio_mat4_mul(mio_mat4_rotate_y(-cam->yaw), mio_mat4_identity());
    camera_rot = mio_mat4_mul(mio_mat4_rotate_x(-cam->pitch), camera_rot);
    camera_pos = mio_mat4_translate(mio_vec3_scale(cam->pos, -1.0f));
    viewProjectionMatrix = mio_mat4_mul(cam->projection, mio_mat4_mul(camera_rot, camera_pos));
    mio_3d_draw_aabb_lines(cam, viewProjectionMatrix, mio_vec3_sub(worldMin, mio_vec3(pad, pad, pad)), mio_vec3_add(worldMax, mio_vec3(pad, pad, pad)), t);
}

mioMesh mio_create_cylinder_mesh(real32 radius, real32 height, int32 segments) {
    mioMesh mesh;
    int32 vertexCount = segments * 2 + 2;
    int32 indexCount = segments * 12;
    int32 i, k;
    real32 angle, h_half;

    mesh.vertices = (mioVertex*)sys_alloc(sizeof(mioVertex) * vertexCount);
    mesh.indices = (int32*)sys_alloc(sizeof(int32) * indexCount);
    mesh.vertexCount = vertexCount;
    mesh.indexCount = indexCount;

    h_half = height * 0.5f;

    mesh.vertices[0].pos = mio_vec4(0.0f, h_half, 0.0f, 1.0f);
    mesh.vertices[0].uv = mio_vec2(0.5f, 0.5f);

    for (i = 0; i < segments; i++) {
        angle = (real32)i / (real32)segments * MIO_TAU;
        mesh.vertices[i + 1].pos = mio_vec4(cos(angle) * radius, h_half, sin(angle) * radius, 1.0f);
        mesh.vertices[i + 1].uv = mio_vec2((cos(angle) + 1.0f) * 0.5f, (sin(angle) + 1.0f) * 0.5f);
        mesh.vertices[i + 1 + segments].pos = mio_vec4(cos(angle) * radius, -h_half, sin(angle) * radius, 1.0f);
        mesh.vertices[i + 1 + segments].uv = mio_vec2((cos(angle) + 1.0f) * 0.5f, (sin(angle) + 1.0f) * 0.5f);
    }

    mesh.vertices[vertexCount - 1].pos = mio_vec4(0.0f, -h_half, 0.0f, 1.0f);
    mesh.vertices[vertexCount - 1].uv = mio_vec2(0.5f, 0.5f);

    k = 0;
    for (i = 0; i < segments; i++) {
        int32 i1_top = i + 1;
        int32 i2_top = (i + 1) % segments + 1;
        int32 i1_bottom = i + 1 + segments;
        int32 i2_bottom = (i + 1) % segments + 1 + segments;

        mesh.indices[k++] = 0;
        mesh.indices[k++] = i2_top;
        mesh.indices[k++] = i1_top;

        mesh.indices[k++] = i1_top;
        mesh.indices[k++] = i2_top;
        mesh.indices[k++] = i1_bottom;

        mesh.indices[k++] = i2_top;
        mesh.indices[k++] = i2_bottom;
        mesh.indices[k++] = i1_bottom;

        mesh.indices[k++] = vertexCount - 1;
        mesh.indices[k++] = i1_bottom;
        mesh.indices[k++] = i2_bottom;
    }

    return mesh;
}

mioMesh mio_create_torus_mesh(real32 majorRadius, real32 minorRadius, int32 majorSegments, int32 minorSegments) {
    mioMesh mesh;
    int32 vertexCount = (majorSegments + 1) * (minorSegments + 1);
    int32 indexCount = majorSegments * minorSegments * 6;
    int32 i, j, k;
    real32 majorAngle, minorAngle;
    real32 x, y, z;

    mesh.vertices = (mioVertex*)sys_alloc(sizeof(mioVertex) * vertexCount);
    mesh.indices = (int32*)sys_alloc(sizeof(int32) * indexCount);
    mesh.vertexCount = vertexCount;
    mesh.indexCount = indexCount;

    k = 0;
    for (i = 0; i <= majorSegments; i++) {
        majorAngle = (real32)i / (real32)majorSegments * MIO_TAU;
        for (j = 0; j <= minorSegments; j++) {
            minorAngle = (real32)j / (real32)minorSegments * MIO_TAU;

            x = (majorRadius + minorRadius * cos(minorAngle)) * cos(majorAngle);
            y = minorRadius * sin(minorAngle);
            z = (majorRadius + minorRadius * cos(minorAngle)) * sin(majorAngle);

            mesh.vertices[k].pos = mio_vec4(x, y, z, 1.0f);
            mesh.vertices[k].uv = mio_vec2((real32)i / majorSegments, (real32)j / minorSegments);
            k++;
        }
    }

    k = 0;
    for (i = 0; i < majorSegments; i++) {
        for (j = 0; j < minorSegments; j++) {
            int32 i1 = i * (minorSegments + 1) + j;
            int32 i2 = (i + 1) * (minorSegments + 1) + j;
            int32 i3 = (i + 1) * (minorSegments + 1) + (j + 1);
            int32 i4 = i * (minorSegments + 1) + (j + 1);

            mesh.indices[k++] = i1;
            mesh.indices[k++] = i3;
            mesh.indices[k++] = i2;

            mesh.indices[k++] = i1;
            mesh.indices[k++] = i4;
            mesh.indices[k++] = i3;
        }
    }

    return mesh;
}

uint32 mio_alpha_blend_fast(uint32 fg, uint32 bg) {
  uint32 fgA = (fg >> 24) & 0xFF;
  uint32 bgR = (bg >> 16) & 0xFF;
  uint32 bgG = (bg >> 8) & 0xFF;
  uint32 bgB = (bg >> 0) & 0xFF;
  uint32 bgA = (bg >> 24) & 0xFF;

  uint32 fgA_n = fgA + (fgA >> 7);
  uint32 inv_fgA_n = 256 - fgA_n;

  uint32 outR = ((((fg >> 16) & 0xFF) * fgA_n) + (bgR * inv_fgA_n)) >> 8;
  uint32 outG = ((((fg >> 8) & 0xFF) * fgA_n) + (bgG * inv_fgA_n)) >> 8;
  uint32 outB = ((((fg >> 0) & 0xFF) * fgA_n) + (bgB * inv_fgA_n)) >> 8;

  uint32 outA = fgA + ((bgA * inv_fgA_n) >> 8);

  outA = MIO_MIN(outA, 0xFF);

  return (outA << 24) | (outR << 16) | (outG << 8) | outB;
}

/* @THREADS ******************************************************************/

#if defined(_WIN32)
#define MIO_THREAD_COUNT_MAX 64

typedef void (*PFMIOJOBPROC)(void *data);

typedef struct {
  PFMIOJOBPROC jobProc;
  void *data;
} mioWorkItem;

typedef struct {
  HANDLE workAvailable;
  HANDLE workDone;
  HANDLE thread;
  int32 id;
  mioWorkItem *work;
  int32 isWorking;
} mioWorker;

typedef struct {
  mioWorker workers[MIO_THREAD_COUNT_MAX];
  int32 terminate;
} mioThreadPool;

MIO_GLOBAL mioThreadPool G_MIO_THREAD_POOL = {0};

DWORD WINAPI mio_thread_worker(LPVOID lpParam) {
  mioWorker *worker = (mioWorker *)lpParam;
  HANDLE workHandles[] = {worker->workAvailable};
  while (G_MIO_THREAD_POOL.terminate == 0) {
    WaitForMultipleObjects(1, workHandles, FALSE, INFINITE);
    if (G_MIO_THREAD_POOL.terminate != 0) { break; }
    if (worker->work && worker->work->jobProc) {
      worker->work->jobProc(worker->work->data);
    }
    worker->isWorking = 0;
    SetEvent(worker->workDone);
  }
  return 0;
}

void mio_thread_pool_init(void) {
  int32 i;
  G_MIO_THREAD_POOL.terminate = 0;
  for (i = 0; i < MIO_THREAD_COUNT_MAX; i++) {
    G_MIO_THREAD_POOL.workers[i].id = i;
    G_MIO_THREAD_POOL.workers[i].workAvailable =
        CreateEvent(NULL, FALSE, FALSE, NULL);
    G_MIO_THREAD_POOL.workers[i].workDone =
        CreateEvent(NULL, FALSE, FALSE, NULL);
    G_MIO_THREAD_POOL.workers[i].thread = CreateThread(
        NULL, 0, (LPTHREAD_START_ROUTINE)mio_thread_worker,
        (LPVOID)&G_MIO_THREAD_POOL.workers[i], 0, NULL);
  }
}

void mio_thread_pool_dispatch(mioWorkItem *items, int32 count) {
  int32 i;
  HANDLE doneHandles[MIO_THREAD_COUNT_MAX];
  for (i = 0; i < count && i < MIO_THREAD_COUNT_MAX; i++) {
    G_MIO_THREAD_POOL.workers[i].work = &items[i];
    G_MIO_THREAD_POOL.workers[i].isWorking = 1;
    doneHandles[i] = G_MIO_THREAD_POOL.workers[i].workDone;
    SetEvent(G_MIO_THREAD_POOL.workers[i].workAvailable);
  }
  WaitForMultipleObjects(count, doneHandles, TRUE, INFINITE);
}

void mio_thread_pool_shutdown(void) {
  int32 i;
  G_MIO_THREAD_POOL.terminate = 1;
  for (i = 0; i < MIO_THREAD_COUNT_MAX; i++) {
    SetEvent(G_MIO_THREAD_POOL.workers[i].workAvailable);
  }
  for (i = 0; i < MIO_THREAD_COUNT_MAX; i++) {
    WaitForSingleObject(G_MIO_THREAD_POOL.workers[i].thread, 5000);
    CloseHandle(G_MIO_THREAD_POOL.workers[i].thread);
    CloseHandle(G_MIO_THREAD_POOL.workers[i].workAvailable);
    CloseHandle(G_MIO_THREAD_POOL.workers[i].workDone);
  }
}
#endif

/* @LOADERS ******************************************************************/

int mio_strlen(const char *s) {
  int count = 0;
  while (*s) {
    count++;
    s++;
  }
  return count;
}

int mio_strncmp(const char *s1, const char *s2, int n) {
  while (n-- > 0) {
    if (*s1 != *s2) { return (int)*s1 - (int)*s2; }
    if (*s1 == '\0') { return 0; }
    s1++;
    s2++;
  }
  return 0;
}

real32 mio_strtof(const char **str) {
  real32 res = 0.0f;
  real32 sign = 1.0f;
  real32 decimal = 0.1f;
  int isDecimal = 0;
  if (**str == '-') {
    sign = -1.0f;
    (*str)++;
  } else if (**str == '+') {
    (*str)++;
  }
  while (**str >= '0' && **str <= '9') {
    res = res * 10.0f + (**str - '0');
    (*str)++;
  }
  if (**str == '.') {
    isDecimal = 1;
    (*str)++;
  }
  while (isDecimal && **str >= '0' && **str <= '9') {
    res = res + (**str - '0') * decimal;
    decimal *= 0.1f;
    (*str)++;
  }
  return res * sign;
}

int mio_strtoi(const char **str) {
  int res = 0;
  int sign = 1;
  if (**str == '-') {
    sign = -1;
    (*str)++;
  } else if (**str == '+') {
    (*str)++;
  }
  while (**str >= '0' && **str <= '9') {
    res = res * 10 + (**str - '0');
    (*str)++;
  }
  return res * sign;
}

mioMesh mio_load_obj(const char *filepath) {
  uint8 *file_data;
  int32 file_size;
  int32 f_count_actual;
  const char *current_line;
  const char *end_of_file;
  mioMesh mesh = {0};
  mioVec3 *temp_v = NULL;
  mioVec2 *temp_vt = NULL;
  mioVec3 *temp_vn = NULL;
  int32 v_count = 0;
  int32 vt_count = 0;
  int32 vn_count = 0;
  int32 f_count_target = 0;
  int32 i, len;

  file_data = sys_load_file(filepath, &file_size);
  if (!file_data) { return mesh; }

  current_line = (const char *)file_data;
  end_of_file = current_line + file_size;

  while (current_line < end_of_file) {
    if (mio_strncmp(current_line, "v ", 2) == 0) {
      v_count++;
    } else if (mio_strncmp(current_line, "vt ", 3) == 0) {
      vt_count++;
    } else if (mio_strncmp(current_line, "vn ", 3) == 0) {
      vn_count++;
    } else if (mio_strncmp(current_line, "f ", 2) == 0) {
      const char *p = current_line + 2;
      int num_vertices = 0;
      int in_token = 0;
      while (*p && *p != '\n' && *p != '\r') {
        if (*p != ' ') {
          if (in_token == 0) {
            num_vertices++;
            in_token = 1;
          }
        } else {
          in_token = 0;
        }
        p++;
      }
      if (num_vertices >= 3) { f_count_target += (num_vertices - 2) * 3; }
    }

    while (*current_line && *current_line != '\n') { current_line++; }
    if (*current_line == '\n') { current_line++; }
  }

  temp_v = (mioVec3 *)sys_realloc(NULL, v_count * sizeof(mioVec3));
  temp_vt = (mioVec2 *)sys_realloc(NULL, vt_count * sizeof(mioVec2));
  temp_vn = (mioVec3 *)sys_realloc(NULL, vn_count * sizeof(mioVec3));
  mesh.vertices =
      (mioVertex *)sys_realloc(NULL, f_count_target * sizeof(mioVertex));
  mesh.indices = (int32 *)sys_realloc(NULL, f_count_target * sizeof(int32));
  mesh.vertexCount = f_count_target;
  mesh.indexCount = f_count_target;

  if (!temp_v || !temp_vt || !temp_vn || !mesh.vertices || !mesh.indices) {
    sys_free(file_data);
    sys_free(temp_v);
    sys_free(temp_vt);
    sys_free(temp_vn);
    sys_free(mesh.vertices);
    sys_free(mesh.indices);
    mesh.vertices = NULL;
    mesh.indices = NULL;
    mesh.vertexCount = 0;
    mesh.indexCount = 0;
    return mesh;
  }

  v_count = 0;
  vt_count = 0;
  vn_count = 0;
  f_count_actual = 0;

  current_line = (const char *)file_data;
  while (current_line < end_of_file) {
    if (mio_strncmp(current_line, "v ", 2) == 0) {
      const char *p = current_line + 2;
      while (*p == ' ') { p++; }
      temp_v[v_count].x = mio_strtof(&p);
      while (*p == ' ') { p++; }
      temp_v[v_count].y = mio_strtof(&p);
      while (*p == ' ') { p++; }
      temp_v[v_count].z = mio_strtof(&p);
      v_count++;
    } else if (mio_strncmp(current_line, "vt ", 3) == 0) {
      const char *p = current_line + 3;
      while (*p == ' ') { p++; }
      temp_vt[vt_count].x = mio_strtof(&p);
      while (*p == ' ') { p++; }
      temp_vt[vt_count].y = mio_strtof(&p);
      vt_count++;
    } else if (mio_strncmp(current_line, "vn ", 3) == 0) {
      const char *p = current_line + 3;
      while (*p == ' ') { p++; }
      temp_vn[vn_count].x = mio_strtof(&p);
      while (*p == ' ') { p++; }
      temp_vn[vn_count].y = mio_strtof(&p);
      while (*p == ' ') { p++; }
      temp_vn[vn_count].z = mio_strtof(&p);
      vn_count++;
    } else if (mio_strncmp(current_line, "f ", 2) == 0) {
      const char *p = current_line + 2;
      int v_indices[32], vt_indices[32], vn_indices[32];
      int num_verts_in_face = 0;
      while (num_verts_in_face < 32) {
        int v_i = 0, vt_i = 0, vn_i = 0;
        while (*p == ' ') { p++; }
        if (*p == '\0' || *p == '\n' || *p == '\r') { break; }

        v_i = mio_strtoi(&p);
        if (*p == '/') {
          p++;
          if (*p != '/') { vt_i = mio_strtoi(&p); }
          if (*p == '/') {
            p++;
            vn_i = mio_strtoi(&p);
          }
        }

        v_indices[num_verts_in_face] = v_i;
        vt_indices[num_verts_in_face] = vt_i;
        vn_indices[num_verts_in_face] = vn_i;
        num_verts_in_face++;
      }

      for (i = 1; i < num_verts_in_face - 1; i++) {
        int idx[] = {0, i, i + 1};
        for (len = 0; len < 3; len++) {
          int v_idx = v_indices[idx[len]];
          int vt_idx = vt_indices[idx[len]];
          int vn_idx = vn_indices[idx[len]];

          if (v_count == 0) {
            v_idx = 0;
          } else if (v_idx < 1 || v_idx > v_count) {
            v_idx = 1;
          }

          if (vt_count == 0) {
            vt_idx = 0;
          } else if (vt_idx < 1 || vt_idx > vt_count) {
            vt_idx = 0;
          }

          if (vn_count == 0) {
            vn_idx = 0;
          } else if (vn_idx < 1 || vn_idx > vn_count) {
            vn_idx = 0;
          }

          if (v_idx >= 1) {
            mesh.vertices[f_count_actual].pos = mio_vec4(
                temp_v[v_idx - 1].x, temp_v[v_idx - 1].y, temp_v[v_idx - 1].z,
                1.0f);
            if (vt_idx) {
              mesh.vertices[f_count_actual].uv =
                  mio_vec2(temp_vt[vt_idx - 1].x, temp_vt[vt_idx - 1].y);
            } else {
              mesh.vertices[f_count_actual].uv = mio_vec2(0.0f, 0.0f);
            }
            if (vn_idx) {
              mesh.vertices[f_count_actual].normal = mio_vec3(
                  temp_vn[vn_idx - 1].x, temp_vn[vn_idx - 1].y,
                  temp_vn[vn_idx - 1].z);
            } else {
              mesh.vertices[f_count_actual].normal =
                  mio_vec3(0.0f, 0.0f, 0.0f);
            }

            mesh.indices[f_count_actual] = f_count_actual;
            f_count_actual++;
          }
        }
      }
    }

    while (*current_line && *current_line != '\n') { current_line++; }
    if (*current_line == '\n') { current_line++; }
  }

  mesh.vertexCount = f_count_actual;
  mesh.indexCount = f_count_actual;

  sys_free(file_data);
  sys_free(temp_v);
  sys_free(temp_vt);
  sys_free(temp_vn);
  return mesh;
}

uint32 mio_bmp_read_uint16(const uint8 *data) {
  return (uint32)data[0] | ((uint32)data[1] << 8);
}

uint32 mio_bmp_read_uint32(const uint8 *data) {
  return (uint32)data[0] | ((uint32)data[1] << 8) | ((uint32)data[2] << 16) |
         ((uint32)data[3] << 24);
}

mioTexture mio_load_bmp(const char *filepath) {
  uint8 *file_data;
  int32 file_size;
  mioTexture texture = {0};
  uint32 data_offset;
  uint32 header_size;
  uint32 image_width;
  uint32 image_height;
  uint32 bits_per_pixel;
  uint32 compression;
  uint32 i, j;
  uint32 row_padding;
  uint32 row_size;
  uint32 pixel_data_size;
  uint32 temp_w;

  file_data = sys_load_file(filepath, &file_size);
  if (!file_data) {
    sys_log("Failed to load file %s\n", filepath);
    return texture;
  }

  if (file_size < 54 || mio_strncmp((const char *)file_data, "BM", 2) != 0) {
    sys_log("Wrong file\n");
    sys_free(file_data);
    return texture;
  }

  data_offset = mio_bmp_read_uint32(file_data + 10);
  header_size = mio_bmp_read_uint32(file_data + 14);
  image_width = mio_bmp_read_uint32(file_data + 18);
  image_height = mio_bmp_read_uint32(file_data + 22);
  bits_per_pixel = mio_bmp_read_uint16(file_data + 28);
  compression = mio_bmp_read_uint32(file_data + 30);

  if (header_size < 40 || bits_per_pixel != 24 || compression != 0) {
    sys_log(
        "Wrong format: %d/40 %d/24 %d/0\n", header_size, bits_per_pixel,
        compression);
    sys_free(file_data);
    return texture;
  }

  row_size = (image_width * bits_per_pixel / 8);
  row_padding = (4 - (row_size % 4)) % 4;
  row_size += row_padding;
  pixel_data_size = image_height * row_size;
  if ((uint32)file_size < data_offset + pixel_data_size) {
    sys_log(
        "Malformed file: %d/%d\n", file_size, data_offset + pixel_data_size);
    sys_free(file_data);
    return texture;
  }

  texture.width = image_width;
  texture.height = image_height;
  temp_w = image_width;
  texture.data = (uint32 *)sys_alloc(temp_w * image_height * sizeof(uint32));
  if (!texture.data) {
    sys_log(
        "Failed to allocate texture memory: %dkb\n",
        temp_w * image_height * sizeof(uint32) / 1024);
    sys_free(file_data);
    return texture;
  }

  for (i = 0; i < image_height; i++) {
    for (j = 0; j < image_width; j++) {
      uint32 pixel_index = (image_height - 1 - i) * image_width + j;
      uint32 file_index = data_offset + i * row_size + j * 3;
      uint8 b = file_data[file_index + 0];
      uint8 g = file_data[file_index + 1];
      uint8 r = file_data[file_index + 2];
      texture.data[pixel_index] = MIO_RGBA(r, g, b, 0xFF);
    }
  }
  sys_log("Loaded image: %s %dx%d\n", filepath, texture.width, texture.height);
  sys_free(file_data);
  return texture;
}

/* @SETUP ********************************************************************/

int32 mio_app_run(
    mioApp* app, const char *title, int32 width, int32 height, int32 res,
    int32 fps) {
  int32 ret = 0;
  uint32 run = TRUE;
  real64 last_time = 0.0;
  real64 accumulated_time = 0.0;
  const real64 fixed_dt = 1.0 / 60.0;
  size_t fbSize = sizeof(uint32) * 2560 * 1440 * 4;
  void *fbMem = sys_alloc((int32)fbSize);
  if (!sys_init(title, width, height, 1) || !fbMem) { return 1; }
  G_APP.render = mio_render_context(fbMem, fbSize, TRUE, res);
  if (!G_APP.render.colourData) {
    sys_shutdown();
    return 1;
  }
	G_APP.render.camera = mio_3d_camera(TRUE, 0.8f, 0.01f, 100.0f, 20.0f, 2.0f);
  if (app->init && !app->init(app->state)) {
    sys_free(fbMem);
    sys_shutdown();
    return 1;
  }
  last_time = sys_get_time();
  sys_set_timer(16);
  G_SYS.fpsUpdateCount = 0;
  while (run) {
    real64 current_time = sys_get_time();
    real64 delta_time = current_time - last_time;
    mioSystemSize winSize = sys_get_window_size();
    mio_render_context_resize(winSize.width/res, winSize.height/res);
    last_time = current_time;
    accumulated_time += delta_time;
    while (accumulated_time >= fixed_dt) {
      if (app->update) {
        run = sys_process_messages();
				mio_3d_camera_update(
					&app->render.camera,
					app->render.width,
					app->render.height,
					TRUE, fixed_dt);
        app->update(app->state, fixed_dt);
        G_SYS.fpsUpdateCount++;
      }
      accumulated_time -= fixed_dt;
    }
    if (app->draw) {
			sys_present();
		}
    sys_log_perf();
#if 0
		sys_sleep(2);
#endif
	}
  if (app->shutdown) { app->shutdown(app->state); }
  sys_free(fbMem);
  sys_shutdown();
  return ret;
}

int32 mio_user_init(void *state);
int32 mio_user_update(void *state, real64 dt);
int32 mio_user_draw(void *state);
int32 mio_user_exit(void *state);

int32 mio_run(
    void *state, const char *title, int32 width, int32 height, int32 res,
    int32 fps) {
  G_APP.state = state;
  G_APP.init = mio_user_init;
  G_APP.update = mio_user_update;
  G_APP.draw = mio_user_draw;
  G_APP.shutdown = mio_user_exit;

  mio_3d_meshes_init();
  return mio_app_run(&G_APP, title, width, height, res, fps);
}

SYSRET mio_audio_init(PFSYSTEMAUDIOCB cb, void *ud) {
  return sys_init_audio(
      SYS_AUDIO_DEFAULT_SAMPLE_RATE, SYS_AUDIO_DEFAULT_CHANNELS, cb, ud);
}

#endif /* MIO_H */
