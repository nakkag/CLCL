/*
 * CLCL
 *
 * fmt_file.c
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
#include "Data.h"
#include "fmt_file_view.h"
#include "dpi.h"

#include "resource.h"

/* Define */
#define TOOLTIP_MAX						100

/* Global Variables */
static HICON file_icon;
static HWND hFileWnd;

extern HINSTANCE hInst;

/* Local Function Prototypes */

/*
 * file_initialize - 初期化
 */
__declspec(dllexport) BOOL CALLBACK file_initialize(void)
{
	if (file_icon == NULL) {
		file_icon = LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON_FILE), IMAGE_ICON, Scale(16), Scale(16), 0);
	}
	fileview_regist(hInst);
	return TRUE;
}

/*
 * file_get_icon - 形式用のアイコンを取得
 */
__declspec(dllexport) HICON CALLBACK file_get_icon(const int icon_size, BOOL *free_icon)
{
	*free_icon = FALSE;
	return file_icon;
}

/*
 * file_free - 終了処理
 */
__declspec(dllexport) BOOL CALLBACK file_free(void)
{
	if (file_icon != NULL) {
		DestroyIcon(file_icon);
		file_icon = NULL;
	}
	return TRUE;
}

/*
 * file_initialize_item - アイテム情報の初期化
 */
__declspec(dllexport) BOOL CALLBACK file_initialize_item(DATA_INFO *di, const BOOL set_init_data)
{
	return FALSE;
}

/*
 * file_copy_data - データのコピー
 */
__declspec(dllexport) HANDLE CALLBACK file_copy_data(const TCHAR *format_name, const HANDLE data, DWORD *ret_size)
{
	return NULL;
}

/*
 * file_data_to_bytes - データをバイト列に変換
 */
__declspec(dllexport) BYTE* CALLBACK file_data_to_bytes(const DATA_INFO *di, DWORD *ret_size)
{
	return NULL;
}

/*
 * file_bytes_to_data - バイト列をデータに変換
 */
__declspec(dllexport) HANDLE CALLBACK file_bytes_to_data(const TCHAR *format_name, const BYTE *data, DWORD *size)
{
	return NULL;
}

/*
 * file_get_file_info - コモンダイアログ情報の取得
 */
__declspec(dllexport) int CALLBACK file_get_file_info(const TCHAR *format_name, const DATA_INFO *di, OPENFILENAME *of, const BOOL mode)
{
	return 0;
}

/*
 * file_data_to_file - データをファイルに保存
 */
__declspec(dllexport) BOOL CALLBACK file_data_to_file(DATA_INFO *di, const TCHAR *file_name, const int filter_index, TCHAR *err_str)
{
	return FALSE;
}

/*
 * file_file_to_data - ファイルからデータを作成
 */
__declspec(dllexport) HANDLE CALLBACK file_file_to_data(const TCHAR *file_name, const TCHAR *format_name, DWORD *ret_size, TCHAR *err_str)
{
	HDROP ret = NULL;
	TCHAR **buf;

	if (lstrcmpi(format_name, TEXT("DROP FILE LIST")) != 0) {
		return NULL;
	}

	if ((buf = (TCHAR **)mem_calloc(sizeof(TCHAR *))) == NULL) {
		return NULL;
	}
	if ((buf[0] = (TCHAR *)mem_alloc(sizeof(TCHAR) * MAX_PATH)) != NULL) {
		lstrcpy(buf[0], file_name);
		ret = create_dropfile(buf, 1, ret_size);
		mem_free((void **)&(buf[0]));
	}
	mem_free((void **)&buf);
	return ret;
}

/*
 * file_free_data - データの解放
 */
__declspec(dllexport) BOOL CALLBACK file_free_data(const TCHAR *format_name, HANDLE data)
{
	return FALSE;
}

/*
 * file_free_item - アイテム情報の解放
 */
__declspec(dllexport) BOOL CALLBACK file_free_item(DATA_INFO *di)
{
	return FALSE;
}

/*
 * file_get_menu_title - メニュータイトルの取得
 */
__declspec(dllexport) BOOL CALLBACK file_get_menu_title(DATA_INFO *di)
{
	TCHAR buf[MAX_PATH];
	TCHAR menu_title[BUF_SIZE * 2];
	TCHAR *p, *r;
	int cnt;

	if (di->data == NULL || lstrcmpi(di->format_name, TEXT("DROP FILE LIST")) != 0) {
		return FALSE;
	}
	if ((cnt = DragQueryFile(di->data, 0xFFFFFFFF, NULL, 0)) == 0) {
		return FALSE;
	}

	// メニュー用文字列
	DragQueryFile(di->data, 0, buf, MAX_PATH - 1);
	for (r = p = buf; *p != TEXT('\0'); p++) {
#ifndef UNICODE
		if (IsDBCSLeadByte((BYTE)*p) == TRUE) {
			p++;
			continue;
		}
#endif	// UNICODE
		if (*p == TEXT('\\') || *p == TEXT('/')) {
			r = p;
		}
	}
	if (r != buf) {
		r++;
	}

	if (cnt == 1) {
		di->menu_title = alloc_copy(r);
	} else {
		wsprintf(menu_title, TEXT("%d files (%s...)"), cnt, r);
		di->menu_title = alloc_copy(menu_title);
	}
	di->free_title = TRUE;
	return TRUE;
}

/*
 * file_get_menu_icon - メニュー用アイコンの取得
 */
__declspec(dllexport) BOOL CALLBACK file_get_menu_icon(DATA_INFO *di, const int icon_size)
{
	di->menu_icon = file_icon;
	di->free_icon = FALSE;
	return TRUE;
}

/*
 * file_get_menu_bitmap - メニュー用ビットマップの取得
 */
__declspec(dllexport) BOOL CALLBACK file_get_menu_bitmap(DATA_INFO *di, const int width, const int height)
{
	return FALSE;
}

/*
 * file_get_tooltip_text - メニュー用ツールチップテキスト
 */
__declspec(dllexport) TCHAR* CALLBACK file_get_tooltip_text(DATA_INFO *di)
{
	TCHAR *ret;
	TCHAR *p;
	int cnt;
	int i;

	if (di->data == NULL || lstrcmpi(di->format_name, TEXT("DROP FILE LIST")) != 0) {
		return NULL;
	}
	if ((cnt = DragQueryFile(di->data, 0xFFFFFFFF, NULL, 0)) == 0) {
		return NULL;
	}
	if (cnt > TOOLTIP_MAX) {
		cnt = TOOLTIP_MAX;
	}

	p = ret = mem_alloc((sizeof(TCHAR) * (MAX_PATH + 2)) * cnt);
	if (ret == NULL) {
		return NULL;
	}
	for (i = 0; i < cnt; i++) {
		DragQueryFile(di->data, i, p, MAX_PATH - 1);
		p += lstrlen(p);
		*(p++) = TEXT('\r');
		*(p++) = TEXT('\n');
	}
	*p = TEXT('\0');
	return ret;
}

/*
 * file_window_create - データ表示ウィンドウの作成
 */
__declspec(dllexport) HWND CALLBACK file_window_create(const HWND parent_wnd)
{
	if (hFileWnd == NULL) {
		hFileWnd = fileview_create(hInst, parent_wnd, 0);
	}
	return hFileWnd;
}

/*
 * file_window_destroy - データ表示ウィンドウの破棄
 */
__declspec(dllexport) BOOL CALLBACK file_window_destroy(const HWND hWnd)
{
	hFileWnd = NULL;
	return TRUE;
}

/*
 * file_window_show_data - データの表示
 */
__declspec(dllexport) BOOL CALLBACK file_window_show_data(const HWND hWnd, DATA_INFO *di, const BOOL lock)
{
	if (di->data == NULL || lstrcmpi(di->format_name, TEXT("DROP FILE LIST")) != 0) {
		return TRUE;
	}
	SendMessage(hWnd, WM_SET_FILEDATA, (WPARAM)lock, (LPARAM)di->data);
	return TRUE;
}

/*
 * file_window_save_data - データの保存
 */
__declspec(dllexport) BOOL CALLBACK file_window_save_data(const HWND hWnd, DATA_INFO *di)
{
	HDROP data;

	if ((data = (HDROP)SendMessage(hWnd, WM_GET_FILEDATA, 0, 0)) == NULL) {
		return FALSE;
	}
	if (di->data != NULL) {
		GlobalFree(di->data);
	}
	di->data = data;
	di->size = GlobalSize(data);
	return TRUE;
}

/*
 * file_window_hide_data - データの非表示
 */
__declspec(dllexport) BOOL CALLBACK file_window_hide_data(const HWND hWnd, DATA_INFO *di)
{
	SendMessage(hWnd, WM_SET_FILEDATA, 0, 0);
	return TRUE;
}
/* End of source */
