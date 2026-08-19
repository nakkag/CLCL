/*
 * tool_text
 *
 * convert_date.c
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

extern TCHAR date_format[];
extern TCHAR time_format[];

/* Local Function Prototypes */

/*
 * item_convert_date - テキストの日時変換
 * Text to Datetime Conversion
 */
static int item_convert_date(DATA_INFO *di)
{
	HANDLE ret;
	BYTE *from_mem, *to_mem;
	TCHAR buf[BUF_SIZE];
	TCHAR *p, *r, *s, *t;
	int size;

	// コピー元ロック
	// lock the source
	if ((from_mem = GlobalLock(di->data)) == NULL) {
		return TOOL_ERROR;
	}

	// 変換後サイズ取得
	// Get size for conversion
	for (p = (TCHAR *)from_mem, size = 0; *p != TEXT('\0'); p++) {
		if (IsDBCSLeadByte((BYTE)*p) == TRUE) {
			p++;
			size += 2;
			continue;
		}
		if (*p != TEXT('%')) {
			size++;
			continue;
		}
		p++;
		switch (*p) {
		case TEXT('f'): case TEXT('F'):
			// 形式指定
			// format specification
			break;

		case TEXT('d'): case TEXT('D'):
			// 日付
			// date
			t = (*date_format == TEXT('\0')) ? NULL : date_format;
			GetDateFormat(0, 0, NULL, t, buf, BUF_SIZE - 1);
			size += lstrlen(buf);
			break;

		case TEXT('t'): case TEXT('T'):
			// 時間
			// time
			t = (*time_format == TEXT('\0')) ? NULL : time_format;
			GetTimeFormat(0, 0, NULL, t, buf, BUF_SIZE - 1);
			size += lstrlen(buf);
			break;

		case TEXT('%'):
			// %
			size++;
			break;

		default:
			size += 2;
		}
	}
	size++;

	// コピー先確保
	// reserve memory for copy target
	if ((ret = GlobalAlloc(GHND, sizeof(TCHAR) * size)) == NULL) {
		GlobalUnlock(di->data);
		return TOOL_ERROR;
	}
	// コピー先ロック
	// lock copy target
	if ((to_mem = GlobalLock(ret)) == NULL) {
		GlobalFree(ret);
		GlobalUnlock(di->data);
		return TOOL_ERROR;
	}

	// 変換
	// conversion
	for (p = (TCHAR *)from_mem, r = (TCHAR *)to_mem; *p != TEXT('\0'); p++) {
		if (IsDBCSLeadByte((BYTE)*p) == TRUE) {
			*(r++) = *(p++);
			*(r++) = *p;
			continue;
		}
		if (*p != TEXT('%')) {
			*(r++) = *p;
			continue;
		}
		p++;
		switch (*p) {
		case TEXT('f'): case TEXT('F'):
			// 形式指定
			// format specification
			if (*(p + 1) != TEXT('"')) {
				*(r++) = TEXT('%');
				*(r++) = *p;
				break;
			}
			t = p + 2;
			for (s = t; *s != TEXT('\0') && *s != TEXT('"'); s++);
			if (*s != TEXT('"')) {
				*(r++) = TEXT('%');
				*(r++) = *p;
				break;
			}

			p = s;
			break;

		case TEXT('d'): case TEXT('D'):
			// 日付
			// date
			t = (*date_format == TEXT('\0')) ? NULL : date_format;
			GetDateFormat(0, 0, NULL, t, buf, BUF_SIZE - 1);
			lstrcpy(r, buf);
			r += lstrlen(r);
			break;

		case TEXT('t'): case TEXT('T'):
			// 時間
			// time
			t = (*time_format == TEXT('\0')) ? NULL : time_format;
			GetTimeFormat(0, 0, NULL, t, buf, BUF_SIZE - 1);
			lstrcpy(r, buf);
			r += lstrlen(r);
			break;

		case TEXT('%'):
			// %
			*(r++) = TEXT('%');
			break;

		default:
			*(r++) = TEXT('%');
			*(r++) = *p;
			break;
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
 * convert_date - テキストの日時変換
 * Text to Datetime Conversion
 */
__declspec(dllexport) int CALLBACK convert_date(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	DATA_INFO *di;
	int ret = TOOL_SUCCEED;

	// 登録アイテムからのみ実行
	if ((tei->call_type & CALLTYPE_ITEM_TO_CLIPBOARD) && !(tei->call_type & CALLTYPE_REGIST)) {
		return TOOL_SUCCEED;
	}
	for (; tdi != NULL; tdi = tdi->next) {
#ifdef UNICODE
		di = (DATA_INFO *)SendMessage(hWnd, WM_ITEM_GET_FORMAT_TO_ITEM, (WPARAM)TEXT("UNICODE TEXT"), (LPARAM)tdi->di);
#else
		di = (DATA_INFO *)SendMessage(hWnd, WM_ITEM_GET_FORMAT_TO_ITEM, (WPARAM)TEXT("TEXT"), (LPARAM)tdi->di);
#endif
		if (di != NULL && di->data != NULL) {
			ret |= item_convert_date(di);
		}
	}
	return ret;
}

/*
 * set_format_proc - 日時形式を設定
 */
static BOOL CALLBACK set_format_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		SendDlgItemMessage(hDlg, IDC_COMBO_DATE, CB_ADDSTRING, 0, (LPARAM)TEXT("yyyy/MM/dd"));
		SendDlgItemMessage(hDlg, IDC_COMBO_DATE, CB_ADDSTRING, 0, (LPARAM)TEXT("yy/MM/dd"));
		SendDlgItemMessage(hDlg, IDC_COMBO_DATE, CB_ADDSTRING, 0, (LPARAM)TEXT("yy/M/d"));
		SendDlgItemMessage(hDlg, IDC_COMBO_DATE, CB_ADDSTRING, 0, (LPARAM)TEXT("yyyy/M/d"));
		SendDlgItemMessage(hDlg, IDC_COMBO_DATE, CB_ADDSTRING, 0, (LPARAM)TEXT("yy/MM/dd' ('ddd')'"));
		SendDlgItemMessage(hDlg, IDC_COMBO_DATE, CB_ADDSTRING, 0, (LPARAM)TEXT("yyyy/MM/dd' ('ddd')'"));
		SendDlgItemMessage(hDlg, IDC_COMBO_DATE, CB_ADDSTRING, 0, (LPARAM)TEXT("yyyy/M/d' ('ddd')'"));
		SendDlgItemMessage(hDlg, IDC_COMBO_DATE, CB_ADDSTRING, 0, (LPARAM)TEXT("yyyy-MM-dd"));

		SendDlgItemMessage(hDlg, IDC_COMBO_DATE, CB_ADDSTRING, 0, (LPARAM)TEXT("yyyy'年'M'月'd'日'"));
		SendDlgItemMessage(hDlg, IDC_COMBO_DATE, CB_ADDSTRING, 0, (LPARAM)TEXT("yyyy'年'MM'月'dd'日'"));
		SendDlgItemMessage(hDlg, IDC_COMBO_DATE, CB_ADDSTRING, 0, (LPARAM)TEXT("yyyy'年'M'月'd'日' dddd"));
		SendDlgItemMessage(hDlg, IDC_COMBO_DATE, CB_ADDSTRING, 0, (LPARAM)TEXT("yyyy'年'MM'月'dd'日' dddd"));

		SendDlgItemMessage(hDlg, IDC_COMBO_TIME, CB_ADDSTRING, 0, (LPARAM)TEXT("H:mm:ss"));
		SendDlgItemMessage(hDlg, IDC_COMBO_TIME, CB_ADDSTRING, 0, (LPARAM)TEXT("HH:mm:ss"));
		SendDlgItemMessage(hDlg, IDC_COMBO_TIME, CB_ADDSTRING, 0, (LPARAM)TEXT("tt h:mm:ss"));
		SendDlgItemMessage(hDlg, IDC_COMBO_TIME, CB_ADDSTRING, 0, (LPARAM)TEXT("tt hh:mm:ss"));

		SendDlgItemMessage(hDlg, IDC_COMBO_DATE, WM_SETTEXT, 0, (LPARAM)date_format);
		SendDlgItemMessage(hDlg, IDC_COMBO_TIME, WM_SETTEXT, 0, (LPARAM)time_format);
		break;

	case WM_CLOSE:
		EndDialog(hDlg, FALSE);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			SendDlgItemMessage(hDlg, IDC_COMBO_DATE, WM_GETTEXT, BUF_SIZE - 1, (LPARAM)date_format);
			SendDlgItemMessage(hDlg, IDC_COMBO_TIME, WM_GETTEXT, BUF_SIZE - 1, (LPARAM)time_format);

			WritePrivateProfileString(TEXT("convert_date"), TEXT("date_format"), date_format, ini_path);
			WritePrivateProfileString(TEXT("convert_date"), TEXT("time_format"), time_format, ini_path);
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
 * convert_date_property - プロパティ表示
 */
__declspec(dllexport) BOOL CALLBACK convert_date_property(const HWND hWnd, TOOL_EXEC_INFO *tei)
{
	DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_DATE_TIME_FORMAT), hWnd, set_format_proc, 0);
	return TRUE;
}
/* End of source */
