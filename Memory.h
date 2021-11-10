/*
 * CLCL
 *
 * Memory.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_MEMORY_H
#define _INC_MEMORY_H

/* Include Files */

/* Define */

/* Struct */

/* Function Prototypes */
void *mem_alloc(const DWORD size);
void *mem_calloc(const DWORD size);
void mem_free(void **mem);
#ifdef _DEBUG
void mem_debug(void);
#endif
int mem_cmp(const BYTE *mem1, const DWORD size1, const BYTE *mem2, const DWORD size2);
TCHAR *alloc_copy(const TCHAR *buf);
TCHAR *alloc_copy_n(const TCHAR *buf, const int size);

#ifdef UNICODE
char *alloc_tchar_to_char(const TCHAR *str);
#else
#define alloc_tchar_to_char	alloc_copy
#endif

#ifdef UNICODE
TCHAR *alloc_char_to_tchar(const char *str);
#else
#define alloc_char_to_tchar	alloc_copy
#endif

#endif
/* End of source */
