/*
 * CLCL
 *
 * ToolTip.c
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
#include "Ini.h"
#include "Font.h"

#include "resource.h"

/* Define */
#define WINDOW_CLASS					TEXT("CLCLTooltip")

#define WM_TOOLTIP_SHOW					(WM_APP + 1)
#define WM_TOOLTIP_HIDE					(WM_APP + 2)

#define ID_SHOW_TIMER					1
#define ID_MOUSE_TIMER					2

#define MOUSE_INTERVAL					100


#ifndef SPI_GETTOOLTIPANIMATION
#define SPI_GETTOOLTIPANIMATION			0x00001016
#endif
#ifndef SPI_GETTOOLTIPFADE
#define SPI_GETTOOLTIPFADE				0x00001018
#endif

#ifndef AW_VER_POSITIVE
#define AW_VER_POSITIVE					0x00000004
#endif
#ifndef AW_SLIDE
#define AW_SLIDE						0x00040000
#endif
#ifndef AW_BLEND
#define AW_BLEND						0x00080000
#endif

/* Global Variables */
typedef struct _TOOLTIP_INFO {
	HFONT hfont;

	TCHAR *buf;
	POINT pt;
	int top;
	HWND hWnd;
} TOOLTIP_INFO;

// オプション
extern OPTION_INFO option;

/* Local Function Prototypes */
static int tooltip_get_cursor_height(const HCURSOR hcursor);
static void tooltip_draw_text(const TOOLTIP_INFO *ti, const HDC hdc, RECT *rect);
static LRESULT CALLBACK tooltip_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

/*
 * tooltip_get_cursor_height - マウスカーソルの高さを取得
 */
static int tooltip_get_cursor_height(const HCURSOR hcursor)
{
	HDC hdc, mdc;
	HBITMAP hbmp, ret_hbmp;
	ICONINFO icon_info;
	int width, height;
	int x, y;

	// カーソルの大きさ取得
	width = GetSystemMetrics(SM_CXCURSOR);
	height = GetSystemMetrics(SM_CYCURSOR);

	// カーソル(マスク)の描画
	hdc = GetDC(NULL);
	mdc = CreateCompatibleDC(hdc);
	hbmp = CreateCompatibleBitmap(hdc, width, height);
	ret_hbmp = SelectObject(mdc, hbmp);
	DrawIconEx(mdc, 0, 0, hcursor, width, height, 0, NULL, DI_MASK);

	// カーソルの高さ取得
	for (y = height - 1; y >= 0; y--) {
		for (x = 0; x < width; x++) {
			if (GetPixel(mdc, x, y) != RGB(255, 255, 255)) {
				break;
			}
		}
		if (x < width) {
			break;
		}
	}
	SelectObject(mdc, ret_hbmp);
	DeleteObject(hbmp);
	DeleteDC(mdc);
	ReleaseDC(NULL, hdc);

	// ホットスポットの位置を取得
	ZeroMemory(&icon_info, sizeof(ICONINFO));
	GetIconInfo(hcursor, &icon_info);
	if (icon_info.hbmMask != NULL) {
		DeleteObject(icon_info.hbmMask);
	}
	if (icon_info.hbmColor != NULL) {
		DeleteObject(icon_info.hbmColor);
	}
	return (((y < 0) ? height : y) - icon_info.yHotspot);
}

/*
 * tooltip_draw_text - ツールチップの描画
 */
static void tooltip_draw_text(const TOOLTIP_INFO *ti, const HDC hdc, RECT *rect)
{
	DRAWTEXTPARAMS dtp;
	HBRUSH hbrush;
	HFONT hRetFont;
#ifdef TOOLTIP_COLOR
	DWORD color_infoback = (*option.tooltip_color_back.color_str != TEXT('\0')) ?
		option.tooltip_color_back.color : GetSysColor(COLOR_INFOBK);
	DWORD color_infotext = (*option.tooltip_color_text.color_str != TEXT('\0')) ?
		option.tooltip_color_text.color : GetSysColor(COLOR_INFOTEXT);
#else	// TOOLTIP_COLOR
	DWORD color_infoback = GetSysColor(COLOR_INFOBK);
	DWORD color_infotext = GetSysColor(COLOR_INFOTEXT);
#endif	// TOOLTIP_COLOR

	// 背景の塗りつぶし
	hbrush = CreateSolidBrush(color_infoback);
	FillRect(hdc, rect, hbrush);
	DeleteObject(hbrush);

	if (ti->buf != NULL) {
		// テキストの描画
		hRetFont = SelectObject(hdc, (ti->hfont != NULL) ? ti->hfont : GetStockObject(DEFAULT_GUI_FONT));
		SetRect(rect,
			rect->left + option.tooltip_margin_x,
			rect->top + option.tooltip_margin_y,
			rect->right - option.tooltip_margin_x,
			rect->bottom - option.tooltip_margin_y);

		SetTextColor(hdc, color_infotext);
		SetBkColor(hdc, color_infoback);
		SetBkMode(hdc, TRANSPARENT);

		ZeroMemory(&dtp, sizeof(DRAWTEXTPARAMS));
		dtp.cbSize = sizeof(DRAWTEXTPARAMS);
		dtp.iTabLength = option.tooltip_tab_length;
		DrawTextEx(hdc, ti->buf, lstrlen(ti->buf),
			rect, DT_EDITCONTROL | DT_NOCLIP | DT_EXPANDTABS | DT_TABSTOP | DT_NOPREFIX, &dtp);
		SelectObject(hdc, hRetFont);
	}
}

/*
 * tooltip_proc - ツールチップ
 */
static LRESULT CALLBACK tooltip_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	NONCLIENTMETRICS ncMetrics;
	PAINTSTRUCT ps;
	DRAWTEXTPARAMS dtp;
	RECT rect;
	POINT pt;
	HDC hdc;
	HFONT hRetFont;
	TOOLTIP_INFO *ti;

	switch (msg) {
	case WM_CREATE:
		if ((ti = mem_calloc(sizeof(TOOLTIP_INFO))) == NULL) {
			return -1;
		}
		if (*option.tooltip_font_name != TEXT('\0')) {
			// フォント作成
			ti->hfont = font_create(option.tooltip_font_name,
				option.tooltip_font_size, option.tooltip_font_charset, option.tooltip_font_weight,
				(option.tooltip_font_italic == 0) ? FALSE : TRUE, FALSE);
		} else {
			ncMetrics.cbSize = sizeof(NONCLIENTMETRICS);
			if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
				sizeof(NONCLIENTMETRICS), &ncMetrics, 0) == TRUE) {
				// デフォルトのフォント作成
				ti->hfont = CreateFontIndirect(&ncMetrics.lfStatusFont);
			}
		}
		// tooltip info to window long
		SetWindowLong(hWnd, GWL_USERDATA, (LPARAM)ti);
		break;

	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;

	case WM_DESTROY:
		if ((ti = (TOOLTIP_INFO *)GetWindowLong(hWnd, GWL_USERDATA)) != NULL) {
			if (ti->hfont != NULL) {
				DeleteObject(ti->hfont);
				ti->hfont = NULL;
			}
			if (ti->buf != NULL) {
				mem_free(&ti->buf);
			}
			mem_free(&ti);
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);

	case WM_SETTINGCHANGE:
		if ((ti = (TOOLTIP_INFO *)GetWindowLong(hWnd, GWL_USERDATA)) == NULL ||
			*option.tooltip_font_name != TEXT('\0') ||
			wParam != SPI_SETNONCLIENTMETRICS) {
			break;
		}
		if (ti->hfont != NULL) {
			DeleteObject(ti->hfont);
			ti->hfont = NULL;
		}
		ncMetrics.cbSize = sizeof(NONCLIENTMETRICS);
		if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
			sizeof(NONCLIENTMETRICS), &ncMetrics, 0) == TRUE) {
			// デフォルトのフォント作成
			ti->hfont = CreateFontIndirect(&ncMetrics.lfStatusFont);
		}
		break;

#ifdef TOOLTIP_ANIMATE
	case WM_PRINT:
		// テキスト描画
		if ((ti = (TOOLTIP_INFO *)GetWindowLong(hWnd, GWL_USERDATA)) == NULL) {
			break;
		}
		// 非クライアントエリアの描画
		DefWindowProc(hWnd, msg, wParam, lParam);

		// ツールチップの描画
		GetClientRect(hWnd, (LPRECT)&rect);
		SetRect(&rect, rect.left + 1, rect.top + 1, rect.right + 1, rect.bottom + 1);
		tooltip_draw_text(ti, (HDC)wParam, &rect);
		break;
#endif

	case WM_PAINT:
		// テキスト描画
		if ((ti = (TOOLTIP_INFO *)GetWindowLong(hWnd, GWL_USERDATA)) == NULL) {
			break;
		}
		hdc = BeginPaint(hWnd, &ps);

		// ツールチップの描画
		GetClientRect(hWnd, (LPRECT)&rect);
		tooltip_draw_text(ti, hdc, &rect);

		EndPaint(hWnd, &ps);
		break;

	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_SETCURSOR:
	case WM_TOOLTIP_HIDE:
		// ツールチップ非表示
		KillTimer(hWnd, ID_SHOW_TIMER);
		KillTimer(hWnd, ID_MOUSE_TIMER);
		ShowWindow(hWnd, SW_HIDE);

		if ((ti = (TOOLTIP_INFO *)GetWindowLong(hWnd, GWL_USERDATA)) != NULL && ti->buf != NULL) {
			mem_free(&ti->buf);
		}
		break;

	case WM_TOOLTIP_SHOW:
		// ツールチップ表示
		KillTimer(hWnd, ID_SHOW_TIMER);
		KillTimer(hWnd, ID_MOUSE_TIMER);
		ShowWindow(hWnd, SW_HIDE);
		if (lParam == 0 ||
			((TOOLTIP_INFO *)lParam)->buf == NULL || *((TOOLTIP_INFO *)lParam)->buf == TEXT('\0')) {
			break;
		}
		if ((ti = (TOOLTIP_INFO *)GetWindowLong(hWnd, GWL_USERDATA)) == NULL) {
			break;
		}

		// 座標設定
		ti->top = ((TOOLTIP_INFO *)lParam)->top;
		ti->pt.x = ((TOOLTIP_INFO *)lParam)->pt.x;
		ti->pt.y = ((TOOLTIP_INFO *)lParam)->pt.y;

		// ウィンドウ取得
		if (ti->pt.x == 0 && ti->pt.y == 0) {
			GetCursorPos(&pt);
			ti->hWnd = WindowFromPoint(pt);
		} else {
			ti->hWnd = NULL;
		}

		// テキスト設定
		if (ti->buf != NULL) {
			mem_free(&ti->buf);
		}
		ti->buf = alloc_copy(((TOOLTIP_INFO *)lParam)->buf);

		// ツールチップ表示
		SetTimer(hWnd, ID_SHOW_TIMER, wParam, NULL);
		break;

	case WM_TIMER:
		switch (wParam) {
		case ID_SHOW_TIMER:
			KillTimer(hWnd, wParam);
			if ((ti = (TOOLTIP_INFO *)GetWindowLong(hWnd, GWL_USERDATA)) == NULL || ti->buf == NULL) {
				SendMessage(hWnd, WM_TOOLTIP_HIDE, 0, 0);
				break;
			}

			// 表示位置取得 (マウス位置)
			if (ti->pt.x == 0 && ti->pt.y == 0) {
				GetCursorPos(&ti->pt);
				ti->top = tooltip_get_cursor_height(GetCursor()) + 1;
				if (ti->hWnd != WindowFromPoint(ti->pt)) {
					SendMessage(hWnd, WM_TOOLTIP_HIDE, 0, 0);
					break;
				}
			}

			// サイズ取得
			hdc = GetDC(hWnd);
			hRetFont = SelectObject(hdc, (ti->hfont != NULL) ? ti->hfont : GetStockObject(DEFAULT_GUI_FONT));
			SetRectEmpty(&rect);

			ZeroMemory(&dtp, sizeof(DRAWTEXTPARAMS));
			dtp.cbSize = sizeof(DRAWTEXTPARAMS);
			dtp.iTabLength = option.tooltip_tab_length;
			DrawTextEx(hdc, ti->buf, lstrlen(ti->buf), &rect,
				DT_CALCRECT | DT_EDITCONTROL | DT_NOCLIP | DT_EXPANDTABS | DT_TABSTOP | DT_NOPREFIX, &dtp);

			SelectObject(hdc, hRetFont);
			ReleaseDC(hWnd, hdc);

			// ウィンドウサイズ設定
			SetRect(&rect,
				ti->pt.x,
				ti->pt.y + ti->top,
				rect.right + (option.tooltip_margin_x * 2) + 2,
				rect.bottom + (option.tooltip_margin_y * 2) + 2);

			// 横位置の補正
			if (rect.left + rect.right > GetSystemMetrics(SM_CXSCREEN)) {
				rect.left = GetSystemMetrics(SM_CXSCREEN) - rect.right;
			}
			if (rect.left < 0) {
				rect.left = 0;
			}

			// 縦位置の補正
			if (rect.top + rect.bottom > GetSystemMetrics(SM_CYSCREEN)) {
				rect.top = ti->pt.y - rect.bottom;
			}
			if (rect.top < 0) {
				rect.top = GetSystemMetrics(SM_CYSCREEN) - rect.bottom;
			}
			if (rect.top < 0) {
				rect.top = 0;
			}

			// ウィンドウの位置とサイズを設定
			SetWindowPos(hWnd, HWND_TOPMOST,
				rect.left, rect.top, rect.right, rect.bottom,
				SWP_NOACTIVATE);

#ifdef TOOLTIP_ANIMATE
			{
				HANDLE user32_lib;
				FARPROC AnimateWindow;
				BOOL effect_flag;

				// ウィンドウ表示
				SystemParametersInfo(SPI_GETTOOLTIPANIMATION, 0, &effect_flag, 0);
				if (effect_flag == TRUE) {
					SystemParametersInfo(SPI_GETTOOLTIPFADE, 0, &effect_flag, 0);
					user32_lib = LoadLibrary(TEXT("user32.dll"));
					if (user32_lib != NULL) {
						AnimateWindow = GetProcAddress(user32_lib, "AnimateWindow");
						if (AnimateWindow != NULL) {
							// アニメーション表示
							AnimateWindow(hWnd, 200, (effect_flag == TRUE) ? AW_BLEND : (AW_SLIDE | AW_VER_POSITIVE));
						}
						FreeLibrary(user32_lib);
					}
				}
			}
#endif
			ShowWindow(hWnd, SW_SHOWNOACTIVATE);
			SetTimer(hWnd, ID_MOUSE_TIMER, MOUSE_INTERVAL, NULL);
			break;

		case ID_MOUSE_TIMER:
			if ((ti = (TOOLTIP_INFO *)GetWindowLong(hWnd, GWL_USERDATA)) == NULL ||
				ti->hWnd == NULL || IsWindowVisible(hWnd) == FALSE) {
				KillTimer(hWnd, wParam);
				break;
			}
			// マウスの下のウィンドウをチェック
			GetCursorPos(&pt);
			if (ti->pt.x != pt.x && ti->pt.y != pt.y && ti->hWnd != WindowFromPoint(pt)) {
				SendMessage(hWnd, WM_TOOLTIP_HIDE, 0, 0);
				break;
			}
			break;
		}
		break;

	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

/*
 * tooltip_show - ツールチップを表示
 */
BOOL tooltip_show(const HWND hToolTip, TCHAR *tip_text, const long x, const long y, const long top)
{
	TOOLTIP_INFO ti;

	ti.buf = tip_text;
	ti.pt.x = x;
	ti.pt.y = y;
	ti.top = top;
	SendMessage(hToolTip, WM_TOOLTIP_SHOW, option.tooltip_show_delay, (LPARAM)&ti);
	return TRUE;
}

/*
 * tooltip_hide - ツールチップを非表示
 */
void tooltip_hide(const HWND hToolTip)
{
	SendMessage(hToolTip, WM_TOOLTIP_HIDE, 0, 0);
}

/*
 * tooltip_close - ツールチップを閉じる
 */
void tooltip_close(const HWND hToolTip)
{
	DestroyWindow(hToolTip);
}

/*
 * tooltip_regist - ウィンドウクラスの登録
 */
BOOL tooltip_regist(const HINSTANCE hInstance)
{
	WNDCLASS wc;

	wc.style = 0;
	wc.lpfnWndProc = (WNDPROC)tooltip_proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_INFOBK + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = WINDOW_CLASS;
	// ウィンドウクラスの登録
	return RegisterClass(&wc);
}

/*
 * tooltip_create - ツールチップの作成
 */
HWND tooltip_create(const HINSTANCE hInstance)
{
	HWND hWnd;

	// ウィンドウの作成
	hWnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
		WINDOW_CLASS,
		TEXT(""),
		WS_POPUP | WS_BORDER,
		0, 0, 0, 0, NULL, NULL, hInstance, NULL);
	return hWnd;
}
/* End of source */
