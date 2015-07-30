#pragma once
#ifndef RENDERER_HPP_INCLUDED
#define RENDERER_HPP_INCLUDED

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <vector>
#include <iostream>
#include <omp.h>

#define _WIDTH 640
#define _HEIGHT 480
#define _WW (1024)
#define _WH (768)
#define _BPP 32

namespace detail {

class IBitmapRenderer {
public:
  virtual ~IBitmapRenderer() {}
  virtual void RenderToBitmap(HDC screenDC) = 0;
  virtual void HandleOutput(VOID *output) = 0;
  virtual void HandleDirection(int direction) = 0;
};

class RendererThread {
public:
  RendererThread(LPTHREAD_START_ROUTINE callback)
      : m_running(false), hThread(INVALID_HANDLE_VALUE), m_callback(callback) {}

  ~RendererThread() {}

  void Start(LPVOID lParam) {
    hThread = CreateThread(NULL, 0, m_callback, lParam, 0, NULL);
    m_running = true;
  }
  void Join() {
    if (m_running)
      m_running = false;

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    hThread = INVALID_HANDLE_VALUE;
  }

  void Delay(DWORD millis) {
    // Try to delay for |millis| time duration.
    // This is called from within the threading function (callback)
    // So it's safe to sleep in the calling thread.
    Sleep(millis);
  }

protected:
  bool m_running;
  HANDLE hThread;
  LPTHREAD_START_ROUTINE m_callback;
  // some protected stuff.
};

typedef unsigned int Uint32;

class RendererSurface {
public:
  RendererSurface(int w, int h, int bpp, IBitmapRenderer *renderer)
      : m_w(w), m_h(h), m_bpp(bpp), m_bitmapRenderer(renderer) {
    m_pixels = new unsigned char[m_w * m_h * (m_bpp / 8)];
    memset(m_pixels, 0, (m_w * m_h * (m_bpp / 8)));
    m_backBuffer = new unsigned char[m_w * m_h * (m_bpp / 8)];
    memset(m_backBuffer, 0, (m_w * m_h * (m_bpp / 8)));
  }

  ~RendererSurface() {}

  void Cleanup() {
    delete[] m_pixels;
    delete[] m_backBuffer;
    std::cout << "Destroying surface";
    m_pixels = nullptr;
    m_backBuffer = nullptr;
    m_bitmapRenderer = nullptr;
  }

  void SetScreen(unsigned char *buffer, HDC screenDC, HDC memDC) {
    m_screen = buffer;
    m_screenDC = screenDC;
    m_dc = memDC;
  }

  void SetDirection(int direction) {
    if (m_bitmapRenderer)
      m_bitmapRenderer->HandleDirection(direction);
  }

  void Flip(bool clear = false) {
    if (!m_pixels || !m_backBuffer)
      return;
    // We need a mechanism to actually present the buffer to the drawing system.
    unsigned char *temp = m_pixels;
    m_pixels = m_backBuffer;
    m_backBuffer = temp;

    memcpy(m_screen, m_pixels, (m_w * m_h * (m_bpp / 8)));
    if (m_bitmapRenderer)
      m_bitmapRenderer->RenderToBitmap(m_dc);

    // BitBlt(m_screenDC, 0, 0, m_w, m_h, m_dc, 0, 0, SRCCOPY);
    StretchBlt(m_screenDC, 0, 0, _WW, _WH, m_dc, 0, 0, m_w, m_h, SRCCOPY);

    if (clear)
      memset(m_backBuffer, 0, (m_w * m_h * (m_bpp / 8)));
  }

  Uint32 *GetPixels() { return reinterpret_cast<Uint32 *>(m_backBuffer); }

  int GetBPP() const { return m_bpp; }

  int GetWidth() const { return m_w; }

  int GetHeight() const { return m_h; }

  IBitmapRenderer *GetRenderer() const { return m_bitmapRenderer; }

protected:
  unsigned char *m_pixels;
  unsigned char *m_backBuffer;
  unsigned char *m_screen;
  int m_w;
  int m_h;
  int m_bpp;
  HDC m_screenDC;
  HDC m_dc;
  IBitmapRenderer *m_bitmapRenderer;
};

} // namespace detail

// Declaration and partial implementation.
class Renderer {
public:
  detail::RendererThread updateThread;
  detail::RendererSurface screen;

  bool bRunning;

  void SetBuffer(unsigned char *buffer, HDC scrDC, HDC memDC) {
    screen.SetScreen(buffer, scrDC, memDC);
    updateThread.Start(static_cast<LPVOID>(this));
    SetRunning(true);
  }
  void SetDirection(int direction) { screen.SetDirection(direction); }

public:
  Renderer(const char *const className, LPTHREAD_START_ROUTINE callback,
           detail::IBitmapRenderer *renderer);

  ~Renderer() {
    updateThread.Join();
    screen.Cleanup();
  }

  bool IsRunning() { return bRunning; }

  void SetRunning(bool bRun) { bRunning = bRun; }
};

// Here we declare the functions and variables used by the renderer instance
namespace forward {
Renderer *g_renderer;

void HandleKey(WPARAM wp, bool pressed) {
  switch (wp) {
  case VK_ESCAPE:
    PostQuitMessage(0);
    break;
  case VK_LEFT:
    if (g_renderer)
      g_renderer->SetDirection(pressed ? 0 : -1);
    break;
  case VK_UP:
    if (g_renderer)
      g_renderer->SetDirection(pressed ? 1 : -1);
    break;
  case VK_RIGHT:
    if (g_renderer)
      g_renderer->SetDirection(pressed ? 2 : -1);
    break;
  case VK_DOWN:
    if (g_renderer)
      g_renderer->SetDirection(pressed ? 3 : -1);
    break;
  default:
    break;
  }
}

long __stdcall WindowProcedure(HWND window, unsigned int msg, WPARAM wp,
                               LPARAM lp) {
  switch (msg) {
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0L;
  case WM_KEYDOWN:
    HandleKey(wp, true);
    return 0L;
  case WM_KEYUP:
    HandleKey(wp, false);
  default:
    return DefWindowProc(window, msg, wp, lp);
  }
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor,
                              LPRECT lprcMonitor, LPARAM dwData) {
  if (!dwData || !lprcMonitor)
    return TRUE;

  std::vector<RECT> *monitors = reinterpret_cast<std::vector<RECT> *>(dwData);
  RECT rct{lprcMonitor->left, lprcMonitor->top, lprcMonitor->right,
           lprcMonitor->bottom};
  monitors->push_back(rct);

  return TRUE;
}
} // namespace forward

// Implementation of the renderer functions.
Renderer::Renderer(const char *const className, LPTHREAD_START_ROUTINE callback,
                   detail::IBitmapRenderer *renderer)
    : screen(_WIDTH, _HEIGHT, _BPP, renderer), updateThread(callback) {
  forward::g_renderer = this;

  HDC windowDC;

  WNDCLASSEX wndclass = {sizeof(WNDCLASSEX), CS_DBLCLKS,
                         forward::WindowProcedure, 0, 0, GetModuleHandle(0),
                         LoadIcon(0, IDI_APPLICATION), LoadCursor(0, IDC_ARROW),
                         HBRUSH(COLOR_WINDOW + 1), 0, className,
                         LoadIcon(0, IDI_APPLICATION)};
  if (RegisterClassEx(&wndclass)) {
	  HWND window = 0;
	  {
    RECT displayRC = {0, 0, _WIDTH, _HEIGHT};
    // Get info on which monitor we want to use.
    
      std::vector<RECT> monitors;
      EnumDisplayMonitors(NULL, NULL, forward::MonitorEnumProc,
                          reinterpret_cast<DWORD>(&monitors));

      std::vector<RECT>::iterator i = monitors.begin();
      if (i != monitors.end())
        displayRC = *i;

      // Now we want to center the window in the display rect.
      int x = displayRC.left +
              (((displayRC.right - displayRC.left) / 2) - (_WW / 2));
      int y = displayRC.top +
              (((displayRC.bottom - displayRC.top) / 2) - (_WH / 2));

      displayRC.left = x;
      displayRC.top = y;
      displayRC.right = displayRC.left + _WW;
      displayRC.bottom = displayRC.top + _WH;
    
    window = CreateWindowEx(0, className, "Utility Renderer",
                                 WS_POPUPWINDOW, displayRC.left, displayRC.top,
                                 _WW, _WH, 0, 0, GetModuleHandle(0), 0);
  }
    if (window) {

      windowDC = GetWindowDC(window);
      HDC hImgDC = CreateCompatibleDC(windowDC);
      if (hImgDC == NULL) {
        MessageBox(NULL, "Dc is NULL", "ERROR!", MB_OK);
        return;
      }
      SetBkMode(hImgDC, TRANSPARENT);
      SetTextColor(hImgDC, RGB(255, 255, 255));
      SetStretchBltMode(hImgDC, COLORONCOLOR);

      BITMAPINFO bf;
      ZeroMemory(&bf, sizeof(BITMAPINFO));

      bf.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
      bf.bmiHeader.biWidth = _WIDTH;
      bf.bmiHeader.biHeight = _HEIGHT;
      bf.bmiHeader.biPlanes = 1;
      bf.bmiHeader.biBitCount = _BPP;
      bf.bmiHeader.biCompression = BI_RGB;
      bf.bmiHeader.biSizeImage = (_WIDTH * _HEIGHT * (_BPP / 8));
      bf.bmiHeader.biXPelsPerMeter = -1;
      bf.bmiHeader.biYPelsPerMeter = -1;

      unsigned char *bits;

      HBITMAP hImg = CreateDIBSection(hImgDC, &bf, DIB_RGB_COLORS,
                                      (void **)&bits, NULL, 0);
      if (hImg == NULL) {
        MessageBox(NULL, "Image is NULL", "ERROR!", MB_OK);
        return;
      } else if (hImg == INVALID_HANDLE_VALUE) {
        MessageBox(NULL, "Image is invalid", "Error!", MB_OK);
        return;
      }

      SelectObject(hImgDC, hImg);

      SetBuffer(bits, windowDC, hImgDC);

      ShowWindow(window, SW_SHOWDEFAULT);
      MSG msg;
      while (GetMessage(&msg, 0, 0, 0))
        DispatchMessage(&msg);
    }
  }
  SetRunning(false);
  forward::g_renderer = nullptr;
}

#endif // RENDERER_HPP_INCLUDED
