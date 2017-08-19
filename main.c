#include <windows.h>
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <float.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

void debugPrint(char *format, ...) {
  va_list argptr;
  va_start(argptr, format);
  char str[1024];
  vsprintf_s(str, sizeof(str), format, argptr);
  va_end(argptr);
  OutputDebugString(str);
}

#if 1
#define BACKBUFFER_WIDTH 500
#define WINDOW_SCALE 1
#else
#define BACKBUFFER_WIDTH 100
#define WINDOW_SCALE 8
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
  /* assert(x >= 0 && x < BACKBUFFER_WIDTH); */
  /* assert(y >= 0 && y < BACKBUFFER_HEIGHT); */

  if (x >= 0 && x < BACKBUFFER_WIDTH && y >= 0 && y < BACKBUFFER_HEIGHT)
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

typedef struct {
  float x, y, z, w;
} Vec4;

Vec4 makeVec4(float x, float y, float z, float w) {
  Vec4 r;
  r.x = x;
  r.y = y;
  r.z = z;
  r.w = w;
  return r;
}

typedef struct {
  float d[4][4];
} Mat4;

Mat4 makeMat4(float d00, float d01, float d02, float d03,
              float d10, float d11, float d12, float d13,
              float d20, float d21, float d22, float d23,
              float d30, float d31, float d32, float d33) {
  Mat4 m;
  m.d[0][0] = d00; m.d[0][1] = d01; m.d[0][2] = d02; m.d[0][3] = d03;
  m.d[1][0] = d10; m.d[1][1] = d11; m.d[1][2] = d12; m.d[1][3] = d13;
  m.d[2][0] = d20; m.d[2][1] = d21; m.d[2][2] = d22; m.d[2][3] = d23;
  m.d[3][0] = d30; m.d[3][1] = d31; m.d[3][2] = d32; m.d[3][3] = d33;
  return m;
}

Mat4 getIdentityMat4() {
  return makeMat4(1,0,0,0,
                  0,1,0,0,
                  0,0,1,0,
                  0,0,0,1);
}

Mat4 mulMat4(Mat4 m, Mat4 n) {
  Mat4 r;

  r.d[0][0] = m.d[0][0]*n.d[0][0] + m.d[0][1]*n.d[1][0] + m.d[0][2]*n.d[2][0] + m.d[0][3]*n.d[3][0];
  r.d[0][1] = m.d[0][0]*n.d[0][1] + m.d[0][1]*n.d[1][1] + m.d[0][2]*n.d[2][1] + m.d[0][3]*n.d[3][1];
  r.d[0][2] = m.d[0][0]*n.d[0][2] + m.d[0][1]*n.d[1][2] + m.d[0][2]*n.d[2][2] + m.d[0][3]*n.d[3][2];
  r.d[0][3] = m.d[0][0]*n.d[0][3] + m.d[0][1]*n.d[1][3] + m.d[0][2]*n.d[2][3] + m.d[0][3]*n.d[3][3];

  r.d[1][0] = m.d[1][0]*n.d[0][0] + m.d[1][1]*n.d[1][0] + m.d[1][2]*n.d[2][0] + m.d[1][3]*n.d[3][0];
  r.d[1][1] = m.d[1][0]*n.d[0][1] + m.d[1][1]*n.d[1][1] + m.d[1][2]*n.d[2][1] + m.d[1][3]*n.d[3][1];
  r.d[1][2] = m.d[1][0]*n.d[0][2] + m.d[1][1]*n.d[1][2] + m.d[1][2]*n.d[2][2] + m.d[1][3]*n.d[3][2];
  r.d[1][3] = m.d[1][0]*n.d[0][3] + m.d[1][1]*n.d[1][3] + m.d[1][2]*n.d[2][3] + m.d[1][3]*n.d[3][3];

  r.d[2][0] = m.d[2][0]*n.d[0][0] + m.d[2][1]*n.d[1][0] + m.d[2][2]*n.d[2][0] + m.d[2][3]*n.d[3][0];
  r.d[2][1] = m.d[2][0]*n.d[0][1] + m.d[2][1]*n.d[1][1] + m.d[2][2]*n.d[2][1] + m.d[2][3]*n.d[3][1];
  r.d[2][2] = m.d[2][0]*n.d[0][2] + m.d[2][1]*n.d[1][2] + m.d[2][2]*n.d[2][2] + m.d[2][3]*n.d[3][2];
  r.d[2][3] = m.d[2][0]*n.d[0][3] + m.d[2][1]*n.d[1][3] + m.d[2][2]*n.d[2][3] + m.d[2][3]*n.d[3][3];

  r.d[3][0] = m.d[3][0]*n.d[0][0] + m.d[3][1]*n.d[1][0] + m.d[3][2]*n.d[2][0] + m.d[3][3]*n.d[3][0];
  r.d[3][1] = m.d[3][0]*n.d[0][1] + m.d[3][1]*n.d[1][1] + m.d[3][2]*n.d[2][1] + m.d[3][3]*n.d[3][1];
  r.d[3][2] = m.d[3][0]*n.d[0][2] + m.d[3][1]*n.d[1][2] + m.d[3][2]*n.d[2][2] + m.d[3][3]*n.d[3][2];
  r.d[3][3] = m.d[3][0]*n.d[0][3] + m.d[3][1]*n.d[1][3] + m.d[3][2]*n.d[2][3] + m.d[3][3]*n.d[3][3];

  return r;
}

Vec4 mulMatVec4(Mat4 m, Vec4 v) {
  Vec4 r;
  r.x = m.d[0][0]*v.x + m.d[0][1]*v.y + m.d[0][2]*v.z + m.d[0][3]*v.w;
  r.y = m.d[1][0]*v.x + m.d[1][1]*v.y + m.d[1][2]*v.z + m.d[1][3]*v.w;
  r.z = m.d[2][0]*v.x + m.d[2][1]*v.y + m.d[2][2]*v.z + m.d[2][3]*v.w;
  r.w = m.d[3][0]*v.x + m.d[3][1]*v.y + m.d[3][2]*v.z + m.d[3][3]*v.w;
  return r;
}

float determinantMat4(Mat4 m) {
  float det =
    m.d[0][3]*m.d[1][2]*m.d[2][1]*m.d[3][0] - m.d[0][2]*m.d[1][3]*m.d[2][1]*m.d[3][0] - m.d[0][3]*m.d[1][1]*m.d[2][2]*m.d[3][0] + m.d[0][1]*m.d[1][3]*m.d[2][2]*m.d[3][0]+
    m.d[0][2]*m.d[1][1]*m.d[2][3]*m.d[3][0] - m.d[0][1]*m.d[1][2]*m.d[2][3]*m.d[3][0] - m.d[0][3]*m.d[1][2]*m.d[2][0]*m.d[3][1] + m.d[0][2]*m.d[1][3]*m.d[2][0]*m.d[3][1]+
    m.d[0][3]*m.d[1][0]*m.d[2][2]*m.d[3][1] - m.d[0][0]*m.d[1][3]*m.d[2][2]*m.d[3][1] - m.d[0][2]*m.d[1][0]*m.d[2][3]*m.d[3][1] + m.d[0][0]*m.d[1][2]*m.d[2][3]*m.d[3][1]+
    m.d[0][3]*m.d[1][1]*m.d[2][0]*m.d[3][2] - m.d[0][1]*m.d[1][3]*m.d[2][0]*m.d[3][2] - m.d[0][3]*m.d[1][0]*m.d[2][1]*m.d[3][2] + m.d[0][0]*m.d[1][3]*m.d[2][1]*m.d[3][2]+
    m.d[0][1]*m.d[1][0]*m.d[2][3]*m.d[3][2] - m.d[0][0]*m.d[1][1]*m.d[2][3]*m.d[3][2] - m.d[0][2]*m.d[1][1]*m.d[2][0]*m.d[3][3] + m.d[0][1]*m.d[1][2]*m.d[2][0]*m.d[3][3]+
    m.d[0][2]*m.d[1][0]*m.d[2][1]*m.d[3][3] - m.d[0][0]*m.d[1][2]*m.d[2][1]*m.d[3][3] - m.d[0][1]*m.d[1][0]*m.d[2][2]*m.d[3][3] + m.d[0][0]*m.d[1][1]*m.d[2][2]*m.d[3][3];
  return det;
}

Mat4 invertMat4(Mat4 m) {
  Mat4 r;
  float det = determinantMat4(m);
  assert(fabs(det) > 0.001f);
  float invDet = 1.0f / det;
  r.d[0][0] = invDet * (m.d[1][2]*m.d[2][3]*m.d[3][1] - m.d[1][3]*m.d[2][2]*m.d[3][1] + m.d[1][3]*m.d[2][1]*m.d[3][2] - m.d[1][1]*m.d[2][3]*m.d[3][2] - m.d[1][2]*m.d[2][1]*m.d[3][3] + m.d[1][1]*m.d[2][2]*m.d[3][3]);
  r.d[0][1] = invDet * (m.d[0][3]*m.d[2][2]*m.d[3][1] - m.d[0][2]*m.d[2][3]*m.d[3][1] - m.d[0][3]*m.d[2][1]*m.d[3][2] + m.d[0][1]*m.d[2][3]*m.d[3][2] + m.d[0][2]*m.d[2][1]*m.d[3][3] - m.d[0][1]*m.d[2][2]*m.d[3][3]);
  r.d[0][2] = invDet * (m.d[0][2]*m.d[1][3]*m.d[3][1] - m.d[0][3]*m.d[1][2]*m.d[3][1] + m.d[0][3]*m.d[1][1]*m.d[3][2] - m.d[0][1]*m.d[1][3]*m.d[3][2] - m.d[0][2]*m.d[1][1]*m.d[3][3] + m.d[0][1]*m.d[1][2]*m.d[3][3]);
  r.d[0][3] = invDet * (m.d[0][3]*m.d[1][2]*m.d[2][1] - m.d[0][2]*m.d[1][3]*m.d[2][1] - m.d[0][3]*m.d[1][1]*m.d[2][2] + m.d[0][1]*m.d[1][3]*m.d[2][2] + m.d[0][2]*m.d[1][1]*m.d[2][3] - m.d[0][1]*m.d[1][2]*m.d[2][3]);
  r.d[1][0] = invDet * (m.d[1][3]*m.d[2][2]*m.d[3][0] - m.d[1][2]*m.d[2][3]*m.d[3][0] - m.d[1][3]*m.d[2][0]*m.d[3][2] + m.d[1][0]*m.d[2][3]*m.d[3][2] + m.d[1][2]*m.d[2][0]*m.d[3][3] - m.d[1][0]*m.d[2][2]*m.d[3][3]);
  r.d[1][1] = invDet * (m.d[0][2]*m.d[2][3]*m.d[3][0] - m.d[0][3]*m.d[2][2]*m.d[3][0] + m.d[0][3]*m.d[2][0]*m.d[3][2] - m.d[0][0]*m.d[2][3]*m.d[3][2] - m.d[0][2]*m.d[2][0]*m.d[3][3] + m.d[0][0]*m.d[2][2]*m.d[3][3]);
  r.d[1][2] = invDet * (m.d[0][3]*m.d[1][2]*m.d[3][0] - m.d[0][2]*m.d[1][3]*m.d[3][0] - m.d[0][3]*m.d[1][0]*m.d[3][2] + m.d[0][0]*m.d[1][3]*m.d[3][2] + m.d[0][2]*m.d[1][0]*m.d[3][3] - m.d[0][0]*m.d[1][2]*m.d[3][3]);
  r.d[1][3] = invDet * (m.d[0][2]*m.d[1][3]*m.d[2][0] - m.d[0][3]*m.d[1][2]*m.d[2][0] + m.d[0][3]*m.d[1][0]*m.d[2][2] - m.d[0][0]*m.d[1][3]*m.d[2][2] - m.d[0][2]*m.d[1][0]*m.d[2][3] + m.d[0][0]*m.d[1][2]*m.d[2][3]);
  r.d[2][0] = invDet * (m.d[1][1]*m.d[2][3]*m.d[3][0] - m.d[1][3]*m.d[2][1]*m.d[3][0] + m.d[1][3]*m.d[2][0]*m.d[3][1] - m.d[1][0]*m.d[2][3]*m.d[3][1] - m.d[1][1]*m.d[2][0]*m.d[3][3] + m.d[1][0]*m.d[2][1]*m.d[3][3]);
  r.d[2][1] = invDet * (m.d[0][3]*m.d[2][1]*m.d[3][0] - m.d[0][1]*m.d[2][3]*m.d[3][0] - m.d[0][3]*m.d[2][0]*m.d[3][1] + m.d[0][0]*m.d[2][3]*m.d[3][1] + m.d[0][1]*m.d[2][0]*m.d[3][3] - m.d[0][0]*m.d[2][1]*m.d[3][3]);
  r.d[2][2] = invDet * (m.d[0][1]*m.d[1][3]*m.d[3][0] - m.d[0][3]*m.d[1][1]*m.d[3][0] + m.d[0][3]*m.d[1][0]*m.d[3][1] - m.d[0][0]*m.d[1][3]*m.d[3][1] - m.d[0][1]*m.d[1][0]*m.d[3][3] + m.d[0][0]*m.d[1][1]*m.d[3][3]);
  r.d[2][3] = invDet * (m.d[0][3]*m.d[1][1]*m.d[2][0] - m.d[0][1]*m.d[1][3]*m.d[2][0] - m.d[0][3]*m.d[1][0]*m.d[2][1] + m.d[0][0]*m.d[1][3]*m.d[2][1] + m.d[0][1]*m.d[1][0]*m.d[2][3] - m.d[0][0]*m.d[1][1]*m.d[2][3]);
  r.d[3][0] = invDet * (m.d[1][2]*m.d[2][1]*m.d[3][0] - m.d[1][1]*m.d[2][2]*m.d[3][0] - m.d[1][2]*m.d[2][0]*m.d[3][1] + m.d[1][0]*m.d[2][2]*m.d[3][1] + m.d[1][1]*m.d[2][0]*m.d[3][2] - m.d[1][0]*m.d[2][1]*m.d[3][2]);
  r.d[3][1] = invDet * (m.d[0][1]*m.d[2][2]*m.d[3][0] - m.d[0][2]*m.d[2][1]*m.d[3][0] + m.d[0][2]*m.d[2][0]*m.d[3][1] - m.d[0][0]*m.d[2][2]*m.d[3][1] - m.d[0][1]*m.d[2][0]*m.d[3][2] + m.d[0][0]*m.d[2][1]*m.d[3][2]);
  r.d[3][2] = invDet * (m.d[0][2]*m.d[1][1]*m.d[3][0] - m.d[0][1]*m.d[1][2]*m.d[3][0] - m.d[0][2]*m.d[1][0]*m.d[3][1] + m.d[0][0]*m.d[1][2]*m.d[3][1] + m.d[0][1]*m.d[1][0]*m.d[3][2] - m.d[0][0]*m.d[1][1]*m.d[3][2]);
  r.d[3][3] = invDet * (m.d[0][1]*m.d[1][2]*m.d[2][0] - m.d[0][2]*m.d[1][1]*m.d[2][0] + m.d[0][2]*m.d[1][0]*m.d[2][1] - m.d[0][0]*m.d[1][2]*m.d[2][1] - m.d[0][1]*m.d[1][0]*m.d[2][2] + m.d[0][0]*m.d[1][1]*m.d[2][2]);
  return r;
}

Mat4 transposeMat4(Mat4 m) {
  Mat4 r;
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      r.d[i][j] = m.d[j][i];
    }
  }
  return r;
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

uint32_t makeColor(int r, int g, int b) {
  assert(r >= 0 && r <= 0xFF);
  assert(g >= 0 && g <= 0xFF);
  assert(b >= 0 && b <= 0xFF);
  //0xFFRRGGBB
  uint32_t color = (0xFF << 8*3) | (r << 8*2) | (g << 8*1) | (b << 8*0);
  return color;
}

u32 scaleColor(u32 color, float scale) {
  u8 r = (color >> 8*2) & 0xFF;
  u8 g = (color >> 8*1) & 0xFF;
  u8 b = (color >> 8*0) & 0xFF;
  u8 newR = (u8)(r*scale);
  u8 newG = (u8)(g*scale);
  u8 newB = (u8)(b*scale);
  return makeColor(newR, newG, newB);
}

#pragma pack(push, 1)
typedef struct {
  u16 origin;
  u16 length;
  u8 bitsPerEntry;
} TGAColorMap;

typedef struct {
  u16 xOrigin;
  u16 yOrigin;
  u16 width;
  u16 height;
  u8 bitsPerPixel;
  u8 descriptor;
} TGAImageSpecification;

typedef struct {
  u8 numCharsInIdField;
  u8 colorMapType;
  u8 imageType;
  TGAColorMap colorMap;
  TGAImageSpecification imageSpec;
} TGAHeader;
#pragma pack(pop)

typedef struct {
  u32 *pixels;
  u32 width;
  u32 height;
} Texture;

Texture readTGAFile(char *filePath) {
  Texture result;
  BOOL success;

  HANDLE fileHandle = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  assert(fileHandle != INVALID_HANDLE_VALUE);

  LARGE_INTEGER fileSize;
  success = GetFileSizeEx(fileHandle, &fileSize);
  assert(success);

  u8 *fileContents = malloc(fileSize.LowPart);

  DWORD numBytesRead;
  success = ReadFile(fileHandle, fileContents, fileSize.LowPart, &numBytesRead, NULL);
  assert(success);
  assert(numBytesRead == fileSize.LowPart);

  TGAHeader *header = (TGAHeader *)fileContents;
  u8 *data = fileContents + sizeof(TGAHeader);
  assert(header->imageType == 10);
  assert(header->imageSpec.bitsPerPixel == 24);
  assert(header->numCharsInIdField == 0);
  assert(header->colorMapType == 0);
  assert(header->imageSpec.xOrigin == 0);
  assert(header->imageSpec.yOrigin == 0);
  result.width = header->imageSpec.width;
  result.height = header->imageSpec.height;
  result.pixels = malloc(result.width * result.height * sizeof(u32));
  u32 *tex = result.pixels;
  u32 *texEnd = result.pixels + (result.width*result.height);

  while (tex < texEnd) {
    bool isRLE = *data & 0x80;
    u8 length = (*(data++) & 0x7F) + 1;
    if (isRLE) {
      u8 b = *(data++);
      u8 g = *(data++);
      u8 r = *(data++);
      u32 color = makeColor(r, g, b);
      for (int j = 0; j < length; ++j) {
        *(tex++) = color;
      }
    } else {
      for (int j = 0; j < length; ++j) {
        u8 b = *(data++);
        u8 g = *(data++);
        u8 r = *(data++);
        u32 color = makeColor(r, g, b);
        *(tex++) = color;
      }
    }
  }

  free(fileContents);
  return result;
}

float zBuffer[BACKBUFFER_WIDTH*BACKBUFFER_HEIGHT];
bool isTextured = true;
bool isGoroud = true;

void drawTriangleBarycentric(float x0, float y0, float z0, float u0, float v0,
                             float x1, float y1, float z1, float u1, float v1,
                             float x2, float y2, float z2, float u2, float v2,
                             float in0, float in1, float in2,
                             Texture texture) {
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
      if (x < 0 || x >= BACKBUFFER_WIDTH) continue;
      if (y < 0 || y >= BACKBUFFER_HEIGHT) continue;
      Vec3 p = makeVec3((float)x, (float)y, 0);
      Vec3 b = getBarycentricCoords(A, B, C, p);
      if (b.x < 0 || b.y < 0 || b.z < 0) continue;
      float z = z0*b.x + z1*b.y + z2*b.z;
      int i = x + BACKBUFFER_WIDTH*y;
      assert(i >= 0 && i < BACKBUFFER_WIDTH*BACKBUFFER_HEIGHT);
      if (z > zBuffer[i]) {
        zBuffer[i] = z;

        // texture
        float u = u0*b.x + u1*b.y + u2*b.z;
        float v = v0*b.x + v1*b.y + v2*b.z;
        int tx = (int)(u*(texture.width-1));
        int ty = (int)(v*(texture.height-1));
        u32 texColor = texture.pixels[tx + ty*texture.width];

        // goroud shading
        float intensity = in0*b.x + in1*b.y + in2*b.z;

        u32 color;

        if (isTextured) {
          if (isGoroud) {
            color = scaleColor(texColor, intensity);
          } else {
            color = texColor;
          }
        } else {
          int g = (int)(intensity*255.0f);
          color = makeColor(g,g,g);
        }
        setPixel(x, y, color);
      }
    }
  }
}

typedef struct {
  int v[3];
  int vt[3];
  int vn[3];
} Face;

#define NUM_VERTICES 1258
Vec3 vertices[NUM_VERTICES];
Vec3 normals[NUM_VERTICES];

#define NUM_FACES 2492
Face faces[NUM_FACES];

#define NUM_TEX_VERTS 1339
Vec3 texVerts[NUM_TEX_VERTS];

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
  Vec3 *vt = texVerts;
  Vec3 *vn = normals;
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
      else if (line[0] == 'v' && line[1] == 't' && line[2] == ' ') {
        int numAssigned = sscanf_s(line, "vt %f %f %f", &vt->x, &vt->y, &vt->z);
        assert(numAssigned == 3);
        ++vt;
      }
      else if (line[0] == 'v' && line[1] == 'n' && line[2] == ' ') {
        int numAssigned = sscanf_s(line, "vn %f %f %f", &vn->x, &vn->y, &vn->z);
        assert(numAssigned == 3);
        ++vn;
      }
      else if (line[0] == 'f' && line[1] == ' ') {
        int numAssigned = sscanf_s(line, "f %d/%d/%d %d/%d/%d %d/%d/%d", &f->v[0], &f->vt[0], &f->vn[0], &f->v[1], &f->vt[1], &f->vn[1], &f->v[2], &f->vt[2], &f->vn[2]);
        assert(numAssigned == 9);
        f->v[0] -= 1;
        f->v[1] -= 1;
        f->v[2] -= 1;
        f->vt[0] -= 1;
        f->vt[1] -= 1;
        f->vt[2] -= 1;
        f->vn[0] -= 1;
        f->vn[1] -= 1;
        f->vn[2] -= 1;
        assert(f->v[0] >= 0 && f->v[0] < NUM_VERTICES);
        assert(f->v[1] >= 0 && f->v[1] < NUM_VERTICES);
        assert(f->v[2] >= 0 && f->v[2] < NUM_VERTICES);
        assert(f->vt[0] >= 0 && f->vt[0] < NUM_TEX_VERTS);
        assert(f->vt[1] >= 0 && f->vt[1] < NUM_TEX_VERTS);
        assert(f->vt[2] >= 0 && f->vt[2] < NUM_TEX_VERTS);
        assert(f->vn[0] >= 0 && f->vn[0] < NUM_VERTICES);
        assert(f->vn[1] >= 0 && f->vn[1] < NUM_VERTICES);
        assert(f->vn[2] >= 0 && f->vn[2] < NUM_VERTICES);
        ++f;
      }
    }
  }

  assert(v == vertices + NUM_VERTICES);
  assert(f == faces + NUM_FACES);

  free(fileContents);
}

typedef enum {BUTTON_EXIT, BUTTON_ACTION, BUTTON_F1, BUTTON_F2, BUTTON_F3, BUTTON_F4, BUTTON_F5, BUTTON_F6, BUTTON_F7, BUTTON_COUNT} Button;

bool buttonIsDown[BUTTON_COUNT];
bool buttonWasDown[BUTTON_COUNT];

bool buttonIsPressed(Button button) {
  return buttonIsDown[button] && !buttonWasDown[button];
}

Mat4 getLookAtMat(Vec3 eye, Vec3 center, Vec3 up) {
  Vec3 z = normalizeVec3(subVec3(eye, center));
  Vec3 x = normalizeVec3(crossVec3(z, up));
  Vec3 y = crossVec3(x, z);
  Mat4 mInv = makeMat4(x.x, x.y, x.z, 0,
                       y.x, y.y, y.z, 0,
                       z.x, z.y, z.z, 0,
                         0,   0,   0, 1);
  Mat4 tr = makeMat4(1, 0, 0, -center.x,
                     0, 1, 0, -center.y,
                     0, 0, 1, -center.z,
                     0, 0, 0,         1);
  return mulMat4(mInv, tr);
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

  HWND wnd = CreateWindowEx(0, wndClass.lpszClassName, "Renderer", wndStyle, 300, 0,
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

  Vec3 cameraPos = makeVec3(3.0f, 3.0f, 4.0f);
  Vec3 cameraTarget = makeVec3(0, 0, 0);
  bool isCameraEnabled = true;
  //

  readObjFile();

#if 1
  Texture texture = readTGAFile("african_head_diffuse.tga");
#else
  Texture texture = readTGAFile("test.tga");
#endif

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
              case VK_F2:
                buttonIsDown[BUTTON_F2] = isDown;
                break;
              case VK_F3:
                buttonIsDown[BUTTON_F3] = isDown;
                break;
              case VK_F4:
                buttonIsDown[BUTTON_F4] = isDown;
              case VK_F5:
                buttonIsDown[BUTTON_F5] = isDown;
                break;
              case VK_F6:
                buttonIsDown[BUTTON_F6] = isDown;
              case VK_F7:
                buttonIsDown[BUTTON_F7] = isDown;
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

    static bool perspectiveEnabled = true;
    if (buttonIsPressed(BUTTON_F1)) {
      perspectiveEnabled = !perspectiveEnabled;
    }
    if (buttonIsPressed(BUTTON_F4)) {
      isTextured = !isTextured;
      if (isTextured) debugPrint("texture on\n");
      else debugPrint("texture off\n");
    }
    if (buttonIsPressed(BUTTON_F5)) {
      isGoroud = !isGoroud;
      if (isGoroud) debugPrint("goroud on\n");
      else debugPrint("goroud off\n");
    }

    {
      POINT p;
      GetCursorPos(&p);
      ScreenToClient(wnd, &p);
      mousePosX = (int) ((float)p.x / (float)WINDOW_SCALE);
      mousePosY = (BACKBUFFER_HEIGHT-1) - (int) ((float)p.y / (float)WINDOW_SCALE);
      //debugPrint("%d,%d\n", mousePosX, mousePosY);
    }

    drawFilledRect(0, 0, BACKBUFFER_WIDTH-1, BACKBUFFER_HEIGHT-1, makeColor(135, 181, 218));

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

    for (int i = 0; i < BACKBUFFER_WIDTH*BACKBUFFER_HEIGHT; ++i) {
      zBuffer[i] = -9999.0f;
    }

#if 1
    Vec3 lightDir = makeVec3(0,0,-1);
    float cameraStep = 0.5f;
    if (buttonIsDown[BUTTON_F3]) {
      cameraPos.z += cameraStep;
      debugPrint("cameraPos.z: %f\n", cameraPos.z);
    }
    if (buttonIsDown[BUTTON_F2]) {
      cameraPos.z -= cameraStep;
      debugPrint("cameraPos.z: %f\n", cameraPos.z);
    }
    if (buttonIsPressed(BUTTON_F6)) {
      isCameraEnabled = !isCameraEnabled;
      debugPrint("isCameraEnabled: %d\n", isCameraEnabled);
    }
    float cameraZ = lengthVec3(subVec3(cameraPos, cameraTarget));
    float r = perspectiveEnabled ? -1.0f/cameraZ : 0.0f;
    Mat4 projectionMatrix = makeMat4(1,0,0,0,
                                     0,1,0,0,
                                     0,0,1,0,
                                     0,0,r,1);
    Mat4 viewMat = isCameraEnabled ? getLookAtMat(cameraPos, cameraTarget, makeVec3(0, 1, 0)) : getIdentityMat4();

    Mat4 viewportMat;
    {
      float w = BACKBUFFER_WIDTH-1;
      float h = BACKBUFFER_HEIGHT-1;
      float d = 255.0f; //map z from [-1,1] to [0,255]
      viewportMat = makeMat4(w/2.0f,      0,      0, w/2.0f,
                                  0, h/2.0f,      0, h/2.0f,
                                  0,      0, d/2.0f, d/2.0f,
                                  0,      0,      0,      1);
    }

    Mat4 transformMat = mulMat4(viewportMat, mulMat4(projectionMatrix, viewMat));
    Mat4 normalTransformMat = invertMat4(transposeMat4(transformMat));

    for (int i = 0; i < NUM_FACES; ++i) {
      Face *f = &faces[i];

      Vec3 v0orig = vertices[f->v[0]];
      Vec3 v1orig = vertices[f->v[1]];
      Vec3 v2orig = vertices[f->v[2]];

      Vec4 v04 = makeVec4(v0orig.x, v0orig.y, v0orig.z, 1.0f);
      Vec4 v14 = makeVec4(v1orig.x, v1orig.y, v1orig.z, 1.0f);
      Vec4 v24 = makeVec4(v2orig.x, v2orig.y, v2orig.z, 1.0f);

      Vec4 v0h = mulMatVec4(transformMat, v04);
      Vec4 v1h = mulMatVec4(transformMat, v14);
      Vec4 v2h = mulMatVec4(transformMat, v24);

      v0h = makeVec4(v0h.x/v0h.w, v0h.y/v0h.w, v0h.z/v0h.w, v0h.w/v0h.w);
      v1h = makeVec4(v1h.x/v1h.w, v1h.y/v1h.w, v1h.z/v1h.w, v1h.w/v1h.w);
      v2h = makeVec4(v2h.x/v2h.w, v2h.y/v2h.w, v2h.z/v2h.w, v2h.w/v2h.w);

      Vec3 *vt0 = &texVerts[f->vt[0]];
      Vec3 *vt1 = &texVerts[f->vt[1]];
      Vec3 *vt2 = &texVerts[f->vt[2]];

      float x0 = v0h.x;
      float y0 = v0h.y;
      float x1 = v1h.x;
      float y1 = v1h.y;
      float x2 = v2h.x;
      float y2 = v2h.y;

      Vec3 n0 = normals[f->vn[0]];
      Vec3 n1 = normals[f->vn[1]];
      Vec3 n2 = normals[f->vn[2]];

      Vec4 n04 = mulMatVec4(normalTransformMat, makeVec4(n0.x, n0.y, n0.z, 0.0f));
      Vec4 n14 = mulMatVec4(normalTransformMat, makeVec4(n1.x, n1.y, n1.z, 0.0f));
      Vec4 n24 = mulMatVec4(normalTransformMat, makeVec4(n2.x, n2.y, n2.z, 0.0f));

      n0 = makeVec3(n04.x, n04.y, n04.z);
      n1 = makeVec3(n14.x, n14.y, n14.z);
      n2 = makeVec3(n24.x, n24.y, n24.z);

      n0 = normalizeVec3(n0);
      n1 = normalizeVec3(n1);
      n2 = normalizeVec3(n2);

      float in0 = -dotVec3(n0, lightDir);
      float in1 = -dotVec3(n1, lightDir);
      float in2 = -dotVec3(n2, lightDir);
      if (in0 < 0) in0 = 0;
      if (in1 < 0) in1 = 0;
      if (in2 < 0) in2 = 0;
      assert(in0 >= 0.0f && in0 <= 1.0f);
      assert(in1 >= 0.0f && in1 <= 1.0f);
      assert(in2 >= 0.0f && in2 <= 1.0f);

      drawTriangleBarycentric(x0, y0, v0h.z, vt0->x, vt0->y,
                              x1, y1, v1h.z, vt1->x, vt1->y,
                              x2, y2, v2h.z, vt2->x, vt2->y,
                              in0, in1, in2,
                              texture);
    }
#endif

#if 0
#if 0
    for (u32 i = 0; i < TEX_SIZE; ++i) {
      f32 tx = (f32)(i % TEX_WIDTH) / (TEX_WIDTH-1);
      f32 ty = (f32)(i / TEX_WIDTH) / (TEX_WIDTH-1);
      u32 x = (u32)(tx*(BACKBUFFER_WIDTH-1));
      u32 y = (u32)(ty*(BACKBUFFER_HEIGHT-1));
      u32 color = texture[i];
      setPixel(x, y, color);
    }
#else
    for (u32 i = 0; i < texture.width*texture.height; ++i) {
      u32 x = i % texture.width;
      u32 y = i / texture.width;
      u32 color = texture.pixels[i];
      if (x < BACKBUFFER_WIDTH && y < BACKBUFFER_HEIGHT)
        setPixel(x, y, color);
    }
#endif
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
