/*
 * CLCL
 *
 * Memory.c
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#include <windows.h>
#include <tchar.h>

#include "String.h"

/* Define */

/* Global Variables */

/* Local Function Prototypes */
#ifdef _DEBUG
static long all_alloc_size = 0;

// #define MEM_CHECK
#ifdef MEM_CHECK
#define ADDRESS_CNT		100000
#define DEBUG_ADDRESS	0
static long address[ADDRESS_CNT];
static int address_index;
#endif	// MEM_CHECK

#endif	// _DEBUG

/*
 * mem_alloc - バッファを確保
 */
void *mem_alloc(const DWORD size)
{
#ifdef _DEBUG
	void *mem;

	mem = LocalAlloc(LMEM_FIXED, size);
	all_alloc_size += LocalSize(mem);
#ifdef MEM_CHECK
	if (address_index < ADDRESS_CNT) {
		if (address_index == DEBUG_ADDRESS) {
			address[address_index] = (long)mem;
		} else {
			address[address_index] = (long)mem;
		}
		address_index++;
	}
#endif	// MEM_CHECK
	return mem;
#else	// _DEBUG
	return LocalAlloc(LMEM_FIXED, size);
#endif	// _DEBUG
}

/*
 * mem_calloc - 初期化してバッファを確保
 */
void *mem_calloc(const DWORD size)
{
#ifdef _DEBUG
	void *mem;

	mem = LocalAlloc(LPTR, size);
	all_alloc_size += LocalSize(mem);
#ifdef MEM_CHECK
	if (address_index < ADDRESS_CNT) {
		if (address_index == DEBUG_ADDRESS) {
			address[address_index] = (long)mem;
		} else {
			address[address_index] = (long)mem;
		}
		address_index++;
	}
#endif	// MEM_CHECK
	return mem;
#else	// _DEBUG
	return LocalAlloc(LPTR, size);
#endif	// _DEBUG
}

/*
 * mem_free - バッファを解放
 */
void mem_free(void **mem)
{
	if (*mem != NULL) {
#ifdef _DEBUG
		all_alloc_size -= LocalSize(*mem);
#ifdef MEM_CHECK
		{
			int i;

			for (i = 0; i < ADDRESS_CNT; i++) {
				if (address[i] == (long)*mem) {
					address[i] = 0;
					break;
				}
			}
		}
#endif	// MEM_CHECK
#endif	// _DEBUG
		LocalFree(*mem);
		*mem = NULL;
	}
}

/*
 * mem_debug - メモリ情報の表示
 */
#ifdef _DEBUG
void mem_debug(void)
{
	TCHAR buf[256];

	if (all_alloc_size <= 0) {
		return;
	}

	wsprintf(buf, TEXT("Memory leak: %lu bytes"), all_alloc_size);
	MessageBox(NULL, buf, TEXT("debug"), 0);
#ifdef MEM_CHECK
	{
		int i;

		for (i = 0; i < ADDRESS_CNT; i++) {
			if (address[i] != 0) {
				wsprintf(buf, TEXT("leak address: %u, %lu"), i, address[i]);
				MessageBox(NULL, buf, TEXT("debug"), 0);
				break;
			}
		}
	}
#endif	// MEM_CHECK
}
#endif	// _DEBUG

/*
 * mem_cmp - メモリの比較
 */
int mem_cmp(const BYTE *mem1, const DWORD size1, const BYTE *mem2, const DWORD size2)
{
	if (size1 != size2) {
		return 1;
	}
	return memcmp(mem1, mem2, size1);
}

/*
 * alloc_copy - バッファを確保して文字列をコピーする
 */
TCHAR *alloc_copy(const TCHAR *buf)
{
	TCHAR *ret;

	if (buf == NULL) {
		return NULL;
	}
	if ((ret = (TCHAR *)mem_alloc(sizeof(TCHAR) * (lstrlen(buf) + 1))) == NULL) {
		return NULL;
	}
	lstrcpy(ret, buf);
	return ret;
}

/*
 * alloc_copy_n - バッファを確保して指定長さ分の文字列をコピーする
 */
TCHAR *alloc_copy_n(const TCHAR *buf, const int size)
{
	TCHAR *ret;

	if (buf == NULL) {
		return NULL;
	}
	if ((ret = (TCHAR *)mem_alloc(sizeof(TCHAR) * (size + 1))) == NULL) {
		return NULL;
	}
	lstrcpyn(ret, buf, size);
	return ret;
}

/*
 * alloc_tchar_to_char - メモリを確保して TCHAR を char に変換する
 */
#ifdef UNICODE
char *alloc_tchar_to_char(const TCHAR *str)
{
	char *cchar;
	int len;

	len = tchar_to_char_size(str);
	if ((cchar = (char *)mem_alloc(len + 1)) == NULL) {
		return NULL;
	}
	tchar_to_char(str, cchar, len);
	return cchar;
}
#endif

/*
 * alloc_char_to_tchar - メモリを確保して char を TCHAR に変換する
 */
#ifdef UNICODE
TCHAR *alloc_char_to_tchar(const char *str)
{
	TCHAR *tchar;
	int len;

	len = char_to_tchar_size(str);
	tchar = (TCHAR *)mem_alloc(sizeof(TCHAR) * (len + 1));
	if (tchar == NULL) {
		return NULL;
	}
	char_to_tchar(str, tchar, len);
	return tchar;
}
#endif
/* End of source */
