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

/* Define */

/* Global Variables */

/* Local Function Prototypes */

/*
 * item_to_lower - 文字列を小文字に変換
 */
static int item_to_lower(DATA_INFO *di)
{
	BYTE *mem;

	// メモリのロック
	if ((mem = GlobalLock(di->data)) == NULL) {
		return TOOL_ERROR;
	}

	// 小文字に変換
	CharLower((TCHAR *)mem);

	GlobalUnlock(di->data);
	return TOOL_DATA_MODIFIED;
}

/*
 * to_lower - テキストを小文字に変換
 *
 *	引数 / argument:
 *		hWnd - 呼び出し元ウィンドウ / the calling window
 *		tei - ツール実行情報 / tool execution information
 *		tdi - ツール用アイテム情報 / item information for tools
 *
 *	戻り値 / Return value:
 *		TOOL_
 */
__declspec(dllexport) int CALLBACK to_lower(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
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
			ret |= item_to_lower(di);
		}
	}
	return ret;
}

/*
 * to_lower_property - プロパティ表示
 */
__declspec(dllexport) BOOL CALLBACK to_lower_property(const HWND hWnd, TOOL_EXEC_INFO *tei)
{
	return FALSE;
}
/* End of source */
