/*
 * tool_text
 *
 * to_upper.c
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

/* Define */

/* Global Variables */

/* Local Function Prototypes */

/*
 * item_to_upper - 文字列を大文字に変換
 */
static int item_to_upper(DATA_INFO *di)
{
	BYTE *mem;

	// メモリのロック
	if ((mem = GlobalLock(di->data)) == NULL) {
		return TOOL_ERROR;
	}

	// 大文字に変換
	CharUpper((TCHAR *)mem);

	GlobalUnlock(di->data);
	return TOOL_DATA_MODIFIED;
}

/*
 * to_upper - テキストを大文字に変換
 */
__declspec(dllexport) int CALLBACK to_upper(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
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
			ret |= item_to_upper(di);
		}
	}
	return ret;
}

/*
 * to_upper_property - プロパティ表示
 */
__declspec(dllexport) BOOL CALLBACK to_upper_property(const HWND hWnd, TOOL_EXEC_INFO *tei)
{
	return FALSE;
}
/* End of source */
