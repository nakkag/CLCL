/*
 * tool_text
 *
 * delete_crlf.c
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

extern int delete_crlf_delete_space;

/* Local Function Prototypes */

/*
 * item_delete_crlf - 改行の除去
 */
static int item_delete_crlf(DATA_INFO *di)
{
	HANDLE ret;
	BYTE *from_mem, *to_mem;
	TCHAR *p, *r;
	int size;

	// コピー元ロック
	if ((from_mem = GlobalLock(di->data)) == NULL) {
		return TOOL_ERROR;
	}

	// 改行を除去したサイズを取得
	size = 0;
	p = (TCHAR *)from_mem;
	if (delete_crlf_delete_space == 1) {
		for (; *p == TEXT(' ') || *p == TEXT('\t'); p++)
			;
	}
	while (*p != TEXT('\0')) {
#ifndef UNICODE
		if (IsDBCSLeadByte((BYTE)*p) == TRUE) {
			p += 2;
			size += 2;
			continue;
		}
#endif
		if (*p == TEXT('\r') || *p == TEXT('\n')) {
			for (; *p == TEXT('\r') || *p == TEXT('\n'); p++)
				;
			if (delete_crlf_delete_space == 1) {
				for (; *p == TEXT(' ') || *p == TEXT('\t'); p++)
					;
			}
		} else {
			p++;
			size++;
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

	// 改行の除去
	p = (TCHAR *)from_mem;
	r = (TCHAR *)to_mem;
	if (delete_crlf_delete_space == 1) {
		for (; *p == TEXT(' ') || *p == TEXT('\t'); p++)
			;
	}
	while (*p != TEXT('\0')) {
#ifndef UNICODE
		if (IsDBCSLeadByte((BYTE)*p) == TRUE) {
			*(r++) = *(p++);
			*(r++) = *(p++);
			continue;
		}
#endif
		if (*p == TEXT('\r') || *p == TEXT('\n')) {
			for (; *p == TEXT('\r') || *p == TEXT('\n'); p++)
				;
			if (delete_crlf_delete_space == 1) {
				for (; *p == TEXT(' ') || *p == TEXT('\t'); p++)
					;
			}
		} else {
			*(r++) = *(p++);
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
 * delete_crlf - 改行の除去
 */
__declspec(dllexport) int CALLBACK delete_crlf(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	DATA_INFO *di;
	int ret = TOOL_SUCCEED;

	for (; tdi != NULL; tdi = tdi->next) {
#ifdef UNICODE
		di = (DATA_INFO *)SendMessage(hWnd, WM_ITEM_GET_FORMAT_TO_ITEM, (WPARAM)TEXT("UNICODE TEXT"), (LPARAM)tdi->di);
#else
		di = (DATA_INFO *)SendMessage(hWnd, WM_ITEM_GET_FORMAT_TO_ITEM, (WPARAM)TEXT("TEXT"), (LPARAM)tdi->di);
#endif
		if (di != NULL && di->data != NULL) {
			ret |= item_delete_crlf(di);
		}
	}
	return ret;
}

/*
 * set_delete_crlf_proc - 設定
 */
BOOL CALLBACK set_delete_crlf_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	TCHAR buf[BUF_SIZE];

	switch (uMsg) {
	case WM_INITDIALOG:
		CheckDlgButton(hDlg, IDC_CHECK_DELETE_SPACE, delete_crlf_delete_space);
		break;

	case WM_CLOSE:
		EndDialog(hDlg, FALSE);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			delete_crlf_delete_space = IsDlgButtonChecked(hDlg, IDC_CHECK_DELETE_SPACE);

			wsprintf(buf, TEXT("%d"), delete_crlf_delete_space);
			WritePrivateProfileString(TEXT("delete_crlf"), TEXT("delete_space"), buf, ini_path);
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
 * delete_crlf_property - プロパティ表示
 */
__declspec(dllexport) BOOL CALLBACK delete_crlf_property(const HWND hWnd, TOOL_EXEC_INFO *tei)
{
	DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_DELETE_CRLF), hWnd, set_delete_crlf_proc, 0);
	return TRUE;
}
/* End of source */
