/*
  clip_gravure_bmp.c

  >mingw32-make -f makefile.tdmgcc64

  Usage:

    >clip_gravure_bmp
      save bitmap from clipboard (abort when clipboard data is not bitmap)

    >clip_gravure_bmp [o]
      same as above, but capture full screen when clipboard data is not bitmap
*/

#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#if defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64)
#include <time.h>
#else
#include <time.h>
#include <sys/time.h>
#endif

#define FN_GRAVURE "\\tmp\\________________gravure_%s.bmp"
#define LEN_TS 32

#define OFFBITS (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER))

char *makeTimestamp()
{
  static char ts[LEN_TS];
#if defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64)
  SYSTEMTIME st;
  // GetSystemTime(&st); // UTC
  GetLocalTime(&st); // tz=default
  sprintf(ts, "%04d%02d%02d_%02d%02d%02d.%03d000",
    st.wYear, st.wMonth, st.wDay,
    st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
#else
  time_t tt;
  struct timeval tv;
  struct tm *t;
  gettimeofday(&tv, NULL); // NULL means tz=default
  tt = tv.tv_sec; // *** CAUTION ***
  t = localtime(&tt);
  sprintf(ts, "%04d%02d%02d_%02d%02d%02d.%06d",
    t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
    t->tm_hour, t->tm_min, t->tm_sec, tv.tv_usec);
#endif
  return ts;
}

int saveBMP(HBITMAP hbmp, int c, int r, HDC hdc)
{
  size_t wlen, sz;
  LPBYTE p;
  FILE *fp;
  char fn[MAX_PATH];
  sprintf(fn, FN_GRAVURE, makeTimestamp());
  if(!(fp = fopen(fn, "wb"))){
    fprintf(stderr, "cannot create: %s\n", fn);
    return 1;
  }
  wlen = c * 3;
  if(wlen % 4) wlen += 4 - wlen % 4;
  sz = OFFBITS + r * wlen;
  if(p = (LPBYTE)malloc(sz)){
    LPBITMAPFILEHEADER bh = (LPBITMAPFILEHEADER)p;
    LPBITMAPINFOHEADER bi = (LPBITMAPINFOHEADER)(p + sizeof(BITMAPFILEHEADER));
    LPBYTE pixels = p + OFFBITS;
    memset(bh, 0, sizeof(BITMAPFILEHEADER));
    memset(bi, 0, sizeof(BITMAPINFOHEADER));
    bh->bfType = ('M' << 8) | 'B';
    bh->bfSize = sz;
    bh->bfOffBits = OFFBITS;
    bi->biSize = sizeof(BITMAPINFOHEADER);
    bi->biWidth = c;
    bi->biHeight = r;
    bi->biPlanes = 1; // not be 3
    bi->biBitCount = 24; // not be 8
    bi->biCompression = BI_RGB;
    GetDIBits(hdc, hbmp, 0, r, pixels, (LPBITMAPINFO)bi, DIB_RGB_COLORS);
    fwrite(p, sz, 1, fp);
    fprintf(stdout, "saved to %s\n", fn);
    free(p);
  }
  fclose(fp);
  return 0;
}

int main(int ac, char **av)
{
  HWND hwnd = GetDesktopWindow();
  if(!OpenClipboard(hwnd)){
    fprintf(stderr, "cannot open clipboard\n");
    return 1;
  }
  if(!IsClipboardFormatAvailable(CF_BITMAP)){
    if(ac >= 2){
      HBITMAP hbmp, obmp;
      int w, h;
      HDC hsdc = CreateDCW(L"DISPLAY", NULL, NULL, NULL);
      HDC hmdc = CreateCompatibleDC(hsdc);
      RECT rc;
      GetClientRect(hwnd, &rc);
      w = rc.right - rc.left;
      h = rc.bottom - rc.top;
      hbmp = CreateCompatibleBitmap(hsdc, w, h);
      obmp = SelectObject(hmdc, hbmp);
      BitBlt(hmdc, 0, 0, w, h, hsdc, 0, 0, SRCCOPY);
      SelectObject(hmdc, obmp);
      EmptyClipboard();
      SetClipboardData(CF_BITMAP, hbmp);
      DeleteDC(hmdc);
      DeleteDC(hsdc);
    }else{
      fprintf(stderr, "no bitmap in clipboard\n");
      CloseClipboard();
      return 2;
    }
  }
  if(IsClipboardFormatAvailable(CF_BITMAP)){
    HBITMAP hbmp = (HBITMAP)GetClipboardData(CF_BITMAP);
    HDC hdc = GetDC(hwnd);
    HDC hmdc = CreateCompatibleDC(hdc);
    HBITMAP obmp = SelectObject(hmdc, hbmp);
    BITMAP bm;
    GetObject(hbmp, sizeof(BITMAP), (LPSTR)&bm);
    BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hmdc, 0, 0, SRCCOPY);
    SelectObject(hmdc, obmp);
    saveBMP(hbmp, bm.bmWidth, bm.bmHeight, hmdc);
    // must not delete hbmp and bm
    DeleteDC(hmdc);
    ReleaseDC(hwnd, hdc);
    CloseClipboard();
  }else{
    fprintf(stderr, "cannot gravure to clipboard\n");
    CloseClipboard();
    return 3;
  }
  return 0;
}
