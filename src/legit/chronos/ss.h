#ifndef SS_ENGINE_H
#define SS_ENGINE_H

#define M_PI 3.14159265358979323846

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>

#define COBJMACROS
#include <initguid.h>

#include <Objbase.h>
#include <Audioclient.h>
#include <Mmdeviceapi.h>
#elif defined(__linux__)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <alsa/asoundlib.h>
#include <unistd.h>
#include <pthread.h>
#endif

typedef struct ss_window_t ss_window_t;
typedef struct ss_audio_t ss_audio_t;

typedef struct ss_input_t {
    int key_st[256];
    int mouse_x;
    int mouse_y;
    int mouse_l;
    int mouse_r;
} ss_input_t;

typedef void (*ss_audio_cb_t)(short *buf, int n_frames, int s_rate, int ch, void *udata);

#if defined(_WIN32)
typedef HANDLE ss_thread_t;
typedef unsigned long (__stdcall *ss_thread_func_t)(void *arg);
#elif defined(__linux__)
typedef pthread_t ss_thread_t;
typedef void *(*ss_thread_func_t)(void *arg);
#endif

ss_window_t *ss_open_window(int w, int h, const char *title);
void ss_close_window(ss_window_t *wnd);
int ss_process_events(ss_window_t *wnd);
void ss_blit_fb(ss_window_t *wnd, unsigned char *fb, int w, int h);
ss_input_t ss_get_input(ss_window_t *wnd);

ss_audio_t *ss_open_audio(ss_audio_cb_t play_cb, ss_audio_cb_t rec_cb, int s_rate, int ch, int buf_frames, void *udata);
void ss_close_audio(ss_audio_t *audio);

ss_thread_t ss_thread_create(ss_thread_func_t func, void *arg);
void ss_thread_join(ss_thread_t thread);

#ifdef SS_ENGINE_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if defined(_WIN32)

ss_thread_t ss_thread_create(ss_thread_func_t func, void *arg) {
    return (ss_thread_t)_beginthreadex(NULL, 0, (unsigned int (__stdcall *)(void *))func, arg, 0, NULL);
}

void ss_thread_join(ss_thread_t thread) {
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
}

#elif defined(__linux__)

ss_thread_t ss_thread_create(ss_thread_func_t func, void *arg) {
    pthread_t thread;
    pthread_create(&thread, NULL, func, arg);
    return thread;
}

void ss_thread_join(ss_thread_t thread) {
    pthread_join(thread, NULL);
}

#endif

#if defined(_WIN32)

struct ss_window_t {
    HWND hwnd;
    HDC hdc_wnd;
    BITMAPINFO bmi;
    int w;
    int h;
    ss_input_t input;
    int running;
};

struct ss_audio_t {
    ss_thread_t a_thread;
    int running;

    IMMDeviceEnumerator *pEnum;
    IMMDevice *pDev_p;
    IMMDevice *pDev_c;

    IAudioClient *pAC_p;
    IAudioRenderClient *pRC;
    HANDLE hEvt_p;
    UINT32 buf_fr_c_p;

    IAudioClient *pAC_c;
    IAudioCaptureClient *pCC;
    HANDLE hEvt_c;
    UINT32 buf_fr_c_c;

    int s_rate;
    int n_ch;
    ss_audio_cb_t play_cb;
    ss_audio_cb_t rec_cb;
    void *udata;
    int buf_frames;
};

static LRESULT CALLBACK ss_window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    ss_window_t *wnd = (ss_window_t *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (!wnd) return DefWindowProc(hwnd, uMsg, wParam, lParam);
		switch (uMsg) {
        case WM_DESTROY:
        case WM_CLOSE:
            wnd->running = 0;
            return 0;
        case WM_SIZE:
            wnd->w = LOWORD(lParam);
            wnd->h = HIWORD(lParam);
            return 0;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if (wParam < 256) wnd->input.key_st[wParam] = 1;
            return 0;
        case WM_KEYUP:
        case WM_SYSKEYUP:
            if (wParam < 256) wnd->input.key_st[wParam] = 0;
            return 0;
        case WM_LBUTTONDOWN:
            wnd->input.mouse_l = 1;
            return 0;
        case WM_LBUTTONUP:
            wnd->input.mouse_l = 0;
            return 0;
        case WM_RBUTTONDOWN:
            wnd->input.mouse_r = 1;
            return 0;
        case WM_RBUTTONUP:
            wnd->input.mouse_r = 0;
            return 0;
        case WM_MOUSEMOVE:
            wnd->input.mouse_x = LOWORD(lParam);
            wnd->input.mouse_y = HIWORD(lParam);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

ss_window_t *ss_open_window(int w, int h, const char *title) {
    ss_window_t *wnd;
    WNDCLASSEX wc;
    RECT rect;
    DWORD dwStyle;

    wnd = (ss_window_t *)malloc(sizeof(ss_window_t));
    if (!wnd) return NULL;
    memset(wnd, 0, sizeof(ss_window_t));

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = ss_window_proc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "SS_WNDCLASS";
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc)) {
        free(wnd);
        return NULL;
    }

    rect.left = 0;
    rect.top = 0;
    rect.right = w;
    rect.bottom = h;
    dwStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    AdjustWindowRect(&rect, dwStyle, FALSE);

    wnd->hwnd = CreateWindowEx(
        0,
        "SS_WNDCLASS",
        title,
        dwStyle,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        NULL,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );

    if (!wnd->hwnd) {
        UnregisterClass("SS_WNDCLASS", GetModuleHandle(NULL));
        free(wnd);
        return NULL;
    }

    wnd->w = w;
    wnd->h = h;
    wnd->running = 1;
    wnd->hdc_wnd = GetDC(wnd->hwnd);

    wnd->bmi.bmiHeader.biSize = sizeof(wnd->bmi.bmiHeader);
    wnd->bmi.bmiHeader.biWidth = w;
    wnd->bmi.bmiHeader.biHeight = -h;
    wnd->bmi.bmiHeader.biPlanes = 1;
    wnd->bmi.bmiHeader.biBitCount = 32;
    wnd->bmi.bmiHeader.biCompression = BI_RGB;

    SetWindowLongPtr(wnd->hwnd, GWLP_USERDATA, (LONG_PTR)wnd);

    return wnd;
}

void ss_close_window(ss_window_t *wnd) {
    if (wnd) {
        if (wnd->hdc_wnd) ReleaseDC(wnd->hwnd, wnd->hdc_wnd);
        if (wnd->hwnd) DestroyWindow(wnd->hwnd);
        free(wnd);
    }
    UnregisterClass("SS_WNDCLASS", GetModuleHandle(NULL));
}

int ss_process_events(ss_window_t *wnd) {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return wnd->running;
}

void ss_blit_fb(ss_window_t *wnd, unsigned char *fb, int w, int h) {
    RECT client_rect;
    GetClientRect(wnd->hwnd, &client_rect);
    wnd->w = client_rect.right;
    wnd->h = client_rect.bottom;
    wnd->bmi.bmiHeader.biWidth = w;
    wnd->bmi.bmiHeader.biHeight = -h;
    StretchDIBits(wnd->hdc_wnd, 0, 0, wnd->w, wnd->h, 0, 0, w, h, fb, &wnd->bmi, DIB_RGB_COLORS, SRCCOPY);
}

ss_input_t ss_get_input(ss_window_t *wnd) {
    return wnd->input;
}

static unsigned long __stdcall ss_was_thread(void *arg) {
    ss_audio_t *a = (ss_audio_t *)arg;
    UINT32 pad;
    UINT32 n_avail;
    BYTE *pData;
    HRESULT hr;
    int p_active = a->play_cb != NULL;
    int r_active = a->rec_cb != NULL;
    HANDLE w_hndl[2];
    DWORD w_res;
    int n_w_hndl = 0;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (p_active) w_hndl[n_w_hndl++] = a->hEvt_p;
    if (r_active) w_hndl[n_w_hndl++] = a->hEvt_c;

    while (a->running) {
        w_res = WaitForMultipleObjects(n_w_hndl, w_hndl, FALSE, 500);

        if (w_res == WAIT_TIMEOUT) continue;

        if (p_active && (w_res == WAIT_OBJECT_0 || (r_active && w_res == (WAIT_OBJECT_0 + 1)))) {
            hr = IAudioClient_GetCurrentPadding(a->pAC_p, &pad);
            if (FAILED(hr)) break;

            n_avail = a->buf_fr_c_p - pad;

            if (n_avail > 0) {
                hr = IAudioRenderClient_GetBuffer(a->pRC, n_avail, &pData);
                if (FAILED(hr)) break;

                a->play_cb((short *)pData, (int)n_avail, a->s_rate, a->n_ch, a->udata);

                hr = IAudioRenderClient_ReleaseBuffer(a->pRC, n_avail, 0);
                if (FAILED(hr)) break;
            }
        }

        if (r_active && (w_res == WAIT_OBJECT_0 + (p_active ? 1 : 0))) {
            UINT32 n_to_read;
            DWORD flags;

            hr = IAudioCaptureClient_GetNextPacketSize(a->pCC, &n_to_read);
            if (FAILED(hr)) break;

            while (n_to_read > 0) {
                hr = IAudioCaptureClient_GetBuffer(a->pCC, &pData, &n_avail, &flags, NULL, NULL);
                if (FAILED(hr)) break;

                a->rec_cb((short *)pData, (int)n_avail, a->s_rate, a->n_ch, a->udata);

                hr = IAudioCaptureClient_ReleaseBuffer(a->pCC, n_avail);
                if (FAILED(hr)) break;

                hr = IAudioCaptureClient_GetNextPacketSize(a->pCC, &n_to_read);
                if (FAILED(hr)) break;
            }
        }
    }

    CoUninitialize();
    return 0;
}

static HRESULT ss_was_init(
    ss_audio_t *a,
    EDataFlow flow,
    AUDCLNT_SHAREMODE mode,
    IAudioClient **ppAC,
    UINT32 *pBufFrCount,
    HANDLE *phEvt,
    void **ppClient
) {
    WAVEFORMATEX wfx;
    REFERENCE_TIME hnsReqDur = (REFERENCE_TIME)a->buf_frames * 10000000L / a->s_rate;
    IMMDevice *pDev = (flow == eRender) ? a->pDev_p : a->pDev_c;
    HRESULT hr;

    if (!pDev) return S_FALSE;

    hr = IMMDevice_Activate(pDev, &IID_IAudioClient, CLSCTX_ALL, NULL, (void **)ppAC);
    if (FAILED(hr)) return hr;

    memset(&wfx, 0, sizeof(WAVEFORMATEX));
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = (WORD)a->n_ch;
    wfx.nSamplesPerSec = (DWORD)a->s_rate;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.cbSize = 0;

    *phEvt = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!*phEvt) { IAudioClient_Release(*ppAC); *ppAC = NULL; return E_FAIL; }

    hr = IAudioClient_Initialize(*ppAC, mode, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, hnsReqDur, 0, &wfx, NULL);
    if (FAILED(hr)) { CloseHandle(*phEvt); *phEvt = NULL; IAudioClient_Release(*ppAC); *ppAC = NULL; return hr; }

    hr = IAudioClient_SetEventHandle(*ppAC, *phEvt);
    if (FAILED(hr)) { CloseHandle(*phEvt); *phEvt = NULL; IAudioClient_Release(*ppAC); *ppAC = NULL; return hr; }

    hr = IAudioClient_GetBufferSize(*ppAC, pBufFrCount);
    if (FAILED(hr)) { CloseHandle(*phEvt); *phEvt = NULL; IAudioClient_Release(*ppAC); *ppAC = NULL; return hr; }

    if (flow == eRender) {
        hr = IAudioClient_GetService(*ppAC, &IID_IAudioRenderClient, ppClient);
    } else {
        hr = IAudioClient_GetService(*ppAC, &IID_IAudioCaptureClient, ppClient);
    }
    
    if (FAILED(hr)) { CloseHandle(*phEvt); *phEvt = NULL; IAudioClient_Release(*ppAC); *ppAC = NULL; return hr; }
    
    return hr;
}


ss_audio_t *ss_open_audio(ss_audio_cb_t play_cb, ss_audio_cb_t rec_cb, int s_rate, int n_ch, int buf_frames, void *udata) {
    ss_audio_t *a;
    HRESULT hr;
    
    a = (ss_audio_t *)malloc(sizeof(ss_audio_t));
    if (!a) return NULL;
    memset(a, 0, sizeof(ss_audio_t));

    a->s_rate = s_rate;
    a->n_ch = n_ch;
    a->play_cb = play_cb;
    a->rec_cb = rec_cb;
    a->udata = udata;
    a->buf_frames = buf_frames;
    a->running = 1;

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) { free(a); return NULL; }

    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void**)&a->pEnum);
    if (FAILED(hr)) { CoUninitialize(); free(a); return NULL; }

    if (play_cb) {
        hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(a->pEnum, eRender, eConsole, &a->pDev_p);
        if (FAILED(hr)) { ss_close_audio(a); return NULL; }

        hr = ss_was_init(a, eRender, AUDCLNT_SHAREMODE_SHARED, &a->pAC_p, &a->buf_fr_c_p, &a->hEvt_p, (void**)&a->pRC);
        if (FAILED(hr)) { ss_close_audio(a); return NULL; }
    }

    if (rec_cb) {
        hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(a->pEnum, eCapture, eConsole, &a->pDev_c);
        if (FAILED(hr)) { ss_close_audio(a); return NULL; }

        hr = ss_was_init(a, eCapture, AUDCLNT_SHAREMODE_SHARED, &a->pAC_c, &a->buf_fr_c_c, &a->hEvt_c, (void**)&a->pCC);
        if (FAILED(hr)) { ss_close_audio(a); return NULL; }
    }

    if (play_cb) IAudioClient_Start(a->pAC_p);
    if (rec_cb) IAudioClient_Start(a->pAC_c);

    a->a_thread = ss_thread_create(ss_was_thread, a);
    if (!a->a_thread) { ss_close_audio(a); return NULL; }

    return a;
}

void ss_close_audio(ss_audio_t *a) {
    if (!a) return;

    a->running = 0;
    if (a->a_thread) ss_thread_join(a->a_thread);

    if (a->pAC_p) IAudioClient_Stop(a->pAC_p);
    if (a->pAC_c) IAudioClient_Stop(a->pAC_c);

    if (a->hEvt_p) CloseHandle(a->hEvt_p);
    if (a->hEvt_c) CloseHandle(a->hEvt_c);

    if (a->pRC) IAudioRenderClient_Release(a->pRC);
    if (a->pCC) IAudioCaptureClient_Release(a->pCC);

    if (a->pAC_p) IAudioClient_Release(a->pAC_p);
    if (a->pAC_c) IAudioClient_Release(a->pAC_c);

    if (a->pDev_p) IMMDevice_Release(a->pDev_p);
    if (a->pDev_c) IMMDevice_Release(a->pDev_c);
    if (a->pEnum) IMMDeviceEnumerator_Release(a->pEnum);

    CoUninitialize();
    free(a);
}

#endif

#if defined(__linux__)

#define SS_KEY_MAX 256

struct ss_window_t {
    Display *disp;
    Window wnd;
    XImage *xim;
    GC gc;
    int scr;
    int w;
    int h;
    ss_input_t input;
    int running;
};

struct ss_audio_t {
    ss_thread_t a_thread;
    int running;

    snd_pcm_t *play_h;
    snd_pcm_t *rec_h;
    snd_pcm_uframes_t buf_frames;

    int s_rate;
    int n_ch;
    ss_audio_cb_t play_cb;
    ss_audio_cb_t rec_cb;
    void *udata;
    short *play_buf;
    short *rec_buf;
    int buf_size_b;
};

ss_window_t *ss_open_window(int w, int h, const char *title) {
    ss_window_t *wnd;
    XSetWindowAttributes swa;
    XVisualInfo vinfo;
    Colormap cmap;
    int depth;
    Atom wm_del_wnd;

    wnd = (ss_window_t *)malloc(sizeof(ss_window_t));
    if (!wnd) return NULL;
    memset(wnd, 0, sizeof(ss_window_t));

    wnd->disp = XOpenDisplay(NULL);
    if (!wnd->disp) {
        free(wnd);
        return NULL;
    }

    wnd->scr = DefaultScreen(wnd->disp);
    wnd->w = w;
    wnd->h = h;
    wnd->running = 1;

    depth = DefaultDepth(wnd->disp, wnd->scr);
    if (!XMatchVisualInfo(wnd->disp, wnd->scr, 24, TrueColor, &vinfo) &&
        !XMatchVisualInfo(wnd->disp, wnd->scr, 32, TrueColor, &vinfo)) {
        vinfo.visual = DefaultVisual(wnd->disp, wnd->scr);
        depth = DefaultDepth(wnd->disp, wnd->scr);
    }

    cmap = XCreateColormap(wnd->disp, RootWindow(wnd->disp, wnd->scr), vinfo.visual, AllocNone);

    swa.colormap = cmap;
    swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | StructureNotifyMask;
    
    wnd->wnd = XCreateWindow(
        wnd->disp, RootWindow(wnd->disp, wnd->scr),
        0, 0, w, h, 0, depth, InputOutput, vinfo.visual,
        CWColormap | CWEventMask, &swa
    );

    XStoreName(wnd->disp, wnd->wnd, title);
    XMapWindow(wnd->disp, wnd->wnd);
    
    wm_del_wnd = XInternAtom(wnd->disp, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(wnd->disp, wnd->wnd, &wm_del_wnd, 1);

    wnd->gc = XCreateGC(wnd->disp, wnd->wnd, 0, NULL);

    wnd->xim = XCreateImage(
        wnd->disp, vinfo.visual, depth, ZPixmap, 0,
        (char *)malloc(w * h * 4), w, h, 32, 0
    );
    if (!wnd->xim) {
        ss_close_window(wnd);
        return NULL;
    }
    
    return wnd;
}

void ss_close_window(ss_window_t *wnd) {
    if (wnd) {
        if (wnd->xim) {
            if (wnd->xim->data) free(wnd->xim->data);
            wnd->xim->data = NULL;
            XDestroyImage(wnd->xim);
        }
        if (wnd->gc) XFreeGC(wnd->disp, wnd->gc);
        if (wnd->wnd) XDestroyWindow(wnd->disp, wnd->wnd);
        if (wnd->disp) XCloseDisplay(wnd->disp);
        free(wnd);
    }
}

int ss_process_events(ss_window_t *wnd) {
    XEvent evt;
    KeySym keysym;

    while (XPending(wnd->disp)) {
        XNextEvent(wnd->disp, &evt);
        switch (evt.type) {
            case ClientMessage:
                if ((Atom)evt.xclient.data.l[0] == XInternAtom(wnd->disp, "WM_DELETE_WINDOW", False)) {
                    wnd->running = 0;
                }
                break;
            case KeyPress:
                keysym = XLookupKeysym(&evt.xkey, 0);
                if (keysym < SS_KEY_MAX) wnd->input.key_st[(int)keysym] = 1;
                break;
            case KeyRelease:
                keysym = XLookupKeysym(&evt.xkey, 0);
                if (keysym < SS_KEY_MAX) wnd->input.key_st[(int)keysym] = 0;
                break;
            case ButtonPress:
                if (evt.xbutton.button == 1) wnd->input.mouse_l = 1;
                if (evt.xbutton.button == 3) wnd->input.mouse_r = 1;
                break;
            case ButtonRelease:
                if (evt.xbutton.button == 1) wnd->input.mouse_l = 0;
                if (evt.xbutton.button == 3) wnd->input.mouse_r = 0;
                break;
            case MotionNotify:
                wnd->input.mouse_x = evt.xmotion.x;
                wnd->input.mouse_y = evt.xmotion.y;
                break;
            case ConfigureNotify:
                if (evt.xconfigure.width != wnd->w || evt.xconfigure.height != wnd->h) {
                    wnd->w = evt.xconfigure.width;
                    wnd->h = evt.xconfigure.height;
                    if (wnd->xim) {
                        if (wnd->xim->data) free(wnd->xim->data);
                        XDestroyImage(wnd->xim);
                    }
                    wnd->xim = XCreateImage(
                        wnd->disp, DefaultVisual(wnd->disp, wnd->scr), DefaultDepth(wnd->disp, wnd->scr), ZPixmap, 0,
                        (char *)malloc(wnd->w * wnd->h * 4), wnd->w, wnd->h, 32, 0
                    );
                }
                break;
            default:
                break;
        }
    }

    return wnd->running;
}

void ss_blit_fb(ss_window_t *wnd, unsigned char *fb, int w, int h) {
    if (wnd->xim->width != w || wnd->xim->height != h) {
        if (wnd->xim->data) free(wnd->xim->data);
        XDestroyImage(wnd->xim);
        wnd->xim = XCreateImage(wnd->disp, DefaultVisual(wnd->disp, wnd->scr), DefaultDepth(wnd->disp, wnd->scr), ZPixmap, 0, (char *)malloc(w * h * 4), w, h, 32, 0);
    }
    memcpy(wnd->xim->data, fb, w * h * 4);
    XPutImage(wnd->disp, wnd->wnd, wnd->gc, wnd->xim, 0, 0, 0, 0, w, h);
    XFlush(wnd->disp);
}

ss_input_t ss_get_input(ss_window_t *wnd) {
    return wnd->input;
}

static int ss_alsa_setup(snd_pcm_t **h, const char *dev, int acc, int s_rate, int n_ch, snd_pcm_uframes_t *buf_frames) {
    snd_pcm_hw_params_t *hw_p;
    unsigned int rate;
    int dir;
    int rc;

    rc = snd_pcm_open(h, dev, acc, 0);
    if (rc < 0) { fprintf(stderr, "ALSA: Playback open failed: %s\n", snd_strerror(rc)); return rc; }

    snd_pcm_hw_params_all(*h, &hw_p);
    snd_pcm_hw_params_any(*h, hw_p);
    snd_pcm_hw_params_set_access(*h, hw_p, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(*h, hw_p, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(*h, hw_p, (unsigned int)n_ch);

    rate = (unsigned int)s_rate;
    snd_pcm_hw_params_set_rate_near(*h, hw_p, &rate, &dir);
    if (rate != (unsigned int)s_rate) {
        snd_pcm_hw_params_free(hw_p);
        fprintf(stderr, "ALSA: Rate mismatch. Requested: %d, Got: %d\n", s_rate, rate);
        return -1;
    }

    snd_pcm_hw_params_set_period_size_near(*h, hw_p, buf_frames, &dir);
    snd_pcm_hw_params_set_buffer_size_near(*h, hw_p, *buf_frames * 4, &dir);
    
    rc = snd_pcm_hw_params(*h, hw_p);
    snd_pcm_hw_params_free(hw_p);
    if (rc < 0) { fprintf(stderr, "ALSA: HW params failed: %s\n", snd_strerror(rc)); return rc; }

    {
        snd_pcm_sw_params_t *sw_p;
        snd_pcm_sw_params_all(*h, &sw_p);
        snd_pcm_sw_params_set_tstamp_mode(*h, sw_p, SND_PCM_TSTAMP_NONE);
        snd_pcm_sw_params_set_avail_min(*h, sw_p, *buf_frames);
        rc = snd_pcm_sw_params(*h, sw_p);
        snd_pcm_sw_params_free(sw_p);
    }
    
    if (rc < 0) { fprintf(stderr, "ALSA: SW params failed: %s\n", snd_strerror(rc)); return rc; }

    return 0;
}

static void *ss_alsa_thread(void *arg) {
    ss_audio_t *a = (ss_audio_t *)arg;
    int rc_p = 0;
    int rc_c = 0;
    
    while (a->running) {
        if (a->play_cb) {
            a->play_cb(a->play_buf, (int)a->buf_frames, a->s_rate, a->n_ch, a->udata);
            rc_p = snd_pcm_writei(a->play_h, a->play_buf, a->buf_frames);
            if (rc_p < 0 && rc_p != -EPIPE) break;
            if (rc_p == -EPIPE) snd_pcm_prepare(a->play_h);
        }
        
        if (a->rec_cb) {
            rc_c = snd_pcm_readi(a->rec_h, a->rec_buf, a->buf_frames);
            if (rc_c < 0 && rc_c != -EPIPE) break;
            if (rc_c == -EPIPE) snd_pcm_prepare(a->rec_h);
            a->rec_cb(a->rec_buf, (int)a->buf_frames, a->s_rate, a->n_ch, a->udata);
        }
        
        if (!a->play_cb && !a->rec_cb) {
            usleep((useconds_t)((double)a->buf_frames * 1000000.0 / a->s_rate));
        }
    }
    
    return NULL;
}

ss_audio_t *ss_open_audio(ss_audio_cb_t play_cb, ss_audio_cb_t rec_cb, int s_rate, int n_ch, int buf_frames, void *udata) {
    ss_audio_t *a;
    snd_pcm_uframes_t alsa_frames = (snd_pcm_uframes_t)buf_frames;
    
    a = (ss_audio_t *)malloc(sizeof(ss_audio_t));
    if (!a) return NULL;
    memset(a, 0, sizeof(ss_audio_t));

    a->s_rate = s_rate;
    a->n_ch = n_ch;
    a->play_cb = play_cb;
    a->rec_cb = rec_cb;
    a->udata = udata;
    a->running = 1;
    a->buf_frames = alsa_frames;
    a->buf_size_b = buf_frames * n_ch * sizeof(short);

    if (play_cb) {
        if (ss_alsa_setup(&a->play_h, "default", SND_PCM_STREAM_PLAYBACK, s_rate, n_ch, &alsa_frames) < 0) {
            ss_close_audio(a);
            return NULL;
        }
        a->buf_frames = alsa_frames;
        a->buf_size_b = (int)a->buf_frames * n_ch * sizeof(short);
        a->play_buf = (short *)malloc(a->buf_size_b);
        if (!a->play_buf) {
            ss_close_audio(a);
            return NULL;
        }
    }

    if (rec_cb) {
        if (ss_alsa_setup(&a->rec_h, "default", SND_PCM_STREAM_CAPTURE, s_rate, n_ch, &alsa_frames) < 0) {
            ss_close_audio(a);
            return NULL;
        }
        a->buf_frames = alsa_frames;
        a->buf_size_b = (int)a->buf_frames * n_ch * sizeof(short);
        a->rec_buf = (short *)malloc(a->buf_size_b);
        if (!a->rec_buf) {
            ss_close_audio(a);
            return NULL;
        }
        snd_pcm_start(a->rec_h);
    }
    
    if (!play_cb && !rec_cb) {
        free(a);
        return NULL;
    }

    a->a_thread = ss_thread_create(ss_alsa_thread, a);
    if (!a->a_thread) { ss_close_audio(a); return NULL; }
        
    return a;
}

void ss_close_audio(ss_audio_t *a) {
    if (!a) return;
    
    a->running = 0;
    if (a->a_thread) ss_thread_join(a->a_thread);
    
    if (a->play_h) {
        snd_pcm_close(a->play_h);
        if (a->play_buf) free(a->play_buf);
    }
    
    if (a->rec_h) {
        snd_pcm_close(a->rec_h);
        if (a->rec_buf) free(a->rec_buf);
    }
    
    free(a);
}

#endif

#endif

#endif
