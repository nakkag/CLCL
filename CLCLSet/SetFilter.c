/*
 * CLCLSet
 *
 * SetFilter.c
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
#include "..\Format.h"
#include "..\Filter.h"
#include "..\ClipBoard.h"
#include "..\Message.h"
#include "..\dpi.h"

#include "CLCLSet.h"

#include "resource.h"

/* Define */
#define ID_ADD_TIMER					1

/* Global Variables */
extern HINSTANCE hInst;
extern int prop_ret;

extern TCHAR cmd_filter[];

// オプション
extern OPTION_INFO option;

/* Local Function Prototypes */
static BOOL CALLBACK set_filter_item_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static void listview_set_text(const HWND hListView, const int i);
static void listview_set_filter(const HWND hListView, FILTER_INFO *fi, const BOOL copy);
static FILTER_INFO *listview_get_filter(const HWND hListView, int *cnt);
static void listview_free_filter(const HWND hListView);

/*
 * set_filter_item_proc - フィルタの項目を設定
 */
static BOOL CALLBACK set_filter_item_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HMENU hMenu;
	RECT button_rect;
	FILTER_INFO *fi;
	TCHAR buf[BUF_SIZE];
	DWORD ret;
	UINT format;
#ifdef OP_XP_STYLE
	static long hTheme;
#endif	// OP_XP_STYLE

	switch (uMsg) {
	case WM_INITDIALOG:
#ifdef OP_XP_STYLE
		// XP
		hTheme = open_theme(GetDlgItem(hDlg, IDC_BUTTON_FORMAT), L"SCROLLBAR");
#endif	// OP_XP_STYLE
		// スピンコントロールの設定
		SendDlgItemMessage(hDlg, IDC_SPIN_SIZE, UDM_SETRANGE, 0, (LPARAM)MAKELONG(UD_MAXVAL, 0));

		if (lParam == 0) {
			// 新規追加
			if (*cmd_filter != TEXT('\0')) {
				SendDlgItemMessage(hDlg, IDC_EDIT_FORMAT_NAME, WM_SETTEXT, 0, (LPARAM)cmd_filter);
				*cmd_filter = TEXT('\0');
			}
			CheckDlgButton(hDlg, IDC_RADIO_ADD, 1);
			SetDlgItemInt(hDlg, IDC_EDIT_SIZE, 0, FALSE);
			SetWindowLong(hDlg, GWL_USERDATA, 0);
			break;
		}
		fi = (FILTER_INFO *)lParam;

		SendDlgItemMessage(hDlg, IDC_EDIT_FORMAT_NAME, WM_SETTEXT, 0, (LPARAM)fi->format_name);
		if (fi->action == FILTER_ACTION_ADD) {
			CheckDlgButton(hDlg, IDC_RADIO_ADD, 1);
		} else {
			CheckDlgButton(hDlg, IDC_RADIO_IGNORE, 1);
		}
		CheckDlgButton(hDlg, IDC_CHECK_NOSAVE, !fi->save);
		SetDlgItemInt(hDlg, IDC_EDIT_SIZE, fi->limit_size, FALSE);

		EnableWindow(GetDlgItem(hDlg, IDC_CHECK_NOSAVE), IsDlgButtonChecked(hDlg, IDC_RADIO_ADD));
		EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SIZE), IsDlgButtonChecked(hDlg, IDC_RADIO_ADD));

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
			draw_theme_scroll((LPDRAWITEMSTRUCT)lParam, DFCS_SCROLLRIGHT, hTheme);
		} else {
			draw_scroll_sontrol((LPDRAWITEMSTRUCT)lParam, DFCS_SCROLLRIGHT);
		}
#else	// OP_XP_STYLE
		draw_scroll_sontrol((LPDRAWITEMSTRUCT)lParam, DFCS_SCROLLRIGHT);
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

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_BUTTON_FORMAT:
			// 形式選択
			if (OpenClipboard(hDlg) == FALSE) {
				break;
			}
			// メニューの作成
			hMenu = CreatePopupMenu();
			format = 0;
			ret = 1;
			while ((format = EnumClipboardFormats(format)) != 0) {
				clipboard_get_format(format, buf);
				AppendMenu(hMenu, MF_STRING, ret++, buf);
			}
			CloseClipboard();
			if (ret == 1) {
				DestroyMenu(hMenu);
				break;
			}

			// メニューの表示
			GetWindowRect(GetDlgItem(hDlg, LOWORD(wParam)), (LPRECT)&button_rect);
			ret = TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_RETURNCMD, button_rect.right, button_rect.top, 0, hDlg, NULL);
			if (ret > 0) {
				GetMenuString(hMenu, ret, buf, BUF_SIZE - 1, MF_BYCOMMAND);
				SendDlgItemMessage(hDlg, IDC_EDIT_FORMAT_NAME, EM_REPLACESEL, 0, (LPARAM)buf);
			}
			DestroyMenu(hMenu);
			break;

		case IDC_RADIO_ADD:
		case IDC_RADIO_IGNORE:
			EnableWindow(GetDlgItem(hDlg, IDC_CHECK_NOSAVE), IsDlgButtonChecked(hDlg, IDC_RADIO_ADD));
			EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SIZE), IsDlgButtonChecked(hDlg, IDC_RADIO_ADD));
			break;

		case IDOK:
			*buf = TEXT('\0');
			SendDlgItemMessage(hDlg, IDC_EDIT_FORMAT_NAME, WM_GETTEXT, BUF_SIZE - 1, (LPARAM)buf);
			if (*buf == TEXT('\0')) {
				MessageBox(hDlg, message_get_res(IDS_FILTER_ERR_NAME), WINDOW_TITLE, MB_ICONEXCLAMATION);
				SetFocus(GetDlgItem(hDlg, IDC_EDIT_FORMAT_NAME));
				break;
			}

			if ((fi = (FILTER_INFO *)GetWindowLong(hDlg, GWL_USERDATA)) == NULL) {
				fi = mem_calloc(sizeof(FILTER_INFO));
			}
			if (fi != NULL) {
				// 設定取得
				alloc_get_text(GetDlgItem(hDlg, IDC_EDIT_FORMAT_NAME), &fi->format_name);
				fi->action = IsDlgButtonChecked(hDlg, IDC_RADIO_IGNORE);
				fi->save = !IsDlgButtonChecked(hDlg, IDC_CHECK_NOSAVE);
				fi->limit_size = GetDlgItemInt(hDlg, IDC_EDIT_SIZE, NULL, FALSE);
			}

			if (GetWindowLong(hDlg, GWL_USERDATA) == 0) {
				// 新規
				HWND pWnd = PropSheet_GetCurrentPageHwnd(GetParent(hDlg));
				listview_set_filter(GetDlgItem(pWnd, IDC_LIST_FILTER), fi, FALSE);
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
	FILTER_INFO *fi;
	TCHAR buf[BUF_SIZE];
	TCHAR *p;

	if ((fi = (FILTER_INFO *)listview_get_lparam(hListView, i)) == NULL) {
		return;
	}
	// 形式名
	ListView_SetItemText(hListView, i, 0, fi->format_name);

	// 動作
	if (fi->action == FILTER_ACTION_ADD) {
		if (fi->save == FILTER_SAVE_SAVE) {
			p = message_get_res(IDS_FILTER_ACTION_ADD);
		} else {
			p = message_get_res(IDS_FILTER_ACTION_ADD_NOSAVE);
		}
	} else {
		p = message_get_res(IDS_FILTER_ACTION_IGNORE);
	}
	ListView_SetItemText(hListView, i, 1, p);

	// 制限サイズ
	wsprintf(buf, TEXT("%d"), fi->limit_size);
	ListView_SetItemText(hListView, i, 2, buf);
}

/*
 * listview_set_filter - ListViewにフィルタ情報を追加する
 */
static void listview_set_filter(const HWND hListView, FILTER_INFO *fi, const BOOL copy)
{
	LV_ITEM lvi;
	FILTER_INFO *new_fi;
	int i;

	if (copy == TRUE) {
		if ((new_fi = mem_calloc(sizeof(FILTER_INFO))) == NULL) {
			return;
		}
		new_fi->format_name = alloc_copy(fi->format_name);
		new_fi->action = fi->action;
		new_fi->save = fi->save;
		new_fi->limit_size = fi->limit_size;
	} else {
		new_fi = fi;
	}

	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.iItem = ListView_GetItemCount(hListView);
	lvi.iSubItem = 0;
	lvi.pszText = TEXT("");
	lvi.cchTextMax = BUF_SIZE - 1;
	lvi.iImage = 0;
	lvi.lParam = (LPARAM)new_fi;
	i = ListView_InsertItem(hListView, &lvi);

	listview_set_text(hListView, i);
}

/*
 * listview_get_filter - フィルタ情報の取得
 */
static FILTER_INFO *listview_get_filter(const HWND hListView, int *cnt)
{
	FILTER_INFO *fi;
	FILTER_INFO *new_fi;
	int i;

	if ((*cnt = ListView_GetItemCount(hListView)) == 0) {
		return NULL;
	}

	// 項目の作成
	if ((new_fi = mem_calloc(sizeof(FILTER_INFO) * *cnt)) == NULL) {
		*cnt = 0;
		return NULL;
	}

	for (i = 0; i < *cnt; i++) {
		if ((fi = (FILTER_INFO *)listview_get_lparam(hListView, i)) == NULL) {
			continue;
		}
		(new_fi + i)->format_name = alloc_copy(fi->format_name);
		(new_fi + i)->action = fi->action;
		(new_fi + i)->save = fi->save;
		(new_fi + i)->limit_size = fi->limit_size;
	}
	return new_fi;
}

/*
 * listview_free_filter - フィルタ情報の解放
 */
static void listview_free_filter(const HWND hListView)
{
	FILTER_INFO *fi;
	int i;

	for (i = 0; i < ListView_GetItemCount(hListView); i++) {
		if ((fi = (FILTER_INFO *)listview_get_lparam(hListView, i)) != NULL) {
			mem_free(&fi->format_name);
			mem_free(&fi);
		}
		listview_set_lparam(hListView, i, 0);
	}
}

/*
 * set_filter_proc - フィルタ設定のプロシージャ
 */
BOOL CALLBACK set_filter_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LV_COLUMN lvc;
	FILTER_INFO *fi;
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
		if (option.filter_all_action == FILTER_ACTION_ADD) {
			CheckDlgButton(hDlg, IDC_RADIO_ALL_ADD, 1);
		} else {
			CheckDlgButton(hDlg, IDC_RADIO_ALL_IGNORE, 1);
		}

		// リストビューのカラムの設定
		i = 0;
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = Scale(150);
		lvc.pszText = message_get_res(IDS_FILTER_LIST_NAME);
		lvc.iSubItem = i++;
		ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_FILTER), lvc.iSubItem, &lvc);

		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = Scale(100);
		lvc.pszText = message_get_res(IDS_FILTER_LIST_ACTION);
		lvc.iSubItem = i++;
		ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_FILTER), lvc.iSubItem, &lvc);

		lvc.fmt = LVCFMT_RIGHT;
		lvc.cx = Scale(80);
		lvc.pszText = message_get_res(IDS_FILTER_LIST_SIZE);
		lvc.iSubItem = i++;
		ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_FILTER), lvc.iSubItem, &lvc);

		// リストビューのスタイルの設定
		SetWindowLong(GetDlgItem(hDlg, IDC_LIST_FILTER), GWL_STYLE,
			GetWindowLong(GetDlgItem(hDlg, IDC_LIST_FILTER), GWL_STYLE) | LVS_SHOWSELALWAYS);
		SendDlgItemMessage(hDlg, IDC_LIST_FILTER, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
			LVS_EX_ONECLICKACTIVATE | LVS_EX_FULLROWSELECT |
			SendDlgItemMessage(hDlg, IDC_LIST_FILTER, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0));

		for (i = 0; i < option.filter_cnt; i++) {
			listview_set_filter(GetDlgItem(hDlg, IDC_LIST_FILTER), option.filter_info + i, TRUE);
		}
		SendMessage(hDlg, WM_LV_EVENT, LVN_ITEMCHANGED, 0);

		if (*cmd_filter != TEXT('\0')) {
			SetTimer(hDlg, ID_ADD_TIMER, 1, NULL);
		}
		break;

	case WM_DESTROY:
		listview_free_filter(GetDlgItem(hDlg, IDC_LIST_FILTER));
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
		if (listview_notify_proc(hDlg, lParam, GetDlgItem(hDlg, IDC_LIST_FILTER)) == 0) {
			return OptionNotifyProc(hDlg, uMsg, wParam, lParam);
		}
		break;

	case WM_LV_EVENT:
		switch (wParam) {
		case LVN_ITEMCHANGED:
			enable = (ListView_GetSelectedCount(GetDlgItem(hDlg, IDC_LIST_FILTER)) <= 0) ? FALSE : TRUE;
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_UP), enable);
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_DOWN), enable);
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_EDIT), enable);
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_DELETE), enable);
			break;
		}
		break;

	case WM_TIMER:
		switch (wParam) {
		case ID_ADD_TIMER:
			KillTimer(hDlg, wParam);
			if ((i = filter_get_index(cmd_filter, 0)) == -1) {
				// 追加
				SendMessage(hDlg, WM_COMMAND, IDC_BUTTON_ADD, 0);
			} else {
				ListView_SetItemState(GetDlgItem(hDlg, IDC_LIST_FILTER), i,
					LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
				// 編集
				SendMessage(hDlg, WM_COMMAND, IDC_BUTTON_EDIT, 0);
			}
			break;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_BUTTON_UP:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_FILTER), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			if (i == 0) {
				break;
			}
			listview_move_item(GetDlgItem(hDlg, IDC_LIST_FILTER), i, -1);
			SetFocus(GetDlgItem(hDlg, LOWORD(wParam)));
			break;

		case IDC_BUTTON_DOWN:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_FILTER), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			if (i == ListView_GetItemCount(GetDlgItem(hDlg, IDC_LIST_FILTER)) - 1) {
				break;
			}
			listview_move_item(GetDlgItem(hDlg, IDC_LIST_FILTER), i, 1);
			SetFocus(GetDlgItem(hDlg, LOWORD(wParam)));
			break;

		case IDC_BUTTON_ADD:
			DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_FILTER_SET), hDlg, set_filter_item_proc, 0);
			break;

		case IDC_BUTTON_EDIT:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_FILTER), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_FILTER_SET), hDlg, set_filter_item_proc,
				listview_get_lparam(GetDlgItem(hDlg, IDC_LIST_FILTER), i));
			listview_set_text(GetDlgItem(hDlg, IDC_LIST_FILTER), i);
			break;

		case IDC_BUTTON_DELETE:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_FILTER), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			if (MessageBox(hDlg, message_get_res(IDS_OPTION_DELETE), WINDOW_TITLE, MB_ICONQUESTION | MB_YESNO) == IDNO) {
				break;
			}
			if ((fi = (FILTER_INFO *)listview_get_lparam(GetDlgItem(hDlg, IDC_LIST_FILTER), i)) != NULL) {
				mem_free(&fi->format_name);
				ini_free_format_name(fi->fn, fi->fn_cnt);
				mem_free(&fi);
			}
			ListView_DeleteItem(GetDlgItem(hDlg, IDC_LIST_FILTER), i);
			ListView_SetItemState(GetDlgItem(hDlg, IDC_LIST_FILTER), i,
				LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
			break;

		case IDOK:
			for (i = 0; i < option.filter_cnt; i++) {
				mem_free(&((option.filter_info + i)->format_name));
				ini_free_format_name((option.filter_info + i)->fn, (option.filter_info + i)->fn_cnt);
			}
			mem_free(&option.filter_info);

			option.filter_all_action = IsDlgButtonChecked(hDlg, IDC_RADIO_ALL_IGNORE);
			option.filter_info = listview_get_filter(GetDlgItem(hDlg, IDC_LIST_FILTER), &option.filter_cnt);
			listview_free_filter(GetDlgItem(hDlg, IDC_LIST_FILTER));
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
