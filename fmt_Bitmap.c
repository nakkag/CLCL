/*
 * CLCL
 *
 * fmt_Bitmap.c
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

#include "General.h"
#include "Memory.h"
#include "Message.h"
#include "Data.h"
#include "Ini.h"
#include "File.h"
#include "Bitmap.h"
#include "fmt_bitmap_view.h"
#include "gdip.h"
#include "dpi.h"

#include "resource.h"

/* Define */
#define BITMAP_NEW_WIDTH			1
#define BITMAP_NEW_HEIGHT			1

/* Global Variables */
static HICON bmp_icon;
// 読み込み済みのアイコンのサイズ
static int bmp_icon_size;
static HWND hBmpWnd;

extern HINSTANCE hInst;
extern OPTION_INFO option;

/* Local Function Prototypes */
static HICON bitmap_load_icon(const int icon_size);
static BYTE *bmp_file_to_dib(const BYTE *data, const DWORD size, DWORD *ret_size);

/*
 * bitmap_initialize - 初期化
 */
__declspec(dllexport) BOOL CALLBACK bitmap_initialize(void)
{
	bitmap_load_icon(GetSystemMetricsDpi(SM_CXSMICON));
	bmpview_regist(hInst);
	init_gdip();
	return TRUE;
}

/*
 * bitmap_load_icon - 形式用のアイコンの読み込み
 */
static HICON bitmap_load_icon(const int icon_size)
{
	if (icon_size <= 0) {
		return bmp_icon;
	}
	if (bmp_icon != NULL && bmp_icon_size == icon_size) {
		return bmp_icon;
	}
	if (bmp_icon != NULL) {
		DestroyIcon(bmp_icon);
	}
	bmp_icon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON_BITMAP), IMAGE_ICON, icon_size, icon_size, 0);
	bmp_icon_size = icon_size;
	return bmp_icon;
}

/*
 * bitmap_get_icon - 形式用のアイコンを取得
 */
__declspec(dllexport) HICON CALLBACK bitmap_get_icon(const int icon_size, BOOL *free_icon)
{
	*free_icon = FALSE;
	return bitmap_load_icon(icon_size);
}

/*
 * bitmap_free - 終了処理
 */
__declspec(dllexport) BOOL CALLBACK bitmap_free(void)
{
	if (bmp_icon != NULL) {
		DestroyIcon(bmp_icon);
		bmp_icon = NULL;
	}
	bmp_icon_size = 0;
	shutdown_gdip();
	return TRUE;
}

/*
 * bitmap_initialize_item - アイテム情報の初期化
 */
__declspec(dllexport) BOOL CALLBACK bitmap_initialize_item(DATA_INFO *di, const BOOL set_init_data)
{
	return FALSE;
}

/*
 * bitmap_copy_data - データのコピー
 */
__declspec(dllexport) HANDLE CALLBACK bitmap_copy_data(const TCHAR *format_name, const HANDLE data, DWORD *ret_size)
{
	return NULL;
}

/*
 * bitmap_data_to_bytes - データをバイト列に変換
 */
__declspec(dllexport) BYTE* CALLBACK bitmap_data_to_bytes(const DATA_INFO *di, DWORD *ret_size)
{
	return NULL;
}

/*
 * bitmap_bytes_to_data - バイト列をデータに変換
 */
__declspec(dllexport) HANDLE CALLBACK bitmap_bytes_to_data(const TCHAR *format_name, const BYTE *data, DWORD *size)
{
	return NULL;
}

/*
 * bitmap_get_file_info - コモンダイアログ情報の取得
 */
__declspec(dllexport) int CALLBACK bitmap_get_file_info(const TCHAR *format_name, const DATA_INFO *di, OPENFILENAME *of, const BOOL mode)
{
	of->lpstrFilter = TEXT("*.bmp\0*.bmp\0*.png\0*.png\0*.jpg\0*.jpg\0*.*\0*.*\0\0");
	of->nFilterIndex = 1;
	of->lpstrDefExt = TEXT("bmp");
	return 1;
}

/*
 * bitmap_data_to_file - データをファイルに保存
 */
__declspec(dllexport) BOOL CALLBACK bitmap_data_to_file(DATA_INFO *di, const TCHAR *file_name, const int filter_index, TCHAR *err_str)
{
	BITMAPFILEHEADER hdr;
	BYTE *mem;
	BYTE *save_mem;
	TCHAR* ext;
	DWORD size;
	DWORD offset;

	if (di->data == NULL) {
		return FALSE;
	}

	ext = PathFindExtension(file_name);
	if (lstrcmpi(ext, TEXT(".png")) == 0 || lstrcmpi(ext, TEXT(".jpg")) == 0 || lstrcmpi(ext, TEXT(".jpeg")) == 0) {
		HBITMAP hbmp = NULL;
		BOOL result;
		if (lstrcmpi(di->format_name, TEXT("BITMAP")) != 0) {
			if ((mem = GlobalLock(di->data)) == NULL) {
				message_get_error(GetLastError(), err_str);
				return FALSE;
			}
			hbmp = dib_to_bitmap(mem, (DWORD)GlobalSize(di->data));
			GlobalUnlock(di->data);
			if (hbmp == NULL) {
				message_get_error(GetLastError(), err_str);
				return FALSE;
			}
		}
		else {
			hbmp = (HBITMAP)di->data;
		}
		if (lstrcmpi(ext, TEXT(".png")) == 0) {
			// PNGで保存
			result = (save_png(hbmp, file_name) != 0);
		}
		else {
			// JPEGで保存
			result = (save_jpeg(hbmp, file_name, 90) != 0);
		}
		if (hbmp != (HBITMAP)di->data) {
			DeleteObject(hbmp);
		}
		return result;
	} else {
		// BMPで保存
		if (lstrcmpi(di->format_name, TEXT("BITMAP")) != 0) {
			if ((mem = GlobalLock(di->data)) == NULL) {
				message_get_error(GetLastError(), err_str);
				return FALSE;
			}
			size = di->size;
			if (size > (DWORD)GlobalSize(di->data)) {
				size = (DWORD)GlobalSize(di->data);
			}
		}
		else {
			if ((mem = bitmap_to_dib(di->data, &size)) == NULL) {
				message_get_error(GetLastError(), err_str);
				return FALSE;
			}
		}
		// DIBヘッダの検証と画素データまでのオフセット
		if (size < sizeof(BITMAPINFOHEADER) || (offset = dib_get_bits_offset(mem, size)) == 0) {
			message_get_error(ERROR_INVALID_DATA, err_str);
			if (lstrcmpi(di->format_name, TEXT("BITMAP")) != 0) {
				GlobalUnlock(di->data);
			}
			else {
				mem_free(&mem);
			}
			return FALSE;
		}
		// BITMAPファイルの作成
		hdr.bfType = 0x4d42;
		hdr.bfSize = sizeof(BITMAPFILEHEADER) + size;
		hdr.bfReserved1 = 0;
		hdr.bfReserved2 = 0;
		hdr.bfOffBits = sizeof(BITMAPFILEHEADER) + offset;

		if ((save_mem = mem_alloc(sizeof(BITMAPFILEHEADER) + size)) == NULL) {
			message_get_error(GetLastError(), err_str);
			if (lstrcmpi(di->format_name, TEXT("BITMAP")) != 0) {
				GlobalUnlock(di->data);
			}
			else {
				mem_free(&mem);
			}
			return FALSE;
		}
		CopyMemory(save_mem, &hdr, sizeof(BITMAPFILEHEADER));
		CopyMemory(save_mem + sizeof(BITMAPFILEHEADER), mem, size);

		// ファイルに書き込む
		if (file_write_buf(file_name, save_mem, sizeof(BITMAPFILEHEADER) + size, err_str) == FALSE) {
			mem_free(&save_mem);
			if (lstrcmpi(di->format_name, TEXT("BITMAP")) != 0) {
				GlobalUnlock(di->data);
			}
			else {
				mem_free(&mem);
			}
			return FALSE;
		}
		mem_free(&save_mem);
		if (lstrcmpi(di->format_name, TEXT("BITMAP")) != 0) {
			GlobalUnlock(di->data);
		}
		else {
			mem_free(&mem);
		}
	}
	return TRUE;
}

/*
 * bmp_file_to_dib - BMPファイルの内容からDIB(ヘッダ + カラーテーブル + 画素データ)を作成
 */
static BYTE *bmp_file_to_dib(const BYTE *data, const DWORD size, DWORD *ret_size)
{
	const BITMAPFILEHEADER *bfh = (const BITMAPFILEHEADER *)data;
	const BYTE *dib;
	DWORD dib_size;
	DWORD hdr_size;
	DWORD bits_size;
	BYTE *ret;

	if (data == NULL || size < sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)) {
		return NULL;
	}
	if (bfh->bfType != 0x4d42 || bfh->bfOffBits < sizeof(BITMAPFILEHEADER) || bfh->bfOffBits > size) {
		return NULL;
	}
	dib = data + sizeof(BITMAPFILEHEADER);
	dib_size = size - sizeof(BITMAPFILEHEADER);
	if ((hdr_size = dib_get_bits_offset(dib, dib_size)) == 0) {
		return NULL;
	}
	if (bfh->bfOffBits == sizeof(BITMAPFILEHEADER) + hdr_size) {
		// ヘッダの直後に画素データがある
		if ((ret = mem_alloc(dib_size)) == NULL) {
			return NULL;
		}
		CopyMemory(ret, dib, dib_size);
		*ret_size = dib_size;
		return ret;
	}
	// ヘッダ部と bfOffBits 以降の画素部を連結する
	bits_size = size - bfh->bfOffBits;
	if ((ret = mem_alloc(hdr_size + bits_size)) == NULL) {
		return NULL;
	}
	CopyMemory(ret, dib, hdr_size);
	CopyMemory(ret + hdr_size, data + bfh->bfOffBits, bits_size);
	if (dib_get_bits_offset(ret, hdr_size + bits_size) == 0) {
		mem_free(&ret);
		return NULL;
	}
	*ret_size = hdr_size + bits_size;
	return ret;
}

/*
 * bitmap_file_to_data - ファイルからデータを作成
 */
__declspec(dllexport) HANDLE CALLBACK bitmap_file_to_data(const TCHAR *file_name, const TCHAR *format_name, DWORD *ret_size, TCHAR *err_str)
{
	HANDLE ret;
	BYTE *data;
	BYTE *mem;
	TCHAR* ext;
	DWORD size;

	ext = PathFindExtension(file_name);
	if (lstrcmpi(ext, TEXT(".png")) == 0 || lstrcmpi(ext, TEXT(".jpg")) == 0 || lstrcmpi(ext, TEXT(".jpeg")) == 0) {
		HBITMAP hbmp = image_to_bitmap(NULL, file_name);
		if (hbmp == NULL) {
			message_get_error(ERROR_INVALID_DATA, err_str);
			return NULL;
		}
		if (lstrcmpi(format_name, TEXT("BITMAP")) != 0) {
			if ((data = bitmap_to_dib(hbmp, &size)) == NULL) {
				message_get_error(GetLastError(), err_str);
				DeleteObject(hbmp);
				return NULL;
			}
			// コピー先確保
			if ((ret = GlobalAlloc(GHND, size)) == NULL) {
				message_get_error(GetLastError(), err_str);
				mem_free(&data);
				DeleteObject(hbmp);
				return NULL;
			}
			// コピー先ロック
			if ((mem = GlobalLock(ret)) == NULL) {
				message_get_error(GetLastError(), err_str);
				GlobalFree(ret);
				mem_free(&data);
				DeleteObject(hbmp);
				return NULL;
			}
			// コピー
			CopyMemory(mem, data, size);
			// ロック解除
			GlobalUnlock(ret);

			if (ret_size != NULL) {
				*ret_size = size;
			}
			mem_free(&data);
			DeleteObject(hbmp);
		}
		else {
			ret = hbmp;
			if (ret_size != NULL) {
				mem = bitmap_to_dib(hbmp, ret_size);
				if (mem != NULL) {
					mem_free(&mem);
				}
			}
		}
	}
	else {
		BYTE *dib;
		DWORD dib_size;

		// ファイルの読み込み
		if ((data = file_read_buf(file_name, &size, err_str)) == NULL) {
			return NULL;
		}
		// BMPファイルヘッダの検証とDIBの取り出し
		if ((dib = bmp_file_to_dib(data, size, &dib_size)) == NULL) {
			message_get_error(ERROR_INVALID_DATA, err_str);
			mem_free(&data);
			return NULL;
		}
		mem_free(&data);
		if (lstrcmpi(format_name, TEXT("BITMAP")) != 0) {
			// コピー先確保
			if ((ret = GlobalAlloc(GHND, dib_size)) == NULL) {
				message_get_error(GetLastError(), err_str);
				mem_free(&dib);
				return NULL;
			}
			// コピー先ロック
			if ((mem = GlobalLock(ret)) == NULL) {
				message_get_error(GetLastError(), err_str);
				GlobalFree(ret);
				mem_free(&dib);
				return NULL;
			}
			// コピー
			CopyMemory(mem, dib, dib_size);
			// ロック解除
			GlobalUnlock(ret);
		}
		else {
			if ((ret = dib_to_bitmap(dib, dib_size)) == NULL) {
				message_get_error(GetLastError(), err_str);
				mem_free(&dib);
				return NULL;
			}
		}
		if (ret_size != NULL) {
			*ret_size = dib_size;
		}
		mem_free(&dib);
	}
	return ret;
}

/*
 * bitmap_free_data - データの解放
 */
__declspec(dllexport) BOOL CALLBACK bitmap_free_data(const TCHAR *format_name, HANDLE data)
{
	return FALSE;
}

/*
 * bitmap_free_item - アイテム情報の解放
 */
__declspec(dllexport) BOOL CALLBACK bitmap_free_item(DATA_INFO *di)
{
	return FALSE;
}

/*
 * bitmap_get_menu_title - メニュータイトルの取得
 */
__declspec(dllexport) BOOL CALLBACK bitmap_get_menu_title(DATA_INFO *di)
{
	return FALSE;
}

/*
 * bitmap_get_menu_icon - メニュータイトル
 */
__declspec(dllexport) BOOL CALLBACK bitmap_get_menu_icon(DATA_INFO *di, const int icon_size)
{
	di->menu_icon = bitmap_load_icon(icon_size);
	di->free_icon = FALSE;
	return TRUE;
}

/*
 * bitmap_get_menu_bitmap - メニュー用ビットマップ
 */
__declspec(dllexport) BOOL CALLBACK bitmap_get_menu_bitmap(DATA_INFO *di, const int width, const int height)
{
	HDC hdc;
	HDC from_dc;
	HDC to_dc;
	HBITMAP hbmp;
	HBITMAP old_from_hbmp;
	HBITMAP old_to_hbmp;
	BITMAP bmp;
	BYTE *mem;

	if (di->data == NULL) {
		return FALSE;
	}

	if (lstrcmpi(di->format_name, TEXT("BITMAP")) != 0) {
		if ((mem = GlobalLock(di->data)) == NULL) {
			return FALSE;
		}
		hbmp = dib_to_bitmap(mem, (DWORD)GlobalSize(di->data));
		GlobalUnlock(di->data);
		if (hbmp == NULL) {
			return FALSE;
		}
	} else {
		hbmp = di->data;
	}

	// メニューに表示するビットマップの作成
	if (GetObject(hbmp, sizeof(BITMAP), &bmp) == 0) {
		if (lstrcmpi(di->format_name, TEXT("BITMAP")) != 0) {
			DeleteObject((HGDIOBJ)hbmp);
		}
		return FALSE;
	}

	hdc = GetDC(NULL);

	from_dc = CreateCompatibleDC(hdc);
	old_from_hbmp = SelectObject(from_dc, hbmp);

	to_dc = CreateCompatibleDC(hdc);
	di->menu_bitmap = CreateCompatibleBitmap(hdc, width, height);
	di->free_bitmap = TRUE;
	old_to_hbmp = SelectObject(to_dc, di->menu_bitmap);

	if (width < bmp.bmWidth || height < bmp.bmHeight) {
		SetStretchBltMode(to_dc, HALFTONE);
		SetBrushOrgEx(to_dc, 0, 0, NULL);
	} else {
		SetStretchBltMode(to_dc, COLORONCOLOR);
	}
	StretchBlt(to_dc, 0, 0, width, height,
		from_dc, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);

	SelectObject(from_dc, old_from_hbmp);
	DeleteDC(from_dc);

	SelectObject(to_dc, old_to_hbmp);
	DeleteDC(to_dc);

	ReleaseDC(NULL, hdc);

	if (lstrcmpi(di->format_name, TEXT("BITMAP")) != 0) {
		DeleteObject((HGDIOBJ)hbmp);
	}
	return TRUE;
}

/*
 * bitmap_get_tooltip_text - メニュー用ツールチップテキスト
 */
__declspec(dllexport) TCHAR* CALLBACK bitmap_get_tooltip_text(DATA_INFO *di)
{
	BITMAP bmp;
	HBITMAP hbmp;
	BYTE *mem;
	TCHAR buf[BUF_SIZE];
	TCHAR *ret;

	if (di->data == NULL) {
		return NULL;
	}
	if (lstrcmpi(di->format_name, TEXT("BITMAP")) != 0) {
		if ((mem = GlobalLock(di->data)) == NULL) {
			return NULL;
		}
		hbmp = dib_to_bitmap(mem, (DWORD)GlobalSize(di->data));
		GlobalUnlock(di->data);
		if (hbmp == NULL) {
			return NULL;
		}
	} else {
		hbmp = di->data;
	}

	// 画像情報取得
	if (GetObject(hbmp, sizeof(BITMAP), &bmp) == 0) {
		if (lstrcmpi(di->format_name, TEXT("BITMAP")) != 0) {
			DeleteObject((HGDIOBJ)hbmp);
		}
		return NULL;
	}
	if (di->size < 1024) {
		wnsprintf(buf, _countof(buf), TEXT("%u x %u (%d bytes)"), bmp.bmWidth, bmp.bmHeight, di->size);
	} else {
		wnsprintf(buf, _countof(buf), TEXT("%u x %u (%d KB)"), bmp.bmWidth, bmp.bmHeight, di->size / 1024);
	}
	ret = alloc_copy(buf);

	if (lstrcmpi(di->format_name, TEXT("BITMAP")) != 0) {
		DeleteObject((HGDIOBJ)hbmp);
	}
	return ret;
}

/*
 * bitmap_window_create - データ表示ウィンドウの作成
 */
__declspec(dllexport) HWND CALLBACK bitmap_window_create(const HWND parent_wnd)
{
	if (hBmpWnd == NULL) {
		hBmpWnd = bmpview_create(hInst, parent_wnd, 0);
	}
	return hBmpWnd;
}

/*
 * bitmap_window_destroy - データ表示ウィンドウの破棄
 */
__declspec(dllexport) BOOL CALLBACK bitmap_window_destroy(const HWND hWnd)
{
	hBmpWnd = NULL;
	return TRUE;
}

/*
 * bitmap_window_show_data - データの表示
 */
__declspec(dllexport) BOOL CALLBACK bitmap_window_show_data(const HWND hWnd, DATA_INFO *di, const BOOL lock)
{
	SendMessage(hWnd, WM_SET_BMPDATA, 0, (LPARAM)di);
	return TRUE;
}

/*
 * bitmap_window_save_data - データの保存
 */
__declspec(dllexport) BOOL CALLBACK bitmap_window_save_data(const HWND hWnd, DATA_INFO *di)
{
	return FALSE;
}

/*
 * bitmap_window_hide_data - データの非表示
 */
__declspec(dllexport) BOOL CALLBACK bitmap_window_hide_data(const HWND hWnd, DATA_INFO *di)
{
	SendMessage(hWnd, WM_SET_BMPDATA, 0, 0);
	return TRUE;
}
/* End of source */
