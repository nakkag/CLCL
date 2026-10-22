/*
 * CLCL
 *
 * String.c
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#include <windows.h>
#include <tchar.h>
#include <limits.h>

#include "Memory.h"

/* Define */
#define to_lower(c)		((c >= TEXT('A') && c <= TEXT('Z')) ? (c - TEXT('A') + TEXT('a')) : c)

/* Global Variables */

/* Local Function Prototypes */

/*
 * a2i - 数字の文字列を数値(int)に変換する
 */
int a2i(const char *str)
{
	int num = 0;
	int m = 1;
	int digit;
	int i;

	if (str == NULL) {
		return 0;
	}
	if (*str == '-') {
		m = -1;
		str++;
	} else if (*str == '+') {
		str++;
	}

	// 10桁までを変換し、int の範囲を超える値は上限で打ち切る
	for (i = 0; i < 10 && *str >= '0' && *str <= '9'; i++, str++) {
		digit = *str - '0';
		if (num > (INT_MAX - digit) / 10) {
			num = INT_MAX;
			break;
		}
		num = 10 * num + digit;
	}
	return (num * m);
}

/*
 * x2i - 16進文字列を数値(int)に変換する
 */
int x2i(const char *str)
{
	int ret = 0;
	int num;
	int i;

	if (str == NULL) {
		return 0;
	}

	if (*str == '0' && (*(str + 1) == 'x' || *(str + 1) == 'X')) {
		str += 2;
	} else if (*str == 'x' || *str == 'X') {
		str++;
	}

	for (i = 0; i < 8 && *str != '\0'; i++, str++) {
		if (*str >= '0' && *str <= '9') {
			num = *str - '0';
		} else if (*str >= 'a' && *str <= 'f') {
			num = *str - 'a' + 10;
		} else if (*str >= 'A' && *str <= 'F') {
			num = *str - 'A' + 10;
		} else {
			break;
		}
		ret = ret * 16 + num;
	}
	return ret;
}

/*
 * tx2i - 16進文字列(UNICODE)を数値(int)に変換する
 */
#ifdef UNICODE
int tx2i(const TCHAR *str)
{
	char *c_str;
	int ret = 0;

	if ((c_str = alloc_tchar_to_char(str)) != NULL) {
		ret = x2i(c_str);
		mem_free(&c_str);
	}
	return ret;
}
#endif

/*
 * str2hash - 文字列のハッシュ値を取得
 */
int str2hash(const TCHAR *str)
{
	int hash = 0;

	for (; *str != TEXT('\0'); str++) {
		if (*str != TEXT(' ')) {
			hash = ((hash << 1) + to_lower(*str));
		}
	}
	return hash;
}

/*
 * str_match - ２つの文字列をワイルドカード(*)を使って比較を行う
 */
BOOL str_match(const TCHAR *Ptn, const TCHAR *Str)
{
	const TCHAR *star_ptn = NULL;
	const TCHAR *star_str = NULL;

	while (*Str != TEXT('\0')) {
		if (*Ptn == TEXT('*')) {
			// '*' の位置を記憶して、まずは空文字列に一致させる
			star_ptn = Ptn++;
			star_str = Str;
		} else if (*Ptn == TEXT('?') || to_lower(*Ptn) == to_lower(*Str)) {
			Ptn++;
			Str++;
		} else if (star_ptn != NULL) {
			// 直前の '*' に1文字多く一致させて再試行
			Ptn = star_ptn + 1;
			Str = ++star_str;
		} else {
			return FALSE;
		}
	}
	// 残りのパターンが '*' のみなら一致
	for (; *Ptn == TEXT('*'); Ptn++)
		;
	return (*Ptn == TEXT('\0'));
}

/*
 * Trim - 文字列の前後の空白, Tabを除去する
 */
BOOL Trim(TCHAR *buf)
{
	TCHAR *p, *r;

	// 前後の空白を除いたポインタを取得
	for (p = buf; (*p == TEXT(' ') || *p == TEXT('\t')) && *p != TEXT('\0'); p++)
		;
	for (r = buf + lstrlen(buf) - 1; r > p && (*r == TEXT(' ') || *r == TEXT('\t')); r--)
		;
	*(r + 1) = TEXT('\0');

	// 元の文字列にコピー
	lstrcpy(buf, p);
	return TRUE;
}
/* End of source */
