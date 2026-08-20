/*
 * CLCL
 *
 * dpi.h
  *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
*/

#pragma once

 /* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE

/* Define */
#ifndef USER_DEFAULT_SCREEN_DPI
#define USER_DEFAULT_SCREEN_DPI			96
#endif

#ifndef WM_DPICHANGED
#define WM_DPICHANGED					0x02E0
#endif
// 子ウィンドウへのDPI変更通知
#ifndef WM_DPICHANGED_BEFOREPARENT
#define WM_DPICHANGED_BEFOREPARENT		0x02E2
#endif
#ifndef WM_DPICHANGED_AFTERPARENT
#define WM_DPICHANGED_AFTERPARENT		0x02E3
#endif

/* Struct */
typedef enum PROCESS_DPI_AWARENESS {
	PROCESS_DPI_UNAWARE = 0,
	PROCESS_SYSTEM_DPI_AWARE = 1,
	PROCESS_PER_MONITOR_DPI_AWARE = 2
} PROCESS_DPI_AWARENESS;

/* Function Prototypes */
void InitDpi();

int Scale(int x);
int UnScale(int x);

UINT GetDpi();
void SetDpi(const UINT dpi);
UINT GetScale();
void SetScale(UINT iDPI);

UINT GetWindowDpi(const HWND hWnd);
UINT GetPointDpi(const POINT pt);
UINT SetDpiFromWindow(const HWND hWnd);
UINT SetDpiFromPoint(const POINT pt);

PROCESS_DPI_AWARENESS GetAwareness();
void SetAwareness(PROCESS_DPI_AWARENESS awareness);

int GetSystemMetricsDpi(const int nIndex);
BOOL GetNonClientMetricsDpi(NONCLIENTMETRICS *ncm);
BOOL GetMonitorRectFromPoint(const POINT pt, RECT *pRect);

void ScaleRect(RECT* pRect);
void UnScaleRect(RECT* pRect);
void ScalePoint(POINT* pPoint);
void UnScalePoint(POINT* pPoint);
/* End of source */
