/*
 * CLCL
 *
 * Tool.c
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE

#include "General.h"
#include "Memory.h"
#include "String.h"
#include "Data.h"
#include "Ini.h"
#include "Message.h"
#include "Format.h"
#include "Tool.h"

/* Define */

/* Global Variables */
extern DATA_INFO history_data;
extern DATA_INFO regist_data;

// オプション
extern OPTION_INFO option;

// 旧ツールのデータ情報
typedef struct _CLIPINFO {
	unsigned int Type;
	TCHAR name[BUF_SIZE];
	HANDLE Mem;
	long Size;
	long lParam;
	int Flag;
} CLIPINFO;

/* Local Function Prototypes */
static void tool_data_reflect(const int call_type, TOOL_DATA_INFO *tdi);
static int tool_call_func_old(const HWND hWnd, const TOOL_INFO *ti, const int call_type, TOOL_DATA_INFO *tdi);

/*
 * tool_title_to_index - タイトルからインデックスを取得
 */
int tool_title_to_index(const TCHAR *title)
{
	int i;

	for (i = 0; i < option.tool_cnt; i++) {
		if (!((option.tool_info + i)->call_type & CALLTYPE_MENU)) {
			continue;
		}
		if (lstrcmp((option.tool_info + i)->title, title) == 0) {
			return i;
		}
	}
	return -1;
}

/*
 * tool_initialize - ツール情報の初期化
 */
BOOL tool_initialize(TCHAR *err_str)
{
	HANDLE lib;
	TCHAR buf[BUF_SIZE];
	char cbuf[BUF_SIZE];
	int i;

	for (i = 0; i < option.tool_cnt; i++) {
		if ((option.tool_info + i)->lib_file_path == NULL ||
			*(option.tool_info + i)->lib_file_path == TEXT('\0')) {
			continue;
		}
		// モジュールハンドル取得
		lib = (option.tool_info + i)->lib = LoadLibrary((option.tool_info + i)->lib_file_path);
		if (lib == NULL) {
			message_get_error(GetLastError(), buf);
			wsprintf(err_str, TEXT("%s\r\n%s"), buf, (option.tool_info + i)->lib_file_path);
			return FALSE;
		}
		// 形式毎の関数アドレス取得
		tchar_to_char((option.tool_info + i)->func_name, cbuf, BUF_SIZE - 1);
		if ((option.tool_info + i)->old == 0 || (option.tool_info + i)->old == 2) {
			(option.tool_info + i)->func = GetProcAddress(lib, cbuf);
		} else {
			(option.tool_info + i)->old_func = (OLD_TOOL_FUNC)GetProcAddress(lib, cbuf);
		}
		if ((option.tool_info + i)->func == NULL && (option.tool_info + i)->old_func == NULL) {
			message_get_error(GetLastError(), buf);
			wsprintf(err_str, TEXT("%s\r\n%s\r\n%s"), buf,
				(option.tool_info + i)->lib_file_path, (option.tool_info + i)->func_name);
			return FALSE;
		}
	}
	return TRUE;
}

/*
 * tool_data_copy - ツール用アイテムのコピーを作成
 */
TOOL_DATA_INFO *tool_data_copy(DATA_INFO *di, const BOOL next_copy)
{
	TOOL_DATA_INFO *tdi;

	if (di == NULL) {
		return NULL;
	}

	if ((tdi = (TOOL_DATA_INFO *)mem_calloc(sizeof(TOOL_DATA_INFO))) == NULL) {
		return NULL;
	}
	tdi->struct_size = sizeof(TOOL_DATA_INFO);
	tdi->di = di;
	if (di->child != NULL) {
		tdi->child = tool_data_copy(di->child, TRUE);
	}
	if (next_copy == TRUE && di->next != NULL) {
		tdi->next = tool_data_copy(di->next, TRUE);
	}
	return tdi;
}

/*
 * tool_data_free - ツール用アイテムの解放
 */
void tool_data_free(TOOL_DATA_INFO *tdi)
{
	TOOL_DATA_INFO *wk_tdi;

	while (tdi != NULL) {
		wk_tdi = tdi->next;
		if (tdi->child != NULL) {
			tool_data_free(tdi->child);
		}
		mem_free(&tdi);
		tdi = wk_tdi;
	}
}

/*
 * tool_data_reflect - ツール用アイテムを実体に反映
 */
static void tool_data_reflect(const int call_type, TOOL_DATA_INFO *tdi)
{
	DATA_INFO *di;

	for (; tdi != NULL; tdi = tdi->next) {
		di = tdi->di;
		if (((call_type & CALLTYPE_MENU) || (call_type & CALLTYPE_VIEWER)) &&
			(((call_type & CALLTYPE_HISTORY) && data_check(&history_data, di) == NULL) ||
			((call_type & CALLTYPE_REGIST) && data_check(&regist_data, di) == NULL))) {
			// リスト中にデータが見つからない
			continue;
		}
		data_menu_free_item(di);
		if (tdi->child != NULL) {
			tool_data_reflect(call_type, tdi->child);
		}
	}
}

/*
 * tool_call_func_old - 旧ツールの呼び出し
 */
static int tool_call_func_old(const HWND hWnd, const TOOL_INFO *ti, const int call_type, TOOL_DATA_INFO *tdi)
{
	DATA_INFO *cdi;
	TOOL_DATA_INFO *wk_tdi;
	CLIPINFO ci;
	int old_call_type;
	int i;
	int ret = TOOL_SUCCEED;

	if (ti->old_func == NULL) {
		return TOOL_ERROR;
	}

	if (call_type & CALLTYPE_MENU) {
		if (tdi == NULL) {
			return TOOL_SUCCEED;
		}
		old_call_type = OLD_CALLTYPE_MENU | OLD_CALLTYPE_ITEM_TO_CLIPBOARD;

	} else if (call_type & CALLTYPE_VIEWER) {
		if (tdi == NULL) {
			return TOOL_SUCCEED;
		}
		old_call_type = OLD_CALLTYPE_VIEWER;

	} else if (call_type & CALLTYPE_ADD_HISTORY) {
		if (tdi == NULL) {
			return TOOL_SUCCEED;
		}
		old_call_type = OLD_CALLTYPE_ADD_HISTORY;

	} else if (call_type & CALLTYPE_ITEM_TO_CLIPBOARD) {
		if (tdi == NULL) {
			return TOOL_SUCCEED;
		}
		old_call_type = OLD_CALLTYPE_ITEM_TO_CLIPBOARD;

	} else if (call_type & CALLTYPE_START) {
		old_call_type = OLD_CALLTYPE_START | OLD_CALLTYPE_NOITEM;

	} else if (call_type & CALLTYPE_END) {
		old_call_type = OLD_CALLTYPE_END | OLD_CALLTYPE_NOITEM;

	} else {
		return TOOL_SUCCEED;
	}

	if (tdi == NULL) {
		ti->func(hWnd, NULL, old_call_type, 0);
	} else {
		i = 0;
		for (; tdi != NULL; tdi = tdi->next) {
			if (tdi->di->type == TYPE_FOLDER || tdi->di->type == TYPE_ROOT) {
				continue;
			}
			wk_tdi = tdi;
			if (wk_tdi->di->type == TYPE_ITEM) {
				cdi = format_get_priority_highest(wk_tdi->di);
				for (wk_tdi = wk_tdi->child; wk_tdi != NULL && wk_tdi->di != cdi; wk_tdi = wk_tdi->next)
					;
				if (wk_tdi == NULL) {
					continue;
				}
			}

			ZeroMemory(&ci, sizeof(CLIPINFO));
			ci.Type = wk_tdi->di->format;
			lstrcpy(ci.name, wk_tdi->di->format_name);
			ci.Mem = wk_tdi->di->data;
			ci.Size = wk_tdi->di->size;
			ci.Flag = wk_tdi->di->plugin_param;

			if (ti->old_func(hWnd, (void *)&ci, old_call_type, i++) >= 0) {
				wk_tdi->di->data = ci.Mem;
				wk_tdi->di->size = ci.Size;
				wk_tdi->di->plugin_param = ci.Flag;
				ret |= TOOL_DATA_MODIFIED;
			}
		}
	}
	return ret;
}

/*
 * tool_execute - ツールの呼び出し
 */
int tool_execute(const HWND hWnd, TOOL_INFO *ti, const int call_type, DATA_INFO *di, TOOL_DATA_INFO *tdi)
{
	TOOL_DATA_INFO *wk_tdi = tdi;
	TOOL_EXEC_INFO *tei;
	int ctype = call_type;
	int ret = TOOL_ERROR;

	if (ti == NULL) {
		return TOOL_ERROR;
	}

	// ツール用アイテムの設定
	if (tdi == NULL) {
		wk_tdi = tool_data_copy(di, FALSE);
	}
	// フラグの設定
	if (wk_tdi != NULL && ((ctype & CALLTYPE_MENU) || (ctype & CALLTYPE_VIEWER))) {
		if (data_check(&history_data, wk_tdi->di) != NULL) {
			ctype |= CALLTYPE_HISTORY;
		} else if (data_check(&regist_data, wk_tdi->di) != NULL) {
			ctype |= CALLTYPE_REGIST;
		}
	}

	// 実行情報の設定
	if ((tei = mem_calloc(sizeof(TOOL_EXEC_INFO))) == NULL) {
		if (tdi == NULL) {
			tool_data_free(wk_tdi);
		}
		return TOOL_ERROR;
	}
	tei->struct_size = sizeof(TOOL_EXEC_INFO);
	tei->call_type = ctype;
	if (ti->old == 2) {
		tei->cmd_line = (TCHAR *)alloc_tchar_to_char(tei->cmd_line);
	} else {
		tei->cmd_line = alloc_copy(ti->cmd_line);
	}
	mem_free(&ti->cmd_line);
	tei->lParam = ti->lParam;

	if (ti->old == 0 || ti->old == 2) {
		if (ti->func != NULL) {
			// ツールの呼び出し
			ret = ti->func(hWnd, tei, wk_tdi);
		}
	} else {
		// 旧ツールの呼び出し
		ret = tool_call_func_old(hWnd, ti, ctype, wk_tdi);
	}

	if (ti->old == 2) {
		ti->cmd_line = alloc_char_to_tchar((char *)tei->cmd_line);
	} else {
		ti->cmd_line = alloc_copy(tei->cmd_line);
	}
	mem_free(&tei->cmd_line);
	ti->lParam = tei->lParam;
	mem_free(&tei);

	if (ret & TOOL_DATA_MODIFIED) {
		tool_data_reflect(ctype, wk_tdi);
	}
	if (tdi == NULL) {
		tool_data_free(wk_tdi);
	}
	return ret;
}

/*
 * tool_execute_all - 呼び出し方法にマッチするツールの実行
 */
int tool_execute_all(const HWND hWnd, const int call_type, DATA_INFO *di)
{
	int ret = TOOL_ERROR;
	int i;

	for (i = 0; i < option.tool_cnt; i++) {
		if ((option.tool_info + i)->call_type & call_type) {
			ret |= tool_execute(hWnd, option.tool_info + i, call_type, di, NULL);
			if (ret & TOOL_CANCEL) {
				return ret;
			}
		}
	}
	return ret;
}
/* End of source */
