#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <windowsx.h>

#define ARG_PARSE_IMPLEMENTATION
#define PNGL_IMPLEMENTATION
#define PLATFORM_IMPLEMENTATION

#include "libs\args.h"
#include "libs\plat.h"
#include "libs\pngl.h"

#include <ShlObj.h>

#define WINDOW_CLASS_NAME "IMAGE_VIEWER_CLASS"
#define TIMER_ID_UPDATE 1
#define TIMER_INTERVAL_MS 500
#define TIMER_ID_CLIPBOARD 2
#define TIMER_INTERVAL_CLIPBOARD_MS 200

#define IDM_COPY_HEX            1001
#define IDM_COPY_RGBA           1002
#define IDM_COPY_NORMALIZED     1003
#define IDM_SEPARATOR           1004
#define IDM_COPY_IMAGE          1005
#define IDM_PASTE_IMAGE         1006


typedef struct {
    const char *image_path;
} AppState;

static pngl_uc *g_image_data = NULL;
static int g_width = 0;
static int g_height = 0;
static int g_comp = 0;
static char g_window_title[MAX_PATH_CUSTOM + 32];
static char g_image_path[MAX_PATH_CUSTOM];

static float g_scale = 1.0f;
static float g_offset_x = 0.0f;
static float g_offset_y = 0.0f;

static int g_background_mode = 0;

// New global for drawing mode
// 0: Normal (default)
// 1: Tiled / Repeat
// 2: Scaled-to-Fit
static int g_draw_mode = 0; 

static int g_mouse_down = 0;
static int g_last_mouse_x = 0;
static int g_last_mouse_y = 0;

static long g_last_modified_size = 0;
static long long g_last_modified_time = 0;

static HBITMAP g_hBitmap_mem = NULL;
static HDC g_hdc_mem = NULL;
static HBITMAP g_hBitmap_old = NULL;
static int g_mem_w = 0;
static int g_mem_h = 0;

static HMENU g_hContextMenu = NULL;
static int g_paste_image_enabled = 0;


int handle_image_path(ArgParseState *state, char **argv, int argc, int *i_ptr) {
    AppState *app = (AppState *)state;
    return argparse_parse_string(argv, argc, i_ptr, &app->image_path, "input-image");
}

ArgOption option_table[] = {
    {"-i", "--input", handle_image_path, "Specify the input image file path <val>", 1, 0},
    {"-h", "--help", (ArgParseFunc)-1, "Display this help message", 0, 0},
    {NULL, NULL, NULL, NULL, 0, 0}
};

int file_read_clbk(void *user, char *data, int size) {
    FILE *f = (FILE *)user;
    return (int)fread(data, 1, (size_t)size, f);
}

void file_skip_clbk(void *user, int n) {
    FILE *f = (FILE *)user;
    fseek(f, (long)n, SEEK_CUR);
}

int file_eof_clbk(void *user) {
    FILE *f = (FILE *)user;
    return feof(f);
}

void process_image_data(pngl_uc *data, int w, int h) {
    int i;
    int pixel_count = w * h;
    for (i = 0; i < pixel_count; ++i) {
        pngl_uc *pixel = data + i * 4;
        
        pngl_uc b = pixel[2];
        pngl_uc g = pixel[1];
        pngl_uc r = pixel[0];
        pngl_uc a = pixel[3];
        
        if (a > 0) {
            pixel[0] = (pngl_uc)(((unsigned int)b * a) / 255);
            pixel[1] = (pngl_uc)(((unsigned int)g * a) / 255);
            pixel[2] = (pngl_uc)(((unsigned int)r * a) / 255);
        } else {
            pixel[0] = 0;
            pixel[1] = 0;
            pixel[2] = 0;
        }
    }
}

void reset_view(HWND hWnd) {
    RECT client_rect;
    GetClientRect(hWnd, &client_rect);
    int client_w = client_rect.right;
    int client_h = client_rect.bottom;
    
    g_scale = 1.0f;
    g_offset_x = (float)(client_w - g_width) / 2.0f;
    g_offset_y = (float)(client_h - g_height) / 2.0f;
    InvalidateRect(hWnd, NULL, FALSE);
}

int load_image_from_path(HWND hWnd, const char *path) {
    FILE *f;
    pngl_uc *new_image_data = NULL;
    pngl_io_callbacks callbacks;
    int new_width, new_height, new_comp;

    if (strlen(path) >= MAX_PATH_CUSTOM) {
        OutputDebugString("ERROR: File path is too long.\n");
        return 0;
    }
    
    f = fopen(path, "rb");
    if (f == NULL) {
        OutputDebugString("ERROR: Could not open image file.\n");
        return 0;
    }

    callbacks.read = file_read_clbk;
    callbacks.skip = file_skip_clbk;
    callbacks.eof = file_eof_clbk;

    new_image_data = pngl_load_from_callbacks(&callbacks, f, &new_width, &new_height, &new_comp, PNGL_rgb_alpha);
    fclose(f);

    if (new_image_data == NULL) {
        OutputDebugString("ERROR: Failed to load image data from file.\n");
        return 0;
    }
    
    if (g_image_data != NULL) {
        pngl_image_free(g_image_data);
    }
    
    process_image_data(new_image_data, new_width, new_height);
    g_image_data = new_image_data;
    g_width = new_width;
    g_height = new_height;

    strcpy(g_image_path, path);
    platform_normalize_path(g_image_path);
    g_last_modified_size = platform_file_size(g_image_path);

    sprintf(g_window_title, "Image Viewer: %dx%d - %s", g_width, g_height, g_image_path);
    SetWindowText(hWnd, g_window_title);
    
    reset_view(hWnd);
    return 1;
}

int reload_image_data(HWND hWnd) {
    FILE *f;
    pngl_uc *new_image_data;
    pngl_io_callbacks callbacks;
    int new_width, new_height, new_comp;
    
    f = fopen(g_image_path, "rb");
    if (f == NULL) {
        if (g_image_data != NULL) {
            OutputDebugString("ERROR: Could not reopen image file during update.\n");
        }
        return 0;
    }

    callbacks.read = file_read_clbk;
    callbacks.skip = file_skip_clbk;
    callbacks.eof = file_eof_clbk;

    new_image_data = pngl_load_from_callbacks(&callbacks, f, &new_width, &new_height, &new_comp, PNGL_rgb_alpha);

    fclose(f);

    if (new_image_data == NULL) {
        OutputDebugString("ERROR: Failed to reload image data.\n");
        return 0;
    }
    
    if (g_image_data != NULL) {
        pngl_image_free(g_image_data);
    }
    
    process_image_data(new_image_data, new_width, new_height);
    g_image_data = new_image_data;
    g_width = new_width;
    g_height = new_height;
    
    sprintf(g_window_title, "Image Viewer: %dx%d - %s (Updated)", g_width, g_height, g_image_path);
    SetWindowText(hWnd, g_window_title);
    
    InvalidateRect(hWnd, NULL, FALSE);
    return 1;
}

void check_for_file_updates(HWND hWnd) {
    long current_size;
    
    if (g_image_path[0] == '\0') return;

    current_size = platform_file_size(g_image_path);

    if (current_size != g_last_modified_size) {
        if (reload_image_data(hWnd)) {
            g_last_modified_size = current_size;
        }
    }
}

void draw_background(HDC hdc, const RECT *rect, int mode) {
    COLORREF color;
    HBRUSH brush = NULL;
    int i, j;
    int tile_size = 16;
    RECT tile_rect;
    
    switch (mode) {
        case 0:
        case 9:
            for (i = rect->left; i < rect->right; i += tile_size) {
                for (j = rect->top; j < rect->bottom; j += tile_size) {
                    if (((i / tile_size) + (j / tile_size)) % 2 == 0) {
                        color = (mode == 0) ? RGB(192, 192, 192) : RGB(48, 48, 48);
                    } else {
                        color = (mode == 0) ? RGB(128, 128, 128) : RGB(80, 80, 80);
                    }
                    brush = CreateSolidBrush(color);
                    
                    tile_rect.left = i;
                    tile_rect.top = j;
                    tile_rect.right = i + tile_size;
                    tile_rect.bottom = j + tile_size;
                    
                    FillRect(hdc, &tile_rect, brush);
                    DeleteObject(brush);
                }
            }
            return;
            
        case 1: color = RGB(0, 0, 0); break;
        case 2: color = RGB(255, 255, 255); break;
        case 3: color = RGB(255, 0, 0); break;
        case 4: color = RGB(0, 255, 0); break;
        case 5: color = RGB(0, 0, 255); break;
        case 6: color = RGB(255, 0, 255); break;
        case 7: color = RGB(255, 255, 0); break;
        case 8: color = RGB(0, 255, 255); break;

        default: color = RGB(0, 0, 0); break;
    }
    
    brush = CreateSolidBrush(color);
    FillRect(hdc, rect, brush);
    DeleteObject(brush);
}

void cleanup_gdi_objects(void) {
    if (g_hdc_mem != NULL && g_hBitmap_mem != NULL) {
        SelectObject(g_hdc_mem, g_hBitmap_old);
    }
    if (g_hdc_mem != NULL) {
        DeleteDC(g_hdc_mem);
        g_hdc_mem = NULL;
    }
    if (g_hBitmap_mem != NULL) {
        DeleteObject(g_hBitmap_mem);
        g_hBitmap_mem = NULL;
    }
    g_hBitmap_old = NULL;
    g_mem_w = 0;
    g_mem_h = 0;
}

void do_double_buffered_paint(HWND hWnd) {
    PAINTSTRUCT ps;
    HDC hdc_screen;
    RECT client_rect;
    int client_w, client_h;
    
    hdc_screen = BeginPaint(hWnd, &ps);
    GetClientRect(hWnd, &client_rect);

    client_w = client_rect.right;
    client_h = client_rect.bottom;

    if (g_hdc_mem == NULL || g_hBitmap_mem == NULL || g_mem_w != client_w || g_mem_h != client_h) {
        cleanup_gdi_objects();
        
        g_hdc_mem = CreateCompatibleDC(hdc_screen);
        g_hBitmap_mem = CreateCompatibleBitmap(hdc_screen, client_w, client_h);
        g_hBitmap_old = SelectObject(g_hdc_mem, g_hBitmap_mem);
        
        g_mem_w = client_w;
        g_mem_h = client_h;
    } else {
        SelectObject(g_hdc_mem, g_hBitmap_mem);
    }
    
    draw_background(g_hdc_mem, &client_rect, g_background_mode); 

    if (g_image_data != NULL) {
        int scaled_width = (int)((float)g_width * g_scale);
        int scaled_height = (int)((float)g_height * g_scale);
        int dest_x = (int)g_offset_x;
        int dest_y = (int)g_offset_y;
        
        int draw_w, draw_h, draw_x, draw_y;
        int tile_x_start = 0, tile_y_start = 0;
        int tile_x_end = 0, tile_y_end = 0;

        HDC hdcSrc = CreateCompatibleDC(g_hdc_mem);
        void *pBits;
        HBITMAP hBitmapSrc;
        BITMAPINFO bmi;

        memset(&bmi, 0, sizeof(BITMAPINFO));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = g_width;
        bmi.bmiHeader.biHeight = g_height; 
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        
        hBitmapSrc = CreateDIBSection(
            hdcSrc, 
            &bmi, 
            DIB_RGB_COLORS, 
            &pBits, 
            NULL, 
            0
        );

        if (hBitmapSrc != NULL) {
            HGDIOBJ hOldBitmap = SelectObject(hdcSrc, hBitmapSrc);
            
            size_t row_size = (size_t)g_width * 4;
            for (int i = 0; i < g_height; i++) {
                memcpy((char *)pBits + (g_height - 1 - i) * row_size, 
                       (char *)g_image_data + i * row_size, 
                       row_size);
            }

            BLENDFUNCTION bf;
            bf.BlendOp = AC_SRC_OVER;
            bf.BlendFlags = 0;
            bf.SourceConstantAlpha = 255;
            bf.AlphaFormat = AC_SRC_ALPHA;
            
            // --- DRAW MODE LOGIC START ---
            switch (g_draw_mode) {
                case 0: // Normal (Pan & Zoom)
                default:
                    draw_w = scaled_width;
                    draw_h = scaled_height;
                    draw_x = dest_x;
                    draw_y = dest_y;
                    
                    AlphaBlend(
                        g_hdc_mem,
                        draw_x,
                        draw_y,
                        draw_w,
                        draw_h,
                        hdcSrc,
                        0, 
                        0, 
                        g_width, 
                        g_height,
                        bf
                    );
                    break;
                    
                case 1: // Tiled / Repeat
                    if (scaled_width > 0 && scaled_height > 0) {
                        tile_x_start = (int)floorf((float)(client_rect.left - dest_x) / scaled_width);
                        tile_y_start = (int)floorf((float)(client_rect.top - dest_y) / scaled_height);
                        tile_x_end = (int)ceilf((float)(client_rect.right - dest_x) / scaled_width);
                        tile_y_end = (int)ceilf((float)(client_rect.bottom - dest_y) / scaled_height);

                        for (int i = tile_x_start; i < tile_x_end; ++i) {
                            for (int j = tile_y_start; j < tile_y_end; ++j) {
                                AlphaBlend(
                                    g_hdc_mem,
                                    dest_x + i * scaled_width,
                                    dest_y + j * scaled_height,
                                    scaled_width,
                                    scaled_height,
                                    hdcSrc,
                                    0, 
                                    0, 
                                    g_width, 
                                    g_height,
                                    bf
                                );
                            }
                        }
                    } else {
                        // Handle case where scaled dimensions are zero (e.g. scale is too small)
                    }
                    break;
                    
                case 2: // Scaled-to-Fit (Zoomed to fit the window while maintaining aspect ratio)
                    {
                        float aspect_ratio = (float)g_width / (float)g_height;
                        float window_aspect = (float)client_w / (float)client_h;

                        if (aspect_ratio > window_aspect) { // Image is wider than window
                            draw_w = client_w;
                            draw_h = (int)((float)client_w / aspect_ratio);
                            draw_x = 0;
                            draw_y = (client_h - draw_h) / 2;
                        } else { // Image is taller or aspect ratio is the same
                            draw_h = client_h;
                            draw_w = (int)((float)client_h * aspect_ratio);
                            draw_x = (client_w - draw_w) / 2;
                            draw_y = 0;
                        }
                        
                        AlphaBlend(
                            g_hdc_mem,
                            draw_x,
                            draw_y,
                            draw_w,
                            draw_h,
                            hdcSrc,
                            0, 
                            0, 
                            g_width, 
                            g_height,
                            bf
                        );
                    }
                    break;
            }
            // --- DRAW MODE LOGIC END ---

            SelectObject(hdcSrc, hOldBitmap);
            DeleteObject(hBitmapSrc);
        }

        DeleteDC(hdcSrc);
    }
    
    BitBlt(hdc_screen, 
           0, 0, 
           client_w, client_h, 
           g_hdc_mem, 
           0, 0, 
           SRCCOPY);
           
    EndPaint(hWnd, &ps);
}

static int pixel_to_rgb_and_coords(HWND hWnd, int screen_x, int screen_y, 
                                   pngl_uc *r, pngl_uc *g, pngl_uc *b, pngl_uc *a,
                                   int *img_x, int *img_y) {
    float scaled_width, scaled_height;
    float image_tl_x, image_tl_y;
    int pixel_index;
    
    if (g_image_data == NULL) return 0;

    scaled_width = (float)g_width * g_scale;
    scaled_height = (float)g_height * g_scale;

    image_tl_x = g_offset_x;
    image_tl_y = g_offset_y;

    // The pixel logic is only straightforward in Normal (0) or Tiled (1) mode
    if (g_draw_mode == 0 || g_draw_mode == 1) {
        // For Tiled mode, we need to map the screen coordinates back into the original image's coordinates (0, 0) to (g_width-1, g_height-1)
        if (g_draw_mode == 1) {
            float rel_x = (float)screen_x - image_tl_x;
            float rel_y = (float)screen_y - image_tl_y;
            
            if (scaled_width == 0 || scaled_height == 0) return 0; // Avoid division by zero
            
            float tile_x_f = fmodf(rel_x, scaled_width);
            float tile_y_f = fmodf(rel_y, scaled_height);

            if (tile_x_f < 0.0f) tile_x_f += scaled_width;
            if (tile_y_f < 0.0f) tile_y_f += scaled_height;
            
            *img_x = (int)(tile_x_f / g_scale);
            *img_y = (int)(tile_y_f / g_scale);
            
            if (*img_x < 0 || *img_x >= g_width || *img_y < 0 || *img_y >= g_height) {
                return 0; // Should not happen with the fmodf logic, but safe to check
            }
        
        } else { // Normal mode
            if (screen_x < image_tl_x || screen_y < image_tl_y ||
                screen_x >= image_tl_x + scaled_width || screen_y >= image_tl_y + scaled_height) {
                return 0;
            }

            *img_x = (int)(((float)screen_x - image_tl_x) / g_scale);
            *img_y = (int)(((float)screen_y - image_tl_y) / g_scale);

            if (*img_x < 0 || *img_x >= g_width || *img_y < 0 || *img_y >= g_height) {
                return 0;
            }
        }
        
        pixel_index = (*img_y * g_width + *img_x) * 4;
        
        *b = g_image_data[pixel_index + 0];
        *g = g_image_data[pixel_index + 1];
        *r = g_image_data[pixel_index + 2];
        *a = g_image_data[pixel_index + 3];
        
        if (*a > 0) {
            *b = (pngl_uc)(((unsigned int)*b * 255) / *a);
            *g = (pngl_uc)(((unsigned int)*g * 255) / *a);
            *r = (pngl_uc)(((unsigned int)*r * 255) / *a);
        }

        return 1;
    }

    // For Scaled-to-Fit mode, pixel picking is not supported with the current logic.
    return 0;
}

void copy_text_to_clipboard(HWND hWnd, const char *text) {
    HGLOBAL hGlobal;
    char *pGlobal;
    size_t len = strlen(text) + 1;

    if (!OpenClipboard(hWnd)) return;
    EmptyClipboard();

    hGlobal = GlobalAlloc(GMEM_MOVEABLE, len);
    if (hGlobal == NULL) {
        CloseClipboard();
        return;
    }

    pGlobal = GlobalLock(hGlobal);
    if (pGlobal != NULL) {
        memcpy(pGlobal, text, len);
        GlobalUnlock(hGlobal);
        SetClipboardData(CF_TEXT, hGlobal);
    } else {
        GlobalFree(hGlobal);
    }

    CloseClipboard();
}

void copy_image_to_clipboard(HWND hWnd) {
    
    if (g_image_path[0] != '\0') {
        HGLOBAL hGlobal;
        char *pGlobal;
        size_t path_len = strlen(g_image_path);
        size_t total_size = sizeof(DROPFILES) + (path_len + 2) * sizeof(char); 

        hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, total_size);
        if (hGlobal != NULL) {
            pGlobal = GlobalLock(hGlobal);
            if (pGlobal != NULL) {
                DROPFILES *df = (DROPFILES *)pGlobal;
                char *path_start;
                
                df->pFiles = sizeof(DROPFILES);
                df->pt.x = 0;
                df->pt.y = 0;
                df->fNC = FALSE;
                df->fWide = FALSE;

                path_start = (char *)(df) + sizeof(DROPFILES);
                memcpy(path_start, g_image_path, path_len + 1);
                
                path_start[path_len + 1] = '\0';

                GlobalUnlock(hGlobal);

                if (OpenClipboard(hWnd)) {
                    EmptyClipboard();
                    SetClipboardData(CF_HDROP, hGlobal);
                    CloseClipboard();
                    return; 
                } else {
                    GlobalFree(hGlobal);
                }
            } else {
                GlobalFree(hGlobal);
            }
        }
    }

    
    if (g_image_data == NULL || g_width == 0 || g_height == 0) return;

    {
        BITMAPINFOHEADER bmih;
        HGLOBAL hGlobal;
        void *pGlobal;
        size_t data_size;
        size_t total_size;
        
        pngl_uc *clipboard_data = (pngl_uc *)malloc((size_t)g_width * g_height * 4);
        if (clipboard_data == NULL) return;
        
        for (int i = 0; i < g_width * g_height * 4; i++) {
            clipboard_data[i] = g_image_data[i];
        }

        for (int i = 0; i < g_width * g_height; ++i) {
            pngl_uc *pixel = clipboard_data + i * 4;
            
            pngl_uc b = pixel[0];
            pngl_uc g = pixel[1];
            pngl_uc r = pixel[2];
            pngl_uc a = pixel[3];
            
            if (a > 0) {
                pixel[0] = (pngl_uc)(((unsigned int)b * 255) / a);
                pixel[1] = (pngl_uc)(((unsigned int)g * 255) / a);
                pixel[2] = (pngl_uc)(((unsigned int)r * 255) / a);
            }
        }

        memset(&bmih, 0, sizeof(BITMAPINFOHEADER));
        bmih.biSize = sizeof(BITMAPINFOHEADER);
        bmih.biWidth = g_width;
        bmih.biHeight = g_height; 
        bmih.biPlanes = 1;
        bmih.biBitCount = 32;
        bmih.biCompression = BI_RGB;
        
        data_size = (size_t)g_width * g_height * 4;
        total_size = sizeof(BITMAPINFOHEADER) + data_size;
        
        hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, total_size);
        if (hGlobal == NULL) {
            free(clipboard_data);
            return;
        }

        pGlobal = GlobalLock(hGlobal);
        if (pGlobal != NULL) {
            memcpy(pGlobal, &bmih, sizeof(BITMAPINFOHEADER));
            
            {
                char *dest = (char *)pGlobal + sizeof(BITMAPINFOHEADER);
                char *src_row_start;
                size_t row_size = (size_t)g_width * 4;
                int i;
                
                for (i = 0; i < g_height; i++) {
                    src_row_start = (char *)clipboard_data + (g_height - 1 - i) * row_size;
                    memcpy(dest + i * row_size, src_row_start, row_size);
                }
            }
            
            GlobalUnlock(hGlobal);
            free(clipboard_data);

            if (OpenClipboard(hWnd)) {
                EmptyClipboard();
                SetClipboardData(CF_DIB, hGlobal);
                CloseClipboard();
            } else {
                GlobalFree(hGlobal);
            }
        } else {
            GlobalFree(hGlobal);
            free(clipboard_data);
        }
    }
}

pngl_uc* get_pixels_from_bitmap(HBITMAP hBitmap, int *w, int *h) {
    BITMAP bmp;
    BITMAPINFOHEADER bmih;
    pngl_uc *pixels = NULL;
    HDC hdc = NULL;
    int success = 0;

    if (hBitmap == NULL || GetObject(hBitmap, sizeof(BITMAP), &bmp) == 0) return NULL;
    
    *w = bmp.bmWidth;
    *h = bmp.bmHeight;
    
    pixels = (pngl_uc *)malloc(*w * *h * 4);
    if (pixels == NULL) return NULL;
    
    memset(&bmih, 0, sizeof(BITMAPINFOHEADER));
    bmih.biSize = sizeof(BITMAPINFOHEADER);
    bmih.biWidth = *w;
    bmih.biHeight = -(*h); 
    bmih.biPlanes = 1;
    bmih.biBitCount = 32;
    bmih.biCompression = BI_RGB;

    hdc = CreateCompatibleDC(NULL);
    if (hdc != NULL) {
        HBITMAP hOldBitmap = SelectObject(hdc, hBitmap);
        if (GetDIBits(hdc, hBitmap, 0, *h, pixels, (BITMAPINFO *)&bmih, DIB_RGB_COLORS) > 0) {
            success = 1;
        }
        SelectObject(hdc, hOldBitmap);
        DeleteDC(hdc);
    }

    if (success) {
        return pixels;
    } else {
        free(pixels);
        return NULL;
    }
}

void paste_image_from_clipboard(HWND hWnd) {
    HGLOBAL hGlobal = NULL;
    pngl_uc *new_image_data = NULL;
    int new_width = 0, new_height = 0;
    int success = 0;
    
    if (!OpenClipboard(hWnd)) return;

    
    hGlobal = GetClipboardData(CF_HDROP);
    if (hGlobal != NULL) {
        HDROP hDrop = (HDROP)GlobalLock(hGlobal);
        char path[MAX_PATH_CUSTOM];
        
        if (hDrop != NULL) {
            if (DragQueryFile(hDrop, 0, path, MAX_PATH_CUSTOM) > 0) {
                GlobalUnlock(hDrop);
                CloseClipboard();
                
                if (load_image_from_path(hWnd, path)) {
                    return; 
                }
                if (!OpenClipboard(hWnd)) return;
            }
        }
        if (hDrop != NULL) GlobalUnlock(hDrop);
    }
    
    
    hGlobal = GetClipboardData(CF_DIB);
    if (hGlobal != NULL) {
        BITMAPINFO *pBmi = (BITMAPINFO *)GlobalLock(hGlobal);
        if (pBmi != NULL) {
            BITMAPINFOHEADER *pBmih = &pBmi->bmiHeader;
            
            new_width = (int)pBmih->biWidth;
            new_height = (int)abs(pBmih->biHeight);
            
            if (pBmih->biBitCount == 32 && pBmih->biCompression == BI_RGB && new_width > 0 && new_height > 0) {
                char *pData = (char *)pBmi + pBmih->biSize + pBmih->biClrUsed * sizeof(RGBQUAD);
                size_t row_size = (size_t)new_width * 4;
                int i;
                
                new_image_data = (pngl_uc *)malloc(new_width * new_height * 4);
                if (new_image_data != NULL) {
                    if (pBmih->biHeight > 0) { 
                        for (i = 0; i < new_height; i++) {
                            memcpy(new_image_data + (new_height - 1 - i) * row_size, 
                                   pData + i * row_size, 
                                   row_size);
                        }
                    } else {
                        memcpy(new_image_data, pData, new_width * new_height * 4);
                    }
                    success = 1;
                }
            }
            GlobalUnlock(hGlobal);
        }
    } 
    
    
    if (!success) {
        HBITMAP hBitmap = (HBITMAP)GetClipboardData(CF_BITMAP);
        if (hBitmap != NULL) {
            new_image_data = get_pixels_from_bitmap(hBitmap, &new_width, &new_height);
            if (new_image_data != NULL) {
                success = 1;
            }
        }
    }
    
    CloseClipboard();

    if (success) {
        if (g_image_data != NULL) pngl_image_free(g_image_data);
        
        g_image_data = new_image_data;
        g_width = new_width;
        g_height = new_height;
        
        process_image_data(g_image_data, g_width, g_height);
        
        g_image_path[0] = '\0'; 
        g_last_modified_size = 0;
        
        sprintf(g_window_title, "Image Viewer: %dx%d - (Pasted from Clipboard)", g_width, g_height);
        SetWindowText(hWnd, g_window_title);
        
        reset_view(hWnd);
    } else if (new_image_data != NULL) {
        free(new_image_data);
    }
}

HMENU create_context_menu(void) {
    HMENU hMenu = CreatePopupMenu();
    if (hMenu == NULL) return NULL;
    
    AppendMenu(hMenu, MF_STRING, IDM_COPY_HEX, "Copy Hex (0xAARRGGBB)");
    AppendMenu(hMenu, MF_STRING, IDM_COPY_RGBA, "Copy RGBA (0-255)");
    AppendMenu(hMenu, MF_STRING, IDM_COPY_NORMALIZED, "Copy Normalized RGBA (0.0-1.0)");
    
    AppendMenu(hMenu, MF_SEPARATOR, IDM_SEPARATOR, NULL);
    
    AppendMenu(hMenu, MF_STRING, IDM_COPY_IMAGE, "Copy Image");
    
    AppendMenu(hMenu, MF_STRING, IDM_PASTE_IMAGE, "Paste Image");
    
    return hMenu;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int mouse_x, mouse_y;
    int delta;
    float old_scale, new_scale;
    float scale_factor;
    
    switch (message) {
        case WM_CREATE:
            SetWindowText(hWnd, g_window_title);
            SetTimer(hWnd, TIMER_ID_UPDATE, TIMER_INTERVAL_MS, NULL);
            SetTimer(hWnd, TIMER_ID_CLIPBOARD, TIMER_INTERVAL_CLIPBOARD_MS, NULL);
            g_hContextMenu = create_context_menu();
            DragAcceptFiles(hWnd, TRUE);
            break;

        case WM_TIMER:
            if (wParam == TIMER_ID_UPDATE) {
                check_for_file_updates(hWnd);
            } else if (wParam == TIMER_ID_CLIPBOARD) {
                int available = (IsClipboardFormatAvailable(CF_DIB) || IsClipboardFormatAvailable(CF_BITMAP) || IsClipboardFormatAvailable(CF_HDROP));
                if (available != g_paste_image_enabled) {
                    g_paste_image_enabled = available;
                }
            }
            break;
            
        case WM_DROPFILES:
        {
            HDROP hDrop = (HDROP)wParam;
            UINT count = DragQueryFile(hDrop, (UINT)-1, NULL, 0);
            char path[MAX_PATH_CUSTOM];
            
            if (count > 0) {
                if (DragQueryFile(hDrop, 0, path, MAX_PATH_CUSTOM) > 0) {
                    load_image_from_path(hWnd, path);
                }
            }
            DragFinish(hDrop);
        }
        break;

        case WM_KEYDOWN:
            if (wParam >= '0' && wParam <= '9') {
                g_background_mode = wParam - '0';
                InvalidateRect(hWnd, NULL, FALSE);
            } else if (wParam == 'R') {
                reset_view(hWnd);
            } else if (wParam == VK_TAB) { // Handle TAB key for cycling draw modes
                g_draw_mode = (g_draw_mode + 1) % 3; // 0 -> 1 -> 2 -> 0...
                
                // If the mode is changed to Scale-to-Fit (2), reset view parameters
                if (g_draw_mode == 2) {
                    g_scale = 1.0f;
                    g_offset_x = 0.0f;
                    g_offset_y = 0.0f;
                }
                
                InvalidateRect(hWnd, NULL, FALSE);
            } else if (GetKeyState(VK_CONTROL) & 0x8000) {
                if (wParam == 'C') {
                    copy_image_to_clipboard(hWnd);
                } else if (wParam == 'V') {
                    if (g_paste_image_enabled) {
                        paste_image_from_clipboard(hWnd);
                    }
                }
            }
            break;

        case WM_LBUTTONDOWN:
            g_mouse_down = 1;
            g_last_mouse_x = GET_X_LPARAM(lParam);
            g_last_mouse_y = GET_Y_LPARAM(lParam);
            SetCapture(hWnd);
            break;
            
        case WM_LBUTTONUP:
            g_mouse_down = 0;
            ReleaseCapture();
            break;

        case WM_MOUSEMOVE:
            if (g_mouse_down) {
                int current_x = GET_X_LPARAM(lParam);
                int current_y = GET_Y_LPARAM(lParam);
                
                // Only allow drag/pan in Normal (0) and Tiled (1) modes
                if (g_draw_mode == 0 || g_draw_mode == 1) {
                    g_offset_x += (float)(current_x - g_last_mouse_x);
                    g_offset_y += (float)(current_y - g_last_mouse_y);
                }
                
                g_last_mouse_x = current_x;
                g_last_mouse_y = current_y;
                
                RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
            }
            break;

        case WM_MOUSEWHEEL:
            delta = GET_WHEEL_DELTA_WPARAM(wParam);
            
            // Only allow zoom in Normal (0) and Tiled (1) modes
            if (g_image_data != NULL && (g_draw_mode == 0 || g_draw_mode == 1)) {
                old_scale = g_scale;
                scale_factor = (delta > 0) ? 1.2f : (1.0f / 1.2f);
                new_scale = g_scale * scale_factor;

                if (new_scale > 10.0f) new_scale = 10.0f;
                if (new_scale < 0.05f) new_scale = 0.05f;
                
                if (new_scale != old_scale) {
                    
                    mouse_x = GET_X_LPARAM(lParam);
                    mouse_y = GET_Y_LPARAM(lParam);
                    
                    float image_x_at_cursor = ((float)mouse_x - g_offset_x) / old_scale;
                    float image_y_at_cursor = ((float)mouse_y - g_offset_y) / old_scale;
                    
                    g_offset_x = (float)mouse_x - (image_x_at_cursor * new_scale);
                    g_offset_y = (float)mouse_y - (image_y_at_cursor * new_scale);
                    
                    g_scale = new_scale;
                    
                    RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
                }
            }
            break;
            
        case WM_CONTEXTMENU:
        {
            POINT pt;
            int cmd;
            pngl_uc r, g, b, a;
            int img_x, img_y;
            
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);

            if (g_hContextMenu == NULL) break;
            
            if (ScreenToClient(hWnd, &pt)) {
                
                if (pixel_to_rgb_and_coords(hWnd, pt.x, pt.y, &r, &g, &b, &a, &img_x, &img_y)) {
                    EnableMenuItem(g_hContextMenu, IDM_COPY_HEX, MF_ENABLED);
                    EnableMenuItem(g_hContextMenu, IDM_COPY_RGBA, MF_ENABLED);
                    EnableMenuItem(g_hContextMenu, IDM_COPY_NORMALIZED, MF_ENABLED);
                } else {
                    EnableMenuItem(g_hContextMenu, IDM_COPY_HEX, MF_GRAYED);
                    EnableMenuItem(g_hContextMenu, IDM_COPY_RGBA, MF_GRAYED);
                    EnableMenuItem(g_hContextMenu, IDM_COPY_NORMALIZED, MF_GRAYED);
                }
                
                if (g_paste_image_enabled) {
                    EnableMenuItem(g_hContextMenu, IDM_PASTE_IMAGE, MF_ENABLED);
                    ModifyMenu(g_hContextMenu, IDM_PASTE_IMAGE, MF_BYCOMMAND | MF_STRING, IDM_PASTE_IMAGE, "Paste Image (Ctrl+V)");
                } else {
                    EnableMenuItem(g_hContextMenu, IDM_PASTE_IMAGE, MF_GRAYED);
                    ModifyMenu(g_hContextMenu, IDM_PASTE_IMAGE, MF_BYCOMMAND | MF_STRING, IDM_PASTE_IMAGE, "Paste Image (No Image/File in Clipboard)");
                }

                ClientToScreen(hWnd, &pt);
                cmd = TrackPopupMenu(g_hContextMenu, 
                                     TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY, 
                                     pt.x, pt.y, 
                                     0, hWnd, NULL);

                if (cmd == 0) break;
                
                PostMessage(hWnd, WM_COMMAND, (WPARAM)cmd, 0);
            }
        }
        break;

        case WM_COMMAND:
        {
            int command_id = LOWORD(wParam);
            pngl_uc r, g, b, a;
            int img_x, img_y;
            char buffer[64];
            POINT cursor_pos;
            
            if (command_id == IDM_COPY_HEX || command_id == IDM_COPY_RGBA || command_id == IDM_COPY_NORMALIZED) {
                
                GetCursorPos(&cursor_pos);
                ScreenToClient(hWnd, &cursor_pos);
                
                if (!pixel_to_rgb_and_coords(hWnd, cursor_pos.x, cursor_pos.y, &r, &g, &b, &a, &img_x, &img_y)) {
                    break;
                }
            }

            switch (command_id) {
                case IDM_COPY_HEX:
                    sprintf(buffer, "0x%02X%02X%02X%02X", a, r, g, b);
                    copy_text_to_clipboard(hWnd, buffer);
                    break;
                    
                case IDM_COPY_RGBA:
                    sprintf(buffer, "%d,%d,%d,%d", r, g, b, a);
                    copy_text_to_clipboard(hWnd, buffer);
                    break;
                    
                case IDM_COPY_NORMALIZED:
                    sprintf(buffer, "%.3f,%.3f,%.3f,%.3f", 
                            (float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, (float)a / 255.0f);
                    copy_text_to_clipboard(hWnd, buffer);
                    break;
                    
                case IDM_COPY_IMAGE:
                    copy_image_to_clipboard(hWnd);
                    break;

                case IDM_PASTE_IMAGE:
                    if (g_paste_image_enabled) {
                        paste_image_from_clipboard(hWnd);
                    }
                    break;
            }
        }
        break;

        case WM_PAINT:
            do_double_buffered_paint(hWnd);
            break;

        case WM_SIZE:
            if (LOWORD(lParam) != g_mem_w || HIWORD(lParam) != g_mem_h) {
                 InvalidateRect(hWnd, NULL, FALSE);
            }
            
            if (g_scale == 1.0f && g_image_data != NULL) {
                RECT client_rect;
                GetClientRect(hWnd, &client_rect);
                int client_w = client_rect.right;
                int client_h = client_rect.bottom;
                
                g_offset_x = (float)(client_w - g_width) / 2.0f;
                g_offset_y = (float)(client_h - g_height) / 2.0f;
            }
            break;

        case WM_DESTROY:
            KillTimer(hWnd, TIMER_ID_UPDATE);
            KillTimer(hWnd, TIMER_ID_CLIPBOARD);
            cleanup_gdi_objects();
            if (g_hContextMenu != NULL) {
                DestroyMenu(g_hContextMenu);
            }
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    AppState app;
    FILE *f;
    pngl_uc *temp_image_data;
    pngl_io_callbacks callbacks;
    HWND hWnd;
    WNDCLASSEX wc;
    MSG msg;
    RECT window_rect;
    int window_w, window_h;
    
    memset(&app, 0, sizeof(AppState));
    if (argparse_process_args((ArgParseState *)&app, __argv, __argc, option_table)) {
        return 1;
    }

    if (app.image_path == NULL) {
        argparse_print_help(__argv, option_table);
        return 1;
    }

    if (strlen(app.image_path) >= MAX_PATH_CUSTOM) {
        fprintf(stderr, "error: file path is too long (max %d characters)\n", MAX_PATH_CUSTOM - 1);
        return 1;
    }

    strcpy(g_image_path, app.image_path);
    platform_normalize_path(g_image_path);

    f = fopen(g_image_path, "rb");
    if (f == NULL) {
        fprintf(stderr, "error: cannot open image file '%s'. check if the file exists and is accessible.\n", g_image_path);
        return 1;
    }

    callbacks.read = file_read_clbk;
    callbacks.skip = file_skip_clbk;
    callbacks.eof = file_eof_clbk;

    temp_image_data = pngl_load_from_callbacks(&callbacks, f, &g_width, &g_height, &g_comp, PNGL_rgb_alpha);

    fclose(f);

    if (temp_image_data == NULL) {
        fprintf(stderr, "error: failed to load image '%s': %s\n", g_image_path, pngl_failure_reason());
        return 1;
    }

    g_last_modified_size = platform_file_size(g_image_path);

    process_image_data(temp_image_data, g_width, g_height);
    g_image_data = temp_image_data;
    
    sprintf(g_window_title, "Image Viewer: %dx%d - %s", g_width, g_height, g_image_path);

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0; 
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL; 
    wc.lpszMenuName = NULL;
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc)) {
        fprintf(stderr, "error: failed to register window class\n");
        pngl_image_free(g_image_data);
        return 1;
    }

    window_rect.left = 0;
    window_rect.top = 0;
    window_rect.right = g_width;
    window_rect.bottom = g_height;
    
    if (window_rect.right > 1600) window_rect.right = 1600;
    if (window_rect.bottom > 1000) window_rect.bottom = 1000;
    
    AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, FALSE);
    
    window_w = window_rect.right - window_rect.left;
    window_h = window_rect.bottom - window_rect.top;

    hWnd = CreateWindow(
        WINDOW_CLASS_NAME,
        g_window_title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        window_w,
        window_h,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hWnd) {
        fprintf(stderr, "error: failed to create window\n");
        pngl_image_free(g_image_data);
        return 1;
    }
    
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd); 

    {
        RECT client_rect;
        GetClientRect(hWnd, &client_rect);
        g_offset_x = (float)(client_rect.right - g_width) / 2.0f;
        g_offset_y = (float)(client_rect.bottom - g_height) / 2.0f;
    }

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    pngl_image_free(g_image_data);
    return (int)msg.wParam;
}
