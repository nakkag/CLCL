/*
 * CLCL
 *
 * CLCLPlugin.h Ver 0.0.1
 *
 * Copyright (C) 1996-2003 by Nakashima Tomoaki. All rights reserved.
 *		http://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_CLCLPLUGIN_H
#define _INC_CLCLPLUGIN_H

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE

/* Define */
#ifndef BUF_SIZE
#define BUF_SIZE						256
#endif

// general
#define WM_GET_VERSION					(WM_APP + 100)
#define WM_GET_WORKPATH					(WM_APP + 101)
#define WM_GET_CLIPBOARD_WATCH			(WM_APP + 102)
#define WM_SET_CLIPBOARD_WATCH			(WM_APP + 103)
#define WM_GET_FORMAT_ICON				(WM_APP + 104)
#define WM_ENABLE_ACCELERATOR			(WM_APP + 105)
#define WM_REGIST_HOTKEY				(WM_APP + 106)
#define WM_UNREGIST_HOTKEY				(WM_APP + 107)
// option
#define WM_OPTION_SHOW					(WM_APP + 200)
#define WM_OPTION_GET					(WM_APP + 201)
#define WM_OPTION_LOAD					(WM_APP + 202)
#define WM_OPTION_SAVE					(WM_APP + 203)
// history & regist
#define WM_HISTORY_CHANGED				(WM_APP + 300)
#define WM_HISTORY_GET_ROOT				(WM_APP + 301)
#define WM_HISTORY_LOAD					(WM_APP + 302)
#define WM_HISTORY_SAVE					(WM_APP + 303)
#define WM_REGIST_CHANGED				(WM_APP + 350)
#define WM_REGIST_GET_ROOT				(WM_APP + 351)
#define WM_REGIST_LOAD					(WM_APP + 352)
#define WM_REGIST_SAVE					(WM_APP + 353)
// item
#define WM_ITEM_TO_CLIPBOARD			(WM_APP + 400)
#define WM_ITEM_CREATE					(WM_APP + 401)
#define WM_ITEM_COPY					(WM_APP + 402)
#define WM_ITEM_FREE					(WM_APP + 403)
#define WM_ITEM_FREE_DATA				(WM_APP + 404)
#define WM_ITEM_CHECK					(WM_APP + 405)
#define WM_ITEM_TO_BYTES				(WM_APP + 406)
#define WM_ITEM_FROM_BYTES				(WM_APP + 407)
#define WM_ITEM_TO_FILE					(WM_APP + 408)
#define WM_ITEM_FROM_FILE				(WM_APP + 409)
#define WM_ITEM_GET_PARENT				(WM_APP + 410)
#define WM_ITEM_GET_FORMAT_TO_ITEM		(WM_APP + 411)
#define WM_ITEM_GET_PRIORITY_HIGHEST	(WM_APP + 412)
#define WM_ITEM_GET_TITLE				(WM_APP + 413)
#define WM_ITEM_GET_OPEN_INFO			(WM_APP + 414)
#define WM_ITEM_GET_SAVE_INFO			(WM_APP + 415)
// viewer
#define WM_VIEWER_SHOW					(WM_APP + 500)
#define WM_VIEWER_GET_HWND				(WM_APP + 501)
#define WM_VIEWER_GET_MAIN_HWND			(WM_APP + 504)
#define WM_VIEWER_GET_SELECTION			(WM_APP + 502)
#define WM_VIEWER_SELECT_ITEM			(WM_APP + 503)

// data type
#define TYPE_DATA						0
#define TYPE_ITEM						1
#define TYPE_FOLDER						2
#define TYPE_ROOT						3

// ツールを実行するタイミング
// When to run the tool
#define CALLTYPE_MENU					1				// 動作メニュー / Operation menu
#define CALLTYPE_VIEWER					2				// ビューアのメニュー / Viewer menu
#define CALLTYPE_VIEWER_OPEN			4				// ビューアを開いた時 / when viewer is opened
#define CALLTYPE_VIEWER_CLOSE			8				// ビューアを閉じる時 / when viewer is closed
#define CALLTYPE_ADD_HISTORY			16				// データが履歴に追加される時 / when data is added to history
#define CALLTYPE_ITEM_TO_CLIPBOARD		32				// データをクリップボードに送る時 / when sending data to the clipboard
#define CALLTYPE_START					64				// 起動時 / at startup
#define CALLTYPE_END					128				// 終了時 / when finished
// option only
#define CALLTYPE_MENU_COPY_PASTE		256				// コピーと貼り付けを送る / send copy and paste
// execute only
#define CALLTYPE_HISTORY				512				// 履歴からの呼び出し / call from history
#define CALLTYPE_REGIST					1024			// 登録アイテムからの呼び出し / call from registered item

// ツール戻り値
// tool return value
#define TOOL_ERROR						0				// ツールのエラー / tool error
#define TOOL_SUCCEED					1				// ツールの正常終了 / tool successfully completed
#define TOOL_CANCEL						2				// 以降の処理をキャンセル / cancel further processing
#define TOOL_DATA_MODIFIED				4				// データ変更あり / data changed

/* Struct */
// 形式取得情報
// Format acquisition information
typedef struct _FORMAT_GET_INFO {
	DWORD struct_size;					// 構造体のサイズ / Structure size

	TCHAR format_name[BUF_SIZE];
	TCHAR func_header[BUF_SIZE];
	TCHAR comment[BUF_SIZE];
} FORMAT_GET_INFO;

// ツール取得情報
// Tool acquisition information
typedef struct _TOOL_GET_INFO {
	DWORD struct_size;					// 構造体のサイズ / Structure size

	TCHAR title[BUF_SIZE];
	TCHAR func_name[BUF_SIZE];
	TCHAR cmd_line[BUF_SIZE];
	int call_type;						// CALLTYPE_
} TOOL_GET_INFO;

// ツール実行情報
// Tool execution information
typedef struct _TOOL_EXEC_INFO {
	DWORD struct_size;					// 構造体のサイズ / Structure size

	int call_type;						// CALLTYPE_
	TCHAR *cmd_line;					// ツール設定で指定したコマンドライン / Command line specified in tool settings
	LPARAM lParam;						// ツールに対応するlong値 / long value corresponding to the tool
} TOOL_EXEC_INFO;

// アイテム情報
// Item information
typedef struct _DATA_INFO {
	DWORD struct_size;					// 構造体のサイズ / Structure size

	int type;							// TYPE_
	TCHAR *title;						// タイトル / title

	TCHAR *format_name;					// 形式名 / Format name
	int format_name_hash;				// 形式名のハッシュ / format name hash
	UINT format;						// 形式値 / Format value

	HANDLE data;						// データ / data
	DWORD size;							// サイズ / size

	FILETIME modified;					// 更新日時 / Update date and time
	TCHAR *window_name;					// コピーしたウィンドウタイトル / Copied window title

	TCHAR *plugin_string;				// プラグイン用データ / Data for plugins
	LPARAM plugin_param;

// 以下保存しない情報
// Information not saved below
	TCHAR *menu_title;					// メニューに表示するタイトル (未設定の場合は形式を表示) / Title to display on menu (if not set, display format)
	BOOL free_title;					// タイトルを TRUE-解放する FALSE-解放しない / Title TRUE-Release,  FALSE-Do not release
	HICON menu_icon;					// メニューに表示するアイコンハンドル / Icon handle to display in menu
	BOOL free_icon;						// アイコンハンドルを TRUE-解放する FALSE-解放しない / TRUE - release the icon handle, FALSE - do not release the icon handle
	HBITMAP menu_bitmap;				// メニューに表示するビットマップ / Bitmap to display in menu
	BOOL free_bitmap;					// ビットマップハンドルを TRUE-解放する FALSE-解放しない / TRUE - release bitmap handle, FALSE - do not release
	int menu_bmp_width;					// メニューに表示するビットマップの個別サイズ / Individual size of bitmap displayed in menu
	int menu_bmp_height;
	LPARAM param1;						// プラグイン用データ / Data for plugins
	LPARAM param2;

	struct _DATA_INFO *child;
	struct _DATA_INFO *next;

// Ver 1.0.5
	int hkey_id;						// ホットキー / hotkey
	UINT op_modifiers;
	UINT op_virtkey;
	int op_paste;
} DATA_INFO;

// ツール用アイテム情報
// Tool item information
typedef struct _TOOL_DATA_INFO {
	DWORD struct_size;					// 構造体のサイズ / Structure size

	struct _DATA_INFO *di;				// アイテム情報 / Item information

	struct _TOOL_DATA_INFO *child;
	struct _TOOL_DATA_INFO *next;
} TOOL_DATA_INFO;

#endif
/* End of source */
