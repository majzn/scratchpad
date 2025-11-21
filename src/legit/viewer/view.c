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

#include "view.h"

#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>
#include <winuser.h>
#include <wingdi.h>
#include <ShlObj.h>
#include <CommDlg.h> 
#endif

SavedDrawingObject g_objects[MAX_DRAWING_OBJECTS] = {0};
int g_object_count = 0;

char g_window_title[MAX_PATH_CUSTOM + 64]; 

float g_scale = 1.0f;
float g_offset_x = 0.0f;
float g_offset_y = 0.0f;

int g_background_mode = 0;

int g_mouse_down = 0;
int g_mouse_mid_down = 0; 
int g_last_mouse_x = 0;
int g_last_mouse_y = 0;

int g_dragging_object_index = -1; 
int g_selected_object_index = -1;

u32 *g_framebuffer = NULL;
int g_framebuffer_w = 0;
int g_framebuffer_h = 0;

HMENU g_hContextMenu = NULL;
int g_paste_image_enabled = 0;

int g_is_drawing_mode = 0;
pngl_uc *g_framebuffer_data = NULL; 
pngl_uc *g_temp_framebuffer_data = NULL; 
int g_fb_w = 0;
int g_fb_h = 0;

COLORREF g_draw_color = RGB(255, 0, 0); 
int g_draw_size = 5; 
int g_draw_tool_mode = 0; 
int g_is_grid_overlay = 0; 

int g_shape_mode = 1; 
int g_shape_is_filled = 0; 
int g_shape_is_outlined = 1; 
COLORREF g_shape_fill_color = RGB(0, 0, 255); 
COLORREF g_shape_outline_color = RGB(0, 0, 0); 
int g_grid_rows = 5;
int g_grid_cols = 5;

int g_shape_start_x = 0;
int g_shape_start_y = 0;

COLORREF g_custom_colors[16] = {0}; 

SavedDrawingObject g_clipboard_object = {0}; 


ArgOption option_table[] = {
    {"-i", "--input", handle_image_path, "Specify the input image file path <val>", 0, 0},
    {"-h", "--help", (ArgParseFunc)-1, "Display this help message", 0, 0},
    {NULL, NULL, NULL, NULL, 0, 0}
};


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
            
            for (UINT i = 0; i < count; ++i) {
                if (DragQueryFile(hDrop, i, path, MAX_PATH_CUSTOM) > 0) {
                    load_file_as_new_object(hWnd, path); 
                }
            }
            DragFinish(hDrop);
        }
        break;

        case WM_KEYDOWN:
            if (wParam >= '0' && wParam <= '9') {
                g_background_mode = wParam - '0';
                InvalidateRect(hWnd, NULL, FALSE);
            } else if (wParam == 'R' && g_object_count > 0) {
                reset_view(hWnd);
            } else if (wParam == VK_TAB) { 
                if (g_is_drawing_mode) {
                    g_draw_tool_mode = (g_draw_tool_mode + 1) % 4; 
                    SetMenu(hWnd, create_main_menu()); 
                    DrawMenuBar(hWnd);
                } else if (g_selected_object_index != -1 && g_objects[g_selected_object_index].data != NULL) { 
                    SavedDrawingObject *obj = &g_objects[g_selected_object_index];
                    obj->mode = (obj->mode + 1) % 3;
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
                if (wParam == 'C' && g_object_count > 0) {
                    copy_object_to_clipboard(hWnd);
                } else if (wParam == 'V') {
                    if (g_paste_image_enabled || g_clipboard_object.data != NULL) {
                        paste_image_from_clipboard(hWnd);
                    }
                } else if (wParam == 'S') { 
                    save_objects_to_files(hWnd, g_is_drawing_mode ? 0 : g_selected_object_index != -1); 
                    return 0;
                } else if (wParam == 'X') {
                    if (g_selected_object_index != -1) {
                        delete_selected_object(hWnd);
                    }
                }
            }
            
            if (g_is_drawing_mode) {
                 if (wParam == 'E') { 
                    g_draw_tool_mode = 1; clear_temp_layer(); 
                    SetMenu(hWnd, create_main_menu()); DrawMenuBar(hWnd);
                    update_window_title(hWnd);
                 } else if (wParam == 'P') { 
                    g_draw_tool_mode = 0; clear_temp_layer(); 
                    SetMenu(hWnd, create_main_menu()); DrawMenuBar(hWnd);
                    update_window_title(hWnd);
                 } else if (wParam == 'R') { 
                    g_draw_tool_mode = 2; clear_temp_layer(); 
                    SetMenu(hWnd, create_main_menu()); DrawMenuBar(hWnd);
                    update_window_title(hWnd);
                 } else if (wParam == 'F') { 
                    g_draw_tool_mode = 3; clear_temp_layer(); 
                    SetMenu(hWnd, create_main_menu()); DrawMenuBar(hWnd);
                    update_window_title(hWnd);
                 } else if (wParam == 'C' && (g_draw_tool_mode == 0 || g_draw_tool_mode == 3)) { 
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
                     update_window_title(hWnd);
                 }
                 InvalidateRect(hWnd, NULL, FALSE);
            }
            break;

        case WM_LBUTTONDOWN:
            mouse_x = GET_X_LPARAM(lParam);
            mouse_y = GET_Y_LPARAM(lParam);
            
            if (g_is_drawing_mode) {
                {
                    RECT client_rect;
                    GetClientRect(hWnd, &client_rect);
                    if (g_framebuffer_data == NULL || g_fb_w != client_rect.right || g_fb_h != client_rect.bottom) {
                       init_drawing_buffers(client_rect.right, client_rect.bottom);
                    }
                }
                
                g_mouse_down = 1;
                g_last_mouse_x = mouse_x;
                g_last_mouse_y = mouse_y;
                SetCapture(hWnd);
                
                if (g_draw_tool_mode == 2) { 
                    g_shape_start_x = mouse_x; 
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
                
                for (int i = g_object_count - 1; i >= 0; --i) {
                    const SavedDrawingObject *obj = &g_objects[i];
                    if (obj->data == NULL) continue;
                    
                    int draw_w, draw_h, draw_x, draw_y;
                    get_object_screen_bounds(hWnd, obj, &draw_x, &draw_y, &draw_w, &draw_h);
                    
                    if (obj->mode == 0) { 
                        if (mouse_x >= draw_x && mouse_x <= draw_x + draw_w &&
                            mouse_y >= draw_y && mouse_y <= draw_y + draw_h) 
                        {
                            hit_index = i;
                            break; 
                        }
                    } else if (obj->mode == 1 || obj->mode == 2) {
                        hit_index = i;
                        break; 
                    }
                }
                
                g_selected_object_index = hit_index;
                
                if (hit_index != -1 && g_objects[hit_index].mode == 0) {
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
                        g_offset_x += (float)(current_x - g_last_mouse_x);
                        g_offset_y += (float)(current_y - g_last_mouse_y);
                    } else if (g_mouse_down && g_draw_tool_mode != 3) {
                        if (g_draw_tool_mode == 2) {
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
                            draw_line_to_framebuffer(g_last_mouse_x, g_last_mouse_y, current_x, current_y);
                        }
                    }
                } else {
                    if (g_mouse_mid_down || (g_mouse_down && g_dragging_object_index == -1)) {
                        g_offset_x += (float)(current_x - g_last_mouse_x);
                        g_offset_y += (float)(current_y - g_last_mouse_y);
                    } else if (g_mouse_down && g_dragging_object_index != -1) {
                        if (g_objects[g_dragging_object_index].mode == 0) {
                            SavedDrawingObject *obj = &g_objects[g_dragging_object_index];
                            
                            float dx_screen = (float)(current_x - g_last_mouse_x);
                            float dy_screen = (float)(current_y - g_last_mouse_y);
                            
                            float dx_world = dx_screen / g_scale; 
                            float dy_world = dy_screen / g_scale;
                            
                            obj->virtual_x += dx_world;
                            obj->virtual_y += dy_world;
                        }
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
            
            if (g_object_count > 0 || g_is_drawing_mode) {
              POINT pt;
        			pt.x = GET_X_LPARAM(lParam);
       			 	pt.y = GET_Y_LPARAM(lParam);
        			ScreenToClient(hWnd, &pt);
        			
        			mouse_x = pt.x;
        			mouse_y = pt.y;

							old_scale = g_scale;
              scale_factor = (delta > 0) ? 1.2f : (1.0f / 1.2f);
              new_scale = g_scale * scale_factor;

                if (new_scale > 100.0f) new_scale = 100.0f;
                if (new_scale < 0.01f) new_scale = 0.01f;
                
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
                
                if (g_object_count > 0 && g_objects[0].data != NULL && pixel_to_rgb_and_coords(hWnd, pt.x, pt.y, &r, &g_c, &b, &a, &img_x, &img_y)) {
                    EnableMenuItem(g_hContextMenu, IDM_COPY_HEX, MF_ENABLED);
                    EnableMenuItem(g_hContextMenu, IDM_COPY_RGBA, MF_ENABLED);
                    EnableMenuItem(g_hContextMenu, IDM_COPY_NORMALIZED, MF_ENABLED);
                } else {
                    EnableMenuItem(g_hContextMenu, IDM_COPY_HEX, MF_GRAYED);
                    EnableMenuItem(g_hContextMenu, IDM_COPY_RGBA, MF_GRAYED);
                    EnableMenuItem(g_hContextMenu, IDM_COPY_NORMALIZED, MF_GRAYED);
                }

                if (g_selected_object_index != -1 || g_object_count > 0) {
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
                    {
                        pngl_uc r_raw, g_raw, b_raw, a_raw;
                        pixel_to_rgb_and_coords(hWnd, cursor_pos.x, cursor_pos.y, &r_raw, &g_raw, &b_raw, &a_raw, &img_x, &img_y);
                        char buffer[64];
                        sprintf(buffer, "0x%02X%02X%02X%02X", a_raw, r_raw, g_raw, b_raw);
                        copy_text_to_clipboard(hWnd, buffer);
                    }
                    break;
                    
                case IDM_COPY_RGBA:
                    {
                        pngl_uc r_raw, g_raw, b_raw, a_raw;
                        pixel_to_rgb_and_coords(hWnd, cursor_pos.x, cursor_pos.y, &r_raw, &g_raw, &b_raw, &a_raw, &img_x, &img_y);
                        char buffer[64];
                        sprintf(buffer, "%d,%d,%d,%d", r_raw, g_raw, b_raw, a_raw);
                        copy_text_to_clipboard(hWnd, buffer);
                    }
                    break;
                    
                case IDM_COPY_NORMALIZED:
                    {
                        pngl_uc r_raw, g_raw, b_raw, a_raw;
                        pixel_to_rgb_and_coords(hWnd, cursor_pos.x, cursor_pos.y, &r_raw, &g_raw, &b_raw, &a_raw, &img_x, &img_y);
                        char buffer[64];
                        sprintf(buffer, "%.3f,%.3f,%.3f,%.3f", 
                                (float)r_raw / 255.0f, (float)g_raw / 255.0f, (float)b_raw / 255.0f, (float)a_raw / 255.0f);
                        copy_text_to_clipboard(hWnd, buffer);
                    }
                    break;
                    
                case IDM_COPY_IMAGE:
                    copy_object_to_clipboard(hWnd);
                    break;

                case IDM_PASTE_IMAGE:
                    if (g_paste_image_enabled || g_clipboard_object.data != NULL) {
                        paste_image_from_clipboard(hWnd);
                    }
                    break;
                
                case IDM_SAVE_VIEW:
                    save_objects_to_files(hWnd, g_is_drawing_mode ? 0 : g_selected_object_index != -1); 
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

                init_drawing_buffers(client_w, client_h);
                
                if (client_w != g_framebuffer_w || client_h != g_framebuffer_h) {
                    InvalidateRect(hWnd, NULL, FALSE);
                }
                
                if (g_object_count > 0 && g_objects[0].data != NULL && g_scale == 1.0f) {
                    g_offset_x = (float)(client_w - g_objects[0].w) / 2.0f;
                    g_offset_y = (float)(client_h - g_objects[0].h) / 2.0f;
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
    if (__argc > 1) {
			if(__argv[1][0] == '-') {
				if (argparse_process_args((ArgParseState *)&app, __argv, __argc, option_table)) {
        	return 1;
    		}
			} else {
				app.image_path = __argv[1];
			}
		}

    if (app.image_path != NULL) {
        if (strlen(app.image_path) >= MAX_PATH_CUSTOM) {
            fprintf(stderr, "error: file path is too long (max %d characters)\n", MAX_PATH_CUSTOM);
            
        } else {
             if (!load_image_from_path(NULL, app.image_path)) {
                 fprintf(stderr, "error: failed to load image '%s'\n", app.image_path);
            }
        }
    }

    sprintf(g_window_title, "Image Viewer: No Image Loaded (Mode: %s)", get_mode_name(0));
    
    
    if (g_object_count > 0 && g_objects[0].data != NULL) {
        window_rect.left = 0;
        window_rect.top = 0;
        window_rect.right = g_objects[0].w;
        window_rect.bottom = g_objects[0].h;
        
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
        fprintf(stderr, "error: failed to register window class\n");
        cleanup_drawing_objects();
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
        fprintf(stderr, "error: failed to create window\n");
        cleanup_drawing_objects();
        return 1;
    }
    
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd); 
    
    {
        RECT client_rect;
        GetClientRect(hWnd, &client_rect);
        init_drawing_buffers(client_rect.right, client_rect.bottom);
    }

    if (g_object_count > 0 && g_objects[0].data != NULL) {
        RECT client_rect;
        GetClientRect(hWnd, &client_rect);
        g_offset_x = (float)(client_rect.right - g_objects[0].w) / 2.0f;
        g_offset_y = (float)(client_rect.bottom - g_objects[0].h) / 2.0f;
        update_window_title(hWnd); 
    }

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    cleanup_drawing_objects();
    return (int)msg.wParam;
}
