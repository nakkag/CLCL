/*
 * CLCL
 *
 * SelectFormat.c
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE
#include <commdlg.h>

#include "General.h"
#include "Ini.h"
#include "Message.h"
#include "Format.h"

#include "resource.h"

/* Define */

/* Global Variables */
static TCHAR str_format[BUF_SIZE];
static TCHAR str_file[MAX_PATH];

// オプション
extern OPTION_INFO option;

/* Local Function Prototypes */
static BOOL CALLBACK select_format_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/*
 * select_format_proc - フォーマット選択ウィンドウプロシージャ
 */
static BOOL CALLBACK select_format_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	OPENFILENAME of;
	TCHAR buf[BUF_SIZE];
	TCHAR tmp[BUF_SIZE];
	int i, j;

	switch (uMsg) {
	case WM_INITDIALOG:
		// 現在登録されている形式をコンボボックスに設定
		for (i = 0; i < option.format_cnt; i++) {
			if ((option.format_info + i)->fn == NULL) {
				continue;
			}
			for (j = 0; j < (option.format_info + i)->fn_cnt; j++) {
				SendMessage(GetDlgItem(hDlg, IDC_COMBO_FORMAT), CB_ADDSTRING, 0,
					(LPARAM)((option.format_info + i)->fn + j)->format_name);
			}
		}
		SendMessage(GetDlgItem(hDlg, IDC_COMBO_FORMAT), WM_SETTEXT, 0, (LPARAM)str_format);

		EnableWindow(GetDlgItem(hDlg, IDC_STATIC_FILE), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_EDIT_FILE), FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_FILE_SELECT), FALSE);
		break;

	case WM_CLOSE:
		EndDialog(hDlg, FALSE);
		break;

	case WM_DROPFILES:
		DragQueryFile((HANDLE)wParam, 0, buf, BUF_SIZE - 1);
		SendMessage(GetDlgItem(hDlg, IDC_EDIT_FILE), WM_SETTEXT, 0, (LPARAM)buf);
		DragFinish((HANDLE)wParam);

		SendDlgItemMessage(hDlg, IDC_CHECK_FILE_READ, BM_SETCHECK, 1, 0);
		SendMessage(hDlg, WM_COMMAND, IDC_CHECK_FILE_READ, 0);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CHECK_FILE_READ:
			EnableWindow(GetDlgItem(hDlg, IDC_STATIC_FILE),
				SendDlgItemMessage(hDlg, IDC_CHECK_FILE_READ, BM_GETCHECK, 0, 0));
			EnableWindow(GetDlgItem(hDlg, IDC_EDIT_FILE),
				SendDlgItemMessage(hDlg, IDC_CHECK_FILE_READ, BM_GETCHECK, 0, 0));
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_FILE_SELECT),
				SendDlgItemMessage(hDlg, IDC_CHECK_FILE_READ, BM_GETCHECK, 0, 0));
			break;

		case IDC_BUTTON_FILE_SELECT:
			SendMessage(GetDlgItem(hDlg, IDC_COMBO_FORMAT), WM_GETTEXT, BUF_SIZE - 1, (LPARAM)buf);

			// ファイルの選択
			ZeroMemory(&of, sizeof(OPENFILENAME));
			of.lStructSize = sizeof(OPENFILENAME);
			of.hInstance = NULL;
			of.hwndOwner = hDlg;
			of.lpstrFilter = TEXT("*.*\0*.*\0\0");
			of.nFilterIndex = 1;
			*tmp = TEXT('\0');
			of.lpstrFile = tmp;
			of.nMaxFile = MAX_PATH - 1;
			of.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
			if (format_get_file_info(buf, NULL, &of, TRUE) >= 0 && GetOpenFileName(&of) == FALSE) {
				break;
			}
			SendMessage(GetDlgItem(hDlg, IDC_EDIT_FILE), WM_SETTEXT, 0, (LPARAM)tmp);
			break;

		case IDCANCEL:
			SendMessage(hDlg, WM_CLOSE, 0, 0);
			break;

		case IDOK:
			SendMessage(GetDlgItem(hDlg, IDC_COMBO_FORMAT), WM_GETTEXT, BUF_SIZE - 1, (LPARAM)str_format);
			*str_file = TEXT('\0');
			if (SendDlgItemMessage(hDlg, IDC_CHECK_FILE_READ, BM_GETCHECK, 0, 0) == 1) {
				SendMessage(GetDlgItem(hDlg, IDC_EDIT_FILE), WM_GETTEXT, MAX_PATH - 1, (LPARAM)str_file);
			}
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
 * select_format - フォーマットの選択
 */
BOOL select_format(const HINSTANCE hInst, const HWND hWnd, TCHAR *ret, TCHAR *file_name)
{
	*ret = TEXT('\0');
	*file_name = TEXT('\0');
	if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_SELECT_FORMAT), hWnd, select_format_proc, 0) == TRUE) {
		lstrcpy(ret, str_format);
		lstrcpy(file_name, str_file);
		return TRUE;
	}
	return FALSE;
}
/* End of source */
