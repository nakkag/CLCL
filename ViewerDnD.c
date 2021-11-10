/*
 * CLCL
 *
 * ViewerDnD.c
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
#include "Data.h"
#include "Ini.h"
#include "Message.h"
#include "Viewer.h"
#include "TreeView.h"
#include "ListView.h"
#include "ViewerDnD.h"

#include "resource.h"

/* Define */
#define ERROR_TITLE						TEXT("CLCL - Error")

#define POPUPMENU_DRAGDROP				5

/* Global Variables */
static HTREEITEM drag_item;

// extern
extern HTREEITEM clip_treeitem;
extern HTREEITEM history_treeitem;
extern HTREEITEM regist_treeitem;

extern HMENU h_popup_menu;

// オプション
extern OPTION_INFO option;

/* ocal Function Prototypes */
static int get_drag_mode(const HWND hWnd, const BOOL mode);

/*
 * dragdrop_show_menu - ドラッグ＆ドロップ用のメニューを表示する
 */
int dragdrop_show_menu(const HWND hWnd, const int mode, const BOOL def_move)
{
	UINT ret;
	int ret_mode;

	// 移動メニューの設定
	EnableMenuItem(GetSubMenu(h_popup_menu, POPUPMENU_DRAGDROP), ID_MENU_DGDP_MOVE,
		(mode & DRAG_MODE_MOVE) ? MF_ENABLED : MF_GRAYED);
	// デフォルト項目の設定
	SetMenuDefaultItem(GetSubMenu(h_popup_menu, POPUPMENU_DRAGDROP),
		(def_move == TRUE) ? ID_MENU_DGDP_MOVE : ID_MENU_DGDP_COPY, FALSE);

	// メニューの表示
	_SetForegroundWindow(hWnd);
	ret = menu_show(hWnd, GetSubMenu(h_popup_menu, POPUPMENU_DRAGDROP), NULL);
	switch (ret) {
	case ID_MENU_DGDP_MOVE:
		ret_mode = DRAG_MODE_MOVE;
		break;

	case ID_MENU_DGDP_COPY:
		ret_mode = DRAG_MODE_COPY;
		break;

	default:
		ret_mode = DRAG_MODE_NONE;
		break;
	}
	return ret_mode;
}

/*
 * dragdrop_get_drop_folder - ドロップされたフォルダを取得
 */
HTREEITEM dragdrop_get_drop_folder(const HWND hWnd, const int x, const int y)
{
	HWND hTreeView = GetDlgItem(hWnd, ID_TREE);
	HWND hListView = GetDlgItem(GetDlgItem(hWnd, ID_CONTAINER), ID_LIST);
	HWND point_wnd;
	POINT pt;
	HTREEITEM hItem;
	DATA_INFO *di;
	int iItem;

	// マウスの下のウィンドウ取得
	pt.x = x; pt.y = y;
	if ((point_wnd = WindowFromPoint(pt)) == hTreeView) {
		// ツリービュー
		if ((hItem = treeview_get_hitest(hTreeView)) == NULL) {
			return NULL;
		}
		if (hItem == clip_treeitem || hItem == history_treeitem || hItem == regist_treeitem) {
			// ルートアイテム
			return hItem;
		}
		if ((di = (DATA_INFO *)treeview_get_lparam(hTreeView, hItem)) == NULL || di->type != TYPE_FOLDER) {
			return NULL;
		}
		return hItem;

	} else if (point_wnd == hListView) {
		// リストビュー
		if ((iItem = listview_get_hitest(hListView)) == -1 ||
			(hItem = (HTREEITEM)listview_get_lparam(hListView, iItem)) == NULL ||
			(di = (DATA_INFO *)treeview_get_lparam(hTreeView, hItem)) == NULL ||
			di->type != TYPE_FOLDER) {
			return TreeView_GetSelection(hTreeView);
		}
		return hItem;
	}
	return NULL;
}

/*
 * dragdrop_start_drag - ドラッグ＆ドロップを開始
 */
BOOL dragdrop_start_drag(const HWND hWnd, const HTREEITEM sel_item)
{
	HWND hTreeView = GetDlgItem(hWnd, ID_TREE);
	HWND hListView = GetDlgItem(GetDlgItem(hWnd, ID_CONTAINER), ID_LIST);
	HTREEITEM hItem = sel_item;

	if (sel_item != NULL) {
		if (hItem != clip_treeitem &&
			(DATA_INFO *)treeview_get_lparam(hTreeView, hItem) == NULL) {
			return FALSE;
		}
		drag_item = hItem;

	} else {
		if (ListView_GetSelectedCount(hListView) == 0) {
			return FALSE;
		}
		drag_item = NULL;
	}
	SetCapture(hWnd);
	return TRUE;
}

/*
 * get_drag_mode - ドラッグ中のモードを取得
 */
static int get_drag_mode(const HWND hWnd, const BOOL mode)
{
	HWND hTreeView = GetDlgItem(hWnd, ID_TREE);
	HWND hListView = GetDlgItem(GetDlgItem(hWnd, ID_CONTAINER), ID_LIST);
	HTREEITEM hItem;
	HTREEITEM to_item;
	HTREEITEM wk_item;
	POINT pt;
	DATA_INFO *di;
	int ret = DRAG_MODE_COPY;
	int i;

	GetCursorPos(&pt);
	if ((to_item = dragdrop_get_drop_folder(hWnd, pt.x, pt.y)) == NULL) {
		return DRAG_MODE_NONE;
	}

	if (drag_item != NULL) {
		// ツリービューからのドラッグ
		if (drag_item == to_item || TreeView_GetParent(hTreeView, drag_item) == to_item) {
			return DRAG_MODE_NONE;
		} else if (to_item == clip_treeitem) {
			return DRAG_MODE_NONE;
		} else if (to_item == history_treeitem || to_item == regist_treeitem) {
			// ルートアイテム
		} else {
			if ((di = (DATA_INFO *)treeview_get_lparam(hTreeView, to_item)) == NULL || di->type != TYPE_FOLDER) {
				// フォルダ以外
				return DRAG_MODE_NONE;
			}
		}
		if ((di = (DATA_INFO *)treeview_get_lparam(hTreeView, drag_item)) == NULL) {
			return DRAG_MODE_NONE;
		}
		if (di->type == TYPE_FOLDER) {
			if (to_item == drag_item || to_item == clip_treeitem ||
				treeview_get_rootitem(hTreeView, to_item) == history_treeitem) {
				return DRAG_MODE_NONE;
			}
			wk_item = to_item;
			while ((wk_item = TreeView_GetParent(hTreeView, wk_item)) != NULL) {
				if (wk_item == drag_item) {
					return DRAG_MODE_NONE;
				}
			}
		}
		if (di->type != TYPE_DATA &&
			(treeview_get_rootitem(hTreeView, to_item) == treeview_get_rootitem(hTreeView, drag_item) ||
			(to_item == clip_treeitem && treeview_get_rootitem(hTreeView, drag_item) == history_treeitem)) &&
			(mode == TRUE || GetKeyState(VK_CONTROL) >= 0)) {
			// 移動
			ret |= DRAG_MODE_MOVE;
		} else if (treeview_get_rootitem(hTreeView, drag_item) != clip_treeitem &&
			treeview_get_rootitem(hTreeView, to_item) != treeview_get_rootitem(hTreeView, drag_item) &&
			GetKeyState(VK_CONTROL) < 0) {
			ret |= DRAG_MODE_MOVE;
		}
		return ret;
	}

	// リストビューからのドラッグ
	if (treeview_get_rootitem(hTreeView, to_item) == treeview_get_rootitem(hTreeView, TreeView_GetSelection(hTreeView)) &&
		(mode == TRUE || GetKeyState(VK_CONTROL) >= 0)) {
		ret |= DRAG_MODE_MOVE;
	} else if (treeview_get_rootitem(hTreeView, to_item) != treeview_get_rootitem(hTreeView, TreeView_GetSelection(hTreeView)) &&
		GetKeyState(VK_CONTROL) < 0) {
		ret |= DRAG_MODE_MOVE;
	}
	i = -1;
	while ((i = ListView_GetNextItem(hListView, i, LVNI_SELECTED)) != -1) {
		hItem = (HTREEITEM)listview_get_lparam(hListView, i);
		if (hItem == NULL) {
			return DRAG_MODE_NONE;
		}
		if (hItem == to_item || TreeView_GetParent(hTreeView, hItem) == to_item) {
			return DRAG_MODE_NONE;
		} else if (to_item == clip_treeitem) {
			return DRAG_MODE_NONE;
		} else if (to_item == clip_treeitem || to_item == history_treeitem || to_item == regist_treeitem) {
			// ルートアイテム
		} else {
			if ((di = (DATA_INFO *)treeview_get_lparam(hTreeView, to_item)) == NULL || di->type != TYPE_FOLDER) {
				// フォルダ以外
				return DRAG_MODE_NONE;
			}
		}
		if ((di = (DATA_INFO *)treeview_get_lparam(hTreeView, hItem)) == NULL) {
			return DRAG_MODE_NONE;
		}
		if (di->type == TYPE_FOLDER) {
			if (to_item == hItem || to_item == clip_treeitem ||
				treeview_get_rootitem(hTreeView, to_item) == history_treeitem) {
				return DRAG_MODE_NONE;
			}
			wk_item = to_item;
			while ((wk_item = TreeView_GetParent(hTreeView, wk_item)) != NULL) {
				if (wk_item == hItem) {
					return DRAG_MODE_NONE;
				}
			}
		}
	}
	return ret;
}

/*
 * dragdrop_set_drag_item - ドラッグ中のアイテムを設定
 */
int dragdrop_set_drag_item(const HWND hWnd)
{
	HWND hTreeView = GetDlgItem(hWnd, ID_TREE);
	HWND hListView = GetDlgItem(GetDlgItem(hWnd, ID_CONTAINER), ID_LIST);
	HWND point_wnd;
	POINT pt;
	HTREEITEM hItem;
	DATA_INFO *di;
	int iItem;
	int ret;

	ret = get_drag_mode(hWnd, FALSE);

	// マウスの下のウィンドウ取得
	GetCursorPos(&pt);
	if ((point_wnd = WindowFromPoint(pt)) == hTreeView) {
		ListView_SetItemState(hListView, -1, 0, LVIS_DROPHILITED);

		if ((hItem = treeview_get_hitest(hTreeView)) == NULL) {
			TreeView_SelectDropTarget(hTreeView, NULL);
			return DRAG_MODE_NONE;
		}
		// アイテムを画面上に表示させる
		treeview_scroll(hTreeView);
		if (ret == DRAG_MODE_NONE) {
			TreeView_SelectDropTarget(hTreeView, NULL);
		} else {
			TreeView_SelectDropTarget(hTreeView, hItem);
		}

	} else if (ret != DRAG_MODE_NONE && point_wnd == hListView) {
		TreeView_SelectDropTarget(hTreeView, NULL);

		if ((iItem = listview_get_hitest(hListView)) == -1) {
			ListView_SetItemState(hListView, -1, 0, LVIS_DROPHILITED);
			return ret;
		}
		// アイテムを画面上に表示させる
		ListView_EnsureVisible(hListView, iItem - 1, TRUE);
		ListView_EnsureVisible(hListView, iItem + 1, TRUE);

		// フォルダのチェック
		if ((hItem = (HTREEITEM)listview_get_lparam(hListView, iItem)) == NULL) {
			ListView_SetItemState(hListView, -1, 0, LVIS_DROPHILITED);
			return DRAG_MODE_NONE;
		}
		if ((di = (DATA_INFO *)treeview_get_lparam(hTreeView, hItem)) == NULL || di->type != TYPE_FOLDER) {
			ListView_SetItemState(hListView, -1, 0, LVIS_DROPHILITED);
			return ret;
		}

		// 既にハイライト済み
		if (ListView_GetItemState(hListView, iItem, LVIS_DROPHILITED) == LVIS_DROPHILITED) {
			return ret;
		}
		// マウスの下のアイテムをハイライト状態にする
		ListView_SetItemState(hListView, -1, 0, LVIS_DROPHILITED);
		ListView_SetItemState(hListView, iItem, LVIS_DROPHILITED, LVIS_DROPHILITED);

	} else {
		// ハイライト解除
		TreeView_SelectDropTarget(hTreeView, NULL);
		ListView_SetItemState(hListView, -1, 0, LVIS_DROPHILITED);
		ret = DRAG_MODE_NONE;
	}
	return ret;
}

/*
 * dragdrop_drop_item - ドロップ処理
 */
BOOL dragdrop_drop_item(const HWND hWnd, const UINT msg)
{
	HWND hTreeView = GetDlgItem(hWnd, ID_TREE);
	HWND hListView = GetDlgItem(GetDlgItem(hWnd, ID_CONTAINER), ID_LIST);
	HTREEITEM hItem;
	HTREEITEM to_item;
	HTREEITEM ret_item;
	HTREEITEM wk_Item;
	POINT pt;
	TCHAR err_str[BUF_SIZE];
	int mode;
	int i;
	BOOL def_move;

	// ドロップ先のアイテム取得
	GetCursorPos(&pt);
	if ((to_item = dragdrop_get_drop_folder(hWnd, pt.x, pt.y)) == NULL) {
		return FALSE;
	}

	// ドロップモード取得
	if (msg == WM_RBUTTONUP) {
		if ((mode = get_drag_mode(hWnd, TRUE)) != DRAG_MODE_NONE) {
			def_move = (mode & DRAG_MODE_MOVE) ? TRUE : FALSE;
			if (drag_item == NULL || treeview_get_rootitem(hTreeView, drag_item) != clip_treeitem) {
				mode |= DRAG_MODE_MOVE;
			}
			mode = dragdrop_show_menu(hWnd, mode, def_move);
		}
	} else {
		mode = get_drag_mode(hWnd, FALSE);
	}
	if (mode == DRAG_MODE_NONE) {
		return FALSE;
	}

	if (drag_item != NULL) {
		// ツリービュー
		*err_str = ('\0');
		ret_item = viewer_item_copy(hWnd, drag_item, to_item, (mode & DRAG_MODE_MOVE) ? TRUE : FALSE, err_str);
		if (ret_item == NULL) {
			if (*err_str != ('\0')) {
				MessageBox(hWnd, err_str, ERROR_TITLE, MB_ICONERROR);
				return FALSE;
			}
		} else {
			if (mode & DRAG_MODE_MOVE) {
				// 移動
				treeview_delete_item(hTreeView, drag_item);
			}
			TreeView_Expand(hTreeView, to_item, TVM_EXPAND);
			// リストビュー更新
			treeview_to_listview(hTreeView, TreeView_GetSelection(hTreeView), hListView);
			if (TreeView_GetSelection(hTreeView) == to_item) {
				listview_lparam_select(hListView, (LPARAM)ret_item);
			}
		}
	} else {
		// リストビュー
		if (ListView_GetSelectedCount(hListView) == 0) {
			return FALSE;
		}

		// フォルダにアイテムを追加
		ret_item = NULL;
		i = -1;
		while ((i = ListView_GetNextItem(hListView, i, LVNI_SELECTED)) != -1) {
			if ((hItem = (HTREEITEM)listview_get_lparam(hListView, i)) != NULL) {
				*err_str = TEXT('\0');
				wk_Item = viewer_item_copy(hWnd, hItem, to_item, (mode & DRAG_MODE_MOVE) ? TRUE : FALSE, err_str);
				if (wk_Item == NULL && *err_str != TEXT('\0')) {
					MessageBox(hWnd, err_str, ERROR_TITLE, MB_ICONERROR);
					return FALSE;
				}
				if (ret_item == NULL) {
					ret_item = wk_Item;
				}
				if (mode & DRAG_MODE_MOVE) {
					// 移動
					treeview_delete_item(hTreeView, hItem);
					ListView_DeleteItem(hListView, i);
					i = -1;
				}
			}
		}
		TreeView_Expand(hTreeView, to_item, TVM_EXPAND);
		// リストビュー更新
		treeview_to_listview(hTreeView, TreeView_GetSelection(hTreeView), hListView);
		if (TreeView_GetSelection(hTreeView) == to_item) {
			listview_lparam_select(hListView, (LPARAM)ret_item);
		}
	}
	if (treeview_get_rootitem(hTreeView, to_item) == regist_treeitem) {
		// 登録アイテムの保存
		SendMessage(hWnd, WM_REGIST_SAVE, 0, 0);
	} else if (option.history_save == 1 && option.history_always_save == 1 &&
		treeview_get_rootitem(hTreeView, to_item) == history_treeitem) {
		// 履歴の保存
		SendMessage(hWnd, WM_HISTORY_SAVE, 0, 0);
	}
	SendMessage(hWnd, WM_VIEWER_REFRESH_STATUS, 0, 0);
	return TRUE;
}
/* End of source */
