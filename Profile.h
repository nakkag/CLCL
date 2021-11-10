/*
 * CLCL
 *
 * Profile.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_PROFILE_H
#define _INC_PROFILE_H

/* Include Files */

/* Define */

/* Struct */

/* Function Prototypes */
BOOL profile_initialize(const TCHAR *path, const BOOL ReadFlag);
BOOL profile_flush(const TCHAR *path);
void profile_free(void);
long profile_get_string(const TCHAR *section, const TCHAR *key, const TCHAR *default_str, TCHAR *ret, const long size, const TCHAR *file_path);
TCHAR *profile_alloc_string(const TCHAR *section, const TCHAR *key, const TCHAR *default_str, const TCHAR *file_path);
void profile_free_string(TCHAR *buf);
int profile_get_int(const TCHAR *section, const TCHAR *key, const int default_str, const TCHAR *file_path);
BOOL profile_write_string(const TCHAR *section, const TCHAR *key, const TCHAR *str, const TCHAR *file_path);
BOOL profile_write_int(const TCHAR *section, const TCHAR *key, const int num, const TCHAR *file_path);

#endif
/* End of source */
