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
 */
__declspec(dllexport) BOOL CALLBACK get_tool_info_w(const HWND hWnd, const int index, TOOL_GET_INFO *tgi)
{
	switch (index) {
	case 0:
		lstrcpy(tgi->title, TEXT("テスト1"));
		lstrcpy(tgi->func_name, TEXT("test1"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_MENU | CALLTYPE_VIEWER;
		return TRUE;

	case 1:
		lstrcpy(tgi->title, TEXT("テスト2"));
		lstrcpy(tgi->func_name, TEXT("test2"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_MENU | CALLTYPE_VIEWER;
		return TRUE;

	case 2:
		lstrcpy(tgi->title, TEXT("テスト3"));
		lstrcpy(tgi->func_name, TEXT("test3"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_MENU | CALLTYPE_VIEWER;
		return TRUE;

	case 3:
		lstrcpy(tgi->title, TEXT("テスト4"));
		lstrcpy(tgi->func_name, TEXT("test4"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_MENU | CALLTYPE_VIEWER;
		return TRUE;

	case 4:
		lstrcpy(tgi->title, TEXT("テスト5"));
		lstrcpy(tgi->func_name, TEXT("test5"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_MENU | CALLTYPE_VIEWER;
		return TRUE;

	case 5:
		lstrcpy(tgi->title, TEXT("テスト6"));
		lstrcpy(tgi->func_name, TEXT("test6"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_START | CALLTYPE_END;
		return TRUE;

	case 6:
		lstrcpy(tgi->title, TEXT("テスト7"));
		lstrcpy(tgi->func_name, TEXT("test7"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_MENU | CALLTYPE_VIEWER;
		return TRUE;
	}
	return FALSE;
}

/*
 * test1 - アイテム内の特定形式を削除
 */
__declspec(dllexport) int CALLBACK test1(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	DATA_INFO *cdi;
	DATA_INFO *pdi;

	for (; tdi != NULL; tdi = tdi->next) {
		if (tdi->di->type == TYPE_ITEM) {
			// 形式の検索
			pdi = NULL;
			for (cdi = tdi->di->child; cdi != NULL && lstrcmpi(cdi->format_name, TEXT("TEXT")) != 0; cdi = cdi->next) {
				pdi = cdi;
			}
			if (cdi == NULL) {
				continue;
			}
			// 形式をリストから削除
			if (pdi == NULL) {
				tdi->di->child = cdi->next;
			} else {
				pdi->next = cdi->next;
			}
			cdi->next = NULL;
			// アイテムの解放
			SendMessage(hWnd, WM_ITEM_FREE, 0, (LPARAM)cdi);
		}
	}
	// アイテムの変化を通知
	if (tei->call_type & CALLTYPE_HISTORY) {
		SendMessage(hWnd, WM_HISTORY_CHANGED, 0, 0);
	} else if (tei->call_type & CALLTYPE_REGIST) {
		SendMessage(hWnd, WM_REGIST_CHANGED, 0, 0);
	}
	return TOOL_SUCCEED;
}

/*
 * test2 - アイテムの削除
 */
__declspec(dllexport) int CALLBACK test2(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	DATA_INFO *di;

	if ((di = (DATA_INFO *)SendMessage(hWnd, WM_ITEM_GET_PARENT, 0, (LPARAM)tdi->di)) == NULL) {
		return TOOL_SUCCEED;
	}

	// アイテムをリストから削除
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
	SendMessage(hWnd, WM_ITEM_FREE, 0, (LPARAM)tdi->di);

	// アイテムの変化を通知
	if (tei->call_type & CALLTYPE_HISTORY) {
		SendMessage(hWnd, WM_HISTORY_CHANGED, 0, 0);
	} else if (tei->call_type & CALLTYPE_REGIST) {
		SendMessage(hWnd, WM_REGIST_CHANGED, 0, 0);
	}
	return TOOL_SUCCEED;
}

/*
 * test3 - アイテムの保存
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
	if (SendMessage(hWnd, WM_ITEM_GET_SAVE_INFO, (WPARAM)&of, (LPARAM)di) >= 0 && GetSaveFileName(&of) == FALSE) {
		return TOOL_CANCEL;
	}
	// アイテムをファイルに保存
	SendMessage(hWnd, WM_ITEM_TO_FILE, (WPARAM)file_name, (LPARAM)di);
	return TOOL_SUCCEED;
}

/*
 * test4 - ファイルからテキスト形式のアイテムを作成
 */
__declspec(dllexport) int CALLBACK test4(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	OPENFILENAME of;
	TCHAR file_name[MAX_PATH];
	DATA_INFO *history_di;
	DATA_INFO *di;

	// 履歴の取得
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
	if (SendMessage(hWnd, WM_ITEM_GET_OPEN_INFO, (WPARAM)&of, (LPARAM)TEXT("TEXT")) >= 0 && GetOpenFileName(&of) == FALSE) {
		return TOOL_CANCEL;
	}
	// アイテムの作成
	if ((di = (DATA_INFO *)SendMessage(hWnd, WM_ITEM_CREATE, TYPE_ITEM, 0)) == NULL) {
		return TOOL_CANCEL;
	}
	if ((di->child = (DATA_INFO *)SendMessage(hWnd, WM_ITEM_CREATE, TYPE_DATA, (LPARAM)TEXT("TEXT"))) == NULL) {
		SendMessage(hWnd, WM_ITEM_FREE, 0, (LPARAM)di);
		return TOOL_CANCEL;
	}
	// ファイルを読み込んでアイテムに設定
	SendMessage(hWnd, WM_ITEM_FROM_FILE, (WPARAM)file_name, (LPARAM)di->child);

	// 履歴に追加
	di->next = history_di->child;
	history_di->child = di;
	// 履歴の変化を通知
	SendMessage(hWnd, WM_HISTORY_CHANGED, 0, 0);
	return TOOL_SUCCEED;
}

/*
 * test5 - アイテムのタイトルを表示
 */
__declspec(dllexport) int CALLBACK test5(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	DATA_INFO *di;
	TCHAR buf[BUF_SIZE];

	// ビューアの選択アイテム取得
	if ((di = (DATA_INFO *)SendMessage(hWnd, WM_VIEWER_GET_SELECTION, 0, 0)) != NULL) {
		// 選択アイテムのタイトルを表示
		SendMessage(hWnd, WM_ITEM_GET_TITLE, (WPARAM)buf, (LPARAM)di);
		MessageBox(hWnd, buf, TEXT("sel item"), 0);
	}

	// 履歴の取得
	if ((di = (DATA_INFO *)SendMessage(hWnd, WM_HISTORY_GET_ROOT, 0, 0)) == NULL) {
		return TOOL_SUCCEED;
	}
	for (di = di->child; di != NULL; di = di->next) {
		// アイテムのタイトルを取得
		SendMessage(hWnd, WM_ITEM_GET_TITLE, (WPARAM)buf, (LPARAM)di);
		if (MessageBox(hWnd, buf, TEXT("title"), MB_OKCANCEL) == IDCANCEL) {
			// ビューアでアイテムを選択状態にする
			SendMessage(hWnd, WM_VIEWER_SELECT_ITEM, 0, (LPARAM)di);
			break;
		}
	}
	return TOOL_SUCCEED;
}

/*
 * test6 - データを別ファイルに保存
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
 */
__declspec(dllexport) int CALLBACK test7(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	return TOOL_SUCCEED;
}
/* End of source */
