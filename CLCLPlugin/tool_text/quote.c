/*
 * tool_text
 *
 * quote.c
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

/* Global Variables */
extern HINSTANCE hInst;
extern TCHAR ini_path[];

extern TCHAR quote_char[];

/* Local Function Prototypes */

/*
 * set_quote_proc - 引用符を設定
 */
BOOL CALLBACK set_quote_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		SendDlgItemMessage(hDlg, IDC_EDIT_QUOTE_CHAR, WM_SETTEXT, 0, (LPARAM)quote_char);
		break;

	case WM_CLOSE:
		EndDialog(hDlg, FALSE);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			SendDlgItemMessage(hDlg, IDC_EDIT_QUOTE_CHAR, WM_GETTEXT, BUF_SIZE - 1, (LPARAM)quote_char);

			WritePrivateProfileString(TEXT("quote"), TEXT("char"), quote_char, ini_path);
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
 * item_quote - テキストに引用符を付加
 */
static int item_quote(DATA_INFO *di, const TCHAR *q_char)
{
	HANDLE ret;
	BYTE *from_mem, *to_mem;
	TCHAR *p, *r;
	int size;

	// コピー元ロック
	if ((from_mem = GlobalLock(di->data)) == NULL) {
		return TOOL_ERROR;
	}

	// 引用符の付加したサイズを取得
	size = lstrlen(q_char);
	for (p = (TCHAR *)from_mem; *p != TEXT('\0'); p++) {
#ifndef UNICODE
		if (IsDBCSLeadByte((BYTE)*p) == TRUE) {
			p++;
			size += 2;
			continue;
		}
#endif
		size++;
		if (*p == TEXT('\n') && *(p + 1) != TEXT('\0')) {
			size += lstrlen(q_char);
		}
	}
	size++;

	// コピー先確保
	if ((ret = GlobalAlloc(GHND, sizeof(TCHAR) * size)) == NULL) {
		GlobalUnlock(di->data);
		return TOOL_ERROR;
	}
	// コピー先ロック
	if ((to_mem = GlobalLock(ret)) == NULL) {
		GlobalFree(ret);
		GlobalUnlock(di->data);
		return TOOL_ERROR;
	}

	// 引用符の付加
	lstrcpy((TCHAR *)to_mem, q_char);
	r = (TCHAR *)to_mem + lstrlen(q_char);
	for (p = (TCHAR *)from_mem; *p != TEXT('\0'); p++) {
#ifndef UNICODE
		if (IsDBCSLeadByte((BYTE)*p) == TRUE) {
			*(r++) = *(p++);
			*(r++) = *p;
			continue;
		}
#endif
		*(r++) = *p;
		if (*p == TEXT('\n') && *(p + 1) != TEXT('\0')) {
			lstrcpy(r, q_char);
			r += lstrlen(q_char);
		}
	}
	*r = TEXT('\0');

	GlobalUnlock(ret);
	GlobalUnlock(di->data);

	GlobalFree(di->data);
	di->data = ret;
	di->size = sizeof(TCHAR) * size;
	return TOOL_DATA_MODIFIED;
}

/*
 * quote - テキストに引用符を付加
 */
__declspec(dllexport) int CALLBACK quote(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	DATA_INFO *di;
	TCHAR *q_char = tei->cmd_line;
	int ret = TOOL_SUCCEED;

	if ((tei->cmd_line == NULL || *tei->cmd_line == TEXT('\0')) &&
		((tei->call_type & CALLTYPE_MENU) || (tei->call_type & CALLTYPE_VIEWER))) {
		if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_QUOTE), GetForegroundWindow(), set_quote_proc, 0) == FALSE) {
			return TOOL_CANCEL;
		}
	}
	if (q_char == NULL || *q_char == TEXT('\0')) {
		q_char = quote_char;
	}
	for (; tdi != NULL; tdi = tdi->next) {
		if (SendMessage(hWnd, WM_ITEM_CHECK, 0, (LPARAM)tdi->di) == -1) {
			continue;
		}
#ifdef UNICODE
		di = (DATA_INFO *)SendMessage(hWnd, WM_ITEM_GET_FORMAT_TO_ITEM, (WPARAM)TEXT("UNICODE TEXT"), (LPARAM)tdi->di);
#else
		di = (DATA_INFO *)SendMessage(hWnd, WM_ITEM_GET_FORMAT_TO_ITEM, (WPARAM)TEXT("TEXT"), (LPARAM)tdi->di);
#endif
		if (di != NULL && di->data != NULL) {
			ret |= item_quote(di, q_char);
		}
	}
	return ret;
}

/*
 * quote_property - プロパティ表示
 */
__declspec(dllexport) BOOL CALLBACK quote_property(const HWND hWnd, TOOL_EXEC_INFO *tei)
{
	DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_QUOTE), hWnd, set_quote_proc, 0);
	return TRUE;
}
/* End of source */
