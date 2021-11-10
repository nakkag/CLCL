/*
 * CLCL
 *
 * Format.c
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

/* Define */

/* Global Variables */
// オプション
extern OPTION_INFO option;

/* Local Function Prototypes */

/*
 * format_get_index - 形式情報のインデックスを取得
 */
int format_get_index(const TCHAR *format_name, const int name_hash)
{
	TCHAR *buf;
	int hash;
	int i, j;

	if (format_name == NULL) {
		return -1;
	}

	if ((buf = alloc_copy(format_name)) == NULL) {
		return -1;
	}
	Trim(buf);

	hash = (name_hash == 0) ? str2hash(buf) : name_hash;

	for (i = 0; i < option.format_cnt; i++) {
		if ((option.format_info + i)->fn == NULL) {
			continue;
		}
		for (j = 0; j < (option.format_info + i)->fn_cnt; j++) {
			if (hash == ((option.format_info + i)->fn + j)->format_name_hash &&
				lstrcmpi(buf, ((option.format_info + i)->fn + j)->format_name) == 0) {
				mem_free(&buf);
				return i;
			}
		}
	}
	mem_free(&buf);
	return -1;
}

/*
 * format_get_priority_highest - 優先順位が一番高いデータを取得
 */
DATA_INFO *format_get_priority_highest(DATA_INFO *di)
{
	DATA_INFO *pdi;
	DATA_INFO *ret = di->child;
	int priority = -1;
	int i;

	if (di == NULL || di->type == TYPE_FOLDER || di->child == NULL) {
		return di;
	}
	for (pdi = di->child; pdi != NULL; pdi = pdi->next) {
		i = format_get_index(pdi->format_name, pdi->format_name_hash);
		if (i != -1 && (i < priority || priority == -1)) {
			priority = i;
			ret = pdi;
		}
	}
	return ret;
}

/*
 * format_initialize - 形式情報の初期化
 */
BOOL format_initialize(TCHAR *err_str)
{
	TCHAR buf[BUF_SIZE];
	char cbuf[BUF_SIZE];
	HANDLE lib;
	int i;

	for (i = 0; i < option.format_cnt; i++) {
		// モジュールハンドル取得
		if ((option.format_info + i)->lib_file_path != NULL &&
			*(option.format_info + i)->lib_file_path != TEXT('\0')) {
			lib = (option.format_info + i)->lib = LoadLibrary((option.format_info + i)->lib_file_path);
			if (lib == NULL) {
				message_get_error(GetLastError(), buf);
				wsprintf(err_str, TEXT("%s\r\n%s"), buf, (option.format_info + i)->lib_file_path);
				return FALSE;
			}
		} else {
			// 本体
			lib = GetModuleHandle(NULL);
		}
		// 形式毎の関数アドレス取得
		// general
		wsprintf(buf, TEXT("%sinitialize"), (option.format_info + i)->func_header);
		tchar_to_char(buf, cbuf, BUF_SIZE - 1);
		(option.format_info + i)->func_initialize = GetProcAddress(lib, cbuf);

		wsprintf(buf, TEXT("%sget_icon"), (option.format_info + i)->func_header);
		tchar_to_char(buf, cbuf, BUF_SIZE - 1);
		(option.format_info + i)->func_get_icon = GetProcAddress(lib, cbuf);

		wsprintf(buf, TEXT("%sfree"), (option.format_info + i)->func_header);
		tchar_to_char(buf, cbuf, BUF_SIZE - 1);
		(option.format_info + i)->func_free = GetProcAddress(lib, cbuf);

		// item
		wsprintf(buf, TEXT("%sinitialize_item"), (option.format_info + i)->func_header);
		tchar_to_char(buf, cbuf, BUF_SIZE - 1);
		(option.format_info + i)->func_initialize_item = GetProcAddress(lib, cbuf);

		wsprintf(buf, TEXT("%scopy_data"), (option.format_info + i)->func_header);
		tchar_to_char(buf, cbuf, BUF_SIZE - 1);
		(option.format_info + i)->func_copy_data = GetProcAddress(lib, cbuf);

		wsprintf(buf, TEXT("%sdata_to_bytes"), (option.format_info + i)->func_header);
		tchar_to_char(buf, cbuf, BUF_SIZE - 1);
		(option.format_info + i)->func_data_to_bytes = GetProcAddress(lib, cbuf);

		wsprintf(buf, TEXT("%sbytes_to_data"), (option.format_info + i)->func_header);
		tchar_to_char(buf, cbuf, BUF_SIZE - 1);
		(option.format_info + i)->func_bytes_to_data = GetProcAddress(lib, cbuf);

		wsprintf(buf, TEXT("%sget_file_info"), (option.format_info + i)->func_header);
		tchar_to_char(buf, cbuf, BUF_SIZE - 1);
		(option.format_info + i)->func_get_file_info = GetProcAddress(lib, cbuf);

		wsprintf(buf, TEXT("%sdata_to_file"), (option.format_info + i)->func_header);
		tchar_to_char(buf, cbuf, BUF_SIZE - 1);
		(option.format_info + i)->func_data_to_file = GetProcAddress(lib, cbuf);

		wsprintf(buf, TEXT("%sfile_to_data"), (option.format_info + i)->func_header);
		tchar_to_char(buf, cbuf, BUF_SIZE - 1);
		(option.format_info + i)->func_file_to_data = GetProcAddress(lib, cbuf);

		wsprintf(buf, TEXT("%sfree_data"), (option.format_info + i)->func_header);
		tchar_to_char(buf, cbuf, BUF_SIZE - 1);
		(option.format_info + i)->func_free_data = GetProcAddress(lib, cbuf);

		wsprintf(buf, TEXT("%sfree_item"), (option.format_info + i)->func_header);
		tchar_to_char(buf, cbuf, BUF_SIZE - 1);
		(option.format_info + i)->func_free_item = GetProcAddress(lib, cbuf);

		// menu
		wsprintf(buf, TEXT("%sget_menu_title"), (option.format_info + i)->func_header);
		tchar_to_char(buf, cbuf, BUF_SIZE - 1);
		(option.format_info + i)->func_get_menu_title = GetProcAddress(lib, cbuf);

		wsprintf(buf, TEXT("%sget_menu_icon"), (option.format_info + i)->func_header);
		tchar_to_char(buf, cbuf, BUF_SIZE - 1);
		(option.format_info + i)->func_get_menu_icon = GetProcAddress(lib, cbuf);

		wsprintf(buf, TEXT("%sget_menu_bitmap"), (option.format_info + i)->func_header);
		tchar_to_char(buf, cbuf, BUF_SIZE - 1);
		(option.format_info + i)->func_get_menu_bitmap = GetProcAddress(lib, cbuf);

		wsprintf(buf, TEXT("%sget_tooltip_text"), (option.format_info + i)->func_header);
		tchar_to_char(buf, cbuf, BUF_SIZE - 1);
		(option.format_info + i)->func_get_tooltip_text = GetProcAddress(lib, cbuf);

		// window
		wsprintf(buf, TEXT("%swindow_create"), (option.format_info + i)->func_header);
		tchar_to_char(buf, cbuf, BUF_SIZE - 1);
		(option.format_info + i)->func_window_create = GetProcAddress(lib, cbuf);

		wsprintf(buf, TEXT("%swindow_destroy"), (option.format_info + i)->func_header);
		tchar_to_char(buf, cbuf, BUF_SIZE - 1);
		(option.format_info + i)->func_window_destroy = GetProcAddress(lib, cbuf);

		wsprintf(buf, TEXT("%swindow_show_data"), (option.format_info + i)->func_header);
		tchar_to_char(buf, cbuf, BUF_SIZE - 1);
		(option.format_info + i)->func_window_show_data = GetProcAddress(lib, cbuf);

		wsprintf(buf, TEXT("%swindow_save_data"), (option.format_info + i)->func_header);
		tchar_to_char(buf, cbuf, BUF_SIZE - 1);
		(option.format_info + i)->func_window_save_data = GetProcAddress(lib, cbuf);

		wsprintf(buf, TEXT("%swindow_hide_data"), (option.format_info + i)->func_header);
		tchar_to_char(buf, cbuf, BUF_SIZE - 1);
		(option.format_info + i)->func_window_hide_data = GetProcAddress(lib, cbuf);

		// 初期化
		if ((option.format_info + i)->func_initialize != NULL) {
			((option.format_info + i)->func_initialize)();
		}
	}
	return TRUE;
}

/*
 * format_get_icon - 形式用のアイコンを取得
 */
HICON format_get_icon(const int index, const int icon_size, BOOL *free_icon)
{
	if (index < 0 || (option.format_info + index)->func_get_icon == NULL) {
		return NULL;
	}
	return (HICON)((option.format_info + index)->func_get_icon)(icon_size, free_icon);
}

/*
 * format_free - 形式情報の解放
 */
BOOL format_free(void)
{
	int i;

	for (i = 0; i < option.format_cnt; i++) {
		if ((option.format_info + i)->func_free != NULL) {
			((option.format_info + i)->func_free)();
		}
	}
	return TRUE;
}

/*
 * format_initialize_item - 形式毎のアイテムの初期化
 */
BOOL format_initialize_item(DATA_INFO *di, const BOOL set_init_data)
{
	int i;

	i = format_get_index(di->format_name, di->format_name_hash);
	if (i == -1 || (option.format_info + i)->func_initialize_item == NULL) {
		return FALSE;
	}
	return (BOOL)((option.format_info + i)->func_initialize_item)(di, set_init_data);
}

/*
 * format_copy_data - 形式毎のデータコピー
 */
HANDLE format_copy_data(const TCHAR *format_name, const HANDLE data, DWORD *ret_size)
{
	int i;

	i = format_get_index(format_name, 0);
	if (i == -1 || (option.format_info + i)->func_copy_data == NULL) {
		return NULL;
	}
	return (HANDLE)((option.format_info + i)->func_copy_data)(format_name, data, ret_size);
}

/*
 * format_data_to_bytes - データをバイト列に変換
 */
BYTE *format_data_to_bytes(const DATA_INFO *di, DWORD *ret_size)
{
	int i;

	i = format_get_index(di->format_name, di->format_name_hash);
	if (i == -1 || (option.format_info + i)->func_data_to_bytes == NULL) {
		return NULL;
	}
	return (BYTE *)((option.format_info + i)->func_data_to_bytes)(di, ret_size);
}

/*
 * format_bytes_to_data - バイト列をデータに変換
 */
HANDLE format_bytes_to_data(const TCHAR *format_name, const BYTE *data, DWORD *size)
{
	int i;

	i = format_get_index(format_name, 0);
	if (i == -1 || (option.format_info + i)->func_bytes_to_data == NULL) {
		return NULL;
	}
	return (HANDLE)((option.format_info + i)->func_bytes_to_data)(format_name, data, size);
}

/*
 * format_get_file_info - 形式毎のコモンダイアログ情報の取得 (mode = TRUE-open FALSE-save)
 *
 *	戻り値: -1 - コモンダイアログを表示しない
 *			0  - 未設定
 *			1  - 設定済み
 */
int format_get_file_info(const TCHAR *format_name, const DATA_INFO *di, OPENFILENAME *of, const BOOL mode)
{
	int i;

	i = format_get_index(format_name, 0);
	if (i == -1 || (option.format_info + i)->func_get_file_info == NULL) {
		return FALSE;
	}
	return (int)((option.format_info + i)->func_get_file_info)(format_name, di, of, mode);
}

/*
 * format_data_to_file - データをファイルに保存
 */
BOOL format_data_to_file(DATA_INFO *di, const TCHAR *file_name, const int filter_index, TCHAR *err_str)
{
	int i;

	i = format_get_index(di->format_name, di->format_name_hash);
	if (i == -1 || (option.format_info + i)->func_data_to_file == NULL) {
		return FALSE;
	}
	return (BOOL)((option.format_info + i)->func_data_to_file)(di, file_name, filter_index, err_str);
}

/*
 * format_file_to_data - ファイルからデータを作成
 */
HANDLE format_file_to_data(const TCHAR *file_name, const TCHAR *format_name, DWORD *ret_size, TCHAR *err_str)
{
	int i;

	i = format_get_index(format_name, 0);
	if (i == -1 || (option.format_info + i)->func_file_to_data == NULL) {
		return NULL;
	}
	return (HANDLE)((option.format_info + i)->func_file_to_data)(file_name, format_name, ret_size, err_str);
}

/*
 * format_free_data - 形式毎のデータを解放
 */
BOOL format_free_data(const TCHAR *format_name, HANDLE data)
{
	int i;

	i = format_get_index(format_name, 0);
	if (i == -1 || (option.format_info + i)->func_free_data == NULL) {
		return FALSE;
	}
	return (BOOL)((option.format_info + i)->func_free_data)(format_name, data);
}

/*
 * format_free_item - 形式毎のアイテム情報を解放
 */
BOOL format_free_item(DATA_INFO *di)
{
	int i;

	i = format_get_index(di->format_name, di->format_name_hash);
	if (i == -1 || (option.format_info + i)->func_free_item == NULL) {
		return FALSE;
	}
	return (BOOL)((option.format_info + i)->func_free_item)(di);
}

/*
 * format_get_menu_title - 形式毎のメニュータイトルを取得
 */
BOOL format_get_menu_title(DATA_INFO *di)
{
	int i;

	if (di->menu_title != NULL) {
		return TRUE;
	}
	i = format_get_index(di->format_name, di->format_name_hash);
	if (i == -1 || (option.format_info + i)->func_get_menu_title == NULL) {
		return FALSE;
	}
	return (BOOL)((option.format_info + i)->func_get_menu_title)(di);
}

/*
 * format_get_menu_icon - 形式毎のメニュー用アイコンを取得
 */
BOOL format_get_menu_icon(DATA_INFO *di)
{
	int i;

	if (di->menu_icon != NULL) {
		return TRUE;
	}
	i = format_get_index(di->format_name, di->format_name_hash);
	if (i == -1 || (option.format_info + i)->func_get_menu_icon == NULL) {
		return FALSE;
	}
	return (BOOL)((option.format_info + i)->func_get_menu_icon)(di, option.menu_icon_size);
}

/*
 * format_get_menu_bitmap - 形式毎のメニュー用ビットマップを取得
 */
BOOL format_get_menu_bitmap(DATA_INFO *di)
{
	int i;

	if (di->menu_bitmap != NULL) {
		return TRUE;
	}
	i = format_get_index(di->format_name, di->format_name_hash);
	if (i == -1 || (option.format_info + i)->func_get_menu_bitmap == NULL) {
		return FALSE;
	}
	return (BOOL)((option.format_info + i)->func_get_menu_bitmap)(di, option.menu_bitmap_width, option.menu_bitmap_height);
}

/*
 * format_get_tooltip_text - 形式毎のメニュー用ツールチップテキストを取得
 */
TCHAR *format_get_tooltip_text(DATA_INFO *di)
{
	int i;

	i = format_get_index(di->format_name, di->format_name_hash);
	if (i == -1 || (option.format_info + i)->func_get_tooltip_text == NULL) {
		return FALSE;
	}
	return (TCHAR *)((option.format_info + i)->func_get_tooltip_text)(di);
}

/*
 * format_window_create - 形式毎のデータ表示ウィンドウの作成
 */
BOOL format_window_create(const HWND parent_wnd)
{
	int i;

	for (i = 0; i < option.format_cnt; i++) {
		if ((option.format_info + i)->func_window_create != NULL) {
			(option.format_info + i)->hWnd = (HWND)((option.format_info + i)->func_window_create)(parent_wnd);
		}
	}
	return TRUE;
}

/*
 * format_window_destroy - 形式毎のデータ表示ウィンドウの破棄
 */
BOOL format_window_destroy(void)
{
	int i;

	for (i = 0; i < option.format_cnt; i++) {
		if ((option.format_info + i)->func_window_destroy != NULL) {
			((option.format_info + i)->func_window_destroy)((option.format_info + i)->hWnd);
		}
	}
	return TRUE;
}

/*
 * format_window_show_data - 形式毎のデータを表示
 */
BOOL format_window_show_data(const HWND hWnd, DATA_INFO *di, const BOOL lock)
{
	int i;

	i = format_get_index(di->format_name, di->format_name_hash);
	if (i == -1 || (option.format_info + i)->func_window_show_data == NULL) {
		return FALSE;
	}
	return (BOOL)((option.format_info + i)->func_window_show_data)(hWnd, di, lock);
}

/*
 * format_window_save_data - 形式毎のデータを保存
 */
BOOL format_window_save_data(const HWND hWnd, DATA_INFO *di)
{
	int i;

	i = format_get_index(di->format_name, di->format_name_hash);
	if (i == -1 || (option.format_info + i)->func_window_save_data == NULL) {
		return FALSE;
	}
	return (BOOL)((option.format_info + i)->func_window_save_data)(hWnd, di);
}

/*
 * format_window_hide_data - 形式毎のデータを非表示
 */
BOOL format_window_hide_data(const HWND hWnd, DATA_INFO *di)
{
	int i;

	i = format_get_index(di->format_name, di->format_name_hash);
	if (i == -1 || (option.format_info + i)->func_window_hide_data == NULL) {
		return FALSE;
	}
	return (BOOL)((option.format_info + i)->func_window_hide_data)(hWnd, di);
}
/* End of source */
