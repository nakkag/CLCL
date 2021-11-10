/*
 * CLCLSet
 *
 * SelectKey.c
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE
#include <commctrl.h>
#ifdef OP_XP_STYLE
#include <uxtheme.h>
#include <vssym32.h>
#endif	// OP_XP_STYLE

#include "..\General.h"
#include "..\Memory.h"
#include "..\Message.h"

#include "SelectKey.h"

#include "resource.h"

/* Define */
#define WINDOW_CLASS					TEXT("select_key")

#ifndef WM_THEMECHANGED
#define WM_THEMECHANGED					0x031A
#endif

/* Global Variables */
typedef struct _SELECTKEY_INFO {
	UINT modifiers;
	UINT virtkey;

	int font_height;

#ifdef OP_XP_STYLE
	// XP
	HMODULE hModThemes;
	HTHEME hTheme;
#endif	// OP_XP_STYLE
} SELECTKEY_INFO;

/* Local Function Prototypes */
static void selectkey_get_keyname(const UINT modifiers, const UINT virtkey, TCHAR *ret);
static LRESULT CALLBACK selectkey_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

/*
 * selectkey_get_keyname - キー名を取得
 */
static void selectkey_get_keyname(const UINT modifiers, const UINT virtkey, TCHAR *ret)
{
	UINT scan_code;
	int ext_flag = 0;

	*ret = TEXT('\0');
	if (modifiers & HOTKEYF_CONTROL) {
		lstrcat(ret, TEXT("Ctrl + "));
	}
	if (modifiers & HOTKEYF_SHIFT) {
		lstrcat(ret, TEXT("Shift + "));
	}
	if (modifiers & HOTKEYF_ALT) {
		lstrcat(ret, TEXT("Alt + "));
	}
	if (modifiers & HOTKEYF_WIN) {
		lstrcat(ret, TEXT("Win + "));
	}
	if (virtkey == 0 || (scan_code = MapVirtualKey(virtkey, 0)) <= 0) {
		if (*ret == TEXT('\0')) {
			lstrcpy(ret, message_get_res(IDS_ACTION_TYPE_NOTHING));
		}
		return;
	}
	if (virtkey == VK_APPS ||
		virtkey == VK_PRIOR ||
		virtkey == VK_NEXT ||
		virtkey == VK_END ||
		virtkey == VK_HOME ||
		virtkey == VK_LEFT ||
		virtkey == VK_UP ||
		virtkey == VK_RIGHT ||
		virtkey == VK_DOWN ||
		virtkey == VK_INSERT ||
		virtkey == VK_DELETE ||
		virtkey == VK_NUMLOCK) {
		ext_flag = 1 << 24;
	}
	GetKeyNameText((scan_code << 16) | ext_flag, ret + lstrlen(ret), BUF_SIZE - lstrlen(ret) - 1);
}

/*
 * selectkey_proc - キー選択
 */
static LRESULT CALLBACK selectkey_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	HFONT hRetFont;
	HBRUSH hbrush;
	RECT rect;
	SIZE sz;
	TEXTMETRIC tm;
	SELECTKEY_INFO *si;
	TCHAR buf[BUF_SIZE];
#ifdef OP_XP_STYLE
	RECT clip_rect;
	DWORD stats;
	static FARPROC _OpenThemeData;
	static FARPROC _CloseThemeData;
	static FARPROC _DrawThemeBackground;
#endif	// OP_XP_STYLE

	switch (msg) {
	case WM_CREATE:
		if ((si = mem_calloc(sizeof(SELECTKEY_INFO))) == NULL) {
			return -1;
		}
#ifdef OP_XP_STYLE
		// XP
		if ((si->hModThemes = LoadLibrary(TEXT("uxtheme.dll"))) != NULL) {
			if (_OpenThemeData == NULL) {
				_OpenThemeData = GetProcAddress(si->hModThemes, "OpenThemeData");
			}
			if (_OpenThemeData != NULL) {
				si->hTheme = (HTHEME)_OpenThemeData(hWnd, L"Edit");
			}
		}
#endif	// OP_XP_STYLE
		// 描画の初期化
		hdc = GetDC(hWnd);
		hRetFont = SelectObject(hdc, (HFONT)SendMessage(GetParent(hWnd), WM_GETFONT, 0, 0));
		GetTextMetrics(hdc, &tm);
		si->font_height = tm.tmHeight;
		SelectObject(hdc, hRetFont);
		ReleaseDC(hWnd, hdc);

		// select key info to window long
		SetWindowLong(hWnd, GWL_USERDATA, (LPARAM)si);
		break;

	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;

	case WM_DESTROY:
		if ((si = (SELECTKEY_INFO *)GetWindowLong(hWnd, GWL_USERDATA)) != NULL) {
#ifdef OP_XP_STYLE
			// XP
			if (si->hTheme != NULL) {
				if (_CloseThemeData == NULL) {
					_CloseThemeData = GetProcAddress(si->hModThemes, "CloseThemeData");
				}
				if (_CloseThemeData != NULL) {
					_CloseThemeData(si->hTheme);
				}
			}
			if (si->hModThemes != NULL) {
				FreeLibrary(si->hModThemes);
			}
#endif	// OP_XP_STYLE
			mem_free(&si);
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);

	case WM_SETFOCUS:
		if ((si = (SELECTKEY_INFO *)GetWindowLong(hWnd, GWL_USERDATA)) == NULL) {
			break;
		}
		// キャレットの表示
		CreateCaret(hWnd, 0, 0, si->font_height);
		InvalidateRect(hWnd, NULL, FALSE);
		ShowCaret(hWnd);
		break;

	case WM_KILLFOCUS:
		if ((si = (SELECTKEY_INFO *)GetWindowLong(hWnd, GWL_USERDATA)) == NULL) {
			break;
		}
		if (si->modifiers != 0 && si->virtkey == 0) {
			si->modifiers = 0;
			InvalidateRect(hWnd, NULL, FALSE);
		}
		HideCaret(hWnd);
		DestroyCaret();
		break;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		SetFocus(hWnd);
		break;

	case WM_ENABLE:
		InvalidateRect(hWnd, NULL, FALSE);
		break;

	case WM_PAINT:
		// テキスト描画
		if ((si = (SELECTKEY_INFO *)GetWindowLong(hWnd, GWL_USERDATA)) == NULL) {
			break;
		}
		// キー名の取得
		selectkey_get_keyname(si->modifiers, si->virtkey, buf);

		hdc = BeginPaint(hWnd, &ps);
		GetClientRect(hWnd, (LPRECT)&rect);

		// 背景の塗りつぶし
		if (IsWindowEnabled(hWnd) == FALSE) {
			hbrush = GetSysColorBrush(COLOR_BTNFACE);
		} else {
			hbrush = GetSysColorBrush(COLOR_WINDOW);
		}
		FillRect(hdc, &rect, hbrush);

		// テキストの描画
		if (IsWindowEnabled(hWnd) == FALSE) {
			SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT));
			SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
		} else {
			SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
			SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
		}
		hRetFont = SelectObject(hdc, (HFONT)SendMessage(GetParent(hWnd), WM_GETFONT, 0, 0));
		GetTextExtentPoint32(hdc, buf, lstrlen(buf), &sz);
		ExtTextOut(hdc, 1, 1, ETO_OPAQUE, &rect, buf, lstrlen(buf), NULL);

		if (GetFocus() == hWnd) {
			// キャレットの位置を設定
			SetCaretPos(sz.cx + 1, 1);
		}
		SelectObject(hdc, hRetFont);
		EndPaint(hWnd, &ps);
		break;

#ifdef OP_XP_STYLE
	case WM_NCPAINT:
		if ((si = (SELECTKEY_INFO *)GetWindowLong(hWnd, GWL_USERDATA)) == NULL || si->hTheme == NULL) {
			return DefWindowProc(hWnd, msg, wParam, lParam);
		}
		// XP用の背景描画
		if (_DrawThemeBackground == NULL) {
			_DrawThemeBackground = GetProcAddress(si->hModThemes, "DrawThemeBackground");
		}
		if (_DrawThemeBackground == NULL) {
			return DefWindowProc(hWnd, msg, wParam, lParam);
		}
		// 状態の設定
		if (IsWindowEnabled(hWnd) == 0) {
			stats = ETS_DISABLED;
		} else if (GetFocus() == hWnd) {
			stats = ETS_FOCUSED;
		} else {
			stats = ETS_NORMAL;
		}
		// ウィンドウ枠の描画
		if ((hdc = GetDCEx(hWnd, (HRGN)wParam, DCX_WINDOW | DCX_INTERSECTRGN)) == NULL) {
			hdc = GetWindowDC(hWnd);
		}
		GetWindowRect(hWnd, &rect);
		OffsetRect(&rect, -rect.left, -rect.top);
		ExcludeClipRect(hdc, rect.left + GetSystemMetrics(SM_CXEDGE), rect.top + GetSystemMetrics(SM_CYEDGE),
			rect.right - GetSystemMetrics(SM_CXEDGE), rect.bottom - GetSystemMetrics(SM_CYEDGE));
		clip_rect = rect;
		_DrawThemeBackground(si->hTheme, hdc, EP_EDITTEXT, stats, &rect, &clip_rect);
		ReleaseDC(hWnd, hdc);
		return 0;

	case WM_THEMECHANGED:
		if ((si = (SELECTKEY_INFO *)GetWindowLong(hWnd, GWL_USERDATA)) == NULL || si->hModThemes == NULL) {
			break;
		}
		// XPテーマの変更
		if (si->hTheme != NULL) {
			if (_CloseThemeData == NULL) {
				_CloseThemeData = GetProcAddress(si->hModThemes, "CloseThemeData");
			}
			if (_CloseThemeData != NULL) {
				_CloseThemeData(si->hTheme);
			}
			si->hTheme = NULL;
		}
		if (_OpenThemeData == NULL) {
			_OpenThemeData = GetProcAddress(si->hModThemes, "OpenThemeData");
		}
		if (_OpenThemeData != NULL) {
			si->hTheme = (HTHEME)_OpenThemeData(hWnd, L"Edit");
		}
		break;
#endif	// OP_XP_STYLE

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if ((si = (SELECTKEY_INFO *)GetWindowLong(hWnd, GWL_USERDATA)) == NULL) {
			break;
		}
		if (wParam == VK_TAB) {
			// フォーカスの移動
			if (GetKeyState(VK_SHIFT) & 0x1000) {
				if (si->modifiers != 0 && si->virtkey == 0) {
					si->modifiers = 0;
					InvalidateRect(hWnd, NULL, FALSE);
				}
				SendMessage(GetParent(hWnd), WM_NEXTDLGCTL, 1, FALSE);
			} else {
				SendMessage(GetParent(hWnd), WM_NEXTDLGCTL, 0, FALSE);
			}
			break;
		}
		if (lParam & 0x40000000) {
			// 直前に同じキーが押されている場合は処理しない
			break;
		}

		// 制御キー
		si->modifiers = 0;
		if (GetKeyState(VK_CONTROL) & 0x1000) {
			si->modifiers |= HOTKEYF_CONTROL;
		}
		if (GetKeyState(VK_SHIFT) & 0x1000) {
			si->modifiers |= HOTKEYF_SHIFT;
		}
		if (lParam & 0x20000000) {
			si->modifiers |= HOTKEYF_ALT;
		}
		if (GetKeyState(VK_LWIN) & 0x1000 || GetKeyState(VK_RWIN) & 0x1000) {
			si->modifiers |= HOTKEYF_WIN;
		}

		// 仮想キー
		if (wParam == VK_CONTROL || wParam == VK_SHIFT || wParam == VK_MENU ||
			wParam == VK_RCONTROL || wParam == VK_RSHIFT || wParam == VK_RMENU ||
			wParam == VK_LWIN || wParam == VK_RWIN) {
			si->virtkey = 0;
		} else {
			si->virtkey = wParam;
		}
		InvalidateRect(hWnd, NULL, FALSE);
		break;

	case WM_KEYUP:
	case WM_SYSKEYUP:
		if ((si = (SELECTKEY_INFO *)GetWindowLong(hWnd, GWL_USERDATA)) == NULL) {
			break;
		}
		if (si->virtkey == 0 &&
			(wParam == VK_CONTROL || wParam == VK_SHIFT || wParam == VK_MENU ||
			wParam == VK_RCONTROL || wParam == VK_RSHIFT || wParam == VK_RMENU ||
			wParam == VK_LWIN || wParam == VK_RWIN)) {
			// キー解除
			si->modifiers = 0;
			InvalidateRect(hWnd, NULL, FALSE);
		}
		break;

	case WM_GETDLGCODE:
		return DLGC_WANTARROWS | DLGC_WANTALLKEYS;

	case HKM_GETHOTKEY:
		if ((si = (SELECTKEY_INFO *)GetWindowLong(hWnd, GWL_USERDATA)) == NULL) {
			break;
		}
		return MAKEWORD(si->virtkey, si->modifiers);

	case HKM_SETHOTKEY:
		if ((si = (SELECTKEY_INFO *)GetWindowLong(hWnd, GWL_USERDATA)) == NULL) {
			break;
		}
		si->virtkey = LOBYTE(wParam);
		si->modifiers = HIBYTE(wParam);
		InvalidateRect(hWnd, NULL, FALSE);
		break;

	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

/*
 * init_selectkey - キー選択の初期化
 */
BOOL init_selectkey(const HINSTANCE hInstance)
{
	WNDCLASS wc;

	wc.style = 0;
	wc.lpfnWndProc = (WNDPROC)selectkey_proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_IBEAM);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = WINDOW_CLASS;
	// ウィンドウクラスの登録
	return RegisterClass(&wc);
}
/* End of source */
