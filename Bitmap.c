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
#include "Bitmap.h"

/* Define */
#ifndef BI_ALPHABITFIELDS
#define BI_ALPHABITFIELDS				6
#endif

/* Global Variables */

/* Local Function Prototypes */

/*
 * dib_stride - 1行のバイト数(4バイト境界)を取得
 */
static DWORD dib_stride(const LONG width, const WORD bit_count)
{
	return (DWORD)((((ULONGLONG)width * bit_count + 31) / 32) * 4);
}

/*
 * dib_get_bits_offset - DIBヘッダを検証して画素データまでのオフセットを取得 (不正なら 0)
 */
DWORD dib_get_bits_offset(const BYTE *dib, const DWORD size)
{
	const BITMAPINFOHEADER *bih = (const BITMAPINFOHEADER *)dib;
	ULONGLONG offset;
	ULONGLONG image_size;
	DWORD color_cnt;
	DWORD mask_cnt = 0;
	LONG height;

	if (dib == NULL || size < sizeof(BITMAPINFOHEADER)) {
		return 0;
	}
	if (bih->biSize < sizeof(BITMAPINFOHEADER) || bih->biSize > size) {
		return 0;
	}
	if (bih->biWidth <= 0 || bih->biHeight == 0 || bih->biPlanes != 1) {
		return 0;
	}
	switch (bih->biBitCount) {
	case 1:
	case 4:
	case 8:
	case 16:
	case 24:
	case 32:
		break;
	default:
		return 0;
	}
	// カラーテーブル
	if (bih->biBitCount <= 8) {
		if (bih->biClrUsed > (1UL << bih->biBitCount)) {
			return 0;
		}
		color_cnt = (bih->biClrUsed == 0) ? (1UL << bih->biBitCount) : bih->biClrUsed;
	} else {
		if (bih->biClrUsed > 256) {
			return 0;
		}
		color_cnt = bih->biClrUsed;
	}
	// カラーマスク (BITMAPINFOHEADER の場合のみヘッダの後ろに続く)
	if (bih->biSize == sizeof(BITMAPINFOHEADER)) {
		if (bih->biCompression == BI_BITFIELDS) {
			mask_cnt = 3;
		} else if (bih->biCompression == BI_ALPHABITFIELDS) {
			mask_cnt = 4;
		}
	}
	offset = (ULONGLONG)bih->biSize + (ULONGLONG)color_cnt * sizeof(RGBQUAD) + (ULONGLONG)mask_cnt * sizeof(DWORD);
	if (offset > size) {
		return 0;
	}
	// 画素データのサイズ
	height = (bih->biHeight < 0) ? -bih->biHeight : bih->biHeight;
	switch (bih->biCompression) {
	case BI_RGB:
	case BI_BITFIELDS:
	case BI_ALPHABITFIELDS:
		image_size = (ULONGLONG)dib_stride(bih->biWidth, bih->biBitCount) * (ULONGLONG)height;
		break;
	default:
		// RLE / JPEG / PNG
		image_size = bih->biSizeImage;
		break;
	}
	if (image_size == 0 || offset + image_size > size) {
		return 0;
	}
	return (DWORD)offset;
}

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
	ULONGLONG total;
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
	case 24:
		break;
	default:
		SetLastError(ERROR_INVALID_PARAMETER);
		return NULL;
	}
	if (bmp.bmWidth <= 0 || bmp.bmHeight <= 0) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return NULL;
	}
	len = dib_stride(bmp.bmWidth, bmp.bmBitsPixel);

	hsize = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * color_bit;
	total = (ULONGLONG)hsize + (ULONGLONG)len * (ULONGLONG)bmp.bmHeight;
	if (total > 0x7FFFFFFF) {
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return NULL;
	}

	if ((hdc = GetDC(NULL)) == NULL) {
		return NULL;
	}

	if ((ret = mem_calloc((DWORD)total)) == NULL) {
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

	if (size != NULL) {
		*size = (DWORD)total;
	}
	return ret;
}

/*
 * dib_to_bitmap - DIBをビットマップに変換
 */
HBITMAP dib_to_bitmap(const BYTE *dib, const DWORD size)
{
	HDC hdc;
	HBITMAP ret;
	DWORD offset;
	DWORD err;

	// ヘッダ検証
	if ((offset = dib_get_bits_offset(dib, size)) == 0) {
		SetLastError(ERROR_INVALID_DATA);
		return NULL;
	}

	if ((hdc = GetDC(NULL)) == NULL) {
		return NULL;
	}

	// ビットマップに変換
	ret = CreateDIBitmap(hdc, (const BITMAPINFOHEADER *)dib, CBM_INIT, dib + offset, (const BITMAPINFO *)dib, DIB_RGB_COLORS);
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
