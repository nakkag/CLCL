/*
 * CLCL
 *
 * fmt_rtf.c
 *
 * Copyright (C) 1996-2003 by Nakashima Tomoaki. All rights reserved.
 *		http://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE
#include <richedit.h>

#include "..\CLCLPlugin.h"
#include "fmt_rtf_view.h"
#include "Memory.h"
#include "String.h"
#include "Message.h"

#include "resource.h"

/* Define */
#define TOOLTIP_SIZE			1024

/* Global Variables */
HINSTANCE hInst;

static HANDLE hLib;
static HICON txt_icon;
static HWND hTxtWnd;

/* Local Function Prototypes */

/*
 * DllMain - メイン
 */
int WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved)
{
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		hInst = hInstance;
		break;

	case DLL_PROCESS_DETACH:
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
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
	size = GetFileSize(hFile, NULL);
	if (size == 0xFFFFFFFF) {
		message_get_error(GetLastError(), err_str);
		CloseHandle(hFile);
		return NULL;
	}

	buf = (BYTE *)mem_alloc(size + 1);
	if (buf == NULL) {
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
	CloseHandle(hFile);
	return TRUE;
}

/*
 * x2c - 16進文字列を数値(TCHAR)に変換する
 */
static char x2c(const char *str)
{
	char ret = 0;
	char num;
	int i;

	if (str == NULL) {
		return 0;
	}
	for (i = 0; i < 2 && *str != '\0'; i++, str++) {
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
 * del_rtf - RTF情報の削除
 */
static void del_rtf(char *buf)
{
	char *p, *r;
	char tmp[BUF_SIZE];
	int len = strlen(buf);
	int qt = 0, q;

	p = buf;
	while (*p != '\0') {
		switch (*p) {
		case '\r':
		case '\n':
			// CRLFをスキップ
			for (r = p; *r == '\r' || *r == '\n'; r++)
				;
			MoveMemory(p, r, sizeof(char) * (len - (r - buf) + 1));
			len -= r - p;
			break;

		case '{':
			qt++;
			if (qt > 1) {
				// {...} を削除
				q = 1;
				for (r = p + 1; *r != '\0'; r++) {
					if (*r == '\\' && *(r + 1) != '\0') {
						r++;
					} else if (*r == '{') {
						q++;
					} else if (*r == '}') {
						q--;
						if (q == 0) {
							r++;
							break;
						}
					}
				}
				MoveMemory(p, r, sizeof(char) * (len - (r - buf) + 1));
				len -= r - p;
			} else {
				MoveMemory(p, p + 1, sizeof(char) * (len - (p - buf + 1) + 1));
				len--;
			}
			break;

		case '}':
			qt--;
			MoveMemory(p, p + 1, sizeof(char) * (len - (p - buf + 1) + 1));
			len--;
			break;

		case '\\':
			r = p + 1;
			if (*r == '\\' || *r == '{' || *r == '}') {
				// \\, {, }
				MoveMemory(p, r, sizeof(char) * (len - (r - buf) + 1));
				len--;
				p++;
			} else if (*r == '\'') {
				// char
				r++;
				*(p++) = x2c(r);
				r += 2;
				MoveMemory(p, r, sizeof(char) * (len - (r - buf) + 1));
				len -= r - p;
			} else {
				for (; *r != '\0' && *r != '\\' &&
					*r != '{' && *r != '}' &&
					*r != '\r' && *r != '\n' && *r != ' '; r++)
					;
				if (r - p + 1 < BUF_SIZE) {
					strncpy(tmp, p, r - p + 1);
					if (stricmp(tmp, "\\tab") == 0) {
						// TAB
						*(p++) = '\t';
					} else if (stricmp(tmp, "\\par") == 0) {
						// CRLF
						*(p++) = '\r';
						*(p++) = '\n';
					}
				}
				if (*r == ' ') {
					r++;
				}
				MoveMemory(p, r, sizeof(char) * (len - (r - buf) + 1));
				len -= r - p;
			}
			break;

		default:
			p++;
			break;
		}
	}
}

/*
 * get_format_header - 内部形式を処理するヘッダの取得
 */
__declspec(dllexport) BOOL CALLBACK get_format_header(const HWND hWnd, const int index, FORMAT_GET_INFO *fgi)
{
	switch (index) {
	case 0:
		lstrcpy(fgi->format_name, TEXT("Rich Text Format"));
		lstrcpy(fgi->func_header, TEXT("rtf_"));
		lstrcpy(fgi->comment, TEXT("Rich Text Format"));
		return TRUE;
	}
	return FALSE;
}

/*
 * rtf_show_property - プロパティ表示
 */
__declspec(dllexport) BOOL CALLBACK rtf_show_property(const HWND hWnd)
{
	return FALSE;
}

/*
 * rtf_initialize - 初期化
 */
__declspec(dllexport) BOOL CALLBACK rtf_initialize(void)
{
	if (hLib == NULL) {
		hLib = LoadLibrary(TEXT("RICHED32.DLL"));
	}
	if (txt_icon == NULL) {
		txt_icon = LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON_TEXT), IMAGE_ICON, 16, 16, 0);
	}
	rtfview_regist(hInst);
	return TRUE;
}

/*
 * rtf_get_icon - 形式用のアイコンを取得
 */
__declspec(dllexport) HICON CALLBACK rtf_get_icon(const int icon_size, BOOL *free_icon)
{
	return LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON_TEXT), IMAGE_ICON, icon_size, icon_size, 0);
}

/*
 * rtf_free - 終了処理
 */
__declspec(dllexport) BOOL CALLBACK rtf_free(void)
{
	if (hLib != NULL) {
		FreeLibrary(hLib);
		hLib = NULL;
	}
	if (txt_icon != NULL) {
		DestroyIcon(txt_icon);
		txt_icon = NULL;
	}
	return TRUE;
}

/*
 * rtf_initialize_item - アイテム情報の初期化
 */
__declspec(dllexport) BOOL CALLBACK rtf_initialize_item(DATA_INFO *di, const BOOL set_init_data)
{
	return FALSE;
}

/*
 * rtf_copy_data - データのコピー
 */
__declspec(dllexport) HANDLE CALLBACK rtf_copy_data(const TCHAR *format_name, const HANDLE data, DWORD *ret_size)
{
	HANDLE ret;
	BYTE *from_mem, *to_mem;
	BYTE *p;

	// チェック
	if (data == NULL || IsBadReadPtr(data, 1) == TRUE) {
		return NULL;
	}
	// サイズ取得
	if ((*ret_size = GlobalSize(data)) == 0) {
		return NULL;
	}
	// コピー元ロック
	if ((from_mem = GlobalLock(data)) == NULL) {
		return NULL;
	}

	for (p = from_mem; *ret_size > (DWORD)(p - from_mem) && *p != '\0'; p++)
		;
	*ret_size = p - from_mem + 1;

	// コピー先確保
	if ((ret = GlobalAlloc(GHND, *ret_size)) == NULL) {
		GlobalUnlock(data);
		return NULL;
	}
	// コピー先ロック
	if ((to_mem = GlobalLock(ret)) == NULL) {
		GlobalFree(ret);
		GlobalUnlock(data);
		return NULL;
	}

	// コピー
	CopyMemory(to_mem, from_mem, *ret_size);
	*(to_mem + *ret_size - 1) = '\0';

	// ロック解除
	GlobalUnlock(ret);
	GlobalUnlock(data);
	return ret;
}

/*
 * rtf_data_to_bytes - データをバイト列に変換
 */
__declspec(dllexport) BYTE* CALLBACK rtf_data_to_bytes(const DATA_INFO *di, DWORD *ret_size)
{
	BYTE *mem;
	BYTE *ret;

	if (di->data == NULL || (mem = GlobalLock(di->data)) == NULL) {
		return NULL;
	}
	ret = mem_alloc(di->size);
	if (ret == NULL) {
		GlobalUnlock(di->data);
		return NULL;
	}
	CopyMemory(ret, mem, di->size);
	if (ret_size != NULL) {
		*ret_size = di->size;
	}
	GlobalUnlock(di->data);
	return ret;
}

/*
 * rtf_bytes_to_data - バイト列をデータに変換
 */
__declspec(dllexport) HANDLE CALLBACK rtf_bytes_to_data(const TCHAR *format_name, const BYTE *data, DWORD *size)
{
	BYTE *ret;
	BYTE *mem;

	// コピー先確保
	if (data == NULL || (ret = GlobalAlloc(GHND, *size)) == NULL) {
		return NULL;
	}
	// コピー先ロック
	if ((mem = GlobalLock(ret)) == NULL) {
		GlobalFree(ret);
		return NULL;
	}
	// コピー
	CopyMemory(mem, data, *size);
	// ロック解除
	GlobalUnlock(ret);
	return ret;
}

/*
 * rtf_get_file_info - コモンダイアログ情報の取得
 */
__declspec(dllexport) int CALLBACK rtf_get_file_info(const TCHAR *format_name, const DATA_INFO *di, OPENFILENAME *of, const BOOL mode)
{
	of->lpstrFilter = TEXT("*.rtf\0*.rtf\0*.*\0*.*\0\0");
	of->nFilterIndex = 1;
	of->lpstrDefExt = TEXT("rtf");
	return 1;
}

/*
 * rtf_data_to_file - データをファイルに保存
 */
__declspec(dllexport) BOOL CALLBACK rtf_data_to_file(DATA_INFO *di, const TCHAR *file_name, const int filter_index, TCHAR *err_str)
{
	BYTE *tmp;

	if (di->data == NULL) {
		return FALSE;
	}
	if ((tmp = GlobalLock(di->data)) == NULL) {
		message_get_error(GetLastError(), err_str);
		return FALSE;
	}
	// ファイルに書き込む
	if (file_write_buf(file_name, tmp, di->size - 1, err_str) == FALSE) {
		GlobalUnlock(di->data);
		return FALSE;
	}
	GlobalUnlock(di->data);
	return TRUE;
}

/*
 * rtf_file_to_data - ファイルからデータを作成
 */
__declspec(dllexport) HANDLE CALLBACK rtf_file_to_data(const TCHAR *file_name, const TCHAR *format_name, DWORD *ret_size, TCHAR *err_str)
{
	HANDLE ret = NULL;
	BYTE *data;
	BYTE *mem;
	DWORD size;

	// ファイルの読み込み
	data = file_read_buf(file_name, &size, err_str);
	if (data == NULL) {
		return NULL;
	}
	// コピー先確保
	if ((ret = GlobalAlloc(GHND, size + 1)) == NULL) {
		message_get_error(GetLastError(), err_str);
		mem_free(&data);
		return NULL;
	}
	// コピー先ロック
	if ((mem = GlobalLock(ret)) == NULL) {
		message_get_error(GetLastError(), err_str);
		GlobalFree(ret);
		mem_free(&data);
		return NULL;
	}
	// コピー
	CopyMemory(mem, data, size);
	*(mem + size) = '\0';
	if (ret_size != NULL) {
		*ret_size = size;
	}
	// ロック解除
	GlobalUnlock(ret);
	mem_free(&data);
	return ret;
}

/*
 * rtf_free_data - データの解放
 */
__declspec(dllexport) BOOL CALLBACK rtf_free_data(const TCHAR *format_name, HANDLE data)
{
	if (data == NULL) {
		return TRUE;
	}
	if (GlobalFree((HGLOBAL)data) != NULL) {
		return FALSE;
	}
	return TRUE;
}

/*
 * rtf_free_item - アイテム情報の解放
 */
__declspec(dllexport) BOOL CALLBACK rtf_free_item(DATA_INFO *di)
{
	return FALSE;
}

/*
 * rtf_get_menu_title - メニュータイトルの取得
 */
__declspec(dllexport) BOOL CALLBACK rtf_get_menu_title(DATA_INFO *di)
{
	TCHAR buf[BUF_SIZE];
	TCHAR tmp[BUF_SIZE];
	BYTE *mem;
	TCHAR *p, *r;
	char *cp;

	if (di->data == NULL || (mem = GlobalLock(di->data)) == NULL) {
		return FALSE;
	}
	if (*mem == '\0') {
		GlobalUnlock(di->data);
		return TRUE;
	}

	// メニュー用文字列
	cp = mem_alloc(di->size);
	CopyMemory(cp, mem, di->size);
	GlobalUnlock(di->data);

	del_rtf(cp);
	char_to_tchar(cp, buf, strlen(cp));
	mem_free(&cp);

	lstrcpy(tmp, buf);
	r = buf;
	for (p = tmp; *p == TEXT(' ') || *p == TEXT('\t') || *p == TEXT('\r') || *p == TEXT('\n'); p++)
		;
	while ((r - buf) < (BUF_SIZE - 4) && *p != TEXT('\0')) {
		if (*p == TEXT('\r') || *p == TEXT('\n')) {
			lstrcpy(r, TEXT("..."));
			r += 3;
			break;
		}
		switch (*p) {
		case TEXT(' '):
		case TEXT('\t'):
			(*r++) = TEXT(' ');
			for (; *p == TEXT(' ') || *p == TEXT('\t'); p++)
				;
			break;

		case TEXT('&'):
			(*r++) = TEXT('&');
			(*r++) = *(p++);
			break;

		default:
			(*r++) = *(p++);
			break;
		}
	}
	*r = TEXT('\0');

	if (*buf != TEXT('\0')) {
		di->menu_title = alloc_copy(buf);
		di->free_title = TRUE;
	}
	return TRUE;
}

/*
 * rtf_get_menu_icon - メニュー用アイコンの取得
 */
__declspec(dllexport) BOOL CALLBACK rtf_get_menu_icon(DATA_INFO *di, const int icon_size)
{
	di->menu_icon = txt_icon;
	di->free_icon = FALSE;
	return TRUE;
}

/*
 * rtf_get_menu_bitmap - メニュー用ビットマップの取得
 */
__declspec(dllexport) BOOL CALLBACK rtf_get_menu_bitmap(DATA_INFO *di, const int width, const int height)
{
	return FALSE;
}

/*
 * rtf_get_tooltip_text - メニュー用ツールチップテキスト
 */
__declspec(dllexport) TCHAR* CALLBACK rtf_get_tooltip_text(DATA_INFO *di)
{
	TCHAR *ret, *p;
	char *cp;
	BYTE *mem;

	if (di->data == NULL || (mem = GlobalLock(di->data)) == NULL) {
		return NULL;
	}
	if (*mem == '\0') {
		GlobalUnlock(di->data);
		return NULL;
	}
	ret = mem_alloc(sizeof(TCHAR) * TOOLTIP_SIZE);
	if (ret == NULL) {
		GlobalUnlock(di->data);
		return NULL;
	}
	cp = mem_alloc(di->size);
	CopyMemory(cp, mem, di->size);
	GlobalUnlock(di->data);
	del_rtf(cp);
	p = alloc_char_to_tchar(cp);
	mem_free(&cp);
	lstrcpyn(ret, p, TOOLTIP_SIZE);
	mem_free(&p);
	return ret;
}

/*
 * rtf_window_create - データ表示ウィンドウの作成
 */
__declspec(dllexport) HWND CALLBACK rtf_window_create(const HWND parent_wnd)
{
	if (hTxtWnd == NULL) {
		hTxtWnd = rtfview_create(hInst, parent_wnd, 0);
	}
	return hTxtWnd;
}

/*
 * rtf_window_destroy - データ表示ウィンドウの破棄
 */
__declspec(dllexport) BOOL CALLBACK rtf_window_destroy(const HWND hWnd)
{
	hTxtWnd = NULL;
	return TRUE;
}

/*
 * rtf_window_show_data - データの表示
 */
__declspec(dllexport) BOOL CALLBACK rtf_window_show_data(const HWND hWnd, DATA_INFO *di, const BOOL lock)
{
	BYTE *mem;

	if (di->data == NULL) {
		SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM)TEXT(""));
	} else {
		if ((mem = GlobalLock(di->data)) == NULL) {
			return FALSE;
		}
		SendMessage(hWnd, WM_SETRTF, 0, (LPARAM)mem);
		GlobalUnlock(di->data);
	}

	if (lock == TRUE) {
		SendMessage(hWnd, EM_SETOPTIONS, ECOOP_OR, ECO_READONLY);
	} else {
		SendMessage(hWnd, EM_SETOPTIONS, ECOOP_AND, ~ECO_READONLY);
	}
	SendMessage(hWnd, EM_SETMODIFY, FALSE, 0);
	return TRUE;
}

/*
 * rtf_window_save_data - データの保存
 */
__declspec(dllexport) BOOL CALLBACK rtf_window_save_data(const HWND hWnd, DATA_INFO *di)
{
	HANDLE data;
	BYTE *to_mem;
	DWORD size;
	HLOCAL hloc;
	char *from_mem;

	if (hWnd == NULL ||
		(SendMessage(hWnd, EM_GETOPTIONS, 0, 0) & ECO_READONLY) ||
		SendMessage(hWnd, EM_GETMODIFY, 0, 0) == FALSE) {
		return FALSE;
	}
	if (di->data != NULL) {
		GlobalFree(di->data);
		di->data = NULL;
		di->size = 0;
	}

	// 現在表示されている内容の取得
	if ((hloc = (HLOCAL)SendMessage(hWnd, WM_GETRTF, 0, (LPARAM)&size)) == NULL) {
		return FALSE;
	}
	if ((from_mem = LocalLock(hloc)) == NULL) {
		LocalFree(hloc);
		return FALSE;
	}
	// データの作成
	if ((data = GlobalAlloc(GHND, size + 1)) == NULL) {
		LocalUnlock(hloc);
		LocalFree(hloc);
		return FALSE;
	}
	if ((to_mem = GlobalLock(data)) == NULL) {
		GlobalFree(data);
		LocalUnlock(hloc);
		LocalFree(hloc);
		return FALSE;
	}
	strcpy(to_mem, from_mem);
	GlobalUnlock(data);
	LocalUnlock(hloc);
	LocalFree(hloc);

	// 新しいデータを設定
	di->data = data;
	di->size = size + 1;

	SendMessage(hWnd, EM_SETMODIFY, FALSE, 0);
	return TRUE;
}

/*
 * rtf_window_hide_data - データの非表示
 */
__declspec(dllexport) BOOL CALLBACK rtf_window_hide_data(const HWND hWnd, DATA_INFO *di)
{
	SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM)TEXT(""));
	return TRUE;
}
/* End of source */
