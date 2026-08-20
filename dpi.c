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
#ifndef MONITOR_DEFAULTTOPRIMARY
#define MONITOR_DEFAULTTOPRIMARY    0x00000001
#endif
#ifndef MONITOR_DEFAULTTONEAREST
#define MONITOR_DEFAULTTONEAREST    0x00000002
#endif

// GetDpiForMonitor の MONITOR_DPI_TYPE
#define MDT_EFFECTIVE_DPI			0

// DPIの範囲
#define DPI_MIN						48
#define DPI_MAX						960

/* Struct */
typedef HRESULT (WINAPI *GETDPIFORMONITOR)(HMONITOR, int, UINT *, UINT *);
typedef HRESULT (WINAPI *GETPROCESSDPIAWARENESS)(HANDLE, PROCESS_DPI_AWARENESS *);
typedef HRESULT (WINAPI *SETPROCESSDPIAWARENESS)(PROCESS_DPI_AWARENESS);
typedef UINT (WINAPI *GETDPIFORWINDOW)(HWND);
typedef UINT (WINAPI *GETDPIFORSYSTEM)(void);
typedef int (WINAPI *GETSYSTEMMETRICSFORDPI)(int, UINT);
typedef BOOL (WINAPI *SYSTEMPARAMETERSINFOFORDPI)(UINT, UINT, PVOID, UINT, UINT);

/* Global Variables */
static HMODULE hModShcore;

// 現在の描画対象のDPI
static UINT m_nDpi = USER_DEFAULT_SCREEN_DPI;
static PROCESS_DPI_AWARENESS m_Awareness = PROCESS_SYSTEM_DPI_AWARE;

/* Local Function Prototypes */
static GETDPIFORMONITOR _GetDpiForMonitor;
static GETPROCESSDPIAWARENESS _GetProcessDpiAwareness;
static SETPROCESSDPIAWARENESS _SetProcessDpiAwareness;
static GETDPIFORWINDOW _GetDpiForWindow;
static GETDPIFORSYSTEM _GetDpiForSystem;
static GETSYSTEMMETRICSFORDPI _GetSystemMetricsForDpi;
static SYSTEMPARAMETERSINFOFORDPI _SystemParametersInfoForDpi;

static UINT get_monitor_dpi(const HMONITOR hMonitor);


/*
 * get_monitor_dpi - モニタのDPIの取得
 */
static UINT get_monitor_dpi(const HMONITOR hMonitor)
{
	UINT dpix = 0, dpiy = 0;

	if (hMonitor == NULL || _GetDpiForMonitor == NULL) {
		return 0;
	}
	if (_GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpix, &dpiy) != S_OK) {
		return 0;
	}
	return dpix;
}

/*
 * InitDpi - DPIの初期化
 */
void InitDpi()
{
	HMODULE hModUser32;
	UINT dpi = 0;

	// user32.dllのAPI
	hModUser32 = GetModuleHandle(TEXT("user32.dll"));
	if (hModUser32 != NULL) {
		_GetDpiForWindow = (GETDPIFORWINDOW)GetProcAddress(hModUser32, "GetDpiForWindow");
		_GetDpiForSystem = (GETDPIFORSYSTEM)GetProcAddress(hModUser32, "GetDpiForSystem");
		_GetSystemMetricsForDpi = (GETSYSTEMMETRICSFORDPI)GetProcAddress(hModUser32, "GetSystemMetricsForDpi");
		_SystemParametersInfoForDpi = (SYSTEMPARAMETERSINFOFORDPI)GetProcAddress(hModUser32, "SystemParametersInfoForDpi");
	}
	// shcore.dllのAPI
	if ((hModShcore = LoadLibrary(TEXT("shcore.dll"))) != NULL) {
		_GetDpiForMonitor = (GETDPIFORMONITOR)GetProcAddress(hModShcore, "GetDpiForMonitor");
		_GetProcessDpiAwareness = (GETPROCESSDPIAWARENESS)GetProcAddress(hModShcore, "GetProcessDpiAwareness");
		_SetProcessDpiAwareness = (SETPROCESSDPIAWARENESS)GetProcAddress(hModShcore, "SetProcessDpiAwareness");
	}

	// DPI対応レベルの取得
	GetAwareness();

	// プライマリモニタのDPIを取得
	dpi = get_monitor_dpi(MonitorFromWindow(NULL, MONITOR_DEFAULTTOPRIMARY));
	if (dpi == 0 && _GetDpiForSystem != NULL) {
		dpi = _GetDpiForSystem();
	}
	if (dpi == 0) {
		HDC hdc = GetDC(NULL);
		if (hdc != NULL) {
			dpi = GetDeviceCaps(hdc, LOGPIXELSX);
			ReleaseDC(NULL, hdc);
		}
	}
	SetDpi(dpi);
}

/*
 * Scale - スケール変換した値の取得
 */
int Scale(int x)
{
	if (m_nDpi == USER_DEFAULT_SCREEN_DPI) {
		return x;
	}
	return MulDiv(x, m_nDpi, USER_DEFAULT_SCREEN_DPI);
}

/*
 * UnScale - スケールを戻した値の取得
 */
int UnScale(int x)
{
	if (m_nDpi == USER_DEFAULT_SCREEN_DPI) {
		return x;
	}
	return MulDiv(x, USER_DEFAULT_SCREEN_DPI, m_nDpi);
}

/*
 * GetDpi - 現在のDPIの取得
 */
UINT GetDpi()
{
	return m_nDpi;
}

/*
 * SetDpi - 現在のDPIの設定
 */
void SetDpi(const UINT dpi)
{
	if (m_Awareness == PROCESS_DPI_UNAWARE) {
		m_nDpi = USER_DEFAULT_SCREEN_DPI;
		return;
	}
	if (dpi < DPI_MIN || dpi > DPI_MAX) {
		return;
	}
	m_nDpi = dpi;
}

/*
 * GetScale - スケールの取得
 */
UINT GetScale()
{
	return MulDiv(m_nDpi, 100, USER_DEFAULT_SCREEN_DPI);
}

/*
 * SetScale - スケールの設定
 */
void SetScale(UINT iDPI)
{
	SetDpi(iDPI);
}

/*
 * GetWindowDpi - ウィンドウのDPIの取得
 */
UINT GetWindowDpi(const HWND hWnd)
{
	if (hWnd == NULL) {
		return 0;
	}
	if (_GetDpiForWindow != NULL) {
		UINT dpi = _GetDpiForWindow(hWnd);
		if (dpi != 0) {
			return dpi;
		}
	}
	return get_monitor_dpi(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST));
}

/*
 * GetPointDpi - 座標が含まれるモニタのDPIの取得
 */
UINT GetPointDpi(const POINT pt)
{
	return get_monitor_dpi(MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST));
}

/*
 * SetDpiFromWindow - ウィンドウのDPIを現在のDPIに設定
 */
UINT SetDpiFromWindow(const HWND hWnd)
{
	SetDpi(GetWindowDpi(hWnd));
	return m_nDpi;
}

/*
 * SetDpiFromPoint - 座標が含まれるモニタのDPIを現在のDPIに設定
 */
UINT SetDpiFromPoint(const POINT pt)
{
	SetDpi(GetPointDpi(pt));
	return m_nDpi;
}

/*
 * GetAwareness - Awarenessの取得
 */
PROCESS_DPI_AWARENESS GetAwareness()
{
	PROCESS_DPI_AWARENESS awareness;

	if (_GetProcessDpiAwareness == NULL) {
		return m_Awareness;
	}
	if (_GetProcessDpiAwareness(NULL, &awareness) == S_OK) {
		m_Awareness = awareness;
	}
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
	if (_SetProcessDpiAwareness(awareness) == S_OK) {
		m_Awareness = awareness;
	}
}

/*
 * GetSystemMetricsDpi - 現在のDPIでのシステムメトリックの取得
 */
int GetSystemMetricsDpi(const int nIndex)
{
	if (_GetSystemMetricsForDpi != NULL) {
		return _GetSystemMetricsForDpi(nIndex, m_nDpi);
	}
	return GetSystemMetrics(nIndex);
}

/*
 * GetNonClientMetricsDpi - 現在のDPIでの非クライアント領域のメトリックの取得
 */
BOOL GetNonClientMetricsDpi(NONCLIENTMETRICS *ncm)
{
	if (ncm == NULL) {
		return FALSE;
	}
	ncm->cbSize = sizeof(NONCLIENTMETRICS);
	if (_SystemParametersInfoForDpi != NULL &&
		_SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), ncm, 0, m_nDpi) != FALSE) {
		return TRUE;
	}
	return SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), ncm, 0);
}

/*
 * GetMonitorRectFromPoint - 座標が含まれるモニタの矩形の取得
 */
BOOL GetMonitorRectFromPoint(const POINT pt, RECT *pRect)
{
	MONITORINFO mi;
	HMONITOR hMonitor;

	if (pRect == NULL) {
		return FALSE;
	}
	hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
	mi.cbSize = sizeof(MONITORINFO);
	if (hMonitor == NULL || GetMonitorInfo(hMonitor, &mi) == FALSE) {
		SetRect(pRect, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
		return FALSE;
	}
	*pRect = mi.rcMonitor;
	return TRUE;
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
/* End of source */
