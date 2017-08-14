#include <windows.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <float.h>

void debugPrint(char *format, ...) {
  va_list argptr;
  va_start(argptr, format);
  char str[1024];
  vsprintf_s(str, sizeof(str), format, argptr);
  va_end(argptr);
  OutputDebugString(str);
}

#define BACKBUFFER_WIDTH 512
#define BACKBUFFER_HEIGHT BACKBUFFER_WIDTH
#define BACKBUFFER_BYTES (BACKBUFFER_WIDTH * BACKBUFFER_HEIGHT * sizeof(uint32_t))
#define WINDOW_SCALE 1
#define WINDOW_WIDTH (BACKBUFFER_WIDTH * WINDOW_SCALE)
#define WINDOW_HEIGHT (BACKBUFFER_HEIGHT * WINDOW_SCALE)

uint32_t *backbuffer;

void setPixel(int x, int y, uint32_t color) {
  backbuffer[y*BACKBUFFER_WIDTH + x] = color;
}

void drawFilledRect(int left, int top, int right, int bottom, uint32_t color) {
  for (int y = top; y <= bottom; ++y) {
    for (int x = left; x <= right; ++x) {
      setPixel(x, y, color);
    }
  }
}

typedef enum {BUTTON_EXIT, BUTTON_ACTION, BUTTON_COUNT} Button;

bool buttonIsDown[BUTTON_COUNT];
bool buttonWasDown[BUTTON_COUNT];

bool buttonIsPressed(Button button) {
  return buttonIsDown[button] && !buttonWasDown[button];
}

LRESULT CALLBACK wndProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  switch (msg) {
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    default:
      return DefWindowProc(wnd, msg, wparam, lparam);
  }
  return 0;
}

int CALLBACK WinMain(HINSTANCE inst, HINSTANCE prevInst, LPSTR cmdLine, int cmdShow) {
  UNREFERENCED_PARAMETER(prevInst);
  UNREFERENCED_PARAMETER(cmdLine);

  WNDCLASS wndClass = {0};
  wndClass.style = CS_HREDRAW | CS_VREDRAW;
  wndClass.lpfnWndProc = wndProc;
  wndClass.hInstance = inst;
  wndClass.hCursor = LoadCursor(0, IDC_ARROW);
  wndClass.lpszClassName = "Renderer";
  RegisterClass(&wndClass);

  RECT crect = {0};
  crect.right = WINDOW_WIDTH;
  crect.bottom = WINDOW_HEIGHT;

  DWORD wndStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
  AdjustWindowRect(&crect, wndStyle, 0);

  HWND wnd = CreateWindowEx(0, wndClass.lpszClassName, "Renderer", wndStyle, 300, 100,
                            crect.right - crect.left, crect.bottom - crect.top,
                            0, 0, inst, 0);
  ShowWindow(wnd, cmdShow);
  UpdateWindow(wnd);

  HDC deviceContext = GetDC(wnd);
  BITMAPINFO bitmapInfo;

  {
    backbuffer = malloc(BACKBUFFER_BYTES);

    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth = BACKBUFFER_WIDTH;
    bitmapInfo.bmiHeader.biHeight = BACKBUFFER_HEIGHT;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
  }

  //

  float dt = 0.0f;
  float targetFps = 60.0f;
  float maxDt = 1.0f / targetFps;
  LARGE_INTEGER perfcFreq = {0};
  LARGE_INTEGER perfc = {0};
  LARGE_INTEGER perfcPrev = {0};

  QueryPerformanceFrequency(&perfcFreq);
  QueryPerformanceCounter(&perfc);

  //

  int mousePosX = 0;
  int mousePosY = 0;

  bool gameIsRunning = true;

  while (gameIsRunning) {
    perfcPrev = perfc;
    QueryPerformanceCounter(&perfc);
    dt = (float)(perfc.QuadPart - perfcPrev.QuadPart) / (float)perfcFreq.QuadPart;
    if (dt > maxDt) {
      dt = maxDt;
    }

    for (int button = 0; button < BUTTON_COUNT; ++button) {
      buttonWasDown[button] = buttonIsDown[button];
    }

    MSG msg;
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
      switch (msg.message) {
        case WM_QUIT:
          gameIsRunning = false;
          break;

        case WM_KEYDOWN:
        case WM_KEYUP:
          {
            bool isDown = ((msg.lParam & (1 << 31)) == 0);
            switch (msg.wParam) {
              case VK_ESCAPE:
                buttonIsDown[BUTTON_EXIT] = isDown;
                break;
            }
          }
          break;

        case WM_LBUTTONDOWN:
          buttonIsDown[BUTTON_ACTION] = true;
          break;

        case WM_LBUTTONUP:
          buttonIsDown[BUTTON_ACTION] = false;
          break;

        default:
          TranslateMessage(&msg);
          DispatchMessage(&msg);
          break;
      }
    }

    if (buttonIsDown[BUTTON_EXIT]) {
      gameIsRunning = false;
    }

    {
      POINT p;
      GetCursorPos(&p);
      ScreenToClient(wnd, &p);
      mousePosX = (int) ((float)p.x / (float)WINDOW_SCALE);
      mousePosY = (int) ((float)p.y / (float)WINDOW_SCALE);
      //debugPrint("%d,%d\n", mousePosX, mousePosY);
    }

    drawFilledRect(0, 0, BACKBUFFER_WIDTH-1, BACKBUFFER_HEIGHT-1, 0xFFFF0000);

    StretchDIBits(deviceContext,
                  0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
                  0, 0, BACKBUFFER_WIDTH, BACKBUFFER_HEIGHT,
                  backbuffer, &bitmapInfo,
                  DIB_RGB_COLORS, SRCCOPY);
  }
}
