/*
 * CLCL
 *
 * File.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_FILE_H
#define _INC_FILE_H

/* Include Files */

/* Define */

/* Struct */

/* Function Prototypes */
BOOL file_name_check(TCHAR *file_name);
void file_name_conv(TCHAR *file_name, TCHAR conv_char);
BOOL file_check_directory(const TCHAR *path);
BOOL file_check_file(const TCHAR *path);
BYTE *file_read_buf(const TCHAR *path, DWORD *ret_size, TCHAR *err_str);
BOOL file_write_buf(const TCHAR *path, const BYTE *data, const DWORD size, TCHAR *err_str);
BOOL file_read_data(const TCHAR *path, DATA_INFO **root, TCHAR *err_str);
BOOL file_write_data(const TCHAR *path, DATA_INFO *di, TCHAR *err_str);
BOOL shell_open(const TCHAR *file_name, const TCHAR *command_line);

#endif
/* End of source */
