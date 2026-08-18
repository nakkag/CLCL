/*
 * tool_utl
 *
 * multi_save.c
 *
 * Copyright (C) 1996-2003 by Nakashima Tomoaki. All rights reserved.
 *		http://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE
#include <shlobj.h>

#include "..\CLCLPlugin.h"

#include "resource.h"

/* Define */

/* Global Variables */
static TCHAR save_path[MAX_PATH];

extern HINSTANCE hInst;
extern TCHAR ini_path[];

extern TCHAR multi_save_format[];
extern TCHAR multi_save_file_name[];

/* Local Function Prototypes */

/*
 * folder_select - フォルダを選択
 */
static BOOL folder_select(const HWND hWnd, TCHAR *ret)
{
	BROWSEINFO bi;
	LPITEMIDLIST itemid_list;
	TCHAR buf[BUF_SIZE];
	TCHAR title[BUF_SIZE];
	LoadString(hInst, IDS_SEL_OUTPUT_FOLDER, title, BUF_SIZE - 1);

	bi.hwndOwner = hWnd;
	bi.pidlRoot = (const struct _ITEMIDLIST *)CSIDL_DESKTOP;
	*buf = TEXT('\0');
	bi.pszDisplayName = buf;
	bi.lpszTitle = title;
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	bi.lpfn = NULL;
	bi.lParam = 0;
	bi.iImage = 0;

	// フォルダ選択画面を表示する
	itemid_list = SHBrowseForFolder(&bi);
	if (itemid_list == NULL) {
		return FALSE;
	}
	// 選択されていたフォルダのフルパスを取得する
	SHGetPathFromIDList(itemid_list, ret);
	return TRUE;
}

/*
 * format_to_combo - コンボボックスに形式名を設定
 */
static void format_to_combo(const HWND hCombo, TOOL_DATA_INFO *tdi)
{
	for (; tdi != NULL; tdi = tdi->next) {
		if (tdi->di->type == TYPE_ITEM) {
			format_to_combo(hCombo, tdi->child);
		}
		if (tdi->di->type != TYPE_DATA) {
			continue;
		}
		if (SendMessage(hCombo, CB_FINDSTRING, -1, (LPARAM)tdi->di->format_name) >= 0) {
			continue;
		}
		SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)tdi->di->format_name);
	}
}

/*
 * set_multi_save_proc - 保存情報設定
 */
BOOL CALLBACK set_multi_save_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	TCHAR path[MAX_PATH];

	switch (uMsg) {
	case WM_INITDIALOG:
		format_to_combo(GetDlgItem(hDlg, IDC_COMBO_FORMAT), (TOOL_DATA_INFO *)lParam);

		SendDlgItemMessage(hDlg, IDC_COMBO_FORMAT, WM_SETTEXT, 0, (LPARAM)multi_save_format);
		SendDlgItemMessage(hDlg, IDC_EDIT_PATH, WM_SETTEXT, 0, (LPARAM)save_path);
		SendDlgItemMessage(hDlg, IDC_EDIT_FILE_NAME, WM_SETTEXT, 0, (LPARAM)multi_save_file_name);
		break;

	case WM_CLOSE:
		EndDialog(hDlg, FALSE);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_BUTTON_SELECT_PATH:
			if (folder_select(hDlg, path) == FALSE) {
				break;
			}
			SendDlgItemMessage(hDlg, IDC_EDIT_PATH, WM_SETTEXT, 0, (LPARAM)path);
			break;

		case IDOK:
			SendDlgItemMessage(hDlg, IDC_COMBO_FORMAT, WM_GETTEXT, BUF_SIZE - 1, (LPARAM)multi_save_format);
			SendDlgItemMessage(hDlg, IDC_EDIT_PATH, WM_GETTEXT, BUF_SIZE - 1, (LPARAM)save_path);
			SendDlgItemMessage(hDlg, IDC_EDIT_FILE_NAME, WM_GETTEXT, BUF_SIZE - 1, (LPARAM)multi_save_file_name);

			WritePrivateProfileString(TEXT("multi_save"), TEXT("format"), multi_save_format, ini_path);
			WritePrivateProfileString(TEXT("multi_save"), TEXT("file_name"), multi_save_file_name, ini_path);
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
 * multi_save - 複数アイテムをファイルに保存
 */
__declspec(dllexport) int CALLBACK multi_save(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	TOOL_DATA_INFO *wk_tdi;
	TCHAR file_name[MAX_PATH];
	TCHAR path[MAX_PATH];
	int i;

	if (tdi == NULL) {
		return TOOL_SUCCEED;
	}

	// 出力先の選択
	if (folder_select(hWnd, save_path) == FALSE) {
		return TOOL_CANCEL;
	}
	// 保存情報設定
	if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_MULTI_SAVE), hWnd, set_multi_save_proc, (LPARAM)tdi) == FALSE) {
		return TOOL_CANCEL;
	}

	for (i = 0; tdi != NULL; tdi = tdi->next, i++) {
		if (tdi->di->type == TYPE_FOLDER) {
			continue;
		} else if (tdi->di->type == TYPE_ITEM) {
			for (wk_tdi = tdi->child; wk_tdi != NULL && lstrcmpi(multi_save_format, wk_tdi->di->format_name) != 0; wk_tdi = wk_tdi->next)
				;
			if (wk_tdi == NULL) {
				continue;
			}
		} else if (tdi->di->type == TYPE_DATA) {
			if (lstrcmpi(multi_save_format, tdi->di->format_name) != 0) {
				continue;
			}
			wk_tdi = tdi;
		} else {
			continue;
		}
		wsprintf(file_name, multi_save_file_name, i);
		wsprintf(path, TEXT("%s\\%s"), save_path, file_name);
		if (SendMessage(hWnd, WM_ITEM_TO_FILE, (WPARAM)path, (LPARAM)wk_tdi->di) == FALSE) {
			TCHAR failed[BUF_SIZE];
			LoadString(hInst, IDS_SAVE_FAILED, failed, BUF_SIZE - 1);
			TCHAR caption[BUF_SIZE];
			LoadString(hInst, IDS_CAPTION_OUTPUT, caption, BUF_SIZE - 1);
			MessageBox(hWnd, failed, caption, MB_ICONERROR);
			return TOOL_CANCEL;
		}
	}
	return TOOL_SUCCEED;
}

/*
 * multi_save_property - プロパティ表示
 */
__declspec(dllexport) BOOL CALLBACK multi_save_property(const HWND hWnd, TOOL_EXEC_INFO *tei)
{
	return FALSE;
}
/* End of source */
