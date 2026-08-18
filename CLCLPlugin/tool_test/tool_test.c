/*
 * tool_test
 *
 * tool_test.c
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
HINSTANCE hInst;

/* Local Function Prototypes */

/*
 * DllMain - メイン
 */
int WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved)
{
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		hInst = hInstance;
		break;

	case DLL_PROCESS_DETACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}

/*
 * get_tool_info_w - ツール情報取得
 * Get tool information
 *
 *	引数 / argument:
 *		hWnd - 呼び出し元ウィンドウ / the calling window
 *		index - 取得のインデックス (0～) / the index of the acquisition (from 0)
 *		tgi - ツール取得情報 / tool retrieval information
 *
 *	戻り値 / Return value:
 *		TRUE - 次に取得するツールあり / has tools to get next
 *		FALSE - 取得の終了 / end of acquisition
 */
__declspec(dllexport) BOOL CALLBACK get_tool_info_w(const HWND hWnd, const int index, TOOL_GET_INFO *tgi)
{
	switch (index) {
	case 0:
		// Remove specific format in item
		LoadString(hInst, IDS_TEST1, tgi->title, BUF_SIZE - 1);
		lstrcpy(tgi->func_name, TEXT("test1"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_MENU | CALLTYPE_VIEWER;
		return TRUE;

	case 1:
		// Delete item
		LoadString(hInst, IDS_TEST2, tgi->title, BUF_SIZE - 1);
		lstrcpy(tgi->func_name, TEXT("test2"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_MENU | CALLTYPE_VIEWER;
		return TRUE;

	case 2:
		// Save item
		LoadString(hInst, IDS_TEST3, tgi->title, BUF_SIZE - 1);
		lstrcpy(tgi->func_name, TEXT("test3"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_MENU | CALLTYPE_VIEWER;
		return TRUE;

	case 3:
		// Create a text item from a file
		LoadString(hInst, IDS_TEST4, tgi->title, BUF_SIZE - 1);
		lstrcpy(tgi->func_name, TEXT("test4"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_MENU | CALLTYPE_VIEWER;
		return TRUE;

	case 4:
		// Show item title
		LoadString(hInst, IDS_TEST5, tgi->title, BUF_SIZE - 1);
		lstrcpy(tgi->func_name, TEXT("test5"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_MENU | CALLTYPE_VIEWER;
		return TRUE;

	case 5:
		// Save the data in a separate file
		LoadString(hInst, IDS_TEST6, tgi->title, BUF_SIZE - 1);
		lstrcpy(tgi->func_name, TEXT("test6"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_START | CALLTYPE_END;
		return TRUE;

	case 6:
		// Test7
		LoadString(hInst, IDS_TEST7, tgi->title, BUF_SIZE - 1);
		lstrcpy(tgi->func_name, TEXT("test7"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_MENU | CALLTYPE_VIEWER;
		return TRUE;
	}
	return FALSE;
}

/*
 * test1 - アイテム内の特定形式を削除
 * test1 - Remove specific format in item 
 *
 *	引数 / argument:
 *		hWnd - 呼び出し元ウィンドウ / the calling window
 *		tei - ツール実行情報 / tool execution information
 *		tdi - ツール用アイテム情報 / item information for tools
 *
 *	戻り値 / Return value:
 *		TOOL_
 */
__declspec(dllexport) int CALLBACK test1(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	DATA_INFO *cdi;
	DATA_INFO *pdi;

	for (; tdi != NULL; tdi = tdi->next) {
		if (tdi->di->type == TYPE_ITEM) {
			// 形式の検索
			// Format search 
			pdi = NULL;
			for (cdi = tdi->di->child; cdi != NULL && lstrcmpi(cdi->format_name, TEXT("TEXT")) != 0; cdi = cdi->next) {
				pdi = cdi;
			}
			if (cdi == NULL) {
				continue;
			}
			// 形式をリストから削除
			// Remove format from list
			if (pdi == NULL) {
				tdi->di->child = cdi->next;
			} else {
				pdi->next = cdi->next;
			}
			cdi->next = NULL;
			// アイテムの解放
			// Release items 
			SendMessage(hWnd, WM_ITEM_FREE, 0, (LPARAM)cdi);
		}
	}
	// アイテムの変化を通知
	// Notify item changes 
	if (tei->call_type & CALLTYPE_HISTORY) {
		SendMessage(hWnd, WM_HISTORY_CHANGED, 0, 0);
	} else if (tei->call_type & CALLTYPE_REGIST) {
		SendMessage(hWnd, WM_REGIST_CHANGED, 0, 0);
	}
	return TOOL_SUCCEED;
}

/*
 * test2 - アイテムの削除
 * test2 - Delete item
 */
__declspec(dllexport) int CALLBACK test2(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	DATA_INFO *di;

	if ((di = (DATA_INFO *)SendMessage(hWnd, WM_ITEM_GET_PARENT, 0, (LPARAM)tdi->di)) == NULL) {
		return TOOL_SUCCEED;
	}

	// アイテムをリストから削除
	// Remove item from list
	if (di->child == tdi->di) {
		di->child = tdi->di->next;
	} else {
		for (di = di->child; di != NULL && di->next != tdi->di; di = di->next)
			;
		if (di == NULL) {
			return TOOL_SUCCEED;
		}
		di->next = tdi->di->next;
	}
	tdi->di->next = NULL;
	// アイテムの解放
	// Release items 
	SendMessage(hWnd, WM_ITEM_FREE, 0, (LPARAM)tdi->di);

	// アイテムの変化を通知
	// Notify item changes 
	if (tei->call_type & CALLTYPE_HISTORY) {
		SendMessage(hWnd, WM_HISTORY_CHANGED, 0, 0);
	} else if (tei->call_type & CALLTYPE_REGIST) {
		SendMessage(hWnd, WM_REGIST_CHANGED, 0, 0);
	}
	return TOOL_SUCCEED;
}

/*
 * test3 - アイテムの保存
 * test3 - Save item
 */
__declspec(dllexport) int CALLBACK test3(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	OPENFILENAME of;
	TCHAR file_name[MAX_PATH];
	DATA_INFO *di;

	if ((di = (DATA_INFO *)SendMessage(hWnd, WM_ITEM_GET_PRIORITY_HIGHEST, 0, (LPARAM)tdi->di)) == NULL) {
		return TOOL_SUCCEED;
	}
	ZeroMemory(&of, sizeof(OPENFILENAME));
	of.lStructSize = sizeof(OPENFILENAME);
	of.hInstance = hInst;
	of.hwndOwner = hWnd;
	of.lpstrFilter = TEXT("*.*\0*.*\0\0");
	of.nFilterIndex = 1;
	*file_name = TEXT('\0');
	of.lpstrFile = file_name;
	of.nMaxFile = MAX_PATH - 1;
	of.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	// 保存情報取得とファイル名選択
	// Get saved information and select file name
	if (SendMessage(hWnd, WM_ITEM_GET_SAVE_INFO, (WPARAM)&of, (LPARAM)di) >= 0 && GetSaveFileName(&of) == FALSE) {
		return TOOL_CANCEL;
	}
	// アイテムをファイルに保存
	// Save item to file
	SendMessage(hWnd, WM_ITEM_TO_FILE, (WPARAM)file_name, (LPARAM)di);
	return TOOL_SUCCEED;
}

/*
 * test4 - ファイルからテキスト形式のアイテムを作成
 * test4 - Create a text item from a file
 */
__declspec(dllexport) int CALLBACK test4(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	OPENFILENAME of;
	TCHAR file_name[MAX_PATH];
	DATA_INFO *history_di;
	DATA_INFO *di;

	// 履歴の取得
	// Get history
	if ((history_di = (DATA_INFO *)SendMessage(hWnd, WM_HISTORY_GET_ROOT, 0, 0)) == NULL) {
		return TOOL_SUCCEED;
	}

	ZeroMemory(&of, sizeof(OPENFILENAME));
	of.lStructSize = sizeof(OPENFILENAME);
	of.hInstance = hInst;
	of.hwndOwner = hWnd;
	of.lpstrFilter = TEXT("*.*\0*.*\0\0");
	of.nFilterIndex = 1;
	*file_name = TEXT('\0');
	of.lpstrFile = file_name;
	of.nMaxFile = MAX_PATH - 1;
	of.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	// 保存情報取得とファイル名選択
	// Get saved information and select file name
	if (SendMessage(hWnd, WM_ITEM_GET_OPEN_INFO, (WPARAM)&of, (LPARAM)TEXT("TEXT")) >= 0 && GetOpenFileName(&of) == FALSE) {
		return TOOL_CANCEL;
	}
	// アイテムの作成
	// Creating an item
	if ((di = (DATA_INFO *)SendMessage(hWnd, WM_ITEM_CREATE, TYPE_ITEM, 0)) == NULL) {
		return TOOL_CANCEL;
	}
	if ((di->child = (DATA_INFO *)SendMessage(hWnd, WM_ITEM_CREATE, TYPE_DATA, (LPARAM)TEXT("TEXT"))) == NULL) {
		SendMessage(hWnd, WM_ITEM_FREE, 0, (LPARAM)di);
		return TOOL_CANCEL;
	}
	// ファイルを読み込んでアイテムに設定
	// Load a file and set it as an item
	SendMessage(hWnd, WM_ITEM_FROM_FILE, (WPARAM)file_name, (LPARAM)di->child);

	// 履歴に追加
	// Add to history
	di->next = history_di->child;
	history_di->child = di;
	// 履歴の変化を通知
	// Notify history changes
	SendMessage(hWnd, WM_HISTORY_CHANGED, 0, 0);
	return TOOL_SUCCEED;
}

/*
 * test5 - アイテムのタイトルを表示
 * test5 - Show item title
 */
__declspec(dllexport) int CALLBACK test5(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	DATA_INFO *di;
	TCHAR buf[BUF_SIZE];

	// ビューアの選択アイテム取得
	// Get selected items in the viewer
	if ((di = (DATA_INFO *)SendMessage(hWnd, WM_VIEWER_GET_SELECTION, 0, 0)) != NULL) {
		// 選択アイテムのタイトルを表示
		// Show title of selected item
		SendMessage(hWnd, WM_ITEM_GET_TITLE, (WPARAM)buf, (LPARAM)di);
		MessageBox(hWnd, buf, TEXT("sel item"), 0);
	}

	// 履歴の取得
	// Get history
	if ((di = (DATA_INFO *)SendMessage(hWnd, WM_HISTORY_GET_ROOT, 0, 0)) == NULL) {
		return TOOL_SUCCEED;
	}
	for (di = di->child; di != NULL; di = di->next) {
		// アイテムのタイトルを取得
		// Get the title of the item
		SendMessage(hWnd, WM_ITEM_GET_TITLE, (WPARAM)buf, (LPARAM)di);
		if (MessageBox(hWnd, buf, TEXT("title"), MB_OKCANCEL) == IDCANCEL) {
			// ビューアでアイテムを選択状態にする
			// Select an item in the viewer 
			SendMessage(hWnd, WM_VIEWER_SELECT_ITEM, 0, (LPARAM)di);
			break;
		}
	}
	return TOOL_SUCCEED;
}

/*
 * test6 - データを別ファイルに保存
 * test6 - Save the data in a separate file
 */
__declspec(dllexport) int CALLBACK test6(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	DATA_INFO *di;
	TCHAR buf[BUF_SIZE];
	TCHAR test_filename[BUF_SIZE];
	TCHAR history_filename[BUF_SIZE];

	SendMessage(hWnd, WM_GET_WORKPATH, 0, (LPARAM)buf);
	wsprintf(test_filename, TEXT("%s\\test.dat"), buf);
	wsprintf(history_filename, TEXT("%s\\history.dat"), buf);

	if (tei->call_type & CALLTYPE_START) {
		if (CopyFile(test_filename, history_filename, FALSE) == TRUE) {
			SendMessage(hWnd, WM_HISTORY_LOAD, 0, 0);
		}

	} else if (tei->call_type & CALLTYPE_END) {
		SendMessage(hWnd, WM_HISTORY_SAVE, 0, 0);
		CopyFile(history_filename, test_filename, FALSE);
		if ((di = (DATA_INFO *)SendMessage(hWnd, WM_HISTORY_GET_ROOT, 0, 0)) != NULL) {
			SendMessage(hWnd, WM_ITEM_FREE, 0, (LPARAM)di->child);
			di->child = NULL;
			SendMessage(hWnd, WM_HISTORY_CHANGED, 0, 0);
		}
	}
	return TOOL_SUCCEED;
}

/*
 * test7 - テスト
 * test7 - Test
 */
__declspec(dllexport) int CALLBACK test7(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	return TOOL_SUCCEED;
}
/* End of source */
