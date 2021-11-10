/*
 * CLCL
 *
 * main.c
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
#include <tchar.h>
#include <shlobj.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

#include "General.h"
#include "Memory.h"
#include "String.h"
#include "Data.h"
#include "Profile.h"
#include "Ini.h"
#include "Message.h"
#include "File.h"
#include "ClipBoard.h"
#include "History.h"
#include "Regist.h"
#include "Menu.h"
#include "SendKey.h"
#include "Format.h"
#include "Window.h"
#include "Tool.h"
#include "Viewer.h"
#include "Container.h"
#include "BinView.h"
#include "ToolTip.h"
#include "dpi.h"

#include "resource.h"

/* Define */
#define ERROR_TITLE						TEXT("CLCL - Error")
#define MUTEX							TEXT("_CLCL_Mutex_")

#define WM_TRAY_NOTIFY					(WM_APP + 1000)		// タスクトレイ
#define WM_KEY_HOOK						(WM_APP + 1001)		// フック

#define SICONSIZE						Scale(16)

#define TRAY_ID							1					// タスクトレイID

#define ID_HISTORY_TIMER				1					// タイマーID
#define ID_RECHAIN_TIMER				2
#define ID_TOOL_TIMER					3
#define ID_PASTE_TIMER					4
#define ID_KEY_TIMER					5
#define ID_LCLICK_TIMER					6
#define ID_RCLICK_TIMER					7

#define RECLIP_INTERVAL					1000
#define RECHAIN_INTERVAL				60000

#define TOOLFLAG_CALL					1
#define TOOLFLAG_PASTE					2

#define key_wait()						while (GetAsyncKeyState(VK_MENU) < 0 || \
											GetAsyncKeyState(VK_CONTROL) < 0 || \
											GetAsyncKeyState(VK_SHIFT) < 0 || \
											GetAsyncKeyState(VK_LWIN) < 0 || \
											GetAsyncKeyState(VK_RWIN) < 0) Sleep(100)

/* Global Variables */
HINSTANCE hInst;
static TCHAR app_path[MAX_PATH];
TCHAR work_path[MAX_PATH];

static HWND hViewerWnd;
static HWND hToolTip;
static HWND hClipNextWnd;
static HMENU popup_menu;
static HMODULE hook_lib;
static HMODULE themes_lib;
static HICON icon_tray;
static HICON icon_clip;
static HICON icon_clip_ban;
HICON icon_menu_default;
HICON icon_menu_folder;

static POINT menu_sel_pt;
static int menu_sel_top;

static BOOL accel_flag = TRUE;
static BOOL clip_flag;
static int rechain_cnt;

DATA_INFO history_data;
DATA_INFO regist_data;
static DATA_INFO *paste_di;

// ツールメニュー情報
typedef struct _TOOL_MENU_INFO {
	BOOL enable;
	TOOL_INFO *ti;
	int paste;
} TOOL_MENU_INFO;
static TOOL_MENU_INFO tmi;

// フォーカス情報
typedef struct _FOCUS_INFO {
	HWND active_wnd;
	HWND focus_wnd;
	POINT cpos;
	BOOL caret;
} FOCUS_INFO;
static FOCUS_INFO focus_info;

// オプション
extern OPTION_INFO option;

/* Local Function Prototypes */
static void get_focus_info(FOCUS_INFO *fi);
static void set_focus_info(const FOCUS_INFO *fi);
static BOOL tray_message(const HWND hWnd, const DWORD dwMessage, const UINT uID, const HICON hIcon, const TCHAR *pszTip);
static void set_tray_icon(const HWND hWnd, const HICON hIcon, const TCHAR *buf);
static void set_tray_tooltip(const HWND hWnd);
static BOOL show_menu_tooltip(const HWND tooltip_wnd, const HMENU hMenu, const UINT id, const BOOL mouse);
static BOOL show_tool_menu(const HWND hWnd, DATA_INFO *di, const int paste);
static BOOL show_popup_menu(const HWND hWnd, const ACTION_INFO *ai, const BOOL caret);
static BOOL action_execute(const HWND hWnd, const int type, const int id, const BOOL caret);
static BOOL action_check(const int type);
static BOOL clipboard_to_history(const HWND hWnd);
static BOOL item_to_clipboard(const HWND hWnd, DATA_INFO *from_di, const BOOL delete_flag);
static BOOL load_history(const HWND hWnd, const int load_flag);
static BOOL load_regist(const HWND hWnd);
static BOOL save_history(const HWND hWnd, const int save_flag);
static BOOL save_regist(const HWND hWnd);
static void regist_hotkey(const HWND hWnd, const BOOL show_err);
static void unregist_hotkey(const HWND hWnd);
static void regist_hook(const HWND hWnd);
static void unregist_hook(void);
static BOOL winodw_initialize(const HWND hWnd);
static BOOL winodw_reset(const HWND hWnd);
static BOOL winodw_save(const HWND hWnd);
static BOOL winodw_end(const HWND hWnd);
static LRESULT CALLBACK main_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void get_work_path(const HINSTANCE hInstance);
static void commnad_line_func(const HWND hWnd);
static BOOL init_application(const HINSTANCE hInstance);
static HWND init_instance(const HINSTANCE hInstance, const int CmdShow);

/*
 * theme_open - XPテーマを開く
 */
#ifdef OP_XP_STYLE
HTHEME theme_open(const HWND hWnd)
{
	static FARPROC _OpenThemeData;
	HTHEME hTheme = NULL;

	if (themes_lib == NULL && (themes_lib = LoadLibrary(TEXT("uxtheme.dll"))) == NULL) {
		return NULL;
	}
	if (_OpenThemeData == NULL) {
		_OpenThemeData = GetProcAddress(themes_lib, "OpenThemeData");
	}
	if (_OpenThemeData != NULL) {
		hTheme = (HTHEME)_OpenThemeData(hWnd, L"Edit");
	}
	return hTheme;
}
#endif

/*
 * theme_close - XPテーマを閉じる
 */
#ifdef OP_XP_STYLE
void theme_close(const HTHEME hTheme)
{
	static FARPROC _CloseThemeData;

	if (themes_lib == NULL || hTheme == NULL) {
		return;
	}
	if (_CloseThemeData == NULL) {
		_CloseThemeData = GetProcAddress(themes_lib, "CloseThemeData");
	}
	if (_CloseThemeData != NULL) {
		_CloseThemeData(hTheme);
	}
}
#endif

/*
 * theme_free - XPテーマの解放
 */
#ifdef OP_XP_STYLE
void theme_free(void)
{
	if (themes_lib != NULL) {
		FreeLibrary(themes_lib);
		themes_lib = NULL;
	}
}
#endif

/*
 * theme_draw - XPテーマで描画
 */
#ifdef OP_XP_STYLE
BOOL theme_draw(const HWND hWnd, const HRGN draw_hrgn, const HTHEME hTheme)
{
	static FARPROC _DrawThemeBackground;
	RECT rect, clip_rect;
	HRGN hrgn;
	HDC hdc;
	DWORD stats;

	if (themes_lib == NULL || hTheme == NULL) {
		return FALSE;
	}
	if (_DrawThemeBackground == NULL) {
		_DrawThemeBackground = GetProcAddress(themes_lib, "DrawThemeBackground");
	}
	if (_DrawThemeBackground == NULL) {
		return FALSE;
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
	hdc = GetDCEx(hWnd, draw_hrgn, DCX_WINDOW | DCX_INTERSECTRGN);
	if (hdc == NULL) {
		hdc = GetWindowDC(hWnd);
	}
	GetWindowRect(hWnd, &rect);
	OffsetRect(&rect, -rect.left, -rect.top);
	ExcludeClipRect(hdc, rect.left + GetSystemMetrics(SM_CXEDGE), rect.top + GetSystemMetrics(SM_CYEDGE),
		rect.right - GetSystemMetrics(SM_CXEDGE), rect.bottom - GetSystemMetrics(SM_CYEDGE));
	clip_rect = rect;
	_DrawThemeBackground(hTheme, hdc, EP_EDITTEXT, stats, &rect, &clip_rect);
	ReleaseDC(hWnd, hdc);

	// スクロールバーの描画
	GetWindowRect(hWnd, (LPRECT)&rect);
	hrgn = CreateRectRgn(rect.left + GetSystemMetrics(SM_CXEDGE), rect.top + GetSystemMetrics(SM_CYEDGE),
		rect.right - GetSystemMetrics(SM_CXEDGE), rect.bottom - GetSystemMetrics(SM_CYEDGE));
	CombineRgn(hrgn, hrgn, draw_hrgn, RGN_AND);
	DefWindowProc(hWnd, WM_NCPAINT, (WPARAM)hrgn, 0);
	DeleteObject(hrgn);
	return TRUE;
}
#endif

/*
 * set_menu_layerer - メニューを半透明にする (Windows2000～)
 */
#ifdef MENU_LAYERER
static BOOL set_menu_layerer(const HWND hWnd, const int alpha)
{
#ifndef WS_EX_LAYERED
#define WS_EX_LAYERED           0x00080000
#endif
#ifndef LWA_COLORKEY
#define LWA_COLORKEY            0x00000001
#endif
#ifndef LWA_ALPHA
#define LWA_ALPHA               0x00000002
#endif
	HANDLE user32_lib;
	FARPROC SetLayeredWindowAttributes;
	long lStyle;

	if (alpha <= 0 || alpha >= 255) {
		return TRUE;
	}

	// 半透明用API取得
	user32_lib = LoadLibrary(TEXT("user32.dll"));
	if (user32_lib == NULL) {
		return FALSE;
	}
	SetLayeredWindowAttributes = GetProcAddress(user32_lib, "SetLayeredWindowAttributes");
	if (SetLayeredWindowAttributes == NULL) {
		FreeLibrary(user32_lib);
		return FALSE;
	}

	lStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
	if (lStyle & WS_EX_LAYERED) {
		// 既に半透明済み
		FreeLibrary(user32_lib);
		return TRUE;
	}
	lStyle |= WS_EX_LAYERED;
	SetWindowLong(hWnd, GWL_EXSTYLE, lStyle);

	// 半透明
	SetLayeredWindowAttributes(hWnd, 0, alpha, LWA_ALPHA);
	FreeLibrary(user32_lib);
	return TRUE;
}
#endif

/*
 * _SetForegroundWindow - ウィンドウをアクティブにする
 */
BOOL _SetForegroundWindow(const HWND hWnd)
{
#ifndef SPI_GETFOREGROUNDLOCKTIMEOUT
#define SPI_GETFOREGROUNDLOCKTIMEOUT		  0x2000
#endif
#ifndef SPI_SETFOREGROUNDLOCKTIMEOUT
#define SPI_SETFOREGROUNDLOCKTIMEOUT		  0x2001
#endif
	int nTargetID, nForegroundID;
	UINT nTimeout;
	BOOL ret;

	nForegroundID = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
	nTargetID = GetWindowThreadProcessId(hWnd, NULL);
	AttachThreadInput(nTargetID, nForegroundID, TRUE);

	SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &nTimeout, 0);
	SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (PVOID)0, 0);

	ret = SetForegroundWindow(hWnd);

	SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (PVOID)nTimeout, 0);

	AttachThreadInput(nTargetID, nForegroundID, FALSE);
	return ret;
}

/*
 * get_focus_info - フォーカス情報を取得
 */
static void get_focus_info(FOCUS_INFO *fi)
{
	// フォーカスを持つウィンドウの取得
	fi->active_wnd = GetForegroundWindow();
	AttachThreadInput(GetWindowThreadProcessId(fi->active_wnd, NULL), GetCurrentThreadId(), TRUE);
	fi->focus_wnd = GetFocus();
	// キャレット位置取得
	if (GetCaretPos(&fi->cpos) == TRUE && (fi->cpos.x > 0 || fi->cpos.y > 0)) {
		ClientToScreen(fi->focus_wnd, &fi->cpos);
		fi->caret = TRUE;
	} else {
		fi->caret = FALSE;
	}
	AttachThreadInput(GetWindowThreadProcessId(fi->active_wnd, NULL), GetCurrentThreadId(), FALSE);
}

/*
 * set_focus_info - ウィンドウのフォーカスを設定
 */
static void set_focus(const HWND active_wnd, const HWND focus_wnd)
{
	if (focus_wnd != NULL) {
		AttachThreadInput(GetWindowThreadProcessId(active_wnd, NULL), GetCurrentThreadId(), TRUE);
		if (GetFocus() != focus_wnd) {
			SetFocus(focus_wnd);
		}
		AttachThreadInput(GetWindowThreadProcessId(active_wnd, NULL), GetCurrentThreadId(), FALSE);
	}
}
static void set_focus_info(const FOCUS_INFO *fi)
{
	// アクティブウィンドウの設定
	_SetForegroundWindow(fi->active_wnd);
	SendMessage(fi->active_wnd, WM_NCACTIVATE, (WPARAM)TRUE, 0);
	// フォーカスの設定
	if (window_focus_check(fi->active_wnd) == TRUE) {
		set_focus(fi->active_wnd, fi->focus_wnd);
	}
}

/*
 * tray_message - タスクトレイのアイコンの設定
 */
static BOOL tray_message(const HWND hWnd, const DWORD dwMessage, const UINT uID, const HICON hIcon, const TCHAR *pszTip)
{
	NOTIFYICONDATA tnd;

	tnd.cbSize = sizeof(NOTIFYICONDATA);
	tnd.hWnd = hWnd;
	tnd.uID	= uID;
	tnd.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	tnd.uCallbackMessage = WM_TRAY_NOTIFY;
	tnd.hIcon = hIcon;
	lstrcpyn(tnd.szTip, (pszTip == NULL) ? TEXT("") : pszTip, 64 - 1);
	return Shell_NotifyIcon(dwMessage, &tnd);
}

/*
 * set_tray_icon - タスクトレイにアイコンを設定する
 */
static void set_tray_icon(const HWND hWnd, const HICON hIcon, const TCHAR *buf)
{
	if (hIcon == NULL || option.main_show_trayicon == 0) {
		return;
	}
	if (tray_message(hWnd, NIM_MODIFY, TRAY_ID, hIcon, buf) == FALSE) {
		// 変更できなかった場合は追加を行う
		int i;
		for (i = 0; i < 5; i++) {
			// 追加できなかった場合はリトライする
			if (tray_message(hWnd, NIM_ADD, TRAY_ID, hIcon, buf)) {
				break;
			}
			Sleep(5000);
		}
	}
}

/*
 * set_tray_tooltip - タスクトレイのツールチップを設定
 */
static void set_tray_tooltip(const HWND hWnd)
{
	if (option.main_show_trayicon == 0) {
		return;
	}
	if (history_data.child == NULL) {
		set_tray_icon(hWnd, icon_tray, MAIN_WINDOW_TITLE);
	} else {
		set_tray_icon(hWnd, icon_tray, data_get_title(history_data.child));
	}
}

/*
 * show_menu_tooltip - メニューのツールチップ表示
 */
static BOOL show_menu_tooltip(const HWND tooltip_wnd, const HMENU hMenu, const UINT id, const BOOL mouse)
{
	MENU_ITEM_INFO *mii;
	DATA_INFO *di;
	TCHAR *buf;

	// IDからメニュー情報を取得
	mii = menu_get_info(id);
	if (mii == NULL) {
		tooltip_hide(tooltip_wnd);
		return FALSE;
	}
	di = mii->show_di;
	if (di == NULL) {
		tooltip_hide(tooltip_wnd);
		return FALSE;
	}
	// ツールチップに表示するテキストを取得
	buf = format_get_tooltip_text(di);
	if (buf == NULL) {
		tooltip_hide(tooltip_wnd);
		return FALSE;
	}
	if (mouse == TRUE) {
		// マウス位置
		menu_sel_pt.x = menu_sel_pt.y = 0;
		menu_sel_top = 0;
	}
	// ツールチップ表示
	tooltip_show(tooltip_wnd, buf, menu_sel_pt.x, menu_sel_pt.y, menu_sel_top);
	mem_free(&buf);
	return TRUE;
}

/*
 * show_tool_menu - ツールメニューを表示
 */
static BOOL show_tool_menu(const HWND hWnd, DATA_INFO *di, const int paste)
{
	MENU_ITEM_INFO *mii;
	MENU_INFO mi;
	int ret;

	if (popup_menu != NULL) {
		return FALSE;
	}
	// メニュー作成
	ZeroMemory(&mi, sizeof(MENU_INFO));
	mi.content = MENU_CONTENT_TOOL;
	popup_menu = menu_create(hWnd, &mi, 1, NULL, NULL);
	if (popup_menu == NULL) {
		menu_free();
		return FALSE;
	}
	if (GetMenuItemCount(popup_menu) == 0) {
		menu_destory(popup_menu);
		popup_menu = NULL;
		menu_free();
		return FALSE;
	}
	// メニュー表示
	_SetForegroundWindow(hWnd);
	ret = menu_show(hWnd, popup_menu, NULL);
	menu_destory(popup_menu);
	popup_menu = NULL;

	mii = menu_get_info(ret);
	if (ret <= 0 || ret == IDCANCEL || mii == NULL) {
		menu_free();
		return FALSE;
	}
	if (mii->ti->copy_paste == 1) {
		// クリップボードに送ってからツールを実行
		tmi.enable = TRUE;
		tmi.ti = mii->ti;
		tmi.paste = (GetKeyState(VK_SHIFT) >= 0) ? paste : 0;
		menu_free();
		return TRUE;
	}
	// ツールの実行
	if (tool_execute(hWnd, mii->ti, CALLTYPE_MENU, di, NULL) & TOOL_DATA_MODIFIED) {
		if (data_check(&history_data, di) != NULL) {
			SendMessage(hWnd, WM_HISTORY_CHANGED, 0, 0);
		} else if (data_check(&regist_data, di) != NULL) {
			SendMessage(hWnd, WM_REGIST_CHANGED, 0, 0);
		}
	}
	menu_free();
	return FALSE;
}

/*
 * show_popup_menu - ポップアップメニューを表示
 */
static BOOL show_popup_menu(const HWND hWnd, const ACTION_INFO *ai, const BOOL caret)
{
	MENU_ITEM_INFO *mii;
	FOCUS_INFO fi;
	int ret;
	BOOL caret_flag = caret;

	if (popup_menu != NULL) {
		// ポップアップメニュー表示中
		_SetForegroundWindow(hWnd);
		return FALSE;
	}
	CopyMemory(&fi, &focus_info, sizeof(FOCUS_INFO));
	if (caret == TRUE || fi.active_wnd == NULL) {
		// フォーカス情報取得
		get_focus_info(&fi);
	}
	if (ai->caret == 0 || fi.caret == FALSE) {
		caret_flag = FALSE;
	}

	// キー初期化
	GetAsyncKeyState(VK_RBUTTON);

	// メニュー作成
	popup_menu = menu_create(hWnd, ai->menu_info, ai->menu_cnt, history_data.child, regist_data.child);
	if (popup_menu == NULL) {
		menu_free();
		return FALSE;
	}
	if (GetMenuItemCount(popup_menu) == 0) {
		menu_destory(popup_menu);
		popup_menu = NULL;
		menu_free();
		return FALSE;
	}
	// メニュー表示
	_SetForegroundWindow(hWnd);
	ret = menu_show(hWnd, popup_menu, (caret_flag == TRUE) ? &fi.cpos : NULL);
	menu_destory(popup_menu);
	popup_menu = NULL;

	mii = menu_get_info(ret);
	if (ret <= 0 || ret == IDCANCEL || mii == NULL) {
		// キャンセル
		if (GetForegroundWindow() == hWnd) {
			set_focus_info(&fi);
		}

	} else if (mii->set_di != NULL) {
		// アイテム
		if ((GetAsyncKeyState(VK_RBUTTON) == 1 || GetKeyState(VK_CONTROL) < 0) &&
			option.menu_show_tool_menu == 1) {
			DATA_INFO *di = mii->set_di;
			menu_free();
			// ツールメニュー表示
			if (show_tool_menu(hWnd, di, ai->paste) == TRUE) {
				set_focus_info(&fi);
				SendMessage(hWnd, WM_ITEM_TO_CLIPBOARD, 0, (LPARAM)di);
			} else {
				set_focus_info(&fi);
			}
			return TRUE;
		}
		// クリップボードにデータを設定
		set_focus_info(&fi);
		SendMessage(hWnd, WM_ITEM_TO_CLIPBOARD, 0, (LPARAM)mii->set_di);
		if (ai->paste == 1 && GetKeyState(VK_SHIFT) >= 0) {
			// キーを離すまで待機
			key_wait();
			// ホットキーの解除
			unregist_hotkey(hWnd);
			// 貼り付け
			sendkey_paste(fi.active_wnd);
			// ホットキーの登録
			regist_hotkey(hWnd, FALSE);
		}

	} else if (mii->ti != NULL) {
		// ツール
		set_focus_info(&fi);
		if (mii->ti->copy_paste == 1) {
			tmi.enable = TRUE;
			tmi.ti = mii->ti;
			tmi.paste = (GetKeyState(VK_SHIFT) >= 0) ? ai->paste : 0;
			// キーを離すまで待機
			key_wait();
			SetTimer(hWnd, ID_TOOL_TIMER, option.tool_valid_interval, NULL);
			// コピー
			sendkey_copy(fi.active_wnd);
		} else {
			// ツール実行
			if (tool_execute(hWnd, mii->ti, CALLTYPE_MENU, history_data.child, NULL) & TOOL_DATA_MODIFIED) {
				SendMessage(hWnd, WM_HISTORY_CHANGED, 0, 0);
				SendMessage(hWnd, WM_ITEM_TO_CLIPBOARD, 0, (LPARAM)history_data.child);
			}
		}

	} else if (mii->mi != NULL) {
		// アプリケーションの起動
		shell_open(mii->mi->path, mii->mi->cmd);

	} else {
		// コマンド
		SendMessage(hWnd, WM_COMMAND, ret, 0);
	}
	// メニュー情報の解放
	menu_free();
	return TRUE;
}

/*
 * action_execute - Actionの実行
 */
static BOOL action_execute(const HWND hWnd, const int type, const int id, const BOOL caret)
{
	DATA_INFO *di;
	int i;
	BOOL ret;

	ZeroMemory(&tmi, sizeof(TOOL_MENU_INFO));

	// 動作の検索
	for (i = 0; i < option.action_cnt; i++) {
		if (type == (option.action_info + i)->type && (option.action_info + i)->enable != 0) {
			if (type == ACTION_TYPE_HOTKEY && id != (option.action_info + i)->id) {
				continue;
			}
			break;
		}
	}
	// ツールの検索
	if (i >= option.action_cnt && type == ACTION_TYPE_HOTKEY) {
		for (i = 0; i < option.tool_cnt; i++) {
			if (id != (option.tool_info + i)->id) {
				continue;
			}
			if ((option.tool_info + i)->copy_paste == 1) {
				tmi.enable = TRUE;
				tmi.ti = option.tool_info + i;
				tmi.paste = 1;
				// キーを離すまで待機
				key_wait();
				SetTimer(hWnd, ID_TOOL_TIMER, option.tool_valid_interval, NULL);
				// コピー
				sendkey_copy(GetForegroundWindow());
			} else {
				// ツール実行
				if (tool_execute(hWnd, option.tool_info + i, CALLTYPE_MENU, history_data.child, NULL) & TOOL_DATA_MODIFIED) {
					SendMessage(hWnd, WM_HISTORY_CHANGED, 0, 0);
					SendMessage(hWnd, WM_ITEM_TO_CLIPBOARD, 0, (LPARAM)history_data.child);
				}
			}
			return TRUE;
		}
		if (i >= option.tool_cnt) {
			// 登録アイテムを直接貼り付け
			di = regist_hotkey_to_item(regist_data.child, id);
			if (di != NULL) {
				paste_di = di;
				SetTimer(hWnd, ID_PASTE_TIMER, 1, NULL);
			}
		}
		return TRUE;
	}
	if (i >= option.action_cnt) {
		return TRUE;
	}

	// 動作を実行
	switch ((option.action_info + i)->action) {
	case ACTION_POPUPMEMU:
		// ポップアップメニュー
		ret = show_popup_menu(hWnd, option.action_info + i, caret);
		ZeroMemory(&focus_info, sizeof(FOCUS_INFO));
		return ret;

	case ACTION_VIEWER:
		// ビューア表示
		SendMessage(hWnd, WM_COMMAND, ID_MENUITEM_VIEWER, 0);
		break;

	case ACTION_OPTION:
		// オプション
		SendMessage(hWnd, WM_COMMAND, ID_MENUITEM_OPTION, 0);
		break;

	case ACTION_CLIPBOARD_WATCH:
		// クリップボード監視切り替え
		SendMessage(hWnd, WM_COMMAND, ID_MENUITEM_CLIPBOARD_WATCH, 0);
		break;

	case ACTION_EXIT:
		// 終了
		SendMessage(hWnd, WM_COMMAND, ID_MENUITEM_EXIT, 0);
		break;
	}
	return TRUE;
}

/*
 * action_check - Actionのチェック
 */
static BOOL action_check(const int type)
{
	int i;

	// 動作の検索
	for (i = 0; i < option.action_cnt; i++) {
		if (type == (option.action_info + i)->type && (option.action_info + i)->enable != 0) {
			return TRUE;
		}
	}
	return FALSE;
}

/*
 * clipboard_to_history - クリップボードの内容を履歴に追加
 */
static BOOL clipboard_to_history(const HWND hWnd)
{
	DATA_INFO *di;
	TCHAR err_str[BUF_SIZE];
	TOOL_MENU_INFO cp_tmi;

	CopyMemory(&cp_tmi, &tmi, sizeof(TOOL_MENU_INFO));

	// 除外ウィンドウのチェック
	if (window_ignore_check(GetForegroundWindow()) == FALSE) {
		KillTimer(hWnd, ID_HISTORY_TIMER);
		KillTimer(hWnd, ID_TOOL_TIMER);
		ZeroMemory(&tmi, sizeof(TOOL_MENU_INFO));
		return TRUE;
	}

	if (OpenClipboard(hWnd) == FALSE) {
		// クリップボードが利用可能になるまで待機
		SetTimer(hWnd, ID_HISTORY_TIMER, RECLIP_INTERVAL, NULL);
		if (tmi.enable == TRUE) {
			SetTimer(hWnd, ID_TOOL_TIMER, option.tool_valid_interval, NULL);
		}
		return FALSE;
	}
	KillTimer(hWnd, ID_HISTORY_TIMER);
	KillTimer(hWnd, ID_TOOL_TIMER);
	ZeroMemory(&tmi, sizeof(TOOL_MENU_INFO));

	// クリップボードからアイテムを作成
	*err_str = TEXT('\0');
	if ((di = clipboard_to_item(err_str)) == NULL) {
		CloseClipboard();
		if (*err_str != TEXT('\0')) {
			_SetForegroundWindow(hWnd);
			MessageBox(hWnd, err_str, ERROR_TITLE, MB_ICONERROR);
		}
		return TRUE;
	}
	CloseClipboard();

	// 履歴に追加
	if (history_add(&history_data.child, di, (cp_tmi.enable == TRUE) ? FALSE : TRUE) == FALSE) {
		data_free(di);
		return TRUE;
	}
	// 履歴に追加された時に実行するツール
	tool_execute_all(hWnd, CALLTYPE_ADD_HISTORY, di);

	// メニューからツール実行
	if (cp_tmi.enable == TRUE &&
		(!(tool_execute(hWnd, cp_tmi.ti, CALLTYPE_MENU, di, NULL) & TOOL_CANCEL) ||
		window_paste_check(GetForegroundWindow()) == TRUE) &&
		data_check(&history_data, di) != NULL) {

		data_delete(&history_data.child, di, FALSE);
		// クリップボードにデータを送る
		SendMessage(hWnd, WM_ITEM_TO_CLIPBOARD, 0, (LPARAM)di);
		data_free(di);
		if (cp_tmi.paste != 0 && cp_tmi.ti != NULL && cp_tmi.ti->copy_paste == 1) {
			// キーを離すまで待機
			key_wait();
			// ホットキーの解除
			unregist_hotkey(hWnd);
			// 貼り付け
			sendkey_paste(GetForegroundWindow());
			// ホットキーの登録
			regist_hotkey(hWnd, FALSE);
		}
	}

	// タスクトレイのツールチップ設定
	set_tray_tooltip(hWnd);
	if (option.history_save == 1 && option.history_always_save == 1) {
		// 履歴の保存
		SendMessage(hWnd, WM_HISTORY_SAVE, 0, 0);
	}
	// 履歴の変化を通知
	SendMessage(hWnd, WM_HISTORY_CHANGED, 0, 0);
	return TRUE;
}

/*
 * item_to_clipboard - アイテムをクリップボードに送る
 */
static BOOL item_to_clipboard(const HWND hWnd, DATA_INFO *from_di, const BOOL delete_flag)
{
	DATA_INFO *di;
	TCHAR err_str[BUF_SIZE];
	int call_type = CALLTYPE_ITEM_TO_CLIPBOARD;

	// データのコピー
	if ((di = data_item_copy(from_di, FALSE, FALSE, err_str)) == NULL) {
		if (*err_str != TEXT('\0')) {
			_SetForegroundWindow(hWnd);
			MessageBox(hWnd, err_str, ERROR_TITLE, MB_ICONERROR);
		}
		return FALSE;
	}

	if (data_check(&history_data, from_di) != NULL) {
		call_type |= CALLTYPE_HISTORY;
	} else if (data_check(&regist_data, from_di) != NULL) {
		call_type |= CALLTYPE_REGIST;
		// 登録アイテムを履歴に入れない
		if (option.history_ignore_regist_item == 1) {
			clip_flag = TRUE;
		}
	}
	// データをクリップボードに送る時に実行するツール
	if (tool_execute_all(hWnd, call_type, di) & TOOL_CANCEL) {
		data_free(di);
		return FALSE;
	}

	// クリップボードに送る
	*err_str = TEXT('\0');
	if (clipboard_set_datainfo(hWnd, di, err_str) == FALSE &&
		*err_str != TEXT('\0')) {
		clip_flag = FALSE;
		data_free(di);
		_SetForegroundWindow(hWnd);
		MessageBox(hWnd, err_str, ERROR_TITLE, MB_ICONERROR);
		return FALSE;
	}
	data_free(di);

	if (clip_flag == TRUE) {
		clip_flag = FALSE;
		if (hViewerWnd != NULL) {
			// クリップボードの変化を通知
			SendMessage(hViewerWnd, WM_VIEWER_CHANGE_CLIPBOARD, 0, 0);
		}
	}
	if ((call_type & CALLTYPE_HISTORY) &&
		delete_flag == TRUE && option.history_delete == 1 && from_di->type == TYPE_ITEM &&
		window_ignore_check(GetForegroundWindow()) == TRUE) {
		// 履歴削除
		if (data_delete(&history_data.child, from_di, TRUE) == TRUE) {
			// 履歴の変化を通知
			SendMessage(hWnd, WM_HISTORY_CHANGED, 0, 0);
		}
	}
	return TRUE;
}

/*
 * load_history - 履歴の読み込み
 */
static BOOL load_history(const HWND hWnd, const int load_flag)
{
	TCHAR path[MAX_PATH];
	TCHAR err_str[BUF_SIZE + MAX_PATH];

	history_data.type = TYPE_ROOT;

	// 履歴の読み込み
	if (load_flag != 0 || option.history_save == 1) {
		wsprintf(path, TEXT("%s\\%s"), work_path, HISTORY_FILENAME);
		*err_str = TEXT('\0');
		if (file_read_data(path, &history_data.child, err_str) == FALSE && *err_str != TEXT('\0')) {
			_SetForegroundWindow(hWnd);
			lstrcat(err_str, TEXT("\r\n"));
			lstrcat(err_str, path);
			MessageBox(hWnd, err_str, ERROR_TITLE, MB_ICONERROR);
			return FALSE;
		}
	}
	return TRUE;
}

/*
 * load_regist - 登録アイテムの読み込み
 */
static BOOL load_regist(const HWND hWnd)
{
	TCHAR path[MAX_PATH];
	TCHAR err_str[BUF_SIZE + MAX_PATH];

	regist_data.type = TYPE_ROOT;

	// 登録アイテムの読み込み
	wsprintf(path, TEXT("%s\\%s"), work_path, REGIST_FILENAME);
	*err_str = TEXT('\0');
	if (file_read_data(path, &regist_data.child, err_str) == FALSE && *err_str != TEXT('\0')) {
		_SetForegroundWindow(hWnd);
		lstrcat(err_str, TEXT("\r\n"));
		lstrcat(err_str, path);
		MessageBox(hWnd, err_str, ERROR_TITLE, MB_ICONERROR);
		return FALSE;
	}
	return TRUE;
}

/*
 * save_history - 履歴の保存
 */
static BOOL save_history(const HWND hWnd, const int save_flag)
{
	DATA_INFO *di;
	TCHAR path[MAX_PATH];
	TCHAR err_str[BUF_SIZE + MAX_PATH];

	if (save_flag == 0 && option.history_save == 0) {
		// 履歴を保存しない
		wsprintf(path, TEXT("%s\\%s"), work_path, HISTORY_FILENAME);
		DeleteFile(path);
		return TRUE;
	}

	// 保存フィルタのチェック
	if (filter_list_save_check(history_data.child) == FALSE) {
		di = history_data.child;
	} else {
		// 保存フィルタをかけたアイテムリストを作成
		if ((di = filter_list_copy(history_data.child, err_str)) == NULL) {
			if (*err_str != TEXT('\0')) {
				_SetForegroundWindow(hWnd);
				MessageBox(hWnd, err_str, ERROR_TITLE, MB_ICONERROR);
				return FALSE;
			}
		}
	}

	// 履歴の保存
	wsprintf(path, TEXT("%s\\%s"), work_path, HISTORY_FILENAME);
	*err_str = TEXT('\0');
	if (file_write_data(path, di, err_str) == FALSE) {
		if (*err_str != TEXT('\0')) {
			_SetForegroundWindow(hWnd);
			lstrcat(err_str, TEXT("\r\n"));
			lstrcat(err_str, path);
			MessageBox(hWnd, err_str, ERROR_TITLE, MB_ICONERROR);
		}
		if (di != history_data.child) {
			data_free(di);
		}
		return FALSE;
	}
	if (di != history_data.child) {
		data_free(di);
	}
	return TRUE;
}

/*
 * save_regist - 登録アイテムの保存
 */
static BOOL save_regist(const HWND hWnd)
{
	TCHAR path[MAX_PATH];
	TCHAR err_str[BUF_SIZE + MAX_PATH];

	// 登録アイテムの保存
	wsprintf(path, TEXT("%s\\%s"), work_path, REGIST_FILENAME);
	*err_str = TEXT('\0');
	if (file_write_data(path, regist_data.child, err_str) == FALSE) {
		if (*err_str != TEXT('\0')) {
			_SetForegroundWindow(hWnd);
			lstrcat(err_str, TEXT("\r\n"));
			lstrcat(err_str, path);
			MessageBox(hWnd, err_str, ERROR_TITLE, MB_ICONERROR);
		}
		return FALSE;
	}
	return TRUE;
}

/*
 * regist_hotkey - ホットキーの登録
 */
static void regist_hotkey(const HWND hWnd, const BOOL show_err)
{
	BOOL hk_err = FALSE;
	int id;
	int i;

	// Action
	for (i = 0; i < option.action_cnt; i++) {
		if ((option.action_info + i)->type == ACTION_TYPE_HOTKEY && (option.action_info + i)->enable != 0 &&
			RegisterHotKey(hWnd, (option.action_info + i)->id,
			(option.action_info + i)->modifiers, (option.action_info + i)->virtkey) == FALSE) {
			hk_err = TRUE;
		}
	}
	// Tool
	for (i = 0; i < option.tool_cnt; i++) {
		if (((option.tool_info + i)->call_type & CALLTYPE_MENU) &&
			(option.tool_info + i)->virtkey != 0 &&
			RegisterHotKey(hWnd, (option.tool_info + i)->id,
			(option.tool_info + i)->modifiers, (option.tool_info + i)->virtkey) == FALSE) {
			hk_err = TRUE;
		}
	}
	// Regist
	id = HKEY_ID + option.action_cnt + option.tool_cnt;
	if (regist_regist_hotkey(hWnd, regist_data.child, &id) == FALSE) {
		hk_err = TRUE;
	}

	if (hk_err == TRUE && option.action_show_hotkey_error == 1 && show_err == TRUE) {
		// 登録エラー
		MessageBox(hWnd, message_get_res(IDS_ERROR_HOTKEY), ERROR_TITLE, MB_ICONERROR);
	}
}

/*
 * unregist_hotkey - ホットキーの解除
 */
static void unregist_hotkey(const HWND hWnd)
{
	int i;

	// Action
	for (i = 0; i < option.action_cnt; i++) {
		if ((option.action_info + i)->type == ACTION_TYPE_HOTKEY && (option.action_info + i)->enable != 0) {
			UnregisterHotKey(hWnd, (option.action_info + i)->id);
		}
	}
	// Tool
	for (i = 0; i < option.tool_cnt; i++) {
		if (((option.tool_info + i)->call_type & CALLTYPE_MENU) && (option.tool_info + i)->virtkey != 0) {
			UnregisterHotKey(hWnd, (option.tool_info + i)->id);
		}
	}
	// Regist
	regist_unregist_hotkey(hWnd, regist_data.child);
}

/*
 * regist_hook - フックの登録
 */
static void regist_hook(const HWND hWnd)
{
	FARPROC SetHook;
	TCHAR err_str[BUF_SIZE];
	int i;

	// フックの登録
	for (i = 0; i < option.action_cnt; i++) {
		if ((option.action_info + i)->enable != 0 &&
			((option.action_info + i)->type == ACTION_TYPE_CTRL_CTRL ||
			(option.action_info + i)->type == ACTION_TYPE_SHIFT_SHIFT ||
			(option.action_info + i)->type == ACTION_TYPE_ALT_ALT)) {
			hook_lib = LoadLibrary(HOOK_LIB);
			if (hook_lib == NULL) {
				message_get_error(GetLastError(), err_str);
				if (*err_str != TEXT('\0')) {
					lstrcat(err_str, TEXT("\r\n"));
					lstrcat(err_str, HOOK_LIB);
					MessageBox(hWnd, err_str, ERROR_TITLE, MB_ICONERROR);
				}
				break;
			}
			SetHook = GetProcAddress(hook_lib, "SetHook");
			if (SetHook == NULL) {
				message_get_error(GetLastError(), err_str);
				if (*err_str != TEXT('\0')) {
					lstrcat(err_str, TEXT("\r\n"));
					lstrcat(err_str, HOOK_LIB);
					MessageBox(hWnd, err_str, ERROR_TITLE, MB_ICONERROR);
				}
				break;
			}
			SetHook(hWnd, WM_KEY_HOOK);
			break;
		}
	}
}

/*
 * unregist_hook - フックの解除
 */
static void unregist_hook(void)
{
	FARPROC UnHook;

	// フックの解除
	if (hook_lib != NULL) {
		UnHook = GetProcAddress(hook_lib, "UnHook");
		if (UnHook != NULL) {
			UnHook();
		}
		FreeLibrary(hook_lib);
		hook_lib = NULL;
	}
}

/*
 * winodw_initialize - ウィンドウの初期化
 */
static BOOL winodw_initialize(const HWND hWnd)
{
	TCHAR err_str[BUF_SIZE + MAX_PATH];

	*err_str = TEXT('\0');

	// 形式情報の初期化
	if (format_initialize(err_str) == FALSE && *err_str != TEXT('\0')) {
		_SetForegroundWindow(hWnd);
		MessageBox(hWnd, err_str, ERROR_TITLE, MB_ICONERROR);
	}
	// ツール情報の初期化
	if (tool_initialize(err_str) == FALSE && *err_str != TEXT('\0')) {
		_SetForegroundWindow(hWnd);
		MessageBox(hWnd, err_str, ERROR_TITLE, MB_ICONERROR);
	}
	// 履歴の読み込み
	if (load_history(hWnd, 0) == FALSE) {
		return FALSE;
	}
	// 登録アイテムの読み込み
	if (load_regist(hWnd) == FALSE) {
		return FALSE;
	}

	// ツールチップの作成
	hToolTip = tooltip_create(hInst);

	// タスクトレイにアイコンを登録
	if (GetAwareness() != PROCESS_DPI_UNAWARE && GetScale() >= 300) {
		icon_clip = LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON_CLIP),
			IMAGE_ICON, 48, 48, 0);
		icon_clip_ban = LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON_CLIP_BAN),
			IMAGE_ICON, 48, 48, 0);
	}
	else if (GetAwareness() != PROCESS_DPI_UNAWARE && GetScale() >= 150) {
		icon_clip = LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON_CLIP),
			IMAGE_ICON, 32, 32, 0);
		icon_clip_ban = LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON_CLIP_BAN),
			IMAGE_ICON, 32, 32, 0);
	}
	else {
		icon_clip = LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON_CLIP),
			IMAGE_ICON, 16, 16, 0);
		icon_clip_ban = LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON_CLIP_BAN),
			IMAGE_ICON, 16, 16, 0);
	}
	icon_tray = (option.main_clipboard_watch == 1) ? icon_clip : icon_clip_ban;
	set_tray_icon(hWnd, icon_tray, MAIN_WINDOW_TITLE);

	// メニューに表示するアイコンの読み込み
	icon_menu_default = LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON_DEFAULT),
		IMAGE_ICON, SICONSIZE, SICONSIZE, 0);
	icon_menu_folder = LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON_FOLDER),
		IMAGE_ICON, SICONSIZE, SICONSIZE, 0);

	// クリップボード監視開始
	if (option.main_clipboard_watch == 1) {
		hClipNextWnd = SetClipboardViewer(hWnd);
		SetTimer(hWnd, ID_RECHAIN_TIMER, RECHAIN_INTERVAL, NULL);
	}

	// ホットキーの登録
	regist_hotkey(hWnd, TRUE);
	// フックの登録
	regist_hook(hWnd);

	// 起動時に実行するツール
	tool_execute_all(hWnd, CALLTYPE_START, NULL);

	// ビューア表示
	if (option.main_show_viewer == 1) {
		SendMessage(hWnd, WM_COMMAND, ID_MENUITEM_VIEWER, 0);
	}
	// コマンドライン処理
	commnad_line_func(FindWindow(MAIN_WND_CLASS, MAIN_WINDOW_TITLE));
	return TRUE;
}

/*
 * winodw_reset - 設定再読み込み
 */
static BOOL winodw_reset(const HWND hWnd)
{
	TCHAR err_str[BUF_SIZE];
	BOOL show_viewer = FALSE;
	int show_flag = SW_SHOW;

	if (hViewerWnd != NULL) {
		show_viewer = TRUE;
		if (IsIconic(hViewerWnd) == TRUE) {
			show_flag = SW_MINIMIZE;
		} else if (IsZoomed(hViewerWnd) == TRUE) {
			show_flag = SW_SHOWMAXIMIZED;
		}
		SendMessage(hViewerWnd, WM_CLOSE, 0, 0);
	}

	// ホットキーの解除
	unregist_hotkey(hWnd);
	// フックの解除
	unregist_hook();

	// アイテムのメニュー情報を解放
	data_menu_free(history_data.child);
	data_menu_free(regist_data.child);
	// 形式情報の解放
	format_free();

	// 設定の解放
	ini_free();
	// 設定の読み込み
	get_work_path(hInst);
	if (ini_get_option(err_str) == FALSE) {
		MessageBox(hWnd, err_str, ERROR_TITLE, MB_ICONERROR);
		return FALSE;
	}

	// 形式情報の初期化
	if (format_initialize(err_str) == FALSE && *err_str != TEXT('\0')) {
		_SetForegroundWindow(hWnd);
		MessageBox(hWnd, err_str, ERROR_TITLE, MB_ICONERROR);
	}
	// ツール情報の初期化
	if (tool_initialize(err_str) == FALSE && *err_str != TEXT('\0')) {
		_SetForegroundWindow(hWnd);
		MessageBox(hWnd, err_str, ERROR_TITLE, MB_ICONERROR);
	}

	// ホットキーの登録
	regist_hotkey(hWnd, TRUE);
	// フックの登録
	regist_hook(hWnd);

	SetTimer(hWnd, ID_RECHAIN_TIMER, RECHAIN_INTERVAL, NULL);

	if (hViewerWnd == NULL && show_viewer == TRUE) {
		hViewerWnd = (HWND)-1;
		hViewerWnd = viewer_create(hWnd, show_flag);
		_SetForegroundWindow(hViewerWnd);
	}
	return TRUE;
}

/*
 * winodw_save - ウィンドウの保存処理
 */
static BOOL winodw_save(const HWND hWnd)
{
	// 終了時に実行するツール
	tool_execute_all(hWnd, CALLTYPE_END, NULL);

	// 登録アイテムの保存
	if (save_regist(hWnd) == FALSE &&
		MessageBox(hWnd, message_get_res(IDS_ERROR_END), ERROR_TITLE, MB_ICONQUESTION | MB_YESNO) == IDNO) {
		return FALSE;
	}
	// 履歴の保存
	if (save_history(hWnd, 0) == FALSE &&
		MessageBox(hWnd, message_get_res(IDS_ERROR_END), ERROR_TITLE, MB_ICONQUESTION | MB_YESNO) == IDNO) {
		return FALSE;
	}
	// 設定の保存
	ini_put_option();
	return TRUE;
}

/*
 * winodw_end - ウィンドウの終了処理
 */
static BOOL winodw_end(const HWND hWnd)
{
	if (hViewerWnd != NULL) {
		SendMessage(hViewerWnd, WM_CLOSE, 0, 0);
	}
	if (hToolTip != NULL) {
		tooltip_close(hToolTip);
		hToolTip = NULL;
	}

	// クリップボード監視解除
	KillTimer(hWnd, ID_RECHAIN_TIMER);
	if (option.main_clipboard_watch == 1) {
		ChangeClipboardChain(hWnd, hClipNextWnd);
		hClipNextWnd = NULL;
	}

	// ホットキーの解除
	unregist_hotkey(hWnd);
	// フックの解除
	unregist_hook();

	// 履歴の解放
	data_free(history_data.child);
	data_free(regist_data.child);
	// 形式情報の解放
	format_free();

#ifdef OP_XP_STYLE
	theme_free();
#endif

	if (option.main_show_trayicon != 0) {
		tray_message(hWnd, NIM_DELETE, TRAY_ID, NULL, NULL);
	}
	DestroyIcon(icon_clip);
	DestroyIcon(icon_clip_ban);
	DestroyIcon(icon_menu_default);
	DestroyIcon(icon_menu_folder);
	return TRUE;
}

/*
 * main_proc - メインウィンドウプロシージャ
 */
static LRESULT CALLBACK main_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static int key_cnt, key_flag;
	static UINT prev_key;
	static BOOL save_flag = FALSE;
	static UINT WM_TASKBARCREATED;

	switch (msg) {
	case WM_CREATE:
		WM_TASKBARCREATED = RegisterWindowMessage(TEXT("TaskbarCreated"));
		// ウィンドウ作成
		if (winodw_initialize(hWnd) == FALSE) {
			return -1;
		}
		break;

	case WM_QUERYENDSESSION:
		// Windows終了
		if (winodw_save(hWnd) == FALSE) {
			return FALSE;
		}
		save_flag = TRUE;
		return TRUE;

	case WM_ENDSESSION:
		// Windows終了
		winodw_end(hWnd);
		DestroyWindow(hWnd);
		return 0;

	case WM_CLOSE:
		// ウィンドウを閉じる
		if (winodw_save(hWnd) == FALSE) {
			break;
		}
		save_flag = TRUE;
		winodw_end(hWnd);
		DestroyWindow(hWnd);
		break;

	case WM_DESTROY:
		// ウィンドウの破棄
		if (save_flag == FALSE) {
			winodw_save(hWnd);
		}
		PostQuitMessage(0);
		break;

	case WM_MEASUREITEM:
		// メニュー描画設定
		if (wParam == 0) {
			menu_set_drawitem((MEASUREITEMSTRUCT *)lParam);
		}
		break;

	case WM_DRAWITEM:
		// メニュー描画
		if (wParam == 0) {
			HWND menu_wnd;

#ifdef MENU_LAYERER
			menu_wnd = WindowFromDC(((DRAWITEMSTRUCT *)lParam)->hDC);
			set_menu_layerer(menu_wnd, option.menu_alpha);
#endif
			if (((DRAWITEMSTRUCT *)lParam)->itemState & ODS_SELECTED) {
				// ツールチップの表示位置を設定
				menu_sel_pt.x = ((DRAWITEMSTRUCT *)lParam)->rcItem.left +
					(((DRAWITEMSTRUCT *)lParam)->rcItem.right - ((DRAWITEMSTRUCT *)lParam)->rcItem.left) / 2;
				menu_sel_pt.y = ((DRAWITEMSTRUCT *)lParam)->rcItem.top;
				menu_sel_top = ((DRAWITEMSTRUCT *)lParam)->rcItem.bottom - ((DRAWITEMSTRUCT *)lParam)->rcItem.top + 1;

				menu_wnd = WindowFromDC(((DRAWITEMSTRUCT *)lParam)->hDC);
				ClientToScreen(menu_wnd, &menu_sel_pt);
				if (menu_sel_pt.y > GetSystemMetrics(SM_CYSCREEN)) {
					menu_sel_pt.x = menu_sel_pt.y = 0;
					menu_sel_top = 0;
				}
			}
			menu_drawitem((DRAWITEMSTRUCT *)lParam);
		}
		break;

	case WM_MENUCHAR:
		// メニューアクセラレータ
		if (HIWORD(wParam) == MF_POPUP) {
			return menu_accelerator((HMENU)lParam, (TCHAR)LOWORD(wParam));
		}
		break;

	case WM_MENUSELECT:
		// 選択メニューのツールチップ表示
		if (option.menu_show_tooltip == 0 ||
			(UINT)LOWORD(wParam) == 0xFFFF ||
			(UINT)HIWORD(wParam) & MF_SEPARATOR) {
			tooltip_hide(hToolTip);
			break;
		}
		// ツールチップを表示
		show_menu_tooltip(hToolTip, (HMENU)lParam, (UINT)LOWORD(wParam),
			((UINT)HIWORD(wParam) & MF_MOUSESELECT) ? TRUE : FALSE);
		break;

	case WM_EXITMENULOOP:
		// ツールチップを非表示
		tooltip_hide(hToolTip);
		break;

	case WM_CHANGECBCHAIN:
		// クリップボードチェーンの変更
		if ((HWND)wParam == hClipNextWnd && (HWND)lParam != hWnd) {
			hClipNextWnd = (HWND)lParam;
		} else if (hClipNextWnd != NULL && hClipNextWnd != hWnd) {
			SendMessage(hClipNextWnd, msg, wParam, lParam);
		}
		break;

	case WM_DRAWCLIPBOARD:
		// クリップボード監視
		if (hClipNextWnd != NULL && hClipNextWnd != hWnd) {
			SendMessage(hClipNextWnd, msg, wParam, lParam);
		}
		if (clip_flag == TRUE) {
			// 履歴に入れない
			break;
		}
		// 履歴に追加
		SetTimer(hWnd, ID_HISTORY_TIMER, option.history_add_interval, NULL);
		SetTimer(hWnd, ID_RECHAIN_TIMER, RECHAIN_INTERVAL, NULL);
		rechain_cnt = 0;
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case ID_MENUITEM_EXIT:
			// 終了
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			break;

		case ID_MENUITEM_VIEWER:
			// ビューア
			if (hViewerWnd != NULL) {
				if (option.viewer_toggle == 1) {
					SendMessage(hViewerWnd, WM_CLOSE, 0, 0);
					break;
				}
				if (IsIconic(hViewerWnd) != 0) {
					ShowWindow(hViewerWnd, SW_RESTORE);
				}
				_SetForegroundWindow(hViewerWnd);
				break;
			}
			_SetForegroundWindow(hWnd);
			hViewerWnd = (HWND)-1;
			hViewerWnd = viewer_create(hWnd, SW_SHOW);
			_SetForegroundWindow(hViewerWnd);
			break;

		case ID_MENUITEM_OPTION:
			// オプション
			SendMessage(hWnd, WM_OPTION_SHOW, 0, 0);
			break;

		case ID_MENUITEM_CLIPBOARD_WATCH:
			// クリップボード監視切り替え
			SendMessage(hWnd, WM_SET_CLIPBOARD_WATCH, !option.main_clipboard_watch, 0);
			break;
		}
		break;

	case WM_TIMER:
		// タイマー
		switch (wParam) {
		case ID_HISTORY_TIMER:
			// クリップボードのデータを履歴に追加
			if (clipboard_to_history(hWnd) == TRUE && hViewerWnd != NULL) {
				// クリップボードの変化を通知
				SendMessage(hViewerWnd, WM_VIEWER_CHANGE_CLIPBOARD, 0, 0);
			}
			break;

		case ID_RECHAIN_TIMER:
			// クリップボード再監視
			if (option.main_clipboard_watch == 0 ||
				option.main_clipboard_rechain_minute <= 0) {
				KillTimer(hWnd, wParam);
				break;
			}
			rechain_cnt++;
			if (rechain_cnt >= option.main_clipboard_rechain_minute) {
				rechain_cnt = 0;
				if (GetClipboardViewer() == hWnd) {
					// 再監視の必要なし
					break;
				}
				// クリップボード監視解除
				ChangeClipboardChain(hWnd, hClipNextWnd);
				hClipNextWnd = NULL;
				// クリップボード監視開始
				clip_flag = TRUE;
				hClipNextWnd = SetClipboardViewer(hWnd);
				clip_flag = FALSE;
			}
			break;

		case ID_TOOL_TIMER:
			// ツールキャンセル
			KillTimer(hWnd, wParam);
			ZeroMemory(&tmi, sizeof(TOOL_MENU_INFO));
			break;

		case ID_PASTE_TIMER:
			// 登録アイテムを直接貼り付け
			if (paste_di == NULL) {
				KillTimer(hWnd, wParam);
				break;
			}
			if (GetAsyncKeyState(VK_MENU) < 0 ||
				GetAsyncKeyState(VK_CONTROL) < 0 ||
				GetAsyncKeyState(VK_SHIFT) < 0 ||
				GetAsyncKeyState(VK_LWIN) < 0 ||
				GetAsyncKeyState(VK_RWIN) < 0) {
				break;
			}
			KillTimer(hWnd, wParam);

			// データをクリップボードに送る
			SendMessage(hWnd, WM_ITEM_TO_CLIPBOARD, 0, (LPARAM)paste_di);
			if (paste_di->op_paste == 1) {
				// ホットキーの解除
				unregist_hotkey(hWnd);
				// 貼り付け
				sendkey_paste(GetForegroundWindow());
				// ホットキーの登録
				regist_hotkey(hWnd, FALSE);
			}
			paste_di = NULL;
			break;

		case ID_KEY_TIMER:
			// キー２回押し用
			KillTimer(hWnd, wParam);
			if (key_cnt > 1) {
				// キーが２回押された時の動作呼び出し
				switch (prev_key) {
				case VK_CONTROL:
					action_execute(hWnd, ACTION_TYPE_CTRL_CTRL, 0, TRUE);
					break;

				case VK_SHIFT:
					action_execute(hWnd, ACTION_TYPE_SHIFT_SHIFT, 0, TRUE);
					break;

				case VK_MENU:
					action_execute(hWnd, ACTION_TYPE_ALT_ALT, 0, TRUE);
					break;
				}
			}
			key_cnt = 0;
			break;

		case ID_LCLICK_TIMER:
			// タスクトレイ左クリック
			if (GetAsyncKeyState(VK_LBUTTON) < 0) {
				SetTimer(hWnd, ID_LCLICK_TIMER, 1, NULL);
				break;
			}
			KillTimer(hWnd, wParam);
			action_execute(hWnd, ACTION_TYPE_TRAY_LEFT, 0, FALSE);
			break;

		case ID_RCLICK_TIMER:
			// タスクトレイ右クリック
			if (GetAsyncKeyState(VK_RBUTTON) < 0) {
				SetTimer(hWnd, ID_RCLICK_TIMER, 1, NULL);
				break;
			}
			KillTimer(hWnd, wParam);
			action_execute(hWnd, ACTION_TYPE_TRAY_RIGHT, 0, FALSE);
			break;
		}
		break;

	case WM_TRAY_NOTIFY:
		// タスクトレイメッセージ
		switch (LOWORD(lParam)) {
		case WM_LBUTTONDOWN:
			// 左ダブルクリック判定用のタイマー
			SetTimer(hWnd, ID_LCLICK_TIMER,
				(action_check(ACTION_TYPE_TRAY_LEFT_DBLCLK) == TRUE) ? GetDoubleClickTime() : 1, NULL);
			break;

		case WM_LBUTTONDBLCLK:
			KillTimer(hWnd, ID_LCLICK_TIMER);
			action_execute(hWnd, ACTION_TYPE_TRAY_LEFT_DBLCLK, 0, FALSE);
			break;

		case WM_RBUTTONDOWN:
			// 右ダブルクリック判定用のタイマー
			SetTimer(hWnd, ID_RCLICK_TIMER,
				(action_check(ACTION_TYPE_TRAY_RIGHT_DBLCLK) == TRUE) ? GetDoubleClickTime() : 1, NULL);
			break;

		case WM_RBUTTONDBLCLK:
			KillTimer(hWnd, ID_RCLICK_TIMER);
			action_execute(hWnd, ACTION_TYPE_TRAY_RIGHT_DBLCLK, 0, FALSE);
			break;

		case WM_MOUSEMOVE:
			{
				HWND active_wnd, mouse_wnd;
				POINT pt;

				active_wnd = GetForegroundWindow();
				// タスクバーの判定
				GetCursorPos(&pt);
				mouse_wnd = WindowFromPoint(pt);
				while (mouse_wnd != NULL && mouse_wnd != active_wnd) mouse_wnd = GetParent(mouse_wnd);
				if (active_wnd != focus_info.active_wnd && active_wnd != hWnd && active_wnd != mouse_wnd) {
					// フォーカス情報取得
					get_focus_info(&focus_info);
				}
			}
			break;
		}
		break;

	case WM_KEY_HOOK:
		// キーボードフック
		switch (wParam) {
		case VK_CONTROL:
		case VK_SHIFT:
		case VK_MENU:
			if (prev_key != wParam) {
				prev_key = wParam;
				if (key_flag == 1) {
					key_flag = -1;
				}
				key_cnt = 0;
			}
			if ((lParam & 0x80000000) != 0) {
				// key up
				if (key_flag == 1) {
					key_cnt++;
					SetTimer(hWnd, ID_KEY_TIMER, ((key_cnt == 1) ? option.action_double_press_time : 1), NULL);
				}
				key_flag = 0;
			} else if (key_flag == 0) {
				// key down
				key_flag = 1;
			}
			break;

		default:
			// 他のキーが押された場合は無効にする
			if (key_flag == 1) {
				key_flag = -1;
			}
			key_cnt = 0;
			break;
		}
		break;

	case WM_HOTKEY:
		// ホットキー
		action_execute(hWnd, ACTION_TYPE_HOTKEY, (int)wParam, TRUE);
		break;

	case WM_VIEWER_NOTIFY_CLOSE:
		// ビューア終了通知
		hViewerWnd = NULL;
		break;

	case WM_GET_VERSION:
		// バージョン取得
		return APP_VAR;

	case WM_GET_WORKPATH:
		// 作業ディレクトリ取得
		if (lParam == 0) {
			break;
		}
		lstrcpy((TCHAR *)lParam, work_path);
		break;

	case WM_GET_CLIPBOARD_WATCH:
		// クリップボード監視状態の取得
		return option.main_clipboard_watch;

	case WM_SET_CLIPBOARD_WATCH:
		// クリップボード監視切り替え
		ZeroMemory(&tmi, sizeof(TOOL_MENU_INFO));
		if (wParam != 0) {
			option.main_clipboard_watch = 1;
			icon_tray = icon_clip;
			// クリップボード監視開始
			hClipNextWnd = SetClipboardViewer(hWnd);
			SetTimer(hWnd, ID_RECHAIN_TIMER, RECHAIN_INTERVAL, NULL);
		} else {
			option.main_clipboard_watch = 0;
			icon_tray = icon_clip_ban;
			// クリップボード監視解除
			KillTimer(hWnd, ID_RECHAIN_TIMER);
			ChangeClipboardChain(hWnd, hClipNextWnd);
			hClipNextWnd = NULL;
		}
		set_tray_tooltip(hWnd);
		if (hViewerWnd != NULL) {
			SendMessage(hViewerWnd, WM_VIEWER_CHANGE_WATCH, 0, 0);
		}
		break;

	case WM_GET_FORMAT_ICON:
		// 形式用アイコンの取得
		if (lParam != 0) {
			HICON hIcon, ret;
			BOOL free_icon = TRUE;

			hIcon = format_get_icon(format_get_index((TCHAR *)lParam, 0), wParam, &free_icon);
			if (hIcon == NULL) {
				return 0;
			}
			ret = CopyIcon(hIcon);
			if (free_icon == TRUE) {
				DestroyIcon(hIcon);
			}
			return (LRESULT)ret;
		}
		return 0;

	case WM_ENABLE_ACCELERATOR:
		// アクセラレータの有効・無効の切り替え
		accel_flag = (BOOL)wParam;
		break;

	case WM_REGIST_HOTKEY:
		// ホットキーの登録
		regist_hotkey(hWnd, TRUE);
		break;

	case WM_UNREGIST_HOTKEY:
		// ホットキーの解除
		unregist_hotkey(hWnd);
		break;

	case WM_OPTION_SHOW:
		// オプション表示
		{
			TCHAR buf[MAX_PATH];

			wsprintf(buf, TEXT("%s\\%s"), app_path, OPTION_EXE);
			shell_open(buf, (TCHAR *)lParam);
		}
		break;

	case WM_OPTION_GET:
		// オプションの取得
		return (LRESULT)&option;

	case WM_OPTION_LOAD:
		// 設定の読み込み
		winodw_reset(hWnd);
		break;

	case WM_OPTION_SAVE:
		// 設定の保存
		ini_put_option();
		break;

	case WM_HISTORY_CHANGED:
		// 履歴の内容変化
		if (hViewerWnd != NULL) {
			return SendMessage(hViewerWnd, msg, wParam, lParam);
		} else {
			data_adjust(&history_data.child);
		}
		break;

	case WM_HISTORY_GET_ROOT:
		// 履歴アイテムの取得
		return (LRESULT)&history_data;

	case WM_HISTORY_LOAD:
		// 履歴の読み込み
		if (wParam == 0 && option.history_save == 0) {
			return TRUE;
		}
		data_free(history_data.child);
		history_data.child = NULL;
		return load_history(hWnd, wParam);

	case WM_HISTORY_SAVE:
		// 履歴の保存
		return save_history(hWnd, wParam);

	case WM_REGIST_CHANGED:
		// 登録アイテムの内容変化
		if (hViewerWnd != NULL) {
			return SendMessage(hViewerWnd, msg, wParam, lParam);
		} else {
			data_adjust(&regist_data.child);
		}
		break;

	case WM_REGIST_GET_ROOT:
		// 登録アイテムの取得
		return (LRESULT)&regist_data;

	case WM_REGIST_LOAD:
		// 登録アイテムの読み込み
		data_free(regist_data.child);
		regist_data.child = NULL;
		return load_regist(hWnd);

	case WM_REGIST_SAVE:
		// 登録アイテムの保存
		return save_regist(hWnd);

	case WM_ITEM_TO_CLIPBOARD:
		// データをクリップボードに設定
		if (lParam == 0) {
			return FALSE;
		}
		return item_to_clipboard(hWnd, (DATA_INFO *)lParam, (wParam == 0) ? TRUE : FALSE);

	case WM_ITEM_CREATE:
		// アイテムの作成
		switch (wParam) {
		case TYPE_DATA:
			// データの作成
			if (lParam == 0) {
				return 0;
			}
			return (LRESULT)data_create_data(0, (TCHAR *)lParam, NULL, 0, TRUE, NULL);
		case TYPE_ITEM:
			// アイテムの作成
			return (LRESULT)data_create_item((TCHAR *)lParam, TRUE, NULL);
		case TYPE_FOLDER:
			// フォルダの作成
			if (lParam == 0) {
				return 0;
			}
			return (LRESULT)data_create_folder((TCHAR *)lParam, NULL);
		}
		return 0;

	case WM_ITEM_COPY:
		// アイテムのコピー
		if (lParam == 0) {
			return 0;
		}
		return (LRESULT)data_item_copy((DATA_INFO *)lParam, (BOOL)wParam, FALSE, NULL);

	case WM_ITEM_FREE:
		// アイテムの解放
		if ((DATA_INFO *)lParam == &history_data ||
			(DATA_INFO *)lParam == &regist_data) {
			break;
		}
		data_free((DATA_INFO *)lParam);
		break;

	case WM_ITEM_FREE_DATA:
		// データの解放
		if (wParam == 0 || lParam == 0) {
			break;
		}
		if (format_free_data((TCHAR *)wParam, (HANDLE)lParam) == FALSE) {
			clipboard_free_data((TCHAR *)wParam, (HANDLE)lParam);
		}
		break;

	case WM_ITEM_CHECK:
		// アイテムの存在チェック
		if (data_check(&history_data, (DATA_INFO *)lParam) != NULL) {
			return 0;
		}
		if (data_check(&regist_data, (DATA_INFO *)lParam) != NULL) {
			return 1;
		}
		return -1;

	case WM_ITEM_TO_BYTES:
		// アイテムからバイト列を取得
		if (lParam != 0) {
			BYTE *ret;

			if ((ret = format_data_to_bytes((DATA_INFO *)lParam, (DWORD *)wParam)) == NULL) {
				ret = clipboard_data_to_bytes((DATA_INFO *)lParam, (DWORD *)wParam);
			}
			return (LRESULT)ret;
		}
		break;

	case WM_ITEM_FROM_BYTES:
		// バイト列からデータを作成しアイテムに設定
		if (lParam == 0) {
			break;
		}
		if (((DATA_INFO *)lParam)->data != NULL) {
			if (format_free_data(((DATA_INFO *)lParam)->format_name, ((DATA_INFO *)lParam)->data) == FALSE) {
				clipboard_free_data(((DATA_INFO *)lParam)->format_name, ((DATA_INFO *)lParam)->data);
			}
		}
		if ((((DATA_INFO *)lParam)->data = format_bytes_to_data(((DATA_INFO *)lParam)->format_name,
			(BYTE *)wParam, &((DATA_INFO *)lParam)->size)) == NULL) {

			((DATA_INFO *)lParam)->data = clipboard_bytes_to_data(((DATA_INFO *)lParam)->format_name,
				(BYTE *)wParam, &((DATA_INFO *)lParam)->size);
		}
		break;

	case WM_ITEM_TO_FILE:
		// アイテムをファイルに保存
		if (lParam != 0) {
			TCHAR err_str[BUF_SIZE];

			*err_str = TEXT('\0');
			if (format_data_to_file((DATA_INFO *)lParam, (TCHAR *)wParam, 0, err_str) == FALSE) {
				if (*err_str != TEXT('\0')) {
					return FALSE;
				}
				if (clipboard_data_to_file((DATA_INFO *)lParam, (TCHAR *)wParam, 0, err_str) == FALSE && *err_str != TEXT('\0')) {
					return FALSE;
				}
			}
			return TRUE;
		}
		return FALSE;

	case WM_ITEM_FROM_FILE:
		// ファイルからデータを作成してアイテムに設定
		if (lParam != 0) {
			TCHAR err_str[BUF_SIZE];

			if (((DATA_INFO *)lParam)->data != NULL) {
				if (format_free_data(((DATA_INFO *)lParam)->format_name, ((DATA_INFO *)lParam)->data) == FALSE) {
					clipboard_free_data(((DATA_INFO *)lParam)->format_name, ((DATA_INFO *)lParam)->data);
				}
			}
			*err_str = TEXT('\0');
			if ((((DATA_INFO *)lParam)->data = format_file_to_data((TCHAR *)wParam,
				((DATA_INFO *)lParam)->format_name,
				&((DATA_INFO *)lParam)->size, err_str)) == NULL) {
				if (*err_str != TEXT('\0')) {
					return FALSE;
				}
				if ((((DATA_INFO *)lParam)->data = clipboard_file_to_data((TCHAR *)wParam,
					((DATA_INFO *)lParam)->format_name,
					&((DATA_INFO *)lParam)->size, err_str)) == NULL && *err_str != TEXT('\0')) {
					return FALSE;
				}
			}
			return TRUE;
		}
		return FALSE;

	case WM_ITEM_GET_PARENT:
		// 親アイテムの取得
		{
			DATA_INFO *di;

			if ((di = data_check(&history_data, (DATA_INFO *)lParam)) != NULL) {
				return (LRESULT)di;
			}
			if ((di = data_check(&regist_data, (DATA_INFO *)lParam)) != NULL) {
				return (LRESULT)di;
			}
		}
		return (LRESULT)NULL;

	case WM_ITEM_GET_FORMAT_TO_ITEM:
		// 形式名からアイテムを取得
		if (lParam != 0 && wParam != 0) {
			DATA_INFO *di = (DATA_INFO *)lParam;

			if (di->type == TYPE_ITEM) {
				for (di = di->child; di != NULL && lstrcmpi(di->format_name, (TCHAR *)wParam) != 0; di = di->next)
					;
				return (LRESULT)di;

			} else if (di->type == TYPE_DATA) {
				if (lstrcmpi(di->format_name, (TCHAR *)wParam) != 0) {
					return (LRESULT)di;
				}
			}
		}
		return (LRESULT)NULL;

	case WM_ITEM_GET_PRIORITY_HIGHEST:
		// 優先順位の高い形式のアイテムを選択
		return (LRESULT)format_get_priority_highest((DATA_INFO *)lParam);

	case WM_ITEM_GET_TITLE:
		// アイテムのタイトル取得
		if (lParam != 0) {
			DATA_INFO *di;

			di = format_get_priority_highest((DATA_INFO *)lParam);
			data_menu_free_item(di);
			// メニューに表示するタイトルを取得
			format_get_menu_title(di);
			// メニューに表示するアイコンを取得
			format_get_menu_icon(di);
			// メニューに表示するビットマップを取得
			format_get_menu_bitmap(di);

			if ((TCHAR *)wParam != NULL) {
				lstrcpy((TCHAR *)wParam, data_get_title(di));
			}
		}
		break;

	case WM_ITEM_GET_OPEN_INFO:
		// アイテムを開く情報
		return format_get_file_info((TCHAR *)lParam, NULL, (OPENFILENAME *)wParam, TRUE);

	case WM_ITEM_GET_SAVE_INFO:
		// アイテムの保存情報
		return format_get_file_info(((DATA_INFO *)lParam)->format_name, (DATA_INFO *)lParam,
			(OPENFILENAME *)wParam, FALSE);

	case WM_VIEWER_SHOW:
		// ビューア表示
		SendMessage(hWnd, WM_COMMAND, ID_MENUITEM_VIEWER, 0);
		break;

	case WM_VIEWER_GET_HWND:
		// ビューアのウィンドウハンドルを取得
		return (LRESULT)hViewerWnd;

	case WM_VIEWER_GET_MAIN_HWND:
		// 本体のウィンドウハンドルを取得
		return (LRESULT)hWnd;

	case WM_VIEWER_GET_SELECTION:
		// 選択アイテムを取得
		if (hViewerWnd != NULL) {
			return SendMessage(hViewerWnd, msg, wParam, lParam);
		}
		return (LRESULT)NULL;

	case WM_VIEWER_SELECT_ITEM:
		// ツリーアイテムを選択
		if (hViewerWnd != NULL) {
			return SendMessage(hViewerWnd, msg, wParam, lParam);
		}
		return FALSE;

	default:
		if (msg == WM_TASKBARCREATED) {
			set_tray_icon(hWnd, icon_tray, MAIN_WINDOW_TITLE);
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

/*
 * copy_old_file - 旧バージョンのファイルを移行
 */
static void copy_old_file()
{
	TCHAR general_ini_path[MAX_PATH];
	TCHAR tmp_path[MAX_PATH];
	TCHAR user_name[BUF_SIZE];
	TCHAR buf[BUF_SIZE];
	DWORD i;

	lstrcpy(tmp_path, app_path);

	// 現在のログインユーザ名の取得
	i = BUF_SIZE - 1;
	if (GetUserName(user_name, &i) == FALSE) {
		lstrcpy(user_name, DEFAULT_USER);
	}

	// 共通の設定を使用する場合のユーザ名の取得
	wsprintf(general_ini_path, TEXT("%s\\%s"), app_path, GENERAL_INI);
	if (PathFileExists(general_ini_path) == FALSE) {
		if (!SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, tmp_path))) {
			return;
		}
		lstrcat(tmp_path, TEXT("\\VirtualStore\\Program Files (x86)\\CLCL"));
		wsprintf(general_ini_path, TEXT("%s\\%s"), tmp_path, GENERAL_INI);
		if (PathFileExists(general_ini_path) == FALSE) {
			return;
		}
	}

	profile_initialize(general_ini_path, TRUE);
	profile_get_string(TEXT("GENERAL"), TEXT("User"), TEXT(""), buf, BUF_SIZE - 1, general_ini_path);
	profile_write_string(TEXT("GENERAL"), TEXT("User"), buf, general_ini_path);
	if (*buf != TEXT('\0')) {
		lstrcpy(user_name, buf);
	}
	profile_get_string(TEXT("GENERAL"), TEXT("WorkDir"), TEXT(""), buf, BUF_SIZE - 1, general_ini_path);
	profile_write_string(TEXT("GENERAL"), TEXT("WorkDir"), buf, general_ini_path);
	if (*buf != TEXT('\0')) {
		lstrcpy(tmp_path, buf);
	}
	profile_flush(general_ini_path);
	profile_free();

	file_name_conv(user_name, TEXT('_'));

	TCHAR old_path[MAX_PATH];
	wsprintf(old_path, TEXT("%s\\%s"), tmp_path, user_name);

	TCHAR old_file[MAX_PATH];
	TCHAR new_file[MAX_PATH];

	wsprintf(old_file, TEXT("%s\\%s"), old_path, USER_INI);
	wsprintf(new_file, TEXT("%s\\%s"), work_path, USER_INI);
	CopyFile(old_file, new_file, TRUE);

	wsprintf(old_file, TEXT("%s\\%s"), old_path, HISTORY_FILENAME);
	wsprintf(new_file, TEXT("%s\\%s"), work_path, HISTORY_FILENAME);
	CopyFile(old_file, new_file, TRUE);

	wsprintf(old_file, TEXT("%s\\%s"), old_path, REGIST_FILENAME);
	wsprintf(new_file, TEXT("%s\\%s"), work_path, REGIST_FILENAME);
	CopyFile(old_file, new_file, TRUE);
}

/*
 * get_work_path - 作業ディレクトリの作成
 */
static void get_work_path(const HINSTANCE hInstance)
{
	TCHAR *p, *r;

	// アプリケーションのパスを取得
	GetModuleFileName(hInstance, app_path, MAX_PATH - 1);
	for (p = r = app_path; *p != TEXT('\0'); p++) {
#ifndef UNICODE
		if (IsDBCSLeadByte((BYTE)*p) == TRUE) {
			p++;
			continue;
		}
#endif	// UNICODE
		if (*p == TEXT('\\') || *p == TEXT('/')) {
			r = p;
		}
	}
	*r = TEXT('\0');

	int portable = 0;
	TCHAR app_ini_path[MAX_PATH];
	wsprintf(app_ini_path, TEXT("%s\\%s"), app_path, APP_INI);
	if (PathFileExists(app_ini_path) == TRUE) {
		profile_initialize(app_ini_path, TRUE);
		portable = profile_get_int(TEXT("GENERAL"), TEXT("portable"), 0, app_ini_path);
		profile_free();
	}
	if (portable == 1) {
		lstrcpy(work_path, app_path);
	}
	else {
		BOOL check_old_path = FALSE;
		if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, work_path))) {
			lstrcat(work_path, TEXT("\\CLCL"));
			if (PathFileExists(work_path) == FALSE) {
				check_old_path = TRUE;
			}
			CreateDirectory(work_path, NULL);
		}
		if (check_old_path) {
			copy_old_file();
		}
	}
}

/*
 * commnad_line_func - コマンドライン処理
 */
static void commnad_line_func(const HWND hWnd)
{
	HWND vWnd;
	TCHAR *p;

	if (hWnd == NULL) {
		return;
	}

	p = GetCommandLine();
	// 実行ファイル名の除去
    if (*p == TEXT('"')) {
		for (p++; *p != TEXT('\0') && *p != TEXT('"'); p++)
			;
		if (*p != TEXT('\0')) {
			p++;
		}
	} else {
		for (; *p != TEXT('\0') && *p != TEXT(' '); p++)
			;
	}

	for (; *p != TEXT('\0') && *p != TEXT('/') && *p != TEXT('-'); p++)
		;
	if (*p == TEXT('\0')) {
		return;
	}
	for (p++; *p != TEXT('\0'); p++) {
		if (*p == TEXT(' ')) {
			for (; *p != TEXT('\0') && *p != TEXT('/') && *p != TEXT('-'); p++)
				;
			if (*p == TEXT('\0') || *(++p) == TEXT('\0')) {
				break;
			}
		}
		switch (*p) {
		case TEXT('v'): case TEXT('V'):
			// ビューア表示
			vWnd = (HWND)SendMessage(hWnd, WM_VIEWER_GET_HWND, 0, 0);
			if (vWnd == NULL) {
				SendMessage(hWnd, WM_VIEWER_SHOW, 0, 0);
			} else {
				if (IsIconic(vWnd) != 0) {
					ShowWindow(vWnd, SW_RESTORE);
				}
				_SetForegroundWindow(vWnd);
			}
			break;

		case TEXT('w'): case TEXT('W'):
			// クリップボード監視
			SendMessage(hWnd, WM_SET_CLIPBOARD_WATCH, 1, 0);
			break;

		case TEXT('n'): case TEXT('N'):
			// クリップボード監視解除
			SendMessage(hWnd, WM_SET_CLIPBOARD_WATCH, 0, 0);
			break;

		case TEXT('x'): case TEXT('X'):
			// 終了
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			break;
		}
	}
}

/*
 * init_application - ウィンドウクラスの登録
 */
static BOOL init_application(const HINSTANCE hInstance)
{
	WNDCLASS wc;

	wc.style = 0;
	wc.lpfnWndProc = (WNDPROC)main_proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON_MAIN));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = MAIN_WND_CLASS;
	// ウィンドウクラスの登録
	return RegisterClass(&wc);
}

/*
 * init_instance - ウィンドウの作成
 */
static HWND init_instance(const HINSTANCE hInstance, const int CmdShow)
{
	HWND hWnd;

	// ウィンドウの作成
	hWnd = CreateWindow(MAIN_WND_CLASS,
		MAIN_WINDOW_TITLE,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL);
	return hWnd;
}

/*
 * WinMain - メイン
 */
int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	HANDLE hMutex = NULL;
	HANDLE hAccel;
	MSG msg;
	TCHAR err_str[BUF_SIZE];
#ifndef _DEBUG
	SECURITY_DESCRIPTOR sd;
	SECURITY_ATTRIBUTES sa;
#endif

	hInst = hInstance;

#ifndef _DEBUG
	// 2重起動チェック
	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&sd, TRUE, 0, FALSE);	    
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = &sd;
	sa.bInheritHandle = TRUE; 
	hMutex = CreateMutex(&sa, FALSE, MUTEX);
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		// コマンドライン処理
		commnad_line_func(FindWindow(MAIN_WND_CLASS, MAIN_WINDOW_TITLE));
		if (hMutex != NULL) {
			CloseHandle(hMutex);
		}
		return 0;
	}
#endif

	// DPIの初期化
	InitDpi();
	// CommonControlの初期化
	InitCommonControls();
	// OLEの初期化
	OleInitialize(NULL);
	// 設定取得
	get_work_path(hInstance);
	if (ini_get_option(err_str) == FALSE) {
		MessageBox(NULL, err_str, ERROR_TITLE, MB_ICONERROR);
		if (hMutex != NULL) {
			CloseHandle(hMutex);
		}
		return 0;
	}

	// ビューアの登録
	if (viewer_regist(hInstance) == FALSE ||
		container_regist(hInstance) == FALSE || binview_regist(hInstance) == FALSE) {
		MessageBox(NULL, message_get_res(IDS_ERROR_WINDOW_INIT), ERROR_TITLE, MB_ICONERROR);
		if (hMutex != NULL) {
			CloseHandle(hMutex);
		}
		return 0;
	}
	// メインウィンドウの作成
	if (tooltip_regist(hInstance) == FALSE ||
		init_application(hInstance) == FALSE || init_instance(hInstance, nCmdShow) == NULL) {
		MessageBox(NULL, message_get_res(IDS_ERROR_WINDOW_INIT), ERROR_TITLE, MB_ICONERROR);
		if (hMutex != NULL) {
			CloseHandle(hMutex);
		}
		return 0;
	}
	hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR));
	// ウィンドウメッセージ処理
	while (GetMessage(&msg, NULL, 0, 0) == TRUE) {
		if (accel_flag == TRUE && hViewerWnd != NULL && hViewerWnd == GetForegroundWindow() &&
			(TranslateAccelerator(hViewerWnd, hAccel, &msg) == TRUE)) {
			continue;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// 設定の解放
	ini_free();
	OleUninitialize();
	if (hMutex != NULL) {
		CloseHandle(hMutex);
	}
#ifdef _DEBUG
	mem_debug();
#endif
	return msg.wParam;
}
/* End of source */
