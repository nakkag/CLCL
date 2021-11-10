/*
 * CLCL
 *
 * ClipBoard.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_CLIPBOARD_H
#define _INC_CLIPBOARD_H

/* Include Files */

/* Define */

/* Struct */

/* Function Prototypes */
UINT clipboard_get_format(const UINT format, TCHAR *type_name);
DATA_INFO *clipboard_get_datainfo(const BOOL use_filter, const BOOL get_data, TCHAR *err_str);
BOOL clipboard_set_datainfo(const HWND hWnd, DATA_INFO *set_di, TCHAR *err_str);
DATA_INFO *clipboard_to_item(TCHAR *err_str);
HANDLE clipboard_copy_data(const UINT format, const HANDLE data, DWORD *ret_size);
BYTE *clipboard_data_to_bytes(const DATA_INFO *di, DWORD *ret_size);
HANDLE clipboard_bytes_to_data(TCHAR *format_name, const BYTE *data, DWORD *size);
BOOL clipboard_data_to_file(DATA_INFO *di, const TCHAR *file_name, const int filter_index, TCHAR *err_str);
HANDLE clipboard_file_to_data(const TCHAR *file_name, TCHAR *format_name, DWORD *ret_size, TCHAR *err_str);
BOOL clipboard_free_data(TCHAR *format_name, HANDLE data);

#endif
/* End of source */
