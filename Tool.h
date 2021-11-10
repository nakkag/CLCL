/*
 * CLCL
 *
 * Tool.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_TOOL_H
#define _INC_TOOL_H

/* Include Files */
#include "General.h"
#include "Data.h"

/* Define */
// ツールを実行するタイミング
#define CALLTYPE_MENU					1				// 動作メニュー
#define CALLTYPE_VIEWER					2				// ビューアのメニュー
#define CALLTYPE_VIEWER_OPEN			4				// ビューアを開いた時
#define CALLTYPE_VIEWER_CLOSE			8				// ビューアを閉じる時
#define CALLTYPE_ADD_HISTORY			16				// データが履歴に追加される時
#define CALLTYPE_ITEM_TO_CLIPBOARD		32				// データをクリップボードに送る時
#define CALLTYPE_START					64				// 起動時
#define CALLTYPE_END					128				// 終了時
// option only
#define CALLTYPE_MENU_COPY_PASTE		256				// コピーと貼り付けを送る
// execute only
#define CALLTYPE_HISTORY				512				// 履歴からの呼び出し
#define CALLTYPE_REGIST					1024			// 登録アイテムからの呼び出し

// ツール戻り値
#define TOOL_ERROR						0				// ツールのエラー
#define TOOL_SUCCEED					1				// ツールの正常終了
#define TOOL_CANCEL						2				// 以降の処理をキャンセル
#define TOOL_DATA_MODIFIED				4				// データ変更あり


// ツール呼び出し方法 (旧ver)
#define OLD_CALLTYPE_VIEWER				0
#define OLD_CALLTYPE_ADD_HISTORY		1
#define OLD_CALLTYPE_ITEM_TO_CLIPBOARD	2
#define OLD_CALLTYPE_NOITEM				4
#define OLD_CALLTYPE_MENU				8
#define OLD_CALLTYPE_START				16
#define OLD_CALLTYPE_END				32

// 旧ツールの関数形式
typedef int (__cdecl *OLD_TOOL_FUNC)(HWND, void *, int, int);
typedef int (__cdecl *OLD_GET_FUNC)(int, TCHAR *, TCHAR *, long *);

/* Struct */
// ツール情報
typedef struct _TOOL_INFO {
	TCHAR *title;
	TCHAR *lib_file_path;
	TCHAR *func_name;
	TCHAR *cmd_line;
	int call_type;						// CALLTYPE_
	int copy_paste;

	// hot key
	int id;
	UINT modifiers;
	UINT virtkey;

	HANDLE lib;
	FARPROC func;
	OLD_TOOL_FUNC old_func;

	int old;

	LPARAM lParam;						// ツールに対応するlong値
} TOOL_INFO;

// ツール取得情報
typedef struct _TOOL_GET_INFO {
	DWORD struct_size;					// 構造体のサイズ

	TCHAR title[BUF_SIZE];
	TCHAR func_name[BUF_SIZE];
	TCHAR cmd_line[BUF_SIZE];
	int call_type;						// CALLTYPE_
} TOOL_GET_INFO;
typedef struct _TOOL_GET_INFO_A {
	DWORD struct_size;					// 構造体のサイズ

	char title[BUF_SIZE];
	char func_name[BUF_SIZE];
	char cmd_line[BUF_SIZE];
	int call_type;						// CALLTYPE_
} TOOL_GET_INFO_A;

// ツール実行情報
typedef struct _TOOL_EXEC_INFO {
	DWORD struct_size;					// 構造体のサイズ

	int call_type;						// CALLTYPE_
	TCHAR *cmd_line;					// ツール設定で指定したコマンドライン
	LPARAM lParam;						// ツールに対応するlong値
} TOOL_EXEC_INFO;

// ツール用アイテム情報
typedef struct _TOOL_DATA_INFO {
	DWORD struct_size;					// 構造体のサイズ

	struct _DATA_INFO *di;				// アイテム情報

	struct _TOOL_DATA_INFO *child;
	struct _TOOL_DATA_INFO *next;
} TOOL_DATA_INFO;

/* Function Prototypes */
int tool_title_to_index(const TCHAR *title);
BOOL tool_initialize(TCHAR *err_str);
TOOL_DATA_INFO *tool_data_copy(DATA_INFO *di, const BOOL next_copy);
void tool_data_free(TOOL_DATA_INFO *tdi);
int tool_execute(const HWND hWnd, TOOL_INFO *ti, const int call_type, DATA_INFO *di, TOOL_DATA_INFO *tdi);
int tool_execute_all(const HWND hWnd, const int call_type, DATA_INFO *di);

#endif
/* End of source */
