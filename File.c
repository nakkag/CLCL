/*
 * CLCL
 *
 * File.c
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

#include "General.h"
#include "Memory.h"
#include "String.h"
#include "Data.h"
#include "Message.h"
#include "File.h"
#include "Format.h"
#include "ClipBoard.h"

/* Define */

/* Global Variables */
extern HINSTANCE hInst;

/* ocal Function Prototypes */
static void file_expand_option(DATA_INFO *di, char *option);
static BYTE *file_file_to_item(const BYTE *buf, BYTE *p, const DWORD size, DATA_INFO **root, const int level, TCHAR *err_str);
static BOOL file_item_to_file(const HANDLE hFile, DATA_INFO *di, TCHAR *err_str);

/*
 * file_name_check - ファイル名にできない文字列が含まれていないかチェックを行う
 */
BOOL file_name_check(TCHAR *file_name)
{
	TCHAR *p;

	for (p = file_name; *p != TEXT('\0'); p++) {
#ifndef UNICODE
		if (IsDBCSLeadByte((BYTE)*p) == TRUE) {
			// ２バイトコードの場合
			p++;
			continue;

		}
#endif
		// ファイル名にできない文字のチェック
		if (*p == TEXT('\\') ||
			*p == TEXT('/') ||
			*p == TEXT(':') ||
			*p == TEXT(',') ||
			*p == TEXT(';') ||
			*p == TEXT('*') ||
			*p == TEXT('?') ||
			*p == TEXT('\"') ||
			*p == TEXT('<') ||
			*p == TEXT('>') ||
			*p == TEXT('|')) {
			return FALSE;
		}
	}
	return TRUE;
}

/*
 * file_name_conv - ファイル名に使えない文字を変換する
 */
void file_name_conv(TCHAR *file_name, TCHAR conv_char)
{
	TCHAR *p;

	for (p = file_name; *p != TEXT('\0'); p++) {
#ifndef UNICODE
		if (IsDBCSLeadByte((BYTE)*p) == TRUE) {
			// ２バイトコードの場合
			p++;
			continue;
		}
#endif
		// ファイル名にできない文字は指定の文字に変換
		if (*p == TEXT('\\') ||
			*p == TEXT('/') ||
			*p == TEXT(':') ||
			*p == TEXT(',') ||
			*p == TEXT(';') ||
			*p == TEXT('*') ||
			*p == TEXT('?') ||
			*p == TEXT('\"') ||
			*p == TEXT('<') ||
			*p == TEXT('>') ||
			*p == TEXT('|')) {
			*p = conv_char;
		}
	}
}

/*
 * file_check_directory - ディレクトリが存在するかチェックする
 */
BOOL file_check_directory(const TCHAR *path)
{
	WIN32_FIND_DATA FindData;
	HANDLE hFindFile;

	if ((hFindFile = FindFirstFile(path, &FindData)) == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	FindClose(hFindFile);

	if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		// ディレクトリが存在した場合
		return TRUE;
	}
	return FALSE;
}

/*
 * file_check_file - ファイルが存在するかチェックする
 */
BOOL file_check_file(const TCHAR *path)
{
	WIN32_FIND_DATA FindData;
	HANDLE hFindFile;

	if ((hFindFile = FindFirstFile(path, &FindData)) == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	FindClose(hFindFile);

	if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
		// ファイルが存在した場合
		return TRUE;
	}
	return FALSE;
}

/*
 * file_read_buf - ファイルを読み込む
 */
BYTE *file_read_buf(const TCHAR *path, DWORD *ret_size, TCHAR *err_str)
{
	HANDLE hFile;
	DWORD size;
	DWORD ret;
	BYTE *buf;

	// ファイルを開く
	hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == NULL || hFile == (HANDLE)-1) {
		message_get_error(GetLastError(), err_str);
		return NULL;
	}
	if ((size = GetFileSize(hFile, NULL)) == 0xFFFFFFFF) {
		message_get_error(GetLastError(), err_str);
		CloseHandle(hFile);
		return NULL;
	}

	if ((buf = (BYTE *)mem_alloc(size + 1)) == NULL) {
		message_get_error(GetLastError(), err_str);
		CloseHandle(hFile);
		return NULL;
	}
	// ファイルを読みこむ
	if (ReadFile(hFile, buf, size, &ret, NULL) == FALSE) {
		message_get_error(GetLastError(), err_str);
		mem_free(&buf);
		CloseHandle(hFile);
		return NULL;
	}
	CloseHandle(hFile);

	if (ret_size != NULL) {
		*ret_size = size;
	}
	return buf;
}

/*
 * file_write_buf - ファイルに書き込む
 */
BOOL file_write_buf(const TCHAR *path, const BYTE *data, const DWORD size, TCHAR *err_str)
{
	HANDLE hFile;
	DWORD ret;

	// ファイルを開く
	hFile = CreateFile(path, GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == NULL || hFile == (HANDLE)-1) {
		message_get_error(GetLastError(), err_str);
		return FALSE;
	}
	// ファイルの書き込み
	if (WriteFile(hFile, data, size, &ret, NULL) == FALSE) {
		message_get_error(GetLastError(), err_str);
		CloseHandle(hFile);
		return FALSE;
	}
	FlushFileBuffers(hFile);
	CloseHandle(hFile);
	return TRUE;
}

/*
 * file_expand_option - オプション文字列を展開
 */
static void file_expand_option(DATA_INFO *di, char *option)
{
	char *p = option;

	if (option == NULL || *option == '\0') {
		return;
	}

	di->op_modifiers = a2i(p);
	for (; *p != ',' && *p != '\0'; p++)
		;
	if (*p == '\0') {
		return;
	}
	p++;

	di->op_virtkey = a2i(p);
	for (; *p != ',' && *p != '\0'; p++)
		;
	if (*p == '\0') {
		return;
	}
	p++;

	di->op_paste = a2i(p);
}

/*
 * file_file_to_item - ファイルを読み込んでアイテムリストに変換
 */
static BYTE *file_file_to_item(const BYTE *buf, BYTE *p, const DWORD size, DATA_INFO **root, const int level, TCHAR *err_str)
{
	DATA_INFO *di = *root;
	DATA_INFO *new_item;
#ifndef OPTION_SET
	DATA_INFO *cdi;
	DATA_INFO *child_item;
	BYTE *mem;
	int i;
#endif	// OPTION_SET
	DWORD data_size;

	while (size > (DWORD)(p - buf)) {
		switch (*p) {
		case '\x5':
			if (level == 0) {
				return NULL;
			}
			p++;
			return p;

		case '\x4':
			p++;
			// フォルダの作成
			if ((new_item = data_create_folder(NULL, err_str)) == NULL) {
				return NULL;
			}
			// タイトル
			if (size > (DWORD)(p - buf) && *p != '\x2') {
				if (*p != '\0') {
					new_item->title = alloc_char_to_tchar(p);
				}
				for (; *p != '\0'; p++)
					;
				p++;
			}
			for (; size > (DWORD)(p - buf) && *p != '\x1' && *p != '\x4' && *p != '\x5'; p++)
				;

			// 形式の追加
			if (*root == NULL) {
				*root = new_item;
			} else {
				di->next = new_item;
			}
			di = new_item;
			
			if ((p = file_file_to_item(buf, p, size, &di->child, level + 1, err_str)) == NULL) {
				return NULL;
			}
			break;

		case '\x1':
			p++;
#ifndef OPTION_SET
			// 親アイテムの作成
			new_item = data_create_item(NULL, FALSE, err_str);
			if (new_item == NULL) {
				return NULL;
			}
#endif	// OPTION_SET

			// タイトル
			if (size > (DWORD)(p - buf) && *p != '\x2') {
#ifndef OPTION_SET
				if (*p != '\0') {
					new_item->title = alloc_char_to_tchar(p);
				}
#endif	// OPTION_SET
				for (; size > (DWORD)(p - buf) && *p != '\0'; p++)
					;
				p++;
			}
			// 更新日時
			if (size > (DWORD)(p - buf) && *p != '\x2') {
#ifndef OPTION_SET
				if (*p != '\0') {
					new_item->modified.dwHighDateTime = x2i(p);
					for (i = 0; size > (DWORD)(p - buf) && *p != '\0' && i < 8; p++, i++)
						;
				}
				if (*p != '\0') {
					new_item->modified.dwLowDateTime = x2i(p);
				}
#endif	// OPTION_SET
				for (; *p != '\0'; p++)
					;
				p++;
			}
			// ウィンドウ名
			if (size > (DWORD)(p - buf) && *p != '\x2') {
#ifndef OPTION_SET
				if (*p != '\0') {
					new_item->window_name = alloc_char_to_tchar(p);
				}
#endif	// OPTION_SET
				for (; size > (DWORD)(p - buf) && *p != '\0'; p++)
					;
				p++;
			}
			// ツール用文字列
			if (size > (DWORD)(p - buf) && *p != '\x2') {
#ifndef OPTION_SET
				if (*p != '\0') {
					new_item->plugin_string = alloc_char_to_tchar(p);
				}
#endif	// OPTION_SET
				for (; size > (DWORD)(p - buf) && *p != '\0'; p++)
					;
				p++;
			}
			// ツール用long
			if (size > (DWORD)(p - buf) && *p != '\x2') {
#ifndef OPTION_SET
				new_item->plugin_param = a2i(p);
#endif	// OPTION_SET
				for (; size > (DWORD)(p - buf) && *p != '\0'; p++)
					;
				p++;
			}
			// オプション
			if (size > (DWORD)(p - buf) && *p != '\x2') {
#ifndef OPTION_SET
				if (*p != '\0') {
					file_expand_option(new_item, p);
				}
#endif	// OPTION_SET
				for (; size > (DWORD)(p - buf) && *p != '\0'; p++)
					;
				p++;
			}
			// ヘッダ開始位置までスキップ
			for (; size > (DWORD)(p - buf) && *p != '\x2'; p++)
				;
			if (size <= (DWORD)(p - buf)) {
				break;
			}
			p++;

#ifndef OPTION_SET
			if (*root == NULL) {
				*root = new_item;
			} else {
				di->next = new_item;
			}
			di = new_item;
			cdi = NULL;
#endif	// OPTION_SET

		default:
#ifndef OPTION_SET
			if (di == NULL) {
				return NULL;
			}

			// 形式毎のアイテムの作成
			if ((child_item = (DATA_INFO *)mem_calloc(sizeof(DATA_INFO))) == NULL) {
				message_get_error(GetLastError(), err_str);
				return NULL;
			}
			child_item->struct_size = sizeof(DATA_INFO);
			child_item->type = TYPE_DATA;
#endif	// OPTION_SET

			// ヘッダの読み込み
			// サイズ
			data_size = a2i(p);
#ifndef OPTION_SET
			child_item->size = data_size;
#endif	// OPTION_SET
			for (; size > (DWORD)(p - buf) && *p != '\0'; p++)
				;
			p++;
			// フォーマット
			if (size > (DWORD)(p - buf) && *p != '\x3') {
#ifndef OPTION_SET
				child_item->format_name = alloc_char_to_tchar(p);
				child_item->format_name_hash = str2hash(child_item->format_name);
				child_item->format = clipboard_get_format(0, child_item->format_name);
#endif	// OPTION_SET
				for (; size > (DWORD)(p - buf) && *p != '\0'; p++)
					;
				p++;
			}
			// ツール用文字列
			if (size > (DWORD)(p - buf) && *p != '\x3') {
#ifndef OPTION_SET
				child_item->plugin_string = alloc_char_to_tchar(p);
#endif	// OPTION_SET
				for (; size > (DWORD)(p - buf) && *p != '\0'; p++)
					;
				p++;
			}
			// ツール用long
			if (size > (DWORD)(p - buf) && *p != '\x3') {
#ifndef OPTION_SET
				child_item->plugin_param = a2i(p);
#endif	// OPTION_SET
				for (; size > (DWORD)(p - buf) && *p != '\0'; p++)
					;
				p++;
			}
			// オプション
			if (size > (DWORD)(p - buf) && *p != '\x3') {
#ifndef OPTION_SET
				if (*p != '\0') {
					file_expand_option(child_item, p);
				}
#endif	// OPTION_SET
				for (; size > (DWORD)(p - buf) && *p != '\0'; p++)
					;
				p++;
			}
			// データ開始位置までスキップ
			for (; size > (DWORD)(p - buf) && *p != '\x3'; p++)
				;
			if (size <= (DWORD)(p - buf)) {
				break;
			}
			p++;

			if (data_size > 0) {
#ifndef OPTION_SET
				// データ
				if ((mem = mem_alloc(data_size)) == NULL) {
					message_get_error(GetLastError(), err_str);
					data_free(child_item);
					return NULL;
				}
				CopyMemory(mem, p, data_size);
#endif	// OPTION_SET
				p += data_size;

#ifndef OPTION_SET
				if ((child_item->data = format_bytes_to_data(child_item->format_name, mem, &child_item->size)) == NULL) {
					child_item->data = clipboard_bytes_to_data(child_item->format_name, mem, &child_item->size);
				}
				mem_free(&mem);
#endif	// OPTION_SET
			}

#ifndef OPTION_SET
			// 形式の追加
			if (cdi == NULL) {
				di->child = child_item;
			} else {
				cdi->next = child_item;
			}
			cdi = child_item;
#endif	// OPTION_SET
			break;
		}
	}
	return p;
}

/*
 * file_read_data - ファイルを読み込んでアイテムリストに変換
 */
BOOL file_read_data(const TCHAR *path, DATA_INFO **root, TCHAR *err_str)
{
	BYTE *buf;
	DWORD size;

	// ファイルの読み込み
	if ((buf = file_read_buf(path, &size, err_str)) == NULL) {
		return FALSE;
	}
	if (file_file_to_item(buf, buf, size, root, 0, err_str) == NULL) {
		mem_free(&buf);
		return FALSE;
	}
	mem_free(&buf);
	return TRUE;
}

/*
 * file_item_to_file - アイテムリストをファイルに書き込む
 *
 *	\x1 title \0 modified \0 app_name \0 plugin_string \0 plugin_param \0 option \0		#アイテム
 *		\x2 size \0 format_name \0 plugin_string \0 plugin_param \0  option \0			#データヘッダ
 *			\x3 data																	#データ
 *		\x2 size \0 format_name \0 plugin_string \0 plugin_param \0  option \0
 *			\x3 data
 *
 *	\x4 title \0																		#フォルダ
 *		\x1 title \0 modified \0 app_name \0 plugin_string \0 plugin_param \0 option \0
 *			\x2 size \0 format_name \0 plugin_string \0 plugin_param \0 option \0
 *				\x3 data
 *			\x2 size \0 format_name \0 plugin_string \0 plugin_param \0 option \0
 *				\x3 data
 *	\x5																					#フォルダ終了マーク
 */
static BOOL file_item_to_file(const HANDLE hFile, DATA_INFO *di, TCHAR *err_str)
{
	DATA_INFO *cdi;
	BYTE *mem, *buf, *p;
	TCHAR str_date[BUF_SIZE];
	TCHAR str_size[BUF_SIZE];
	TCHAR str_param[BUF_SIZE];
	TCHAR str_op[BUF_SIZE];
	TCHAR *tp;
	DWORD len, size;
	DWORD ret;
	int i;

	// 保存文字列の作成
	for (; di != NULL; di = di->next) {
		if (di->type == TYPE_FOLDER) {
			// フォルダの書き込み
			len = 1;	// \x4
			if (di->title != NULL) {
				len += tchar_to_char_size(di->title);
			}
			len++;		// \0

			// 確保
			if ((p = buf = mem_alloc(len)) == NULL) {
				message_get_error(GetLastError(), err_str);
				return FALSE;
			}

			// アイテム開始マーク
			*(p++) = '\x4';
			// タイトル
			if (di->title != NULL) {
				tchar_to_char(di->title, p, tchar_to_char_size(di->title));
				p += tchar_to_char_size(di->title);
			}
			*(p++) = '\0';

			// 書き込み
			if (WriteFile(hFile, buf, len, &ret, NULL) == FALSE) {
				message_get_error(GetLastError(), err_str);
				mem_free(&buf);
				return FALSE;
			}
			mem_free(&buf);

			if (file_item_to_file(hFile, di->child, err_str) == FALSE) {
				return FALSE;
			}
			// 書き込み
			if (WriteFile(hFile, "\x5", 1, &ret, NULL) == FALSE) {
				message_get_error(GetLastError(), err_str);
				return FALSE;
			}
			continue;
		}

		if (di->type != TYPE_ITEM || di->child == NULL) {
			continue;
		}
		// サイズ取得
		len = 1;	// \x1
		if (di->title != NULL) {
			len += tchar_to_char_size(di->title);
		}
		len++;		// \0
		len += 16;
		len++;		// \0
		if (di->window_name != NULL) {
			len += tchar_to_char_size(di->window_name);
		}
		len++;		// \0
		if (di->plugin_string != NULL) {
			len += tchar_to_char_size(di->plugin_string);
		}
		len++;		// \0
		_itot_s(di->plugin_param, str_param, BUF_SIZE, 10);
		len += tchar_to_char_size(str_param);
		len++;		// \0
		tp = str_op;
		_itot_s(di->op_modifiers, tp, BUF_SIZE, 10);
		tp += lstrlen(tp);
		*(tp++) = TEXT(',');
		_itot_s(di->op_virtkey, tp, BUF_SIZE - (tp - str_op), 10);
		tp += lstrlen(tp);
		*(tp++) = TEXT(',');
		_itot_s(di->op_paste, tp, BUF_SIZE - (tp - str_op), 10);
		len += tchar_to_char_size(str_op);
		len++;		// \0
		len++;		// \x2

		// 確保
		if ((p = buf = mem_alloc(len)) == NULL) {
			message_get_error(GetLastError(), err_str);
			return FALSE;
		}

		// アイテム開始マーク
		*(p++) = '\x1';

		// タイトル
		if (di->title != NULL) {
			tchar_to_char(di->title, p, tchar_to_char_size(di->title));
			p += tchar_to_char_size(di->title);
		}
		*(p++) = '\0';
		// 更新日時
		_itot_s(di->modified.dwHighDateTime, str_date, BUF_SIZE, 16);
		for (i = 0; i < 8 - lstrlen(str_date); i++) {
			*(p++) = '0';
		}
		tchar_to_char(str_date, p, tchar_to_char_size(str_date));
		p += lstrlen(str_date);
		_itot_s(di->modified.dwLowDateTime, str_date, BUF_SIZE, 16);
		for (i = 0; i < 8 - lstrlen(str_date); i++) {
			*(p++) = '0';
		}
		tchar_to_char(str_date, p, tchar_to_char_size(str_date));
		p += lstrlen(str_date);
		*(p++) = '\0';
		// ウィンドウ名
		if (di->window_name != NULL) {
			tchar_to_char(di->window_name, p, tchar_to_char_size(di->window_name));
			p += tchar_to_char_size(di->window_name);
		}
		*(p++) = '\0';
		// ツール用文字列
		if (di->plugin_string != NULL) {
			tchar_to_char(di->plugin_string, p, tchar_to_char_size(di->plugin_string));
			p += tchar_to_char_size(di->plugin_string);
		}
		*(p++) = '\0';
		// ツール用long
		tchar_to_char(str_param, p, tchar_to_char_size(str_param));
		p += tchar_to_char_size(str_param);
		*(p++) = '\0';
		// オプション
		tchar_to_char(str_op, p, tchar_to_char_size(str_op));
		p += tchar_to_char_size(str_op);
		*(p++) = '\0';
		// ヘッダ開始マーク
		*(p++) = '\x2';

		// 書き込み
		if (WriteFile(hFile, buf, len, &ret, NULL) == FALSE) {
			message_get_error(GetLastError(), err_str);
			mem_free(&buf);
			return FALSE;
		}
		mem_free(&buf);

		for (cdi = di->child; cdi != NULL; cdi = cdi->next) {
			// サイズ取得
			len = 0;
			// データ取得
			if ((mem = format_data_to_bytes(cdi, &size)) == NULL) {
				mem = clipboard_data_to_bytes(cdi, &size);
			}
			if (mem != NULL) {
				// サイズ
				_itot_s(size, str_size, BUF_SIZE, 10);
				len += tchar_to_char_size(str_size);
				len++;		// \0
				len += size;
			} else {
				*str_size = TEXT('0');
				*(str_size + 1) = TEXT('\0');
				len++;		// 0
				len++;		// \0
			}
			if (cdi->format_name != NULL) {
				len += tchar_to_char_size(cdi->format_name);
			}
			len++;			// \0
			if (cdi->plugin_string != NULL) {
				len += tchar_to_char_size(cdi->plugin_string);
			}
			len++;			// \0
			_itot_s(cdi->plugin_param, str_param, BUF_SIZE, 10);
			len += tchar_to_char_size(str_param);
			len++;			// \0
			tp = str_op;
			_itot_s(cdi->op_modifiers, tp, BUF_SIZE, 10);
			tp += lstrlen(tp);
			*(tp++) = TEXT(',');
			_itot_s(cdi->op_virtkey, tp, BUF_SIZE - (tp - str_op), 10);
			tp += lstrlen(tp);
			*(tp++) = TEXT(',');
			_itot_s(cdi->op_paste, tp, BUF_SIZE - (tp - str_op), 10);
			len += tchar_to_char_size(str_op);
			len++;			// \0
			len++;			// \x3

			// 確保
			if ((p = buf = mem_alloc(len)) == NULL) {
				message_get_error(GetLastError(), err_str);
				return FALSE;
			}

			// ヘッダ作成
			// サイズ
			tchar_to_char(str_size, p, tchar_to_char_size(str_size));
			p += tchar_to_char_size(str_size);
			*(p++) = '\0';
			// フォーマット名
			if (cdi->format_name != NULL) {
				tchar_to_char(cdi->format_name, p, tchar_to_char_size(cdi->format_name));
				p += tchar_to_char_size(cdi->format_name);
			}
			*(p++) = '\0';
			// ツール用文字列
			if (cdi->plugin_string != NULL) {
				tchar_to_char(cdi->plugin_string, p, tchar_to_char_size(cdi->plugin_string));
				p += tchar_to_char_size(cdi->plugin_string);
			}
			*(p++) = '\0';
			// ツール用long
			tchar_to_char(str_param, p, tchar_to_char_size(str_param));
			p += tchar_to_char_size(str_param);
			*(p++) = '\0';
			// オプション
			tchar_to_char(str_op, p, tchar_to_char_size(str_op));
			p += tchar_to_char_size(str_op);
			*(p++) = '\0';
			// データ開始マーク
			*(p++) = '\x3';

			// データ
			if (mem != NULL) {
				CopyMemory(p, mem, size);
				mem_free(&mem);
			}

			// 書き込み
			if (WriteFile(hFile, buf, len, &ret, NULL) == FALSE) {
				message_get_error(GetLastError(), err_str);
				mem_free(&buf);
				return FALSE;
			}
			mem_free(&buf);
		}
	}
	return TRUE;
}

/*
 * file_write_data - アイテムリストファイルの作成
 */
BOOL file_write_data(const TCHAR *path, DATA_INFO *di, TCHAR *err_str)
{
	HANDLE hFile;

	TCHAR tmp_path[MAX_PATH];

	wsprintf(tmp_path, TEXT("%s.tmp"), path);
	DeleteFile(tmp_path);

	// 保存
	hFile = CreateFile(tmp_path, GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == NULL || hFile == (HANDLE)-1) {
		message_get_error(GetLastError(), err_str);
		return FALSE;
	}
	if (file_item_to_file(hFile, di, err_str) == FALSE) {
		CloseHandle(hFile);
		DeleteFile(tmp_path);
		return FALSE;
	}
	CloseHandle(hFile);

	CopyFile(tmp_path, path, FALSE);
	DeleteFile(tmp_path);
	return TRUE;
}

/*
 * shell_open - ファイルを実行
 */
BOOL shell_open(const TCHAR *file_name, const TCHAR *command_line)
{
	SHELLEXECUTEINFO sei;

	ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));
	sei.cbSize = sizeof(sei);
	sei.fMask = 0;
	sei.hInstApp = hInst;
	sei.hwnd = NULL;
	sei.lpVerb = NULL;
	sei.lpFile = file_name;
	sei.lpParameters = command_line;
	sei.lpDirectory = NULL;
	sei.nShow = SW_SHOWNORMAL;
	return ShellExecuteEx(&sei);
}
/* End of source */
