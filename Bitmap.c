/*
 * CLCL
 *
 * Bitmap.c
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE

#include "General.h"
#include "Memory.h"

/* Define */

/* Global Variables */

/* Local Function Prototypes */

/*
 * bitmap_to_dib - ビットマップをDIBに変換
 */
BYTE *bitmap_to_dib(const HBITMAP hbmp, DWORD *size)
{
	HDC hdc;
	BITMAP bmp;
	BYTE *ret;		// BITMAPINFO
	DWORD biComp = BI_RGB;
	DWORD len;
	DWORD hsize;
	DWORD err;
	int color_bit = 0;

	// BITMAP情報取得
	if (GetObject(hbmp, sizeof(BITMAP), &bmp) == 0) {
		return NULL;
	}
	switch (bmp.bmBitsPixel) {
	case 1:
		color_bit = 2;
		break;
	case 4:
		color_bit = 16;
		break;
	case 8:
		color_bit = 256;
		break;
	case 16:
	case 32:
		color_bit = 3;
		biComp = BI_BITFIELDS;
		break;
	}
	len = (((bmp.bmWidth * bmp.bmBitsPixel) / 8) % 4)
		? ((((bmp.bmWidth * bmp.bmBitsPixel) / 8) / 4) + 1) * 4
		: (bmp.bmWidth * bmp.bmBitsPixel) / 8;

	if ((hdc = GetDC(NULL)) == NULL) {
		return NULL;
	}

	hsize = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * color_bit;
	if ((ret = mem_calloc(hsize + len * bmp.bmHeight)) == NULL) {
		err = GetLastError();
		ReleaseDC(NULL, hdc);
		SetLastError(err);
		return NULL;
	}
	// DIBヘッダ
	((BITMAPINFO *)ret)->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	((BITMAPINFO *)ret)->bmiHeader.biWidth = bmp.bmWidth;
	((BITMAPINFO *)ret)->bmiHeader.biHeight = bmp.bmHeight;
	((BITMAPINFO *)ret)->bmiHeader.biPlanes = 1;
	((BITMAPINFO *)ret)->bmiHeader.biBitCount = bmp.bmBitsPixel;
	((BITMAPINFO *)ret)->bmiHeader.biCompression = biComp;
	((BITMAPINFO *)ret)->bmiHeader.biSizeImage = len * bmp.bmHeight;
	((BITMAPINFO *)ret)->bmiHeader.biClrImportant = 0;

	// DIB取得
	if (GetDIBits(hdc, hbmp, 0, bmp.bmHeight, ret + hsize, (BITMAPINFO *)ret, DIB_RGB_COLORS) == 0) {
		err = GetLastError();
		ReleaseDC(NULL, hdc);
		mem_free(&ret);
		SetLastError(err);
		return NULL;
	}
	ReleaseDC(NULL, hdc);
	*size = hsize + len * bmp.bmHeight;
	return ret;
}

/*
 * dib_to_bitmap - DIBをビットマップに変換
 */
HBITMAP dib_to_bitmap(const BYTE *dib)
{
	HDC hdc;
	HBITMAP ret;
	DWORD hsize;
	DWORD err;
	int color_bit;

	if ((hdc = GetDC(NULL)) == NULL) {
		return NULL;
	}
	
	// ヘッダサイズ取得
	if ((color_bit = ((BITMAPINFOHEADER *)dib)->biClrUsed) == 0) {
		color_bit = ((BITMAPINFOHEADER *)dib)->biPlanes * ((BITMAPINFOHEADER *)dib)->biBitCount;
		if (color_bit == 1) {
			color_bit = 2;
		} else if (color_bit <= 4) {
			color_bit = 16;
		} else if (color_bit <= 8) {
			color_bit = 256;
		} else if (color_bit <= 16) {
			color_bit = 3;
		} else if (color_bit <= 24) {
			color_bit = 0;
		} else {
			color_bit = 3;
		}
	}
	hsize = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * color_bit;

	// ビットマップに変換
	ret = CreateDIBitmap(hdc, (BITMAPINFOHEADER *)dib, CBM_INIT, dib + hsize, (BITMAPINFO *)dib, DIB_RGB_COLORS);
	if (ret == NULL) {
		err = GetLastError();
		ReleaseDC(NULL, hdc);
		SetLastError(err);
		return NULL;
	}
	ReleaseDC(NULL, hdc);
	return ret;
}
/* End of source */
