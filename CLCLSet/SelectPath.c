/*
 * CLCLSet
 *
 * SelectPath.c
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE
#include <commctrl.h>

#include "..\General.h"
#include "..\Message.h"
#include "..\Data.h"

#include "resource.h"

/* Define */
#define SICONSIZE						16

#define ICON_REGIST						0
#define ICON_FOLDER						1
#define ICON_FOLDER_OPEN				2

/* Global Variables */
static HINSTANCE hInst;
static HTREEITEM regist_treeitem;
static TCHAR *ret_str;

extern TCHAR work_path[];

/* Local Function Prototypes */
static int imagelist_icon_add(const HINSTANCE hInstance, const HIMAGELIST icon_list, const int index);
static int imagelist_fileicon_add(const HIMAGELIST icon_list, const TCHAR *path, const UINT flag);
static HIMAGELIST create_imagelist(const HINSTANCE hInstance);
static HTREEITEM treeview_set_item(const HWND hTreeView, TCHAR *buf, const HTREEITEM hParent, const HTREEITEM After,
							const int icon, const int select_icon, LPARAM lParam);
static void treeview_datainfo_to_treeitem(const HWND hTreeView, const HTREEITEM parent_item, DATA_INFO *di);
static BOOL treeview_get_text(const HWND hTreeView, const HTREEITEM hItem, TCHAR *text);
static void treeview_get_path(const HWND hTreeView, const HTREEITEM root, const HTREEITEM hItem, TCHAR *ret);
static HTREEITEM treeview_path_to_item(const HWND hTreeView, const HTREEITEM hItem, TCHAR *path);
static BOOL CALLBACK select_folder_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/*
 * imagelist_icon_add - イメージリストにアイコンを追加
 *
 *	ファイルが指定されていない場合はリソースから取得
 */
static int imagelist_icon_add(const HINSTANCE hInstance, const HIMAGELIST icon_list, const int index)
{
	HICON hIcon;
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

	SHGetFileInfo(path, SHGFI_USEFILEATTRIBUTES, &shfi, sizeof(SHFILEINFO),
		SHGFI_ICON | SHGFI_SMALLICON | flag);
	hIcon = shfi.hIcon;
	// イメージリストにアイコンを追加
	ret = ImageList_AddIcon(icon_list, hIcon);
	DestroyIcon(hIcon);
	return ret;
}

/*
 * create_imagelist - イメージリストの作成
 */
static HIMAGELIST create_imagelist(const HINSTANCE hInstance)
{
	HIMAGELIST icon_list;

	icon_list = ImageList_Create(SICONSIZE, SICONSIZE, ILC_COLOR16 | ILC_MASK, 0, 0);
	ImageList_SetBkColor(icon_list, GetSysColor(COLOR_WINDOW));

	imagelist_icon_add(hInstance, icon_list, IDI_ICON_REGIST);
	// フォルダ
	imagelist_fileicon_add(icon_list, work_path, 0);
	imagelist_fileicon_add(icon_list, work_path, SHGFI_OPENICON);
	return icon_list;
}

/*
 * treeview_set_item - ツリービューアイテムの追加
 */
static HTREEITEM treeview_set_item(const HWND hTreeView, TCHAR *buf, const HTREEITEM hParent, const HTREEITEM After,
							const int icon, const int select_icon, LPARAM lParam)
{
	TV_INSERTSTRUCT tvitn;
	TV_ITEM tvit;

	tvit.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
	tvit.iImage = icon;
	tvit.iSelectedImage = select_icon;
	tvit.hItem = NULL;
	tvit.state = 0;
	tvit.stateMask = 0;
	tvit.cChildren = 0;
	tvit.lParam = lParam;
	tvit.pszText = buf;
	tvit.cchTextMax = BUF_SIZE - 1;

	tvitn.hInsertAfter = After;
	tvitn.hParent = hParent;
	tvitn.item = tvit;
	return TreeView_InsertItem(hTreeView, &tvitn);
}

/*
 * treeview_datainfo_to_treeitem - ツリービューにデータを設定
 */
static void treeview_datainfo_to_treeitem(const HWND hTreeView, const HTREEITEM parent_item, DATA_INFO *di)
{
	HTREEITEM hItem;

	for (; di != NULL; di = di->next) {
		switch (di->type) {
		case TYPE_FOLDER:
			// フォルダ
			hItem = treeview_set_item(hTreeView, di->title, parent_item, (HTREEITEM)TVI_LAST,
				ICON_FOLDER, ICON_FOLDER_OPEN, (LPARAM)di);
			if (hItem == NULL) {
				return;
			}
			treeview_datainfo_to_treeitem(hTreeView, hItem, di->child);
			break;

		case TYPE_ITEM:
		case TYPE_DATA:
			break;
		}
	}
}

/*
 * treeview_get_text - アイテムのテキストを取得
 */
static BOOL treeview_get_text(const HWND hTreeView, const HTREEITEM hItem, TCHAR *text)
{
	TV_ITEM tvit;

	if (hItem == NULL) {
		return FALSE;
	}
	tvit.mask = TVIF_TEXT;
	tvit.hItem = hItem;
	tvit.pszText = text;
	tvit.cchTextMax = BUF_SIZE - 1;
	return TreeView_GetItem(hTreeView, &tvit);
}

/*
 * TreeView_GetPath - ツリービューアイテムのパスを取得する
 */
static void treeview_get_path(const HWND hTreeView, const HTREEITEM root, const HTREEITEM hItem, TCHAR *ret)
{
	HTREEITEM pItem;
	TCHAR buf[BUF_SIZE];
	TCHAR name[BUF_SIZE];
	TCHAR work[BUF_SIZE];

	// ルートアイテムの場合はそのままパスを返す
	if (hItem == root) {
		lstrcpy(ret, TEXT("\\"));
		return;
	}

	if (treeview_get_text(hTreeView, hItem, buf) == -1) {
		*ret = TEXT('\0');
		return;
	}
	pItem = TreeView_GetParent(hTreeView, hItem);
	while (pItem != root) {
		treeview_get_text(hTreeView, pItem, name);
		wsprintf(work, TEXT("%s\\%s"), name, buf);
		lstrcpy(buf, work);

		pItem = TreeView_GetParent(hTreeView, pItem);
	}
	// 指定の文字列と結合
	wsprintf(ret, TEXT("\\%s"), buf);
}

/*
 * treeview_path_to_item - パスからアイテムを検索
 */
static HTREEITEM treeview_path_to_item(const HWND hTreeView, const HTREEITEM hItem, TCHAR *path)
{
	HTREEITEM cItem;
	TCHAR buf[BUF_SIZE];
	TCHAR name[BUF_SIZE];
	TCHAR *p, *r;

	if (*path == TEXT('\0')) {
		return NULL;
	}

	p = path;
	if (*p == TEXT('\\') || *p == TEXT('/')) {
		p++;
	}
	if (*p == '\0') {
		return hItem;
	}
	// パスを展開
	for (r = buf; *p != TEXT('\0'); p++, r++) {
#ifndef UNICODE
		if (IsDBCSLeadByte((BYTE)*p) == TRUE) {
			*(r++) = *(p++);
			*r = *p;
			continue;
		}
#endif
		if (*p == TEXT('\\') || *p == TEXT('/')) {
			break;
		}
		*r = *p;
	}
	*r = TEXT('\0');

	// 名前からアイテムを取得
	cItem = TreeView_GetChild(hTreeView, hItem);
	while (cItem != NULL) {
		treeview_get_text(hTreeView, cItem, name);
		if (lstrcmpi(buf, name) == 0) {
			break;
		}
		cItem = TreeView_GetNextSibling(hTreeView, cItem);
	}
	if (cItem == NULL) {
		return NULL;
	}

	if (*p != TEXT('\0')) {
		// 再帰
		return treeview_path_to_item(hTreeView, cItem, p);
	}
	return cItem;
}

/*
 * select_folder_proc - フォルダ選択ウィンドウプロシージャ
 */
static BOOL CALLBACK select_folder_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HTREEITEM hItem;

	switch (uMsg) {
	case WM_INITDIALOG:
		// イメージリストの設定
		TreeView_SetImageList(GetDlgItem(hDlg, IDC_TREE), create_imagelist(hInst), TVSIL_NORMAL);
		// アイテムの作成
		regist_treeitem = treeview_set_item(GetDlgItem(hDlg, IDC_TREE),  message_get_res(IDS_TREEITEM_REGIST),
			(HTREEITEM)TVI_ROOT, (HTREEITEM)TVI_LAST, ICON_REGIST, ICON_REGIST, 0);
		treeview_datainfo_to_treeitem(GetDlgItem(hDlg, IDC_TREE), regist_treeitem, (DATA_INFO *)lParam);
		TreeView_Expand(GetDlgItem(hDlg, IDC_TREE),
			TreeView_GetRoot(GetDlgItem(hDlg, IDC_TREE)), TVE_EXPAND);

		// パスからアイテムを検索
		if ((hItem = treeview_path_to_item(GetDlgItem(hDlg, IDC_TREE), regist_treeitem, ret_str)) != NULL) {
			TreeView_SelectItem(GetDlgItem(hDlg, IDC_TREE), hItem);
		}
		break;

	case WM_CLOSE:
		EndDialog(hDlg, FALSE);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			SendMessage(hDlg, WM_CLOSE, 0, 0);
			break;

		case IDOK:
			// 選択アイテムのパス取得
			treeview_get_path(GetDlgItem(hDlg, IDC_TREE), regist_treeitem,
				TreeView_GetSelection(GetDlgItem(hDlg, IDC_TREE)), ret_str);
			EndDialog(hDlg, TRUE);
			break;
		}
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

/*
 * select_path - パスの選択
 */
BOOL select_path(const HINSTANCE hInstance, const HWND hWnd, const DATA_INFO *di, TCHAR *ret)
{
	hInst = hInstance;
	ret_str = ret;
	return DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_SELECT_FOLDER), hWnd, select_folder_proc, (LPARAM)di);
}
/* End of source */
