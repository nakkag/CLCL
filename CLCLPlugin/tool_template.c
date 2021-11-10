/*
 * CLCL
 *
 * tool_template.c
 *
 * Copyright (C) 1996-2003 by Nakashima Tomoaki. All rights reserved.
 *		http://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE

#include "CLCLPlugin.h"

/* Define */

/* Global Variables */

/* Local Function Prototypes */

/*
 * get_tool_info_w - ツール情報取得
 *
 *	引数:
 *		hWnd - 呼び出し元ウィンドウ
 *		index - 取得のインデックス (0～)
 *		tgi - ツール取得情報
 *
 *	戻り値:
 *		TRUE - 次に取得するツールあり
 *		FALSE - 取得の終了
 */
__declspec(dllexport) BOOL CALLBACK get_tool_info_w(const HWND hWnd, const int index, TOOL_GET_INFO *tgi)
{
	switch (index) {
	case 0:
		lstrcpy(tgi->title, TEXT("タイトル"));
		lstrcpy(tgi->func_name, TEXT("func_tool"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_MENU | CALLTYPE_VIEWER;	// CALLTYPE_
		return TRUE;

	case 1:
		return FALSE;
	}
	return FALSE;
}

/*
 * func_tool - ツール処理
 *
 *	引数:
 *		hWnd - 呼び出し元ウィンドウ
 *		tei - ツール実行情報
 *		tdi - ツール用アイテム情報
 *
 *	戻り値:
 *		TOOL_
 */
__declspec(dllexport) int CALLBACK func_tool(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	return TOOL_SUCCEED;
}

/*
 * func_tool_property - プロパティ表示
 *
 *	引数:
 *		hWnd - オプションウィンドウのハンドル
 *		tei - ツール実行情報
 *
 *	戻り値:
 *		TRUE - プロパティあり
 *		FALSE - プロパティなし
 */
__declspec(dllexport) BOOL CALLBACK func_tool_property(const HWND hWnd, TOOL_EXEC_INFO *tei)
{
	return FALSE;
}
/* End of source */
