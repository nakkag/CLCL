/*
 * CLCL
 *
 * gdip.cpp
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

 /* Include Files */

#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE
#include <vector>
#include <gdiplus.h>

using namespace Gdiplus;

#include "gdip.h"

extern "C" {
#include "dpi.h"
}

#pragma comment(lib, "gdiplus.lib")

/* Define */

/* Global Variables */
static HMODULE hModuleThread;
static ULONG_PTR gdiplusToken;

typedef Status(WINAPI* pGdipCreateBitmapFromHBITMAP)(HBITMAP, HPALETTE, GpBitmap**);
typedef Status(WINAPI* pGdipSaveImageToFile)(GpBitmap*, const WCHAR*, const CLSID*, const Gdiplus::EncoderParameters*);
typedef Status(WINAPI* pGdipDisposeImage)(GpBitmap*);

static pGdipCreateBitmapFromHBITMAP _GdipCreateBitmapFromHBITMAP;
static pGdipSaveImageToFile _GdipSaveImageToFile;
static pGdipDisposeImage _GdipDisposeImage;

/* Local Function Prototypes */

/*
 * init_gdip - GDI+の初期化
 */
void init_gdip()
{
	hModuleThread = LoadLibrary(TEXT("gdiplus.dll"));
	if (hModuleThread != NULL) {
		_GdipCreateBitmapFromHBITMAP = (pGdipCreateBitmapFromHBITMAP)GetProcAddress(hModuleThread, "GdipCreateBitmapFromHBITMAP");
		_GdipSaveImageToFile = (pGdipSaveImageToFile)GetProcAddress(hModuleThread, "GdipSaveImageToFile");
		_GdipDisposeImage = (pGdipDisposeImage)GetProcAddress(hModuleThread, "GdipDisposeImage");
	}

	GdiplusStartupInput gdiplusStartupInput;
	if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, 0) != Ok) {
		gdiplusToken = 0;
	}
}

/*
 * shutdown_gdip - GDI+の終了処理
 */
void shutdown_gdip()
{
	if (gdiplusToken != 0) {
		GdiplusShutdown(gdiplusToken);
		gdiplusToken = 0;
	}
	if (hModuleThread != NULL) {
		FreeLibrary(hModuleThread);
		hModuleThread = NULL;
	}
	_GdipCreateBitmapFromHBITMAP = NULL;
	_GdipSaveImageToFile = NULL;
	_GdipDisposeImage = NULL;
}

/*
 * save_jpeg - HBITMAPをJPEGで保存
 */
int save_jpeg(HBITMAP hBmp, LPCWSTR lpszFilename, ULONG uQuality)
{
	GpBitmap* pBitmap = NULL;
	CLSID imageCLSID;
	Gdiplus::EncoderParameters encoderParams;
	Status status;

	if (_GdipCreateBitmapFromHBITMAP == NULL || _GdipSaveImageToFile == NULL || _GdipDisposeImage == NULL ||
		hBmp == NULL || lpszFilename == NULL) {
		return 0;
	}
	if (_GdipCreateBitmapFromHBITMAP(hBmp, NULL, &pBitmap) != Ok || pBitmap == NULL) {
		return 0;
	}
	if (CLSIDFromString(L"{557CF401-1A04-11D3-9A73-0000F81EF32E}", &imageCLSID) != NOERROR) {	//jpeg
		_GdipDisposeImage(pBitmap);
		return 0;
	}
	encoderParams.Count = 1;
	encoderParams.Parameter[0].NumberOfValues = 1;
	encoderParams.Parameter[0].Guid = Gdiplus::EncoderQuality;
	encoderParams.Parameter[0].Type = EncoderParameterValueTypeLong;
	encoderParams.Parameter[0].Value = &uQuality;
	status = _GdipSaveImageToFile(pBitmap, lpszFilename, &imageCLSID, &encoderParams);
	_GdipDisposeImage(pBitmap);
	return (status == Ok) ? 1 : 0;
}

/*
 * save_png - HBITMAPをPNGで保存
 */
int save_png(HBITMAP hBmp, LPCWSTR lpszFilename)
{
	GpBitmap* pBitmap = NULL;
	CLSID imageCLSID;
	Status status;

	if (_GdipCreateBitmapFromHBITMAP == NULL || _GdipSaveImageToFile == NULL || _GdipDisposeImage == NULL ||
		hBmp == NULL || lpszFilename == NULL) {
		return 0;
	}
	if (_GdipCreateBitmapFromHBITMAP(hBmp, NULL, &pBitmap) != Ok || pBitmap == NULL) {
		return 0;
	}
	if (CLSIDFromString(L"{557CF406-1A04-11D3-9A73-0000F81EF32E}", &imageCLSID) != NOERROR) {	//png
		_GdipDisposeImage(pBitmap);
		return 0;
	}
	status = _GdipSaveImageToFile(pBitmap, lpszFilename, &imageCLSID, NULL);
	_GdipDisposeImage(pBitmap);
	return (status == Ok) ? 1 : 0;
}

/*
 * image_to_bitmap - 画像ファイルをHBITMAPに変換
 *   hdc が NULL の場合は画面のDCを内部で取得・解放する
 */
HBITMAP image_to_bitmap(HDC hdc, LPCWSTR lpszFilename)
{
	Gdiplus::Image* image;
	HDC ref_dc = hdc;
	HDC mdc;
	HBITMAP hbmp;
	HBITMAP hOldBmp;
	UINT width, height;

	if (lpszFilename == NULL || gdiplusToken == 0) {
		return NULL;
	}
	image = new Gdiplus::Image(lpszFilename);
	if (image == NULL) {
		return NULL;
	}
	width = image->GetWidth();
	height = image->GetHeight();
	if (image->GetLastStatus() != Ok || width == 0 || height == 0) {
		delete image;
		return NULL;
	}

	if (ref_dc == NULL && (ref_dc = ::GetDC(NULL)) == NULL) {
		delete image;
		return NULL;
	}
	mdc = ::CreateCompatibleDC(ref_dc);
	hbmp = ::CreateCompatibleBitmap(ref_dc, width, height);
	if (hdc == NULL) {
		::ReleaseDC(NULL, ref_dc);
	}
	if (mdc == NULL || hbmp == NULL) {
		if (mdc != NULL) {
			::DeleteDC(mdc);
		}
		if (hbmp != NULL) {
			::DeleteObject(hbmp);
		}
		delete image;
		return NULL;
	}
	hOldBmp = (HBITMAP)::SelectObject(mdc, hbmp);
	{
		Gdiplus::Graphics graphics(mdc);
		if (graphics.GetLastStatus() != Ok ||
			graphics.DrawImage(image, 0, 0, (INT)width, (INT)height) != Ok) {
			::SelectObject(mdc, hOldBmp);
			::DeleteDC(mdc);
			::DeleteObject(hbmp);
			delete image;
			return NULL;
		}
	}
	delete image;

	::SelectObject(mdc, hOldBmp);
	::DeleteDC(mdc);
	return hbmp;
}
/* End of source */
