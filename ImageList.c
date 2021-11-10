/*
 * CLCL
 *
 * ImageList.c
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef	_INC_OLE
#include <commctrl.h>

#include "General.h"
#include "Format.h"
#include "Ini.h"
#include "dpi.h"

#include "resource.h"

/* Define */
#define LICONSIZE			Scale(32)
#define SICONSIZE			Scale(16)

/* Global Variables */
extern TCHAR work_path[];

// オプション
extern OPTION_INFO option;

/* Local Function Prototypes */
static int imagelist_icon_add(const HINSTANCE hInstance, const HIMAGELIST icon_list, const int index);
static int imagelist_fileicon_add(const HIMAGELIST icon_list, const TCHAR *path, const UINT flag);

/*
 * imagelist_icon_add - イメージリストにアイコンを追加
 *
 *	ファイルが指定されていない場合はリソースから取得
 */
static int imagelist_icon_add(const HINSTANCE hInstance, const HIMAGELIST icon_list, const int index)
{
	HICON hIcon = NULL;
	int ret;

	hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(index), IMAGE_ICON,
		SICONSIZE, SICONSIZE, 0);
	// イメージリストにアイコンを追加
	ret = ImageList_AddIcon(icon_list, hIcon);
	DestroyIcon(hIcon);
	return ret;
}

/*
 * imagelist_fileicon_add - イメージリストに関連付けされたアイコンを追加
 */
static int imagelist_fileicon_add(const HIMAGELIST icon_list, const TCHAR *path, const UINT flag)
{
	SHFILEINFO shfi;
	HICON hIcon;
	int ret;

	if (SHGetFileInfo(path, SHGFI_USEFILEATTRIBUTES, &shfi, sizeof(SHFILEINFO),
		SHGFI_ICON | SHGFI_SMALLICON | flag) == 0) {
		return -1;
	}
	hIcon = shfi.hIcon;
	if (hIcon == NULL) {
		return -1;
	}
	// イメージリストにアイコンを追加
	ret = ImageList_AddIcon(icon_list, hIcon);
	DestroyIcon(hIcon);
	return ret;
}

/*
 * create_imagelist - イメージリストの作成
 */
HIMAGELIST create_imagelist(const HINSTANCE hInstance)
{
	HIMAGELIST icon_list;
	HICON hIcon;
	int i;
	BOOL free_icon;

	icon_list = ImageList_Create(SICONSIZE, SICONSIZE, ILC_COLOR4 | ILC_MASK, 0, 0);
	ImageList_SetBkColor(icon_list, GetSysColor(COLOR_WINDOW));

	imagelist_icon_add(hInstance, icon_list, IDI_ICON_CLIPBOARD);
	imagelist_icon_add(hInstance, icon_list, IDI_ICON_MAIN);
	imagelist_icon_add(hInstance, icon_list, IDI_ICON_REGIST);
	imagelist_icon_add(hInstance, icon_list, IDI_ICON_FOLDER);
	imagelist_icon_add(hInstance, icon_list, IDI_ICON_FOLDER);
	// 未定義の形式アイコン
	imagelist_icon_add(hInstance, icon_list, IDI_ICON_DEFAULT);

	// 形式毎のアイコン追加
	for (i = 0; i < option.format_cnt; i++) {
		free_icon = TRUE;
		if ((hIcon = format_get_icon(i, SICONSIZE, &free_icon)) == NULL) {
			imagelist_icon_add(hInstance, icon_list, IDI_ICON_DEFAULT);
		} else {
			ImageList_AddIcon(icon_list, hIcon);
			if (free_icon == TRUE) {
				DestroyIcon(hIcon);
			}
		}
	}
	return icon_list;
}
/* End of source */
