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

typedef struct ss_audio_t {
    int s_rate; 
} ss_audio_t;

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

struct ss_audio_internal {
    int s_rate; 

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

    wnd = (ss_window_t *)malloc(sizeof(struct ss_window_t));
    if (!wnd) return NULL;
    memset(wnd, 0, sizeof(struct ss_window_t));

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
    struct ss_audio_internal *a;
    UINT32 pad;
    UINT32 n_avail;
    BYTE *pData;
    HRESULT hr;
    int p_active;
    int r_active;
    HANDLE w_hndl[2];
    DWORD w_res;
    int n_w_hndl;
    UINT32 n_to_read;
    DWORD flags;

    a = (struct ss_audio_internal *)arg;
    p_active = a->play_cb != NULL;
    r_active = a->rec_cb != NULL;
    n_w_hndl = 0;

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

const char* HResultToString(HRESULT hr) {
    switch (hr) {
        case S_OK: return "S_OK";
        case E_FAIL: return "E_FAIL";
        case E_INVALIDARG: return "E_INVALIDARG";
        case AUDCLNT_E_DEVICE_INVALIDATED: return "AUDCLNT_E_DEVICE_INVALIDATED";
        case AUDCLNT_E_UNSUPPORTED_FORMAT: return "AUDCLNT_E_UNSUPPORTED_FORMAT";
        case AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED: return "AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED";
        case AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED: return "AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED";
        default: return "Unknown HRESULT";
    }
}


static HRESULT ss_was_init(
    struct ss_audio_internal *a,
    EDataFlow flow,
    AUDCLNT_SHAREMODE mode,
    IAudioClient **ppAC,
    UINT32 *pBufFrCount,
    HANDLE *phEvt,
    void **ppClient
) {
    WAVEFORMATEX *pWfx_mix = NULL;
    WAVEFORMATEX *pWfx_use = NULL;
    WAVEFORMATEX wfx_candidate = {0};
    REFERENCE_TIME hnsReqDur;
    IMMDevice *pDev;
    HRESULT hr;
    
    // --- 1. Activate IAudioClient ---
    pDev = (flow == eRender) ? a->pDev_p : a->pDev_c;

    if (!pDev) {
        fprintf(stderr, "[SS_AUDIO_ERROR] Device pointer is NULL for flow %s.\n", (flow == eRender) ? "eRender" : "eCapture");
        return S_FALSE;
    }
    fprintf(stderr, "[SS_AUDIO_DEBUG] Activating IAudioClient for device...\n");
    hr = IMMDevice_Activate(pDev, &IID_IAudioClient, CLSCTX_ALL, NULL, (void **)ppAC);
    if (FAILED(hr)) {
        fprintf(stderr, "[SS_AUDIO_ERROR] IMMDevice_Activate failed: %s (0x%lX)\n", HResultToString(hr), hr);
        return hr;
    }
    fprintf(stderr, "[SS_AUDIO_DEBUG] IAudioClient activated.\n");

    // --- 2. Query Mix Format (System Default) ---
    fprintf(stderr, "[SS_AUDIO_DEBUG] Querying Mix Format...\n");
    hr = IAudioClient_GetMixFormat(*ppAC, &pWfx_mix);
    if (FAILED(hr)) { 
        fprintf(stderr, "[SS_AUDIO_ERROR] GetMixFormat failed: %s (0x%lX)\n", HResultToString(hr), hr);
        IAudioClient_Release(*ppAC); *ppAC = NULL; return hr; 
    }
    
    // Set Chronos context parameters based on Mix Format
    a->s_rate = (int)pWfx_mix->nSamplesPerSec; 
    a->n_ch = pWfx_mix->nChannels; 
    
    if (a->s_rate != 44100) {
        fprintf(stderr, "[SS_AUDIO_WARN] System is using %d Hz Mix Rate. Chronos context will be updated.\n", a->s_rate);
    }
    fprintf(stderr, "[SS_AUDIO_DEBUG] Device Mix Format: Rate=%d Hz, Channels=%d, Bits=%d\n", 
            a->s_rate, a->n_ch, pWfx_mix->wBitsPerSample);

    // --- 3. Try 16-bit PCM (Preferred for short* buffer) ---
    // Construct a candidate format: 16-bit PCM, using the device's actual rate and channel count.
    wfx_candidate.wFormatTag = WAVE_FORMAT_PCM;
    wfx_candidate.nChannels = a->n_ch;
    wfx_candidate.nSamplesPerSec = a->s_rate;
    wfx_candidate.wBitsPerSample = 16;
    wfx_candidate.nBlockAlign = wfx_candidate.nChannels * (wfx_candidate.wBitsPerSample / 8);
    wfx_candidate.nAvgBytesPerSec = wfx_candidate.nSamplesPerSec * wfx_candidate.nBlockAlign;
    wfx_candidate.cbSize = 0; // Not using WAVEFORMATEXTENSIBLE here

    // Check if the 16-bit PCM format is supported
    hr = IAudioClient_IsFormatSupported(*ppAC, mode, &wfx_candidate, &pWfx_use);
    
    if (hr == S_OK) {
        // --- 4a. 16-bit PCM Supported ---
        // The simple 16-bit format is supported (often necessary if the system mix is 24/32-bit float).
        fprintf(stderr, "[SS_AUDIO_DEBUG] 16-bit PCM format supported. Using candidate format.\n");
        pWfx_use = &wfx_candidate;
    } else {
        // --- 4b. Use Mix Format (Let system handle conversion) ---
        // If 16-bit is not directly supported, use the original mix format. WASAPI should handle
        // the required conversion (e.g., 24-bit system mix -> 16-bit app buffer).
        fprintf(stderr, "[SS_AUDIO_DEBUG] 16-bit PCM not directly supported (HR: 0x%lX). Falling back to Mix Format.\n", hr);
        pWfx_use = pWfx_mix;
    }
    
    // --- 5. Calculate Duration and Initialize ---
    // Use the actual device rate (a->s_rate) for the required duration.
    hnsReqDur = (REFERENCE_TIME)a->buf_frames * 10000000L / a->s_rate; 
    fprintf(stderr, "[SS_AUDIO_DEBUG] Requested Buffer Duration: %lldns\n", hnsReqDur);


    fprintf(stderr, "[SS_AUDIO_DEBUG] Creating Event Handle...\n");
    *phEvt = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!*phEvt) { 
        CoTaskMemFree(pWfx_mix); IAudioClient_Release(*ppAC); *ppAC = NULL; return E_FAIL; 
    }

    fprintf(stderr, "[SS_AUDIO_DEBUG] Initializing IAudioClient...\n");
    // Initialize with the chosen format (pWfx_use)
    hr = IAudioClient_Initialize(*ppAC, mode, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, hnsReqDur, 0, pWfx_use, NULL);
    
    CoTaskMemFree(pWfx_mix); // Free the mix format memory
    if (pWfx_use != &wfx_candidate && pWfx_use != pWfx_mix) CoTaskMemFree(pWfx_use); // Free format supported memory if needed

    if (FAILED(hr)) { 
        fprintf(stderr, "[SS_AUDIO_ERROR] IAudioClient_Initialize failed: %s (0x%lX)\n", HResultToString(hr), hr);
        CloseHandle(*phEvt); *phEvt = NULL; IAudioClient_Release(*ppAC); *ppAC = NULL; return hr; 
    }
    fprintf(stderr, "[SS_AUDIO_DEBUG] IAudioClient initialized successfully.\n");


    fprintf(stderr, "[SS_AUDIO_DEBUG] Setting Event Handle...\n");
    hr = IAudioClient_SetEventHandle(*ppAC, *phEvt);
    if (FAILED(hr)) { 
        fprintf(stderr, "[SS_AUDIO_ERROR] SetEventHandle failed: %s (0x%lX)\n", HResultToString(hr), hr);
        CloseHandle(*phEvt); *phEvt = NULL; IAudioClient_Release(*ppAC); *ppAC = NULL; return hr; 
    }

    fprintf(stderr, "[SS_AUDIO_DEBUG] Getting Buffer Size...\n");
    hr = IAudioClient_GetBufferSize(*ppAC, pBufFrCount);
    if (FAILED(hr)) { 
        fprintf(stderr, "[SS_AUDIO_ERROR] GetBufferSize failed: %s (0x%lX)\n", HResultToString(hr), hr);
        CloseHandle(*phEvt); *phEvt = NULL; IAudioClient_Release(*ppAC); *ppAC = NULL; return hr; 
    }

    fprintf(stderr, "[SS_AUDIO_DEBUG] Getting Service Client...\n");
    if (flow == eRender) {
        hr = IAudioClient_GetService(*ppAC, &IID_IAudioRenderClient, ppClient);
    } else {
        hr = IAudioClient_GetService(*ppAC, &IID_IAudioCaptureClient, ppClient);
    }
    
    if (FAILED(hr)) { 
        fprintf(stderr, "[SS_AUDIO_ERROR] GetService failed: %s (0x%lX)\n", HResultToString(hr), hr);
        CloseHandle(*phEvt); *phEvt = NULL; IAudioClient_Release(*ppAC); *ppAC = NULL; return hr; 
    }
    
    fprintf(stderr, "[SS_AUDIO_DEBUG] ss_was_init finished successfully.\n");
    return hr;
}

ss_audio_t *ss_open_audio(ss_audio_cb_t play_cb, ss_audio_cb_t rec_cb, int s_rate, int n_ch, int buf_frames, void *udata) {
    struct ss_audio_internal *a;
    HRESULT hr;
    
    fprintf(stderr, "[SS_AUDIO_DEBUG] Entering ss_open_audio. Attempting allocation...\n");
    a = (struct ss_audio_internal *)malloc(sizeof(struct ss_audio_internal)); 
    if (!a) { 
        fprintf(stderr, "[SS_AUDIO_ERROR] Failed to allocate ss_audio_internal memory.\n");
        return NULL; 
    }
    
    /* FIX: Initializer list fix */
    a->s_rate = s_rate;
    a->n_ch = n_ch;
    a->play_cb = play_cb;
    a->rec_cb = rec_cb;
    a->udata = udata;
    a->buf_frames = buf_frames;
    a->running = 1;
    a->a_thread = NULL;
    a->pEnum = NULL;
    a->pDev_p = NULL;
    a->pDev_c = NULL;
    a->pAC_p = NULL;
    a->pRC = NULL;
    a->hEvt_p = NULL;
    a->pAC_c = NULL;
    a->pCC = NULL;
    a->hEvt_c = NULL;


    fprintf(stderr, "[SS_AUDIO_DEBUG] Initializing COM with COINIT_APARTMENTTHREADED...\n");
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED); 
    if (FAILED(hr)) { 
        fprintf(stderr, "[SS_AUDIO_ERROR] CoInitializeEx failed: %s (0x%lX)\n", HResultToString(hr), hr);
        free(a); 
        return NULL; 
    }
    fprintf(stderr, "[SS_AUDIO_DEBUG] CoInitializeEx successful.\n");

    fprintf(stderr, "[SS_AUDIO_DEBUG] Creating IMMDeviceEnumerator instance...\n");
    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, (void**)&a->pEnum);
    if (FAILED(hr)) { 
        fprintf(stderr, "[SS_AUDIO_ERROR] CoCreateInstance IMMDeviceEnumerator failed: %s (0x%lX)\n", HResultToString(hr), hr);
        CoUninitialize(); 
        free(a); 
        return NULL; 
    }
    fprintf(stderr, "[SS_AUDIO_DEBUG] IMMDeviceEnumerator created successfully.\n");

    if (play_cb) {
        fprintf(stderr, "[SS_AUDIO_DEBUG] Getting Playback (eRender) default endpoint...\n");
        hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(a->pEnum, eRender, eConsole, &a->pDev_p);
        if (FAILED(hr)) { 
            fprintf(stderr, "[SS_AUDIO_ERROR] GetDefaultAudioEndpoint eRender failed: %s (0x%lX)\n", HResultToString(hr), hr);
            ss_close_audio((ss_audio_t*)a); 
            return NULL; 
        }
        
        hr = ss_was_init(a, eRender, AUDCLNT_SHAREMODE_SHARED, &a->pAC_p, &a->buf_fr_c_p, &a->hEvt_p, (void**)&a->pRC);
        if (FAILED(hr)) { 
            fprintf(stderr, "[SS_AUDIO_ERROR] Playback initialization failed. Check ss_was_init errors above.\n");
            ss_close_audio((ss_audio_t*)a); 
            return NULL; 
        }
    }

    if (rec_cb) {
        fprintf(stderr, "[SS_AUDIO_DEBUG] Getting Capture (eCapture) default endpoint...\n");
        hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(a->pEnum, eCapture, eConsole, &a->pDev_c);
        if (FAILED(hr)) { 
            fprintf(stderr, "[SS_AUDIO_ERROR] GetDefaultAudioEndpoint eCapture failed: %s (0x%lX)\n", HResultToString(hr), hr);
            ss_close_audio((ss_audio_t*)a); 
            return NULL; 
        }

        hr = ss_was_init(a, eCapture, AUDCLNT_SHAREMODE_SHARED, &a->pAC_c, &a->buf_fr_c_c, &a->hEvt_c, (void**)&a->pCC);
        if (FAILED(hr)) { 
            fprintf(stderr, "[SS_AUDIO_ERROR] Capture initialization failed. Check ss_was_init errors above.\n");
            ss_close_audio((ss_audio_t*)a); 
            return NULL; 
        }
    }

    if (play_cb && a->pAC_p) { fprintf(stderr, "[SS_AUDIO_DEBUG] Starting Playback stream.\n"); IAudioClient_Start(a->pAC_p); }
    if (rec_cb && a->pAC_c) { fprintf(stderr, "[SS_AUDIO_DEBUG] Starting Capture stream.\n"); IAudioClient_Start(a->pAC_c); }

    fprintf(stderr, "[SS_AUDIO_DEBUG] Creating audio thread...\n");
    a->a_thread = ss_thread_create(ss_was_thread, a);
    if (!a->a_thread) { 
        fprintf(stderr, "[SS_AUDIO_ERROR] Failed to create audio thread.\n");
        ss_close_audio((ss_audio_t*)a); 
        return NULL; 
    }
    fprintf(stderr, "[SS_AUDIO_DEBUG] Exiting ss_open_audio successfully.\n");


    return (ss_audio_t*)a;
}

void ss_close_audio(ss_audio_t *a) {
    struct ss_audio_internal *internal_a = (struct ss_audio_internal*)a;
    if (!internal_a) return;

    fprintf(stderr, "[SS_AUDIO_DEBUG] Closing audio system...\n");
    internal_a->running = 0;
    if (internal_a->a_thread) ss_thread_join(internal_a->a_thread);

    if (internal_a->pAC_p) IAudioClient_Stop(internal_a->pAC_p);
    if (internal_a->pAC_c) IAudioClient_Stop(internal_a->pAC_c);

    if (internal_a->hEvt_p) CloseHandle(internal_a->hEvt_p);
    if (internal_a->hEvt_c) CloseHandle(internal_a->hEvt_c);

    if (internal_a->pRC) IAudioRenderClient_Release(internal_a->pRC);
    if (internal_a->pCC) IAudioCaptureClient_Release(internal_a->pCC);

    if (internal_a->pAC_p) IAudioClient_Release(internal_a->pAC_p);
    if (internal_a->pAC_c) IAudioClient_Release(internal_a->pAC_c);

    if (internal_a->pDev_p) IMMDevice_Release(internal_a->pDev_p);
    if (internal_a->pDev_c) IMMDevice_Release(internal_a->pDev_c);
    if (internal_a->pEnum) IMMDeviceEnumerator_Release(internal_a->pEnum);

    CoUninitialize();
    free(internal_a);
    fprintf(stderr, "[SS_AUDIO_DEBUG] Audio system closed.\n");
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

struct ss_audio_internal {
    int s_rate; 

    ss_thread_t a_thread;
    int running;

    snd_pcm_t *play_h;
    snd_pcm_t *rec_h;
    snd_pcm_uframes_t buf_frames;

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

    wnd = (ss_window_t *)malloc(sizeof(struct ss_window_t));
    if (!wnd) return NULL;
    memset(wnd, 0, sizeof(struct ss_window_t));

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
    struct ss_audio_internal *a = (struct ss_audio_internal *)arg;
    int rc_p;
    int rc_c;

    rc_p = 0;
    rc_c = 0;
    
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
    struct ss_audio_internal *a;
    snd_pcm_uframes_t alsa_frames;
    
    alsa_frames = (snd_pcm_uframes_t)buf_frames;
    
    a = (struct ss_audio_internal *)malloc(sizeof(struct ss_audio_internal));
    if (!a) return NULL;
    
    /* FIX: Initializer list fix */
    a->s_rate = s_rate;
    a->n_ch = n_ch;
    a->play_cb = play_cb;
    a->rec_cb = rec_cb;
    a->udata = udata;
    a->running = 1;
    a->buf_frames = alsa_frames;
    a->buf_size_b = buf_frames * n_ch * sizeof(short);

    a->a_thread = NULL;
    a->play_h = NULL;
    a->rec_h = NULL;
    a->play_buf = NULL;
    a->rec_buf = NULL;


    if (play_cb) {
        if (ss_alsa_setup(&a->play_h, "default", SND_PCM_STREAM_PLAYBACK, s_rate, n_ch, &alsa_frames) < 0) {
            ss_close_audio((ss_audio_t*)a);
            return NULL;
        }
        a->buf_frames = alsa_frames;
        a->buf_size_b = (int)a->buf_frames * n_ch * sizeof(short);
        a->play_buf = (short *)malloc(a->buf_size_b);
        if (!a->play_buf) {
            ss_close_audio((ss_audio_t*)a);
            return NULL;
        }
    }

    if (rec_cb) {
        if (ss_alsa_setup(&a->rec_h, "default", SND_PCM_STREAM_CAPTURE, s_rate, n_ch, &alsa_frames) < 0) {
            ss_close_audio((ss_audio_t*)a);
            return NULL;
        }
        a->buf_frames = alsa_frames;
        a->buf_size_b = (int)a->buf_frames * n_ch * sizeof(short);
        a->rec_buf = (short *)malloc(a->buf_size_b);
        if (!a->rec_buf) {
            ss_close_audio((ss_audio_t*)a);
            return NULL;
        }
        snd_pcm_start(a->rec_h);
    }
    
    if (!play_cb && !rec_cb) {
        free(a);
        return NULL;
    }

    a->a_thread = ss_thread_create(ss_alsa_thread, a);
    if (!a->a_thread) { ss_close_audio((ss_audio_t*)a); return NULL; }
        
    return (ss_audio_t*)a;
}

void ss_close_audio(ss_audio_t *a) {
    struct ss_audio_internal *internal_a = (struct ss_audio_internal*)a;
    if (!internal_a) return;

    internal_a->running = 0;
    if (internal_a->a_thread) ss_thread_join(internal_a->a_thread);

    if (internal_a->play_h) {
        snd_pcm_close(internal_a->play_h);
        if (internal_a->play_buf) free(internal_a->play_buf);
    }
    
    if (internal_a->rec_h) {
        snd_pcm_close(internal_a->rec_h);
        if (internal_a->rec_buf) free(internal_a->rec_buf);
    }
    
    free(internal_a);
}
#endif
#endif
#endif