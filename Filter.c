/*
 * CLCL
 *
 * Filter.c
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
#include "Filter.h"

/* Define */

/* Global Variables */
// オプション
extern OPTION_INFO option;

/* Local Function Prototypes */
static BOOL filter_save_check(const TCHAR *format_name);
static DATA_INFO *filter_item_copy(const DATA_INFO *di, TCHAR *err_str);

/*
 * filter_get_index - フィルタ情報のインデックスを取得
 */
int filter_get_index(const TCHAR *format_name, const int name_hash)
{
	TCHAR *buf;
	int hash;
	int i, j;

	if (format_name == NULL || (buf = alloc_copy(format_name)) == NULL) {
		return -1;
	}
	Trim(buf);
	hash = (name_hash == 0) ? str2hash(buf) : name_hash;
	for (i = 0; i < option.filter_cnt; i++) {
		if ((option.filter_info + i)->fn == NULL) {
			continue;
		}
		for (j = 0; j < (option.filter_info + i)->fn_cnt; j++) {
			if (hash == ((option.filter_info + i)->fn + j)->format_name_hash &&
				lstrcmpi(buf, ((option.filter_info + i)->fn + j)->format_name) == 0) {
				mem_free(&buf);
				return i;
			}
		}
	}
	mem_free(&buf);
	return -1;
}

/*
 * filter_format_check - 形式のチェック
 */
BOOL filter_format_check(const TCHAR *format_name)
{
	int i;

	if ((i = filter_get_index(format_name, 0)) == -1) {
		return ((option.filter_all_action == FILTER_ACTION_ADD) ? TRUE : FALSE);
	}
	return (((option.filter_info + i)->action == FILTER_ACTION_ADD) ? TRUE : FALSE);
}

/*
 * filter_size_check - サイズのチェック
 */
BOOL filter_size_check(const TCHAR *format_name, const DWORD size)
{
	int i;

	if ((i = filter_get_index(format_name, 0)) == -1) {
		return TRUE;
	}
	if ((option.filter_info + i)->limit_size <= 0) {
		return TRUE;
	}
	if (size > (option.filter_info + i)->limit_size) {
		return FALSE;
	}
	return TRUE;
}

/*
 * filter_save_check - 形式の保存チェック
 */
static BOOL filter_save_check(const TCHAR *format_name)
{
	int i;

	if ((i = filter_get_index(format_name, 0)) == -1) {
		return ((option.filter_all_action == FILTER_ACTION_ADD) ? TRUE : FALSE);
	}
	return (((option.filter_info + i)->action == FILTER_ACTION_IGNORE ||
		(option.filter_info + i)->save == FILTER_SAVE_NOSAVE) ? FALSE : TRUE);
}

/*
 * filter_list_save_check - 保存フィルタが有効がチェック
 */
BOOL filter_list_save_check(DATA_INFO *di)
{
	DATA_INFO *cdi;

	for (; di != NULL; di = di->next) {
		switch (di->type) {
		case TYPE_FOLDER:
			if (filter_list_save_check(di->child) == TRUE) {
				return TRUE;
			}
			break;

		case TYPE_ITEM:
			for (cdi = di->child; cdi != NULL; cdi = cdi->next) {
				// 保存フィルタのチェック
				if (filter_save_check(cdi->format_name) == FALSE) {
					return TRUE;
				}
			}
			break;
		}
	}
	return FALSE;
}

/*
 * filter_item_copy - フィルタをかけてアイテムのコピー
 */
static DATA_INFO *filter_item_copy(const DATA_INFO *di, TCHAR *err_str)
{
	DATA_INFO *new_item = NULL;
	DATA_INFO *copy_di;
	DATA_INFO *cdi;
	DATA_INFO *wk_di;

	switch (di->type) {
	case TYPE_FOLDER:
		if ((new_item = data_create_folder(di->title, err_str)) == NULL) {
			return NULL;
		}
		if ((new_item->child = filter_list_copy(di->child, err_str)) == NULL && *err_str != TEXT('\0')) {
			data_free(new_item);
			return NULL;
		}
		break;

	case TYPE_ITEM:
		if ((new_item = data_create_item(di->title, FALSE, err_str)) == NULL) {
			return NULL;
		}
		new_item->modified.dwLowDateTime = di->modified.dwLowDateTime;
		new_item->modified.dwHighDateTime = di->modified.dwHighDateTime;
		new_item->window_name = alloc_copy(di->window_name);
		new_item->plugin_string = alloc_copy(di->plugin_string);
		new_item->plugin_param = di->plugin_param;

		for (cdi = di->child; cdi != NULL; cdi = cdi->next) {
			// 保存フィルタのチェック
			if (filter_save_check(cdi->format_name) == TRUE) {
				if ((copy_di = data_item_copy(cdi, FALSE, FALSE, err_str)) == NULL) {
					data_free(new_item);
					return NULL;
				}
				if (new_item->child == NULL) {
					new_item->child = copy_di;
				} else {
					for (wk_di = new_item->child; wk_di->next != NULL; wk_di = wk_di->next)
						;
					wk_di->next = copy_di;
				}
			}
		}
		if (new_item->child == NULL) {
			data_free(new_item);
			return NULL;
		}
		break;
	}
	return new_item;
}

/*
 * filter_list_copy - フィルタをかけてアイテムリストのコピー
 */
DATA_INFO *filter_list_copy(DATA_INFO *di, TCHAR *err_str)
{
	DATA_INFO *root_item = NULL;
	DATA_INFO *new_item;
	DATA_INFO *cdi;

	*err_str = TEXT('\0');
	for (; di != NULL; di = di->next) {
		if ((new_item = filter_item_copy(di, err_str)) == NULL && *err_str != TEXT('\0')) {
			data_free(root_item);
			return NULL;
		}
		if (new_item != NULL) {
			if (root_item == NULL) {
				root_item = new_item;
			} else {
				cdi->next = new_item;
			}
			cdi = new_item;
		}
	}
	return root_item;
}
/* End of source */
