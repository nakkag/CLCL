/*
 * CLCL
 *
 * SelectIcon.c
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef	_INC_OLE
#include <commctrl.h>

#include "resource.h"

/* Define */
#define BUF_SIZE						256

#define WM_LV_EVENT						(WM_APP + 100)

#define FILEFILTER_ICON					TEXT("Icon files\0*.ico;*.exe;*.dll;*.icl\0") \
											TEXT("*.exe\0*.exe\0*.dll\0*.dll;*.icl\0") \
											TEXT("*.ico\0*.ico\0All files\0*.*\0\0")

/* Global Variables */
typedef struct _ICON_INFO {
	TCHAR *path;
	int index;
} ICON_INFO;

/* Local Function Prototypes */
static LPARAM listView_get_lparam(const HWND hListView, const int i);
static LRESULT listview_notify_proc(const HWND hWnd, const LPARAM lParam, const HWND hListView);
static int file_select(const HWND hDlg, const TCHAR *oFilter, const int Index, TCHAR *ret);
static int set_list_icon(const HWND hListView, const TCHAR *path, const int index);
static BOOL CALLBACK select_icon_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/*
 * listView_get_lparam - アイテムのLPARAMを取得
 */
static LPARAM listView_get_lparam(const HWND hListView, const int i)
{
	LV_ITEM lvi;

	ZeroMemory(&lvi, sizeof(LVITEM));
	lvi.mask = LVIF_PARAM;
	lvi.iItem = i;
	ListView_GetItem(hListView, &lvi);
	return lvi.lParam;
}

/*
 * listview_notify_proc - リストビューメッセージ
 */
static LRESULT listview_notify_proc(const HWND hWnd, const LPARAM lParam, const HWND hListView)
{
	NMHDR *CForm = (NMHDR *)lParam;
	LV_KEYDOWN *LKey = (LV_KEYDOWN *)lParam;
	LV_DISPINFO *plv = (LV_DISPINFO *)lParam;

	if (CForm->hwndFrom != hListView) {
		return 0;
	}

	switch (plv->hdr.code) {
	case LVN_ITEMCHANGED:		// アイテムの選択状態の変更
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
 * file_select - ファイル選択ダイアログの表示
 */
static int file_select(const HWND hDlg, const TCHAR *oFilter, const int Index, TCHAR *ret)
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
 * set_list_icon - リストビューにアイコン一覧を設定
 */
static int set_list_icon(const HWND hListView, const TCHAR *path, const int index)
{
	LV_ITEM lvi;
	HIMAGELIST icon_list;
	HICON hIcon = NULL;
	HICON hsIcon = NULL;
	TCHAR buf[BUF_SIZE];
	int icon_cnt;
	int ret;
	int i, j;

	SendMessage(hListView, WM_SETREDRAW, (WPARAM)FALSE, 0);
	// リストビューのクリア
	ListView_DeleteAllItems(hListView);

	if (path == NULL) {
		SendMessage(hListView, WM_SETREDRAW, (WPARAM)TRUE, 0);
		UpdateWindow(hListView);
		return 0;
	}
	// アイコン数取得
	if ((icon_cnt = ExtractIconEx(path, -1, NULL, NULL, 1)) <= 0) {
		SendMessage(hListView, WM_SETREDRAW, (WPARAM)TRUE, 0);
		UpdateWindow(hListView);
		return 0;
	}

	// イメージリストの作成、設定
	if ((icon_list = ListView_GetImageList(hListView, LVSIL_NORMAL)) == NULL) {
		icon_list = ImageList_Create(32, 32, ILC_COLOR16 | ILC_MASK, 0, 0);
		ImageList_SetBkColor(icon_list, GetSysColor(COLOR_WINDOW));
		ListView_SetImageList(hListView, icon_list, LVSIL_NORMAL);
	} else {
		ImageList_Remove(icon_list, -1);
	}

	for (i = 0; i < icon_cnt; i++) {
		// アイコンの抽出
		ExtractIconEx(path, i, &hIcon, &hsIcon, 1);
		if (hIcon != NULL) {
			// イメージリストに追加
			ret = ImageList_AddIcon(icon_list, hIcon);
			DestroyIcon(hIcon);

			// リストビューにアイテムを追加
			wsprintf(buf, TEXT("%d"), i);
			lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
			lvi.iItem = ListView_GetItemCount(hListView);
			lvi.iSubItem = 0;
			lvi.pszText = buf;
			lvi.cchTextMax = BUF_SIZE;
			lvi.iImage = ret;
			lvi.lParam = ret;
			j = ListView_InsertItem(hListView, &lvi);
			if (index == ret) {
				ListView_SetItemState(hListView, j, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
				ListView_EnsureVisible(hListView, j, TRUE);
			}
		}
		if (hsIcon != NULL) {
			DestroyIcon(hsIcon);
		}
	}
	SendMessage(hListView, WM_SETREDRAW, (WPARAM)TRUE, 0);
	UpdateWindow(hListView);
	return icon_cnt;
}

/*
 * select_icon_proc - アイコン選択ウィンドウプロシージャ
 */
static BOOL CALLBACK select_icon_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static ICON_INFO *icon_info;
	TCHAR buf[BUF_SIZE];
	int i;

	switch (uMsg) {
	case WM_INITDIALOG:
		icon_info = (ICON_INFO *)lParam;
		SendDlgItemMessage(hDlg, IDC_EDIT_FILE, WM_SETTEXT, 0, (LPARAM)icon_info->path);
		set_list_icon(GetDlgItem(hDlg, IDC_LIST_ICON), icon_info->path, icon_info->index);
		SendMessage(hDlg, WM_LV_EVENT, LVN_ITEMCHANGED, 0);
		SendDlgItemMessage(hDlg, IDC_EDIT_FILE, EM_SETMODIFY, FALSE, 0);
		break;

	case WM_CLOSE:
		ImageList_Destroy((void *)ListView_GetImageList(GetDlgItem(hDlg, IDC_LIST_ICON), LVSIL_NORMAL));
		EndDialog(hDlg, FALSE);
		break;

	case WM_NOTIFY:
		listview_notify_proc(hDlg, lParam, GetDlgItem(hDlg, IDC_LIST_ICON));
		break;

	case WM_LV_EVENT:
		if (wParam == LVN_ITEMCHANGED) {
			EnableWindow(GetDlgItem(hDlg, IDOK),
				(ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_ICON), -1, LVNI_SELECTED) == -1) ? FALSE : TRUE);
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_EDIT_FILE:
			if (HIWORD(wParam) == EN_KILLFOCUS && SendDlgItemMessage(hDlg, IDC_EDIT_FILE, EM_GETMODIFY, 0, 0) == TRUE) {
				// 変更時アイコン一覧を取得しなおす
				SendDlgItemMessage(hDlg, IDC_EDIT_FILE, WM_GETTEXT, BUF_SIZE - 1, (LPARAM)buf);
				set_list_icon(GetDlgItem(hDlg, IDC_LIST_ICON), buf, -1);
				SendMessage(hDlg, WM_LV_EVENT, LVN_ITEMCHANGED, 0);
				SendDlgItemMessage(hDlg, IDC_EDIT_FILE, EM_SETMODIFY, FALSE, 0);
			}
			break;

		case IDC_BUTTON_BROWS:
			// アイコンファイル選択
			if (file_select(hDlg, FILEFILTER_ICON, 1, buf) == -1) {
				break;
			}
			SendDlgItemMessage(hDlg, IDC_EDIT_FILE, WM_SETTEXT, 0, (LPARAM)buf);
			set_list_icon(GetDlgItem(hDlg, IDC_LIST_ICON), buf, -1);
			SendMessage(hDlg, WM_LV_EVENT, LVN_ITEMCHANGED, 0);
			break;

		case IDCANCEL:
			SendMessage(hDlg, WM_CLOSE, 0, 0);
			break;

		case IDOK:
		case IDC_BUTTON_EDIT:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_ICON), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			SendDlgItemMessage(hDlg, IDC_EDIT_FILE, WM_GETTEXT, BUF_SIZE - 1, (LPARAM)icon_info->path);
			icon_info->index = listView_get_lparam(GetDlgItem(hDlg, IDC_LIST_ICON), i);

			ImageList_Destroy((void *)ListView_GetImageList(GetDlgItem(hDlg, IDC_LIST_ICON), LVSIL_NORMAL));
			EndDialog(hDlg, TRUE);
			break;
		}
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

/*
 * SelectIcon - アイコン選択ウィンドウの表示
 */
int select_icon(const HINSTANCE hInst, const HWND hWnd, TCHAR *path, const int index)
{
	ICON_INFO icon_info;

	icon_info.path = path;
	icon_info.index = index;

	if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_SELECT_ICON), hWnd, select_icon_proc, (LPARAM)&icon_info) == FALSE) {
		return -1;
	}
	return icon_info.index;
}
/* End of source */
