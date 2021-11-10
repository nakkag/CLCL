/*
 * CLCLSet
 *
 * SetSendkey.c
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
#include "..\Ini.h"
#include "..\SendKey.h"
#include "..\Message.h"
#include "..\dpi.h"

#include "CLCLSet.h"
#include "SelectKey.h"

#include "resource.h"

/* Define */

/* Global Variables */
extern HINSTANCE hInst;
extern int prop_ret;

// オプション
extern OPTION_INFO option;

/* Local Function Prototypes */
static BOOL CALLBACK set_sendkey_item_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static void listview_set_text(const HWND hListView, const int i);
static void listview_set_sendkey(const HWND hListView, SENDKEY_INFO *si, const BOOL copy);
static SENDKEY_INFO *listview_get_sendkey(const HWND hListView, int *cnt);
static void listview_free_sendkey(const HWND hListView);

/*
 * set_sendkey_item_proc - キー設定の項目を設定
 */
static BOOL CALLBACK set_sendkey_item_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LV_COLUMN lvc;
	SENDKEY_INFO *si;
	TCHAR buf[BUF_SIZE];
	TCHAR tmp[BUF_SIZE];
	int i;
#ifdef OP_XP_STYLE
	static long hTheme;
#endif	// OP_XP_STYLE

	switch (uMsg) {
	case WM_INITDIALOG:
#ifdef OP_XP_STYLE
		// XP
		hTheme = open_theme(GetDlgItem(hDlg, IDC_BUTTON_SELECT), L"SCROLLBAR");
#endif	// OP_XP_STYLE
		// スピンコントロールの設定
		SendDlgItemMessage(hDlg, IDC_SPIN_COPY_WAIT, UDM_SETRANGE, 0, (LPARAM)MAKELONG(UD_MAXVAL, 0));
		SendDlgItemMessage(hDlg, IDC_SPIN_PASTE_WAIT, UDM_SETRANGE, 0, (LPARAM)MAKELONG(UD_MAXVAL, 0));

		i = 0;
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = Scale(170);
		lvc.pszText = message_get_res(IDS_SENDKEY_LIST_TITLE);
		lvc.iSubItem = i++;
		ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_WINDOW), lvc.iSubItem, &lvc);

		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = Scale(160);
		lvc.pszText = message_get_res(IDS_SENDKEY_LIST_CLASSNAME);
		lvc.iSubItem = i++;
		ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_WINDOW), lvc.iSubItem, &lvc);

		// リストビューのスタイルの設定
		SetWindowLong(GetDlgItem(hDlg, IDC_LIST_WINDOW), GWL_STYLE,
			GetWindowLong(GetDlgItem(hDlg, IDC_LIST_WINDOW), GWL_STYLE) | LVS_SHOWSELALWAYS);
		SendDlgItemMessage(hDlg, IDC_LIST_WINDOW, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
			LVS_EX_ONECLICKACTIVATE | LVS_EX_FULLROWSELECT |
			SendDlgItemMessage(hDlg, IDC_LIST_WINDOW, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0));

		EnumWindows((WNDENUMPROC)enum_windows_proc, (LPARAM)GetDlgItem(hDlg, IDC_LIST_WINDOW));

		if (lParam == 0) {
			// 新規追加
			SetDlgItemInt(hDlg, IDC_EDIT_COPY_WAIT, DEFAULT_COPY_WAIT, FALSE);
			SetDlgItemInt(hDlg, IDC_EDIT_PASTE_WAIT, DEFAULT_PASTE_WAIT, FALSE);
			SetWindowLong(hDlg, GWL_USERDATA, 0);
			break;
		}
		si = (SENDKEY_INFO *)lParam;

		SendDlgItemMessage(hDlg, IDC_EDIT_TITLE, WM_SETTEXT, 0, (LPARAM)si->title);
		SendDlgItemMessage(hDlg, IDC_EDIT_CLASSNAME, WM_SETTEXT, 0, (LPARAM)si->class_name);

		// コピー
		i = 0;
		if (si->copy_modifiers & MOD_SHIFT) {
			i |= HOTKEYF_SHIFT;
		}
		if (si->copy_modifiers & MOD_CONTROL) {
			i |= HOTKEYF_CONTROL;
		}
		if (si->copy_modifiers & MOD_ALT) {
			i |= HOTKEYF_ALT;
		}
		if (si->copy_modifiers & MOD_WIN) {
			i |= HOTKEYF_WIN;
		}
		if (si->copy_virtkey != 0) {
			SendDlgItemMessage(hDlg, IDC_HOTKEY_COPY, HKM_SETHOTKEY,
				(WPARAM)MAKEWORD(si->copy_virtkey, i), 0);
		}
		SetDlgItemInt(hDlg, IDC_EDIT_COPY_WAIT, si->copy_wait, FALSE);

		// 貼り付け
		i = 0;
		if (si->paste_modifiers & MOD_SHIFT) {
			i |= HOTKEYF_SHIFT;
		}
		if (si->paste_modifiers & MOD_CONTROL) {
			i |= HOTKEYF_CONTROL;
		}
		if (si->paste_modifiers & MOD_ALT) {
			i |= HOTKEYF_ALT;
		}
		if (si->paste_modifiers & MOD_WIN) {
			i |= HOTKEYF_WIN;
		}
		if (si->paste_virtkey != 0) {
			SendDlgItemMessage(hDlg, IDC_HOTKEY_PASTE, HKM_SETHOTKEY,
				(WPARAM)MAKEWORD(si->paste_virtkey, i), 0);
		}
		SetDlgItemInt(hDlg, IDC_EDIT_PASTE_WAIT, si->paste_wait, FALSE);
		
		SetWindowLong(hDlg, GWL_USERDATA, lParam);
		break;

	case WM_CLOSE:
#ifdef OP_XP_STYLE
		if (hTheme != 0) {
			close_theme(hTheme);
		}
#endif	// OP_XP_STYLE
		EndDialog(hDlg, FALSE);
		break;

	case WM_DRAWITEM:
		// ボタンの描画
#ifdef OP_XP_STYLE
		if (hTheme != 0) {
			draw_theme_scroll((LPDRAWITEMSTRUCT)lParam, DFCS_SCROLLDOWN, hTheme);
		} else {
			draw_scroll_sontrol((LPDRAWITEMSTRUCT)lParam, DFCS_SCROLLDOWN);
		}
#else	// OP_XP_STYLE
		draw_scroll_sontrol((LPDRAWITEMSTRUCT)lParam, DFCS_SCROLLDOWN);
#endif	// OP_XP_STYLE
		break;

#ifdef OP_XP_STYLE
	case WM_THEMECHANGED:
		// テーマの変更
		if (hTheme != 0) {
			close_theme(hTheme);
		}
		hTheme = open_theme(GetDlgItem(hDlg, IDC_BUTTON_FORMAT), L"SCROLLBAR");
		break;
#endif	// OP_XP_STYLE

	case WM_NOTIFY:
		if (((NMHDR *)lParam)->hwndFrom == GetWindow(GetDlgItem(hDlg, IDC_LIST_WINDOW), GW_CHILD)) {
			return listview_header_notify_proc(GetDlgItem(hDlg, IDC_LIST_WINDOW), lParam);
		}
		listview_notify_proc(hDlg, lParam, GetDlgItem(hDlg, IDC_LIST_WINDOW));
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_BUTTON_RELOAD:
			ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_LIST_WINDOW));
			EnumWindows((WNDENUMPROC)enum_windows_proc, (LPARAM)GetDlgItem(hDlg, IDC_LIST_WINDOW));
			break;

		case IDC_BUTTON_SELECT:
		case IDC_BUTTON_EDIT:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_WINDOW), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			ListView_GetItemText(GetDlgItem(hDlg, IDC_LIST_WINDOW), i, 0, buf, BUF_SIZE - 1);
			SendDlgItemMessage(hDlg, IDC_EDIT_TITLE, WM_SETTEXT, 0, (LPARAM)buf);
			ListView_GetItemText(GetDlgItem(hDlg, IDC_LIST_WINDOW), i, 1, buf, BUF_SIZE - 1);
			SendDlgItemMessage(hDlg, IDC_EDIT_CLASSNAME, WM_SETTEXT, 0, (LPARAM)buf);
			break;

		case IDOK:
			*buf = TEXT('\0');
			SendDlgItemMessage(hDlg, IDC_EDIT_TITLE, WM_GETTEXT, BUF_SIZE - 1, (LPARAM)buf);
			*tmp = TEXT('\0');
			SendDlgItemMessage(hDlg, IDC_EDIT_CLASSNAME, WM_GETTEXT, BUF_SIZE - 1, (LPARAM)tmp);
			if (*buf == TEXT('\0') && *tmp == TEXT('\0')) {
				MessageBox(hDlg, message_get_res(IDS_SENDKEY_ERR_INPUT), WINDOW_TITLE, MB_ICONEXCLAMATION);
				SetFocus(GetDlgItem(hDlg, IDC_EDIT_TITLE));
				break;
			}

			if ((si = (SENDKEY_INFO *)GetWindowLong(hDlg, GWL_USERDATA)) == NULL) {
				si = mem_calloc(sizeof(SENDKEY_INFO));
			}
			if (si != NULL) {
				// 設定取得
				alloc_get_text(GetDlgItem(hDlg, IDC_EDIT_TITLE), &si->title);
				alloc_get_text(GetDlgItem(hDlg, IDC_EDIT_CLASSNAME), &si->class_name);

				// コピー
				i = SendDlgItemMessage(hDlg,IDC_HOTKEY_COPY, HKM_GETHOTKEY, 0, 0);
				si->copy_virtkey = LOBYTE(i);
				i = HIBYTE(i);
				si->copy_modifiers = ((i & HOTKEYF_SHIFT) ? MOD_SHIFT : 0) |
					((i & HOTKEYF_CONTROL) ? MOD_CONTROL : 0) |
					((i & HOTKEYF_ALT) ? MOD_ALT : 0) |
					((i & HOTKEYF_WIN) ? MOD_WIN : 0);
				si->copy_wait = GetDlgItemInt(hDlg, IDC_EDIT_COPY_WAIT, NULL, FALSE);

				// 貼り付け
				i = SendDlgItemMessage(hDlg,IDC_HOTKEY_PASTE, HKM_GETHOTKEY, 0, 0);
				si->paste_virtkey = LOBYTE(i);
				i = HIBYTE(i);
				si->paste_modifiers = ((i & HOTKEYF_SHIFT) ? MOD_SHIFT : 0) |
					((i & HOTKEYF_CONTROL) ? MOD_CONTROL : 0) |
					((i & HOTKEYF_ALT) ? MOD_ALT : 0) |
					((i & HOTKEYF_WIN) ? MOD_WIN : 0);
				si->paste_wait = GetDlgItemInt(hDlg, IDC_EDIT_PASTE_WAIT, NULL, FALSE);
			}

			if (GetWindowLong(hDlg, GWL_USERDATA) == 0) {
				// 新規
				HWND pWnd = PropSheet_GetCurrentPageHwnd(GetParent(hDlg));
				listview_set_sendkey(GetDlgItem(pWnd, IDC_LIST_SENDKEY), si, FALSE);
			}
#ifdef OP_XP_STYLE
			if (hTheme != 0) {
				close_theme(hTheme);
			}
#endif	// OP_XP_STYLE
			EndDialog(hDlg, TRUE);
			break;

		case IDCANCEL:
#ifdef OP_XP_STYLE
			if (hTheme != 0) {
				close_theme(hTheme);
			}
#endif	// OP_XP_STYLE
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
 * listview_set_text - ListViewのテキストを設定
 */
static void listview_set_text(const HWND hListView, const int i)
{
	SENDKEY_INFO *si;
	TCHAR buf[BUF_SIZE];

	if ((si = (SENDKEY_INFO *)listview_get_lparam(hListView, i)) == NULL) {
		return;
	}
	// タイトル
	ListView_SetItemText(hListView, i, 0, si->title);
	// クラス名
	ListView_SetItemText(hListView, i, 1, si->class_name);

	// コピー
	get_keyname(si->copy_modifiers, si->copy_virtkey, buf);
	ListView_SetItemText(hListView, i, 2, buf);

	// 貼り付け
	get_keyname(si->paste_modifiers, si->paste_virtkey, buf);
	ListView_SetItemText(hListView, i, 3, buf);
}

/*
 * listview_set_sendkey - ListViewにキー設定を追加する
 */
static void listview_set_sendkey(const HWND hListView, SENDKEY_INFO *si, const BOOL copy)
{
	LV_ITEM lvi;
	SENDKEY_INFO *new_si;
	int i;

	if (copy == TRUE) {
		if ((new_si = mem_calloc(sizeof(SENDKEY_INFO))) == NULL) {
			return;
		}
		CopyMemory(new_si, si, sizeof(SENDKEY_INFO));
		new_si->title = alloc_copy(si->title);
		new_si->class_name = alloc_copy(si->class_name);
	} else {
		new_si = si;
	}

	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.iItem = ListView_GetItemCount(hListView);
	lvi.iSubItem = 0;
	lvi.pszText = TEXT("");
	lvi.cchTextMax = BUF_SIZE - 1;
	lvi.iImage = 0;
	lvi.lParam = (LPARAM)new_si;
	i = ListView_InsertItem(hListView, &lvi);

	listview_set_text(hListView, i);
}

/*
 * listview_get_sendkey - キー設定の取得
 */
static SENDKEY_INFO *listview_get_sendkey(const HWND hListView, int *cnt)
{
	SENDKEY_INFO *si;
	SENDKEY_INFO *new_si;
	int i;

	if ((*cnt = ListView_GetItemCount(hListView)) == 0) {
		return NULL;
	}

	// 項目の作成
	if ((new_si = mem_calloc(sizeof(SENDKEY_INFO) * *cnt)) == NULL) {
		*cnt = 0;
		return NULL;
	}

	for (i = 0; i < *cnt; i++) {
		if ((si = (SENDKEY_INFO *)listview_get_lparam(hListView, i)) == NULL) {
			continue;
		}
		CopyMemory(new_si + i, si, sizeof(SENDKEY_INFO));
		(new_si + i)->title = alloc_copy(si->title);
		(new_si + i)->class_name = alloc_copy(si->class_name);
	}
	return new_si;
}

/*
 * listview_free_sendkey - キー設定の解放
 */
static void listview_free_sendkey(const HWND hListView)
{
	SENDKEY_INFO *si;
	int i;

	for (i = 0; i < ListView_GetItemCount(hListView); i++) {
		if ((si = (SENDKEY_INFO *)listview_get_lparam(hListView, i)) != NULL) {
			mem_free(&si->title);
			mem_free(&si->class_name);
			mem_free(&si);
		}
		listview_set_lparam(hListView, i, 0);
	}
}

/*
 * set_sendkey_proc - キー設定のプロシージャ
 */
BOOL CALLBACK set_sendkey_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LV_COLUMN lvc;
	SENDKEY_INFO *si;
	int i;
	BOOL enable;
#ifdef OP_XP_STYLE
	static long hThemeUp, hThemeDown;
#endif	// OP_XP_STYLE

	switch (uMsg) {
	case WM_INITDIALOG:
#ifdef OP_XP_STYLE
		// XP
		hThemeUp = open_theme(GetDlgItem(hDlg, IDC_BUTTON_UP), L"SCROLLBAR");
		hThemeDown = open_theme(GetDlgItem(hDlg, IDC_BUTTON_DOWN), L"SCROLLBAR");
#endif	// OP_XP_STYLE
		// スピンコントロールの設定
		SendDlgItemMessage(hDlg, IDC_SPIN_COPY_WAIT, UDM_SETRANGE, 0, (LPARAM)MAKELONG(UD_MAXVAL, 0));
		SendDlgItemMessage(hDlg, IDC_SPIN_PASTE_WAIT, UDM_SETRANGE, 0, (LPARAM)MAKELONG(UD_MAXVAL, 0));

		// コピー
		i = 0;
		if (option.def_copy_modifiers & MOD_SHIFT) {
			i |= HOTKEYF_SHIFT;
		}
		if (option.def_copy_modifiers & MOD_CONTROL) {
			i |= HOTKEYF_CONTROL;
		}
		if (option.def_copy_modifiers & MOD_ALT) {
			i |= HOTKEYF_ALT;
		}
		if (option.def_copy_modifiers & MOD_WIN) {
			i |= HOTKEYF_WIN;
		}
		if (option.def_copy_virtkey != 0) {
			SendDlgItemMessage(hDlg, IDC_HOTKEY_COPY, HKM_SETHOTKEY,
				(WPARAM)MAKEWORD(option.def_copy_virtkey, i), 0);
		}
		SetDlgItemInt(hDlg, IDC_EDIT_COPY_WAIT, option.def_copy_wait, FALSE);

		// 貼り付け
		i = 0;
		if (option.def_paste_modifiers & MOD_SHIFT) {
			i |= HOTKEYF_SHIFT;
		}
		if (option.def_paste_modifiers & MOD_CONTROL) {
			i |= HOTKEYF_CONTROL;
		}
		if (option.def_paste_modifiers & MOD_ALT) {
			i |= HOTKEYF_ALT;
		}
		if (option.def_paste_modifiers & MOD_WIN) {
			i |= HOTKEYF_WIN;
		}
		if (option.def_paste_virtkey != 0) {
			SendDlgItemMessage(hDlg, IDC_HOTKEY_PASTE, HKM_SETHOTKEY,
				(WPARAM)MAKEWORD(option.def_paste_virtkey, i), 0);
		}
		SetDlgItemInt(hDlg, IDC_EDIT_PASTE_WAIT, option.def_paste_wait, FALSE);

		// リストビューのカラムの設定
		i = 0;
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = Scale(100);
		lvc.pszText = message_get_res(IDS_SENDKEY_LIST_TITLE);
		lvc.iSubItem = i++;
		ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_SENDKEY), lvc.iSubItem, &lvc);

		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = Scale(100);
		lvc.pszText = message_get_res(IDS_SENDKEY_LIST_CLASSNAME);
		lvc.iSubItem = i++;
		ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_SENDKEY), lvc.iSubItem, &lvc);

		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = Scale(65);
		lvc.pszText = message_get_res(IDS_SENDKEY_LIST_COPY);
		lvc.iSubItem = i++;
		ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_SENDKEY), lvc.iSubItem, &lvc);

		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = Scale(65);
		lvc.pszText = message_get_res(IDS_SENDKEY_LIST_PASTE);
		lvc.iSubItem = i++;
		ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_SENDKEY), lvc.iSubItem, &lvc);

		// リストビューのスタイルの設定
		SetWindowLong(GetDlgItem(hDlg, IDC_LIST_SENDKEY), GWL_STYLE,
			GetWindowLong(GetDlgItem(hDlg, IDC_LIST_SENDKEY), GWL_STYLE) | LVS_SHOWSELALWAYS);
		SendDlgItemMessage(hDlg, IDC_LIST_SENDKEY, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
			LVS_EX_ONECLICKACTIVATE | LVS_EX_FULLROWSELECT |
			SendDlgItemMessage(hDlg, IDC_LIST_SENDKEY, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0));

		for (i = 0; i < option.sendkey_cnt; i++) {
			listview_set_sendkey(GetDlgItem(hDlg, IDC_LIST_SENDKEY), option.sendkey_info + i, TRUE);
		}
		SendMessage(hDlg, WM_LV_EVENT, LVN_ITEMCHANGED, 0);
		break;

	case WM_DESTROY:
		listview_free_sendkey(GetDlgItem(hDlg, IDC_LIST_SENDKEY));
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

	case WM_NOTIFY:
		if (listview_notify_proc(hDlg, lParam, GetDlgItem(hDlg, IDC_LIST_SENDKEY)) == 0) {
			return OptionNotifyProc(hDlg, uMsg, wParam, lParam);
		}
		break;

	case WM_LV_EVENT:
		switch (wParam) {
		case LVN_ITEMCHANGED:
			enable = (ListView_GetSelectedCount(GetDlgItem(hDlg, IDC_LIST_SENDKEY)) <= 0) ? FALSE : TRUE;
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_UP), enable);
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_DOWN), enable);
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_EDIT), enable);
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_DELETE), enable);
			break;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_BUTTON_UP:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_SENDKEY), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			if (i == 0) {
				break;
			}
			listview_move_item(GetDlgItem(hDlg, IDC_LIST_SENDKEY), i, -1);
			SetFocus(GetDlgItem(hDlg, LOWORD(wParam)));
			break;

		case IDC_BUTTON_DOWN:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_SENDKEY), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			if (i == ListView_GetItemCount(GetDlgItem(hDlg, IDC_LIST_SENDKEY)) - 1) {
				break;
			}
			listview_move_item(GetDlgItem(hDlg, IDC_LIST_SENDKEY), i, 1);
			SetFocus(GetDlgItem(hDlg, LOWORD(wParam)));
			break;

		case IDC_BUTTON_ADD:
			DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_SENDKEY_SET), hDlg, set_sendkey_item_proc, 0);
			break;

		case IDC_BUTTON_EDIT:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_SENDKEY), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_SENDKEY_SET), hDlg, set_sendkey_item_proc,
				listview_get_lparam(GetDlgItem(hDlg, IDC_LIST_SENDKEY), i));
			listview_set_text(GetDlgItem(hDlg, IDC_LIST_SENDKEY), i);
			break;

		case IDC_BUTTON_DELETE:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_SENDKEY), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			if (MessageBox(hDlg, message_get_res(IDS_OPTION_DELETE), WINDOW_TITLE, MB_ICONQUESTION | MB_YESNO) == IDNO) {
				break;
			}
			if ((si = (SENDKEY_INFO *)listview_get_lparam(GetDlgItem(hDlg, IDC_LIST_SENDKEY), i)) != NULL) {
				mem_free(&si->title);
				mem_free(&si->class_name);
				mem_free(&si);
			}
			ListView_DeleteItem(GetDlgItem(hDlg, IDC_LIST_SENDKEY), i);
			ListView_SetItemState(GetDlgItem(hDlg, IDC_LIST_SENDKEY), i,
				LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
			break;

		case IDOK:
			// コピー
			i = SendDlgItemMessage(hDlg,IDC_HOTKEY_COPY, HKM_GETHOTKEY, 0, 0);
			option.def_copy_virtkey = LOBYTE(i);
			i = HIBYTE(i);
			option.def_copy_modifiers = ((i & HOTKEYF_SHIFT) ? MOD_SHIFT : 0) |
				((i & HOTKEYF_CONTROL) ? MOD_CONTROL : 0) |
				((i & HOTKEYF_ALT) ? MOD_ALT : 0) |
				((i & HOTKEYF_WIN) ? MOD_WIN : 0);
			option.def_copy_wait = GetDlgItemInt(hDlg, IDC_EDIT_COPY_WAIT, NULL, FALSE);

			// 貼り付け
			i = SendDlgItemMessage(hDlg,IDC_HOTKEY_PASTE, HKM_GETHOTKEY, 0, 0);
			option.def_paste_virtkey = LOBYTE(i);
			i = HIBYTE(i);
			option.def_paste_modifiers = ((i & HOTKEYF_SHIFT) ? MOD_SHIFT : 0) |
				((i & HOTKEYF_CONTROL) ? MOD_CONTROL : 0) |
				((i & HOTKEYF_ALT) ? MOD_ALT : 0) |
				((i & HOTKEYF_WIN) ? MOD_WIN : 0);
			option.def_paste_wait = GetDlgItemInt(hDlg, IDC_EDIT_PASTE_WAIT, NULL, FALSE);

			for (i = 0; i < option.sendkey_cnt; i++) {
				mem_free(&((option.sendkey_info + i)->title));
				mem_free(&((option.sendkey_info + i)->class_name));
			}
			mem_free(&option.sendkey_info);

			option.sendkey_info = listview_get_sendkey(GetDlgItem(hDlg, IDC_LIST_SENDKEY), &option.sendkey_cnt);
			listview_free_sendkey(GetDlgItem(hDlg, IDC_LIST_SENDKEY));
			prop_ret = 1;
			break;

		case IDPCANCEL:
			break;
		}
		break;

	default:
		return FALSE;
	}
	return TRUE;
}
/* End of source */
