/*
 * CLCL
 *
 * SelectFolder.c
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

#include "General.h"
#include "ImageList.h"
#include "TreeView.h"

#include "resource.h"

/* Define */

/* Global Variables */
static HWND from_treeview;
static HTREEITEM from_root_item;
static HTREEITEM ret_item;
static TCHAR *message;

/* Local Function Prototypes */
static BOOL treeview_copy_tree(const HWND from_tv, const HTREEITEM from_item, const HWND to_tv, const HTREEITEM parent_item);
static BOOL CALLBACK select_folder_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/*
 * treeview_copy_tree - ツリビューアイテムのコピーを作成
 */
static BOOL treeview_copy_tree(const HWND from_tv, const HTREEITEM from_item, const HWND to_tv, const HTREEITEM parent_item)
{
	TV_INSERTSTRUCT tvitn;
	TV_ITEM tvit;
	HTREEITEM new_item;
	HTREEITEM cItem;
	TCHAR buf[BUF_SIZE];

	tvit.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
	tvit.hItem = from_item;
	tvit.cchTextMax = BUF_SIZE - 1;
	tvit.pszText = buf;
	tvit.iImage = 0;
	tvit.iSelectedImage = 0;
	if (TreeView_GetItem(from_tv, &tvit) == FALSE) {
		return FALSE;
	}
	if (tvit.iImage != ICON_REGIST && tvit.iImage != ICON_FOLDER) {
		return TRUE;
	}

	tvit.lParam = (LPARAM)from_item;
	tvit.hItem = NULL;
	tvit.cChildren = 0;

	tvitn.hParent = (HTREEITEM)parent_item;
	tvitn.item = tvit;
	if ((new_item = TreeView_InsertItem(to_tv, &tvitn)) == NULL) {
		return FALSE;
	}

	cItem = TreeView_GetChild(from_tv, from_item);
	while (cItem != NULL) {
		// 再帰
		if (treeview_copy_tree(from_tv, cItem, to_tv, new_item) == FALSE) {
			return FALSE;
		}
		cItem = TreeView_GetNextSibling(from_tv, cItem);
	}
	return TRUE;
}

/*
 * select_folder_proc - フォルダ選択ウィンドウプロシージャ
 */
static BOOL CALLBACK select_folder_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		SendMessage(GetDlgItem(hDlg, IDC_STATIC_MSG), WM_SETTEXT, 0, (LPARAM)message);

		// イメージリストの設定
		TreeView_SetImageList(GetDlgItem(hDlg, IDC_TREE),
			TreeView_GetImageList(from_treeview, TVSIL_NORMAL), TVSIL_NORMAL);

		// アイテムのコピー
		treeview_copy_tree(from_treeview, from_root_item, GetDlgItem(hDlg, IDC_TREE), TVI_ROOT);
		TreeView_Expand(GetDlgItem(hDlg, IDC_TREE),
			TreeView_GetRoot(GetDlgItem(hDlg, IDC_TREE)), TVE_EXPAND);
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
			// 選択アイテムの取得
			ret_item = (HTREEITEM)treeview_get_lparam(GetDlgItem(hDlg, IDC_TREE),
				TreeView_GetSelection(GetDlgItem(hDlg, IDC_TREE)));
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
 * select_folder - フォルダの選択
 */
HTREEITEM select_folder(const HINSTANCE hInst, const HWND hWnd, const HWND hTreeView, const HTREEITEM root_item, TCHAR *msg)
{
	from_treeview = hTreeView;
	from_root_item = root_item;
	message = msg;
	ret_item = NULL;

	DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_SELECT_FOLDER), hWnd, select_folder_proc, 0);
	return ret_item;
}
/* End of source */
