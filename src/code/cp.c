#define _CRT_SECURE_NO_WARNINGS
#define OEMRESOURCE
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <Vss.h> 

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define GRID_SIZE 16
#define TIMER_ID 1
#define TIMER_INTERVAL 20

#define CORNER_RADIUS 35 
#define BORDER_THICKNESS 0 
#define BORDER_SLICE_SIZE 30 

#define BORDER_IMAGE_PATH L"image.png" 

#define RESIZE_GRIP_SIZE 10

#define FIXED_CELL_SIZE 15

#define SLOW_FACTOR 6

#define INNER_PADDING BORDER_SLICE_SIZE 

#define TEXT_AREA_HEIGHT 30
#define MARGIN 5
#define TEXT_BAR_HEIGHT 20

HHOOK g_hMouseHook = NULL;
HINSTANCE g_hInstance = NULL;
HWND g_hWnd = NULL;
int g_bSamplingActive = 0;
int g_bMagnifyDisplayActive = 0;

static int g_bPrecisionModeActive = 0;
static int s_bKeyWasDown = 0;
static int s_bShiftWasDown = 0;
static int s_bFreezeActive = 0;
static int s_bFKeyWasDown = 0;

static int s_currentCursorX = 0;
static int s_currentCursorY = 0;
static int s_lastTrackedCursorX = 0;
static int s_lastTrackedCursorY = 0;
static float s_accumulatedDeltaX = 0.0f;
static float s_accumulatedDeltaY = 0.0f;

static int s_bControlsActive = 0;
static int s_bIgnoreNextClick = 0;

static HWND g_hEditCurrent = NULL;
static HFONT g_hEditFont = NULL;

static WNDPROC g_pfnEditOriginalProc = NULL;

static int s_magnificationLevel = 4;
static const int MIN_MAGNIFICATION_LEVEL = 1;
static const int MAX_MAGNIFICATION_LEVEL = 20;
static const int MAG_STEP = 1;

static HBITMAP g_hFrozenBitmap = NULL;
static HBITMAP g_hOldFrozenBitmap = NULL;
static HDC g_hFrozenDC = NULL;
static int g_frozenCaptureWidth = 0;
static int g_frozenCaptureHeight = 0;
static int g_frozenScreenX = 0;
static int g_frozenScreenY = 0;

static int s_frozenDisplayX = 0;
static int s_frozenDisplayY = 0;

typedef struct {
  int r, g, b, a;
} ColorRGBA;

static ColorRGBA g_CurrentColor = {0, 0, 0, 0};
static ColorRGBA g_LastPickedColor = {0, 0, 0, 0};

static int s_lastDisplayX = 0;
static int s_lastDisplayY = 0;

static int s_actualCursorX = 0;
static int s_actualCursorY = 0;

static HBITMAP g_hBorderHBitmap = NULL;
static HDC g_hBorderMemDC = NULL;
static int g_borderWidth = 0;
static int g_borderHeight = 0;

#define WM_PICK_COLOR_FINAL (WM_USER + 1)
#define WM_EDIT_CONTEXTMENU_ACTIVATE (WM_USER + 2)
#define WM_SHOW_COPIED_MESSAGE (WM_USER + 3)

static int s_bShowCopiedMessage = 0;
static DWORD s_dwCopiedMessageStartTime = 0;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void DrawMagnifier(HDC hDC, int gridCenterX, int gridCenterY, int actualCursorX, int actualCursorY);
void DrawNineSliceBorder(HDC hDC, int width, int height); 
HBITMAP LoadImageIntoHBitmap(const wchar_t* filePath, int* width, int* height);
void PickColorAndPrint(int x, int y);
HWND GetControlUnderCursor(void);

void UpdateLastPickedColorText(HWND hwnd) {
    char bufferLast[256];

    sprintf(bufferLast,
            "%3d %3d %3d | %02X%02X%02X",
            g_LastPickedColor.r, g_LastPickedColor.g, g_LastPickedColor.b,
            g_LastPickedColor.r, g_LastPickedColor.g, g_LastPickedColor.b);

    if (g_hEditCurrent) {
        SetWindowText(g_hEditCurrent, bufferLast);
    }
}

void InvalidateGridOnly(HWND hwnd) {
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);

    RECT gridRect;
    int maxGridHeight;
    int alignedGridHeight;
    int activeCellSize = s_magnificationLevel;

    // Grid now uses full height
    maxGridHeight = clientRect.bottom; 

    alignedGridHeight = (maxGridHeight / activeCellSize) * activeCellSize;
    if (alignedGridHeight < 0) alignedGridHeight = 0;

    gridRect.left = 0;
    gridRect.top = 0;
    gridRect.right = clientRect.right;
    gridRect.bottom = alignedGridHeight;

    InvalidateRect(hwnd, &gridRect, FALSE);
}

void CleanupFrozenGDI(void) {
    if (g_hFrozenDC) {
        if (g_hOldFrozenBitmap) {
            SelectObject(g_hFrozenDC, g_hOldFrozenBitmap);
        }
        DeleteDC(g_hFrozenDC);
        g_hFrozenDC = NULL;
    }
    if (g_hFrozenBitmap) {
        DeleteObject(g_hFrozenBitmap);
        g_hFrozenBitmap = NULL;
    }
    g_frozenCaptureWidth = 0;
    g_frozenCaptureHeight = 0;
}

int IsCursorInWindowClient(void) {
  POINT pt;
  RECT rc;
  GetCursorPos(&pt);
  if (!ScreenToClient(g_hWnd, &pt)) {
    return 0;
  }
  GetClientRect(g_hWnd, &rc);
  return PtInRect(&rc, pt);
}

HWND GetControlUnderCursor(void) {
  POINT pt;
  HWND hWndControl = NULL;

  GetCursorPos(&pt);

  hWndControl = WindowFromPoint(pt);

  if (hWndControl == g_hEditCurrent) {
    return hWndControl;
  }

  return NULL;
}

void PickColorAndPrint(int x, int y) {
  HDC hScreenDC;
  COLORREF color;
  int r, g, b;

  hScreenDC = GetDC(NULL);
  if (hScreenDC == NULL) {
    return;
  }

  color = GetPixel(hScreenDC, x, y);
  ReleaseDC(NULL, hScreenDC);

  if (color == CLR_INVALID) {
    return;
  }

  r = GetRValue(color);
  g = GetGValue(color);
  b = GetBValue(color);

  g_LastPickedColor.r = r;
  g_LastPickedColor.g = g;
  g_LastPickedColor.b = b;
  g_LastPickedColor.a = 255;

  s_lastDisplayX = x;
  s_lastDisplayY = y;

  printf("\n\n--- Final Pick ---\n");
  printf("Mode: %s\n", s_bFreezeActive ? "FROZEN DISPLAY" : "LIVE");
  printf("Location: (%d, %d)\n", x, y);
  printf("RGB: R=%3d, G=%3d, B=%3d\n", r, g, b);
  printf("Hex: #%02X%02X%02X\n", r, g, b);
  printf("------------------\n");

  UpdateLastPickedColorText(g_hWnd);
  InvalidateGridOnly(g_hWnd);
}

HBITMAP LoadImageIntoHBitmap(const wchar_t* filePath, int* width, int* height) {
    char ansiPath[260];
    wcstombs(ansiPath, filePath, sizeof(ansiPath));

    int channels;
    unsigned char* data = stbi_load(ansiPath, width, height, &channels, 4); 

    if (!data) {
        fprintf(stderr, "stb_image failed to load image: %s\n", stbi_failure_reason());
        return NULL;
    }
    
    for (int y = 0; y < *height; y++) {
        for (int x = 0; x < *width; x++) {
            unsigned char* p = data + (y * *width + x) * 4;
            unsigned char a = p[3];
            
            p[0] = (unsigned char)(((unsigned int)p[0] * a) / 255); 
            p[1] = (unsigned char)(((unsigned int)p[1] * a) / 255); 
            p[2] = (unsigned char)(((unsigned int)p[2] * a) / 255); 
        }
    }
    
    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = *width;
    bmi.bmiHeader.biHeight = -(*height); 
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32; 
    bmi.bmiHeader.biCompression = BI_RGB; 

    HDC hScreenDC = GetDC(NULL);
    VOID *pvBits;
    HBITMAP hBitmap = CreateDIBSection(hScreenDC, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    ReleaseDC(NULL, hScreenDC);
    
    if (hBitmap) {
        unsigned char* dest = (unsigned char*)pvBits;
        for (int i = 0; i < (*width) * (*height); ++i) {
            dest[i * 4 + 0] = data[i * 4 + 2]; 
            dest[i * 4 + 1] = data[i * 4 + 1]; 
            dest[i * 4 + 2] = data[i * 4 + 0]; 
            dest[i * 4 + 3] = data[i * 4 + 3]; 
        }
    } else {
        fprintf(stderr, "CreateDIBSection failed.\n");
    }

    stbi_image_free(data);
    return hBitmap;
}

void DrawNineSliceBorder(HDC hDC, int width, int height) {
    if (!g_hBorderHBitmap || !g_hBorderMemDC) return;

    const int CORNER_SIZE = BORDER_SLICE_SIZE; 
    
    if (width < CORNER_SIZE * 2 || height < CORNER_SIZE * 2) {
        return;
    }

    int middleWidth = width - 2 * CORNER_SIZE;
    int middleHeight = height - 2 * CORNER_SIZE;
    
    int srcMidW = g_borderWidth - 2 * CORNER_SIZE;
    int srcMidH = g_borderHeight - 2 * CORNER_SIZE;
    
    BLENDFUNCTION blendFunc;
    blendFunc.BlendOp = AC_SRC_OVER;
    blendFunc.BlendFlags = 0;
    blendFunc.SourceConstantAlpha = 255; 
    blendFunc.AlphaFormat = AC_SRC_ALPHA; 

    HBITMAP hOldBitmap = SelectObject(g_hBorderMemDC, g_hBorderHBitmap);

    AlphaBlend(hDC, 0, 0, CORNER_SIZE, CORNER_SIZE,
               g_hBorderMemDC, 0, 0, CORNER_SIZE, CORNER_SIZE,
               blendFunc);

    AlphaBlend(hDC, CORNER_SIZE, 0, middleWidth, CORNER_SIZE,
               g_hBorderMemDC, CORNER_SIZE, 0, srcMidW, CORNER_SIZE, 
               blendFunc);

    AlphaBlend(hDC, width - CORNER_SIZE, 0, CORNER_SIZE, CORNER_SIZE,
               g_hBorderMemDC, g_borderWidth - CORNER_SIZE, 0, CORNER_SIZE, CORNER_SIZE, 
               blendFunc);

    AlphaBlend(hDC, 0, CORNER_SIZE, CORNER_SIZE, middleHeight,
               g_hBorderMemDC, 0, CORNER_SIZE, CORNER_SIZE, srcMidH, 
               blendFunc);
    
    AlphaBlend(hDC, width - CORNER_SIZE, CORNER_SIZE, CORNER_SIZE, middleHeight,
               g_hBorderMemDC, g_borderWidth - CORNER_SIZE, CORNER_SIZE, CORNER_SIZE, srcMidH, 
               blendFunc);

    AlphaBlend(hDC, 0, height - CORNER_SIZE, CORNER_SIZE, CORNER_SIZE,
               g_hBorderMemDC, 0, g_borderHeight - CORNER_SIZE, CORNER_SIZE, CORNER_SIZE, 
               blendFunc);

    AlphaBlend(hDC, CORNER_SIZE, height - CORNER_SIZE, middleWidth, CORNER_SIZE,
               g_hBorderMemDC, CORNER_SIZE, g_borderHeight - CORNER_SIZE, srcMidW, CORNER_SIZE, 
               blendFunc);

    AlphaBlend(hDC, width - CORNER_SIZE, height - CORNER_SIZE, CORNER_SIZE, CORNER_SIZE,
               g_hBorderMemDC, g_borderWidth - CORNER_SIZE, g_borderHeight - CORNER_SIZE, CORNER_SIZE, CORNER_SIZE, 
               blendFunc);
    
    SelectObject(g_hBorderMemDC, hOldBitmap);
}

void DrawMagnifier(HDC hDC, int gridCenterX, int gridCenterY, int actualCursorX, int actualCursorY) {
  HDC hScreenDC;
  HDC hBufferDC;
  HBITMAP hBufferBitmap;
  HBITMAP hOldBitmap;
  RECT clientRect;
  int windowWidth, windowHeight;
  int magFactor = s_magnificationLevel;
  int visibleGridSizeX, visibleGridSizeY;
  char bufferCurrent[128];
  int displayCenterX;
  int displayCenterY;
  int gridDrawableHeight;
  int gridDrawableWidth;
  int gridWidth;
  int gridHeight;
  int captureWidth;
  int captureHeight;
  int startPixelX;
  int startPixelY;
  COLORREF centerColor;
  HPEN hOldPen;
  HBRUSH hOldBrush;
  HPEN hPenRed;
  int center_x_start;
  int center_y_start;
  int center_x_end;
  int center_y_end;
  RECT textRect;

  GetClientRect(g_hWnd, &clientRect);
  windowWidth = clientRect.right;
  windowHeight = clientRect.bottom;

  // Grid uses full client height
  gridDrawableHeight = windowHeight; 
  gridDrawableWidth = windowWidth;

  visibleGridSizeY = gridDrawableHeight / magFactor;
  visibleGridSizeX = gridDrawableWidth / magFactor;

  if (visibleGridSizeX <= 0 || visibleGridSizeY <= 0) {
    return;
  }

  gridWidth = visibleGridSizeX * magFactor;
  gridHeight = visibleGridSizeY * magFactor;

  captureWidth = visibleGridSizeX;
  captureHeight = visibleGridSizeY;

  displayCenterX = s_bFreezeActive ? s_frozenDisplayX : gridCenterX;
  displayCenterY = s_bFreezeActive ? s_frozenDisplayY : gridCenterY;

  startPixelX = displayCenterX - (visibleGridSizeX / 2);
  startPixelY = displayCenterY - (visibleGridSizeY / 2);

  hScreenDC = GetDC(NULL);
  if (hScreenDC == NULL) {
    return;
  }

  hBufferDC = CreateCompatibleDC(hDC);
  hBufferBitmap = CreateCompatibleBitmap(hDC, gridWidth, gridHeight);
  hOldBitmap = SelectObject(hBufferDC, hBufferBitmap);

  if (s_bFreezeActive && g_hFrozenDC) {
    int offsetX;
    int offsetY;

    offsetX = startPixelX - g_frozenScreenX;
    offsetY = startPixelY - g_frozenScreenY;

    SetStretchBltMode(hBufferDC, COLORONCOLOR);
    StretchBlt(hBufferDC, 0, 0, gridWidth, gridHeight,
               g_hFrozenDC, offsetX, offsetY,
               captureWidth, captureHeight, SRCCOPY);

  } else {
    HDC hMemDC;
    HBITMAP hCaptureBitmap;

    hMemDC = CreateCompatibleDC(hScreenDC);
    hCaptureBitmap = CreateCompatibleBitmap(hScreenDC, captureWidth, captureHeight);
    SelectObject(hMemDC, hCaptureBitmap);

    BitBlt(hMemDC, 0, 0, captureWidth, captureHeight, hScreenDC, startPixelX,
           startPixelY, SRCCOPY);

    if (s_bFreezeActive && g_hFrozenDC == NULL) {

        CleanupFrozenGDI();

        g_hFrozenDC = CreateCompatibleDC(hScreenDC);
        g_hFrozenBitmap = CreateCompatibleBitmap(hScreenDC, captureWidth, captureHeight);
        g_hOldFrozenBitmap = SelectObject(g_hFrozenDC, g_hFrozenBitmap);

        BitBlt(g_hFrozenDC, 0, 0, captureWidth, captureHeight, hMemDC, 0, 0, SRCCOPY);

        g_frozenCaptureWidth = captureWidth;
        g_frozenCaptureHeight = captureHeight;
        g_frozenScreenX = startPixelX;
        g_frozenScreenY = startPixelY;
    }

    SetStretchBltMode(hBufferDC, COLORONCOLOR);
    StretchBlt(hBufferDC, 0, 0, gridWidth, gridHeight, hMemDC, 0, 0,
               captureWidth, captureHeight, SRCCOPY);

    DeleteObject(hCaptureBitmap);
    DeleteDC(hMemDC);
  }

  int colorSampleX, colorSampleY;
  
  if (s_bFreezeActive) {
      colorSampleX = actualCursorX;
      colorSampleY = actualCursorY;
  } else {
      colorSampleX = displayCenterX;
      colorSampleY = displayCenterY;
  }

  centerColor = GetPixel(hScreenDC, colorSampleX, colorSampleY);

  g_CurrentColor.r = GetRValue(centerColor);
  g_CurrentColor.g = GetGValue(centerColor);
  g_CurrentColor.b = GetBValue(centerColor);
  g_CurrentColor.a = 255;

  if (s_bControlsActive) {
      g_CurrentColor.r = 0;
      g_CurrentColor.g = 0;
      g_CurrentColor.b = 0;
  }

  hPenRed = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
  hOldPen = SelectObject(hBufferDC, hPenRed);
  hOldBrush = SelectObject(hBufferDC, GetStockObject(HOLLOW_BRUSH));

  center_x_start = (visibleGridSizeX / 2) * magFactor;
  center_y_start = (visibleGridSizeY / 2) * magFactor;
  center_x_end = center_x_start + magFactor;
  center_y_end = center_y_start + magFactor;

  Rectangle(hBufferDC, center_x_start, center_y_start, center_x_end,
            center_y_end);

  SelectObject(hBufferDC, hOldBrush);
  SelectObject(hBufferDC, hOldPen);
  DeleteObject(hPenRed);

  BitBlt(hDC, 0, 0, gridWidth, gridHeight, hBufferDC, 0, 0, SRCCOPY);

  SelectObject(hBufferDC, hOldBitmap);
  DeleteObject(hBufferBitmap);
  DeleteDC(hBufferDC);


  SetBkMode(hDC, TRANSPARENT);
  SetTextColor(hDC, RGB(255, 255, 255));
  SelectObject(hDC, g_hEditFont);

  sprintf(bufferCurrent,
          "%3d %3d %3d | %02X%02X%02X | 1:%d %s",
          g_CurrentColor.r, g_CurrentColor.g, g_CurrentColor.b,
          g_CurrentColor.r, g_CurrentColor.g, g_CurrentColor.b, magFactor,
          s_bFreezeActive ? " [F]" : "");

  textRect.left = INNER_PADDING;
  textRect.top = INNER_PADDING;
  textRect.right = windowWidth - INNER_PADDING;
  textRect.bottom = windowHeight; // Use full height to draw text at top

  DrawText(hDC, bufferCurrent, -1, &textRect, DT_CENTER | DT_TOP | DT_SINGLELINE);
  sprintf(bufferCurrent, "R - move slower");

  textRect.left = INNER_PADDING;
  textRect.top = INNER_PADDING + 12;
  textRect.right = windowWidth - INNER_PADDING;
  textRect.bottom = windowHeight; // Use full height to draw text at top

  DrawText(hDC, bufferCurrent, -1, &textRect, DT_CENTER | DT_TOP | DT_SINGLELINE);
  sprintf(bufferCurrent, "F - freeze image");

  textRect.left = INNER_PADDING;
  textRect.top = INNER_PADDING + 24;
  textRect.right = windowWidth - INNER_PADDING;
  textRect.bottom = windowHeight; // Use full height to draw text at top

  DrawText(hDC, bufferCurrent, -1, &textRect, DT_CENTER | DT_TOP | DT_SINGLELINE);
  sprintf(bufferCurrent, "ESC - exit");

  textRect.left = INNER_PADDING;
  textRect.top = INNER_PADDING + 36;
  textRect.right = windowWidth - INNER_PADDING;
  textRect.bottom = windowHeight; // Use full height to draw text at top

  DrawText(hDC, bufferCurrent, -1, &textRect, DT_CENTER | DT_TOP | DT_SINGLELINE);
	ReleaseDC(NULL, hScreenDC);
}

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode >= 0) {

    if (s_bIgnoreNextClick) {
        if (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN ||
            wParam == WM_LBUTTONUP || wParam == WM_RBUTTONUP) {

            s_bIgnoreNextClick = 0;
            return 1;
        }
    }

    if (g_hWnd) {
      if (wParam == WM_LBUTTONDOWN) {
          if (!s_bControlsActive) {
            if (!g_bSamplingActive) {
              g_bSamplingActive = 1;
            }
          }
      }

      if (wParam == WM_LBUTTONUP) {
          if (!s_bControlsActive) {
            if (g_bSamplingActive) {
              g_bSamplingActive = 0;
              PostMessage(g_hWnd, WM_PICK_COLOR_FINAL, 0, 0);
            }
          }
      }
    }
  }
  return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}

LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_RBUTTONDOWN) {
        
        DWORD dwStart, dwEnd;
        int len;
        
        SendMessage(hwnd, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
        
        if (dwStart != dwEnd) {
            SendMessage(hwnd, WM_COPY, 0, 0); 
            
            SendMessage(hwnd, EM_SETSEL, (WPARAM)dwEnd, (LPARAM)dwEnd);

        } else {
            len = GetWindowTextLength(hwnd);
            if (len > 0) {
                char* buffer = (char*)malloc(len + 1);
                if (buffer) {
                    GetWindowText(hwnd, buffer, len + 1);
                    
                    if (OpenClipboard(GetParent(hwnd))) {
                        EmptyClipboard();
                        
                        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, len + 1);
                        if (hGlobal != NULL) {
                            char* pData = (char*)GlobalLock(hGlobal);
                            if (pData != NULL) {
                                strcpy(pData, buffer);
                                GlobalUnlock(hGlobal);
                                SetClipboardData(CF_TEXT, hGlobal);
                            } else {
                                GlobalFree(hGlobal);
                            }
                        }
                        CloseClipboard();
                    }
                    free(buffer);
                }
            }
        }
        
        PostMessage(GetParent(hwnd), WM_SHOW_COPIED_MESSAGE, 0, 0);

        return 0; 
    }
    
    return CallWindowProc(g_pfnEditOriginalProc, hwnd, uMsg, wParam, lParam);
}

int CheckResizeHit(POINT pt, int width, int height) {
    const int GRIP_SIZE = RESIZE_GRIP_SIZE;
    int left;
    int right;
    int top;
    int bottom;

    left = pt.x < GRIP_SIZE;
    right = pt.x >= (width - GRIP_SIZE);
    top = pt.y < GRIP_SIZE;
    bottom = pt.y >= (height - GRIP_SIZE);

    if (left && top) return HTTOPLEFT;
    if (right && top) return HTTOPRIGHT;
    if (left && bottom) return HTBOTTOMLEFT;
    if (right && bottom) return HTBOTTOMRIGHT;
    if (left) return HTLEFT;
    if (right) return HTRIGHT;
    if (top) return HTTOP;
    if (bottom) return HTBOTTOM;

    return HTCLIENT;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                            LPARAM lParam) {

  TRACKMOUSEEVENT tme;
  POINT cursor;
  POINT pt;
  RECT rc;
  LRESULT hit;
  int oldMagLevel;
  int innerGridWidth;
  int innerGridHeight;
  int contentWidth;
  int textControlX;
  static int s_textControlFixedW = 0; // Static to hold the calculated fixed width
  int textControlWidth;
  int initialTextStart;
  HDC hEditDC;
  HWND hEditControl;
  int newWidth;
  int newHeight;
  HRGN hRgn;
  int activeCellSize;
  int maxGridHeight;
  int alignedGridHeight;
  int textControlY;
  int frameWidth;
  int frameHeight;
  RECT windowRect, clientRect;
  int clientWidth, clientHeight;
  int minGridDimension;
  PAINTSTRUCT ps;
  HDC hDC;
  HDC hMemDC;
  HBITMAP hBitmap;
  HBITMAP hOldBitmap;
  int windowWidth;
  int windowHeight;
  short pKeyState;
  int isPDown;
  short fKeyState;
  int isFDown;
  short shiftKeyState;
  int isShiftDown;
  int bShouldInvalidate;
  int deltaX;
  int deltaY;
  int actualMoveX;
  int actualMoveY;

  switch (uMsg) {
  case WM_CREATE:
    innerGridWidth = GRID_SIZE * FIXED_CELL_SIZE;
    innerGridHeight = GRID_SIZE * FIXED_CELL_SIZE;

    contentWidth = innerGridWidth;

    textControlX = INNER_PADDING;

    g_hWnd = hwnd;

    printf("--- Win32 Magnifying Color Picker ---\n");
    printf("ACTION: Press and hold the LEFT mouse button anywhere on the "
           "screen.\n");
    printf("RELEASE: Lift the LEFT mouse button to get the final color.\n");
    printf("TOGGLE FREEZE (F): Freeze the magnified image.\n");
    printf("TOGGLE PRECISION (P): Deprecated. Use [+] and [-] for zoom.\n");
    printf("HOLD SHIFT for slow movement.\n");
    printf("Press [+] to zoom in (lower cell size) / [-] to zoom out (higher "
           "cell size).\n");
    printf("Nine-slice border (stb_image/AlphaBlend) is active with slice size: %dpx.\n", BORDER_SLICE_SIZE);


    GetCursorPos(&cursor);
    s_currentCursorX = cursor.x;
    s_currentCursorY = cursor.y;
    s_lastTrackedCursorX = cursor.x;
    s_lastTrackedCursorY = cursor.y;
    s_accumulatedDeltaX = 0.0f;
    s_accumulatedDeltaY = 0.0f;
    s_bShiftWasDown = 0;
    s_bFreezeActive = 0;
    s_bFKeyWasDown = 0;
    s_frozenDisplayX = 0;
    s_frozenDisplayY = 0;
    s_lastDisplayX = 0;
    s_lastDisplayY = 0;
    s_bIgnoreNextClick = 0;
    s_actualCursorX = cursor.x;
    s_actualCursorY = cursor.y;


    SetTimer(hwnd, TIMER_ID, TIMER_INTERVAL, NULL);

    tme.cbSize = sizeof(TRACKMOUSEEVENT);
    tme.dwFlags = TME_LEAVE;
    tme.hwndTrack = hwnd;
    TrackMouseEvent(&tme);

    g_hEditFont =
        CreateFont(-12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                   OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                   DEFAULT_PITCH | FF_SWISS, "Arial");

    // Calculate fixed width for the text control based on initial text
    {
        HDC tempDC = GetDC(hwnd);
        HFONT hOldFont = SelectObject(tempDC, g_hEditFont);
        SIZE size;
        const char* initialText = "click anywhere to sample";
        GetTextExtentPoint32(tempDC, initialText, (int)strlen(initialText), &size);
        s_textControlFixedW = size.cx + 2 * MARGIN; // Add margin/padding for visual space
        SelectObject(tempDC, hOldFont);
        ReleaseDC(hwnd, tempDC);
        
        // Use the calculated fixed width
        textControlWidth = s_textControlFixedW;
        
        // Center the text box initially (using initial innerGridWidth)
        textControlX = (innerGridWidth - textControlWidth) / 2;
    }

    // Calculate Y position from bottom edge (innerGridHeight is initial client height)
    initialTextStart = innerGridHeight - TEXT_AREA_HEIGHT - INNER_PADDING + MARGIN; 

    g_hEditCurrent =
        CreateWindowEx(0, "EDIT", "click anywhere to sample",
                       WS_CHILD | WS_VISIBLE | ES_READONLY | ES_CENTER,
                       textControlX, initialTextStart,
                       textControlWidth, TEXT_AREA_HEIGHT - 2 * MARGIN,
                       hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

    if (g_hEditCurrent) {
      SendMessage(g_hEditCurrent, WM_SETFONT, (WPARAM)g_hEditFont, 0);
      
      g_pfnEditOriginalProc = (WNDPROC)SetWindowLongPtr(g_hEditCurrent, 
                                                        GWLP_WNDPROC, 
                                                        (LONG_PTR)EditSubclassProc);
    }

    return 0;
case WM_CTLCOLOREDIT:
    hEditDC = (HDC)wParam;
    hEditControl = (HWND)lParam;

    if (hEditControl == g_hEditCurrent) {
      SetTextColor(hEditDC, RGB(255, 255, 255));
      SetBkColor(hEditDC, RGB(0, 0, 0));
      return (LRESULT)GetStockObject(BLACK_BRUSH);
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
 case WM_CTLCOLORSTATIC:
    hEditDC = (HDC)wParam;
    hEditControl = (HWND)lParam;

    if (hEditControl == g_hEditCurrent) {
      SetTextColor(hEditDC, RGB(255, 255, 255));
      SetBkColor(hEditDC, RGB(0, 0, 0));
      return (LRESULT)GetStockObject(BLACK_BRUSH);
    }
    break;
case WM_SIZE:
    newWidth = LOWORD(lParam);
    newHeight = HIWORD(lParam);

    hRgn = CreateRoundRectRgn(0, 0, newWidth + 1, newHeight + 1,
                                   CORNER_RADIUS, CORNER_RADIUS);
    SetWindowRgn(hwnd, hRgn, TRUE);

    activeCellSize = s_magnificationLevel;

    // Grid uses full height, so maxGridHeight is just newHeight
    maxGridHeight = newHeight;

    alignedGridHeight = (maxGridHeight / activeCellSize) * activeCellSize;
    if (alignedGridHeight < 0) alignedGridHeight = 0;

    // Calculate Y position from bottom edge
    textControlY = newHeight - TEXT_AREA_HEIGHT - INNER_PADDING + MARGIN; 

    contentWidth = newWidth;
    
    // Position the fixed-width text control in the center of the new width
    textControlWidth = s_textControlFixedW; 
    textControlX = (contentWidth - textControlWidth) / 2;


    if (g_hEditCurrent) {
      SetWindowPos(g_hEditCurrent, NULL, textControlX, textControlY,
                   textControlWidth, TEXT_AREA_HEIGHT - 2 * MARGIN,
                   SWP_NOZORDER);
    }
    break;

  case WM_ERASEBKGND:
    return 1;

  case WM_SETCURSOR:
    break;

  case WM_LBUTTONDOWN:
    if (GetControlUnderCursor() == NULL) {
        SetFocus(hwnd);
        
        if (s_bFreezeActive) {
            s_bIgnoreNextClick = 1;
            
            PostMessage(hwnd, WM_PICK_COLOR_FINAL, 0, 0);
        }
    }
    return 0;

  case WM_COMMAND:
    if ((HWND)lParam == g_hEditCurrent) {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    break;

  case WM_CONTEXTMENU:
    break;

  case WM_SHOW_COPIED_MESSAGE:
    s_bShowCopiedMessage = 1;
    s_dwCopiedMessageStartTime = GetTickCount();
    InvalidateRect(hwnd, NULL, FALSE);
    return 0;

  case WM_KEYDOWN:
    if (wParam == VK_ESCAPE) {
      DestroyWindow(hwnd);
      return 0;
    }

    oldMagLevel = s_magnificationLevel;

    if (wParam == VK_OEM_MINUS || wParam == VK_SUBTRACT) {
      if (s_magnificationLevel > MIN_MAGNIFICATION_LEVEL) {
        s_magnificationLevel -= MAG_STEP;
      }
    } else if (wParam == VK_OEM_PLUS || wParam == VK_ADD) {
      if (s_magnificationLevel < MAX_MAGNIFICATION_LEVEL) {
        s_magnificationLevel += MAG_STEP;
      }
    }

    if (s_magnificationLevel != oldMagLevel) {
      printf("Magnification Level changed to: 1:%d\n", s_magnificationLevel);
      InvalidateGridOnly(hwnd);
    }

    if (wParam == 0x46) {
        return 0;
    } else {
        return 0;
    }
    break;

  case WM_SIZING:
    {
        RECT* pRect = (RECT*)lParam;
        RECT client_rect;

        GetClientRect(hwnd, &client_rect);

        int target_width, target_height;

        GetWindowRect(hwnd, &windowRect);

        frameWidth = (windowRect.right - windowRect.left) - client_rect.right;
        frameHeight = (windowRect.bottom - windowRect.top) - client_rect.bottom;

        clientWidth = pRect->right - pRect->left - frameWidth;
        clientHeight = pRect->bottom - pRect->top - frameHeight;

        activeCellSize = s_magnificationLevel;
        minGridDimension = 5 * activeCellSize;

        // The minimum visible height needs to include the minimum grid size 
        // AND the space required for the overlay (TEXT_AREA_HEIGHT + 2*INNER_PADDING)
        int minTotalHeight = minGridDimension + TEXT_AREA_HEIGHT + 2 * INNER_PADDING;
        
        innerGridHeight = clientHeight;

        if (innerGridHeight < minTotalHeight) {
            innerGridHeight = minTotalHeight;
        }

        // The client width must be aligned
        target_width = (clientWidth / activeCellSize) * activeCellSize;
        // The total window height is aligned, not just the grid part
        target_height = (innerGridHeight / activeCellSize) * activeCellSize; 

        if (target_width < minGridDimension) {
            target_width = minGridDimension;
        }

        clientWidth = target_width;
        clientHeight = target_height;


        if (wParam == WMSZ_LEFT || wParam == WMSZ_BOTTOMLEFT ||
            wParam == WMSZ_TOPLEFT) {
          pRect->left = pRect->right - (clientWidth + frameWidth);
        } else if (wParam == WMSZ_RIGHT || wParam == WMSZ_BOTTOMRIGHT ||
                   wParam == WMSZ_TOPRIGHT) {
          pRect->right = pRect->left + (clientWidth + frameWidth);
        }

        if (wParam == WMSZ_TOP || wParam == WMSZ_TOPLEFT ||
            wParam == WMSZ_TOPRIGHT) {
          pRect->top = pRect->bottom - (clientHeight + frameHeight);
        } else if (wParam == WMSZ_BOTTOM || wParam == WMSZ_BOTTOMLEFT ||
                   wParam == WMSZ_BOTTOMRIGHT) {
          pRect->bottom = pRect->top + (clientHeight + frameHeight);
        }
    }
    return TRUE;

  case WM_MOUSEMOVE:
    if (!g_bMagnifyDisplayActive) {
      tme.cbSize = sizeof(TRACKMOUSEEVENT);
      tme.dwFlags = TME_LEAVE;
      tme.hwndTrack = hwnd;
      TrackMouseEvent(&tme);
      g_bMagnifyDisplayActive = 1;
    }

    if (!s_bControlsActive && g_bMagnifyDisplayActive) {
        InvalidateGridOnly(hwnd);
    }
    return 0;

  case WM_MOUSELEAVE:
    g_bMagnifyDisplayActive = 0;
    return 0;

  case WM_TIMER:
    if (wParam == TIMER_ID) {

      int bControlsActiveOld = s_bControlsActive;

      pKeyState = GetAsyncKeyState(0x50);
      isPDown = (pKeyState & 0x8000) != 0;

      fKeyState = GetAsyncKeyState(0x46);
      isFDown = (fKeyState & 0x8000) != 0;

      shiftKeyState = GetAsyncKeyState(VK_SHIFT);
      isShiftDown = (shiftKeyState & 0x8000) != 0;

      bShouldInvalidate = 0;

      if (GetControlUnderCursor() != NULL) {
        s_bControlsActive = 1;
      } else {
        s_bControlsActive = 0;
      }

      if (s_bControlsActive != bControlsActiveOld) {
          bShouldInvalidate = 1;
      }

      if (isPDown && !s_bKeyWasDown) {
        g_bPrecisionModeActive = !g_bPrecisionModeActive;
        printf("TOGGLE PRECISION (P): Deprecated. Use [+] and [-] for zoom.\n");

        bShouldInvalidate = 1;

        GetCursorPos(&cursor);
        s_currentCursorX = cursor.x;
        s_currentCursorY = cursor.y;
        s_lastTrackedCursorX = cursor.x;
        s_lastTrackedCursorY = cursor.y;
        s_accumulatedDeltaX = 0.0f;
        s_accumulatedDeltaY = 0.0f;
      }
      s_bKeyWasDown = isPDown;

      if (isFDown && !s_bFKeyWasDown) {
          s_bFreezeActive = !s_bFreezeActive;
          printf("Magnifier %s.\n", s_bFreezeActive ? "FROZEN DISPLAY" : "UNFROZEN DISPLAY");

          if (s_bFreezeActive) {
             s_frozenDisplayX = s_currentCursorX;
             s_frozenDisplayY = s_currentCursorY;
          } else {
              CleanupFrozenGDI();
          }
          bShouldInvalidate = 1;
      }
      s_bFKeyWasDown = isFDown;

      if (GetCursorPos(&cursor)) {
        
        s_actualCursorX = cursor.x;
        s_actualCursorY = cursor.y;

        deltaX = cursor.x - s_lastTrackedCursorX;
        deltaY = cursor.y - s_lastTrackedCursorY;

        if ((deltaX != 0 || deltaY != 0) && !s_bControlsActive) {
           bShouldInvalidate = 1;
        }

        if (s_bControlsActive) {
            s_accumulatedDeltaX = 0.0f;
            s_accumulatedDeltaY = 0.0f;
            s_currentCursorX = cursor.x;
            s_currentCursorY = cursor.y;
        }
        else if (s_bFreezeActive) {
            s_currentCursorX = cursor.x;
            s_currentCursorY = cursor.y;
            s_accumulatedDeltaX = 0.0f;
            s_accumulatedDeltaY = 0.0f;
        } else {
            s_bShiftWasDown = isShiftDown;

            if (isShiftDown) {
                s_accumulatedDeltaX += (float)deltaX / SLOW_FACTOR;
                s_accumulatedDeltaY += (float)deltaY / SLOW_FACTOR;

                actualMoveX = (int)s_accumulatedDeltaX;
                actualMoveY = (int)s_accumulatedDeltaY;

                s_currentCursorX += actualMoveX;
                s_currentCursorY += actualMoveY;

                s_accumulatedDeltaX -= actualMoveX;
                s_accumulatedDeltaY -= actualMoveY;

            } else {
                s_currentCursorX = cursor.x;
                s_currentCursorY = cursor.y;
                s_accumulatedDeltaX = 0.0f;
                s_accumulatedDeltaY = 0.0f;
            }
        }

        s_lastTrackedCursorX = cursor.x;
        s_lastTrackedCursorY = cursor.y;

        if (bShouldInvalidate) {
           InvalidateGridOnly(hwnd);
        }
      }
    }
    return 0;

  case WM_PAINT:
    hDC = BeginPaint(hwnd, &ps);

    GetClientRect(hwnd, &clientRect);
    windowWidth = clientRect.right;
    windowHeight = clientRect.bottom;

    hMemDC = CreateCompatibleDC(hDC);
    hBitmap = CreateCompatibleBitmap(hDC, windowWidth, windowHeight);
    hOldBitmap = SelectObject(hMemDC, hBitmap);

    FillRect(hMemDC, &clientRect, (HBRUSH)GetStockObject(BLACK_BRUSH));

    // Draw grid over full client area
    DrawMagnifier(hMemDC, s_currentCursorX, s_currentCursorY, s_actualCursorX, s_actualCursorY);
    
    // Draw the Nine-Slice Border on top of the grid
    DrawNineSliceBorder(hMemDC, windowWidth, windowHeight);

    if (s_bShowCopiedMessage) {
        RECT copiedRect;
        GetClientRect(hwnd, &copiedRect);
        
        // This centering logic must use the fixed width in mind if it's based on it, 
        // but here it's just relative to window size.
        copiedRect.left = (copiedRect.right - copiedRect.left)/2 - 20;
        copiedRect.top = copiedRect.bottom - 80;
        copiedRect.bottom = copiedRect.bottom - 60; 

        SetBkMode(hMemDC, TRANSPARENT);
        SetTextColor(hMemDC, RGB(0, 255, 0));
        SelectObject(hMemDC, g_hEditFont);
        
        DrawText(hMemDC, "COPIED", -1, &copiedRect, 0);
        
        if (GetTickCount() - s_dwCopiedMessageStartTime > 1500) {
            s_bShowCopiedMessage = 0;
            InvalidateRect(hwnd, &copiedRect, FALSE); 
        }
    }

    BitBlt(hDC, 0, 0, windowWidth, windowHeight, hMemDC, 0, 0, SRCCOPY);

    SelectObject(hMemDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemDC);

    EndPaint(hwnd, &ps);
    return 0;

  case WM_PICK_COLOR_FINAL:
    if (!s_bControlsActive) {
      
      int pickX, pickY;
      
      if (s_bFreezeActive) {
          pickX = s_actualCursorX;
          pickY = s_actualCursorY;
      } else {
          pickX = s_currentCursorX;
          pickY = s_currentCursorY;
      }
      
      PickColorAndPrint(pickX, pickY);
    }
    return 0;

  case WM_CLOSE:
    KillTimer(hwnd, TIMER_ID);
    DestroyWindow(hwnd);
    return 0;

  case WM_DESTROY:
    if (g_hMouseHook) {
      UnhookWindowsHookEx(g_hMouseHook);
    }

    if (g_hEditFont) {
      DeleteObject(g_hEditFont);
    }
    
    if (g_hEditCurrent && g_pfnEditOriginalProc) {
        SetWindowLongPtr(g_hEditCurrent, GWLP_WNDPROC, (LONG_PTR)g_pfnEditOriginalProc);
    }

    if (g_hBorderHBitmap) {
        DeleteObject(g_hBorderHBitmap);
        g_hBorderHBitmap = NULL;
    }
    if (g_hBorderMemDC) {
        DeleteDC(g_hBorderMemDC);
        g_hBorderMemDC = NULL;
    }

    CleanupFrozenGDI();

    PostQuitMessage(0);
    return 0;

  case WM_NCHITTEST:
    GetCursorPos(&pt);
    ScreenToClient(hwnd, &pt);
    GetClientRect(hwnd, &rc);

    hit = CheckResizeHit(pt, rc.right, rc.bottom);
    if (hit != HTCLIENT) return hit;

    if (g_hEditCurrent) {
        RECT editRect;
        GetWindowRect(g_hEditCurrent, &editRect);
        ScreenToClient(hwnd, (POINT*)&editRect.left);
        ScreenToClient(hwnd, (POINT*)&editRect.right);

        if (PtInRect(&editRect, pt)) {
            return HTCLIENT;
        }
    }

    return HTCAPTION;
  }

  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
  WNDCLASSEX wc;
  HWND hwnd;
  MSG Msg;
  char g_szClassName[] = "ColorPickerClass";
  RECT rect;
  DWORD dwStyle;
  int initialWidth;
  int initialHeight;
  int screenX;
  int screenY;

  g_hInstance = hInstance;

  dwStyle = WS_POPUP | WS_VISIBLE;

  g_hBorderHBitmap = LoadImageIntoHBitmap(BORDER_IMAGE_PATH, &g_borderWidth, &g_borderHeight);
  HWND hConsole = GetConsoleWindow();
    // Hide the console window
    ShowWindow(hConsole, SW_HIDE);
  if (g_hBorderHBitmap) {
      g_hBorderMemDC = CreateCompatibleDC(NULL); 
      printf("Border image loaded successfully via stb_image. Dimensions: %d x %d\n", 
             g_borderWidth, g_borderHeight);
  } else {
      fprintf(stderr, "FATAL: Failed to load border image via stb_image. Border will not be visible.\n");
  }

  memset(&wc, 0, sizeof(WNDCLASSEX));

  wc.cbSize = sizeof(WNDCLASSEX);
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
  wc.lpszClassName = g_szClassName;
  wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);

  if (!RegisterClassEx(&wc)) {
    fprintf(stderr, "Window Registration Failed!\n");
    return 1;
  }

  rect.left = 0;
  rect.top = 0;
  rect.right = GRID_SIZE * FIXED_CELL_SIZE;
  // Initial height is back to the base grid size
  rect.bottom = GRID_SIZE * FIXED_CELL_SIZE; 

  initialWidth = rect.right;
  initialHeight = rect.bottom;

  screenX = (GetSystemMetrics(SM_CXSCREEN) - initialWidth) / 2;
  screenY = (GetSystemMetrics(SM_CYSCREEN) - initialHeight) / 2;

  hwnd = CreateWindowEx(
      WS_EX_TOPMOST,
      g_szClassName, "Eyedropper", dwStyle,
      screenX, screenY,
      initialWidth, initialHeight,
      NULL, NULL, hInstance,
      NULL);

  if (hwnd == NULL) {
    fprintf(stderr, "Window Creation Failed!\n");
    return 1;
  }

  UpdateWindow(hwnd);
  g_hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, g_hInstance, 0);

  if (g_hMouseHook == NULL) {
    fprintf(stderr, "Failed to install global mouse hook!\n");
  }

  while (GetMessage(&Msg, NULL, 0, 0) > 0) {
    TranslateMessage(&Msg);
    DispatchMessage(&Msg);
  }

  return (int)Msg.wParam;
}
