/*
 * CLCLSet
 *
 * SetTool.c
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

#include "..\General.h"
#include "..\Memory.h"
#include "..\String.h"
#include "..\Ini.h"
#include "..\Tool.h"
#include "..\Message.h"
#include "..\dpi.h"

#include "CLCLSet.h"
#include "SelectKey.h"

#include "resource.h"

/* Define */

/* Global Variables */
extern HINSTANCE hInst;
extern int prop_ret;
extern TCHAR work_path[];

// オプション
extern OPTION_INFO option;

/* Local Function Prototypes */
static BOOL dll_to_list(const HWND hDlg, const TCHAR *lib_path, int *old);
static BOOL CALLBACK select_tools_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK select_tool_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static void set_call_type(const HWND hDlg, const int call_type);
static BOOL CALLBACK set_tool_item_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static BOOL show_property(const HWND hDlg, TOOL_INFO *ti);
static void listview_set_text(const HWND hListView, const int i);
static void listview_set_tool(const HWND hListView, TOOL_INFO *ti, const BOOL copy);
static TOOL_INFO *listview_get_tool(const HWND hListView, int *cnt);
static void listview_free_tool(const HWND hListView);

/*
 * dll_to_list - DLL内の関数をリストビューに表示
 */
static BOOL dll_to_list(const HWND hDlg, const TCHAR *lib_path, int *old)
{
	HANDLE lib;
	FARPROC func_get_tool_info;
	OLD_GET_FUNC old_func_get_tool_info;
	LV_COLUMN lvc;
	LV_ITEM lvi;
	TOOL_GET_INFO tgi;
	TCHAR err_str[BUF_SIZE];
	long param;
	int call_type;
	int i, j;

	// リストビューの設定
	i = 0;
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = Scale(130);
	lvc.pszText = message_get_res(IDS_TOOL_LIST_TITLE);
	lvc.iSubItem = i++;
	ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_HEADER), lvc.iSubItem, &lvc);

	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = Scale(80);
	lvc.pszText = message_get_res(IDS_TOOL_LIST_FUNC);
	lvc.iSubItem = i++;
	ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_HEADER), lvc.iSubItem, &lvc);

	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = Scale(90);
	lvc.pszText = message_get_res(IDS_TOOL_LIST_CMD_LINE);
	lvc.iSubItem = i++;
	ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_HEADER), lvc.iSubItem, &lvc);

	// リストビューのスタイルの設定
	SetWindowLong(GetDlgItem(hDlg, IDC_LIST_HEADER), GWL_STYLE,
		GetWindowLong(GetDlgItem(hDlg, IDC_LIST_HEADER), GWL_STYLE) | LVS_SHOWSELALWAYS);
	SendDlgItemMessage(hDlg, IDC_LIST_HEADER, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
		LVS_EX_ONECLICKACTIVATE | LVS_EX_FULLROWSELECT |
		SendDlgItemMessage(hDlg, IDC_LIST_HEADER, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0));

	// DLLロード
	if ((lib = LoadLibrary(lib_path)) == NULL) {
		message_get_error(GetLastError(), err_str);
		if (*err_str != TEXT('\0')) {
			MessageBox(hDlg, err_str, lib_path, MB_ICONERROR);
		}
		return FALSE;
	}
	// 関数アドレス取得
	if ((func_get_tool_info = GetProcAddress(lib, "get_tool_info_w")) != NULL) {
		*old = 0;
	} else if ((func_get_tool_info = GetProcAddress(lib, "get_tool_info")) != NULL) {
		*old = 2;
	} else {
		if ((old_func_get_tool_info = (OLD_GET_FUNC)GetProcAddress(lib, "GetToolInfo")) == NULL) {
			message_get_error(GetLastError(), err_str);
			if (*err_str != TEXT('\0')) {
				MessageBox(hDlg, err_str, lib_path, MB_ICONERROR);
			}
			FreeLibrary(lib);
			return FALSE;
		}
		*old = 1;
	}

	// アイテムの追加
	i = 0;
	while (1) {
		ZeroMemory(&tgi, sizeof(TOOL_GET_INFO));
		tgi.struct_size = sizeof(TOOL_GET_INFO);
		if (*old == 0) {
			if (func_get_tool_info(hDlg, i++, &tgi) == FALSE) {
				break;
			}
			call_type = tgi.call_type;
		} else if (*old == 2) {
			TOOL_GET_INFO_A tgia;
			ZeroMemory(&tgia, sizeof(TOOL_GET_INFO_A));
			tgia.struct_size = sizeof(TOOL_GET_INFO_A);
			if (func_get_tool_info(hDlg, i++, &tgia) == FALSE) {
				break;
			}
			char_to_tchar(tgia.title, tgi.title, BUF_SIZE);
			char_to_tchar(tgia.func_name, tgi.func_name, BUF_SIZE);
			char_to_tchar(tgia.cmd_line, tgi.cmd_line, BUF_SIZE);
			call_type = tgia.call_type;
		} else {
			if ((call_type = old_func_get_tool_info(i++, tgi.title, tgi.func_name, &param)) <= -1) {
				break;
			}
		}
		ZeroMemory(&lvi, sizeof(LV_ITEM));
		lvi.mask = LVIF_TEXT | LVIF_PARAM;
		lvi.iItem = ListView_GetItemCount(GetDlgItem(hDlg, IDC_LIST_HEADER));
		lvi.iSubItem = 0;
		lvi.pszText = tgi.title;
		lvi.cchTextMax = BUF_SIZE - 1;
		lvi.lParam = call_type;
		j = ListView_InsertItem(GetDlgItem(hDlg, IDC_LIST_HEADER), &lvi);
		ListView_SetItemText(GetDlgItem(hDlg, IDC_LIST_HEADER), j, 1, tgi.func_name);
		ListView_SetItemText(GetDlgItem(hDlg, IDC_LIST_HEADER), j, 2, tgi.cmd_line);
	}
	FreeLibrary(lib);
	return TRUE;
}

/*
 * select_tools_proc - ツール選択ウィンドウプロシージャ
 */
static BOOL CALLBACK select_tools_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HWND pWnd;
	TOOL_INFO *ti;
	TCHAR buf[BUF_SIZE];
	static TCHAR lib_path[BUF_SIZE];
	int call_type;
	int i, j;
	static int old;

	switch (uMsg) {
	case WM_INITDIALOG:
		SetWindowText(hDlg, message_get_res(IDS_TOOL_SELECT_TITLE));
		SetWindowText(GetDlgItem(hDlg, IDC_STATIC_MSG), message_get_res(IDS_TOOL_SELECT_MSG));

		if (dll_to_list(hDlg, (TCHAR *)lParam, &old) == FALSE) {
			EndDialog(hDlg, FALSE);
			break;
		}
		// リストビューのスタイルの設定
		SetWindowLong(GetDlgItem(hDlg, IDC_LIST_HEADER), GWL_STYLE,
			GetWindowLong(GetDlgItem(hDlg, IDC_LIST_HEADER), GWL_STYLE) & ~LVS_SINGLESEL);
		ListView_SetItemState(GetDlgItem(hDlg, IDC_LIST_HEADER), -1, LVIS_SELECTED, LVIS_SELECTED);

		lstrcpy(lib_path, (TCHAR *)lParam);

		if (ListView_GetSelectedCount(GetDlgItem(hDlg, IDC_LIST_HEADER)) <= 0) {
			EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
		} else {
			EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
		}
		break;

	case WM_CLOSE:
		EndDialog(hDlg, FALSE);
		break;

	case WM_NOTIFY:
		listview_notify_proc(hDlg, lParam, GetDlgItem(hDlg, IDC_LIST_HEADER));
		break;

	case WM_LV_EVENT:
		switch (wParam) {
		case LVN_ITEMCHANGED:
			if (ListView_GetSelectedCount(GetDlgItem(hDlg, IDC_LIST_HEADER)) <= 0) {
				EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
			} else {
				EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
			}
			break;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			SendMessage(hDlg, WM_CLOSE, 0, 0);
			break;

		case IDC_BUTTON_EDIT:
		case IDOK:
			pWnd = PropSheet_GetCurrentPageHwnd(GetParent(hDlg));

			i = -1;
			while ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_HEADER), i, LVNI_SELECTED)) != -1) {
				if ((ti = mem_calloc(sizeof(TOOL_INFO))) != NULL) {
					// 設定取得
					ti->lib_file_path = alloc_copy(lib_path);

					ListView_GetItemText(GetDlgItem(hDlg, IDC_LIST_HEADER), i, 0, buf, BUF_SIZE - 1);
					ti->title = alloc_copy(buf);
					ListView_GetItemText(GetDlgItem(hDlg, IDC_LIST_HEADER), i, 1, buf, BUF_SIZE - 1);
					ti->func_name = alloc_copy(buf);
					ListView_GetItemText(GetDlgItem(hDlg, IDC_LIST_HEADER), i, 2, buf, BUF_SIZE - 1);
					ti->cmd_line = alloc_copy(buf);

					call_type = listview_get_lparam(GetDlgItem(hDlg, IDC_LIST_HEADER), i);
					if (old == 1) {
						j = CALLTYPE_VIEWER;
						if (!(call_type & OLD_CALLTYPE_MENU)) {
							j |= CALLTYPE_MENU;
							ti->copy_paste = 1;
						}
						if (call_type & OLD_CALLTYPE_ADD_HISTORY) {
							j |= CALLTYPE_ADD_HISTORY;
						}
						if (call_type & OLD_CALLTYPE_ITEM_TO_CLIPBOARD) {
							j |= CALLTYPE_ITEM_TO_CLIPBOARD;
						}
						if (call_type & OLD_CALLTYPE_START) {
							j |= CALLTYPE_START;
						}
						if (call_type & OLD_CALLTYPE_END) {
							j |= CALLTYPE_END;
						}
						ti->call_type = j;
					} else {
						ti->copy_paste = (call_type & CALLTYPE_MENU_COPY_PASTE) ? 1 : 0;
						ti->call_type = call_type & ~CALLTYPE_MENU_COPY_PASTE;
					}
					ti->old = old;

					// 新規追加
					listview_set_tool(GetDlgItem(pWnd, IDC_LIST_TOOL), ti, FALSE);
				}
			}
			EndDialog(hDlg, TRUE);
			break;
		}
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

	default:
		return FALSE;
	}
	return TRUE;
}

/*
 * select_tool_proc - ツール選択ウィンドウプロシージャ
 */
static BOOL CALLBACK select_tool_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	TCHAR buf[BUF_SIZE];
	int call_type;
	int i, j;
	static int old;

	switch (uMsg) {
	case WM_INITDIALOG:
		SetWindowText(hDlg, message_get_res(IDS_TOOL_SELECT_TITLE));
		SetWindowText(GetDlgItem(hDlg, IDC_STATIC_MSG), message_get_res(IDS_TOOL_SELECT_MSG));

		if (dll_to_list(hDlg, (TCHAR *)lParam, &old) == FALSE) {
			EndDialog(hDlg, FALSE);
			break;
		}

		EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
		break;

	case WM_CLOSE:
		EndDialog(hDlg, FALSE);
		break;

	case WM_NOTIFY:
		listview_notify_proc(hDlg, lParam, GetDlgItem(hDlg, IDC_LIST_HEADER));
		break;

	case WM_LV_EVENT:
		switch (wParam) {
		case LVN_ITEMCHANGED:
			if (ListView_GetSelectedCount(GetDlgItem(hDlg, IDC_LIST_HEADER)) <= 0) {
				EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
			} else {
				EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
			}
			break;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			SendMessage(hDlg, WM_CLOSE, 0, 0);
			break;

		case IDC_BUTTON_EDIT:
		case IDOK:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_HEADER), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			ListView_GetItemText(GetDlgItem(hDlg, IDC_LIST_HEADER), i, 0, buf, BUF_SIZE - 1);
			SendDlgItemMessage(GetParent(hDlg), IDC_EDIT_TITLE, WM_SETTEXT, 0, (LPARAM)buf);

			ListView_GetItemText(GetDlgItem(hDlg, IDC_LIST_HEADER), i, 1, buf, BUF_SIZE - 1);
			SendDlgItemMessage(GetParent(hDlg), IDC_EDIT_HEADER, WM_SETTEXT, 0, (LPARAM)buf);

			ListView_GetItemText(GetDlgItem(hDlg, IDC_LIST_HEADER), i, 2, buf, BUF_SIZE - 1);
			SendDlgItemMessage(GetParent(hDlg), IDC_EDIT_CMD_LINE, WM_SETTEXT, 0, (LPARAM)buf);

			call_type = listview_get_lparam(GetDlgItem(hDlg, IDC_LIST_HEADER), i);
			if (old == 1) {
				j = CALLTYPE_VIEWER;
				if (!(call_type & OLD_CALLTYPE_MENU)) {
					j |= (CALLTYPE_MENU | CALLTYPE_MENU_COPY_PASTE);
				}
				if (call_type & OLD_CALLTYPE_ADD_HISTORY) {
					j |= CALLTYPE_ADD_HISTORY;
				}
				if (call_type & OLD_CALLTYPE_ITEM_TO_CLIPBOARD) {
					j |= CALLTYPE_ITEM_TO_CLIPBOARD;
				}
				if (call_type & OLD_CALLTYPE_START) {
					j |= CALLTYPE_START;
				}
				if (call_type & OLD_CALLTYPE_END) {
					j |= CALLTYPE_END;
				}
				call_type = j;
			}
			set_call_type(GetParent(hDlg), call_type);

			CheckDlgButton(GetParent(hDlg), IDC_CHECK_OLD, old);
			EndDialog(hDlg, TRUE);
			break;
		}
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

	default:
		return FALSE;
	}
	return TRUE;
}

/*
 * set_call_type - 呼び出し方法の設定
 */
static void set_call_type(const HWND hDlg, const int call_type)
{
	CheckDlgButton(hDlg, IDC_CHECK_MENU, (call_type & CALLTYPE_MENU) ? 1 : 0);
	CheckDlgButton(hDlg, IDC_CHECK_VIEWER, (call_type & CALLTYPE_VIEWER) ? 1 : 0);
	CheckDlgButton(hDlg, IDC_CHECK_VIEWER_OPEN, (call_type & CALLTYPE_VIEWER_OPEN) ? 1 : 0);
	CheckDlgButton(hDlg, IDC_CHECK_VIEWER_CLOSE, (call_type & CALLTYPE_VIEWER_CLOSE) ? 1 : 0);
	CheckDlgButton(hDlg, IDC_CHECK_HISTORY, (call_type & CALLTYPE_ADD_HISTORY) ? 1 : 0);
	CheckDlgButton(hDlg, IDC_CHECK_CLIPBOARD, (call_type & CALLTYPE_ITEM_TO_CLIPBOARD) ? 1 : 0);
	CheckDlgButton(hDlg, IDC_CHECK_START, (call_type & CALLTYPE_START) ? 1 : 0);
	CheckDlgButton(hDlg, IDC_CHECK_END, (call_type & CALLTYPE_END) ? 1 : 0);
	CheckDlgButton(hDlg, IDC_CHECK_COPY_PATE, (call_type & CALLTYPE_MENU_COPY_PASTE) ? 1 : 0);
}

/*
 * set_tool_item_proc - ツールの項目を設定
 */
static BOOL CALLBACK set_tool_item_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	TOOL_INFO *ti;
	TCHAR buf[BUF_SIZE];
	int i;

	switch (uMsg) {
	case WM_INITDIALOG:
		if (lParam == 0) {
			// 新規追加
			SendMessage(hDlg, WM_COMMAND, IDC_CHECK_MENU, 0);
			SetWindowLong(hDlg, GWL_USERDATA, 0);
			break;
		}
		ti = (TOOL_INFO *)lParam;

		SendDlgItemMessage(hDlg, IDC_EDIT_TITLE, WM_SETTEXT, 0, (LPARAM)ti->title);
		SendDlgItemMessage(hDlg, IDC_EDIT_LIB_PATH, WM_SETTEXT, 0, (LPARAM)ti->lib_file_path);
		SendDlgItemMessage(hDlg, IDC_EDIT_HEADER, WM_SETTEXT, 0, (LPARAM)ti->func_name);
		SendDlgItemMessage(hDlg, IDC_EDIT_CMD_LINE, WM_SETTEXT, 0, (LPARAM)ti->cmd_line);

		set_call_type(hDlg, ti->call_type);
		CheckDlgButton(hDlg, IDC_CHECK_COPY_PATE, ti->copy_paste);
		CheckDlgButton(hDlg, IDC_CHECK_OLD, ti->old);

		i = 0;
		if (ti->modifiers & MOD_SHIFT) {
			i |= HOTKEYF_SHIFT;
		}
		if (ti->modifiers & MOD_CONTROL) {
			i |= HOTKEYF_CONTROL;
		}
		if (ti->modifiers & MOD_ALT) {
			i |= HOTKEYF_ALT;
		}
		if (ti->modifiers & MOD_WIN) {
			i |= HOTKEYF_WIN;
		}
		if (ti->virtkey != 0) {
			SendDlgItemMessage(hDlg, IDC_HOTKEY_TOOL, HKM_SETHOTKEY,
				(WPARAM)MAKEWORD(ti->virtkey, i), 0);
		}
		SendMessage(hDlg, WM_COMMAND, IDC_CHECK_MENU, 0);

		SetWindowLong(hDlg, GWL_USERDATA, lParam);
		break;

	case WM_CLOSE:
		EndDialog(hDlg, FALSE);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CHECK_MENU:
			EnableWindow(GetDlgItem(hDlg, IDC_CHECK_COPY_PATE), IsDlgButtonChecked(hDlg, IDC_CHECK_MENU));
			EnableWindow(GetDlgItem(hDlg, IDC_HOTKEY_TOOL), IsDlgButtonChecked(hDlg, IDC_CHECK_MENU));
			break;

		case IDC_BUTTON_FILE_SELECT:
			// ファイル選択
			SetFocus(GetDlgItem(hDlg, IDC_EDIT_LIB_PATH));
			if (file_select(hDlg, TEXT("*.dll\0*.dll\0*.*\0*.*\0\0"), 1, buf) == -1) {
				break;
			}
			SendDlgItemMessage(hDlg, IDC_EDIT_LIB_PATH, WM_SETTEXT, 0, (LPARAM)buf);
			SendDlgItemMessage(hDlg, IDC_EDIT_HEADER, WM_SETTEXT, 0, (LPARAM)TEXT(""));

		case IDC_BUTTON_FUNC_SELECT:
			// ツール選択
			*buf = TEXT('\0');
			SendDlgItemMessage(hDlg, IDC_EDIT_LIB_PATH, WM_GETTEXT, BUF_SIZE - 1, (LPARAM)buf);
			if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_SELECT_FUNC), hDlg, select_tool_proc, (LPARAM)buf) == TRUE) {
				SendMessage(hDlg, WM_COMMAND, IDC_CHECK_MENU, 0);
			}
			break;

		case IDOK:
			*buf = TEXT('\0');
			SendDlgItemMessage(hDlg, IDC_EDIT_TITLE, WM_GETTEXT, BUF_SIZE - 1, (LPARAM)buf);
			if (*buf == TEXT('\0')) {
				MessageBox(hDlg, message_get_res(IDS_TOOL_ERR_TITLE), WINDOW_TITLE, MB_ICONEXCLAMATION);
				SetFocus(GetDlgItem(hDlg, IDC_EDIT_TITLE));
				break;
			}

			if ((ti = (TOOL_INFO *)GetWindowLong(hDlg, GWL_USERDATA)) == NULL) {
				ti = mem_calloc(sizeof(TOOL_INFO));
			}
			if (ti != NULL) {
				// 設定取得
				alloc_get_text(GetDlgItem(hDlg, IDC_EDIT_TITLE), &ti->title);
				alloc_get_text(GetDlgItem(hDlg, IDC_EDIT_LIB_PATH), &ti->lib_file_path);
				alloc_get_text(GetDlgItem(hDlg, IDC_EDIT_HEADER), &ti->func_name);
				alloc_get_text(GetDlgItem(hDlg, IDC_EDIT_CMD_LINE), &ti->cmd_line);
				
				ti->call_type = 0;
				ti->call_type |= (IsDlgButtonChecked(hDlg, IDC_CHECK_MENU) * CALLTYPE_MENU);
				ti->call_type |= (IsDlgButtonChecked(hDlg, IDC_CHECK_VIEWER) * CALLTYPE_VIEWER);
				ti->call_type |= (IsDlgButtonChecked(hDlg, IDC_CHECK_VIEWER_OPEN) * CALLTYPE_VIEWER_OPEN);
				ti->call_type |= (IsDlgButtonChecked(hDlg, IDC_CHECK_VIEWER_CLOSE) * CALLTYPE_VIEWER_CLOSE);
				ti->call_type |= (IsDlgButtonChecked(hDlg, IDC_CHECK_HISTORY) * CALLTYPE_ADD_HISTORY);
				ti->call_type |= (IsDlgButtonChecked(hDlg, IDC_CHECK_CLIPBOARD) * CALLTYPE_ITEM_TO_CLIPBOARD);
				ti->call_type |= (IsDlgButtonChecked(hDlg, IDC_CHECK_START) * CALLTYPE_START);
				ti->call_type |= (IsDlgButtonChecked(hDlg, IDC_CHECK_END) * CALLTYPE_END);

				ti->copy_paste = IsDlgButtonChecked(hDlg, IDC_CHECK_COPY_PATE);
				ti->old = IsDlgButtonChecked(hDlg, IDC_CHECK_OLD);

				i = SendDlgItemMessage(hDlg, IDC_HOTKEY_TOOL, HKM_GETHOTKEY, 0, 0);
				ti->virtkey = LOBYTE(i);
				i = HIBYTE(i);
				ti->modifiers = ((i & HOTKEYF_SHIFT) ? MOD_SHIFT : 0) |
					((i & HOTKEYF_CONTROL) ? MOD_CONTROL : 0) |
					((i & HOTKEYF_ALT) ? MOD_ALT : 0) |
					((i & HOTKEYF_WIN) ? MOD_WIN : 0);
			}

			if (GetWindowLong(hDlg, GWL_USERDATA) == 0) {
				// 新規
				HWND pWnd = PropSheet_GetCurrentPageHwnd(GetParent(hDlg));
				listview_set_tool(GetDlgItem(pWnd, IDC_LIST_TOOL), ti, FALSE);
			}
			EndDialog(hDlg, TRUE);
			break;

		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			break;
		}
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

/*
 * show_property - 形式毎のプロパティ表示
 */
static BOOL show_property(const HWND hDlg, TOOL_INFO *ti)
{
	HANDLE lib;
	FARPROC func_property;
	TOOL_EXEC_INFO *tei;
	TCHAR err_str[BUF_SIZE];
	TCHAR buf[BUF_SIZE];
	char cbuf[BUF_SIZE];
	BOOL ret = FALSE;

	// DLLロード
	if ((lib = LoadLibrary(ti->lib_file_path)) == NULL) {
		message_get_error(GetLastError(), err_str);
		if (*err_str != TEXT('\0')) {
			MessageBox(hDlg, err_str, ti->lib_file_path, MB_ICONERROR);
		}
		return FALSE;
	}
	// 関数アドレス取得
	wsprintf(buf, TEXT("%s_property"), ti->func_name);
	tchar_to_char(buf, cbuf, BUF_SIZE - 1);
	func_property = GetProcAddress(lib, cbuf);
	if (func_property != NULL) {
		// 実行情報の設定
		if ((tei = mem_calloc(sizeof(TOOL_EXEC_INFO))) != NULL) {
			tei->struct_size = sizeof(TOOL_EXEC_INFO);
			tei->call_type = 0;
			if (ti->old == 2) {
				tei->cmd_line = (TCHAR *)alloc_tchar_to_char(tei->cmd_line);
			} else {
				tei->cmd_line = alloc_copy(ti->cmd_line);
			}
			mem_free(&ti->cmd_line);
			// プロパティ表示
			ret = func_property(hDlg, tei);
			if (ti->old == 2) {
				ti->cmd_line = alloc_char_to_tchar((char *)tei->cmd_line);
			} else {
				ti->cmd_line = alloc_copy(tei->cmd_line);
			}
			mem_free(&tei->cmd_line);
			mem_free(&tei);
		}
	}
	FreeLibrary(lib);
	if (ret == FALSE) {
		MessageBox(hDlg, message_get_res(IDS_TOOL_NON_PROP), WINDOW_TITLE, MB_OK | MB_ICONEXCLAMATION);
	}
	return ret;
}

/*
 * listview_set_text - ListViewのテキストを設定
 */
static void listview_set_text(const HWND hListView, const int i)
{
	TOOL_INFO *ti;
	TCHAR buf[BUF_SIZE];

	if ((ti = (TOOL_INFO *)listview_get_lparam(hListView, i)) == NULL) {
		return;
	}
	// 形式名
	ListView_SetItemText(hListView, i, 0, ti->title);
	// DLL
	ListView_SetItemText(hListView, i, 1, ti->lib_file_path);
	// 関数名
	ListView_SetItemText(hListView, i, 2, ti->func_name);
	// ホットキー
	if (ti->call_type & CALLTYPE_MENU) {
		get_keyname(ti->modifiers, ti->virtkey, buf);
	} else {
		get_keyname(0, 0, buf);
	}
	ListView_SetItemText(hListView, i, 3, buf);
}

/*
 * listview_set_tool - ListViewにツール情報を追加する
 */
static void listview_set_tool(const HWND hListView, TOOL_INFO *ti, const BOOL copy)
{
	LV_ITEM lvi;
	TOOL_INFO *new_ti;
	int i;

	if (copy == TRUE) {
		if ((new_ti = mem_calloc(sizeof(TOOL_INFO))) == NULL) {
			return;
		}
		new_ti->title = alloc_copy(ti->title);
		new_ti->lib_file_path = alloc_copy(ti->lib_file_path);
		new_ti->func_name = alloc_copy(ti->func_name);
		new_ti->cmd_line = alloc_copy(ti->cmd_line);
		new_ti->call_type = ti->call_type;
		new_ti->copy_paste = ti->copy_paste;
		new_ti->old = ti->old;
		new_ti->modifiers = ti->modifiers;
		new_ti->virtkey = ti->virtkey;
	} else {
		new_ti = ti;
	}

	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.iItem = ListView_GetItemCount(hListView);
	lvi.iSubItem = 0;
	lvi.pszText = TEXT("");
	lvi.cchTextMax = BUF_SIZE - 1;
	lvi.iImage = 0;
	lvi.lParam = (LPARAM)new_ti;
	i = ListView_InsertItem(hListView, &lvi);

	listview_set_text(hListView, i);
}

/*
 * listview_get_tool - ツール情報の取得
 */
static TOOL_INFO *listview_get_tool(const HWND hListView, int *cnt)
{
	TOOL_INFO *ti;
	TOOL_INFO *new_ti;
	int i;

	if ((*cnt = ListView_GetItemCount(hListView)) == 0) {
		return NULL;
	}

	// 項目の作成
	if ((new_ti = mem_calloc(sizeof(TOOL_INFO) * *cnt)) == NULL) {
		*cnt = 0;
		return NULL;
	}

	for (i = 0; i < *cnt; i++) {
		if ((ti = (TOOL_INFO *)listview_get_lparam(hListView, i)) == NULL) {
			continue;
		}
		(new_ti + i)->title = alloc_copy(ti->title);
		(new_ti + i)->lib_file_path = alloc_copy(ti->lib_file_path);
		(new_ti + i)->func_name = alloc_copy(ti->func_name);
		(new_ti + i)->cmd_line = alloc_copy(ti->cmd_line);
		(new_ti + i)->call_type = ti->call_type;
		(new_ti + i)->copy_paste = ti->copy_paste;
		(new_ti + i)->old = ti->old;
		(new_ti + i)->modifiers = ti->modifiers;
		(new_ti + i)->virtkey = ti->virtkey;
	}
	return new_ti;
}

/*
 * listview_free_tool - ツール情報の解放
 */
static void listview_free_tool(const HWND hListView)
{
	TOOL_INFO *ti;
	int i;

	for (i = 0; i < ListView_GetItemCount(hListView); i++) {
		if ((ti = (TOOL_INFO *)listview_get_lparam(hListView, i)) != NULL) {
			mem_free(&ti->title);
			mem_free(&ti->lib_file_path);
			mem_free(&ti->func_name);
			mem_free(&ti->cmd_line);
			mem_free(&ti);
		}
		listview_set_lparam(hListView, i, 0);
	}
}

/*
 * set_tool_proc - ツール設定のプロシージャ
 */
BOOL CALLBACK set_tool_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LV_COLUMN lvc;
	TOOL_INFO *ti;
	TCHAR buf[BUF_SIZE];
	int cnt;
	int i;
	BOOL enable;
#ifdef OP_XP_STYLE
	static long hThemeUp, hThemeDown;
#endif	// OP_XP_STYLE

	switch (uMsg) {
	case WM_INITDIALOG:
		// D&Dを受け付ける
		SetWindowLong(hDlg, GWL_EXSTYLE, GetWindowLong(hDlg, GWL_EXSTYLE) | WS_EX_ACCEPTFILES);
#ifdef OP_XP_STYLE
		// XP
		hThemeUp = open_theme(GetDlgItem(hDlg, IDC_BUTTON_UP), L"SCROLLBAR");
		hThemeDown = open_theme(GetDlgItem(hDlg, IDC_BUTTON_DOWN), L"SCROLLBAR");
#endif	// OP_XP_STYLE
		// リストビューのカラムの設定
		i = 0;
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = Scale(110);
		lvc.pszText = message_get_res(IDS_TOOL_LIST_TITLE);
		lvc.iSubItem = i++;
		ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_TOOL), lvc.iSubItem, &lvc);

		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = Scale(70);
		lvc.pszText = message_get_res(IDS_TOOL_LIST_DLL);
		lvc.iSubItem = i++;
		ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_TOOL), lvc.iSubItem, &lvc);

		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = Scale(70);
		lvc.pszText = message_get_res(IDS_TOOL_LIST_FUNC);
		lvc.iSubItem = i++;
		ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_TOOL), lvc.iSubItem, &lvc);

		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = Scale(80);
		lvc.pszText = message_get_res(IDS_TOOL_LIST_HOTKEY);
		lvc.iSubItem = i++;
		ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_TOOL), lvc.iSubItem, &lvc);

		// リストビューのスタイルの設定
		SetWindowLong(GetDlgItem(hDlg, IDC_LIST_TOOL), GWL_STYLE,
			GetWindowLong(GetDlgItem(hDlg, IDC_LIST_TOOL), GWL_STYLE) | LVS_SHOWSELALWAYS);
		SendDlgItemMessage(hDlg, IDC_LIST_TOOL, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
			LVS_EX_ONECLICKACTIVATE | LVS_EX_FULLROWSELECT |
			SendDlgItemMessage(hDlg, IDC_LIST_TOOL, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0));

		for (i = 0; i < option.tool_cnt; i++) {
			listview_set_tool(GetDlgItem(hDlg, IDC_LIST_TOOL), option.tool_info + i, TRUE);
		}
		SendMessage(hDlg, WM_LV_EVENT, LVN_ITEMCHANGED, 0);
		break;

	case WM_DESTROY:
		listview_free_tool(GetDlgItem(hDlg, IDC_LIST_TOOL));
#ifdef OP_XP_STYLE
		// XP
		if (hThemeUp != 0 && hThemeDown != 0) {
			close_theme(hThemeUp);
			close_theme(hThemeDown);
		}
#endif	// OP_XP_STYLE
		break;

	case WM_DRAWITEM:
		switch ((UINT)wParam) {
		case IDC_BUTTON_UP:
			i = DFCS_SCROLLUP;
			break;

		case IDC_BUTTON_DOWN:
			i = DFCS_SCROLLDOWN;
			break;

		default:
			return FALSE;
		}
		// ボタンの描画
#ifdef OP_XP_STYLE
		if (hThemeUp != 0 && hThemeDown != 0) {
			draw_theme_scroll((LPDRAWITEMSTRUCT)lParam, i, (i == DFCS_SCROLLUP) ? hThemeUp : hThemeDown);
		} else {
			draw_scroll_sontrol((LPDRAWITEMSTRUCT)lParam, i);
		}
#else	// OP_XP_STYLE
		draw_scroll_sontrol((LPDRAWITEMSTRUCT)lParam, i);
#endif	// OP_XP_STYLE
		break;

#ifdef OP_XP_STYLE
	case WM_THEMECHANGED:
		// テーマの変更
		if (hThemeUp != 0 && hThemeDown != 0) {
			close_theme(hThemeUp);
			close_theme(hThemeDown);
		}
		hThemeUp = open_theme(GetDlgItem(hDlg, IDC_BUTTON_UP), L"SCROLLBAR");
		hThemeDown = open_theme(GetDlgItem(hDlg, IDC_BUTTON_DOWN), L"SCROLLBAR");
		break;
#endif	// OP_XP_STYLE

	case WM_DROPFILES:
		cnt = DragQueryFile((HANDLE)wParam, 0xFFFFFFFF, NULL, 0);
		for (i = 0; i < cnt; i++) {
			DragQueryFile((HANDLE)wParam, i, buf, BUF_SIZE - 1);
			if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_SELECT_FUNC), hDlg, select_tools_proc, (LPARAM)buf) == FALSE) {
				break;
			}
		}
		DragFinish((HANDLE)wParam);
		break;

	case WM_SHOWWINDOW:
		if (wParam == FALSE) {
			for (i = 0; i < option.tool_cnt; i++) {
				mem_free(&((option.tool_info + i)->title));
				mem_free(&((option.tool_info + i)->lib_file_path));
				mem_free(&((option.tool_info + i)->func_name));
				mem_free(&((option.tool_info + i)->cmd_line));
			}
			mem_free(&option.tool_info);

			option.tool_info = listview_get_tool(GetDlgItem(hDlg, IDC_LIST_TOOL), &option.tool_cnt);
		}
		break;

	case WM_NOTIFY:
		if (listview_notify_proc(hDlg, lParam, GetDlgItem(hDlg, IDC_LIST_TOOL)) == 0) {
			return OptionNotifyProc(hDlg, uMsg, wParam, lParam);
		}
		break;

	case WM_LV_EVENT:
		switch (wParam) {
		case LVN_ITEMCHANGED:
			enable = (ListView_GetSelectedCount(GetDlgItem(hDlg, IDC_LIST_TOOL)) <= 0) ? FALSE : TRUE;
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_UP), enable);
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_DOWN), enable);
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_EDIT), enable);
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_DELETE), enable);
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_PROP), enable);
			break;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_BUTTON_UP:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_TOOL), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			if (i == 0) {
				break;
			}
			listview_move_item(GetDlgItem(hDlg, IDC_LIST_TOOL), i, -1);
			SetFocus(GetDlgItem(hDlg, LOWORD(wParam)));
			break;

		case IDC_BUTTON_DOWN:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_TOOL), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			if (i == ListView_GetItemCount(GetDlgItem(hDlg, IDC_LIST_TOOL)) - 1) {
				break;
			}
			listview_move_item(GetDlgItem(hDlg, IDC_LIST_TOOL), i, 1);
			SetFocus(GetDlgItem(hDlg, LOWORD(wParam)));
			break;

		case IDC_BUTTON_ADD:
			DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_TOOL_SET), hDlg, set_tool_item_proc, 0);
			break;

		case IDC_BUTTON_EDIT:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_TOOL), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_TOOL_SET), hDlg, set_tool_item_proc,
				listview_get_lparam(GetDlgItem(hDlg, IDC_LIST_TOOL), i));
			listview_set_text(GetDlgItem(hDlg, IDC_LIST_TOOL), i);
			break;

		case IDC_BUTTON_DELETE:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_TOOL), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			if (MessageBox(hDlg, message_get_res(IDS_OPTION_DELETE), WINDOW_TITLE, MB_ICONQUESTION | MB_YESNO) == IDNO) {
				break;
			}
			if ((ti = (TOOL_INFO *)listview_get_lparam(GetDlgItem(hDlg, IDC_LIST_TOOL), i)) != NULL) {
				mem_free(&ti->title);
				mem_free(&ti->lib_file_path);
				mem_free(&ti->func_name);
				mem_free(&ti->cmd_line);
				mem_free(&ti);
			}
			ListView_DeleteItem(GetDlgItem(hDlg, IDC_LIST_TOOL), i);
			ListView_SetItemState(GetDlgItem(hDlg, IDC_LIST_TOOL), i,
				LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
			break;

		case IDC_BUTTON_PROP:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_TOOL), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			show_property(hDlg, (TOOL_INFO *)listview_get_lparam(GetDlgItem(hDlg, IDC_LIST_TOOL), i));
			break;

		case IDOK:
			for (i = 0; i < option.tool_cnt; i++) {
				mem_free(&((option.tool_info + i)->title));
				mem_free(&((option.tool_info + i)->lib_file_path));
				mem_free(&((option.tool_info + i)->func_name));
				mem_free(&((option.tool_info + i)->cmd_line));
			}
			mem_free(&option.tool_info);

			option.tool_info = listview_get_tool(GetDlgItem(hDlg, IDC_LIST_TOOL), &option.tool_cnt);
			listview_free_tool(GetDlgItem(hDlg, IDC_LIST_TOOL));
			prop_ret = 1;
			break;

		case IDPCANCEL:
			break;
		}
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

	default:
		return FALSE;
	}
	return TRUE;
}
/* End of source */
