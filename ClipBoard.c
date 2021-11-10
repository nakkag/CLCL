/*
 * CLCL
 *
 * ClipBoard.c
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
#include "File.h"
#include "ClipBoard.h"
#include "Format.h"
#include "Filter.h"
#include "Bitmap.h"

/* Define */

/* Global Variables */
typedef struct _FORMAT_NAME_INFO {
	UINT format;
	TCHAR *name;
} FORMAT_NAME_INFO;

/* Local Function Prototypes */
static BOOL clipboard_set_data(const UINT format, const TCHAR *name, const HANDLE data, TCHAR *err_str);

/*
 * clipboard_get_format - クリップボード形式名の取得
 */
UINT clipboard_get_format(const UINT format, TCHAR *type_name)
{
	int i;

	const FORMAT_NAME_INFO fi[] = {
		CF_TEXT,			TEXT("TEXT"),
		CF_BITMAP,			TEXT("BITMAP"),
		CF_METAFILEPICT,	TEXT("METAFILE PICTURE"),
		CF_SYLK,			TEXT("SYLK"),
		CF_DIF,				TEXT("DIF"),
		CF_TIFF,			TEXT("TIFF"),
		CF_OEMTEXT,			TEXT("OEM TEXT"),
		CF_DIB,				TEXT("DIB"),
		CF_PALETTE,			TEXT("PALETTE"),
		CF_PENDATA,			TEXT("PEN DATA"),
		CF_RIFF,			TEXT("RIFF"),
		CF_WAVE,			TEXT("WAVE DATA"),
		CF_UNICODETEXT,		TEXT("UNICODE TEXT"),
		CF_ENHMETAFILE,		TEXT("ENHANCED METAFILE"),
		CF_HDROP,			TEXT("DROP FILE LIST"),
		CF_LOCALE,			TEXT("LOCALE"),
		CF_MAX,				TEXT("MAX"),
		CF_OWNERDISPLAY,	TEXT("OWNER DISPLAY"),
		CF_DSPTEXT,			TEXT("PRIVATE TEXT"),
		CF_DSPBITMAP,		TEXT("PRIVATE BITMAP"),
		CF_DSPMETAFILEPICT,	TEXT("PRIVATE METAFILE PICTURE"),
		CF_DSPENHMETAFILE,	TEXT("PRIVATE ENHANCED METAFILE"),
		CF_PRIVATEFIRST,	TEXT("PRIVATE FIRST"),
		CF_PRIVATELAST,		TEXT("PRIVATE LAST"),
		CF_GDIOBJFIRST,		TEXT("GDI OBJECT FIRST"),
		CF_GDIOBJLAST,		TEXT("GDI OBJECT LAST"),
		0,					TEXT(""),
	};

	if (format != 0) {
		// format から名前を取得
		*type_name = TEXT('\0');
		if (GetClipboardFormatName(format, type_name, BUF_SIZE - 1) != 0) {
			return 0;
		}
		for (i = 0; (fi + i)->format != 0; i++) {
			if (format == (fi + i)->format) {
				lstrcpy(type_name, (fi + i)->name);
				break;
			}
		}
		if ((fi + i)->format == 0) {
			lstrcpy(type_name, (fi + i)->name);
		}

	} else {
		// 名前から format を取得
		for (i = 0; (fi + i)->format != 0; i++) {
			if (lstrcmpi(type_name, (fi + i)->name) == 0) {
				return ((int)(fi + i)->format);
			}
		}
	}
	return 0;
}

/*
 * clipboard_get_datainfo - クリップボードの内容からデータリストを作成
 */
DATA_INFO *clipboard_get_datainfo(const BOOL use_filter, const BOOL get_data, TCHAR *err_str)
{
	DATA_INFO *ret_di = NULL;
	DATA_INFO *new_item;
	DATA_INFO *di;
	TCHAR buf[BUF_SIZE];
	HANDLE data;
	UINT format;

	format = 0;
	while ((format = EnumClipboardFormats(format)) != 0) {
		clipboard_get_format(format, buf);

		// フィルタ (形式)
		if (use_filter == TRUE && filter_format_check(buf) == FALSE) {
			continue;
		}

		// アイテムの作成
		if ((new_item = data_create_data(format, buf, NULL, 0, FALSE, err_str)) == NULL) {
			return NULL;
		}
		if (get_data == TRUE) {
			// クリップボードデータのコピー
			if ((data = GetClipboardData(format)) != NULL &&
				(new_item->data = format_copy_data(new_item->format_name, data, &new_item->size)) == NULL) {
				new_item->data = clipboard_copy_data(format, data, &new_item->size);
			}
		}
		
		// フィルタ (サイズ)
		if (use_filter == TRUE && filter_size_check(new_item->format_name, new_item->size) == FALSE) {
			data_free(new_item);
			continue;
		}

		// リストに追加
		if (ret_di == NULL) {
			ret_di = new_item;
		} else {
			di->next = new_item;
		}
		di = new_item;
	}
	return ret_di;
}

/*
 * clipboard_to_item - クリップボードのデータからアイテムを作成
 */
DATA_INFO *clipboard_to_item(TCHAR *err_str)
{
	DATA_INFO *new_item;

	// アイテムの作成
	if ((new_item = data_create_item(NULL, TRUE, err_str)) == NULL) {
		return NULL;
	}
	// クリップボードデータ取得
	if ((new_item->child = clipboard_get_datainfo(TRUE, TRUE, err_str)) == NULL) {
		data_free(new_item);
		return NULL;
	}
	return new_item;
}

/*
 * clipboard_set_data - クリップボードにデータを設定
 */
static BOOL clipboard_set_data(const UINT format, const TCHAR *name, const HANDLE data, TCHAR *err_str)
{
	UINT fmt = format;

	// クリップボード形式を取得
	if (fmt == 0 && (fmt = RegisterClipboardFormat(name)) == 0) {
		message_get_error(GetLastError(), err_str);
		return FALSE;
	}
	// クリップボードにデータを設定
	if (SetClipboardData(fmt, data) == NULL && data != NULL) {
		message_get_error(GetLastError(), err_str);
		return FALSE;
	}
	return TRUE;
}

/*
 * clipboard_set_datainfo - クリップボードにデータを設定
 */
BOOL clipboard_set_datainfo(const HWND hWnd, DATA_INFO *set_di, TCHAR *err_str)
{
	DATA_INFO *di;

	if (set_di == NULL) {
		return FALSE;
	}
	// クリップボードの初期化
	if (OpenClipboard(hWnd) == FALSE) {
		message_get_error(GetLastError(), err_str);
		return FALSE;
	}
	if (EmptyClipboard() == FALSE) {
		message_get_error(GetLastError(), err_str);
		CloseClipboard();
		return FALSE;
	}
	switch (set_di->type) {
	case TYPE_ITEM:
		// 子アイテムを全て追加
		for (di = set_di->child; di != NULL; di = di->next) {
			if (clipboard_set_data(di->format, di->format_name, di->data, err_str) == FALSE) {
				CloseClipboard();
				return FALSE;
			}
			di->data = NULL;
		}
		break;

	case TYPE_DATA:
		// 1件のみ追加
		if (clipboard_set_data(set_di->format, set_di->format_name, set_di->data, err_str) == FALSE) {
			CloseClipboard();
			return FALSE;
		}
		set_di->data = NULL;
		break;
	}
	CloseClipboard();
	return TRUE;
}

/*
 * clipboard_copy_data - クリップボードデータのコピーを作成
 */
HANDLE clipboard_copy_data(const UINT format, const HANDLE data, DWORD *ret_size)
{
	HANDLE ret = NULL;
	BYTE *from_mem, *to_mem;
	LOGPALETTE *lpal;
	WORD pcnt;

	if (data == NULL) {
		return NULL;
	}

	switch (format) {
	case CF_PALETTE:
		// パレット
		pcnt = 0;
		if (GetObject(data, sizeof(WORD), &pcnt) == 0) {
			return NULL;
		}
		if ((lpal = mem_calloc(sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * pcnt))) == NULL) {
			return NULL;
		}
		lpal->palVersion = 0x300;
		lpal->palNumEntries = pcnt;
		if (GetPaletteEntries(data, 0, pcnt, lpal->palPalEntry) == 0) {
			mem_free(&lpal);
			return NULL;
		}

		ret = CreatePalette(lpal);
		*ret_size = sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * pcnt);

		mem_free(&lpal);
		break;

	case CF_DSPBITMAP:
	case CF_BITMAP:
		// ビットマップ
		if ((to_mem = bitmap_to_dib(data, ret_size)) == NULL) {
			return NULL;
		}
		ret = dib_to_bitmap(to_mem);
		mem_free(&to_mem);
		break;

	case CF_OWNERDISPLAY:
		*ret_size = 0;
		break;

	case CF_DSPMETAFILEPICT:
	case CF_METAFILEPICT:
		// コピー元ロック
		if ((from_mem = GlobalLock(data)) == NULL) {
			return NULL;
		}
		// メタファイル
		if ((ret = GlobalAlloc(GHND, sizeof(METAFILEPICT))) == NULL) {
			GlobalUnlock(data);
			return NULL;
		}
		// コピー先ロック
		if ((to_mem = GlobalLock(ret)) == NULL) {
			GlobalFree(ret);
			GlobalUnlock(data);
			return NULL;
		}
		CopyMemory(to_mem, from_mem, sizeof(METAFILEPICT));
		if ((((METAFILEPICT *)to_mem)->hMF = CopyMetaFile(((METAFILEPICT *)from_mem)->hMF, NULL)) != NULL) {
			*ret_size = sizeof(METAFILEPICT) + GetMetaFileBitsEx(((METAFILEPICT *)to_mem)->hMF, 0, NULL);
		}
		// ロック解除
		GlobalUnlock(ret);
		GlobalUnlock(data);
		break;

	case CF_DSPENHMETAFILE:
	case CF_ENHMETAFILE:
		// 拡張メタファイル
		if ((ret = CopyEnhMetaFile(data, NULL)) != NULL) {
			*ret_size = GetEnhMetaFileBits(ret, 0, NULL);
		}
		break;

	default:
		// その他
		// メモリチェック
		if (IsBadReadPtr(data, 1) == TRUE) {
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

		// ロック解除
		GlobalUnlock(ret);
		GlobalUnlock(data);
		break;
	}
	return ret;
}

/*
 * clipboard_data_to_bytes - データをバイト列に変換
 */
BYTE *clipboard_data_to_bytes(const DATA_INFO *di, DWORD *ret_size)
{
	BYTE *ret = NULL;
	BYTE *tmp;
	DWORD i;
	DWORD size = 0;

	if (di->data == NULL) {
		return NULL;
	}

	switch (di->format) {
	case CF_PALETTE:
		// パレット
		i = 0;
		GetObject(di->data, sizeof(WORD), &i);
		size = sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * i);
		if ((ret = mem_calloc(size)) == NULL) {
			break;
		}
		((LOGPALETTE *)ret)->palVersion = 0x300;
		((LOGPALETTE *)ret)->palNumEntries = (WORD)i;
		GetPaletteEntries(di->data, 0, i, ((LOGPALETTE *)ret)->palPalEntry);
		break;

	case CF_DSPBITMAP:
	case CF_BITMAP:
		// ビットマップ
		if ((ret = bitmap_to_dib(di->data, &size)) == NULL) {
			break;
		}
		break;

	case CF_OWNERDISPLAY:
		break;

	case CF_DSPMETAFILEPICT:
	case CF_METAFILEPICT:
		// メタファイル
		if ((tmp = GlobalLock(di->data)) == NULL) {
			break;
		}
		size = GetMetaFileBitsEx(((METAFILEPICT *)tmp)->hMF, 0, NULL);
		if ((ret = mem_alloc(size + sizeof(METAFILEPICT))) == NULL) {
			GlobalUnlock(di->data);
			break;
		}
		CopyMemory(ret, tmp, sizeof(METAFILEPICT));
		if (GetMetaFileBitsEx(((METAFILEPICT *)tmp)->hMF, size, ret + sizeof(METAFILEPICT)) == 0) {
			mem_free(&ret);
			GlobalUnlock(di->data);
			break;
		}
		size += sizeof(METAFILEPICT);
		GlobalUnlock(di->data);
		break;

	case CF_DSPENHMETAFILE:
	case CF_ENHMETAFILE:
		// 拡張メタファイル
		size = GetEnhMetaFileBits(di->data, 0, NULL);
		if ((ret = mem_alloc(size)) == NULL) {
			break;
		}
		if (GetEnhMetaFileBits(di->data, size, ret) == 0) {
			mem_free(&ret);
			break;
		}
		break;

	default:
		// その他
		if ((tmp = GlobalLock(di->data)) == NULL) {
			break;
		}
		size = di->size;
		if ((ret = mem_alloc(size)) == NULL) {
			GlobalUnlock(di->data);
			break;
		}
		CopyMemory(ret, tmp, size);
		GlobalUnlock(di->data);
		break;
	}
	if (ret_size != NULL) {
		*ret_size = size;
	}
	return ret;
}

/*
 * clipboard_bytes_to_data - バイト列をデータに変換
 */
HANDLE clipboard_bytes_to_data(TCHAR *format_name, const BYTE *data, DWORD *size)
{
	HANDLE ret = NULL;
	BYTE *to_mem;

	if (data == NULL) {
		return NULL;
	}
	switch (clipboard_get_format(0, format_name)) {
	case CF_PALETTE:
		// パレット
		ret = CreatePalette((LOGPALETTE *)data);
		break;

	case CF_DSPBITMAP:
	case CF_BITMAP:
		// ビットマップ
		ret = dib_to_bitmap(data);
		break;

	case CF_OWNERDISPLAY:
		break;

	case CF_DSPMETAFILEPICT:
	case CF_METAFILEPICT:
		// メタファイル
		if ((ret = GlobalAlloc(GHND, sizeof(METAFILEPICT))) == NULL) {
			break;
		}
		if ((to_mem = GlobalLock(ret)) == NULL) {
			GlobalFree(ret);
			ret = NULL;
			break;
		}

		CopyMemory(to_mem, data, sizeof(METAFILEPICT));
		if ((((METAFILEPICT *)to_mem)->hMF = SetMetaFileBitsEx(*size - sizeof(METAFILEPICT), data + sizeof(METAFILEPICT))) == NULL) {
			GlobalUnlock(ret);
			GlobalFree(ret);
			ret = NULL;
			break;
		}
		GlobalUnlock(ret);
		break;

	case CF_DSPENHMETAFILE:
	case CF_ENHMETAFILE:
		ret = SetEnhMetaFileBits(*size, data);
		break;

	default:
		// その他
		// コピー先確保
		if ((ret = GlobalAlloc(GHND, *size)) == NULL) {
			return NULL;
		}
		// コピー先ロック
		if ((to_mem = GlobalLock(ret)) == NULL) {
			GlobalFree(ret);
			return NULL;
		}
		// コピー
		CopyMemory(to_mem, data, *size);
		// ロック解除
		GlobalUnlock(ret);
		break;
	}
	return ret;
}

/*
 * clipboard_data_to_file - データをファイルに保存
 */
BOOL clipboard_data_to_file(DATA_INFO *di, const TCHAR *file_name, const int filter_index, TCHAR *err_str)
{
	HMETAFILE hMeta;
	HENHMETAFILE enh_meta;
	BYTE *tmp;
	DWORD size;

	if (di->data == NULL) {
		if (file_write_buf(file_name, NULL, 0, err_str) == FALSE) {
			return FALSE;
		}
		return TRUE;
	}

	switch (di->format) {
	case CF_OWNERDISPLAY:
		return FALSE;

	case CF_DSPMETAFILEPICT:
	case CF_METAFILEPICT:
		// メタファイル
		if ((tmp = GlobalLock(di->data)) == NULL) {
			message_get_error(GetLastError(), err_str);
			return FALSE;
		}
		if ((hMeta = CopyMetaFile(((METAFILEPICT *)tmp)->hMF, file_name)) == NULL) {
			message_get_error(GetLastError(), err_str);
			GlobalUnlock(di->data);
			return FALSE;
		}
		DeleteMetaFile(hMeta);

		GlobalUnlock(di->data);
		break;

	case CF_DSPENHMETAFILE:
	case CF_ENHMETAFILE:
		// 拡張メタファイル
		if ((enh_meta = CopyEnhMetaFile(di->data, file_name)) == NULL) {
			message_get_error(GetLastError(), err_str);
			return FALSE;
		}
		DeleteEnhMetaFile(enh_meta);
		break;

	default:
		// その他
		// データをバイト列に変換
		if ((tmp = clipboard_data_to_bytes(di, &size)) == NULL) {
			message_get_error(GetLastError(), err_str);
			return FALSE;
		}
		// ファイルに書き込む
		if (file_write_buf(file_name, tmp, di->size, err_str) == FALSE) {
			mem_free(&tmp);
			return FALSE;
		}
		mem_free(&tmp);
		break;
	}
	return TRUE;
}

/*
 * clipboard_file_to_data - ファイルからデータを作成
 */
HANDLE clipboard_file_to_data(const TCHAR *file_name, TCHAR *format_name, DWORD *ret_size, TCHAR *err_str)
{
	HANDLE ret = NULL;
	BYTE *data;
	BYTE *mem;
	DWORD size;

	switch (clipboard_get_format(0, format_name)) {
	case CF_OWNERDISPLAY:
		return NULL;

	case CF_DSPMETAFILEPICT:
	case CF_METAFILEPICT:
		// メタファイル
		if ((ret = GlobalAlloc(GHND, sizeof(METAFILEPICT))) == NULL) {
			message_get_error(GetLastError(), err_str);
			return NULL;
		}
		if ((mem = GlobalLock(ret)) == NULL) {
			message_get_error(GetLastError(), err_str);
			GlobalFree(ret);
			return NULL;
		}
		if ((((METAFILEPICT *)mem)->hMF = GetMetaFile(file_name)) == NULL) {
			message_get_error(GetLastError(), err_str);
			GlobalUnlock(ret);
			GlobalFree(ret);
			return NULL;
		}
		size = GetMetaFileBitsEx(((METAFILEPICT *)mem)->hMF, 0, NULL);
		GlobalUnlock(ret);
		break;

	case CF_DSPENHMETAFILE:
	case CF_ENHMETAFILE:
		if ((ret = GetEnhMetaFile(file_name)) == NULL) {
			message_get_error(GetLastError(), err_str);
		}
		size = GetEnhMetaFileBits(ret, 0, NULL);
		break;

	default:
		// その他
		// ファイルの読み込み
		if ((data = file_read_buf(file_name, &size, err_str)) == NULL) {
			return NULL;
		}
		// バイト列をデータに変換
		ret = clipboard_bytes_to_data(format_name, data, &size);
		mem_free(&data);
		break;
	}
	if (ret_size != NULL) {
		*ret_size = size;
	}
	return ret;
}

/*
 * clipboard_free_data - クリップボード形式毎のメモリの解放
 */
BOOL clipboard_free_data(TCHAR *format_name, HANDLE data)
{
	BOOL ret = FALSE;
	BYTE *mem;

	if (data == NULL) {
		return TRUE;
	}

	switch (clipboard_get_format(0, format_name)) {
	case CF_PALETTE:
		// パレット
		ret = DeleteObject((HGDIOBJ)data);
		break;

	case CF_DSPBITMAP:
	case CF_BITMAP:
		// ビットマップ
		ret = DeleteObject((HGDIOBJ)data);
		break;

	case CF_OWNERDISPLAY:
		break;

	case CF_DSPMETAFILEPICT:
	case CF_METAFILEPICT:
		// メタファイル
		if ((mem = GlobalLock(data)) != NULL) {
			DeleteMetaFile(((METAFILEPICT *)mem)->hMF);
			GlobalUnlock(data);
		}
		if (GlobalFree((HGLOBAL)data) == NULL) {
			ret = TRUE;
		}
		break;

	case CF_DSPENHMETAFILE:
	case CF_ENHMETAFILE:
		// 拡張メタファイル
		ret = DeleteEnhMetaFile((HENHMETAFILE)data);
		break;

	default:
		// その他
		if (GlobalFree((HGLOBAL)data) == NULL) {
			ret = TRUE;
		}
		break;
	}
	return ret;
}
/* End of source */
