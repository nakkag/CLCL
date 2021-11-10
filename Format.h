/*
 * CLCL
 *
 * Format.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_FORMAT_H
#define _INC_FORMAT_H

/* Include Files */
#include "Data.h"

/* Define */

/* Struct */
// 形式名情報
typedef struct _FORMAT_NAME {
	TCHAR *format_name;
	int format_name_hash;
} FORMAT_NAME;

// 形式情報
typedef struct _FORMAT_INFO {
	TCHAR *format_name;

	FORMAT_NAME *fn;
	int fn_cnt;

	TCHAR *lib_file_path;
	TCHAR *func_header;

	HANDLE lib;

	// general
	FARPROC func_initialize;
	FARPROC func_get_icon;
	FARPROC func_free;

	// item
	FARPROC func_initialize_item;
	FARPROC func_copy_data;
	FARPROC func_data_to_bytes;
	FARPROC func_bytes_to_data;
	FARPROC func_get_file_info;
	FARPROC func_data_to_file;
	FARPROC func_file_to_data;
	FARPROC func_free_data;
	FARPROC func_free_item;

	// menu
	FARPROC func_get_menu_title;
	FARPROC func_get_menu_icon;
	FARPROC func_get_menu_bitmap;
	FARPROC func_get_tooltip_text;

	// window
	HWND hWnd;
	FARPROC func_window_create;
	FARPROC func_window_destroy;
	FARPROC func_window_show_data;
	FARPROC func_window_save_data;
	FARPROC func_window_hide_data;
} FORMAT_INFO;

// 形式取得情報
typedef struct _FORMAT_GET_INFO {
	DWORD struct_size;					// 構造体のサイズ

	TCHAR format_name[BUF_SIZE];
	TCHAR func_header[BUF_SIZE];
	TCHAR comment[BUF_SIZE];
} FORMAT_GET_INFO;

/* Function Prototypes */
int format_get_index(const TCHAR *format_name, const int name_hash);
DATA_INFO *format_get_priority_highest(DATA_INFO *di);

// general
BOOL format_initialize(TCHAR *err_str);
HICON format_get_icon(const int index, const int icon_size, BOOL *free_icon);
BOOL format_free(void);

// item
BOOL format_initialize_item(DATA_INFO *di, const BOOL set_init_data);
HANDLE format_copy_data(const TCHAR *format_name, const HANDLE data, DWORD *ret_size);
BYTE *format_data_to_bytes(const DATA_INFO *di, DWORD *ret_size);
HANDLE format_bytes_to_data(const TCHAR *format_name, const BYTE *data, DWORD *size);
int format_get_file_info(const TCHAR *format_name, const DATA_INFO *di, OPENFILENAME *of, const BOOL mode);
BOOL format_data_to_file(DATA_INFO *di, const TCHAR *file_name, const int filter_index, TCHAR *err_str);
HANDLE format_file_to_data(const TCHAR *file_name, const TCHAR *format_name, DWORD *ret_size, TCHAR *err_str);
BOOL format_free_data(const TCHAR *format_name, HANDLE data);
BOOL format_free_item(DATA_INFO *di);

// menu
BOOL format_get_menu_title(DATA_INFO *di);
BOOL format_get_menu_icon(DATA_INFO *di);
BOOL format_get_menu_bitmap(DATA_INFO *di);
TCHAR *format_get_tooltip_text(DATA_INFO *di);

// window
BOOL format_window_create(const HWND parent_wnd);
BOOL format_window_destroy(void);
BOOL format_window_show_data(const HWND hWnd, DATA_INFO *di, const BOOL lock);
BOOL format_window_save_data(const HWND hWnd, DATA_INFO *di);
BOOL format_window_hide_data(const HWND hWnd, DATA_INFO *di);

#endif
/* End of source */
