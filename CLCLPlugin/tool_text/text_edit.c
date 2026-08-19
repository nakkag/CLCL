/*
 * tool_text
 *
 * to_lower.c
 *
 * Copyright (C) 1996-2003 by Nakashima Tomoaki. All rights reserved.
 *		http://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE

#include "..\CLCLPlugin.h"

#include "resource.h"

/* Define */
#define ID_SELECT_TIMER					1

/* Global Variables */
extern HINSTANCE hInst;

/* Local Function Prototypes */

/*
 * set_edit_proc - テキスト編集プロシージャ
 */
BOOL CALLBACK set_edit_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT window_rect;
	DATA_INFO *di;
	BYTE *mem;
	int len;

	switch (uMsg) {
	case WM_INITDIALOG:
		SendDlgItemMessage(hDlg, IDC_EDIT_TEXT, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(FALSE, 0));

		di = (DATA_INFO *)lParam;
		// メモリのロック
		if ((mem = GlobalLock(di->data)) != NULL) {
			SendDlgItemMessage(hDlg, IDC_EDIT_TEXT, WM_SETTEXT, 0, (LPARAM)mem);
			GlobalUnlock(di->data);
		}
		SetWindowLong(hDlg, GWL_USERDATA, lParam);
		SetTimer(hDlg, ID_SELECT_TIMER, 1, NULL);
		break;

	case WM_SIZE:
		GetWindowRect(hDlg, (LPRECT)&window_rect);
		SetWindowPos(GetDlgItem(hDlg, IDC_EDIT_TEXT), 0, 0, 0,
			(window_rect.right - window_rect.left) - (GetSystemMetrics(SM_CXFRAME) * 2),
			(window_rect.bottom - window_rect.top) - GetSystemMetrics(SM_CYSIZE) - (GetSystemMetrics(SM_CXFRAME) * 2) - 30,
			SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDOK), 0,
			(window_rect.right - window_rect.left) - (GetSystemMetrics(SM_CXFRAME) * 2) - 200,
			(window_rect.bottom - window_rect.top) - GetSystemMetrics(SM_CYSIZE) - (GetSystemMetrics(SM_CXFRAME) * 2) - 25,
			0, 0, SWP_NOZORDER | SWP_NOSIZE);
		SetWindowPos(GetDlgItem(hDlg, IDCANCEL), 0,
			(window_rect.right - window_rect.left) - (GetSystemMetrics(SM_CXFRAME) * 2) - 100,
			(window_rect.bottom - window_rect.top) - GetSystemMetrics(SM_CYSIZE) - (GetSystemMetrics(SM_CXFRAME) * 2) - 25,
			0, 0, SWP_NOZORDER | SWP_NOSIZE);
		InvalidateRect(GetDlgItem(hDlg, IDOK), NULL, FALSE);
		UpdateWindow(GetDlgItem(hDlg, IDOK));
		InvalidateRect(GetDlgItem(hDlg, IDCANCEL), NULL, FALSE);
		UpdateWindow(GetDlgItem(hDlg, IDCANCEL));
		break;

	case WM_CLOSE:
		EndDialog(hDlg, FALSE);
		break;

	case WM_TIMER:
		// タイマー
		switch (wParam) {
		case ID_SELECT_TIMER:
			KillTimer(hDlg, wParam);
			SendDlgItemMessage(hDlg, IDC_EDIT_TEXT, EM_SETSEL, 0, 0);
			break;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			di = (DATA_INFO *)GetWindowLong(hDlg, GWL_USERDATA);
			if (di == NULL) {
				EndDialog(hDlg, FALSE);
				break;
			}
			GlobalFree(di->data);

			len = SendDlgItemMessage(hDlg, IDC_EDIT_TEXT, WM_GETTEXTLENGTH, 0, 0) + 1;
			di->data = GlobalAlloc(GPTR, sizeof(TCHAR) * len);
			if (di->data != NULL) {
				if ((mem = GlobalLock(di->data)) != NULL) {
					*mem = TEXT('\0');
					SendDlgItemMessage(hDlg, IDC_EDIT_TEXT, WM_GETTEXT, len, (LPARAM)mem);
					GlobalUnlock(di->data);
					di->size = sizeof(TCHAR) * len;
				}
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
 * text_edit - テキストを小文字に変換
 *
 *	引数 / argument:
 *		hWnd - 呼び出し元ウィンドウ / the calling window
 *		tei - ツール実行情報 / tool execution information
 *		tdi - ツール用アイテム情報 / item information for tools
 *
 *	戻り値 / Return value:
 *		TOOL_
 */
__declspec(dllexport) int CALLBACK text_edit(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	DATA_INFO *di;

	if (tdi == NULL) {
		return TOOL_SUCCEED;
	}

	// 形式選択
#ifdef UNICODE
	di = (DATA_INFO *)SendMessage(hWnd, WM_ITEM_GET_FORMAT_TO_ITEM, (WPARAM)TEXT("UNICODE TEXT"), (LPARAM)tdi->di);
#else
	di = (DATA_INFO *)SendMessage(hWnd, WM_ITEM_GET_FORMAT_TO_ITEM, (WPARAM)TEXT("TEXT"), (LPARAM)tdi->di);
#endif
	if (di == NULL || di->data == NULL) {
		return TOOL_SUCCEED;
	}

	if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_TEXT_EDIT), GetForegroundWindow(), set_edit_proc, (LPARAM)di) == FALSE) {
		return TOOL_CANCEL;
	}
	return TOOL_DATA_MODIFIED;
}

/*
 * text_edit_property - プロパティ表示
 */
__declspec(dllexport) BOOL CALLBACK text_edit_property(const HWND hWnd, TOOL_EXEC_INFO *tei)
{
	return FALSE;
}
/* End of source */
