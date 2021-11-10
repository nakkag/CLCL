/*
 * CLCL
 *
 * Regist.c
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
#include "File.h"
#include "Regist.h"
#include "ClipBoard.h"
#include "Format.h"

/* Define */

/* Global Variables */
// オプション
extern OPTION_INFO option;

/* Local Function Prototypes */

/*
 * regist_path_to_item - パスから登録アイテムを取得
 */
DATA_INFO *regist_path_to_item(DATA_INFO *di, TCHAR *path)
{
	TCHAR buf[BUF_SIZE];
	TCHAR *p, *r;

	if (path == NULL || path == TEXT('\0')) {
		return di;
	}
	if (di == NULL) {
		return NULL;
	}

	p = path;
	if (*p == TEXT('\\') || *p == TEXT('/')) {
		p++;
	}
	if (*p == TEXT('\0')) {
		return di;
	}

	// パスを展開
	for (r = buf; *p != TEXT('\0'); p++, r++) {
#ifndef UNICODE
		if (IsDBCSLeadByte((BYTE)*p) == TRUE) {
			*(r++) = *(p++);
			*r = *p;
			continue;
		}
#endif
		if (*p == TEXT('\\') || *p == TEXT('/')) {
			break;
		}
		*r = *p;
	}
	*r = TEXT('\0');

	// 名前からアイテムを取得
	for (; di != NULL; di = di->next) {
		if (di->type == TYPE_FOLDER && di->title != NULL &&
			lstrcmpi(di->title, buf) == 0) {
			break;
		}
	}
	if (di == NULL) {
		return NULL;
	}

	if (*p != TEXT('\0')) {
		// 再帰
		return regist_path_to_item(di->child, p);
	}
	return di->child;
}

/*
 * regist_create_folder - 登録アイテムにフォルダを追加
 */
DATA_INFO *regist_create_folder(DATA_INFO **root, const TCHAR *title, TCHAR *err_str)
{
	DATA_INFO *new_item;
	DATA_INFO *di;

	// 同名のフォルダはエラーとする
	for (di = *root; di != NULL; di = di->next) {
		if (di->type == TYPE_FOLDER && lstrcmpi(di->title, title) == 0) {
			return NULL;
		}
	}

	// フォルダの作成
	if ((new_item = data_create_folder(title, err_str)) == NULL) {
		return NULL;
	}
	// フォルダの追加
	if (*root == NULL) {
		new_item->next = *root;
		*root = new_item;
	} else {
		for (di = *root; di->next != NULL; di = di->next)
			;
		di->next = new_item;
	}
	return new_item;
}

/*
 * regist_merge_item - 登録アイテムのマージ
 */
DATA_INFO *regist_merge_item(DATA_INFO **root, const DATA_INFO *from_di, const BOOL move_flag, TCHAR *err_str)
{
	DATA_INFO *di, *wk_di;

	switch (from_di->type) {
	case TYPE_DATA:
	case TYPE_ITEM:
		if ((wk_di = data_item_copy(from_di, FALSE, move_flag, err_str)) == NULL) {
			return NULL;
		}
		// ウィンドウ名の除去
		mem_free(&wk_di->window_name);
		if (*root == NULL) {
			*root = wk_di;
			di = *root;
		} else {
			for (di = *root; di->next != NULL; di = di->next)
				;
			di->next = wk_di;
			di = di->next;
		}
		break;

	case TYPE_FOLDER:
		// フォルダの検索
		for (wk_di = *root; wk_di != NULL; wk_di = wk_di->next) {
			if (wk_di->type == TYPE_FOLDER && lstrcmpi(wk_di->title, from_di->title) == 0) {
				break;
			}
		}
		if (wk_di == NULL) {
			// フォルダの追加
			if ((wk_di = regist_create_folder(root, from_di->title, err_str)) == NULL) {
				return NULL;
			}
		}
		// アイテムをコピー
		for (di = from_di->child; di != NULL; di = di->next) {
			if (regist_merge_item(&wk_di->child, di, move_flag, err_str) == NULL) {
				break;
			}
		}
		di = wk_di;
		break;
	}
	return di;
}

/*
 * regist_regist_hotkey - ホットキーの登録
 */
BOOL regist_regist_hotkey(const HWND hWnd, DATA_INFO *di, int *id)
{
	DATA_INFO *cdi = di;
	BOOL ret = TRUE;

	for (; cdi != NULL; cdi = cdi->next) {
		switch (cdi->type) {
		case TYPE_FOLDER:
			if (regist_regist_hotkey(hWnd, cdi->child, id) == FALSE) {
				ret = FALSE;
			}
			break;

		case TYPE_ITEM:
			if (cdi->op_virtkey != 0) {
				if (RegisterHotKey(hWnd, *id,
					cdi->op_modifiers, cdi->op_virtkey) == FALSE) {
					ret = FALSE;
				} else {
					cdi->hkey_id = *id;
					(*id)++;
				}
			}
			break;

		case TYPE_DATA:
			break;
		}
	}
	return ret;
}

/*
 * regist_unregist_hotkey - ホットキーの解除
 */
void regist_unregist_hotkey(const HWND hWnd, DATA_INFO *di)
{
	DATA_INFO *cdi = di;

	for (; cdi != NULL; cdi = cdi->next) {
		switch (cdi->type) {
		case TYPE_FOLDER:
			regist_unregist_hotkey(hWnd, cdi->child);
			break;

		case TYPE_ITEM:
			if (cdi->hkey_id != 0) {
				UnregisterHotKey(hWnd, cdi->hkey_id);
				cdi->hkey_id = 0;
			}
			break;

		case TYPE_DATA:
			break;
		}
	}
}

/*
 * regist_hotkey_to_item - ホットキーIDからアイテムを検索
 */
DATA_INFO *regist_hotkey_to_item(DATA_INFO *di, const int id)
{
	DATA_INFO *cdi = di;
	DATA_INFO *ret;

	for (; cdi != NULL; cdi = cdi->next) {
		switch (cdi->type) {
		case TYPE_FOLDER:
			if ((ret = regist_hotkey_to_item(cdi->child, id)) != NULL) {
				return ret;
			}
			break;

		case TYPE_ITEM:
			if (cdi->hkey_id == id) {
				return cdi;
			}
			break;

		case TYPE_DATA:
			break;
		}
	}
	return NULL;
}
/* End of source */
