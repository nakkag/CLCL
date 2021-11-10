/*
 * CLCLSet
 *
 * SetWindow.c
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
#include "..\Filter.h"
#include "..\Message.h"
#include "..\dpi.h"

#include "CLCLSet.h"

#include "resource.h"

/* Define */

/* Global Variables */
extern HINSTANCE hInst;
extern int prop_ret;

// オプション
extern OPTION_INFO option;

/* Local Function Prototypes */
static BOOL CALLBACK set_window_item_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static void listview_set_text(const HWND hListView, const int i);
static void listview_set_window(const HWND hListView, WINDOW_FILTER_INFO *wfi, const BOOL copy);
static WINDOW_FILTER_INFO *listview_get_window(const HWND hListView, int *cnt);
static void listview_free_window(const HWND hListView);

/*
 * set_window_item_proc - ウィンドウフィルタの項目を設定
 */
static BOOL CALLBACK set_window_item_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LV_COLUMN lvc;
	WINDOW_FILTER_INFO *wfi;
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
		i = 0;
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = Scale(170);
		lvc.pszText = message_get_res(IDS_WINDOW_LIST_TITLE);
		lvc.iSubItem = i++;
		ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_WINDOW), lvc.iSubItem, &lvc);

		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = Scale(160);
		lvc.pszText = message_get_res(IDS_WINDOW_LIST_CLASSNAME);
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
			SetWindowLong(hDlg, GWL_USERDATA, 0);
			break;
		}
		wfi = (WINDOW_FILTER_INFO *)lParam;

		SendDlgItemMessage(hDlg, IDC_EDIT_TITLE, WM_SETTEXT, 0, (LPARAM)wfi->title);
		SendDlgItemMessage(hDlg, IDC_EDIT_CLASSNAME, WM_SETTEXT, 0, (LPARAM)wfi->class_name);

		CheckDlgButton(hDlg, IDC_CHECK_IGNORE, wfi->ignore);
		CheckDlgButton(hDlg, IDC_CHECK_FOCUS, !wfi->focus);
		CheckDlgButton(hDlg, IDC_CHECK_PASTE, wfi->paste);

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
				MessageBox(hDlg, message_get_res(IDS_WINDOW_ERR_INPUT), WINDOW_TITLE, MB_ICONEXCLAMATION);
				SetFocus(GetDlgItem(hDlg, IDC_EDIT_TITLE));
				break;
			}

			if ((wfi = (WINDOW_FILTER_INFO *)GetWindowLong(hDlg, GWL_USERDATA)) == NULL) {
				wfi = mem_calloc(sizeof(WINDOW_FILTER_INFO));
			}
			if (wfi != NULL) {
				// 設定取得
				alloc_get_text(GetDlgItem(hDlg, IDC_EDIT_TITLE), &wfi->title);
				alloc_get_text(GetDlgItem(hDlg, IDC_EDIT_CLASSNAME), &wfi->class_name);

				wfi->ignore = IsDlgButtonChecked(hDlg, IDC_CHECK_IGNORE);
				wfi->focus = !IsDlgButtonChecked(hDlg, IDC_CHECK_FOCUS);
				wfi->paste = IsDlgButtonChecked(hDlg, IDC_CHECK_PASTE);
			}

			if (GetWindowLong(hDlg, GWL_USERDATA) == 0) {
				// 新規
				HWND pWnd = PropSheet_GetCurrentPageHwnd(GetParent(hDlg));
				listview_set_window(GetDlgItem(pWnd, IDC_LIST_WINDOW), wfi, FALSE);
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
	WINDOW_FILTER_INFO *wfi;

	if ((wfi = (WINDOW_FILTER_INFO *)listview_get_lparam(hListView, i)) == NULL) {
		return;
	}
	// タイトル
	ListView_SetItemText(hListView, i, 0, wfi->title);
	// クラス名
	ListView_SetItemText(hListView, i, 1, wfi->class_name);

	ListView_SetItemText(hListView, i, 2, (wfi->ignore != 0) ?
		message_get_res(IDS_WINDOW_LIST_IGNORE_ON) : message_get_res(IDS_WINDOW_LIST_IGNORE_OFF));
	ListView_SetItemText(hListView, i, 3, (wfi->focus == 0) ?
		message_get_res(IDS_WINDOW_LIST_FOCUS_ON) : message_get_res(IDS_WINDOW_LIST_FOCUS_OFF));
	ListView_SetItemText(hListView, i, 4, (wfi->paste != 0) ?
		message_get_res(IDS_WINDOW_LIST_PASTE_ON) : message_get_res(IDS_WINDOW_LIST_PASTE_OFF));
}

/*
 * listview_set_window - ListViewにウィンドウフィルタ情報を追加する
 */
static void listview_set_window(const HWND hListView, WINDOW_FILTER_INFO *wfi, const BOOL copy)
{
	LV_ITEM lvi;
	WINDOW_FILTER_INFO *new_wfi;
	int i;

	if (copy == TRUE) {
		if ((new_wfi = mem_calloc(sizeof(WINDOW_FILTER_INFO))) == NULL) {
			return;
		}
		CopyMemory(new_wfi, wfi, sizeof(WINDOW_FILTER_INFO));
		new_wfi->title = alloc_copy(wfi->title);
		new_wfi->class_name = alloc_copy(wfi->class_name);
	} else {
		new_wfi = wfi;
	}

	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.iItem = ListView_GetItemCount(hListView);
	lvi.iSubItem = 0;
	lvi.pszText = TEXT("");
	lvi.cchTextMax = BUF_SIZE - 1;
	lvi.iImage = 0;
	lvi.lParam = (LPARAM)new_wfi;
	i = ListView_InsertItem(hListView, &lvi);

	listview_set_text(hListView, i);
}

/*
 * listview_get_window - ウィンドウフィルタ情報の取得
 */
static WINDOW_FILTER_INFO *listview_get_window(const HWND hListView, int *cnt)
{
	WINDOW_FILTER_INFO *wfi;
	WINDOW_FILTER_INFO *new_wfi;
	int i;

	if ((*cnt = ListView_GetItemCount(hListView)) == 0) {
		return NULL;
	}

	// 項目の作成
	if ((new_wfi = mem_calloc(sizeof(WINDOW_FILTER_INFO) * *cnt)) == NULL) {
		*cnt = 0;
		return NULL;
	}

	for (i = 0; i < *cnt; i++) {
		if ((wfi = (WINDOW_FILTER_INFO *)listview_get_lparam(hListView, i)) == NULL) {
			continue;
		}
		CopyMemory(new_wfi + i, wfi, sizeof(WINDOW_FILTER_INFO));
		(new_wfi + i)->title = alloc_copy(wfi->title);
		(new_wfi + i)->class_name = alloc_copy(wfi->class_name);
	}
	return new_wfi;
}

/*
 * listview_free_window - ウィンドウフィルタ情報の解放
 */
static void listview_free_window(const HWND hListView)
{
	WINDOW_FILTER_INFO *wfi;
	int i;

	for (i = 0; i < ListView_GetItemCount(hListView); i++) {
		if ((wfi = (WINDOW_FILTER_INFO *)listview_get_lparam(hListView, i)) != NULL) {
			mem_free(&wfi->title);
			mem_free(&wfi->class_name);
			mem_free(&wfi);
		}
		listview_set_lparam(hListView, i, 0);
	}
}

/*
 * set_window_proc - ウィンドウフィルタ設定のプロシージャ
 */
BOOL CALLBACK set_window_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LV_COLUMN lvc;
	WINDOW_FILTER_INFO *wfi;
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
		// リストビューのカラムの設定
		i = 0;
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = Scale(100);
		lvc.pszText = message_get_res(IDS_WINDOW_LIST_TITLE);
		lvc.iSubItem = i++;
		ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_WINDOW), lvc.iSubItem, &lvc);

		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = Scale(100);
		lvc.pszText = message_get_res(IDS_WINDOW_LIST_CLASSNAME);
		lvc.iSubItem = i++;
		ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_WINDOW), lvc.iSubItem, &lvc);

		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = Scale(70);
		lvc.pszText = message_get_res(IDS_WINDOW_LIST_IGNORE);
		lvc.iSubItem = i++;
		ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_WINDOW), lvc.iSubItem, &lvc);

		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = Scale(70);
		lvc.pszText = message_get_res(IDS_WINDOW_LIST_FOCUS);
		lvc.iSubItem = i++;
		ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_WINDOW), lvc.iSubItem, &lvc);

		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = Scale(70);
		lvc.pszText = message_get_res(IDS_WINDOW_LIST_PASTE);
		lvc.iSubItem = i++;
		ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_WINDOW), lvc.iSubItem, &lvc);

		// リストビューのスタイルの設定
		SetWindowLong(GetDlgItem(hDlg, IDC_LIST_WINDOW), GWL_STYLE,
			GetWindowLong(GetDlgItem(hDlg, IDC_LIST_WINDOW), GWL_STYLE) | LVS_SHOWSELALWAYS);
		SendDlgItemMessage(hDlg, IDC_LIST_WINDOW, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
			LVS_EX_ONECLICKACTIVATE | LVS_EX_FULLROWSELECT |
			SendDlgItemMessage(hDlg, IDC_LIST_WINDOW, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0));

		for (i = 0; i < option.window_filter_cnt; i++) {
			listview_set_window(GetDlgItem(hDlg, IDC_LIST_WINDOW), option.window_filter_info + i, TRUE);
		}
		SendMessage(hDlg, WM_LV_EVENT, LVN_ITEMCHANGED, 0);
		break;

	case WM_DESTROY:
		listview_free_window(GetDlgItem(hDlg, IDC_LIST_WINDOW));
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
		if (listview_notify_proc(hDlg, lParam, GetDlgItem(hDlg, IDC_LIST_WINDOW)) == 0) {
			return OptionNotifyProc(hDlg, uMsg, wParam, lParam);
		}
		break;

	case WM_LV_EVENT:
		switch (wParam) {
		case LVN_ITEMCHANGED:
			enable = (ListView_GetSelectedCount(GetDlgItem(hDlg, IDC_LIST_WINDOW)) <= 0) ? FALSE : TRUE;
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
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_WINDOW), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			if (i == 0) {
				break;
			}
			listview_move_item(GetDlgItem(hDlg, IDC_LIST_WINDOW), i, -1);
			SetFocus(GetDlgItem(hDlg, LOWORD(wParam)));
			break;

		case IDC_BUTTON_DOWN:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_WINDOW), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			if (i == ListView_GetItemCount(GetDlgItem(hDlg, IDC_LIST_WINDOW)) - 1) {
				break;
			}
			listview_move_item(GetDlgItem(hDlg, IDC_LIST_WINDOW), i, 1);
			SetFocus(GetDlgItem(hDlg, LOWORD(wParam)));
			break;

		case IDC_BUTTON_ADD:
			DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_WINDOW_SET), hDlg, set_window_item_proc, 0);
			break;

		case IDC_BUTTON_EDIT:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_WINDOW), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_WINDOW_SET), hDlg, set_window_item_proc,
				listview_get_lparam(GetDlgItem(hDlg, IDC_LIST_WINDOW), i));
			listview_set_text(GetDlgItem(hDlg, IDC_LIST_WINDOW), i);
			break;

		case IDC_BUTTON_DELETE:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_WINDOW), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			if (MessageBox(hDlg, message_get_res(IDS_OPTION_DELETE), WINDOW_TITLE, MB_ICONQUESTION | MB_YESNO) == IDNO) {
				break;
			}
			if ((wfi = (WINDOW_FILTER_INFO *)listview_get_lparam(GetDlgItem(hDlg, IDC_LIST_WINDOW), i)) != NULL) {
				mem_free(&wfi->title);
				mem_free(&wfi->class_name);
				mem_free(&wfi);
			}
			ListView_DeleteItem(GetDlgItem(hDlg, IDC_LIST_WINDOW), i);
			ListView_SetItemState(GetDlgItem(hDlg, IDC_LIST_WINDOW), i,
				LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
			break;

		case IDOK:
			for (i = 0; i < option.window_filter_cnt; i++) {
				mem_free(&((option.window_filter_info + i)->title));
				mem_free(&((option.window_filter_info + i)->class_name));
			}
			mem_free(&option.window_filter_info);

			option.window_filter_info = listview_get_window(GetDlgItem(hDlg, IDC_LIST_WINDOW), &option.window_filter_cnt);
			listview_free_window(GetDlgItem(hDlg, IDC_LIST_WINDOW));
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
