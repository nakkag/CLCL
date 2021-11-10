/*
 * CLCL
 *
 * Filter.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_FILTER_H
#define _INC_FILTER_H

/* Include Files */
#include "Format.h"

/* Define */
#define FILTER_ACTION_ADD				0
#define FILTER_ACTION_IGNORE			1

#define FILTER_SAVE_NOSAVE				0
#define FILTER_SAVE_SAVE				1

/* Struct */
// 形式フィルタ
typedef struct _FILTER_INFO {
	TCHAR *format_name;

	FORMAT_NAME *fn;
	int fn_cnt;

	int action;							// FILTER_ACTION_
	int save;							// FILTER_SAVE_
	DWORD limit_size;
} FILTER_INFO;

/* Function Prototypes */
int filter_get_index(const TCHAR *format_name, const int name_hash);
BOOL filter_format_check(const TCHAR *format_name);
BOOL filter_size_check(const TCHAR *format_name, const DWORD size);
BOOL filter_list_save_check(DATA_INFO *di);
DATA_INFO *filter_list_copy(DATA_INFO *di, TCHAR *err_str);

#endif
/* End of source */
