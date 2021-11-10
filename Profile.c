/*
 * CLCL
 *
 * Profile.c
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE
#include <tchar.h>

#include "Profile.h"
#include "memory.h"

/* Define */
#define BUF_SIZE						256
#define ALLOC_SIZE						10

/* Global Variables */
typedef struct _KEY {
	TCHAR key_name[BUF_SIZE];
	int hash;
	TCHAR *string;
	BOOL comment_flag;
} KEY_INFO;

typedef struct _SECTION {
	TCHAR section_name[BUF_SIZE];
	int hash;
	KEY_INFO *key_info;
	int key_count;
	int key_size;
} SECTION_INFO;

static SECTION_INFO *section_info;
static int section_count;
static int section_size;

/* Local Function Prototypes */
static BOOL trim(TCHAR *buf);
static int str2hash(const TCHAR *str);

static BOOL section_add(const TCHAR *section_name);
static int section_find(const TCHAR *section_name);
static BOOL key_add(SECTION_INFO *si, const TCHAR *key_name, const TCHAR *str, const BOOL comment_flag);
static int key_find(const SECTION_INFO *si, const TCHAR *key_name);
static BOOL profile_write_data(const TCHAR *section, const TCHAR *key, const TCHAR *str, const TCHAR *file_path);

/*
 * trim - 文字列の前後の空白, Tabを除去する
 */
static BOOL trim(TCHAR *buf)
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

/*
 * str2hash - 文字列のハッシュ値を取得
 */
static int str2hash(const TCHAR *str)
{
#define to_lower(c)		((c >= TEXT('A') && c <= TEXT('Z')) ? (c - TEXT('A') + TEXT('a')) : c)
	int hash = 0;

	for (; *str != TEXT('\0'); str++) {
		if (*str != TEXT(' ')) {
			hash = ((hash << 1) + to_lower(*str));
		}
	}
	return hash;
}

/*
 * section_add - セクションの追加
 */
static BOOL section_add(const TCHAR *section_name)
{
	SECTION_INFO *tmp_section;

	if (section_name == NULL || *section_name == TEXT('\0')) {
		return FALSE;
	}

	if (section_size < section_count + 1) {
		// 再確保
		section_size += ALLOC_SIZE;
		if ((tmp_section = (SECTION_INFO *)mem_calloc(sizeof(SECTION_INFO) * section_size)) == NULL) {
			return FALSE;
		}
		if (section_info != NULL) {
			CopyMemory(tmp_section, section_info, sizeof(SECTION_INFO) * section_count);
			mem_free(&section_info);
		}
		section_info = tmp_section;
	}
	// セクション追加
	lstrcpyn((section_info + section_count)->section_name, section_name, BUF_SIZE);
	trim((section_info + section_count)->section_name);
	(section_info + section_count)->hash = str2hash((section_info + section_count)->section_name);

	section_count++;
	return TRUE;
}

/*
 * section_find - セクションの検索
 */
static int section_find(const TCHAR *section_name)
{
	int hash;
	int i;

	if (section_info == NULL || section_name == NULL || *section_name == TEXT('\0')) {
		return -1;
	}

	hash = str2hash(section_name);
	for (i = 0; i < section_count; i++) {
		if ((section_info + i)->hash != hash) {
			continue;
		}
		if (lstrcmpi((section_info + i)->section_name, section_name) == 0) {
			return i;
		}
	}
	return -1;
}

/*
 * key_add - キーの追加
 */
static BOOL key_add(SECTION_INFO *si, const TCHAR *key_name, const TCHAR *str, const BOOL comment_flag)
{
	KEY_INFO *tmp_key;
	int index = -1;

	if (key_name == NULL || *key_name == TEXT('\0') || str == NULL) {
		return FALSE;
	}

	if (si->key_size < si->key_count + 1) {
		// 再確保
		si->key_size += ALLOC_SIZE;
		if ((tmp_key = (KEY_INFO *)mem_calloc(sizeof(KEY_INFO) * si->key_size)) == NULL) {
			return FALSE;
		}
		if (si->key_info != NULL) {
			CopyMemory(tmp_key, si->key_info, sizeof(KEY_INFO) * si->key_count);
			mem_free(&si->key_info);
		}
		si->key_info = tmp_key;
	}
	// キー追加
	(si->key_info + si->key_count)->string = (TCHAR *)mem_alloc(sizeof(TCHAR) * (lstrlen(str) + 1));
	if ((si->key_info + si->key_count)->string == NULL) {
		return FALSE;
	}
	lstrcpyn((si->key_info + si->key_count)->key_name, key_name, BUF_SIZE);
	trim((si->key_info + si->key_count)->key_name);
	if (comment_flag == FALSE) {
		(si->key_info + si->key_count)->hash = str2hash((si->key_info + si->key_count)->key_name);
	}
	lstrcpy((si->key_info + si->key_count)->string, str);
	(si->key_info + si->key_count)->comment_flag = comment_flag;

	si->key_count++;
	return TRUE;
}

/*
 * key_find - キーの検索
 */
static int key_find(const SECTION_INFO *si, const TCHAR *key_name)
{
	int hash;
	int i;

	if (si->key_info == NULL || key_name == NULL || *key_name == TEXT('\0')) {
		return -1;
	}

	hash = str2hash(key_name);
	for (i = 0; i < si->key_count; i++) {
		if ((si->key_info + i)->comment_flag == TRUE ||
			(si->key_info + i)->hash != hash) {
			continue;
		}
		if (lstrcmpi((si->key_info + i)->key_name, key_name) == 0) {
			return i;
		}
	}
	return -1;
}

/*
 * profile_initialize - 初期化
 */
BOOL profile_initialize(const TCHAR *path, const BOOL ReadFlag)
{
	HANDLE hFile;
	TCHAR *buf, *p, *r, *s;
	TCHAR tmp[BUF_SIZE];
	BYTE *cbuf;
	DWORD size_low, size_high;
	DWORD ret;
	long len;

	if (ReadFlag == FALSE) {
		section_info = NULL;
		return TRUE;
	}

	// ファイルを開く
	hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == NULL || hFile == (HANDLE)-1) {
		return FALSE;
	}
	// 確保するサイズの取得
	if ((size_low = GetFileSize(hFile, &size_high)) == 0xFFFFFFFF) {
		CloseHandle(hFile);
		return FALSE;
	}

	// 読み取る領域の確保
	if ((cbuf = (BYTE *)mem_alloc(size_low + sizeof(TCHAR))) == NULL) {
		CloseHandle(hFile);
		return FALSE;
	}
	// ファイルを読み込む
	if (ReadFile(hFile, cbuf, size_low, &ret, NULL) == FALSE) {
		mem_free(&cbuf);
		CloseHandle(hFile);
		return FALSE;
	}
	CloseHandle(hFile);

#ifdef UNICODE
	if(size_low > 2 && *cbuf == 0xFF && *(cbuf + 1) == 0xFE) {
		// BOMをスキップ
		len = (size_low - 2) / sizeof(TCHAR);
		buf = (TCHAR *)(cbuf + 2);
		*(buf + len) = TEXT('\0');
	} else {
		// 元ファイルをバックアップ
		TCHAR backup_path[MAX_PATH];
		wsprintf(backup_path, TEXT("%s.back"), path);
		CopyFile(path, backup_path, FALSE);
		// Unicodeに変換
		*(cbuf + size_low) = '\0';
		len = MultiByteToWideChar(CP_ACP, 0, cbuf, -1, NULL, 0);
		if ((buf = (TCHAR *)mem_alloc(sizeof(TCHAR) * (len + 1))) == NULL) {
			mem_free(&cbuf);
			return FALSE;
		}
		MultiByteToWideChar(CP_ACP, 0, cbuf, -1, buf, len);
		mem_free(&cbuf);
		cbuf = (BYTE *)buf;
	}
#else
	if(size_low > 2 && *cbuf == 0xFF && *(cbuf + 1) == 0xFE) {
		mem_free(&cbuf);
		return FALSE;
	}
	len = size_low / sizeof(TCHAR);
	buf = cbuf;
#endif

	// セクションの確保
	if ((section_info = (SECTION_INFO *)mem_calloc(sizeof(SECTION_INFO) * ALLOC_SIZE)) == NULL) {
		mem_free(&cbuf);
		return FALSE;
	}
	section_count = 1;
	section_size = ALLOC_SIZE;

	p = buf;
	while ((len > (p - buf)) && *p != TEXT('\0')) {
		for (r = p; (len > (r - buf)) && (*r != TEXT('\r') && *r != TEXT('\n')); r++)
			;

		switch (*p) {
		case TEXT('['):
			// セクションの追加
			if (p == r || *(r - 1) != TEXT(']')) {
				break;
			}
			*(r - 1) = TEXT('\0');
			section_add(p + 1);
			break;

		case TEXT('\r'):
		case TEXT('\n'):
			break;

		default:
			if (section_info == NULL || p == r) {
				break;
			}
			if (*p == TEXT('#')) {
				// コメント
				for (s = tmp; p < r; p++, s++) {
					*s = *p;
				}
				*s = TEXT('\0');
				key_add((section_info + section_count - 1), tmp, TEXT(""), TRUE);
			} else {
				// キーの追加
				for (s = tmp; p < r; p++, s++) {
					if (*p == TEXT('=')) {
						break;
					}
					*s = *p;
				}
				*s = TEXT('\0');
				if (*p == TEXT('=')) {
					p++;
				}
				*r = TEXT('\0');
				key_add((section_info + section_count - 1), tmp, p, FALSE);
			}
			if (len > (r - buf)) {
				r++;
			}
		}
		p = r;
		for (; (len > (p - buf)) && (*p == TEXT('\r') || *p == TEXT('\n')); p++)
			;
	}
	mem_free(&cbuf);
	return TRUE;
}

/*
 * profile_flush - バッファをファイルに書き込む
 */
BOOL profile_flush(const TCHAR *path)
{
	HANDLE hFile;
	TCHAR *buf, *p;
	int len;
	int i, j;
	DWORD ret;

	if (section_info == NULL) {
		return FALSE;
	}

	// 保存サイズの計算
	len = 0;
	for (i = 0; i < section_count; i++) {
		if ((section_info + i)->key_info == NULL) {
			continue;
		}
		// セクション名
		if (i != 0) {
			len += lstrlen((section_info + i)->section_name) + 4;
		}
		for (j = 0; j < (section_info + i)->key_count; j++) {
			if (*((section_info + i)->key_info + j)->key_name == TEXT('\0')) {
				continue;
			}
			// キー名
			len += lstrlen(((section_info + i)->key_info + j)->key_name);
			if (((section_info + i)->key_info + j)->comment_flag == FALSE) {
				len++;
				if (((section_info + i)->key_info + j)->string != NULL) {
					// 文字列
					len += lstrlen(((section_info + i)->key_info + j)->string);
				}
			}
			len += 2;
		}
		len += 2;
	}

	// 保存するための領域の確保
#ifdef UNICODE
	if ((buf = (TCHAR *)mem_alloc(2 + sizeof(TCHAR) * (len + 1))) == NULL) {
		return FALSE;
	}
	// BOM
	*((BYTE *)buf) = 0xFF;
	*(((BYTE *)buf) + 1) = 0xFE;
	p = (TCHAR *)(((BYTE *)buf) + 2);
#else
	if ((p = buf = (TCHAR *)mem_alloc(sizeof(TCHAR) * (len + 1))) == NULL) {
		return FALSE;
	}
#endif
	// 保存文字列の作成
	for (i = 0; i < section_count; i++) {
		if ((section_info + i)->key_info == NULL) {
			continue;
		}
		// セクション名
		if (i != 0) {
			*(p++) = TEXT('[');
			lstrcpy(p, (section_info + i)->section_name);
			p += lstrlen(p);
			*(p++) = TEXT(']');
			*(p++) = TEXT('\r');
			*(p++) = TEXT('\n');
		}
		for (j = 0; j < (section_info + i)->key_count; j++) {
			if (*((section_info + i)->key_info + j)->key_name == TEXT('\0')) {
				continue;
			}
			// キー名
			lstrcpy(p, ((section_info + i)->key_info + j)->key_name);
			p += lstrlen(p);
			if (((section_info + i)->key_info + j)->comment_flag == FALSE) {
				*(p++) = TEXT('=');
				if (((section_info + i)->key_info + j)->string != NULL) {
					// 文字列
					lstrcpy(p, ((section_info + i)->key_info + j)->string);
					p += lstrlen(p);
				}
			}
			*(p++) = TEXT('\r');
			*(p++) = TEXT('\n');
		}
		*(p++) = TEXT('\r');
		*(p++) = TEXT('\n');
	}
	*p = TEXT('\0');

	// ファイルを開く
	hFile = CreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == NULL || hFile == (HANDLE)-1) {
		mem_free(&buf);
		return FALSE;
	}
	// ファイルに書き込む
#ifdef UNICODE
	if (WriteFile(hFile, buf, 2 + sizeof(TCHAR) * len, &ret, NULL) == FALSE) {
#else
	if (WriteFile(hFile, buf, sizeof(TCHAR) * len, &ret, NULL) == FALSE) {
#endif
		mem_free(&buf);
		CloseHandle(hFile);
		return FALSE;
	}
	// メモリの解放
	mem_free(&buf);
	// ファイルを閉じる
	FlushFileBuffers(hFile);
	CloseHandle(hFile);
	return TRUE;
}

/*
 * profile_free - 設定情報の解放
 */
void profile_free(void)
{
	int i, j;

	if (section_info == NULL) {
		return;
	}
	for (i = 0; i < section_count; i++) {
		if ((section_info + i)->key_info == NULL) {
			continue;
		}
		// キーの解放
		for (j = 0; j < (section_info + i)->key_count; j++) {
			if (((section_info + i)->key_info + j)->string != NULL) {
				mem_free(&((section_info + i)->key_info + j)->string);
			}
		}
		mem_free(&(section_info + i)->key_info);
	}
	mem_free(&section_info);
	section_info = NULL;
	section_count = 0;
	section_size = 0;
}

/*
 * profile_get_string - 文字列の取得
 */
long profile_get_string(const TCHAR *section, const TCHAR *key, const TCHAR *default_str, TCHAR *ret, const long size, const TCHAR *file_path)
{
	TCHAR *buf, *p;
	int section_index;
	int key_index;
	int len;

	// セクションの検索
	if ((section_index = section_find(section)) == -1) {
		lstrcpyn(ret, default_str, size);
		return lstrlen(ret);
	}

	// キーの検索
	key_index = key_find((section_info + section_index), key);
	if (key_index == -1 || ((section_info + section_index)->key_info + key_index)->string == NULL) {
		lstrcpyn(ret, default_str, size);
		return lstrlen(ret);
	}

	// 内容の取得
	buf = (TCHAR *)mem_alloc(sizeof(TCHAR) * (lstrlen(((section_info + section_index)->key_info + key_index)->string) + 1));
	if (buf != NULL) {
		lstrcpy(buf, ((section_info + section_index)->key_info + key_index)->string);
		trim(buf);
		p = (*buf == TEXT('\"')) ? buf + 1 : buf;
		if ((len = lstrlen(p)) > 0 && *(p + len - 1) == TEXT('\"')) {
			*(p + len - 1) = TEXT('\0');
		}
		lstrcpyn(ret, p, size);
		mem_free(&buf);
	} else {
		lstrcpyn(ret, ((section_info + section_index)->key_info + key_index)->string, size);
	}
	return lstrlen(ret);
}

/*
 * profile_alloc_string - バッファを確保して文字列の取得
 */
TCHAR *profile_alloc_string(const TCHAR *section, const TCHAR *key, const TCHAR *default_str, const TCHAR *file_path)
{
	TCHAR *buf;
	int section_index;
	int key_index;
	int len;

	// セクションの検索
	if ((section_index = section_find(section)) == -1) {
		if ((buf = (TCHAR *)mem_alloc(sizeof(TCHAR) * (lstrlen(default_str) + 1))) != NULL) {
			lstrcpy(buf, default_str);
			return buf;
		}
		return NULL;
	}

	// キーの検索
	key_index = key_find((section_info + section_index), key);
	if (key_index == -1 || ((section_info + section_index)->key_info + key_index)->string == NULL) {
		if ((buf = (TCHAR *)mem_alloc(sizeof(TCHAR) * (lstrlen(default_str) + 1))) != NULL) {
			lstrcpy(buf, default_str);
			return buf;
		}
		return NULL;
	}

	// 内容の取得
	buf = (TCHAR *)mem_alloc(sizeof(TCHAR) * (lstrlen(((section_info + section_index)->key_info + key_index)->string) + 1));
	if (buf != NULL) {
		lstrcpy(buf, ((section_info + section_index)->key_info + key_index)->string);
		trim(buf);
		if (*buf == TEXT('\"')) {
			lstrcpy(buf, buf + 1);
			if ((len = lstrlen(buf)) > 0 && *(buf + len - 1) == TEXT('\"')) {
				*(buf + len - 1) = TEXT('\0');
			}
		}
		return buf;
	}
	return NULL;
}

/*
 * profile_free_string - バッファの解放
 */
void profile_free_string(TCHAR *buf)
{
	mem_free(&buf);
}

/*
 * profile_get_int - 数値の取得
 */
int profile_get_int(const TCHAR *section, const TCHAR *key, const int default_str, const TCHAR *file_path)
{
	TCHAR *buf, *p;
	int section_index;
	int key_index;
	int ret;
	int len;

	// セクションの検索
	if ((section_index = section_find(section)) == -1) {
		return default_str;
	}

	// キーの検索
	key_index = key_find((section_info + section_index), key);
	if (key_index == -1 || ((section_info + section_index)->key_info + key_index)->string == NULL) {
		return default_str;
	}

	// 内容の取得
	buf = (TCHAR *)mem_alloc(sizeof(TCHAR) * (lstrlen(((section_info + section_index)->key_info + key_index)->string) + 1));
	if (buf != NULL) {
		lstrcpy(buf, ((section_info + section_index)->key_info + key_index)->string);
		trim(buf);
		p = (*buf == TEXT('\"')) ? buf + 1 : buf;
		if ((len = lstrlen(p)) > 0 && *(p + len - 1) == TEXT('\"')) {
			*(p + len - 1) = TEXT('\0');
		}
		ret = _ttoi(p);
		mem_free(&buf);
	} else {
		ret = _ttoi(((section_info + section_index)->key_info + key_index)->string);
	}
	return ret;
}

/*
 * profile_write_data - データの書き込み
 */
static BOOL profile_write_data(const TCHAR *section, const TCHAR *key, const TCHAR *str, const TCHAR *file_path)
{
	int section_index;
	int key_index;
	int j;

	if (section == NULL) {
		return FALSE;
	}

	if (section_info == NULL) {
		// セクションの確保
		if ((section_info = (SECTION_INFO *)mem_calloc(sizeof(SECTION_INFO) * ALLOC_SIZE)) == NULL) {
			return FALSE;
		}
		section_count = 1;
		section_size = ALLOC_SIZE;
	}

	// セクションの検索
	if ((section_index = section_find(section)) == -1) {
		// セクションの追加
		if (section_add(section) == FALSE) {
			return FALSE;
		}
		section_index = section_count - 1;
	}

	if (key == NULL) {
		if ((section_info + section_index)->key_info != NULL) {
			// キーの削除
			for (j = 0; j < (section_info + section_index)->key_count; j++) {
				if (((section_info + section_index)->key_info + j)->string != NULL) {
					mem_free(&((section_info + section_index)->key_info + j)->string);
				}
			}
			mem_free(&(section_info + section_index)->key_info);
			(section_info + section_index)->key_info = NULL;
			(section_info + section_index)->key_count = 0;
			(section_info + section_index)->key_size = 0;
		}
		return TRUE;
	}

	// キーの検索
	if ((key_index = key_find((section_info + section_index), key)) == -1) {
		// キーの追加
		return key_add((section_info + section_index), key, str, FALSE);
	} else {
		// 内容の変更
		if (((section_info + section_index)->key_info + key_index)->string != NULL) {
			mem_free(&((section_info + section_index)->key_info + key_index)->string);
		}
		if (str == NULL) {
			*((section_info + section_index)->key_info + key_index)->key_name = TEXT('\0');
			((section_info + section_index)->key_info + key_index)->string = NULL;
			return TRUE;
		}
		((section_info + section_index)->key_info + key_index)->string = (TCHAR *)mem_alloc(sizeof(TCHAR) * (lstrlen(str) + 1));
		if (((section_info + section_index)->key_info + key_index)->string == NULL) {
			return FALSE;
		}
		lstrcpy(((section_info + section_index)->key_info + key_index)->string, str);
	}
	return TRUE;
}

/*
 * profile_write_string - 文字列の書き込み
 */
BOOL profile_write_string(const TCHAR *section, const TCHAR *key, const TCHAR *str, const TCHAR *file_path)
{
	TCHAR *buf, *p;
	BOOL ret;

	if (str == NULL || *str == TEXT('\0')) {
		return profile_write_data(section, key, TEXT(""), file_path);
	}

	if ((buf = (TCHAR *)mem_alloc(sizeof(TCHAR) * (lstrlen(str) + 3))) == NULL) {
		return profile_write_data(section, key, str, file_path);
	}
	p = buf;
	*(p++) = TEXT('"');
	lstrcpy(p, str);
	p += lstrlen(p);
	*(p++) = TEXT('"');
	*(p++) = TEXT('\0');
	ret = profile_write_data(section, key, buf, file_path);
	mem_free(&buf);
	return ret;
}

/*
 * profile_write_int - 数値の書き込み
 */
BOOL profile_write_int(const TCHAR *section, const TCHAR *key, const int num, const TCHAR *file_path)
{
	TCHAR ret[BUF_SIZE];

	_itot_s(num, ret, BUF_SIZE, 10);
	return profile_write_data(section, key, ret, file_path);
}
/* End of source */
