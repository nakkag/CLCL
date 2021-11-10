/*
 * CLCLSet
 *
 * SetFormat.c
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
#include "..\Format.h"
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
extern TCHAR work_path[];

extern TCHAR cmd_format[];

// オプション
extern OPTION_INFO option;

/* Local Function Prototypes */
static BOOL CALLBACK get_format_header(const HWND hWnd, const int index, FORMAT_GET_INFO *fgi);
static BOOL dll_to_list(const HWND hDlg, const FARPROC func_get_format_header);
static BOOL CALLBACK select_headers_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK select_header_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK set_format_item_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static BOOL show_property(const HWND hDlg, const FORMAT_INFO *fi);
static void listview_set_text(const HWND hListView, const int i);
static void listview_set_format(const HWND hListView, FORMAT_INFO *fi, const BOOL copy);
static FORMAT_INFO *listview_get_format(const HWND hListView, int *cnt);
static void listview_free_format(const HWND hListView);

/*
 * get_format_header - 内部形式を処理するヘッダの取得
 */
static BOOL CALLBACK get_format_header(const HWND hWnd, const int index, FORMAT_GET_INFO *fgi)
{
	switch (index) {
	case 0:
		lstrcpy(fgi->format_name, TEXT("TEXT"));
		lstrcpy(fgi->func_header, TEXT("text_"));
		lstrcpy(fgi->comment, message_get_res(IDS_FORMAT_COMMENT_TEXT));
		return TRUE;

	case 1:
		lstrcpy(fgi->format_name, TEXT("BITMAP, DIB"));
		lstrcpy(fgi->func_header, TEXT("bitmap_"));
		lstrcpy(fgi->comment, message_get_res(IDS_FORMAT_COMMENT_BITMAP));
		return TRUE;

	case 2:
		lstrcpy(fgi->format_name, TEXT("DROP FILE LIST"));
		lstrcpy(fgi->func_header, TEXT("file_"));
		lstrcpy(fgi->comment, message_get_res(IDS_FORMAT_COMMENT_FILE));
		return TRUE;
	}
	return FALSE;
}

/*
 * dll_to_list - DLL内の関数をリストビューに表示
 */
static BOOL dll_to_list(const HWND hDlg, const FARPROC func_get_format_header)
{
	LV_COLUMN lvc;
	LV_ITEM lvi;
	FORMAT_GET_INFO fgi;
	int i, j;

	// リストビューの設定
	i = 0;
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = Scale(100);
	lvc.pszText = message_get_res(IDS_FORMAT_LIST_FORMAT);
	lvc.iSubItem = i++;
	ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_HEADER), lvc.iSubItem, &lvc);

	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = Scale(80);
	lvc.pszText = message_get_res(IDS_FORMAT_LIST_HEADER);
	lvc.iSubItem = i++;
	ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_HEADER), lvc.iSubItem, &lvc);

	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = Scale(120);
	lvc.pszText = message_get_res(IDS_FORMAT_LIST_COMMENT);
	lvc.iSubItem = i++;
	ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_HEADER), lvc.iSubItem, &lvc);

	// リストビューのスタイルの設定
	SetWindowLong(GetDlgItem(hDlg, IDC_LIST_HEADER), GWL_STYLE,
		GetWindowLong(GetDlgItem(hDlg, IDC_LIST_HEADER), GWL_STYLE) | LVS_SHOWSELALWAYS);
	SendDlgItemMessage(hDlg, IDC_LIST_HEADER, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
		LVS_EX_ONECLICKACTIVATE | LVS_EX_FULLROWSELECT |
		SendDlgItemMessage(hDlg, IDC_LIST_HEADER, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0));

	// アイテムを追加
	i = 0;
	while (1) {
		ZeroMemory(&fgi, sizeof(FORMAT_GET_INFO));
		fgi.struct_size = sizeof(FORMAT_GET_INFO);
		if (func_get_format_header(hDlg, i++, &fgi) == FALSE) {
			break;
		}
		ZeroMemory(&lvi, sizeof(LV_ITEM));
		lvi.mask = LVIF_TEXT;
		lvi.iItem = ListView_GetItemCount(GetDlgItem(hDlg, IDC_LIST_HEADER));
		lvi.iSubItem = 0;
		lvi.pszText = fgi.format_name;
		lvi.cchTextMax = BUF_SIZE - 1;
		j = ListView_InsertItem(GetDlgItem(hDlg, IDC_LIST_HEADER), &lvi);
		ListView_SetItemText(GetDlgItem(hDlg, IDC_LIST_HEADER), j, 1, fgi.func_header);
		ListView_SetItemText(GetDlgItem(hDlg, IDC_LIST_HEADER), j, 2, fgi.comment);
	}
	return TRUE;
}

/*
 * select_headers_proc - ヘッダ選択ウィンドウプロシージャ
 */
static BOOL CALLBACK select_headers_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HWND pWnd;
	HANDLE lib;
	FARPROC func_get_format_header;
	FORMAT_INFO *fi;
	TCHAR buf[BUF_SIZE];
	TCHAR err_str[BUF_SIZE];
	static TCHAR lib_path[BUF_SIZE];
	int i;

	switch (uMsg) {
	case WM_INITDIALOG:
		SetWindowText(hDlg, message_get_res(IDS_FORMAT_SELECT_TITLE));
		SetWindowText(GetDlgItem(hDlg, IDC_STATIC_MSG), message_get_res(IDS_FORMAT_SELECT_MSG));

		// DLLロード
		if ((lib = LoadLibrary((TCHAR *)lParam)) == NULL) {
			message_get_error(GetLastError(), err_str);
			if (*err_str != TEXT('\0')) {
				MessageBox(hDlg, err_str, (TCHAR *)lParam, MB_ICONERROR);
			}
			EndDialog(hDlg, FALSE);
			break;
		}
		// 関数アドレス取得
		if ((func_get_format_header = GetProcAddress(lib, "get_format_header")) == NULL) {
			message_get_error(GetLastError(), err_str);
			if (*err_str != TEXT('\0')) {
				MessageBox(hDlg, err_str, (TCHAR *)lParam, MB_ICONERROR);
			}
			FreeLibrary(lib);
			EndDialog(hDlg, FALSE);
			break;
		}

		// ヘッダ一覧をリストビューに設定
		if (dll_to_list(hDlg, func_get_format_header) == FALSE) {
			FreeLibrary(lib);
			EndDialog(hDlg, FALSE);
			break;
		}
		// リストビューのスタイルの設定
		SetWindowLong(GetDlgItem(hDlg, IDC_LIST_HEADER), GWL_STYLE,
			GetWindowLong(GetDlgItem(hDlg, IDC_LIST_HEADER), GWL_STYLE) & ~LVS_SINGLESEL);
		ListView_SetItemState(GetDlgItem(hDlg, IDC_LIST_HEADER), -1, LVIS_SELECTED, LVIS_SELECTED);

		lstrcpy(lib_path, (TCHAR *)lParam);
		FreeLibrary(lib);

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
				if ((fi = mem_calloc(sizeof(FORMAT_INFO))) != NULL) {
					// 設定取得
					ListView_GetItemText(GetDlgItem(hDlg, IDC_LIST_HEADER), i, 0, buf, BUF_SIZE - 1);
					fi->format_name = alloc_copy(buf);
					fi->lib_file_path = alloc_copy(lib_path);
					ListView_GetItemText(GetDlgItem(hDlg, IDC_LIST_HEADER), i, 1, buf, BUF_SIZE - 1);
					fi->func_header = alloc_copy(buf);

					// 新規追加
					listview_set_format(GetDlgItem(pWnd, IDC_LIST_FORMAT), fi, FALSE);
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
 * select_header_proc - ヘッダ選択ウィンドウプロシージャ
 */
static BOOL CALLBACK select_header_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HANDLE lib;
	FARPROC func_get_format_header;
	TCHAR buf[BUF_SIZE];
	TCHAR err_str[BUF_SIZE];
	int i;

	switch (uMsg) {
	case WM_INITDIALOG:
		SetWindowText(hDlg, message_get_res(IDS_FORMAT_HEAD_SELECT_TITLE));
		SetWindowText(GetDlgItem(hDlg, IDC_STATIC_MSG), message_get_res(IDS_FORMAT_HEAD_SELECT_MSG));

		if (lParam == 0 || *(TCHAR *)lParam == TEXT('\0')) {
			lib = NULL;
			func_get_format_header = get_format_header;
		} else {
			// DLLロード
			if ((lib = LoadLibrary((TCHAR *)lParam)) == NULL) {
				message_get_error(GetLastError(), err_str);
				if (*err_str != TEXT('\0')) {
					MessageBox(hDlg, err_str, (TCHAR *)lParam, MB_ICONERROR);
				}
				EndDialog(hDlg, FALSE);
				break;
			}
			// 関数アドレス取得
			if ((func_get_format_header = GetProcAddress(lib, "get_format_header")) == NULL) {
				message_get_error(GetLastError(), err_str);
				if (*err_str != TEXT('\0')) {
					MessageBox(hDlg, err_str, (TCHAR *)lParam, MB_ICONERROR);
				}
				FreeLibrary(lib);
				EndDialog(hDlg, FALSE);
				break;
			}
		}

		// ヘッダ一覧をリストビューに設定
		if (dll_to_list(hDlg, func_get_format_header) == FALSE) {
			if (lib != NULL) {
				FreeLibrary(lib);
			}
			EndDialog(hDlg, FALSE);
			break;
		}

		if (lib != NULL) {
			FreeLibrary(lib);
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
			ListView_GetItemText(GetDlgItem(hDlg, IDC_LIST_HEADER), i, 1, buf, BUF_SIZE - 1);
			SendDlgItemMessage(GetParent(hDlg), IDC_EDIT_HEADER, WM_SETTEXT, 0, (LPARAM)buf);

			if (SendDlgItemMessage(GetParent(hDlg), IDC_EDIT_FORMAT_NAME, WM_GETTEXTLENGTH, 0, 0) == 0) {
				ListView_GetItemText(GetDlgItem(hDlg, IDC_LIST_HEADER), i, 0, buf, BUF_SIZE - 1);
				SendDlgItemMessage(GetParent(hDlg), IDC_EDIT_FORMAT_NAME, WM_SETTEXT, 0, (LPARAM)buf);
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
 * set_format_item_proc - 形式の項目を設定
 */
static BOOL CALLBACK set_format_item_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HMENU hMenu;
	RECT button_rect;
	FORMAT_INFO *fi;
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
		if (lParam == 0) {
			// 新規追加
			if (*cmd_format != TEXT('\0')) {
				SendDlgItemMessage(hDlg, IDC_EDIT_FORMAT_NAME, WM_SETTEXT, 0, (LPARAM)cmd_format);
				*cmd_format = TEXT('\0');
			}
			SetWindowLong(hDlg, GWL_USERDATA, 0);
			break;
		}
		fi = (FORMAT_INFO *)lParam;

		SendDlgItemMessage(hDlg, IDC_EDIT_FORMAT_NAME, WM_SETTEXT, 0, (LPARAM)fi->format_name);
		SendDlgItemMessage(hDlg, IDC_EDIT_LIB_PATH, WM_SETTEXT, 0, (LPARAM)fi->lib_file_path);
		SendDlgItemMessage(hDlg, IDC_EDIT_HEADER, WM_SETTEXT, 0, (LPARAM)fi->func_header);

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

		case IDC_BUTTON_FILE_SELECT:
			// ファイル選択
			SetFocus(GetDlgItem(hDlg, IDC_EDIT_LIB_PATH));
			if (file_select(hDlg, TEXT("*.dll\0*.dll\0*.*\0*.*\0\0"), 1, buf) == -1) {
				break;
			}
			SendDlgItemMessage(hDlg, IDC_EDIT_LIB_PATH, WM_SETTEXT, 0, (LPARAM)buf);
			SendDlgItemMessage(hDlg, IDC_EDIT_HEADER, WM_SETTEXT, 0, (LPARAM)TEXT(""));

		case IDC_BUTTON_FUNC_SELECT:
			// ヘッダ選択
			*buf = TEXT('\0');
			SendDlgItemMessage(hDlg, IDC_EDIT_LIB_PATH, WM_GETTEXT, BUF_SIZE - 1, (LPARAM)buf);
			if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_SELECT_FUNC), hDlg, select_header_proc, (LPARAM)buf) == TRUE) {
				break;
			}
			break;

		case IDOK:
			*buf = TEXT('\0');
			SendDlgItemMessage(hDlg, IDC_EDIT_FORMAT_NAME, WM_GETTEXT, BUF_SIZE - 1, (LPARAM)buf);
			if (*buf == TEXT('\0')) {
				MessageBox(hDlg, message_get_res(IDS_FORMAT_ERR_NAME), WINDOW_TITLE, MB_ICONEXCLAMATION);
				SetFocus(GetDlgItem(hDlg, IDC_EDIT_FORMAT_NAME));
				break;
			}

			if ((fi = (FORMAT_INFO *)GetWindowLong(hDlg, GWL_USERDATA)) == NULL) {
				fi = mem_calloc(sizeof(FORMAT_INFO));
			}
			if (fi != NULL) {
				// 設定取得
				alloc_get_text(GetDlgItem(hDlg, IDC_EDIT_FORMAT_NAME), &fi->format_name);
				alloc_get_text(GetDlgItem(hDlg, IDC_EDIT_LIB_PATH), &fi->lib_file_path);
				alloc_get_text(GetDlgItem(hDlg, IDC_EDIT_HEADER), &fi->func_header);
			}

			if (GetWindowLong(hDlg, GWL_USERDATA) == 0) {
				// 新規
				HWND pWnd = PropSheet_GetCurrentPageHwnd(GetParent(hDlg));
				listview_set_format(GetDlgItem(pWnd, IDC_LIST_FORMAT), fi, FALSE);
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
 * set_format_property_proc - 内部形式のプロパティを設定
 */
static BOOL CALLBACK set_format_property_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		// スピンコントロールの設定
		SendDlgItemMessage(hDlg, IDC_SPIN_TOOLTIP_SIZE, UDM_SETRANGE, 0, (LPARAM)MAKELONG(UD_MAXVAL, 0));

		SetDlgItemInt(hDlg, IDC_EDIT_TOOLTIP_SIZE, option.fmt_txt_menu_tooltip_size, FALSE);
		break;

	case WM_CLOSE:
		EndDialog(hDlg, FALSE);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			option.fmt_txt_menu_tooltip_size = GetDlgItemInt(hDlg, IDC_EDIT_TOOLTIP_SIZE, NULL, FALSE);
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
static BOOL show_property(const HWND hDlg, const FORMAT_INFO *fi)
{
	HANDLE lib;
	FARPROC func_show_property;
	TCHAR err_str[BUF_SIZE];
	TCHAR buf[BUF_SIZE];
	char cbuf[BUF_SIZE];
	BOOL ret = FALSE;

	if (fi->lib_file_path == NULL || *fi->lib_file_path == TEXT('\0')) {
		if (lstrcmp(fi->func_header, TEXT("text_")) == 0) {
			DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_FORMAT_PROP), hDlg, set_format_property_proc, 0);
			ret = TRUE;
		} else {
			MessageBox(hDlg, message_get_res(IDS_FORMAT_NON_PROP), WINDOW_TITLE, MB_OK | MB_ICONEXCLAMATION);
		}
	} else {
		// DLLロード
		if ((lib = LoadLibrary(fi->lib_file_path)) == NULL) {
			message_get_error(GetLastError(), err_str);
			if (*err_str != TEXT('\0')) {
				MessageBox(hDlg, err_str, fi->lib_file_path, MB_ICONERROR);
			}
			return FALSE;
		}
		// 関数アドレス取得
		wsprintf(buf, TEXT("%sshow_property"), fi->func_header);
		tchar_to_char(buf, cbuf, BUF_SIZE - 1);
		if ((func_show_property = GetProcAddress(lib, cbuf)) != NULL) {
			// プロパティ表示
			ret = func_show_property(hDlg);
		}
		FreeLibrary(lib);
		if (ret == FALSE) {
			MessageBox(hDlg, message_get_res(IDS_FORMAT_NON_PROP), WINDOW_TITLE, MB_OK | MB_ICONEXCLAMATION);
		}
	}
	return ret;
}

/*
 * listview_set_text - ListViewのテキストを設定
 */
static void listview_set_text(const HWND hListView, const int i)
{
	FORMAT_INFO *fi;

	if ((fi = (FORMAT_INFO *)listview_get_lparam(hListView, i)) == NULL) {
		return;
	}
	// 形式名
	ListView_SetItemText(hListView, i, 0, fi->format_name);
	// DLL
	ListView_SetItemText(hListView, i, 1, fi->lib_file_path);
	// ヘッダ
	ListView_SetItemText(hListView, i, 2, fi->func_header);
}

/*
 * listview_set_format - ListViewに形式情報を追加する
 */
static void listview_set_format(const HWND hListView, FORMAT_INFO *fi, const BOOL copy)
{
	LV_ITEM lvi;
	FORMAT_INFO *new_fi;
	int i;

	if (copy == TRUE) {
		if ((new_fi = mem_calloc(sizeof(FORMAT_INFO))) == NULL) {
			return;
		}
		new_fi->format_name = alloc_copy(fi->format_name);
		new_fi->lib_file_path = alloc_copy(fi->lib_file_path);
		new_fi->func_header = alloc_copy(fi->func_header);
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
 * listview_get_format - 形式情報の取得
 */
static FORMAT_INFO *listview_get_format(const HWND hListView, int *cnt)
{
	FORMAT_INFO *fi;
	FORMAT_INFO *new_fi;
	int i;

	if ((*cnt = ListView_GetItemCount(hListView)) == 0) {
		return NULL;
	}

	// 項目の作成
	if ((new_fi = mem_calloc(sizeof(FORMAT_INFO) * *cnt)) == NULL) {
		*cnt = 0;
		return NULL;
	}

	for (i = 0; i < *cnt; i++) {
		if ((fi = (FORMAT_INFO *)listview_get_lparam(hListView, i)) == NULL) {
			continue;
		}
		(new_fi + i)->format_name = alloc_copy(fi->format_name);
		(new_fi + i)->lib_file_path = alloc_copy(fi->lib_file_path);
		(new_fi + i)->func_header = alloc_copy(fi->func_header);
	}
	return new_fi;
}

/*
 * listview_free_format - 形式情報の解放
 */
static void listview_free_format(const HWND hListView)
{
	FORMAT_INFO *fi;
	int i;

	for (i = 0; i < ListView_GetItemCount(hListView); i++) {
		if ((fi = (FORMAT_INFO *)listview_get_lparam(hListView, i)) != NULL) {
			mem_free(&fi->format_name);
			mem_free(&fi->lib_file_path);
			mem_free(&fi->func_header);
			mem_free(&fi);
		}
		listview_set_lparam(hListView, i, 0);
	}
}

/*
 * set_format_proc - 形式設定のプロシージャ
 */
BOOL CALLBACK set_format_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LV_COLUMN lvc;
	FORMAT_INFO *fi;
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
		lvc.cx = Scale(130);
		lvc.pszText = message_get_res(IDS_FORMAT_LIST_NAME);
		lvc.iSubItem = i++;
		ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_FORMAT), lvc.iSubItem, &lvc);

		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = Scale(100);
		lvc.pszText = message_get_res(IDS_FORMAT_LIST_DLL);
		lvc.iSubItem = i++;
		ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_FORMAT), lvc.iSubItem, &lvc);

		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = Scale(100);
		lvc.pszText = message_get_res(IDS_FORMAT_LIST_HEADER);
		lvc.iSubItem = i++;
		ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_FORMAT), lvc.iSubItem, &lvc);

		// リストビューのスタイルの設定
		SetWindowLong(GetDlgItem(hDlg, IDC_LIST_FORMAT), GWL_STYLE,
			GetWindowLong(GetDlgItem(hDlg, IDC_LIST_FORMAT), GWL_STYLE) | LVS_SHOWSELALWAYS);
		SendDlgItemMessage(hDlg, IDC_LIST_FORMAT, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
			LVS_EX_ONECLICKACTIVATE | LVS_EX_FULLROWSELECT |
			SendDlgItemMessage(hDlg, IDC_LIST_FORMAT, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0));

		for (i = 0; i < option.format_cnt; i++) {
			listview_set_format(GetDlgItem(hDlg, IDC_LIST_FORMAT), option.format_info + i, TRUE);
		}
		SendMessage(hDlg, WM_LV_EVENT, LVN_ITEMCHANGED, 0);

		if (*cmd_format != TEXT('\0')) {
			SetTimer(hDlg, ID_ADD_TIMER, 1, NULL);
		}
		break;

	case WM_DESTROY:
		listview_free_format(GetDlgItem(hDlg, IDC_LIST_FORMAT));
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
			if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_SELECT_FUNC), hDlg, select_headers_proc, (LPARAM)buf) == FALSE) {
				break;
			}
		}
		DragFinish((HANDLE)wParam);
		break;

	case WM_NOTIFY:
		if (listview_notify_proc(hDlg, lParam, GetDlgItem(hDlg, IDC_LIST_FORMAT)) == 0) {
			return OptionNotifyProc(hDlg, uMsg, wParam, lParam);
		}
		break;

	case WM_LV_EVENT:
		switch (wParam) {
		case LVN_ITEMCHANGED:
			enable = (ListView_GetSelectedCount(GetDlgItem(hDlg, IDC_LIST_FORMAT)) <= 0) ? FALSE : TRUE;
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_UP), enable);
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_DOWN), enable);
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_EDIT), enable);
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_DELETE), enable);
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_PROP), enable);
			break;
		}
		break;

	case WM_TIMER:
		switch (wParam) {
		case ID_ADD_TIMER:
			KillTimer(hDlg, wParam);
			if ((i = format_get_index(cmd_format, 0)) == -1) {
				// 追加
				SendMessage(hDlg, WM_COMMAND, IDC_BUTTON_ADD, 0);
			} else {
				// 編集
				ListView_SetItemState(GetDlgItem(hDlg, IDC_LIST_FORMAT), i,
					LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
				SendMessage(hDlg, WM_COMMAND, IDC_BUTTON_EDIT, 0);
			}
			break;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_BUTTON_UP:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_FORMAT), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			if (i == 0) {
				break;
			}
			listview_move_item(GetDlgItem(hDlg, IDC_LIST_FORMAT), i, -1);
			SetFocus(GetDlgItem(hDlg, LOWORD(wParam)));
			break;

		case IDC_BUTTON_DOWN:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_FORMAT), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			if (i == ListView_GetItemCount(GetDlgItem(hDlg, IDC_LIST_FORMAT)) - 1) {
				break;
			}
			listview_move_item(GetDlgItem(hDlg, IDC_LIST_FORMAT), i, 1);
			SetFocus(GetDlgItem(hDlg, LOWORD(wParam)));
			break;

		case IDC_BUTTON_ADD:
			DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_FORMAT_SET), hDlg, set_format_item_proc, 0);
			break;

		case IDC_BUTTON_EDIT:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_FORMAT), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_FORMAT_SET), hDlg, set_format_item_proc,
				listview_get_lparam(GetDlgItem(hDlg, IDC_LIST_FORMAT), i));
			listview_set_text(GetDlgItem(hDlg, IDC_LIST_FORMAT), i);
			break;

		case IDC_BUTTON_DELETE:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_FORMAT), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			if (MessageBox(hDlg, message_get_res(IDS_OPTION_DELETE), WINDOW_TITLE, MB_ICONQUESTION | MB_YESNO) == IDNO) {
				break;
			}
			if ((fi = (FORMAT_INFO *)listview_get_lparam(GetDlgItem(hDlg, IDC_LIST_FORMAT), i)) != NULL) {
				mem_free(&fi->format_name);
				ini_free_format_name(fi->fn, fi->fn_cnt);
				mem_free(&fi->lib_file_path);
				mem_free(&fi->func_header);
				mem_free(&fi);
			}
			ListView_DeleteItem(GetDlgItem(hDlg, IDC_LIST_FORMAT), i);
			ListView_SetItemState(GetDlgItem(hDlg, IDC_LIST_FORMAT), i,
				LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
			break;

		case IDC_BUTTON_PROP:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_FORMAT), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			show_property(hDlg, (FORMAT_INFO *)listview_get_lparam(GetDlgItem(hDlg, IDC_LIST_FORMAT), i));
			break;

		case IDOK:
			for (i = 0; i < option.format_cnt; i++) {
				mem_free(&((option.format_info + i)->format_name));
				ini_free_format_name((option.format_info + i)->fn, (option.format_info + i)->fn_cnt);
				mem_free(&((option.format_info + i)->lib_file_path));
				mem_free(&((option.format_info + i)->func_header));
			}
			mem_free(&option.format_info);

			option.format_info = listview_get_format(GetDlgItem(hDlg, IDC_LIST_FORMAT), &option.format_cnt);
			listview_free_format(GetDlgItem(hDlg, IDC_LIST_FORMAT));
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
