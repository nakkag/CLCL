/*
 * CLCL
 *
 * fmt_Text.c
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE

#include "General.h"
#include "Memory.h"
#include "String.h"
#include "Message.h"
#include "Data.h"
#include "Ini.h"
#include "File.h"
#include "fmt_text_view.h"
#include "dpi.h"

#include "resource.h"

/* Define */

/* Global Variables */
static HICON txt_icon;
static HWND hTxtWnd;

extern HINSTANCE hInst;
extern OPTION_INFO option;

/* Local Function Prototypes */

/*
 * text_initialize - 初期化
 */
__declspec(dllexport) BOOL CALLBACK text_initialize(void)
{
	if (txt_icon == NULL) {
		txt_icon = LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON_TEXT), IMAGE_ICON, Scale(16), Scale(16), 0);
	}
	txtview_regist(hInst);
	return TRUE;
}

/*
 * text_get_icon - 形式用のアイコンを取得
 */
__declspec(dllexport) HICON CALLBACK text_get_icon(const int icon_size, BOOL *free_icon)
{
	*free_icon = FALSE;
	return txt_icon;
}

/*
 * text_free - 終了処理
 */
__declspec(dllexport) BOOL CALLBACK text_free(void)
{
	if (txt_icon != NULL) {
		DestroyIcon(txt_icon);
		txt_icon = NULL;
	}
	return TRUE;
}

/*
 * text_initialize_item - アイテム情報の初期化
 */
__declspec(dllexport) BOOL CALLBACK text_initialize_item(DATA_INFO *di, const BOOL set_init_data)
{
	return FALSE;
}

/*
 * text_copy_data - データのコピー
 */
__declspec(dllexport) HANDLE CALLBACK text_copy_data(const TCHAR *format_name, const HANDLE data, DWORD *ret_size)
{
	return NULL;
}

/*
 * text_data_to_bytes - データをバイト列に変換
 */
__declspec(dllexport) BYTE* CALLBACK text_data_to_bytes(const DATA_INFO *di, DWORD *ret_size)
{
	return NULL;
}

/*
 * text_bytes_to_data - バイト列をデータに変換
 */
__declspec(dllexport) HANDLE CALLBACK text_bytes_to_data(const TCHAR *format_name, const BYTE *data, DWORD *size)
{
	return NULL;
}

/*
 * text_get_file_info - コモンダイアログ情報の取得
 */
__declspec(dllexport) int CALLBACK text_get_file_info(const TCHAR *format_name, const DATA_INFO *di, OPENFILENAME *of, const BOOL mode)
{
	of->lpstrFilter = TEXT("*.txt\0*.txt\0*.*\0*.*\0\0");
	of->nFilterIndex = 1;
	of->lpstrDefExt = TEXT("txt");
	if (di != NULL && di->menu_title != NULL && *di->menu_title != TEXT('\0')) {
		TCHAR *p;

		lstrcpyn(of->lpstrFile, di->menu_title, MAX_PATH - 5);
		for (p = of->lpstrFile + lstrlen(of->lpstrFile) - 1; p > of->lpstrFile; p--) {
			if (*p == TEXT('.')) {
				*p = TEXT('\0');
			}
		}
		lstrcat(of->lpstrFile, TEXT(".txt"));
		file_name_conv(of->lpstrFile, TEXT('_'));
	}
	return 1;
}

/*
 * text_data_to_file - データをファイルに保存
 */
__declspec(dllexport) BOOL CALLBACK text_data_to_file(DATA_INFO *di, const TCHAR *file_name, const int filter_index, TCHAR *err_str)
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
	if (di->format == CF_UNICODETEXT) {
		if (file_write_buf(file_name, tmp, di->size - sizeof(WCHAR), err_str) == FALSE) {
			GlobalUnlock(di->data);
			return FALSE;
		}
	} else {
		if (file_write_buf(file_name, tmp, di->size - 1, err_str) == FALSE) {
			GlobalUnlock(di->data);
			return FALSE;
		}
	}
	GlobalUnlock(di->data);
	return TRUE;
}

/*
 * text_file_to_data - ファイルからデータを作成
 */
__declspec(dllexport) HANDLE CALLBACK text_file_to_data(const TCHAR *file_name, const TCHAR *format_name, DWORD *ret_size, TCHAR *err_str)
{
	HANDLE ret = NULL;
	BYTE *data;
	BYTE *mem;
	DWORD size;

	// ファイルの読み込み
	if ((data = file_read_buf(file_name, &size, err_str)) == NULL) {
		return NULL;
	}
	// コピー先確保
	if (lstrcmp(format_name, TEXT("UNICODE TEXT")) == 0) {
		if ((ret = GlobalAlloc(GHND, size + sizeof(WCHAR))) == NULL) {
			message_get_error(GetLastError(), err_str);
			mem_free(&data);
			return NULL;
		}
	} else {
		if ((ret = GlobalAlloc(GHND, size + 1)) == NULL) {
			message_get_error(GetLastError(), err_str);
			mem_free(&data);
			return NULL;
		}
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
	if (lstrcmp(format_name, TEXT("UNICODE TEXT")) == 0) {
		*((WCHAR *)mem + (size / sizeof(WCHAR))) = L'\0';
		if (ret_size != NULL) {
			*ret_size = size + sizeof(WCHAR);
		}
	} else {
		*(mem + size) = '\0';
		if (ret_size != NULL) {
			*ret_size = size;
		}
	}
	// ロック解除
	GlobalUnlock(ret);
	mem_free(&data);
	return ret;
}

/*
 * text_free_data - データの解放
 */
__declspec(dllexport) BOOL CALLBACK text_free_data(const TCHAR *format_name, HANDLE data)
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
 * text_free_item - アイテム情報の解放
 */
__declspec(dllexport) BOOL CALLBACK text_free_item(DATA_INFO *di)
{
	return FALSE;
}

/*
 * text_get_menu_title - メニュータイトルの取得
 */
__declspec(dllexport) BOOL CALLBACK text_get_menu_title(DATA_INFO *di)
{
	TCHAR buf[BUF_SIZE];
	TCHAR tmp[BUF_SIZE];
	BYTE *mem;
	TCHAR *p, *r;
	int size;

	if (di->data == NULL || (mem = GlobalLock(di->data)) == NULL) {
		return FALSE;
	}
	if (di->format == CF_UNICODETEXT) {
		if (*(WCHAR *)mem == L'\0') {
			GlobalUnlock(di->data);
			return TRUE;
		}
	} else if (*mem == '\0') {
		GlobalUnlock(di->data);
		return TRUE;
	}

	// メニュー用文字列
	if (di->format == CF_UNICODETEXT) {
#ifdef UNICODE
		p = (WCHAR *)mem;
#else
		WideCharToMultiByte(CP_ACP, 0, (WCHAR *)mem, di->size / sizeof(WCHAR), tmp, BUF_SIZE - 1, NULL, NULL);
		p = tmp;
#endif
		size = di->size / sizeof(WCHAR);
	} else {
#ifdef UNICODE
		MultiByteToWideChar(CP_ACP, 0, mem, di->size, tmp, BUF_SIZE - 1);
		p = tmp;
#else
		p = mem;
#endif
		size = di->size;
	}

	for (r = buf; size > 0 && (*p == TEXT(' ') || *p == TEXT('\t') || *p == TEXT('\r') || *p == TEXT('\n')); p++, size--)
		;
	while ((r - buf) < (BUF_SIZE - 4) && size > 0 && *p != TEXT('\0')) {
		if (*p == TEXT('\r') || *p == TEXT('\n')) {
			lstrcpy(r, TEXT("..."));
			r += 3;
			break;
		}
		switch (*p) {
		case TEXT(' '):
		case TEXT('\t'):
			(*r++) = TEXT(' ');
			for (; size > 0 && (*p == TEXT(' ') || *p == TEXT('\t')); p++, size--)
				;
			break;

		case TEXT('&'):
			(*r++) = TEXT('&');
			(*r++) = *(p++);
			size--;
			break;

		default:
			(*r++) = *(p++);
			size--;
			break;
		}
	}
	*r = TEXT('\0');
	GlobalUnlock(di->data);

	if (*buf != TEXT('\0')) {
		di->menu_title = alloc_copy(buf);
		di->free_title = TRUE;
	}
	return TRUE;
}

/*
 * text_get_menu_icon - メニュー用アイコンの取得
 */
__declspec(dllexport) BOOL CALLBACK text_get_menu_icon(DATA_INFO *di, const int icon_size)
{
	di->menu_icon = txt_icon;
	di->free_icon = FALSE;
	return TRUE;
}

/*
 * text_get_menu_bitmap - メニュー用ビットマップの取得
 */
__declspec(dllexport) BOOL CALLBACK text_get_menu_bitmap(DATA_INFO *di, const int width, const int height)
{
	return FALSE;
}

/*
 * text_get_tooltip_text - メニュー用ツールチップテキスト
 */
__declspec(dllexport) TCHAR* CALLBACK text_get_tooltip_text(DATA_INFO *di)
{
	TCHAR *ret;
	BYTE *mem;
	int len;

	if (di->data == NULL || (mem = GlobalLock(di->data)) == NULL) {
		return NULL;
	}
	if (di->format == CF_UNICODETEXT) {
		if (*(WCHAR *)mem == L'\0') {
			GlobalUnlock(di->data);
			return NULL;
		}
	} else if (*mem == '\0') {
		GlobalUnlock(di->data);
		return NULL;
	}
	if ((ret = mem_alloc(sizeof(TCHAR) * (option.fmt_txt_menu_tooltip_size + 1))) == NULL) {
		GlobalUnlock(di->data);
		return NULL;
	}
	if (di->format == CF_UNICODETEXT) {
#ifdef UNICODE
		len = (di->size / sizeof(WCHAR) + 1 < (DWORD)option.fmt_txt_menu_tooltip_size + 1) ?
			di->size / sizeof(WCHAR) + 1 : option.fmt_txt_menu_tooltip_size + 1;
		lstrcpyn(ret, (WCHAR *)mem, len);
#else
		len = (di->size / sizeof(WCHAR) < (DWORD)option.fmt_txt_menu_tooltip_size) ?
			di->size / sizeof(WCHAR) : option.fmt_txt_menu_tooltip_size;
		WideCharToMultiByte(CP_ACP, 0, (WCHAR *)mem, di->size / sizeof(WCHAR), ret, option.fmt_txt_menu_tooltip_size, NULL, NULL);
		*(ret + len) = '\0';
#endif
	} else {
#ifdef UNICODE
		len = (di->size < (DWORD)option.fmt_txt_menu_tooltip_size) ?
			di->size : option.fmt_txt_menu_tooltip_size;
		MultiByteToWideChar(CP_ACP, 0, mem, di->size, ret, option.fmt_txt_menu_tooltip_size);
		*(ret + len) = L'\0';
#else
		len = (di->size + 1 < (DWORD)option.fmt_txt_menu_tooltip_size + 1) ?
			di->size + 1 : option.fmt_txt_menu_tooltip_size + 1;
		lstrcpyn(ret, mem, len);
#endif
	}
	GlobalUnlock(di->data);
	return ret;
}

/*
 * text_window_create - データ表示ウィンドウの作成
 */
__declspec(dllexport) HWND CALLBACK text_window_create(const HWND parent_wnd)
{
	DWORD tab[1];

	if (hTxtWnd == NULL) {
		hTxtWnd = CreateWindowEx(WS_EX_CLIENTEDGE, NEDIT_WND_CLASS, NULL,
			WS_TABSTOP | WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_NOHIDESEL |
			((option.fmt_txt_viewer_word_wrap == 0) ? WS_HSCROLL : 0),
			0, 0, 0, 0, parent_wnd, (HMENU)0, hInst, NULL);
		tab[0] = option.fmt_txt_tab_size * 4;
		SendMessage(hTxtWnd, EM_SETTABSTOPS, 1, (LPARAM)tab);
	}
	return hTxtWnd;
}

/*
 * text_window_destroy - データ表示ウィンドウの破棄
 */
__declspec(dllexport) BOOL CALLBACK text_window_destroy(const HWND hWnd)
{
	hTxtWnd = NULL;
	return TRUE;
}

/*
 * text_window_show_data - データの表示
 */
__declspec(dllexport) BOOL CALLBACK text_window_show_data(const HWND hWnd, DATA_INFO *di, const BOOL lock)
{
	BYTE *mem;
	TCHAR *buf;
	int size;

	if (di->data == NULL) {
		SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM)TEXT(""));
	} else {
		if ((mem = GlobalLock(di->data)) == NULL) {
			return FALSE;
		}
		if (di->format == CF_UNICODETEXT) {
#ifdef UNICODE
			SendMessage(hWnd, WM_SETTEXT, di->size / sizeof(WCHAR), (LPARAM)mem);
#else
			size = WideCharToMultiByte(CP_ACP, 0, (WCHAR *)mem, di->size / sizeof(WCHAR), NULL, 0, NULL, NULL);
			if ((buf = mem_alloc(sizeof(char) * (size + 1))) == NULL) {
				GlobalUnlock(di->data);
				return FALSE;
			}
			WideCharToMultiByte(CP_ACP, 0, (WCHAR *)mem, di->size / sizeof(WCHAR), buf, size, NULL, NULL);
			SendMessage(hWnd, WM_SETTEXT, size, (LPARAM)buf);
			mem_free(&buf);
#endif
		} else {
#ifdef UNICODE
			size = MultiByteToWideChar(CP_ACP, 0, mem, di->size, NULL, 0);
			if ((buf = mem_alloc(sizeof(WCHAR) * (size + 1))) == NULL) {
				GlobalUnlock(di->data);
				return FALSE;
			}
			MultiByteToWideChar(CP_ACP, 0, mem, di->size, buf, size);
			SendMessage(hWnd, WM_SETTEXT, size, (LPARAM)buf);
			mem_free(&buf);
#else
			SendMessage(hWnd, WM_SETTEXT, di->size, (LPARAM)mem);
#endif
		}
		GlobalUnlock(di->data);
	}
	SendMessage(hWnd, EM_SETREADONLY, lock, 0);
	return TRUE;
}

/*
 * text_window_save_data - データの保存
 */
__declspec(dllexport) BOOL CALLBACK text_window_save_data(const HWND hWnd, DATA_INFO *di)
{
	HANDLE data;
	BYTE *to_mem;
	DWORD size;
	TCHAR *buf;

	if (hWnd == NULL ||
		(SendMessage(hWnd, EM_GETREADONLY, 0, 0) == TRUE) ||
		SendMessage(hWnd, EM_GETMODIFY, 0, 0) == FALSE) {
		return FALSE;
	}
	if (di->data != NULL) {
		GlobalFree(di->data);
		di->data = NULL;
		di->size = 0;
	}
	size = SendMessage(hWnd, WM_GETTEXTLENGTH, 0, 0);
	// データの作成
	if (di->format == CF_UNICODETEXT) {
#ifdef UNICODE
		if ((data = GlobalAlloc(GHND, sizeof(WCHAR) * (size + 1))) == NULL) {
			return FALSE;
		}
		if ((to_mem = GlobalLock(data)) == NULL) {
			GlobalFree(data);
			return FALSE;
		}
		SendMessage(hWnd, WM_GETTEXT, size + 1, (LPARAM)to_mem);
#else
		// 現在表示されている内容の取得
		if ((buf = mem_alloc(sizeof(TCHAR) * (size + 1))) == NULL) {
			return FALSE;
		}
		SendMessage(hWnd, WM_GETTEXT, size + 1, (LPARAM)buf);

		size = MultiByteToWideChar(CP_ACP, 0, buf, -1, NULL, 0) - 1;
		if ((data = GlobalAlloc(GHND, sizeof(WCHAR) * (size + 1))) == NULL) {
			mem_free(&buf);
			return FALSE;
		}
		if ((to_mem = GlobalLock(data)) == NULL) {
			GlobalFree(data);
			mem_free(&buf);
			return FALSE;
		}
		MultiByteToWideChar(CP_ACP, 0, buf, -1, (WCHAR *)to_mem, size);
		mem_free(&buf);
#endif
		GlobalUnlock(data);
	} else {
#ifdef UNICODE
		// 現在表示されている内容の取得
		if ((buf = mem_alloc(sizeof(TCHAR) * (size + 1))) == NULL) {
			return FALSE;
		}
		SendMessage(hWnd, WM_GETTEXT, size + 1, (LPARAM)buf);

		size = WideCharToMultiByte(CP_ACP, 0, buf, -1, NULL, 0, NULL, NULL) - 1;
		if ((data = GlobalAlloc(GHND, sizeof(char) * (size + 1))) == NULL) {
			mem_free(&buf);
			return FALSE;
		}
		if ((to_mem = GlobalLock(data)) == NULL) {
			GlobalFree(data);
			mem_free(&buf);
			return FALSE;
		}
		WideCharToMultiByte(CP_ACP, 0, buf, -1, to_mem, size + 1, NULL, NULL);
		mem_free(&buf);
#else
		if ((data = GlobalAlloc(GHND, sizeof(char) * (size + 1))) == NULL) {
			return FALSE;
		}
		if ((to_mem = GlobalLock(data)) == NULL) {
			GlobalFree(data);
			return FALSE;
		}
		SendMessage(hWnd, WM_GETTEXT, size + 1, (LPARAM)to_mem);
#endif
		GlobalUnlock(data);
	}
	// 新しいデータを設定
	di->data = data;
	if (di->format == CF_UNICODETEXT) {
		di->size = sizeof(WCHAR) * (size + 1);
	} else {
		di->size = size + 1;
	}
	SendMessage(hWnd, EM_SETMODIFY, FALSE, 0);
	return TRUE;
}

/*
 * text_window_hide_data - データの非表示
 */
__declspec(dllexport) BOOL CALLBACK text_window_hide_data(const HWND hWnd, DATA_INFO *di)
{
	SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM)TEXT(""));
	return TRUE;
}
/* End of source */
