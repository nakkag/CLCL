/*
 * CLCLSet
 *
 * CLCLSet.c
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

#include "..\General.h"
#include "..\Memory.h"
#include "..\Profile.h"
#include "..\Ini.h"
#include "..\Message.h"
#include "..\File.h"
#include "..\dpi.h"

#include "CLCLSet.h"
#include "SetHistory.h"
#include "SetMenu.h"
#include "SetViewer.h"
#include "SetAction.h"
#include "SetFormat.h"
#include "SetFilter.h"
#include "SetWindow.h"
#include "SetSendkey.h"
#include "SetTool.h"
#include "SelectKey.h"

#include "resource.h"

/* Define */
#define ERROR_TITLE						TEXT("CLCLSet - Error")
#define MUTEX							TEXT("_CLCLSET_Mutex_")

#define ABS(n)							((n < 0) ? (n * -1) : n)

/* Global Variables */
HINSTANCE hInst;
TCHAR app_path[MAX_PATH];
TCHAR work_path[MAX_PATH];
int prop_ret;

TCHAR cmd_format[BUF_SIZE];
TCHAR cmd_filter[BUF_SIZE];

static HWND sort_listview;

#ifdef OP_XP_STYLE
static HMODULE hModThemes;
#endif	// OP_XP_STYLE

// オプション
extern OPTION_INFO option;

/* Local Function Prototypes */
#ifdef OP_XP_STYLE
static void init_themes(void);
static void themes_free(void);
#endif	// OP_XP_STYLE
static int CALLBACK compare_func(LPARAM lParam1, LPARAM lParam2, LPARAM colum);
static int show_option(const HWND hWnd, const TCHAR *cmd_line);
static void get_work_path(const HINSTANCE hInstance);

/*
 * init_themes - XP用スタイルの初期化
 */
#ifdef OP_XP_STYLE
static void init_themes(void)
{
	hModThemes = LoadLibrary(TEXT("uxtheme.dll"));
}
#endif	// OP_XP_STYLE

/*
 * themes_free - XP用スタイルの解放
 */
#ifdef OP_XP_STYLE
static void themes_free(void)
{
	if (hModThemes != NULL) {
		FreeLibrary(hModThemes);
		hModThemes = NULL;
	}
}
#endif	// OP_XP_STYLE

/*
 * open_theme - XP用スタイルを開く
 */
#ifdef OP_XP_STYLE
long open_theme(const HWND hWnd, const WCHAR *class_name)
{
	static FARPROC _OpenThemeData;

	if (hModThemes == NULL) {
		return 0;
	}
	if (_OpenThemeData == NULL) {
		_OpenThemeData = GetProcAddress(hModThemes, "OpenThemeData");
	}
	if (_OpenThemeData == NULL) {
		return 0;
	}
	return (long)_OpenThemeData(hWnd, class_name);
}
#endif	// OP_XP_STYLE

/*
 * close_theme - XP用スタイルを閉じる
 */
#ifdef OP_XP_STYLE
void close_theme(long hTheme)
{
	static FARPROC _CloseThemeData;

	if (hModThemes == NULL) {
		return;
	}
	if (_CloseThemeData == NULL) {
		_CloseThemeData = GetProcAddress(hModThemes, "CloseThemeData");
	}
	if (_CloseThemeData == NULL) {
		return;
	}
	_CloseThemeData((HTHEME)hTheme);
}
#endif	// OP_XP_STYLE

/*
 * close_theme - XP用スタイルでスクロールバーのボタンの描画
 */
#ifdef OP_XP_STYLE
BOOL draw_theme_scroll(LPDRAWITEMSTRUCT lpDrawItem, UINT i, long hTheme)
{
	static FARPROC _DrawThemeBackground;
	DWORD state = 0;

	if (hModThemes == NULL) {
		return FALSE;
	}
	if (_DrawThemeBackground == NULL) {
		_DrawThemeBackground = GetProcAddress(hModThemes, "DrawThemeBackground");
	}
	if (_DrawThemeBackground == NULL) {
		return FALSE;
	}
	switch (i) {
	case DFCS_SCROLLUP:
		if (lpDrawItem->itemState & ODS_DISABLED) {
			state = ABS_UPDISABLED;
		} else if (lpDrawItem->itemState & ODS_SELECTED) {
			state = ABS_UPPRESSED;
		} else if (lpDrawItem->itemState & ODS_FOCUS) {
			state = ABS_UPHOT;
		} else {
			state = ABS_UPNORMAL;
		}
		break;

	case DFCS_SCROLLDOWN:
		if (lpDrawItem->itemState & ODS_DISABLED) {
			state = ABS_DOWNDISABLED;
		} else if (lpDrawItem->itemState & ODS_SELECTED) {
			state = ABS_DOWNPRESSED;
		} else if (lpDrawItem->itemState & ODS_FOCUS) {
			state = ABS_DOWNHOT;
		} else {
			state = ABS_DOWNNORMAL;
		}
		break;

	case DFCS_SCROLLRIGHT:
		if (lpDrawItem->itemState & ODS_DISABLED) {
			state = ABS_RIGHTDISABLED;
		} else if (lpDrawItem->itemState & ODS_SELECTED) {
			state = ABS_RIGHTPRESSED;
		} else if (lpDrawItem->itemState & ODS_FOCUS) {
			state = ABS_RIGHTHOT;
		} else {
			state = ABS_RIGHTNORMAL;
		}
		break;
	}
	_DrawThemeBackground((HTHEME)hTheme, lpDrawItem->hDC, SBP_ARROWBTN, state, &(lpDrawItem->rcItem), NULL);
	return TRUE;
}
#endif	// OP_XP_STYLE

/*
 * alloc_get_text - EDITに設定されているサイズ分のメモリを確保してEDITの内容を設定する
 */
void alloc_get_text(const HWND hEdit, TCHAR **buf)
{
	int len;

	if (*buf != NULL) {
		mem_free(buf);
	}
	len = SendMessage(hEdit, WM_GETTEXTLENGTH, 0, 0) + 1;
	if ((*buf = mem_alloc(sizeof(TCHAR) * (len + 1))) != NULL) {
		**buf = TEXT('\0');
		SendMessage(hEdit, WM_GETTEXT, len, (LPARAM)*buf);
	}
}

/*
 * file_select - ファイル選択ダイアログの表示
 */
int file_select(const HWND hDlg, const TCHAR *oFilter, const int Index, TCHAR *ret)
{
	OPENFILENAME of;
	TCHAR filename[MAX_PATH];

	*filename = TEXT('\0');

	ZeroMemory(&of, sizeof(OPENFILENAME));
	of.lStructSize = sizeof(OPENFILENAME);
	of.hwndOwner = hDlg;
	of.lpstrFilter = oFilter;
	of.nMaxCustFilter = 40;
	of.nFilterIndex = Index;
	of.lpstrFile = filename;
	of.nMaxFile = BUF_SIZE - 1;
	of.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	if (GetOpenFileName((LPOPENFILENAME)&of) == TRUE) {
		lstrcpy(ret, of.lpstrFile);
		return of.nFilterIndex;
	}
	return -1;
}

/*
 * draw_scroll_sontrol - スクロールバーのボタンの描画
 */
void draw_scroll_sontrol(LPDRAWITEMSTRUCT lpDrawItem, UINT i)
{
	#define FOCUSRECT_SIZE		3

	if (lpDrawItem->itemState & ODS_DISABLED) {
		// 使用不能
		i |= DFCS_INACTIVE;
	}
	if (lpDrawItem->itemState & ODS_SELECTED) {
		// 選択
		i |= DFCS_PUSHED;
	}

	// フレームコントロールの描画
	DrawFrameControl(lpDrawItem->hDC, &(lpDrawItem->rcItem), DFC_SCROLL, i);

	// フォーカス
	if (lpDrawItem->itemState & ODS_FOCUS) {
		lpDrawItem->rcItem.left += FOCUSRECT_SIZE;
		lpDrawItem->rcItem.top += FOCUSRECT_SIZE;
		lpDrawItem->rcItem.right -= FOCUSRECT_SIZE;
		lpDrawItem->rcItem.bottom -= FOCUSRECT_SIZE;
		DrawFocusRect(lpDrawItem->hDC, &(lpDrawItem->rcItem));
	}
}

/*
 * enum_windows_proc - ウィンドウ列挙プロシージャ
 */
BOOL CALLBACK enum_windows_proc(const HWND hWnd, const LPARAM lParam)
{
	const HWND hListView = (const HWND)lParam;
	LV_ITEM lvi;
	TCHAR title[BUF_SIZE];
	TCHAR class_name[BUF_SIZE];
	TCHAR buf[BUF_SIZE];
	int i;

	// 情報取得
	GetWindowText(hWnd, title, BUF_SIZE - 1);
	GetClassName(hWnd, class_name, BUF_SIZE - 1);

	// 既に追加済みかチェック
	for (i = 0; i < ListView_GetItemCount(hListView); i++) {
		ListView_GetItemText(hListView, i, 0, buf, BUF_SIZE - 1);
		if (lstrcmpi(buf, title) == 0) {
			ListView_GetItemText(hListView, i, 1, buf, BUF_SIZE - 1);
			if (lstrcmpi(buf, class_name) == 0) {
				return TRUE;
			}
		}
	}

	// アイテムの追加
	lvi.mask = LVIF_TEXT;
	lvi.iItem = ListView_GetItemCount(hListView);
	lvi.iSubItem = 0;
	lvi.pszText = title;
	lvi.cchTextMax = BUF_SIZE - 1;
	lvi.iImage = 0;
	lvi.lParam = 0;
	i = ListView_InsertItem(hListView, &lvi);
	ListView_SetItemText(hListView, i, 1, class_name);
	return TRUE;
}

/*
 * listview_set_lparam - アイテムのLPARAMを設定
 */
BOOL listview_set_lparam(const HWND hListView, const int i, const LPARAM lParam)
{
	LV_ITEM lvi;

	ZeroMemory(&lvi, sizeof(LVITEM));
	lvi.mask = LVIF_PARAM;
	lvi.iItem = i;
	lvi.lParam = lParam;
	return ListView_SetItem(hListView, &lvi);
}

/*
 * listview_get_lparam - アイテムのLPARAMを取得
 */
LPARAM listview_get_lparam(const HWND hListView, const int i)
{
	LV_ITEM lvi;

	ZeroMemory(&lvi, sizeof(LVITEM));
	lvi.mask = LVIF_PARAM;
	lvi.iItem = i;
	ListView_GetItem(hListView, &lvi);
	return lvi.lParam;
}

/*
 * listview_move_item - リストビューのアイテムを移動
 */
void listview_move_item(const HWND hListView, int index, const int Move)
{
	HWND header_wnd;
	LV_ITEM lvi;
	TCHAR buf[100][BUF_SIZE];
	int column_cnt;
	int i = 0;
	LPARAM lp;

	// LPARAMの取得
	lp = listview_get_lparam(hListView, index);
	// ヘッダの取得
	if ((header_wnd = ListView_GetHeader(hListView)) == NULL) {
		header_wnd = GetWindow(hListView, GW_CHILD);
	}
	// テキストの取得
	column_cnt = Header_GetItemCount(header_wnd);
	for (i = 0; i < column_cnt; i++) {
		*(*(buf + i)) = TEXT('\0');
		ListView_GetItemText(hListView, index, i, *(buf + i), BUF_SIZE - 1);
	}
	// アイテムの削除
	ListView_DeleteItem(hListView, index);

	index += Move;

	// 移動先にアイテムを追加
	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.iItem = index;
	lvi.iSubItem = 0;
	lvi.pszText = *buf;
	lvi.cchTextMax = BUF_SIZE - 1;
	lvi.lParam = lp;
	i = ListView_InsertItem(hListView, &lvi);
	for (i = 1; i < column_cnt; i++) {
		ListView_SetItemText(hListView, index, i, *(buf + i));
	}
	ListView_SetItemState(hListView, index,
		LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	ListView_EnsureVisible(hListView, index, TRUE);
}

/*
 * listview_notify_proc - リストビューメッセージ
 */
LRESULT listview_notify_proc(const HWND hWnd, const LPARAM lParam, const HWND hListView)
{
	NMHDR *CForm = (NMHDR *)lParam;
	LV_KEYDOWN *LKey = (LV_KEYDOWN *)lParam;
	LV_DISPINFO *plv = (LV_DISPINFO *)lParam;

	if (CForm->hwndFrom != hListView) {
		return 0;
	}

	switch (plv->hdr.code) {
	case LVN_ITEMCHANGED:		// アイテムの選択状態の変更
	case NM_CUSTOMDRAW:
		return SendMessage(hWnd, WM_LV_EVENT, plv->hdr.code, lParam);
	}

	switch (CForm->code) {
	case NM_DBLCLK:				// ダブルクリック
		SendMessage(hWnd, WM_COMMAND, IDC_BUTTON_EDIT, 0);
		return 1;
	}

	switch (LKey->hdr.code) {
	case LVN_KEYDOWN:			// キーダウン
		if (LKey->wVKey == VK_DELETE) {
			SendMessage(hWnd, WM_COMMAND, IDC_BUTTON_DELETE, 0);
			return 1;
		}
	}
	return 0;
}

/*
 * compare_func - ソート用文字列比較
 */
static int CALLBACK compare_func(LPARAM lParam1, LPARAM lParam2, LPARAM colum)
{
	TCHAR buf1[BUF_SIZE];
	TCHAR buf2[BUF_SIZE];
	int order;
	int header;
	int ret;

	// ソート情報
	order = (colum < 0) ? 1 : 0;
	header = ABS(colum) - 1;

	*buf1 = *buf2 = TEXT('\0');
	ListView_GetItemText(sort_listview, lParam1, header, buf1, BUF_SIZE - 1);
	ListView_GetItemText(sort_listview, lParam2, header, buf2, BUF_SIZE - 1);

	ret = lstrcmpi(buf1, buf2);
	return (((ret < 0 && order == 1) || (ret > 0 && order == 0)) ? 1 : -1);
}

/*
 * listview_header_notify_proc - リストビューヘッダメッセージ
 */
LRESULT listview_header_notify_proc(const HWND hListView, const LPARAM lParam)
{
	HD_NOTIFY *hdn = (HD_NOTIFY *)lParam;
	static int colum = 1;
	int i;

	switch (hdn->hdr.code) {
	case HDN_ITEMCLICK:
		// ソートの設定
		sort_listview = hListView;
		colum = (ABS(colum) == (hdn->iItem + 1)) ? (colum * -1) : (hdn->iItem + 1);
		for (i = 0; i < ListView_GetItemCount(hListView); i++) {
			listview_set_lparam(hListView, i, i);
		}
		// ソート
		ListView_SortItems(hListView, compare_func, colum);
		break;
	}
	return FALSE;
}

/*
 * get_keyname - キー名を取得
 */
void get_keyname(const UINT modifiers, const UINT virtkey, TCHAR *ret)
{
	UINT scan_code;
	int ext_flag = 0;

	*ret = TEXT('\0');
	if (modifiers & MOD_CONTROL) {
		lstrcat(ret, TEXT("Ctrl + "));
	}
	if (modifiers & MOD_SHIFT) {
		lstrcat(ret, TEXT("Shift + "));
	}
	if (modifiers & MOD_ALT) {
		lstrcat(ret, TEXT("Alt + "));
	}
	if (modifiers & MOD_WIN) {
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
		virtkey == VK_NUMLOCK) ext_flag = 1 << 24;
	GetKeyNameText((scan_code << 16) | ext_flag, ret + lstrlen(ret), BUF_SIZE - lstrlen(ret) - 1);
}

/*
 * OptionNotifyProc - プロパティシートのイベントの通知
 */
LRESULT OptionNotifyProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PSHNOTIFY *pshn = (PSHNOTIFY FAR *)lParam;
	NMHDR *lpnmhdr = (NMHDR FAR *)&pshn->hdr;

	switch (lpnmhdr->code) {
	case PSN_APPLY:				// OK
		SendMessage(hDlg, WM_COMMAND, IDOK, 0);
		break;

	case PSN_QUERYCANCEL:		// キャンセル
		SendMessage(hDlg, WM_COMMAND, IDPCANCEL, 0);
		break;

	default:
		return PSNRET_NOERROR;
	}
	return PSNRET_NOERROR;
}

/*
 * ViewProperties - オプションの画面の表示
 */
static int show_option(const HWND hWnd, const TCHAR *cmd_line)
{
#define sizeof_PROPSHEETHEADER		40	// 古いコモンコントロール対策
#define PROP_CNT_OPTION				9
	PROPSHEETPAGE psp;
	PROPSHEETHEADER psh;
	HPROPSHEETPAGE hpsp[PROP_CNT_OPTION];

	psp.dwSize = sizeof(PROPSHEETPAGE);
	psp.dwFlags = PSP_DEFAULT;
	psp.hInstance = hInst;

	// 履歴
	psp.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG_HISTORY);
	psp.pfnDlgProc = set_histroy_proc;
	hpsp[0] = CreatePropertySheetPage(&psp);

	// メニュー
	psp.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG_MENU);
	psp.pfnDlgProc = set_menu_proc;
	hpsp[1] = CreatePropertySheetPage(&psp);

	// ビューア
	psp.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG_VIEWER);
	psp.pfnDlgProc = set_viewer_proc;
	hpsp[2] = CreatePropertySheetPage(&psp);

	// 動作
	psp.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG_ACTION);
	psp.pfnDlgProc = set_action_proc;
	hpsp[3] = CreatePropertySheetPage(&psp);

	// 形式
	psp.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG_FORMAT);
	psp.pfnDlgProc = set_format_proc;
	hpsp[4] = CreatePropertySheetPage(&psp);

	// フィルタ
	psp.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG_FILTER);
	psp.pfnDlgProc = set_filter_proc;
	hpsp[5] = CreatePropertySheetPage(&psp);

	// ウィンドウ
	psp.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG_WINDOW);
	psp.pfnDlgProc = set_window_proc;
	hpsp[6] = CreatePropertySheetPage(&psp);

	// キー設定
	psp.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG_SENDKEY);
	psp.pfnDlgProc = set_sendkey_proc;
	hpsp[7] = CreatePropertySheetPage(&psp);

	// ツール
	psp.pszTemplate = MAKEINTRESOURCE(IDD_DIALOG_TOOL);
	psp.pfnDlgProc = set_tool_proc;
	hpsp[8] = CreatePropertySheetPage(&psp);

	ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
	psh.dwSize = sizeof_PROPSHEETHEADER;
	psh.dwFlags = PSH_NOAPPLYNOW;
	psh.hInstance = hInst;
	psh.hwndParent = hWnd;
	psh.pszCaption = message_get_res(IDS_OPTION_TITLE);
	psh.nPages = sizeof(hpsp) / sizeof(HPROPSHEETPAGE);
	psh.phpage = hpsp;

	psh.nStartPage = 0;

	if (cmd_line != NULL && *cmd_line >= TEXT('0') && *cmd_line <= TEXT('8')) {
		psh.nStartPage = *cmd_line - TEXT('0');
		switch (psh.nStartPage) {
		case 4:		// Format
			lstrcpyn(cmd_format, cmd_line + 1, BUF_SIZE - 1);
			break;

		case 5:		// Filter
			lstrcpyn(cmd_filter, cmd_line + 1, BUF_SIZE - 1);
			break;
		}
	}

	prop_ret = 0;
	PropertySheet(&psh);
	return prop_ret;
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
		if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, work_path))) {
			lstrcat(work_path, TEXT("\\CLCL"));
			CreateDirectory(work_path, NULL);
		}
	}
}

/*
 * WinMain - メイン
 */
int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	HANDLE hMutex = NULL;
	HWND CLCLWnd;
	TCHAR *p;
	TCHAR err_str[BUF_SIZE];
	SECURITY_DESCRIPTOR sd;
	SECURITY_ATTRIBUTES sa;

	hInst = hInstance;

	// 2重起動チェック
	InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&sd, TRUE, 0, FALSE);	    
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = &sd;
	sa.bInheritHandle = TRUE; 
	hMutex = CreateMutex(&sa, FALSE, MUTEX);
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		if (hMutex != NULL) {
			CloseHandle(hMutex);
		}
		SetForegroundWindow(FindWindow(TEXT("#32770"), message_get_res(IDS_OPTION_TITLE)));
		return 0;
	}
	
	// メインウィンドウ検索
	if ((CLCLWnd = FindWindow(MAIN_WND_CLASS, MAIN_WINDOW_TITLE)) != NULL) {
		// バージョンチェック
		if (SendMessage(CLCLWnd, WM_GET_VERSION, 0, 0) != APP_VAR) {
			MessageBox(NULL, message_get_res(IDS_OPTION_START_ERROR), ERROR_TITLE, MB_ICONERROR);
			if (hMutex != NULL) {
				CloseHandle(hMutex);
			}
			return 0;
		}
		// 設定保存要求
		SendMessage(CLCLWnd, WM_OPTION_SAVE, 0, 0);
		// ホットキー解除要求
		SendMessage(CLCLWnd, WM_UNREGIST_HOTKEY, 0, 0);
	}

	// DPIの初期化
	InitDpi();

	// 設定取得
	get_work_path(hInstance);
	if (ini_get_option(err_str) == FALSE) {
		MessageBox(NULL, err_str, ERROR_TITLE, MB_ICONERROR);
		if (hMutex != NULL) {
			CloseHandle(hMutex);
		}
		return 0;
	}

	// CommonControlの初期化
	InitCommonControls();
	// キー選択の初期化
	init_selectkey(hInstance);
#ifdef OP_XP_STYLE
	// XPスタイルの初期化
	init_themes();
#endif	// OP_XP_STYLE

	p = lpCmdLine;
	// オプション画面を表示
	if (show_option(NULL, p) == 1) {
		if ((CLCLWnd = FindWindow(MAIN_WND_CLASS, MAIN_WINDOW_TITLE)) != NULL) {
			// 設定保存要求
			SendMessage(CLCLWnd, WM_OPTION_SAVE, 0, 0);
		}
		// 設定の保存
		ini_put_option();

		if (CLCLWnd != NULL) {
			// 設定読み込み要求
			SendMessage(CLCLWnd, WM_OPTION_LOAD, 0, 0);
		}
	} else {
		if ((CLCLWnd = FindWindow(MAIN_WND_CLASS, MAIN_WINDOW_TITLE)) != NULL) {
			// ホットキー設定要求
			SendMessage(CLCLWnd, WM_REGIST_HOTKEY, 0, 0);
		}
	}
	ini_free();
#ifdef OP_XP_STYLE
	themes_free();
#endif	// OP_XP_STYLE

	if (hMutex != NULL) {
		CloseHandle(hMutex);
	}
#ifdef _DEBUG
	mem_debug();
#endif	// _DEBUG
	return 0;
}
/* End of source */
