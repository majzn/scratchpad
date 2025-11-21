#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define ARG_PARSE_IMPLEMENTATION
#define PNGL_IMPLEMENTATION
#define PNGL_WRITE_IMPLEMENTATION
#define PLATFORM_IMPLEMENTATION

#include "libs\args.h"
#include "libs\plat.h"
#include "libs\pngl.h"

#include <windowsx.h>
#include <winuser.h>
#include <wingdi.h>
#include <ShlObj.h>
#include <CommDlg.h> 

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
#define IDM_SAVE_VIEW           1007

#define IDM_DRAW_MENU_BASE      2000
#define IDM_DRAW_COLOR_RED      (IDM_DRAW_MENU_BASE + 1)
#define IDM_DRAW_COLOR_GREEN    (IDM_DRAW_MENU_BASE + 2)
#define IDM_DRAW_COLOR_BLUE     (IDM_DRAW_MENU_BASE + 3)
#define IDM_DRAW_COLOR_BLACK    (IDM_DRAW_MENU_BASE + 4)
#define IDM_DRAW_COLOR_WHITE    (IDM_DRAW_MENU_BASE + 5)
#define IDM_DRAW_TOOL_PEN       (IDM_DRAW_MENU_BASE + 9)
#define IDM_DRAW_TOOL_ERASER    (IDM_DRAW_MENU_BASE + 10)
#define IDM_DRAW_TOOL_SHAPE     (IDM_DRAW_MENU_BASE + 15)
#define IDM_DRAW_CLEAR          (IDM_DRAW_MENU_BASE + 11)
#define IDM_DRAW_TOGGLE         (IDM_DRAW_MENU_BASE + 12)
#define IDM_DRAW_COLOR_SELECTOR (IDM_DRAW_MENU_BASE + 13) 
#define IDM_DRAW_GRID_TOGGLE    (IDM_DRAW_MENU_BASE + 14) 
#define IDM_SHAPE_LINE          (IDM_DRAW_MENU_BASE + 16)
#define IDM_SHAPE_RECTANGLE     (IDM_DRAW_MENU_BASE + 17)
#define IDM_SHAPE_CIRCLE        (IDM_DRAW_MENU_BASE + 18)
#define IDM_SHAPE_FILL_TOGGLE   (IDM_DRAW_MENU_BASE + 19)
#define IDM_SHAPE_OUTLINE_TOGGLE (IDM_DRAW_MENU_BASE + 20)
#define IDM_SHAPE_FILL_COLOR    (IDM_DRAW_MENU_BASE + 21)
#define IDM_SHAPE_OUTLINE_COLOR (IDM_DRAW_MENU_BASE + 22)
#define IDM_DRAW_TOOL_FILL      (IDM_DRAW_MENU_BASE + 23)
#define IDM_SHAPE_GRID          (IDM_DRAW_MENU_BASE + 24)
#define IDM_GRID_ROWS_INC       (IDM_DRAW_MENU_BASE + 25)
#define IDM_GRID_ROWS_DEC       (IDM_DRAW_MENU_BASE + 26)
#define IDM_GRID_COLS_INC       (IDM_DRAW_MENU_BASE + 27)
#define IDM_GRID_COLS_DEC       (IDM_DRAW_MENU_BASE + 28)


#define DRAWING_SCALE_FACTOR 1 
#define MAX_DRAWING_OBJECTS 16
#define MAX_FILL_STACK 100000 

typedef unsigned int u32;

typedef struct {
    int x, y;
} Point;

typedef struct {
    const char *image_path;
} AppState;

typedef struct {
    pngl_uc *data;
    int w;
    int h;
    float virtual_x;
    float virtual_y;
    float scale_at_save;
} SavedDrawingObject;

static pngl_uc *g_image_data = NULL;
static int g_width = 0;
static int g_height = 0;
static int g_comp = 0;
static char g_window_title[MAX_PATH_CUSTOM + 64]; 
static char g_image_path[MAX_PATH_CUSTOM];

static float g_scale = 1.0f;
static float g_offset_x = 0.0f;
static float g_offset_y = 0.0f;

static int g_background_mode = 0;

static int g_draw_mode = 0; 

static int g_mouse_down = 0;
static int g_mouse_mid_down = 0; 
static int g_last_mouse_x = 0;
static int g_last_mouse_y = 0;

static int g_dragging_object_index = -1; 
static int g_selected_object_index = -1;

static long g_last_modified_size = 0;
static long long g_last_modified_time = 0;

static u32 *g_framebuffer = NULL;
static int g_framebuffer_w = 0;
static int g_framebuffer_h = 0;

static HMENU g_hContextMenu = NULL;
static int g_paste_image_enabled = 0;

static int g_image_type = 0;

static int g_is_drawing_mode = 0;
static pngl_uc *g_framebuffer_data = NULL; 
static pngl_uc *g_temp_framebuffer_data = NULL; 
static int g_fb_w = 0;
static int g_fb_h = 0;

static COLORREF g_draw_color = RGB(255, 0, 0); 
static int g_draw_size = 5; 
static int g_draw_tool_mode = 0; 
static int g_is_grid_overlay = 0; 

static int g_shape_mode = 1; 
static int g_shape_is_filled = 0; 
static int g_shape_is_outlined = 1; 
static COLORREF g_shape_fill_color = RGB(0, 0, 255); 
static COLORREF g_shape_outline_color = RGB(0, 0, 0); 
static int g_grid_rows = 5;
static int g_grid_cols = 5;

static int g_shape_start_x = 0;
static int g_shape_start_y = 0;

static COLORREF g_custom_colors[16] = {0}; 

static SavedDrawingObject g_saved_drawings[MAX_DRAWING_OBJECTS] = {0};
static int g_drawing_count = 0;


extern int pngl_write_png(char const *filename, int w, int h, int comp, const void *data, int stride_bytes);
void update_window_title(HWND hWnd);
void clear_drawing_layer(HWND hWnd);
void clear_temp_layer(void);
void render_drawing_buffer_software(u32 *fb, const pngl_uc *buffer, int fb_w, int fb_h);
void draw_line_to_framebuffer(int x0, int y0, int x1, int y1);
HMENU create_main_menu(void);


int allocate_framebuffer(int w, int h) {
    if (g_framebuffer_w != w || g_framebuffer_h != h) {
        if (g_framebuffer != NULL) free(g_framebuffer);
        g_framebuffer = (u32 *)malloc((size_t)w * h * sizeof(u32));
        if (!g_framebuffer) {
            g_framebuffer_w = g_framebuffer_h = 0;
            return 0;
        }
        g_framebuffer_w = w;
        g_framebuffer_h = h;
    }
    return 1;
}

int handle_image_path(ArgParseState *state, char **argv, int argc, int *i_ptr) {
    AppState *app = (AppState *)state;
    return argparse_parse_string(argv, argc, i_ptr, &app->image_path, "input-image");
}

ArgOption option_table[] = {
    {"-i", "--input", handle_image_path, "Specify the input image file path <val>", 0, 0},
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

const char* get_mode_name(int mode) {
    switch (mode) {
        case 0: return "Normal";
        case 1: return "Tiled";
        case 2: return "Fit";
        default: return "Unknown";
    }
}

void update_window_title(HWND hWnd) {
    char mode_str[256];
    if (g_is_drawing_mode) {
        const char* tool_name;
        COLORREF current_color;
        
        if (g_draw_tool_mode == 0) { 
            tool_name = "Pen"; 
            current_color = g_draw_color;
        } else if (g_draw_tool_mode == 1) {
            tool_name = "Eraser";
            current_color = RGB(0, 0, 0); 
        } else if (g_draw_tool_mode == 2) { 
            tool_name = "Shape";
            current_color = g_shape_is_outlined ? g_shape_outline_color : g_shape_fill_color;
        } else {
            tool_name = "Fill";
            current_color = g_draw_color;
        }

        sprintf(mode_str, "Draw Mode ON (Tool: %s, Size: %d, Color: #%06X%s, Objects: %d)", 
            tool_name, 
            g_draw_size, 
            current_color & 0x00FFFFFF,
            g_is_grid_overlay ? ", Grid ON" : "",
            g_drawing_count
        );
    } else {
        sprintf(mode_str, "Mode: %s, %.0f%% (Objects: %d, Sel: %d)", 
            get_mode_name(g_draw_mode), g_scale * 100.0f, g_drawing_count, g_selected_object_index
        );
    }


    if (g_image_data != NULL) {
        char *path_end = strrchr(g_image_path, '\\');
        const char *filename = (path_end != NULL) ? (path_end + 1) : g_image_path;

        sprintf(g_window_title, "Image Viewer: %dx%d (%s) - %s (%s)", 
            g_width, g_height, 
            g_image_type == 1 ? "PNG" : (g_image_type == 2 ? "BMP" : (g_image_type == 0 ? "Clipboard" : "?")),
            filename[0] != '\0' ? filename : "(Clipboard)",
            mode_str
        );
    } else {
        sprintf(g_window_title, "Image Viewer: No Image Loaded (%s)", mode_str);
    }
    if (hWnd != NULL) {
        SetWindowText(hWnd, g_window_title);
    }
}

pngl_uc *load_bmp_data(const char *path, int *w, int *h) {
    HBITMAP hBitmap;
    HDC hdc = NULL;
    pngl_uc *pixels = NULL;
    BITMAP bmp;
    BITMAPINFOHEADER bmih;
    
    hBitmap = (HBITMAP)LoadImage(NULL, path, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    if (hBitmap == NULL) return NULL;
    
    if (GetObject(hBitmap, sizeof(BITMAP), &bmp) == 0) {
        DeleteObject(hBitmap);
        return NULL;
    }
    
    *w = bmp.bmWidth;
    *h = bmp.bmHeight;
    
    pixels = (pngl_uc *)malloc((size_t)(*w) * (*h) * 4);
    if (pixels == NULL) {
        DeleteObject(hBitmap);
        return NULL;
    }
    
    memset(&bmih, 0, sizeof(BITMAPINFOHEADER));
    bmih.biSize = sizeof(BITMAPINFOHEADER);
    bmih.biWidth = *w;
    bmih.biHeight = -(*h); 
    bmih.biPlanes = 1;
    bmih.biBitCount = 32;
    bmih.biCompression = BI_RGB;

    hdc = CreateCompatibleDC(NULL);
    if (hdc != NULL) {
        if (GetDIBits(hdc, hBitmap, 0, *h, pixels, (BITMAPINFO *)&bmih, DIB_RGB_COLORS) == 0) {
            free(pixels);
            pixels = NULL;
        }
        DeleteDC(hdc);
    } else {
        free(pixels);
        pixels = NULL;
    }

    DeleteObject(hBitmap);
    
    if (pixels != NULL) {
        for (int i = 0; i < (*w) * (*h) * 4; i += 4) {
            pngl_uc temp_b = pixels[i + 0];
            pixels[i + 0] = pixels[i + 2]; 
            pixels[i + 2] = temp_b;        
            pixels[i + 3] = 255;           
        }
    }

    return pixels;
}
int load_image_from_path(HWND hWnd, const char *path) {
    pngl_uc *new_image_data = NULL;
    int new_width = 0, new_height = 0;
    int new_comp = 0;
    int success = 0;
    int type = 0;

    if (strlen(path) >= MAX_PATH_CUSTOM) {
        return 0;
    }
    
    const char *ext = strrchr(path, '.');
    if (ext != NULL) {
        if (_stricmp(ext, ".png") == 0) {
            type = 1; 
        } else if (_stricmp(ext, ".bmp") == 0) {
            type = 2; 
        }
    }
    
    if (type == 1) {
        FILE *f = fopen(path, "rb");
        if (f == NULL) {
            return 0;
        }
        pngl_io_callbacks callbacks;
        callbacks.read = file_read_clbk;
        callbacks.skip = file_skip_clbk;
        callbacks.eof = file_eof_clbk;
        new_image_data = pngl_load_from_callbacks(&callbacks, f, &new_width, &new_height, &new_comp, PNGL_rgb_alpha);
        fclose(f);
        if (new_image_data != NULL) success = 1;
        
    } else if (type == 2) {
        new_image_data = load_bmp_data(path, &new_width, &new_height);
        if (new_image_data != NULL) success = 1;

    } else {
        return 0;
    }

    if (success) {
        if (g_image_data != NULL) {
            pngl_image_free(g_image_data);
        }
        
        process_image_data(new_image_data, new_width, new_height);
        g_image_data = new_image_data;
        g_width = new_width;
        g_height = new_height;
        g_comp = 4;
        g_image_type = type;

        strcpy(g_image_path, path);
        platform_normalize_path(g_image_path);
        g_last_modified_size = platform_file_size(g_image_path);
        g_last_modified_time = platform_get_timestamp_ms(); 

        update_window_title(hWnd);
        
        if (hWnd != NULL) {
            reset_view(hWnd);
            clear_drawing_layer(hWnd); 
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 1;
    }
    
    return 0;
}

int reload_image_data(HWND hWnd) {
    if (g_image_path[0] == '\0' || g_image_type != 1) return 0;
    
    FILE *f;
    pngl_uc *new_image_data;
    pngl_io_callbacks callbacks;
    int new_width, new_height, new_comp;
    
    f = fopen(g_image_path, "rb");
    if (f == NULL) {
        return 0;
    }

    callbacks.read = file_read_clbk;
    callbacks.skip = file_skip_clbk;
    callbacks.eof = file_eof_clbk;

    new_image_data = pngl_load_from_callbacks(&callbacks, f, &new_width, &new_height, &new_comp, PNGL_rgb_alpha);

    fclose(f);

    if (new_image_data == NULL) {
        return 0;
    }
    
    if (g_image_data != NULL) {
        pngl_image_free(g_image_data);
    }
    
    process_image_data(new_image_data, new_width, new_height);
    g_image_data = new_image_data;
    g_width = new_width;
    g_height = new_height;
    
    update_window_title(hWnd);
    
    InvalidateRect(hWnd, NULL, FALSE);
    return 1;
}

void check_for_file_updates(HWND hWnd) {
    long current_size;
    
    if (g_image_path[0] == '\0' || g_image_type != 1) return;

    current_size = platform_file_size(g_image_path);

    if (current_size != g_last_modified_size) {
        if (reload_image_data(hWnd)) {
            g_last_modified_size = current_size;
        }
    }
}

u32 get_bg_color(int mode) {
    switch (mode) {
        case 1: return 0xFF000000;
        case 2: return 0xFFFFFFFF;
        case 3: return 0xFFFF0000;
        case 4: return 0xFF00FF00;
        case 5: return 0xFF0000FF;
        case 6: return 0xFFFF00FF;
        case 7: return 0xFFFFFF00;
        case 8: return 0xFF00FFFF;
        case 9: return 0xFF303030;
        default: return 0xFFC0C0C0;
    }
}

void draw_bg_tile(u32 *fb, int fb_w, int fb_h, int mode) {
    u32 color_light, color_dark;
    int x, y;
    int tile_size = 16;
    
    if (mode == 0) {
        color_light = 0xFFC0C0C0; 
        color_dark = 0xFF808080;
    } else if (mode == 9) {
        color_light = 0xFF505050; 
        color_dark = 0xFF303030;
    } else {
        u32 color = get_bg_color(mode);
        for (y = 0; y < fb_h; y++) {
            for (x = 0; x < fb_w; x++) {
                fb[y * fb_w + x] = color;
            }
        }
        return;
    }
    
    for (y = 0; y < fb_h; y++) {
        for (x = 0; x < fb_w; x++) {
            u32 color;
            if (((x / tile_size) + (y / tile_size)) % 2 == 0) {
                color = color_light;
            } else {
                color = color_dark;
            }
            fb[y * fb_w + x] = color;
        }
    }
}

void blend_pixel_32(u32 *dest, u32 src) {
    u32 d_r = (*dest >> 16) & 0xFF;
    u32 d_g = (*dest >> 8) & 0xFF;
    u32 d_b = (*dest >> 0) & 0xFF;

    u32 s_a = (src >> 24) & 0xFF;
    u32 s_r = (src >> 16) & 0xFF;
    u32 s_g = (src >> 8) & 0xFF;
    u32 s_b = (src >> 0) & 0xFF;

    u32 alpha_factor = s_a + (s_a >> 7); 
    u32 inv_alpha_factor = 255 - s_a;
    
    if (s_a == 255) {
        *dest = (src & 0xFFFFFF) | 0xFF000000; 
        return;
    }
    if (s_a == 0) {
        return;
    }
    
    u32 final_r = (s_r * alpha_factor + d_r * inv_alpha_factor) >> 8;
    u32 final_g = (s_g * alpha_factor + d_g * inv_alpha_factor) >> 8;
    u32 final_b = (s_b * alpha_factor + d_b * inv_alpha_factor) >> 8;
    
    *dest = 0xFF000000 | (final_r << 16) | (final_g << 8) | final_b;
}

void draw_image_raw(u32 *fb, int fb_w, int fb_h, 
                    pngl_uc *img_data, int img_w, int img_h,
                    int dest_x, int dest_y, int draw_w, int draw_h) {
    
    float scale_x;
    float scale_y;
    int x_start, x_end, y_start, y_end;
    
    if (draw_w == 0 || draw_h == 0) return;

    scale_x = (float)img_w / (float)draw_w;
    scale_y = (float)img_h / (float)draw_h;

    /* OPTIMIZATION: Calculate clipping bounds (screen coordinates) */
    
    /* Determine visible pixel range on screen (clipping) */
    x_start = max(0, -dest_x);
    y_start = max(0, -dest_y);
    x_end = min(draw_w, fb_w - dest_x);
    y_end = min(draw_h, fb_h - dest_y);
    
    /* Ensure clip range is valid */
    if (x_start >= x_end || y_start >= y_end) return;

    for (int y = y_start; y < y_end; y++) {
        for (int x = x_start; x < x_end; x++) {
            
            int dx = dest_x + x; /* Screen destination X */
            int dy = dest_y + y; /* Screen destination Y */
            
            /* Calculate source coordinates (sampling) */
            int src_x = (int)(x * scale_x);
            int src_y = (int)(y * scale_y);
            
            /* Ensure src coordinates are within image bounds (should generally be true 
               if draw_w/h are proportional to img_w/h, but necessary for safety) */
            if (src_x < 0 || src_x >= img_w || src_y < 0 || src_y >= img_h) continue;

            pngl_uc *src_uc = img_data + (src_y * img_w + src_x) * 4;
            
            u32 b_pm = src_uc[0];
            u32 g_pm = src_uc[1];
            u32 r_pm = src_uc[2];
            u32 a = src_uc[3];

            u32 src_color = (a << 24) | (r_pm << 16) | (g_pm << 8) | b_pm;
            
            u32 *dest_pixel_ptr = fb + dy * fb_w + dx;
            blend_pixel_32(dest_pixel_ptr, src_color);
        }
    }
}

void render_drawing_object_software(u32 *fb, int fb_w, int fb_h, int index, const SavedDrawingObject *obj) {
    if (obj->data == NULL) return;

    float render_scale = g_scale / obj->scale_at_save;

    int draw_w = (int)((float)obj->w * render_scale);
    int draw_h = (int)((float)obj->h * render_scale);
    
    int draw_x = (int)(obj->virtual_x * g_scale + g_offset_x);
    int draw_y = (int)(obj->virtual_y * g_scale + g_offset_y);

    float scale_x;
    float scale_y;
    int x_start, x_end, y_start, y_end;

    if (draw_w == 0 || draw_h == 0) return;
    
    scale_x = (float)obj->w / (float)draw_w;
    scale_y = (float)obj->h / (float)draw_h;
    
    /* OPTIMIZATION: Calculate clipping bounds (screen coordinates) */
    
    x_start = max(0, -draw_x);
    y_start = max(0, -draw_y);
    x_end = min(draw_w, fb_w - draw_x);
    y_end = min(draw_h, fb_h - draw_y);

    if (x_start >= x_end || y_start >= y_end) return;

    for (int y = y_start; y < y_end; y++) {
        for (int x = x_start; x < x_end; x++) {
            
            int dx = draw_x + x;
            int dy = draw_y + y;
            
            /* Calculate source coordinates (sampling) */
            int src_x = (int)(x * scale_x);
            int src_y = (int)(y * scale_y);
            
            if (src_x < 0 || src_x >= obj->w || src_y < 0 || src_y >= obj->h) continue;

            pngl_uc *src_uc = obj->data + (src_y * obj->w + src_x) * 4;
            
            u32 b_pm = src_uc[0];
            u32 g_pm = src_uc[1];
            u32 r_pm = src_uc[2];
            u32 a = src_uc[3];

            u32 src_color = (a << 24) | (r_pm << 16) | (g_pm << 8) | b_pm;
            
            u32 *dest_pixel_ptr = fb + dy * fb_w + dx;
            blend_pixel_32(dest_pixel_ptr, src_color);
        }
    }
}

void render_drawing_buffer_software(u32 *fb, const pngl_uc *buffer, int fb_w, int fb_h) {
    if (buffer == NULL || g_fb_w == 0 || g_fb_h == 0) return;

    for (int y = 0; y < fb_h; y++) {
        for (int x = 0; x < fb_w; x++) {
            
            pngl_uc *src_uc = (pngl_uc *)buffer + (y * g_fb_w + x) * 4;
            
            u32 b_pm = src_uc[0];
            u32 g_pm = src_uc[1];
            u32 r_pm = src_uc[2];
            u32 a = src_uc[3];

            u32 src_color = (a << 24) | (r_pm << 16) | (g_pm << 8) | b_pm;
            
            u32 *dest_pixel_ptr = fb + y * fb_w + x;
            blend_pixel_32(dest_pixel_ptr, src_color);
        }
    }
}

void draw_grid_overlay_software(u32 *fb, int fb_w, int fb_h, int step, COLORREF color) {
    int x, y;
    u32 c = (0xFF << 24) | (GetRValue(color) << 16) | (GetGValue(color) << 8) | GetBValue(color);

    int start_x = (int)fmodf(g_offset_x, (float)step);
    int start_y = (int)fmodf(g_offset_y, (float)step);

    if (start_x < 0) start_x += step;
    if (start_y < 0) start_y += step;

    for (x = start_x; x < fb_w; x += step) {
        for (y = 0; y < fb_h; ++y) {
            if ((x - start_x) % 50 == 0 || (x - start_x) % 50 == 1) { 
                fb[y * fb_w + x] = c;
            }
        }
    }
    
    for (y = start_y; y < fb_h; y += step) {
        for (x = 0; x < fb_w; ++x) {
             if ((y - start_y) % 50 == 0 || (y - start_y) % 50 == 1) { 
                fb[y * fb_w + x] = c;
            }
        }
    }
}


void render_framebuffer(HWND hWnd) {
    RECT client_rect;
    HDC hdc_screen;
    int client_w, client_h;
    
    hdc_screen = GetDC(hWnd);
    GetClientRect(hWnd, &client_rect);

    client_w = client_rect.right;
    client_h = client_rect.bottom;

    if (!allocate_framebuffer(client_w, client_h)) {
        ReleaseDC(hWnd, hdc_screen);
        return;
    }
    
    draw_bg_tile(g_framebuffer, client_w, client_h, g_background_mode); 

    if (g_image_data != NULL) {
        int scaled_width = (int)((float)g_width * g_scale);
        int scaled_height = (int)((float)g_height * g_scale);
        int dest_x = (int)g_offset_x;
        int dest_y = (int)g_offset_y;
        
        int draw_w, draw_h, draw_x, draw_y;
        
        switch (g_draw_mode) {
            case 0:
            default:
                draw_w = scaled_width;
                draw_h = scaled_height;
                draw_x = dest_x;
                draw_y = dest_y;
                
                draw_image_raw(g_framebuffer, client_w, client_h,
                               g_image_data, g_width, g_height,
                               draw_x, draw_y, draw_w, draw_h);
                break;
                
            case 1:
                if (scaled_width > 0 && scaled_height > 0) {
                    int tile_x_start = (int)floorf((float)(client_rect.left - dest_x) / scaled_width);
                    int tile_y_start = (int)floorf((float)(client_rect.top - dest_y) / scaled_height);
                    int tile_x_end = (int)ceilf((float)(client_rect.right - dest_x) / scaled_width);
                    int tile_y_end = (int)ceilf((float)(client_rect.bottom - dest_y) / scaled_height);

                    for (int i = tile_x_start; i < tile_x_end; ++i) {
                        for (int j = tile_y_start; j < tile_y_end; ++j) {
                            draw_image_raw(g_framebuffer, client_w, client_h,
                                           g_image_data, g_width, g_height,
                                           dest_x + i * scaled_width,
                                           dest_y + j * scaled_height,
                                           scaled_width, scaled_height);
                        }
                    }
                }
                break;
                
            case 2:
                {
                    float aspect_ratio = (float)g_width / (float)g_height;
                    float window_aspect = (float)client_w / (float)client_h;

                    if (aspect_ratio > window_aspect) {
                        draw_w = client_w;
                        draw_h = (int)((float)client_w / aspect_ratio);
                        draw_x = 0;
                        draw_y = (client_h - draw_h) / 2;
                    } else {
                        draw_h = client_h;
                        draw_w = (int)((float)client_h * aspect_ratio);
                        draw_x = (client_w - draw_w) / 2;
                        draw_y = 0;
                    }
                    
                    draw_image_raw(g_framebuffer, client_w, client_h,
                                   g_image_data, g_width, g_height,
                                   draw_x, draw_y, draw_w, draw_h);
                }
                break;
        }
    }
    
    for (int i = 0; i < g_drawing_count; ++i) {
        render_drawing_object_software(g_framebuffer, client_w, client_h, i, &g_saved_drawings[i]);
    }
    
    if (g_is_drawing_mode && g_is_grid_overlay) {
        draw_grid_overlay_software(g_framebuffer, client_w, client_h, 50, RGB(128, 128, 128));
    }
    
    if (g_is_drawing_mode) {
        int fb_w_scaled = client_w;
        int fb_h_scaled = client_h;
        
        if (g_framebuffer_data != NULL) {
             render_drawing_buffer_software(g_framebuffer, g_framebuffer_data, fb_w_scaled, fb_h_scaled);
        }
        if (g_mouse_down && g_draw_tool_mode == 2 && g_temp_framebuffer_data != NULL) {
             render_drawing_buffer_software(g_framebuffer, g_temp_framebuffer_data, fb_w_scaled, fb_h_scaled);
        }
    }
    
    {
        BITMAPINFO bmi;
        memset(&bmi, 0, sizeof(BITMAPINFO));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = client_w;
        bmi.bmiHeader.biHeight = -client_h; 
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        
        StretchDIBits(
            hdc_screen,
            0, 0, client_w, client_h,
            0, 0, client_w, client_h,
            g_framebuffer,
            &bmi,
            DIB_RGB_COLORS,
            SRCCOPY
        );
    }
           
    ReleaseDC(hWnd, hdc_screen);
}

void do_double_buffered_paint(HWND hWnd) {
    PAINTSTRUCT ps;
    HDC hdc;
    
    hdc = BeginPaint(hWnd, &ps);
    EndPaint(hWnd, &ps);

    render_framebuffer(hWnd);
    
    if (g_selected_object_index != -1 && !g_is_drawing_mode) {
        const SavedDrawingObject *obj = &g_saved_drawings[g_selected_object_index];
        int draw_w = (int)((float)obj->w * (g_scale / obj->scale_at_save));
        int draw_h = (int)((float)obj->h * (g_scale / obj->scale_at_save));
        int draw_x = (int)(obj->virtual_x * g_scale + g_offset_x);
        int draw_y = (int)(obj->virtual_y * g_scale + g_offset_y);

        HDC hdc_screen = GetDC(hWnd);
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 0));
        HGDIOBJ hOldPen = SelectObject(hdc_screen, hPen);
        HBRUSH hOldBrush = SelectObject(hdc_screen, GetStockObject(NULL_BRUSH));

        Rectangle(hdc_screen, draw_x - 2, draw_y - 2, draw_x + draw_w + 2, draw_y + draw_h + 2);

        SelectObject(hdc_screen, hOldPen);
        SelectObject(hdc_screen, hOldBrush);
        DeleteObject(hPen);
        ReleaseDC(hWnd, hdc_screen);
    }
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

    if (g_draw_mode == 0 || g_draw_mode == 1) {
        if (g_draw_mode == 1) {
            float rel_x = (float)screen_x - image_tl_x;
            float rel_y = (float)screen_y - image_tl_y;
            
            if (scaled_width == 0.0f || scaled_height == 0.0f) return 0;
            
            float tile_x_f = fmodf(rel_x, scaled_width);
            float tile_y_f = fmodf(rel_y, scaled_height);

            if (tile_x_f < 0.0f) tile_x_f += scaled_width;
            if (tile_y_f < 0.0f) tile_y_f += scaled_height;
            
            *img_x = (int)(tile_x_f / g_scale);
            *img_y = (int)(tile_y_f / g_scale);
            
            if (*img_x < 0 || *img_x >= g_width || *img_y < 0 || *img_y >= g_height) {
                return 0;
            }
        
        } else {
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

void save_view_to_file(HWND hWnd) {
    RECT client_rect;
    GetClientRect(hWnd, &client_rect);
    int w = client_rect.right;
    int h = client_rect.bottom;

    if (g_framebuffer == NULL) {
        return;
    }

    char szFile[MAX_PATH_CUSTOM] = "image_view_capture.png";
    
    OPENFILENAME ofn;
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "PNG Files (*.png)\0*.png\0Bitmap Files (*.bmp)\0*.bmp\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    if (GetSaveFileName(&ofn)) {
        int is_png = (ofn.nFilterIndex == 1);
        pngl_uc *output_data = (pngl_uc*)malloc((size_t)w * h * 4);
        if (output_data == NULL) return;
        
        for (int i = 0; i < w * h; ++i) {
            u32 pixel = g_framebuffer[i];
            
            u32 r = (pixel >> 16) & 0xFF;
            u32 g = (pixel >> 8) & 0xFF;
            u32 b = (pixel >> 0) & 0xFF;

            output_data[i * 4 + 0] = b; 
            output_data[i * 4 + 1] = g; 
            output_data[i * 4 + 2] = r; 
            output_data[i * 4 + 3] = 255; 
        }

        if (is_png) {
            pngl_write_png(ofn.lpstrFile, w, h, 4, output_data, w * 4);
        } else {
             /* Saving as a BMP/PNG fallback is complex and depends on library support. 
                Assuming pngl_write_png is the universal write function here for simplicity. */
            pngl_write_png(ofn.lpstrFile, w, h, 4, output_data, w * 4);
        }
        
        free(output_data);
    }
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
        
        for (int i = 0; i < g_width * g_height; ++i) {
            pngl_uc *src_pixel = g_image_data + i * 4;
            pngl_uc *dst_pixel = clipboard_data + i * 4;

            pngl_uc b_pm = src_pixel[0];
            pngl_uc g_pm = src_pixel[1];
            pngl_uc r_pm = src_pixel[2];
            pngl_uc a = src_pixel[3];
            
            if (a > 0) {
                dst_pixel[0] = (pngl_uc)(((unsigned int)b_pm * 255) / a);
                dst_pixel[1] = (pngl_uc)(((unsigned int)g_pm * 255) / a);
                dst_pixel[2] = (pngl_uc)(((unsigned int)r_pm * 255) / a);
            } else {
                dst_pixel[0] = 0;
                dst_pixel[1] = 0;
                dst_pixel[2] = 0;
            }
            dst_pixel[3] = a; 
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
    
    pixels = (pngl_uc *)malloc((size_t)(*w) * (*h) * 4);
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
        for (int i = 0; i < (*w) * (*h) * 4; i += 4) {
            pngl_uc temp_b = pixels[i + 0];
            pixels[i + 0] = pixels[i + 2]; 
            pixels[i + 2] = temp_b;        
            pixels[i + 3] = 255;           
        }
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
            
            if ((pBmih->biBitCount == 32 || pBmih->biBitCount == 24) && pBmih->biCompression == BI_RGB && new_width > 0 && new_height > 0) {
                
                char *pData = (char *)pBmi + pBmih->biSize + pBmih->biClrUsed * sizeof(RGBQUAD);
                size_t src_row_size = ((size_t)new_width * pBmih->biBitCount / 8 + 3) & (~3); 
                size_t dst_row_size = (size_t)new_width * 4;
                int i;
                
                new_image_data = (pngl_uc *)malloc((size_t)new_width * new_height * 4);
                if (new_image_data != NULL) {
                    
                    for (i = 0; i < new_height; i++) {
                        char *src_row;
                        if (pBmih->biHeight > 0) { 
                            src_row = pData + (new_height - 1 - i) * src_row_size;
                        } else {
                            src_row = pData + i * src_row_size;
                        }
                        
                        pngl_uc *dst_pixel = new_image_data + i * dst_row_size;

                        if (pBmih->biBitCount == 32) {
                            memcpy(dst_pixel, src_row, dst_row_size);
                            for (int j = 0; j < new_width; ++j) {
                                pngl_uc b = dst_pixel[j*4 + 0];
                                pngl_uc g = dst_pixel[j*4 + 1];
                                pngl_uc r = dst_pixel[j*4 + 2];
                                pngl_uc a = dst_pixel[j*4 + 3];
                                
                                dst_pixel[j*4 + 0] = r; 
                                dst_pixel[j*4 + 1] = g;
                                dst_pixel[j*4 + 2] = b;
                                dst_pixel[j*4 + 3] = a;
                            }

                        } else if (pBmih->biBitCount == 24) {
                            for (int j = 0; j < new_width; ++j) {
                                dst_pixel[j*4 + 0] = src_row[j*3 + 2]; 
                                dst_pixel[j*4 + 1] = src_row[j*3 + 1]; 
                                dst_pixel[j*4 + 2] = src_row[j*3 + 0]; 
                                dst_pixel[j*4 + 3] = 255; 
                            }
                        }
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
        g_comp = 4;
        g_image_type = 0; 

        process_image_data(g_image_data, g_width, g_height); 
        
        g_image_path[0] = '\0'; 
        g_last_modified_size = 0;
        
        update_window_title(hWnd);
        
        reset_view(hWnd);
        clear_drawing_layer(hWnd); 
    } else if (new_image_data != NULL) {
        free(new_image_data);
    }
}

HMENU create_drawing_menu(void) {
    HMENU hMenuDraw = CreateMenu();
    if (hMenuDraw == NULL) return NULL;
    
    char size_str[32];
    
    HMENU hSubMenuTool = CreatePopupMenu();
    AppendMenu(hSubMenuTool, MF_STRING | (g_draw_tool_mode == 0 ? MF_CHECKED : MF_UNCHECKED), IDM_DRAW_TOOL_PEN, "Pen/Brush");
    AppendMenu(hSubMenuTool, MF_STRING | (g_draw_tool_mode == 1 ? MF_CHECKED : MF_UNCHECKED), IDM_DRAW_TOOL_ERASER, "Eraser"); 
    AppendMenu(hSubMenuTool, MF_STRING | (g_draw_tool_mode == 2 ? MF_CHECKED : MF_UNCHECKED), IDM_DRAW_TOOL_SHAPE, "Shape Tool (Tab)"); 
    AppendMenu(hSubMenuTool, MF_STRING | (g_draw_tool_mode == 3 ? MF_CHECKED : MF_UNCHECKED), IDM_DRAW_TOOL_FILL, "Fill Tool (Tab)"); 
    AppendMenu(hMenuDraw, MF_POPUP, (UINT_PTR)hSubMenuTool, "&Tool");

    sprintf(size_str, "Size: Ctrl+Scroll (%dpx)", g_draw_size);
    AppendMenu(hMenuDraw, MF_STRING | MF_DISABLED, 0, size_str); 
    AppendMenu(hMenuDraw, MF_SEPARATOR, 0, NULL);
    
    if (g_draw_tool_mode == 0 || g_draw_tool_mode == 3) { 
        HMENU hSubMenuColor = CreatePopupMenu();
        AppendMenu(hSubMenuColor, MF_STRING | (g_draw_color == RGB(255, 0, 0) ? MF_CHECKED : MF_UNCHECKED), IDM_DRAW_COLOR_RED, "Red");
        AppendMenu(hSubMenuColor, MF_STRING | (g_draw_color == RGB(0, 255, 0) ? MF_CHECKED : MF_UNCHECKED), IDM_DRAW_COLOR_GREEN, "Green");
        AppendMenu(hSubMenuColor, MF_STRING | (g_draw_color == RGB(0, 0, 255) ? MF_CHECKED : MF_UNCHECKED), IDM_DRAW_COLOR_BLUE, "Blue");
        AppendMenu(hSubMenuColor, MF_STRING | (g_draw_color == RGB(0, 0, 0) ? MF_CHECKED : MF_UNCHECKED), IDM_DRAW_COLOR_BLACK, "Black");
        AppendMenu(hSubMenuColor, MF_STRING | (g_draw_color == RGB(255, 255, 255) ? MF_CHECKED : MF_UNCHECKED), IDM_DRAW_COLOR_WHITE, "White");
        AppendMenu(hSubMenuColor, MF_SEPARATOR, 0, NULL);
        AppendMenu(hSubMenuColor, MF_STRING, IDM_DRAW_COLOR_SELECTOR, "Select Custom...");
        AppendMenu(hMenuDraw, MF_POPUP, (UINT_PTR)hSubMenuColor, "&Primary Color");
    } 
    
    if (g_draw_tool_mode == 2) { 
        HMENU hSubMenuShape = CreatePopupMenu();
        AppendMenu(hSubMenuShape, MF_STRING | (g_shape_mode == 0 ? MF_CHECKED : MF_UNCHECKED), IDM_SHAPE_LINE, "Line");
        AppendMenu(hSubMenuShape, MF_STRING | (g_shape_mode == 1 ? MF_CHECKED : MF_UNCHECKED), IDM_SHAPE_RECTANGLE, "Rectangle");
        AppendMenu(hSubMenuShape, MF_STRING | (g_shape_mode == 2 ? MF_CHECKED : MF_UNCHECKED), IDM_SHAPE_CIRCLE, "Circle");
        AppendMenu(hSubMenuShape, MF_STRING | (g_shape_mode == 3 ? MF_CHECKED : MF_UNCHECKED), IDM_SHAPE_GRID, "Grid");
        AppendMenu(hMenuDraw, MF_POPUP, (UINT_PTR)hSubMenuShape, "&Shape Type");

        if (g_shape_mode != 3) {
            AppendMenu(hMenuDraw, MF_STRING | (g_shape_is_filled ? MF_CHECKED : MF_UNCHECKED), IDM_SHAPE_FILL_TOGGLE, "Toggle Fill");
        }
        AppendMenu(hMenuDraw, MF_STRING | (g_shape_is_outlined ? MF_CHECKED : MF_UNCHECKED), IDM_SHAPE_OUTLINE_TOGGLE, "Toggle Outline");
        
        if (g_shape_mode != 3 && g_shape_is_filled) {
            AppendMenu(hMenuDraw, MF_STRING, IDM_SHAPE_FILL_COLOR, "Set Fill Color");
        }
        if (g_shape_is_outlined || g_shape_mode == 3) {
            AppendMenu(hMenuDraw, MF_STRING, IDM_SHAPE_OUTLINE_COLOR, "Set Outline Color");
        }
        
        if (g_shape_mode == 3) {
            HMENU hSubMenuGrid = CreatePopupMenu();
            char grid_str[32];

            sprintf(grid_str, "Rows: %d", g_grid_rows);
            AppendMenu(hSubMenuGrid, MF_STRING, IDM_GRID_ROWS_INC, grid_str);
            AppendMenu(hSubMenuGrid, MF_STRING, IDM_GRID_ROWS_DEC, grid_str);
            
            AppendMenu(hSubMenuGrid, MF_SEPARATOR, 0, NULL);
            
            sprintf(grid_str, "Cols: %d", g_grid_cols);
            AppendMenu(hSubMenuGrid, MF_STRING, IDM_GRID_COLS_INC, grid_str);
            AppendMenu(hSubMenuGrid, MF_STRING, IDM_GRID_COLS_DEC, grid_str);
            
            AppendMenu(hMenuDraw, MF_POPUP, (UINT_PTR)hSubMenuGrid, "&Grid Config");
        }
    }
    
    AppendMenu(hMenuDraw, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenuDraw, MF_STRING | (g_is_grid_overlay ? MF_CHECKED : MF_UNCHECKED), IDM_DRAW_GRID_TOGGLE, "Toggle View Grid");
    AppendMenu(hMenuDraw, MF_STRING, IDM_DRAW_CLEAR, "Clear Layer (Esc)");

    return hMenuDraw;
}


HMENU create_main_menu(void) {
    HMENU hMenu = CreateMenu();
    HMENU hMenuView = CreateMenu();
    
    AppendMenu(hMenuView, MF_STRING, IDM_COPY_IMAGE, "Copy Image (Ctrl+C)");
    AppendMenu(hMenuView, MF_STRING | (g_paste_image_enabled ? MF_ENABLED : MF_GRAYED), IDM_PASTE_IMAGE, "Paste Image (Ctrl+V)");
    AppendMenu(hMenuView, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenuView, MF_STRING | (g_is_drawing_mode ? MF_CHECKED : MF_UNCHECKED), IDM_DRAW_TOGGLE, "Toggle Drawing Mode (A)");
    AppendMenu(hMenuView, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenuView, MF_STRING, IDM_SAVE_VIEW, g_is_drawing_mode ? "Bake Drawing to Object (Ctrl+S)" : "Save Current View (Ctrl+S)");

    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hMenuView, "&View");
    
    if (g_is_drawing_mode) {
        HMENU hMenuDraw = create_drawing_menu();
        AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hMenuDraw, "&Draw Tools");
    }

    return hMenu;
}


HMENU create_context_menu(void) {
    HMENU hMenu = CreatePopupMenu();
    if (hMenu == NULL) return NULL;
    
    AppendMenu(hMenu, MF_STRING, IDM_COPY_HEX, "Copy Hex (0xAARRGGBB)");
    AppendMenu(hMenu, MF_STRING, IDM_COPY_RGBA, "Copy RGBA (0-255)");
    AppendMenu(hMenu, MF_STRING, IDM_COPY_NORMALIZED, "Copy Normalized RGBA (0.0-1.0)");
    
    AppendMenu(hMenu, MF_SEPARATOR, IDM_SEPARATOR, NULL);
    
    AppendMenu(hMenu, MF_STRING, IDM_COPY_IMAGE, "Copy Image (Ctrl+C)");
    
    if (g_paste_image_enabled) {
        AppendMenu(hMenu, MF_STRING, IDM_PASTE_IMAGE, "Paste Image (Ctrl+V)");
    } else {
        AppendMenu(hMenu, MF_STRING | MF_GRAYED, IDM_PASTE_IMAGE, "Paste Image (No Image/File in Clipboard)");
    }
    
    AppendMenu(hMenu, MF_SEPARATOR, IDM_SEPARATOR, NULL);
    
    AppendMenu(hMenu, MF_STRING, IDM_SAVE_VIEW, g_is_drawing_mode ? "Bake Drawing to Object (Ctrl+S)" : "Save Current View (Ctrl+S)");
    
    return hMenu;
}

void delete_selected_object(HWND hWnd) {
    if (g_selected_object_index >= 0 && g_selected_object_index < g_drawing_count) {
        
        if (g_saved_drawings[g_selected_object_index].data != NULL) {
            pngl_image_free(g_saved_drawings[g_selected_object_index].data);
        }

        for (int i = g_selected_object_index; i < g_drawing_count - 1; ++i) {
            g_saved_drawings[i] = g_saved_drawings[i + 1];
        }
        
        g_drawing_count--;
        g_selected_object_index = -1; 
        g_dragging_object_index = -1;
        update_window_title(hWnd);
        InvalidateRect(hWnd, NULL, FALSE);
    }
}


void cleanup_drawing_objects(void) {
    if (g_framebuffer_data != NULL) {
        free(g_framebuffer_data);
        g_framebuffer_data = NULL;
    }
    if (g_temp_framebuffer_data != NULL) { 
        free(g_temp_framebuffer_data);
        g_temp_framebuffer_data = NULL;
    }
    g_fb_w = 0;
    g_fb_h = 0;

    for(int i = 0; i < MAX_DRAWING_OBJECTS; ++i) {
        if (g_saved_drawings[i].data != NULL) {
            pngl_image_free(g_saved_drawings[i].data);
            g_saved_drawings[i].data = NULL;
        }
    }
    g_drawing_count = 0;
}

void init_drawing_buffers(int w, int h) {
    int scaled_w = w; /* CLEANUP: Removed division by DRAWING_SCALE_FACTOR (1) */
    int scaled_h = h;

    if (g_framebuffer_data == NULL || g_fb_w != scaled_w || g_fb_h != scaled_h) {
        if (g_framebuffer_data != NULL) free(g_framebuffer_data);
        if (g_temp_framebuffer_data != NULL) free(g_temp_framebuffer_data);
        
        g_fb_w = scaled_w;
        g_fb_h = scaled_h;
        g_framebuffer_data = (pngl_uc *)malloc((size_t)g_fb_w * g_fb_h * 4);
        g_temp_framebuffer_data = (pngl_uc *)malloc((size_t)g_fb_w * g_fb_h * 4);
        
        if (g_framebuffer_data != NULL) {
            memset(g_framebuffer_data, 0, (size_t)g_fb_w * g_fb_h * 4);
        } else {
            g_fb_w = g_fb_h = 0;
        }
        if (g_temp_framebuffer_data != NULL) {
            memset(g_temp_framebuffer_data, 0, (size_t)g_fb_w * g_fb_h * 4);
        } else {
            g_fb_w = g_fb_h = 0;
        }
    }
}

void clear_drawing_layer(HWND hWnd) {
    if (g_framebuffer_data != NULL) {
        memset(g_framebuffer_data, 0, (size_t)g_fb_w * g_fb_h * 4);
        InvalidateRect(hWnd, NULL, FALSE);
    }
}

void clear_temp_layer(void) {
    if (g_temp_framebuffer_data != NULL) {
        memset(g_temp_framebuffer_data, 0, (size_t)g_fb_w * g_fb_h * 4);
    }
}

void draw_pixel_to_buffer(int x, int y, COLORREF color, int alpha, pngl_uc *buffer) {
    if (x < 0 || x >= g_fb_w || y < 0 || y >= g_fb_h || buffer == NULL) return;
    
    pngl_uc *pixel = buffer + (y * g_fb_w + x) * 4;
    
    pngl_uc r = GetRValue(color);
    pngl_uc g = GetGValue(color);
    pngl_uc b = GetBValue(color);
    pngl_uc a = (pngl_uc)alpha;
    
    if (g_draw_tool_mode != 1) { 
        if (a > 0) {
            pixel[0] = (pngl_uc)(((unsigned int)b * a) / 255);
            pixel[1] = (pngl_uc)(((unsigned int)g * a) / 255);
            pixel[2] = (pngl_uc)(((unsigned int)r * a) / 255);
        } else {
            pixel[0] = 0;
            pixel[1] = 0;
            pixel[2] = 0;
        }
        pixel[3] = a; 
    } else { 
        pixel[0] = 0;
        pixel[1] = 0;
        pixel[2] = 0;
        pixel[3] = 0;
    }
}

void draw_circle_to_buffer_primitive(int cx, int cy, int radius, COLORREF color, int alpha, pngl_uc *buffer) {
    int r_sq = radius * radius;
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x * x + y * y <= r_sq) {
                draw_pixel_to_buffer(cx + x, cy + y, color, alpha, buffer);
            }
        }
    }
}

void draw_line_segment_to_buffer(int x0, int y0, int x1, int y1, COLORREF color, int radius, pngl_uc *buffer) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    int alpha = 255; 
    
    while (1) {
        draw_circle_to_buffer_primitive(x0, y0, radius, color, alpha, buffer);

        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void draw_line_to_framebuffer(int x0, int y0, int x1, int y1) {
    draw_line_segment_to_buffer(x0, y0, x1, y1, g_draw_color, g_draw_size / 2, g_framebuffer_data);
}

void draw_rectangle_to_buffer(int x0, int y0, int x1, int y1, COLORREF fill, COLORREF outline, int radius, int is_filled, int is_outlined, pngl_uc *buffer) {
    int min_x = min(x0, x1);
    int max_x = max(x0, x1);
    int min_y = min(y0, y1);
    int max_y = max(y0, y1);
    
    if (is_filled) {
        for (int y = min_y; y <= max_y; y++) {
            for (int x = min_x; x <= max_x; x++) {
                draw_pixel_to_buffer(x, y, fill, 255, buffer);
            }
        }
    }
    
    if (is_outlined) {
        draw_line_segment_to_buffer(min_x, min_y, max_x, min_y, outline, radius, buffer);
        draw_line_segment_to_buffer(min_x, max_y, max_x, max_y, outline, radius, buffer);
        draw_line_segment_to_buffer(min_x, min_y, min_x, max_y, outline, radius, buffer);
        draw_line_segment_to_buffer(max_x, min_y, max_x, max_y, outline, radius, buffer);
    }
}

void draw_circle_to_buffer_shape(int x0, int y0, int x1, int y1, COLORREF fill, COLORREF outline, int radius, int is_filled, int is_outlined, pngl_uc *buffer) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int rad = (int)(sqrt((float)dx * dx + (float)dy * dy));
    int cx = x0; 
    int cy = y0; 

    if (is_filled) {
        int r_sq = rad * rad;
        for (int y = -rad; y <= rad; y++) {
            for (int x = -rad; x <= rad; x++) {
                if (x * x + y * y <= r_sq) {
                    draw_pixel_to_buffer(cx + x, cy + y, fill, 255, buffer);
                }
            }
        }
    }
    
    if (is_outlined) {
        int pen_radius = radius;
        int x, y, err;
        
        x = rad;
        y = 0;
        err = 1 - x;
        
        while(x >= y) {
            draw_circle_to_buffer_primitive(cx + x, cy + y, pen_radius, outline, 255, buffer);
            draw_circle_to_buffer_primitive(cx + y, cy + x, pen_radius, outline, 255, buffer);
            draw_circle_to_buffer_primitive(cx - y, cy + x, pen_radius, outline, 255, buffer);
            draw_circle_to_buffer_primitive(cx - x, cy + y, pen_radius, outline, 255, buffer);
            draw_circle_to_buffer_primitive(cx - x, cy - y, pen_radius, outline, 255, buffer);
            draw_circle_to_buffer_primitive(cx - y, cy - x, pen_radius, outline, 255, buffer);
            draw_circle_to_buffer_primitive(cx + y, cy - x, pen_radius, outline, 255, buffer);
            draw_circle_to_buffer_primitive(cx + x, cy - y, pen_radius, outline, 255, buffer);

            y++;
            if (err < 0) {
                err += 2 * y + 1;
            } else {
                x--;
                err += 2 * (y - x) + 1;
            }
        }
    }
}

void draw_grid_to_buffer(int x0, int y0, int x1, int y1, COLORREF outline, int radius, int rows, int cols, pngl_uc *buffer) {
    int min_x = min(x0, x1);
    int max_x = max(x0, x1);
    int min_y = min(y0, y1);
    int max_y = max(y0, y1);
    
    int w = max_x - min_x;
    int h = max_y - min_y;

    if (rows < 1) rows = 1;
    if (cols < 1) cols = 1;

    draw_rectangle_to_buffer(min_x, min_y, max_x, max_y, outline, outline, radius, 0, 1, buffer);
    
    for (int i = 1; i < cols; ++i) {
        int x = min_x + (w * i) / cols;
        draw_line_segment_to_buffer(x, min_y, x, max_y, outline, radius, buffer);
    }
    
    for (int j = 1; j < rows; ++j) {
        int y = min_y + (h * j) / rows;
        draw_line_segment_to_buffer(min_x, y, max_x, y, outline, radius, buffer);
    }
}

void flood_fill_to_buffer(int x, int y, COLORREF new_color, pngl_uc *buffer) {
    Point stack[MAX_FILL_STACK];
    int top = 0;
    
    if (x < 0 || x >= g_fb_w || y < 0 || y >= g_fb_h || buffer == NULL) return; 

    {
        int index = (y * g_fb_w + x) * 4;
        pngl_uc target_b = buffer[index + 0];
        pngl_uc target_g = buffer[index + 1];
        pngl_uc target_r = buffer[index + 2];
        pngl_uc target_a = buffer[index + 3];

        pngl_uc new_r = GetRValue(new_color);
        pngl_uc new_g = GetGValue(new_color);
        pngl_uc new_b = GetBValue(new_color);
        
        new_r = (pngl_uc)(((unsigned int)new_r * 255) / 255);
        new_g = (pngl_uc)(((unsigned int)new_g * 255) / 255);
        new_b = (pngl_uc)(((unsigned int)new_b * 255) / 255);

        if (target_r == new_r && target_g == new_g && target_b == new_b && target_a == 255) return; 
        
        stack[top++] = (Point){x, y};

        while (top > 0) {
            Point p = stack[--top];
            int px = p.x;
            int py = p.y;
            int index_check;

            /* Check boundary before calculating index */
            if (px < 0 || px >= g_fb_w || py < 0 || py >= g_fb_h) continue;

            index_check = (py * g_fb_w + px) * 4;

            if (buffer[index_check + 0] == target_b && 
                buffer[index_check + 1] == target_g && 
                buffer[index_check + 2] == target_r && 
                buffer[index_check + 3] == target_a) 
            {
                buffer[index_check + 0] = new_b;
                buffer[index_check + 1] = new_g;
                buffer[index_check + 2] = new_r;
                buffer[index_check + 3] = 255; 

                if (top < MAX_FILL_STACK - 4) {
                    stack[top++] = (Point){px + 1, py};
                    stack[top++] = (Point){px - 1, py};
                    stack[top++] = (Point){px, py + 1};
                    stack[top++] = (Point){px, py - 1};
                }
            }
        }
    }
}


void handle_drawing_move(HWND hWnd, int current_x, int current_y) {
    if (!g_is_drawing_mode || !g_mouse_down || g_framebuffer_data == NULL) return;
    
    int fb_x1 = current_x; /* CLEANUP: Removed division by DRAWING_SCALE_FACTOR (1) */
    int fb_y1 = current_y;
    int fb_x0 = g_last_mouse_x;
    int fb_y0 = g_last_mouse_y;

    if (g_draw_tool_mode == 2) {
        clear_temp_layer(); 
        
        if (g_temp_framebuffer_data != NULL) {
            switch (g_shape_mode) {
                case 0: draw_line_segment_to_buffer(g_shape_start_x, g_shape_start_y, fb_x1, fb_y1, g_shape_outline_color, g_draw_size / 2, g_temp_framebuffer_data); break;
                case 1: draw_rectangle_to_buffer(g_shape_start_x, g_shape_start_y, fb_x1, fb_y1, g_shape_fill_color, g_shape_outline_color, g_draw_size / 2, g_shape_is_filled, g_shape_is_outlined, g_temp_framebuffer_data); break;
                case 2: draw_circle_to_buffer_shape(g_shape_start_x, g_shape_start_y, fb_x1, fb_y1, g_shape_fill_color, g_shape_outline_color, g_draw_size / 2, g_shape_is_filled, g_shape_is_outlined, g_temp_framebuffer_data); break;
                case 3: draw_grid_to_buffer(g_shape_start_x, g_shape_start_y, fb_x1, fb_y1, g_shape_outline_color, g_draw_size / 2, g_grid_rows, g_grid_cols, g_temp_framebuffer_data); break;
            }
        }

    } else if (g_draw_tool_mode != 3) {
        draw_line_to_framebuffer(fb_x0, fb_y0, fb_x1, fb_y1);
    }

    g_last_mouse_x = current_x;
    g_last_mouse_y = current_y;

    InvalidateRect(hWnd, NULL, FALSE);
}

void bake_current_layer(HWND hWnd) {
    if (g_framebuffer_data == NULL || g_drawing_count >= MAX_DRAWING_OBJECTS) return;

    int min_x = g_fb_w, max_x = 0;
    int min_y = g_fb_h, max_y = 0;
    int found_pixel = 0;

    for (int y = 0; y < g_fb_h; y++) {
        for (int x = 0; x < g_fb_w; x++) {
            pngl_uc *pixel = g_framebuffer_data + (y * g_fb_w + x) * 4;
            if (pixel[3] > 0) { 
                if (x < min_x) min_x = x;
                if (x > max_x) max_x = x;
                if (y < min_y) min_y = y;
                if (y > max_y) max_y = y;
                found_pixel = 1;
            }
        }
    }

    if (!found_pixel) return; 

    int border = g_draw_size * 2;
    min_x = max(0, min_x - border);
    min_y = max(0, min_y - border);
    max_x = min(g_fb_w - 1, max_x + border);
    max_y = min(g_fb_h - 1, max_y + border);

    int new_w = max_x - min_x + 1;
    int new_h = max_y - min_y + 1;

    pngl_uc *baked_data = (pngl_uc *)malloc((size_t)new_w * new_h * 4);
    if (baked_data == NULL) {
        return;
    }

    for (int y = 0; y < new_h; y++) {
        for (int x = 0; x < new_w; x++) {
            pngl_uc *src = g_framebuffer_data + ((min_y + y) * g_fb_w + (min_x + x)) * 4;
            pngl_uc *dst = baked_data + (y * new_w + x) * 4;

            dst[0] = src[0];
            dst[1] = src[1];
            dst[2] = src[2];
            dst[3] = src[3];
        }
    }
    
    if (g_drawing_count < MAX_DRAWING_OBJECTS) {
        g_drawing_count++;
    } else {
        if (g_saved_drawings[0].data != NULL) pngl_image_free(g_saved_drawings[0].data);
        for(int i = 0; i < MAX_DRAWING_OBJECTS - 1; ++i) {
            g_saved_drawings[i] = g_saved_drawings[i + 1];
        }
    }
    
    SavedDrawingObject *new_obj = &g_saved_drawings[g_drawing_count - 1];
    
    new_obj->data = baked_data;
    new_obj->w = new_w;
    new_obj->h = new_h;
    
    /* FIX: Convert screen coordinates (min_x, min_y) to world space */
    new_obj->virtual_x = ((float)min_x - g_offset_x) / g_scale;
    new_obj->virtual_y = ((float)min_y - g_offset_y) / g_scale;
    new_obj->scale_at_save = g_scale; 
    
    clear_drawing_layer(hWnd);
    clear_temp_layer();
    g_is_drawing_mode = 0;
    g_selected_object_index = g_drawing_count - 1;
    update_window_title(hWnd);
    InvalidateRect(hWnd, NULL, FALSE);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int mouse_x, mouse_y;
    int delta;
    float old_scale, new_scale;
    float scale_factor;
    
    switch (message) {
        case WM_CREATE:
            update_window_title(hWnd);
            SetTimer(hWnd, TIMER_ID_UPDATE, TIMER_INTERVAL_MS, NULL);
            SetTimer(hWnd, TIMER_ID_CLIPBOARD, TIMER_INTERVAL_CLIPBOARD_MS, NULL);
            DragAcceptFiles(hWnd, TRUE);
            SetMenu(hWnd, create_main_menu());
            DrawMenuBar(hWnd);
            break;

        case WM_TIMER:
            if (wParam == TIMER_ID_UPDATE) {
                check_for_file_updates(hWnd);
            } else if (wParam == TIMER_ID_CLIPBOARD) {
                int available = (IsClipboardFormatAvailable(CF_DIB) || IsClipboardFormatAvailable(CF_BITMAP) || IsClipboardFormatAvailable(CF_HDROP));
                if (available != g_paste_image_enabled) {
                    g_paste_image_enabled = available;
                    SetMenu(hWnd, create_main_menu());
                    DrawMenuBar(hWnd);
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
            } else if (wParam == 'R' && g_image_data != NULL) {
                reset_view(hWnd);
            } else if (wParam == VK_TAB) { 
                if (g_is_drawing_mode) {
                    g_draw_tool_mode = (g_draw_tool_mode + 1) % 4; 
                    SetMenu(hWnd, create_main_menu()); 
                    DrawMenuBar(hWnd);
                } else if (g_draw_mode != 2) { 
                    g_draw_mode = (g_draw_mode + 1) % 3; 
                    
                    if (g_draw_mode == 2) {
                        g_scale = 1.0f;
                        g_offset_x = 0.0f;
                        g_offset_y = 0.0f;
                    }
                }
                
                update_window_title(hWnd);
                InvalidateRect(hWnd, NULL, FALSE);
            } else if (wParam == 'A') { 
                g_is_drawing_mode = !g_is_drawing_mode;
                clear_temp_layer();
                SetMenu(hWnd, create_main_menu()); 
                DrawMenuBar(hWnd);
                update_window_title(hWnd);
                InvalidateRect(hWnd, NULL, FALSE);
            } else if (wParam == VK_ESCAPE) { 
                if (g_is_drawing_mode) {
                    clear_drawing_layer(hWnd);
                    clear_temp_layer(); 
                }
            } else if (wParam == VK_DELETE || wParam == VK_BACK) {
                if (g_selected_object_index != -1) {
                    delete_selected_object(hWnd);
                }
            } else if (GetKeyState(VK_CONTROL) & 0x8000) {
                if (wParam == 'C' && g_image_data != NULL) {
                    copy_image_to_clipboard(hWnd);
                } else if (wParam == 'V') {
                    if (g_paste_image_enabled) {
                        paste_image_from_clipboard(hWnd);
                    }
                } else if (wParam == 'S') { 
                    if (g_is_drawing_mode) {
                        bake_current_layer(hWnd);
                        SetMenu(hWnd, create_main_menu()); 
                        DrawMenuBar(hWnd);
                    } else {
                        save_view_to_file(hWnd);
                    }
                } else if (wParam == 'X') {
                    if (g_selected_object_index != -1) {
                        delete_selected_object(hWnd);
                    }
                }
            }
            break;

        case WM_LBUTTONDOWN:
            mouse_x = GET_X_LPARAM(lParam);
            mouse_y = GET_Y_LPARAM(lParam);
            
            if (g_is_drawing_mode) {
                g_mouse_down = 1;
                g_last_mouse_x = mouse_x;
                g_last_mouse_y = mouse_y;
                SetCapture(hWnd);
                
                {
                    RECT client_rect;
                    GetClientRect(hWnd, &client_rect);
                    init_drawing_buffers(client_rect.right, client_rect.bottom);
                }
                
                if (g_draw_tool_mode == 2) { 
                    g_shape_start_x = mouse_x; /* CLEANUP: Removed division by DRAWING_SCALE_FACTOR (1) */
                    g_shape_start_y = mouse_y;
                    clear_temp_layer();
                } else if (g_draw_tool_mode == 3) {
                    flood_fill_to_buffer(mouse_x, mouse_y, g_draw_color, g_framebuffer_data);
                    g_mouse_down = 0; 
                } else {
                    handle_drawing_move(hWnd, mouse_x, mouse_y); 
                }
                
                g_selected_object_index = -1;
            } else {
                int hit_index = -1;
                for (int i = g_drawing_count - 1; i >= 0; --i) {
                    const SavedDrawingObject *obj = &g_saved_drawings[i];
                    
                    int obj_screen_w = (int)((float)obj->w * (g_scale / obj->scale_at_save));
                    int obj_screen_h = (int)((float)obj->h * (g_scale / obj->scale_at_save));
                    
                    int obj_screen_x = (int)(obj->virtual_x * g_scale + g_offset_x);
                    int obj_screen_y = (int)(obj->virtual_y * g_scale + g_offset_y);
                    
                    if (mouse_x >= obj_screen_x && mouse_x <= obj_screen_x + obj_screen_w &&
                        mouse_y >= obj_screen_y && mouse_y <= obj_screen_y + obj_screen_h) 
                    {
                        hit_index = i;
                        break; 
                    }
                }
                
                g_selected_object_index = hit_index;
                
                if (hit_index != -1) {
                    g_dragging_object_index = hit_index;
                } else {
                    g_dragging_object_index = -1;
                }

                g_mouse_down = 1;
                g_last_mouse_x = mouse_x;
                g_last_mouse_y = mouse_y;
                SetCapture(hWnd);
            }
            update_window_title(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
            break;
            
        case WM_MBUTTONDOWN:
            mouse_x = GET_X_LPARAM(lParam);
            mouse_y = GET_Y_LPARAM(lParam);
            
            g_mouse_mid_down = 1;
            g_last_mouse_x = mouse_x;
            g_last_mouse_y = mouse_y;
            SetCapture(hWnd);
            
            if (g_selected_object_index != -1) {
                g_selected_object_index = -1;
                update_window_title(hWnd);
            }
            InvalidateRect(hWnd, NULL, FALSE);
            break;

        case WM_LBUTTONUP:
            if (g_is_drawing_mode && g_draw_tool_mode == 2 && g_mouse_down) {
                int end_x = GET_X_LPARAM(lParam);
                int end_y = GET_Y_LPARAM(lParam);
                
                if (g_framebuffer_data != NULL) {
                    switch (g_shape_mode) {
                        case 0: draw_line_segment_to_buffer(g_shape_start_x, g_shape_start_y, end_x, end_y, g_shape_outline_color, g_draw_size / 2, g_framebuffer_data); break;
                        case 1: draw_rectangle_to_buffer(g_shape_start_x, g_shape_start_y, end_x, end_y, g_shape_fill_color, g_shape_outline_color, g_draw_size / 2, g_shape_is_filled, g_shape_is_outlined, g_framebuffer_data); break;
                        case 2: draw_circle_to_buffer_shape(g_shape_start_x, g_shape_start_y, end_x, end_y, g_shape_fill_color, g_shape_outline_color, g_draw_size / 2, g_shape_is_filled, g_shape_is_outlined, g_framebuffer_data); break;
                        case 3: draw_grid_to_buffer(g_shape_start_x, g_shape_start_y, end_x, end_y, g_shape_outline_color, g_draw_size / 2, g_grid_rows, g_grid_cols, g_framebuffer_data); break;
                    }
                }
                clear_temp_layer(); 
            }
            
            g_mouse_down = 0;
            g_dragging_object_index = -1;
            ReleaseCapture();
            InvalidateRect(hWnd, NULL, FALSE);
            break;
            
        case WM_MBUTTONUP:
            g_mouse_mid_down = 0;
            ReleaseCapture();
            InvalidateRect(hWnd, NULL, FALSE);
            break;

        case WM_MOUSEMOVE:
            if (g_mouse_down || g_mouse_mid_down) {
                int current_x = GET_X_LPARAM(lParam);
                int current_y = GET_Y_LPARAM(lParam);
                
                if (g_is_drawing_mode) {
                    if (g_mouse_mid_down) {
                         /* Panning in Drawing Mode (Middle Mouse) */
                        g_offset_x += (float)(current_x - g_last_mouse_x);
                        g_offset_y += (float)(current_y - g_last_mouse_y);
                    } else if (g_mouse_down && g_draw_tool_mode != 3) {
                        /* Drawing/Moving in Drawing Mode (Left Mouse) */
                        if (g_draw_tool_mode == 2) {
                            /* Shape Preview */
                            clear_temp_layer();
                            if (g_temp_framebuffer_data != NULL) {
                                switch (g_shape_mode) {
                                    case 0: draw_line_segment_to_buffer(g_shape_start_x, g_shape_start_y, current_x, current_y, g_shape_outline_color, g_draw_size / 2, g_temp_framebuffer_data); break;
                                    case 1: draw_rectangle_to_buffer(g_shape_start_x, g_shape_start_y, current_x, current_y, g_shape_fill_color, g_shape_outline_color, g_draw_size / 2, g_shape_is_filled, g_shape_is_outlined, g_temp_framebuffer_data); break;
                                    case 2: draw_circle_to_buffer_shape(g_shape_start_x, g_shape_start_y, current_x, current_y, g_shape_fill_color, g_shape_outline_color, g_draw_size / 2, g_shape_is_filled, g_shape_is_outlined, g_temp_framebuffer_data); break;
                                    case 3: draw_grid_to_buffer(g_shape_start_x, g_shape_start_y, current_x, current_y, g_shape_outline_color, g_draw_size / 2, g_grid_rows, g_grid_cols, g_temp_framebuffer_data); break;
                                }
                            }
                        } else {
                             /* Continuous Pen/Eraser drawing on main buffer */
                            draw_line_to_framebuffer(g_last_mouse_x, g_last_mouse_y, current_x, current_y);
                        }
                    }
                } else {
                    /* Not Drawing Mode */
                    if (g_mouse_mid_down || (g_mouse_down && g_dragging_object_index == -1)) {
                        /* Panning (Middle Mouse always pans, Left Mouse pans if no object is grabbed) */
                        if (g_draw_mode == 0 || g_draw_mode == 1) {
                            g_offset_x += (float)(current_x - g_last_mouse_x);
                            g_offset_y += (float)(current_y - g_last_mouse_y);
                        }
                    } else if (g_mouse_down && g_dragging_object_index != -1) {
                        /* Moving a saved object (Left Mouse only) - FIX APPLIED HERE */
                        SavedDrawingObject *obj = &g_saved_drawings[g_dragging_object_index];
                        
                        float dx_screen = (float)(current_x - g_last_mouse_x);
                        float dy_screen = (float)(current_y - g_last_mouse_y);
                        
                        /* FIX: Global scale (g_scale) is the factor to convert screen delta to world delta. */
                        float dx_world = dx_screen / g_scale; 
                        float dy_world = dy_screen / g_scale;
                        
                        /* Update object's virtual position in world space. 
                           The object rendering recalculates scale relative to g_scale/obj->scale_at_save. */
                        obj->virtual_x += dx_world;
                        obj->virtual_y += dy_world;
                    }
                }
                
                g_last_mouse_x = current_x;
                g_last_mouse_y = current_y;
                
                RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
            }
            break;

        case WM_MOUSEWHEEL:
            delta = GET_WHEEL_DELTA_WPARAM(wParam);
            
            if (GetKeyState(VK_CONTROL) & 0x8000) {
                if (g_is_drawing_mode) {
                    if (delta > 0) {
                        g_draw_size = min(50, g_draw_size + 2);
                    } else {
                        g_draw_size = max(1, g_draw_size - 2);
                    }
                    SetMenu(hWnd, create_main_menu()); 
                    DrawMenuBar(hWnd);
                    update_window_title(hWnd);
                    return 0; 
                }
            }
            
            if (g_image_data != NULL || g_drawing_count > 0 || g_is_drawing_mode) {
              POINT pt;
        			pt.x = GET_X_LPARAM(lParam);
       			 	pt.y = GET_Y_LPARAM(lParam);
        			ScreenToClient(hWnd, &pt);
        			
                    /* FIX: Assign converted client coordinates to mouse_x/y */
        			mouse_x = pt.x;
        			mouse_y = pt.y;

							old_scale = g_scale;
              scale_factor = (delta > 0) ? 1.2f : (1.0f / 1.2f);
              new_scale = g_scale * scale_factor;

                if (new_scale > 10.0f) new_scale = 10.0f;
                if (new_scale < 0.05f) new_scale = 0.05f;
                
                if (new_scale != old_scale) {
                    float image_x_at_cursor = ((float)mouse_x - g_offset_x) / old_scale;
                    float image_y_at_cursor = ((float)mouse_y - g_offset_y) / old_scale;
                    
                    g_offset_x = (float)mouse_x - (image_x_at_cursor * new_scale);
                    g_offset_y = (float)mouse_y - (image_y_at_cursor * new_scale);
                    
                    g_scale = new_scale;
                    
                    update_window_title(hWnd);
                    RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
                }
            }
            break;
            
        case WM_CONTEXTMENU:
        {
            POINT pt;
            int cmd;
            pngl_uc r, g_c, b, a;
            int img_x, img_y;
            
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);

            if (g_hContextMenu != NULL) {
                DestroyMenu(g_hContextMenu);
            }
            g_hContextMenu = create_context_menu(); 
            if (g_hContextMenu == NULL) break;
            
            if (ScreenToClient(hWnd, &pt)) {
                
                if (g_image_data != NULL && pixel_to_rgb_and_coords(hWnd, pt.x, pt.y, &r, &g_c, &b, &a, &img_x, &img_y)) {
                    EnableMenuItem(g_hContextMenu, IDM_COPY_HEX, MF_ENABLED);
                    EnableMenuItem(g_hContextMenu, IDM_COPY_RGBA, MF_ENABLED);
                    EnableMenuItem(g_hContextMenu, IDM_COPY_NORMALIZED, MF_ENABLED);
                } else {
                    EnableMenuItem(g_hContextMenu, IDM_COPY_HEX, MF_GRAYED);
                    EnableMenuItem(g_hContextMenu, IDM_COPY_RGBA, MF_GRAYED);
                    EnableMenuItem(g_hContextMenu, IDM_COPY_NORMALIZED, MF_GRAYED);
                }

                if (g_image_data != NULL) {
                     EnableMenuItem(g_hContextMenu, IDM_COPY_IMAGE, MF_ENABLED);
                } else {
                     EnableMenuItem(g_hContextMenu, IDM_COPY_IMAGE, MF_GRAYED);
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
            pngl_uc r, g_c, b, a;
            int img_x, img_y;
            char buffer[64];
            POINT cursor_pos;
            
            if (command_id >= IDM_DRAW_MENU_BASE) {
                switch (command_id) {
                    case IDM_DRAW_COLOR_RED: g_draw_color = RGB(255, 0, 0); break;
                    case IDM_DRAW_COLOR_GREEN: g_draw_color = RGB(0, 255, 0); break;
                    case IDM_DRAW_COLOR_BLUE: g_draw_color = RGB(0, 0, 255); break;
                    case IDM_DRAW_COLOR_BLACK: g_draw_color = RGB(0, 0, 0); break;
                    case IDM_DRAW_COLOR_WHITE: g_draw_color = RGB(255, 255, 255); break;
                    
                    case IDM_DRAW_TOOL_PEN: g_draw_tool_mode = 0; clear_temp_layer(); SetMenu(hWnd, create_main_menu()); DrawMenuBar(hWnd); break;
                    case IDM_DRAW_TOOL_ERASER: g_draw_tool_mode = 1; clear_temp_layer(); SetMenu(hWnd, create_main_menu()); DrawMenuBar(hWnd); break;
                    case IDM_DRAW_TOOL_SHAPE: g_draw_tool_mode = 2; clear_temp_layer(); SetMenu(hWnd, create_main_menu()); DrawMenuBar(hWnd); break;
                    case IDM_DRAW_TOOL_FILL: g_draw_tool_mode = 3; clear_temp_layer(); SetMenu(hWnd, create_main_menu()); DrawMenuBar(hWnd); break;

                    case IDM_DRAW_CLEAR: clear_drawing_layer(hWnd); clear_temp_layer(); break;
                    case IDM_DRAW_TOGGLE: 
                        g_is_drawing_mode = !g_is_drawing_mode; 
                        clear_temp_layer(); 
                        SetMenu(hWnd, create_main_menu()); 
                        DrawMenuBar(hWnd);
                        break;
                    case IDM_DRAW_GRID_TOGGLE: g_is_grid_overlay = !g_is_grid_overlay; break;
                    
                    case IDM_DRAW_COLOR_SELECTOR:
                        {
                            CHOOSECOLOR cc;
                            ZeroMemory(&cc, sizeof(cc));
                            cc.lStructSize = sizeof(cc);
                            cc.hwndOwner = hWnd;
                            cc.lpCustColors = (LPDWORD)g_custom_colors;
                            cc.rgbResult = g_draw_color;
                            cc.Flags = CC_FULLOPEN | CC_RGBINIT;
                            
                            if (ChooseColor(&cc)) {
                                g_draw_color = cc.rgbResult;
                            }
                        }
                        break;
                        
                    case IDM_SHAPE_LINE: g_shape_mode = 0; break;
                    case IDM_SHAPE_RECTANGLE: g_shape_mode = 1; break;
                    case IDM_SHAPE_CIRCLE: g_shape_mode = 2; break;
                    case IDM_SHAPE_GRID: g_shape_mode = 3; SetMenu(hWnd, create_main_menu()); DrawMenuBar(hWnd); break;

                    case IDM_SHAPE_FILL_TOGGLE: 
                        g_shape_is_filled = !g_shape_is_filled; 
                        if (!g_shape_is_filled && !g_shape_is_outlined) g_shape_is_outlined = 1; 
                        SetMenu(hWnd, create_main_menu()); DrawMenuBar(hWnd);
                        break;

                    case IDM_SHAPE_OUTLINE_TOGGLE: 
                        g_shape_is_outlined = !g_shape_is_outlined; 
                        if (!g_shape_is_filled && !g_shape_is_outlined) g_shape_is_filled = 1; 
                        SetMenu(hWnd, create_main_menu()); DrawMenuBar(hWnd);
                        break;

                    case IDM_SHAPE_FILL_COLOR:
                        {
                            CHOOSECOLOR cc;
                            ZeroMemory(&cc, sizeof(cc));
                            cc.lStructSize = sizeof(cc);
                            cc.hwndOwner = hWnd;
                            cc.lpCustColors = (LPDWORD)g_custom_colors;
                            cc.rgbResult = g_shape_fill_color;
                            cc.Flags = CC_FULLOPEN | CC_RGBINIT;
                            
                            if (ChooseColor(&cc)) {
                                g_shape_fill_color = cc.rgbResult;
                            }
                        }
                        break;
                    
                    case IDM_SHAPE_OUTLINE_COLOR:
                        {
                            CHOOSECOLOR cc;
                            ZeroMemory(&cc, sizeof(cc));
                            cc.lStructSize = sizeof(cc);
                            cc.hwndOwner = hWnd;
                            cc.lpCustColors = (LPDWORD)g_custom_colors;
                            cc.rgbResult = g_shape_outline_color;
                            cc.Flags = CC_FULLOPEN | CC_RGBINIT;
                            
                            if (ChooseColor(&cc)) {
                                g_shape_outline_color = cc.rgbResult;
                            }
                        }
                        break;
                    
                    case IDM_GRID_ROWS_INC: g_grid_rows = min(50, g_grid_rows + 1); SetMenu(hWnd, create_main_menu()); DrawMenuBar(hWnd); break;
                    case IDM_GRID_ROWS_DEC: g_grid_rows = max(1, g_grid_rows - 1); SetMenu(hWnd, create_main_menu()); DrawMenuBar(hWnd); break;
                    case IDM_GRID_COLS_INC: g_grid_cols = min(50, g_grid_cols + 1); SetMenu(hWnd, create_main_menu()); DrawMenuBar(hWnd); break;
                    case IDM_GRID_COLS_DEC: g_grid_cols = max(1, g_grid_cols - 1); SetMenu(hWnd, create_main_menu()); DrawMenuBar(hWnd); break;
                }
                update_window_title(hWnd);
                InvalidateRect(hWnd, NULL, FALSE);
                break; 
            }

            if (command_id == IDM_COPY_HEX || command_id == IDM_COPY_RGBA || command_id == IDM_COPY_NORMALIZED) {
                
                GetCursorPos(&cursor_pos);
                ScreenToClient(hWnd, &cursor_pos);
                
                if (!pixel_to_rgb_and_coords(hWnd, cursor_pos.x, cursor_pos.y, &r, &g_c, &b, &a, &img_x, &img_y)) {
                    break;
                }
            }

            switch (command_id) {
                case IDM_COPY_HEX:
                    sprintf(buffer, "0x%02X%02X%02X%02X", a, r, g_c, b);
                    copy_text_to_clipboard(hWnd, buffer);
                    break;
                    
                case IDM_COPY_RGBA:
                    sprintf(buffer, "%d,%d,%d,%d", r, g_c, b, a);
                    copy_text_to_clipboard(hWnd, buffer);
                    break;
                    
                case IDM_COPY_NORMALIZED:
                    sprintf(buffer, "%.3f,%.3f,%.3f,%.3f", 
                            (float)r / 255.0f, (float)g_c / 255.0f, (float)b / 255.0f, (float)a / 255.0f);
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
                
                case IDM_SAVE_VIEW:
                    if (g_is_drawing_mode) {
                        bake_current_layer(hWnd);
                        SetMenu(hWnd, create_main_menu()); 
                        DrawMenuBar(hWnd);
                    } else {
                        save_view_to_file(hWnd);
                    }
                    break;
            }
        }
        break;

        case WM_PAINT:
            do_double_buffered_paint(hWnd);
            break;

        case WM_SIZE:
             {
                RECT client_rect;
                GetClientRect(hWnd, &client_rect);
                int client_w = client_rect.right;
                int client_h = client_rect.bottom;

                /* CRITICAL FIX: Reallocate drawing buffers on size change */
                init_drawing_buffers(client_w, client_h);
                
                if (client_w != g_framebuffer_w || client_h != g_framebuffer_h) {
                    /* Only trigger redraw if size actually changed to avoid recursive paint */
                    InvalidateRect(hWnd, NULL, FALSE);
                }
                
                if (g_image_data != NULL && g_scale == 1.0f) {
                    g_offset_x = (float)(client_w - g_width) / 2.0f;
                    g_offset_y = (float)(client_h - g_height) / 2.0f;
                }
            }
            break;

        case WM_DESTROY:
            KillTimer(hWnd, TIMER_ID_UPDATE);
            KillTimer(hWnd, TIMER_ID_CLIPBOARD);
            cleanup_drawing_objects();
            if (g_hContextMenu != NULL) {
                DestroyMenu(g_hContextMenu);
            }
            if (g_framebuffer != NULL) {
                free(g_framebuffer);
                g_framebuffer = NULL;
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
    HWND hWnd;
    WNDCLASSEX wc;
    MSG msg;
    RECT window_rect;
    int window_w = 800, window_h = 600; 
    
    memset(&app, 0, sizeof(AppState));
    if (argparse_process_args((ArgParseState *)&app, __argv, __argc, option_table)) {
        return 1;
    }

    if (app.image_path != NULL) {
        if (strlen(app.image_path) >= MAX_PATH_CUSTOM) {
            fprintf(stderr, "error: file path is too long (max %d characters)\\n");
            
        } else {
             if (!load_image_from_path(NULL, app.image_path)) {
                 fprintf(stderr, "error: failed to load image '%s'\\n", app.image_path);
            }
        }
    }

    sprintf(g_window_title, "Image Viewer: No Image Loaded (Mode: %s)", get_mode_name(g_draw_mode));
    
    
    if (g_image_data != NULL) {
        window_rect.left = 0;
        window_rect.top = 0;
        window_rect.right = g_width;
        window_rect.bottom = g_height;
        
        if (window_rect.right > 1600) window_rect.right = 1600;
        if (window_rect.bottom > 1000) window_rect.bottom = 1000;
        
        AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, FALSE);
        
        window_w = window_rect.right - window_rect.left;
        window_h = window_rect.bottom - window_rect.top;
    } else {
        window_rect.left = 0;
        window_rect.top = 0;
        window_rect.right = window_w;
        window_rect.bottom = window_h;
        AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, FALSE);
        window_w = window_rect.right - window_rect.left;
        window_h = window_rect.bottom - window_rect.top;
    }


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
        fprintf(stderr, "error: failed to register window class\\n");
        if (g_image_data != NULL) pngl_image_free(g_image_data);
        return 1;
    }

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
        fprintf(stderr, "error: failed to create window\\n");
        if (g_image_data != NULL) pngl_image_free(g_image_data);
        return 1;
    }
    
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd); 
    
    {
        RECT client_rect;
        GetClientRect(hWnd, &client_rect);
        /* Initialize drawing buffers using the starting client size */
        init_drawing_buffers(client_rect.right, client_rect.bottom);
    }

    if (g_image_data != NULL) {
        RECT client_rect;
        GetClientRect(hWnd, &client_rect);
        g_offset_x = (float)(client_rect.right - g_width) / 2.0f;
        g_offset_y = (float)(client_rect.bottom - g_height) / 2.0f;
        update_window_title(hWnd); 
    }

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (g_image_data != NULL) pngl_image_free(g_image_data);
    return (int)msg.wParam;
}