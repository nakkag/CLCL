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
	_GdipCreateBitmapFromHBITMAP = (pGdipCreateBitmapFromHBITMAP)GetProcAddress(hModuleThread, "GdipCreateBitmapFromHBITMAP");
	_GdipSaveImageToFile = (pGdipSaveImageToFile)GetProcAddress(hModuleThread, "GdipSaveImageToFile");
	_GdipDisposeImage = (pGdipDisposeImage)GetProcAddress(hModuleThread, "GdipDisposeImage");

	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, 0);
}

/*
 * shutdown_gdip - GDI+の終了処理
 */
void shutdown_gdip()
{
	GdiplusShutdown(gdiplusToken);
	FreeLibrary(hModuleThread);
}

/*
 * save_jpeg - HBITMAPをJPEGで保存
 */
int save_jpeg(HBITMAP hBmp, LPCWSTR lpszFilename, ULONG uQuality)
{
	GpBitmap* pBitmap = NULL;
	CLSID imageCLSID;
	Gdiplus::EncoderParameters encoderParams;

	if (_GdipCreateBitmapFromHBITMAP == NULL || _GdipSaveImageToFile == NULL || _GdipDisposeImage == NULL) {
		return 0;
	}
	_GdipCreateBitmapFromHBITMAP(hBmp, NULL, &pBitmap);
	CLSIDFromString(L"{557CF401-1A04-11D3-9A73-0000F81EF32E}", &imageCLSID);	//jpeg
	encoderParams.Count = 1;
	encoderParams.Parameter[0].NumberOfValues = 1;
	encoderParams.Parameter[0].Guid = Gdiplus::EncoderQuality;
	encoderParams.Parameter[0].Type = EncoderParameterValueTypeLong;
	encoderParams.Parameter[0].Value = &uQuality;
	_GdipSaveImageToFile(pBitmap, lpszFilename, &imageCLSID, &encoderParams);
	_GdipDisposeImage(pBitmap);
	return 1;
}

/*
 * save_png - HBITMAPをPNGで保存
 */
int save_png(HBITMAP hBmp, LPCWSTR lpszFilename)
{
	GpBitmap* pBitmap = NULL;
	CLSID imageCLSID;

	if (_GdipCreateBitmapFromHBITMAP == NULL || _GdipSaveImageToFile == NULL || _GdipDisposeImage == NULL) {
		return 0;
	}
	_GdipCreateBitmapFromHBITMAP(hBmp, NULL, &pBitmap);
	CLSIDFromString(L"{557CF406-1A04-11D3-9A73-0000F81EF32E}", &imageCLSID);	//png
	_GdipSaveImageToFile(pBitmap, lpszFilename, &imageCLSID, NULL);
	_GdipDisposeImage(pBitmap);
	return 1;
}

/*
 * draw_image_file - 画像ファイルをHBITMAPに変換
 */
HBITMAP image_to_bitmap(HDC hdc, LPCWSTR lpszFilename)
{
	Gdiplus::Image* image = new Gdiplus::Image(lpszFilename);

	HDC mdc = ::CreateCompatibleDC(hdc);
	HBITMAP hbmp = ::CreateCompatibleBitmap(hdc, image->GetWidth(), image->GetHeight());
	HBITMAP hOldBmp = (HBITMAP)::SelectObject(mdc, hbmp);

	Gdiplus::Graphics* graphics = new Gdiplus::Graphics(mdc);
	graphics->DrawImage(image, 0, 0, image->GetWidth(), image->GetHeight());
	delete(image);
	delete(graphics);

	::SelectObject(mdc, hOldBmp);
	::DeleteDC(mdc);
	return hbmp;
}
/* End of source */
