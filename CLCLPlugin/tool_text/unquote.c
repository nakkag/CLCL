/*
 * tool_text
 *
 * unquote.c
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
BOOL CALLBACK set_quote_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/*
 * GetStrLen - 比較用の文字列の長さを取得する
 */
static int GetStrLen(const TCHAR *buf, int len)
{
	int i;

	if (len < 0) {
		return len;
	}

	for (i = 0; i < len; i++) {
		if (*buf == TEXT('\0')) {
			break;
		}
		buf++;
	}
	return i;
}

/*
 * lstrcmpn - ２つの文字列を文字数分比較を行う
 */
int lstrcmpn(const TCHAR *buf1, const TCHAR *buf2, const int len)
{
	int ret;
	int len1, len2;

	len1 = GetStrLen(buf1, len);
	len2 = GetStrLen(buf2, len);

	ret = CompareString(MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), SORT_DEFAULT),
		0, buf1, len1, buf2, len2);
	return ret - 2;
}

/*
 * item_unquote - テキストの引用符を削除
 */
static int item_unquote(DATA_INFO *di, const TCHAR *q_char)
{
	HANDLE ret;
	BYTE *from_mem, *to_mem;
	TCHAR *p, *r;
	int size;

	// コピー元ロック
	if ((from_mem = GlobalLock(di->data)) == NULL) {
		return TOOL_ERROR;
	}

	// 引用符を除去したサイズを取得
	p = (TCHAR *)from_mem;
	if (lstrcmpn(p, q_char, lstrlen(q_char)) == 0) {
		p += lstrlen(q_char);
	}
	size = 0;
	while (*p != TEXT('\0')) {
#ifndef UNICODE
		if (IsDBCSLeadByte((BYTE)*p) == TRUE) {
			p += 2;
			size += 2;
			continue;
		}
#endif
		size++;
		if (*p == TEXT('\n') && *(p + 1) != TEXT('\0')) {
			p++;
			if (lstrcmpn(p, q_char, lstrlen(q_char)) == 0) {
				p += lstrlen(q_char);
			}
		} else {
			p++;
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

	// 引用符の除去
	p = (TCHAR *)from_mem;
	if (lstrcmpn(p, q_char, lstrlen(q_char)) == 0) {
		p += lstrlen(q_char);
	}
	r = (TCHAR *)to_mem;
	while (*p != TEXT('\0')) {
#ifndef UNICODE
		if (IsDBCSLeadByte((BYTE)*p) == TRUE) {
			*(r++) = *(p++);
			*(r++) = *(p++);
			continue;
		}
#endif
		*(r++) = *p;
		if (*p == TEXT('\n') && *(p + 1) != TEXT('\0')) {
			p++;
			if (lstrcmpn(p, q_char, lstrlen(q_char)) == 0) {
				p += lstrlen(q_char);
			}
		} else {
			p++;
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
 * unquote - テキストの引用符を削除
 */
__declspec(dllexport) int CALLBACK unquote(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
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
			ret |= item_unquote(di, q_char);
		}
	}
	return ret;
}

/*
 * unquote_property - プロパティ表示
 */
__declspec(dllexport) BOOL CALLBACK unquote_property(const HWND hWnd, TOOL_EXEC_INFO *tei)
{
	DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_QUOTE), hWnd, set_quote_proc, 0);
	return TRUE;
}
/* End of source */
