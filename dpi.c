/*
 * CLCL
 *
 * dpi.c
  *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
*/

 /* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE

#include "dpi.h"

/* Define */
#ifndef MONITOR_DEFAULTTONEAREST
#define MONITOR_DEFAULTTONEAREST    0x00000002
#endif

/* Global Variables */
static HMODULE hModThemes;

static UINT m_nScaleFactor = 0;
static UINT m_nScaleFactorSDA = 0;
static PROCESS_DPI_AWARENESS m_Awareness = PROCESS_DPI_UNAWARE;

/* Local Function Prototypes */
static FARPROC _GetProcessDpiAwareness;
static FARPROC _SetProcessDpiAwareness;
static FARPROC _GetDpiForMonitor;


/*
 * Scale - スケール変換した値の取得
 */
int Scale(int x)
{
	if (m_Awareness == PROCESS_DPI_UNAWARE)
	{
		return x;
	}
	if (m_Awareness == PROCESS_SYSTEM_DPI_AWARE)
	{
		return MulDiv(x, m_nScaleFactorSDA, 100);
	}
	return MulDiv(x, m_nScaleFactor, 100);
}

/*
 * UnScale - スケールを戻した値の取得
 */
int UnScale(int x)
{
	if (m_Awareness == PROCESS_DPI_UNAWARE)
	{
		return x;
	}
	if (m_Awareness == PROCESS_SYSTEM_DPI_AWARE)
	{
		return MulDiv(x, 100, m_nScaleFactorSDA);
	}
	return MulDiv(x, 100, m_nScaleFactor);
}

/*
 * GetScale - スケールの取得
 */
UINT GetScale()
{
	if (m_Awareness == PROCESS_DPI_UNAWARE)
	{
		return 100;
	}
	if (m_Awareness == PROCESS_SYSTEM_DPI_AWARE)
	{
		return m_nScaleFactorSDA;
	}
	return m_nScaleFactor;
}

/*
 * SetScale - スケールの設定
 */
void SetScale(UINT iDPI)
{
	m_nScaleFactor = MulDiv(iDPI, 100, 96);
	if (m_nScaleFactorSDA == 0)
	{
		m_nScaleFactorSDA = m_nScaleFactor;
	}
}

/*
 * GetAwareness - Awarenessの取得
 */
PROCESS_DPI_AWARENESS GetAwareness()
{
	if (_GetProcessDpiAwareness == NULL) {
		return m_Awareness;
	}
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
	_GetProcessDpiAwareness(hProcess, &m_Awareness);
	return m_Awareness;
}

/*
 * SetAwareness - Awarenessの設定
 */
void SetAwareness(PROCESS_DPI_AWARENESS awareness)
{
	if (_SetProcessDpiAwareness == NULL) {
		return;
	}
	HRESULT hr = _SetProcessDpiAwareness(awareness);
	if (hr == S_OK)
	{
		m_Awareness = awareness;
	}
}

/*
 * ScaleRect - RECTのスケール変換
 */
void ScaleRect(RECT* pRect)
{
	pRect->left = Scale(pRect->left);
	pRect->right = Scale(pRect->right);
	pRect->top = Scale(pRect->top);
	pRect->bottom = Scale(pRect->bottom);
}

/*
 * UnScaleRect - RECTのスケール変換
 */
void UnScaleRect(RECT* pRect)
{
	pRect->left = UnScale(pRect->left);
	pRect->right = UnScale(pRect->right);
	pRect->top = UnScale(pRect->top);
	pRect->bottom = UnScale(pRect->bottom);
}

/*
 * ScalePoint - POINTのスケール変換
 */
void ScalePoint(POINT* pPoint)
{
	pPoint->x = Scale(pPoint->x);
	pPoint->y = Scale(pPoint->y);
}

/*
 * UnScalePoint - POINTのスケール変換
 */
void UnScalePoint(POINT* pPoint)
{
	pPoint->x = UnScale(pPoint->x);
	pPoint->y = UnScale(pPoint->y);
}

/*
 * InitDpi - DPIの初期化
 */
void InitDpi()
{
	if ((hModThemes = LoadLibrary(TEXT("shcore.dll"))) == NULL) {
		return;
	}
	if ((_GetDpiForMonitor = GetProcAddress(hModThemes, "GetDpiForMonitor")) == NULL) {
		return;
	}
	if ((_GetProcessDpiAwareness = GetProcAddress(hModThemes, "GetProcessDpiAwareness")) == NULL) {
		return;
	}
	if ((_SetProcessDpiAwareness = GetProcAddress(hModThemes, "SetProcessDpiAwareness")) == NULL) {
		return;
	}

	// DPIの初期化
	GetAwareness();
	UINT dpix = 0, dpiy = 0;
	POINT pt = { 1, 1 };
	HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
	HRESULT hr = _GetDpiForMonitor(hMonitor, 0, &dpix, &dpiy);
	SetScale(dpix);
}
/* End of source */
