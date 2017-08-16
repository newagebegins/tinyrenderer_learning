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

//#define BACKBUFFER_WIDTH 900
#define BACKBUFFER_WIDTH 200
#define BACKBUFFER_HEIGHT BACKBUFFER_WIDTH
#define BACKBUFFER_BYTES (BACKBUFFER_WIDTH * BACKBUFFER_HEIGHT * sizeof(uint32_t))
//#define WINDOW_SCALE 1
#define WINDOW_SCALE 4
#define WINDOW_WIDTH (BACKBUFFER_WIDTH * WINDOW_SCALE)
#define WINDOW_HEIGHT (BACKBUFFER_HEIGHT * WINDOW_SCALE)

#define WHITE 0xFFFFFFFF
#define RED   0xFFFF0000
#define GREEN 0xFF00FF00

uint32_t *backbuffer;

void setPixel(int x, int y, uint32_t color) {
  assert(x >= 0 && x < BACKBUFFER_WIDTH);
  assert(y >= 0 && y < BACKBUFFER_HEIGHT);
  backbuffer[y*BACKBUFFER_WIDTH + x] = color;
}

void drawFilledRect(int left, int top, int right, int bottom, uint32_t color) {
  for (int y = top; y <= bottom; ++y) {
    for (int x = left; x <= right; ++x) {
      setPixel(x, y, color);
    }
  }
}

void drawLine(int x1, int y1, int x2, int y2, uint32_t color) {
  if (x1 == x2 && y1 == y2) {
    setPixel(x1, y1, color);
    return;
  }

  int xStart, xEnd, yStart, yEnd;
  int dx = x2 - x1;
  int dy = y2 - y1;

  if (abs(dx) > abs(dy)) {
    float m = (float)dy / (float)dx;
    if (x1 < x2) {
      xStart = x1;
      yStart = y1;
      xEnd = x2;
      yEnd = y2;
    } else {
      xStart = x2;
      yStart = y2;
      xEnd = x1;
      yEnd = y1;
    }
    for (int x = xStart; x <= xEnd; ++x) {
      int y = (int)(m * (x - xStart) + yStart);
      setPixel(x, y, color);
    }
  } else {
    float m = (float)dx / (float)dy;
    if (y1 < y2) {
      xStart = x1;
      yStart = y1;
      xEnd = x2;
      yEnd = y2;
    } else {
      xStart = x2;
      yStart = y2;
      xEnd = x1;
      yEnd = y1;
    }
    for (int y = yStart; y <= yEnd; ++y) {
      int x = (int)(m * (y - yStart) + xStart);
      setPixel(x, y, color);
    }
  }
}

#define SWAP(x,y) {int t=x; x=y; y=t;}

void drawTriangleLineSweep(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color) {
  if (y2 < y1) {SWAP(y2, y1); SWAP(x2, x1);}
  if (y1 < y0) {SWAP(y1, y0); SWAP(x1, x0);}
  if (y2 < y1) {SWAP(y2, y1); SWAP(x2, x1);}
  assert(y0 < y1 && y1 < y2);

#if 0
  drawLine(x0, y0, x1, y1, color);
  drawLine(x1, y1, x2, y2, color);
  drawLine(x2, y2, x0, y0, color);
#endif

  for (int y = y0; y <= y1; ++y) {
    float tA = (float)(y - y0) / (y1 - y0);
    int xA = (int)((1.0f - tA)*x0 + tA*x1);
    float tB = (float)(y - y0) / (y2 - y0);
    int xB = (int)((1.0f - tB)*x0 + tB*x2);
    drawLine(xA, y, xB, y, color);
  }

  for (int y = y1; y <= y2; ++y) {
    float tA = (float)(y - y1) / (y2 - y1);
    int xC = (int)((1.0f - tA)*x1 + tA*x2);
    float tB = (float)(y - y0) / (y2 - y0);
    int xD = (int)((1.0f - tB)*x0 + tB*x2);
    drawLine(xC, y, xD, y, color);
  }
}

void drawTriangleBarycentric(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color) {
  int minX = x0;
  if (x1 < minX) minX = x1;
  if (x2 < minX) minX = x2;

  int minY = y0;
  if (y1 < minY) minY = y1;
  if (y2 < minY) minY = y2;

  int maxX = x0;
  if (x1 > maxX) maxX = x1;
  if (x2 > maxX) maxX = x2;

  int maxY = y0;
  if (y1 > maxY) maxY = y1;
  if (y2 > maxY) maxY = y2;

  for (int y = minY; y <= maxY; ++y) {
    for (int x = minX; x <= maxX; ++x) {
      setPixel(x, y, color);
    }
  }
}

bool debugBarycentric = true;

void drawTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color) {
  if (debugBarycentric) {
    drawTriangleBarycentric(x0, y0, x1, y1, x2, y2, color);
  } else {
    drawTriangleLineSweep(x0, y0, x1, y1, x2, y2, color);
  }
}

typedef struct {
  float x, y, z;
} Vec3;

typedef struct {
  int v[3];
} Face;

#define NUM_VERTICES 1258
Vec3 vertices[NUM_VERTICES];

#define NUM_FACES 2492
Face faces[NUM_FACES];

void readObjFile() {
  BOOL success;

  HANDLE fileHandle = CreateFile("african_head.obj", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  assert(fileHandle != INVALID_HANDLE_VALUE);

  LARGE_INTEGER fileSize;
  success = GetFileSizeEx(fileHandle, &fileSize);
  assert(success);

  char *fileContents = malloc(fileSize.LowPart);

  DWORD numBytesRead;
  success = ReadFile(fileHandle, fileContents, fileSize.LowPart, &numBytesRead, NULL);
  assert(success);
  assert(numBytesRead == fileSize.LowPart);

  char *p = fileContents;
  char *end = fileContents + fileSize.LowPart;

  Vec3 *v = vertices;
  Face *f = faces;

  while (p < end) {
    char line[64];
    int lineSize = 0;

    while (*p != '\n' && p < end) {
      line[lineSize++] = *(p++);
    }
    line[lineSize] = '\0';
    ++p;

    if (lineSize > 2) {
      if (line[0] == 'v' && line[1] == ' ') {
        int numAssigned = sscanf_s(line, "v %f %f %f", &v->x, &v->y, &v->z);
        assert(numAssigned == 3);
        ++v;
      }
      if (line[0] == 'f' && line[1] == ' ') {
        int trash;
        int numAssigned = sscanf_s(line, "f %d/%d/%d %d/%d/%d %d/%d/%d", &f->v[0], &trash, &trash, &f->v[1], &trash, &trash, &f->v[2], &trash, &trash);
        assert(numAssigned == 9);
        f->v[0] -= 1;
        f->v[1] -= 1;
        f->v[2] -= 1;
        assert(f->v[0] >= 0 && f->v[0] < NUM_VERTICES);
        assert(f->v[1] >= 0 && f->v[1] < NUM_VERTICES);
        assert(f->v[2] >= 0 && f->v[2] < NUM_VERTICES);
        ++f;
      }
    }
  }

  assert(v == vertices + NUM_VERTICES);
  assert(f == faces + NUM_FACES);

  free(fileContents);
}

typedef enum {BUTTON_EXIT, BUTTON_ACTION, BUTTON_F1, BUTTON_COUNT} Button;

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

  HWND wnd = CreateWindowEx(0, wndClass.lpszClassName, "Renderer", wndStyle, 300, 50,
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

  //

  readObjFile();

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
              case VK_F1:
                buttonIsDown[BUTTON_F1] = isDown;
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

    if (buttonIsPressed(BUTTON_F1)) {
      debugBarycentric = !debugBarycentric;
    }

    {
      POINT p;
      GetCursorPos(&p);
      ScreenToClient(wnd, &p);
      mousePosX = (int) ((float)p.x / (float)WINDOW_SCALE);
      mousePosY = (int) ((float)p.y / (float)WINDOW_SCALE);
      //debugPrint("%d,%d\n", mousePosX, mousePosY);
    }

    drawFilledRect(0, 0, BACKBUFFER_WIDTH-1, BACKBUFFER_HEIGHT-1, 0xFF111133);

#if 0
    drawLine(13, 20, 80, 40, WHITE);
    drawLine(85, 40, 18, 20, RED);

    drawLine(20, 13, 40, 80, RED);
    drawLine(45, 80, 25, 13, WHITE);

    drawLine(45, 80, 60, 80, 0xFF00FF00);
    drawLine(60, 78, 45, 78, 0xFFCCFF00);

    drawLine(45, 30, 45, 80, 0xFF00FF00);
    drawLine(47, 80, 47, 30, 0xFFCCFF00);

    drawLine(5, 5, 5, 5, 0xFFCCFF00);

    drawLine(5, 100, 100, 70, RED);
#endif

#if 0
    for (int i = 0; i < NUM_FACES; ++i) {
      Face *f = &faces[i];
      for (int j = 0; j < 3; ++j) {
        Vec3 *v0 = &vertices[f->v[j]];
        Vec3 *v1 = &vertices[f->v[(j+1)%3]];
        int x0 = (int)((v0->x + 1.0f) * (BACKBUFFER_WIDTH-1) / 2.0f);
        int y0 = (int)((v0->y + 1.0f) * (BACKBUFFER_HEIGHT-1) / 2.0f);
        int x1 = (int)((v1->x + 1.0f) * (BACKBUFFER_WIDTH-1) / 2.0f);
        int y1 = (int)((v1->y + 1.0f) * (BACKBUFFER_HEIGHT-1) / 2.0f);
        drawLine(x0, y0, x1, y1, WHITE);
      }
    }
#endif

    drawTriangle(10, 70, 50, 160, 70, 80, RED);
    drawTriangle(180, 50, 150, 1, 70, 180, WHITE);
    drawTriangle(180, 150, 120, 160, 130, 180, GREEN);

    StretchDIBits(deviceContext,
                  0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
                  0, 0, BACKBUFFER_WIDTH, BACKBUFFER_HEIGHT,
                  backbuffer, &bitmapInfo,
                  DIB_RGB_COLORS, SRCCOPY);
  }
}
