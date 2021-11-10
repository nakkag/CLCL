/*
 * CLCL
 *
 * Data.c
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
#include "ClipBoard.h"
#include "Format.h"

/* Define */

/* Global Variables */
// オプション
extern OPTION_INFO option;

/* Local Function Prototypes */

/*
 * data_create_data - データの作成
 */
DATA_INFO *data_create_data(const UINT format, TCHAR *format_name, const HANDLE data, const DWORD size, const BOOL init, TCHAR *err_str)
{
	DATA_INFO *new_item;

	// アイテムの確保
	if ((new_item = (DATA_INFO *)mem_calloc(sizeof(DATA_INFO))) == NULL) {
		message_get_error(GetLastError(), err_str);
		return NULL;
	}
	new_item->struct_size = sizeof(DATA_INFO);
	new_item->type = TYPE_DATA;
	new_item->format = (format != 0) ? format : clipboard_get_format(0, format_name);
	new_item->format_name = alloc_copy(format_name);
	new_item->format_name_hash = str2hash(new_item->format_name);
	new_item->data = data;
	new_item->size = size;
	format_initialize_item(new_item, (data == NULL) ? init : FALSE);
	return new_item;
}

/*
 * data_create_item - アイテムの作成
 */
DATA_INFO *data_create_item(const TCHAR *title, const BOOL set_date, TCHAR *err_str)
{
	DATA_INFO *new_item;

	// アイテムの確保
	if ((new_item = (DATA_INFO *)mem_calloc(sizeof(DATA_INFO))) == NULL) {
		message_get_error(GetLastError(), err_str);
		return NULL;
	}
	new_item->struct_size = sizeof(DATA_INFO);
	new_item->type = TYPE_ITEM;
	new_item->title = alloc_copy(title);
	if (set_date == TRUE) {
		data_set_modified(new_item);
	}
	return new_item;
}

/*
 * data_create_folder - フォルダの作成
 */
DATA_INFO *data_create_folder(const TCHAR *title, TCHAR *err_str)
{
	DATA_INFO *new_item;

	// アイテムの確保
	if ((new_item = (DATA_INFO *)mem_calloc(sizeof(DATA_INFO))) == NULL) {
		message_get_error(GetLastError(), err_str);
		return NULL;
	}
	new_item->struct_size = sizeof(DATA_INFO);
	new_item->title = alloc_copy(title);
	new_item->type = TYPE_FOLDER;
	return new_item;
}

/*
 * data_item_copy - アイテムのコピーを作成
 */
DATA_INFO *data_item_copy(const DATA_INFO *di, const BOOL next_copy, const BOOL move_flag, TCHAR *err_str)
{
	DATA_INFO *new_di;

	if (di == NULL) {
		return NULL;
	}
	if ((new_di = (DATA_INFO *)mem_calloc(sizeof(DATA_INFO))) == NULL) {
		message_get_error(GetLastError(), err_str);
		return NULL;
	}
	new_di->struct_size = sizeof(DATA_INFO);
	new_di->type = di->type;
	new_di->title = alloc_copy(di->title);
	new_di->format_name = alloc_copy(di->format_name);
	new_di->format_name_hash = di->format_name_hash;
	new_di->format = di->format;
	new_di->modified.dwLowDateTime = di->modified.dwLowDateTime;
	new_di->modified.dwHighDateTime = di->modified.dwHighDateTime;
	new_di->window_name = alloc_copy(di->window_name);
	new_di->plugin_string = alloc_copy(di->plugin_string);
	new_di->plugin_param = di->plugin_param;
	if (move_flag == TRUE) {
		new_di->hkey_id = di->hkey_id;
		new_di->op_modifiers = di->op_modifiers;
		new_di->op_virtkey = di->op_virtkey;
		new_di->op_paste = di->op_paste;
	}
	// データのコピー
	if (di->data != NULL && (new_di->data = format_copy_data(di->format_name, di->data, &new_di->size)) == NULL) {
		new_di->data = clipboard_copy_data(di->format, di->data, &new_di->size);
	}

	// 子アイテムのコピー
	if (di->child != NULL && (new_di->child = data_item_copy(di->child, TRUE, move_flag, err_str)) == NULL) {
		data_free(new_di);
		return NULL;
	}
	// 次アイテムのコピー
	if (next_copy == TRUE && di->next != NULL &&
		(new_di->next = data_item_copy(di->next, TRUE, move_flag, err_str)) == NULL) {
		data_free(new_di);
		return NULL;
	}
	return new_di;
}

/*
 * data_delete - アイテムの削除
 */
BOOL data_delete(DATA_INFO **root, DATA_INFO *del_di, const BOOL free_item)
{
	DATA_INFO *di;

	if (root == NULL || *root == NULL || del_di == NULL) {
		return FALSE;
	}
	if (*root == del_di) {
		*root = del_di->next;
		del_di->next = NULL;
		if (free_item == TRUE) {
			data_free(del_di);
		}
		return TRUE;
	}
	for (di = *root; di != NULL; di = di->next) {
		if (di->next == del_di) {
			// 削除
			di->next = del_di->next;
			del_di->next = NULL;
			if (free_item == TRUE) {
				data_free(del_di);
			}
			return TRUE;
		}
		if (di->child != NULL && data_delete(&di->child, del_di, free_item) == TRUE) {
			return TRUE;
		}
	}
	return FALSE;
}

/*
 * data_adjust - アイテムの整理
 */
void data_adjust(DATA_INFO **root)
{
	DATA_INFO *di = *root;
	DATA_INFO *wk_di;

	while (di != NULL) {
		if (di->type == TYPE_ITEM && di->child == NULL) {
			wk_di = di->next;
			// 削除
			data_delete(root, di, TRUE);
			di = wk_di;
		} else {
			if (di->type == TYPE_FOLDER) {
				data_adjust(&di->child);
			}
			di = di->next;
		}
	}
}

/*
 * data_menu_free - アイテムに関連付けられたメニュー情報を解放
 */
void data_menu_free_item(DATA_INFO *di)
{
	// テキストの解放
	if (di->free_title == TRUE) {
		mem_free(&di->menu_title);
	}
	di->menu_title = NULL;
	di->free_title = FALSE;

	// アイコンの解放
	if (di->free_icon == TRUE && di->menu_icon != NULL) {
		DestroyIcon(di->menu_icon);
	}
	di->menu_icon = NULL;
	di->free_icon = FALSE;

	// ビットマップの解放
	if (di->free_bitmap == TRUE && di->menu_bitmap != NULL) {
		DeleteObject((HGDIOBJ)di->menu_bitmap);
	}
	di->menu_bitmap = NULL;
	di->free_bitmap = FALSE;
	di->menu_bmp_width = 0;
	di->menu_bmp_height = 0;
}
void data_menu_free(DATA_INFO *di)
{
	for (; di != NULL; di = di->next) {
		data_menu_free_item(di);
		if (di->child != NULL) {
			data_menu_free(di->child);
		}
	}
}

/*
 * data_free - アイテムの解放
 */
void data_free(DATA_INFO *di)
{
	DATA_INFO *wk_di;

	while (di != NULL) {
		wk_di = di->next;

		if (di->child != NULL) {
			data_free(di->child);
		}

		format_free_item(di);
		if (di->data != NULL && format_free_data(di->format_name, di->data) == FALSE) {
			clipboard_free_data(di->format_name, di->data);
		}
		data_menu_free_item(di);
		mem_free(&di->title);
		mem_free(&di->format_name);
		mem_free(&di->window_name);
		mem_free(&di->plugin_string);
		mem_free(&di);

		di = wk_di;
	}
}

/*
 * data_check - アイテムの存在チェック
 */
DATA_INFO *data_check(DATA_INFO *di, const DATA_INFO *check_di)
{
	DATA_INFO *cdi;
	DATA_INFO *ret_di;

	if (di == NULL || di->child == NULL) {
		return NULL;
	}
	for (cdi = di->child; cdi != NULL; cdi = cdi->next) {
		if (cdi == check_di) {
			return di;
		}
		if ((ret_di = data_check(cdi, check_di)) != NULL) {
			return ret_di;
		}
	}
	return NULL;
}

/*
 * data_set_modified - 更新日時を設定
 */
void data_set_modified(DATA_INFO *di)
{
	SYSTEMTIME sys_time;

	if (di->type != TYPE_ITEM) {
		ZeroMemory(&di->modified, sizeof(FILETIME));
		return;
	}
	GetLocalTime(&sys_time);
	SystemTimeToFileTime(&sys_time, &di->modified);
}

/*
 * data_get_modified_string - 更新日時文字列を取得
 */
BOOL data_get_modified_string(const DATA_INFO *di, TCHAR *ret)
{
	SYSTEMTIME sys_time;
	TCHAR str_day[BUF_SIZE], str_time[BUF_SIZE];
	TCHAR *p;

	if (di->type != TYPE_ITEM ||
		(di->modified.dwLowDateTime == 0 && di->modified.dwHighDateTime == 0)) {
		*ret = TEXT('\0');
		return FALSE;
	}
	// ファイルタイムをシステムタイムに変換
	if (FileTimeToSystemTime(&di->modified, &sys_time) == FALSE) {
		*ret = TEXT('\0');
		return FALSE;
	}
	// 日付文字列の取得
	p = option.data_date_format;
	if (p == NULL || *p == TEXT('\0')) {
		p = NULL;
	}
	GetDateFormat(0, 0, &sys_time, p, str_day, BUF_SIZE - 1);
	// 時間文字列の取得
	p = option.data_time_format;
	if (p == NULL || *p == TEXT('\0')) {
		p = NULL;
	}
	GetTimeFormat(0, 0, &sys_time, p, str_time, BUF_SIZE - 1);

	wsprintf(ret, TEXT("%s %s"), str_day, str_time);
	return TRUE;
}

/*
 * data_get_title - アイテムのタイトルを取得
 */
TCHAR *data_get_title(DATA_INFO *di)
{
	DATA_INFO *wk_di;
	static TCHAR buf[BUF_SIZE];
	TCHAR *p;
	TCHAR *ret;

	wk_di = format_get_priority_highest(di);
	format_get_menu_title(wk_di);

	if (di->title != NULL) {
		ret = di->title;
	} else if (wk_di->menu_title != NULL) {
		ret = wk_di->menu_title;
	} else if (wk_di->format_name != NULL) {
		p = buf;
		*(p++) = TEXT('(');
		lstrcpyn(p, wk_di->format_name, BUF_SIZE - 3);
		p += lstrlen(p);
		*(p++) = TEXT(')');
		*(p++) = TEXT('\0');
		ret = buf;
	} else {
		ret = TEXT("");
	}
	return ret;
}
/* End of source */
