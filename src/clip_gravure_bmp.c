/*
  clip_gravure_bmp.c

  >mingw32-make -f makefile.tdmgcc64
*/

#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <time.h>

#define FN_GRAVURE "\\tmp\\________________gravure_%s.bmp"
#define LEN_TS 16

#define OFFBITS (sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER))

char *makeTimestamp()
{
  static char ts[LEN_TS];
  time_t tt;
  struct tm *t;
  time(&tt);
  t = localtime(&tt);
  sprintf(ts, "%04d%02d%02d_%02d%02d%02d",
    t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
    t->tm_hour, t->tm_min, t->tm_sec);
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
  OpenClipboard(hwnd);
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
  }else{
    fprintf(stderr, "no bitmap in clipboard\n");
  }
  CloseClipboard();
  return 0;
}
