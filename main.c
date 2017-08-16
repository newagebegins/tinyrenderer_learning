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

#if 1
#define BACKBUFFER_WIDTH 900
#define WINDOW_SCALE 1
#else
#define BACKBUFFER_WIDTH 200
#define WINDOW_SCALE 4
#endif

#define BACKBUFFER_HEIGHT BACKBUFFER_WIDTH
#define BACKBUFFER_BYTES (BACKBUFFER_WIDTH * BACKBUFFER_HEIGHT * sizeof(uint32_t))
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
  assert(y0 <= y1 && y1 <= y2);

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

typedef struct {
  float x, y, z;
} Vec3;

Vec3 crossVec3(Vec3 a, Vec3 b) {
  Vec3 r;
  r.x = a.y*b.z - b.y*a.z;
  r.y = b.x*a.z - a.x*b.z;
  r.z = a.x*b.y - a.y*b.x;
  return r;
}

Vec3 subVec3(Vec3 a, Vec3 b) {
  Vec3 r;
  r.x = a.x - b.x;
  r.y = a.y - b.y;
  r.z = a.z - b.z;
  return r;
}

Vec3 makeVec3(float x, float y, float z) {
  Vec3 r;
  r.x = x;
  r.y = y;
  r.z = z;
  return r;
}

float dotVec3(Vec3 a, Vec3 b) {
  return a.x*b.x + a.y*b.y + a.z*b.z;
}

float lengthSquaredVec3(Vec3 v) {
  return dotVec3(v,v);
}

float lengthVec3(Vec3 v) {
  return sqrtf(lengthSquaredVec3(v));
}

Vec3 scaleVec3(Vec3 v, float a) {
  return makeVec3(v.x*a, v.y*a, v.z*a);
}

Vec3 normalizeVec3(Vec3 v) {
  return scaleVec3(v, 1.0f / lengthVec3(v));
}

Vec3 getBarycentricCoords(Vec3 A, Vec3 B, Vec3 C, Vec3 P) {
  Vec3 AB = subVec3(B, A);
  Vec3 AC = subVec3(C, A);
  Vec3 PA = subVec3(A, P);
  Vec3 v1 = makeVec3(AB.x, AC.x, PA.x);
  Vec3 v2 = makeVec3(AB.y, AC.y, PA.y);
  Vec3 c = crossVec3(v1, v2);
  if (fabs(c.z) < 0.00001f) return makeVec3(-1.0f, 1.0f, 1.0f);
  float u = c.x / c.z;
  float v = c.y / c.z;
  float w = 1.0f - u - v;
  return makeVec3(w, u, v);
}

float zBuffer[BACKBUFFER_WIDTH*BACKBUFFER_HEIGHT];

void drawTriangleBarycentric(float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2, uint32_t color) {
  float minX = x0;
  if (x1 < minX) minX = x1;
  if (x2 < minX) minX = x2;

  float minY = y0;
  if (y1 < minY) minY = y1;
  if (y2 < minY) minY = y2;

  float maxX = x0;
  if (x1 > maxX) maxX = x1;
  if (x2 > maxX) maxX = x2;

  float maxY = y0;
  if (y1 > maxY) maxY = y1;
  if (y2 > maxY) maxY = y2;

  Vec3 A = makeVec3(x0, y0, 0);
  Vec3 B = makeVec3(x1, y1, 0);
  Vec3 C = makeVec3(x2, y2, 0);

  int minXi = (int)(minX + 0.5f);
  int maxXi = (int)(maxX + 0.5f);
  int minYi = (int)(minY + 0.5f);
  int maxYi = (int)(maxY + 0.5f);

  for (int y = minYi; y <= maxYi; ++y) {
    for (int x = minXi; x <= maxXi; ++x) {
      Vec3 p = makeVec3((float)x, (float)y, 0);
      Vec3 b = getBarycentricCoords(A, B, C, p);
      if (b.x < 0 || b.y < 0 || b.z < 0) continue;
      float z = z0*b.x + z1*b.y + z2*b.z;
      int i = x + BACKBUFFER_WIDTH*y;
      if (z > zBuffer[i]) {
        zBuffer[i] = z;
        setPixel(x, y, color);
      }
    }
  }
}

uint32_t makeColor(int r, int g, int b) {
  assert(r >= 0 && r <= 0xFF);
  assert(g >= 0 && g <= 0xFF);
  assert(b >= 0 && b <= 0xFF);
  //0xFFRRGGBB
  uint32_t color = (0xFF << 8*3) | (r << 8*2) | (g << 8*1) | (b << 8*0);
  return color;
}

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

    {
      POINT p;
      GetCursorPos(&p);
      ScreenToClient(wnd, &p);
      mousePosX = (int) ((float)p.x / (float)WINDOW_SCALE);
      mousePosY = (BACKBUFFER_HEIGHT-1) - (int) ((float)p.y / (float)WINDOW_SCALE);
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

    Vec3 lightDir = makeVec3(0,0,-1);

    for (int i = 0; i < BACKBUFFER_WIDTH*BACKBUFFER_HEIGHT; ++i) {
      zBuffer[i] = -9999.0f;
    }

#if 1
    for (int i = 0; i < NUM_FACES; ++i) {
      Face *f = &faces[i];
      Vec3 *v0 = &vertices[f->v[0]];
      Vec3 *v1 = &vertices[f->v[1]];
      Vec3 *v2 = &vertices[f->v[2]];
      float x0 = (v0->x + 1.0f) * (BACKBUFFER_WIDTH-1) / 2.0f;
      float y0 = (v0->y + 1.0f) * (BACKBUFFER_HEIGHT-1) / 2.0f;
      float x1 = (v1->x + 1.0f) * (BACKBUFFER_WIDTH-1) / 2.0f;
      float y1 = (v1->y + 1.0f) * (BACKBUFFER_HEIGHT-1) / 2.0f;
      float x2 = (v2->x + 1.0f) * (BACKBUFFER_WIDTH-1) / 2.0f;
      float y2 = (v2->y + 1.0f) * (BACKBUFFER_HEIGHT-1) / 2.0f;

      Vec3 n = crossVec3(subVec3(*v2, *v0), subVec3(*v1, *v0));
      n = normalizeVec3(n);
      float intensity = dotVec3(n, lightDir);
      if (intensity > 0) {
        int c = (int)(intensity*0xFF);
        uint32_t color = makeColor(c,c,c);
        drawTriangleBarycentric(x0, y0, v0->z, x1, y1, v1->z, x2, y2, v2->z, color);
      }
    }
#endif

#if 0
    drawTriangle(10, 70, 50, 160, 70, 80, RED);
    drawTriangle(180, 50, 150, 1, 70, 180, WHITE);
    drawTriangle(180, 150, 120, 160, 130, 180, GREEN);
#endif

    StretchDIBits(deviceContext,
                  0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
                  0, 0, BACKBUFFER_WIDTH, BACKBUFFER_HEIGHT,
                  backbuffer, &bitmapInfo,
                  DIB_RGB_COLORS, SRCCOPY);
  }
}
