/*
 * CLCLSet
 *
 * SetAction.c
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
#include "..\Memory.h"
#include "..\Ini.h"
#include "..\Menu.h"
#include "..\Message.h"
#include "..\File.h"
#include "..\Data.h"
#include "..\dpi.h"

#include "CLCLSet.h"
#include "SelectIcon.h"
#include "SelectPath.h"
#include "SelectKey.h"

#include "resource.h"

/* Define */
#define WM_TV_EVENT						(WM_APP + 300)

/* Global Variables */
static HTREEITEM root_item;
static DATA_INFO regist_data;

extern HINSTANCE hInst;
extern int prop_ret;
extern TCHAR work_path[];

// オプション
extern OPTION_INFO option;

/* Local Function Prototypes */
static HTREEITEM treeview_set_item(const HWND hTreeView, TCHAR *buf, const HTREEITEM hParent, const HTREEITEM After, LPARAM lParam);
static BOOL treeview_set_text(const HWND hTreeView, const HTREEITEM hItem, TCHAR *text);
static BOOL treeview_set_lparam(const HWND hTreeView, const HTREEITEM hItem, const LPARAM lParam);
static LPARAM treeview_get_lparam(const HWND hTreeView, const HTREEITEM hItem);
static HTREEITEM treeview_copy_item(const HWND hTreeView, const HTREEITEM parent_item, const HTREEITEM hItem, const HTREEITEM After);
static LRESULT treeview_notify_proc(const HWND hWnd, LPARAM lParam);
static HTREEITEM treeview_move_up(const HWND hTreeView);
static HTREEITEM treeview_move_down(const HWND hTreeView);
static void set_enable_control(const HWND hDlg);
static TCHAR *get_tree_text(const MENU_INFO *menu_info);
static void set_menu_item(const HWND hDlg, const MENU_INFO *menu_info, const int menu_cnt, const HTREEITEM pItem);
static MENU_INFO *get_menu_item(const HWND hTreeView, const HTREEITEM pItem, int *cnt);
static void free_menu_item(const HWND hTreeView, const HTREEITEM pItem);
static void set_menu_control(const HWND hDlg, const HTREEITEM se_item);
static void get_menu_control(const HWND hDlg, const HTREEITEM hItem);
static void set_app_title(const HWND hDlg, TCHAR *buf);
static BOOL CALLBACK set_action_item_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static MENU_INFO *copy_menu_info(const MENU_INFO *menu_info, const int menu_cnt);
static ACTION_INFO *copy_action_info(const ACTION_INFO *ai);
static void listview_set_text(const HWND hListView, const int i);
static void listview_set_action(const HWND hListView, ACTION_INFO *ai, const BOOL copy);
static ACTION_INFO *listview_get_action(const HWND hListView, int *cnt);
static void listview_free_action(const HWND hListView);

/*
 * select_tool_proc - ツール選択ウィンドウプロシージャ
 */
static BOOL CALLBACK select_tool_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LV_COLUMN lvc;
	LV_ITEM lvi;
	TCHAR buf[BUF_SIZE];
	int i, j;
	static int old;

	switch (uMsg) {
	case WM_INITDIALOG:
		SetWindowText(hDlg, message_get_res(IDS_TOOL_SELECT_TITLE));
		SetWindowText(GetDlgItem(hDlg, IDC_STATIC_MSG), message_get_res(IDS_TOOL_SELECT_MSG));

		// リストビューの設定
		i = 0;
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = Scale(200);
		lvc.pszText = message_get_res(IDS_TOOL_LIST_TITLE);
		lvc.iSubItem = i++;
		ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_HEADER), lvc.iSubItem, &lvc);

		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = Scale(80);
		lvc.pszText = message_get_res(IDS_TOOL_LIST_FUNC);
		lvc.iSubItem = i++;
		ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_HEADER), lvc.iSubItem, &lvc);

		// リストビューのスタイルの設定
		SetWindowLong(GetDlgItem(hDlg, IDC_LIST_HEADER), GWL_STYLE,
			GetWindowLong(GetDlgItem(hDlg, IDC_LIST_HEADER), GWL_STYLE) | LVS_SHOWSELALWAYS);
		SendDlgItemMessage(hDlg, IDC_LIST_HEADER, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
			LVS_EX_ONECLICKACTIVATE | LVS_EX_FULLROWSELECT |
			SendDlgItemMessage(hDlg, IDC_LIST_HEADER, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0));

		lvi.mask = LVIF_TEXT;
		lvi.iItem = ListView_GetItemCount(GetDlgItem(hDlg, IDC_LIST_HEADER));
		lvi.iSubItem = 0;
		lvi.pszText = message_get_res(IDS_ACTION_TOOL_ALL);
		lvi.cchTextMax = BUF_SIZE - 1;
		ListView_InsertItem(GetDlgItem(hDlg, IDC_LIST_HEADER), &lvi);

		for (i = 0; i < option.tool_cnt; i++) {
			if (!((option.tool_info + i)->call_type & CALLTYPE_MENU) ||
				lstrcmp((option.tool_info + i)->title, TEXT("-")) == 0) {
				continue;
			}
			lvi.mask = LVIF_TEXT;
			lvi.iItem = ListView_GetItemCount(GetDlgItem(hDlg, IDC_LIST_HEADER));
			lvi.iSubItem = 0;
			lvi.pszText = (option.tool_info + i)->title;
			lvi.cchTextMax = BUF_SIZE - 1;
			j = ListView_InsertItem(GetDlgItem(hDlg, IDC_LIST_HEADER), &lvi);
			ListView_SetItemText(GetDlgItem(hDlg, IDC_LIST_HEADER), j, 1, (option.tool_info + i)->func_name);
		}
		EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
		break;

	case WM_CLOSE:
		EndDialog(hDlg, FALSE);
		break;

	case WM_NOTIFY:
		listview_notify_proc(hDlg, lParam, GetDlgItem(hDlg, IDC_LIST_HEADER));
		break;

	case WM_LV_EVENT:
		switch (wParam) {
		case LVN_ITEMCHANGED:
			if (ListView_GetSelectedCount(GetDlgItem(hDlg, IDC_LIST_HEADER)) <= 0) {
				EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
			} else {
				EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
			}
			break;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			SendMessage(hDlg, WM_CLOSE, 0, 0);
			break;

		case IDC_BUTTON_EDIT:
		case IDOK:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_HEADER), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			if (i == 0) {
				SendDlgItemMessage(GetParent(hDlg), IDC_EDIT_PATH, WM_SETTEXT, 0, (LPARAM)TEXT(""));
			} else {
				ListView_GetItemText(GetDlgItem(hDlg, IDC_LIST_HEADER), i, 0, buf, BUF_SIZE - 1);
				SendDlgItemMessage(GetParent(hDlg), IDC_EDIT_PATH, WM_SETTEXT, 0, (LPARAM)buf);
			}
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
 * treeview_set_item - ツリービューアイテムの追加
 */
static HTREEITEM treeview_set_item(const HWND hTreeView, TCHAR *buf, const HTREEITEM hParent, const HTREEITEM After, LPARAM lParam)
{
	TV_INSERTSTRUCT tvitn;
	TV_ITEM tvit;

	tvit.mask = TVIF_TEXT | TVIF_PARAM;
	tvit.iImage = 0;
	tvit.iSelectedImage = 0;
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
 * treeview_set_text - アイテムのテキストを設定
 */
static BOOL treeview_set_text(const HWND hTreeView, const HTREEITEM hItem, TCHAR *text)
{
	TV_ITEM tvit;

	if (hItem == NULL) {
		return FALSE;
	}
	tvit.mask = TVIF_TEXT;
	tvit.hItem = hItem;
	tvit.pszText = text;
	tvit.cchTextMax = BUF_SIZE - 1;
	return TreeView_SetItem(hTreeView, &tvit);
}

/*
 * treeview_set_lparam - アイテムに情報を関連つける
 */
static BOOL treeview_set_lparam(const HWND hTreeView, const HTREEITEM hItem, const LPARAM lParam)
{
	TV_ITEM tvit;

	if (hItem == NULL) {
		return FALSE;
	}
	tvit.mask = TVIF_PARAM;
	tvit.hItem = hItem;
	tvit.lParam = (LPARAM)lParam;
	return TreeView_SetItem(hTreeView, &tvit);
}

/*
 * treeview_get_lparam - アイテムに関連付けられた情報の取得
 */
static LPARAM treeview_get_lparam(const HWND hTreeView, const HTREEITEM hItem)
{
	TV_ITEM tvit;

	if (hItem == NULL) {
		return 0;
	}
	tvit.mask = TVIF_PARAM;
	tvit.hItem = hItem;
	tvit.lParam = 0;
	TreeView_GetItem(hTreeView, &tvit);
	return tvit.lParam;
}

/*
 * treeview_copy_item - ツリビューアイテムのコピーを作成
 */
static HTREEITEM treeview_copy_item(const HWND hTreeView, const HTREEITEM parent_item, const HTREEITEM hItem, const HTREEITEM After)
{
	TV_INSERTSTRUCT tvitn;
	TV_ITEM tvit;
	HTREEITEM new_item;
	HTREEITEM cItem;
	TCHAR buf[BUF_SIZE];

	tvit.mask = TVIF_TEXT | TVIF_PARAM;
	tvit.hItem = hItem;
	tvit.cchTextMax = BUF_SIZE - 1;
	tvit.pszText = buf;
	tvit.iImage = 0;
	tvit.iSelectedImage = 0;
	if (TreeView_GetItem(hTreeView, &tvit) == FALSE) {
		return NULL;
	}
	tvit.hItem = NULL;
	tvit.cChildren = 0;

	tvitn.hInsertAfter = After;
	tvitn.hParent = (HTREEITEM)parent_item;
	tvitn.item = tvit;
	if ((new_item = TreeView_InsertItem(hTreeView, &tvitn)) == NULL) {
		return NULL;
	}

	cItem = TreeView_GetChild(hTreeView, hItem);
	while (cItem != NULL) {
		// 再帰
		treeview_copy_item(hTreeView, new_item, cItem, (HTREEITEM)TVI_LAST);
		cItem = TreeView_GetNextSibling(hTreeView, cItem);
	}
	TreeView_Expand(hTreeView, new_item, TVE_EXPAND);
	return new_item;
}

/*
 * treeview_notify_proc - ツリービューイベント
 */
static LRESULT treeview_notify_proc(const HWND hWnd, LPARAM lParam)
{
	NM_TREEVIEW *nmtv = (NM_TREEVIEW *)lParam;
	TV_KEYDOWN *tvk = (TV_KEYDOWN *)lParam;

	switch (nmtv->hdr.code) {
	case TVN_SELCHANGED:
	case NM_CUSTOMDRAW:
		return SendMessage(hWnd, WM_TV_EVENT, nmtv->hdr.code, lParam);
	}

	switch (tvk->hdr.code) {
	case TVN_KEYDOWN:			// キーダウン
		return SendMessage(hWnd, WM_TV_EVENT, tvk->hdr.code, lParam);
	}
	return FALSE;
}

/*
 * treeview_move_up - アイテムを上に移動
 */
static HTREEITEM treeview_move_up(const HWND hTreeView)
{
	HTREEITEM parent_item, prev_item, ret_item;
	HTREEITEM hItem = TreeView_GetSelection(hTreeView);
	MENU_INFO *mi;

	if (treeview_get_lparam(hTreeView, hItem) == 0) {
		return NULL;
	}

	if ((parent_item = TreeView_GetParent(hTreeView, hItem)) == NULL) {
		return NULL;
	}
	if ((prev_item = TreeView_GetPrevSibling(hTreeView, hItem)) == NULL) {
		if (parent_item == root_item) {
			return NULL;
		}
		// 上の階層に移動
		prev_item = parent_item;
		parent_item = TreeView_GetParent(hTreeView, parent_item);
		if ((prev_item = TreeView_GetPrevSibling(hTreeView, prev_item)) == NULL) {
			prev_item = (HTREEITEM)TVI_FIRST;
		}
	} else {
		if ((mi = (MENU_INFO *)treeview_get_lparam(hTreeView, prev_item)) == NULL) {
			return NULL;
		}
		if (mi->content == MENU_CONTENT_POPUP) {
			// 下の階層に移動
			parent_item = prev_item;
			prev_item = (HTREEITEM)TVI_LAST;
		} else {
			if ((prev_item = TreeView_GetPrevSibling(hTreeView, prev_item)) == NULL) {
				prev_item = (HTREEITEM)TVI_FIRST;
			}
		}
	}
	// アイテムのコピー
	if ((ret_item = treeview_copy_item(hTreeView, parent_item, hItem, prev_item)) == NULL) {
		return NULL;
	}
	TreeView_SelectItem(hTreeView, ret_item);
	TreeView_DeleteItem(hTreeView, hItem);
	return ret_item;
}

/*
 * treeview_move_down - アイテムを下に移動
 */
static HTREEITEM treeview_move_down(const HWND hTreeView)
{
	HTREEITEM parent_item, next_item, ret_item;
	HTREEITEM hItem = TreeView_GetSelection(hTreeView);
	MENU_INFO *mi;

	if (treeview_get_lparam(hTreeView, hItem) == 0) {
		return NULL;
	}

	if ((parent_item = TreeView_GetParent(hTreeView, hItem)) == NULL) {
		return NULL;
	}
	if ((next_item = TreeView_GetNextSibling(hTreeView, hItem)) == NULL) {
		if (parent_item == root_item) {
			return NULL;
		}
		// 上の階層に移動
		next_item = parent_item;
		parent_item = TreeView_GetParent(hTreeView, parent_item);
	} else {
		if ((mi = (MENU_INFO *)treeview_get_lparam(hTreeView, next_item)) == NULL) {
			return NULL;
		}
		if (mi->content == MENU_CONTENT_POPUP) {
			// 下の階層に移動
			parent_item = next_item;
			next_item = (HTREEITEM)TVI_FIRST;
		}
	}
	// アイテムのコピー
	if ((ret_item = treeview_copy_item(hTreeView, parent_item, hItem, next_item)) == NULL) {
		return NULL;
	}
	TreeView_SelectItem(hTreeView, ret_item);
	TreeView_DeleteItem(hTreeView, hItem);
	return ret_item;
}

/*
 * set_enable_control - コントロールの有効、無効を設定
 */
static void set_enable_control(const HWND hDlg)
{
	BOOL menu;
	BOOL enable;
	int i;

	i = SendDlgItemMessage(hDlg, IDC_COMBO_ACTION, CB_GETCURSEL, 0, 0);
	menu = (i == ACTION_POPUPMEMU) ? TRUE : FALSE;

	i = SendDlgItemMessage(hDlg, IDC_COMBO_TYPE, CB_GETCURSEL, 0, 0);
	EnableWindow(GetDlgItem(hDlg, IDC_HOTKEY), i == ACTION_TYPE_HOTKEY);

	EnableWindow(GetDlgItem(hDlg, IDC_CHECK_PASTE), menu);
	enable = menu &&
		(i == ACTION_TYPE_HOTKEY ||
		i == ACTION_TYPE_CTRL_CTRL ||
		i == ACTION_TYPE_SHIFT_SHIFT ||
		i == ACTION_TYPE_ALT_ALT);
	EnableWindow(GetDlgItem(hDlg, IDC_CHECK_CARET), enable);

	// メニュー
	EnableWindow(GetDlgItem(hDlg, IDC_TREE_MENU), menu);

	enable = menu && treeview_get_lparam(GetDlgItem(hDlg, IDC_TREE_MENU),
		TreeView_GetSelection(GetDlgItem(hDlg, IDC_TREE_MENU))) != 0;
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_UP), enable);
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_DOWN), enable);
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_DELETE), enable);

	enable = menu && TreeView_GetSelection(GetDlgItem(hDlg, IDC_TREE_MENU)) != root_item;
	EnableWindow(GetDlgItem(hDlg, IDC_COMBO_CONTENT), enable);

	i = SendDlgItemMessage(hDlg, IDC_COMBO_CONTENT, CB_GETCURSEL, 0, 0);

	enable = menu &&
		(i == MENU_CONTENT_POPUP ||
		i == MENU_CONTENT_VIEWER ||
		i == MENU_CONTENT_OPTION ||
		i == MENU_CONTENT_CLIPBOARD_WATCH ||
		i == MENU_CONTENT_TOOL ||
		i == MENU_CONTENT_APP ||
		i == MENU_CONTENT_CANCEL ||
		i == MENU_CONTENT_EXIT);
	EnableWindow(GetDlgItem(hDlg, IDC_EDIT_TITLE), enable);
	EnableWindow(GetDlgItem(hDlg, IDC_EDIT_ICON_FILE), enable);
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_ICON_FILE_SELECT), enable);
	EnableWindow(GetDlgItem(hDlg, IDC_EDIT_ICON_INDEX), enable);

	enable = menu &&
		(i == MENU_CONTENT_REGIST ||
		i == MENU_CONTENT_REGIST_DESC ||
		i == MENU_CONTENT_TOOL ||
		i == MENU_CONTENT_APP);
	EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PATH), enable);
	EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_PATH_SELECT), enable);
	if (i == MENU_CONTENT_TOOL) {
		SetWindowText(GetDlgItem(hDlg, IDC_STATIC_PATH), message_get_res(IDS_ACTION_TITLE_TOOL));
	} else {
		SetWindowText(GetDlgItem(hDlg, IDC_STATIC_PATH), message_get_res(IDS_ACTION_TITLE_PATH));
	}
	EnableWindow(GetDlgItem(hDlg, IDC_EDIT_CMD), menu && i == MENU_CONTENT_APP);

	enable = menu &&
		(i == MENU_CONTENT_HISTORY ||
		i == MENU_CONTENT_HISTORY_DESC);
	EnableWindow(GetDlgItem(hDlg, IDC_EDIT_MIN), enable);
	EnableWindow(GetDlgItem(hDlg, IDC_EDIT_MAX), enable);
}

/*
 * get_tree_text - ツリーに表示するテキストの取得
 */
static TCHAR *get_tree_text(const MENU_INFO *menu_info)
{
	TCHAR *ret;

	switch (menu_info->content) {
	case MENU_CONTENT_SEPARATOR:
		ret = message_get_res(IDS_ACTION_TREE_SEP);
		break;

	case MENU_CONTENT_HISTORY:
		ret = message_get_res(IDS_ACTION_CONTENT_HISTORY);
		break;

	case MENU_CONTENT_HISTORY_DESC:
		ret = message_get_res(IDS_ACTION_CONTENT_HISTORY_DESC);
		break;

	case MENU_CONTENT_REGIST:
		ret = message_get_res(IDS_ACTION_CONTENT_REGIST);
		break;

	case MENU_CONTENT_REGIST_DESC:
		ret = message_get_res(IDS_ACTION_CONTENT_REGIST_DESC);
		break;

	case MENU_CONTENT_POPUP:
		ret = (menu_info->title == NULL || *menu_info->title == TEXT('\0')) ?
			message_get_res(IDS_ACTION_CONTENT_POPUP) : menu_info->title;
		break;

	case MENU_CONTENT_VIEWER:
		ret = (menu_info->title == NULL || *menu_info->title == TEXT('\0')) ?
			message_get_res(IDS_MENU_VIEWER) : menu_info->title;
		break;

	case MENU_CONTENT_OPTION:
		ret = (menu_info->title == NULL || *menu_info->title == TEXT('\0')) ?
			message_get_res(IDS_MENU_OPTION) : menu_info->title;
		break;

	case MENU_CONTENT_CLIPBOARD_WATCH:
		ret = (menu_info->title == NULL || *menu_info->title == TEXT('\0')) ?
			message_get_res(IDS_MENU_CLIPBOARD_WATCH) : menu_info->title;
		break;

	case MENU_CONTENT_TOOL:
		if (menu_info->path != NULL && *menu_info->path != TEXT('\0')) {
			if (menu_info->title != NULL && *menu_info->title != TEXT('\0')) {
				ret = menu_info->title;
			} else {
				ret = menu_info->path;
			}
		} else {
			ret = message_get_res(IDS_ACTION_TOOL_ALL);
		}
		break;

	case MENU_CONTENT_APP:
		ret = (menu_info->title == NULL || *menu_info->title == TEXT('\0')) ?
			message_get_res(IDS_ACTION_CONTENT_APP) : menu_info->title;
		break;

	case MENU_CONTENT_CANCEL:
		ret = (menu_info->title == NULL || *menu_info->title == TEXT('\0')) ?
			message_get_res(IDS_MENU_CANCEL) : menu_info->title;
		break;

	case MENU_CONTENT_EXIT:
		ret = (menu_info->title == NULL || *menu_info->title == TEXT('\0')) ?
			message_get_res(IDS_MENU_EXIT) : menu_info->title;
		break;

	default:
		ret = menu_info->title;
		break;
	}
	return ret;
}

/*
 * set_menu_item - メニュー項目の設定
 */
static void set_menu_item(const HWND hDlg, const MENU_INFO *menu_info, const int menu_cnt, const HTREEITEM pItem)
{
	HTREEITEM hItem;
	MENU_INFO *mi;
	int i;

	for (i = 0; i < menu_cnt; i++) {
		// メニュー情報のコピー
		if ((mi = mem_calloc(sizeof(MENU_INFO))) == NULL) {
			return;
		}
		CopyMemory(mi, menu_info + i, sizeof(MENU_INFO));
		mi->title = alloc_copy((menu_info + i)->title);
		mi->icon_path = alloc_copy((menu_info + i)->icon_path);
		mi->path = alloc_copy((menu_info + i)->path);
		mi->cmd = alloc_copy((menu_info + i)->cmd);
		mi->mi = NULL;
		mi->mi_cnt = 0;

		hItem = treeview_set_item(GetDlgItem(hDlg, IDC_TREE_MENU), get_tree_text(mi), pItem, (HTREEITEM)TVI_LAST, (LPARAM)mi);
		if ((menu_info + i)->mi_cnt > 0) {
			set_menu_item(hDlg, (menu_info + i)->mi, (menu_info + i)->mi_cnt, hItem);
			TreeView_Expand(GetDlgItem(hDlg, IDC_TREE_MENU), hItem, TVE_EXPAND);
		}
	}
}

/*
 * get_menu_item - メニュー項目の取得
 */
static MENU_INFO *get_menu_item(const HWND hTreeView, const HTREEITEM pItem, int *cnt)
{
	HTREEITEM hItem;
	MENU_INFO *mi;
	MENU_INFO *new_mi;
	int i;

	// 項目数の取得
	hItem = TreeView_GetChild(hTreeView, pItem);
	*cnt = 0;
	while (hItem != NULL) {
		mi = (MENU_INFO *)treeview_get_lparam(hTreeView, hItem);
		if (mi != NULL) {
			(*cnt)++;
		}
		hItem = TreeView_GetNextSibling(hTreeView, hItem);
	}
	if (*cnt == 0) {
		return NULL;
	}

	// 項目の作成
	if ((new_mi = mem_calloc(sizeof(MENU_INFO) * *cnt)) == NULL) {
		*cnt = 0;
		return NULL;
	}
	hItem = TreeView_GetChild(hTreeView, pItem);
	i = 0;
	while (hItem != NULL) {
		if ((mi = (MENU_INFO *)treeview_get_lparam(hTreeView, hItem)) != NULL) {
			CopyMemory((new_mi + i), mi, sizeof(MENU_INFO));
			(new_mi + i)->title = alloc_copy(mi->title);
			(new_mi + i)->icon_path = alloc_copy(mi->icon_path);
			(new_mi + i)->path = alloc_copy(mi->path);
			(new_mi + i)->cmd = alloc_copy(mi->cmd);
			(new_mi + i)->mi_cnt = 0;
			// 再帰
			(new_mi + i)->mi = get_menu_item(hTreeView, hItem, &(new_mi + i)->mi_cnt);
			i++;
		}
		hItem = TreeView_GetNextSibling(hTreeView, hItem);
	}
	return new_mi;
}

/*
 * free_menu_item - メニュー項目の解放
 */
static void free_menu_item(const HWND hTreeView, const HTREEITEM pItem)
{
	HTREEITEM hItem;
	MENU_INFO *mi;

	if ((mi = (MENU_INFO *)treeview_get_lparam(hTreeView, pItem)) != NULL) {
		mem_free(&(mi->title));
		mem_free(&(mi->icon_path));
		mem_free(&(mi->path));
		mem_free(&(mi->cmd));
		mem_free(&mi);
		treeview_set_lparam(hTreeView, pItem, 0);
	}

	hItem = TreeView_GetChild(hTreeView, pItem);
	while (hItem != NULL) {
		// 再帰
		free_menu_item(hTreeView, hItem);
		hItem = TreeView_GetNextSibling(hTreeView, hItem);
	}
}

/*
 * set_menu_control - 項目表示
 */
static void set_menu_control(const HWND hDlg, const HTREEITEM se_item)
{
	HTREEITEM hItem = se_item;
	MENU_INFO *mi = NULL;

	if (hItem == NULL) {
		hItem = TreeView_GetSelection(GetDlgItem(hDlg, IDC_TREE_MENU));
	}
	if (hItem != NULL) {
		mi = (MENU_INFO *)treeview_get_lparam(GetDlgItem(hDlg, IDC_TREE_MENU), hItem);
	}
	if (hItem == NULL || hItem == root_item || mi == NULL) {
		SendDlgItemMessage(hDlg, IDC_COMBO_CONTENT, CB_SETCURSEL, -1, 0);
		SendDlgItemMessage(hDlg, IDC_EDIT_TITLE, WM_SETTEXT, 0, (LPARAM)TEXT(""));
		SendDlgItemMessage(hDlg, IDC_EDIT_PATH, WM_SETTEXT, 0, (LPARAM)TEXT(""));
		SendDlgItemMessage(hDlg, IDC_EDIT_CMD, WM_SETTEXT, 0, (LPARAM)TEXT(""));
		SendDlgItemMessage(hDlg, IDC_EDIT_ICON_FILE, WM_SETTEXT, 0, (LPARAM)TEXT(""));
		SetDlgItemInt(hDlg, IDC_EDIT_ICON_INDEX, 0, FALSE);
		SetDlgItemInt(hDlg, IDC_EDIT_MIN, 0, FALSE);
		SetDlgItemInt(hDlg, IDC_EDIT_MAX, 0, FALSE);
	} else {
		SendDlgItemMessage(hDlg, IDC_COMBO_CONTENT, CB_SETCURSEL, mi->content, 0);
		SendDlgItemMessage(hDlg, IDC_EDIT_TITLE, WM_SETTEXT, 0, (LPARAM)mi->title);
		SendDlgItemMessage(hDlg, IDC_EDIT_PATH, WM_SETTEXT, 0, (LPARAM)mi->path);
		SendDlgItemMessage(hDlg, IDC_EDIT_CMD, WM_SETTEXT, 0, (LPARAM)mi->cmd);
		SendDlgItemMessage(hDlg, IDC_EDIT_ICON_FILE, WM_SETTEXT, 0, (LPARAM)mi->icon_path);
		SetDlgItemInt(hDlg, IDC_EDIT_ICON_INDEX, mi->icon_index, FALSE);
		SetDlgItemInt(hDlg, IDC_EDIT_MIN, mi->min, FALSE);
		SetDlgItemInt(hDlg, IDC_EDIT_MAX, mi->max, FALSE);
	}
	set_enable_control(hDlg);
}

/*
 * get_menu_control - 項目取得
 */
static void get_menu_control(const HWND hDlg, const HTREEITEM hItem)
{
	MENU_INFO *mi = NULL;

	if (hItem == NULL || hItem == root_item || SendDlgItemMessage(hDlg, IDC_COMBO_CONTENT, CB_GETCURSEL, 0, 0) == -1) {
		return;
	}
	if ((mi = (MENU_INFO *)treeview_get_lparam(GetDlgItem(hDlg, IDC_TREE_MENU), hItem)) == NULL) {
		if ((mi = mem_calloc(sizeof(MENU_INFO))) == NULL) {
			return;
		}
		treeview_set_item(GetDlgItem(hDlg, IDC_TREE_MENU),
			message_get_res(IDS_ACTION_TREE_NEW), root_item, (HTREEITEM)TVI_LAST, 0);
	}
	mi->content = SendDlgItemMessage(hDlg, IDC_COMBO_CONTENT, CB_GETCURSEL, 0, 0);
	alloc_get_text(GetDlgItem(hDlg, IDC_EDIT_TITLE), &mi->title);
	alloc_get_text(GetDlgItem(hDlg, IDC_EDIT_PATH), &mi->path);
	alloc_get_text(GetDlgItem(hDlg, IDC_EDIT_CMD), &mi->cmd);
	alloc_get_text(GetDlgItem(hDlg, IDC_EDIT_ICON_FILE), &mi->icon_path);
	mi->icon_index = GetDlgItemInt(hDlg, IDC_EDIT_ICON_INDEX, NULL, FALSE);
	mi->min = GetDlgItemInt(hDlg, IDC_EDIT_MIN, NULL, FALSE);
	mi->max = GetDlgItemInt(hDlg, IDC_EDIT_MAX, NULL, FALSE);

	treeview_set_lparam(GetDlgItem(hDlg, IDC_TREE_MENU), hItem, (LPARAM)mi);
	treeview_set_text(GetDlgItem(hDlg, IDC_TREE_MENU), hItem, get_tree_text(mi));
}

/*
 * set_app_title - 外部アプリケーションのタイトルを設定
 */
static void set_app_title(const HWND hDlg, TCHAR *buf)
{
	TCHAR tmp[BUF_SIZE];
	TCHAR *p, *r;

	*tmp = TEXT('\0');
	SendDlgItemMessage(hDlg, IDC_EDIT_TITLE, WM_GETTEXT, BUF_SIZE - 1, (LPARAM)tmp);
	if (*tmp == TEXT('\0')) {
		// タイトルの設定
		r = TEXT("");
		for (p = buf; *p != TEXT('\0'); p++) {
#ifndef UNICODE
			if (IsDBCSLeadByte((BYTE)*p) == TRUE) {
				p++;
				continue;
			}
#endif	// UNICODE
			if (*p == TEXT('\\') || *p == TEXT('/')) {
				r = p + 1;
			}
		}
		SendDlgItemMessage(hDlg, IDC_EDIT_TITLE, WM_SETTEXT, 0, (LPARAM)r);
	}
}

/*
 * set_action_item_proc - Actionの項目を設定
 */
static BOOL CALLBACK set_action_item_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HTREEITEM cItem;
	ACTION_INFO *ai;
	TCHAR buf[BUF_SIZE];
	int cnt;
	int i;
#ifdef OP_XP_STYLE
	static long hThemeUp, hThemeDown;
#endif	// OP_XP_STYLE

	switch (uMsg) {
	case WM_INITDIALOG:
		// D&Dを受け付ける
		SetWindowLong(hDlg, GWL_EXSTYLE, GetWindowLong(hDlg, GWL_EXSTYLE) | WS_EX_ACCEPTFILES);
#ifdef OP_XP_STYLE
		// XP
		hThemeUp = open_theme(GetDlgItem(hDlg, IDC_BUTTON_UP), L"SCROLLBAR");
		hThemeDown = open_theme(GetDlgItem(hDlg, IDC_BUTTON_DOWN), L"SCROLLBAR");
#endif	// OP_XP_STYLE
		// スピンコントロールの設定
		SendDlgItemMessage(hDlg, IDC_SPIN_ICON_INDEX, UDM_SETRANGE, 0, (LPARAM)MAKELONG(UD_MAXVAL, 0));
		SendDlgItemMessage(hDlg, IDC_SPIN_MIN, UDM_SETRANGE, 0, (LPARAM)MAKELONG(UD_MAXVAL, 0));
		SendDlgItemMessage(hDlg, IDC_SPIN_MAX, UDM_SETRANGE, 0, (LPARAM)MAKELONG(UD_MAXVAL, 0));

#define SET_COMBO_ITEM(id, str)				SendDlgItemMessage(hDlg, id, CB_ADDSTRING, 0, (LPARAM)str);
		SET_COMBO_ITEM(IDC_COMBO_ACTION, message_get_res(IDS_ACTION_POPUPMEMU));
		SET_COMBO_ITEM(IDC_COMBO_ACTION, message_get_res(IDS_ACTION_VIEWER));
		SET_COMBO_ITEM(IDC_COMBO_ACTION, message_get_res(IDS_ACTION_OPTION));
		SET_COMBO_ITEM(IDC_COMBO_ACTION, message_get_res(IDS_ACTION_CLIPBOARD_WATCH));
		SET_COMBO_ITEM(IDC_COMBO_ACTION, message_get_res(IDS_ACTION_EXIT));

		SET_COMBO_ITEM(IDC_COMBO_TYPE, message_get_res(IDS_ACTION_TYPE_HOTKEY));
		SET_COMBO_ITEM(IDC_COMBO_TYPE, message_get_res(IDS_ACTION_TYPE_CTRL_CTRL));
		SET_COMBO_ITEM(IDC_COMBO_TYPE, message_get_res(IDS_ACTION_TYPE_SHIFT_SHIFT));
		SET_COMBO_ITEM(IDC_COMBO_TYPE, message_get_res(IDS_ACTION_TYPE_ALT_ALT));
		SET_COMBO_ITEM(IDC_COMBO_TYPE, message_get_res(IDS_ACTION_TYPE_TRAY_LEFT));
		SET_COMBO_ITEM(IDC_COMBO_TYPE, message_get_res(IDS_ACTION_TYPE_TRAY_LEFT_DBLCLK));
		SET_COMBO_ITEM(IDC_COMBO_TYPE, message_get_res(IDS_ACTION_TYPE_TRAY_RIGHT));
		SET_COMBO_ITEM(IDC_COMBO_TYPE, message_get_res(IDS_ACTION_TYPE_TRAY_RIGHT_DBLCLK));

		SET_COMBO_ITEM(IDC_COMBO_CONTENT, message_get_res(IDS_ACTION_CONTENT_SEPARATOR));
		SET_COMBO_ITEM(IDC_COMBO_CONTENT, message_get_res(IDS_ACTION_CONTENT_HISTORY));
		SET_COMBO_ITEM(IDC_COMBO_CONTENT, message_get_res(IDS_ACTION_CONTENT_HISTORY_DESC));
		SET_COMBO_ITEM(IDC_COMBO_CONTENT, message_get_res(IDS_ACTION_CONTENT_REGIST));
		SET_COMBO_ITEM(IDC_COMBO_CONTENT, message_get_res(IDS_ACTION_CONTENT_REGIST_DESC));
		SET_COMBO_ITEM(IDC_COMBO_CONTENT, message_get_res(IDS_ACTION_CONTENT_POPUP));
		SET_COMBO_ITEM(IDC_COMBO_CONTENT, message_get_res(IDS_ACTION_CONTENT_VIEWER));
		SET_COMBO_ITEM(IDC_COMBO_CONTENT, message_get_res(IDS_ACTION_CONTENT_OPTION));
		SET_COMBO_ITEM(IDC_COMBO_CONTENT, message_get_res(IDS_ACTION_CONTENT_CLIPBOARD_WATCH));
		SET_COMBO_ITEM(IDC_COMBO_CONTENT, message_get_res(IDS_ACTION_CONTENT_TOOL));
		SET_COMBO_ITEM(IDC_COMBO_CONTENT, message_get_res(IDS_ACTION_CONTENT_APP));
		SET_COMBO_ITEM(IDC_COMBO_CONTENT, message_get_res(IDS_ACTION_CONTENT_CANCEL));
		SET_COMBO_ITEM(IDC_COMBO_CONTENT, message_get_res(IDS_ACTION_CONTENT_EXIT));

		if (lParam == 0) {
			// 新規追加
			CheckDlgButton(hDlg, IDC_CHECK_ENABLE, 1);
			CheckDlgButton(hDlg, IDC_CHECK_PASTE, 1);
			CheckDlgButton(hDlg, IDC_CHECK_CARET, 1);

			root_item = treeview_set_item(GetDlgItem(hDlg, IDC_TREE_MENU),
				message_get_res(IDS_ACTION_TREE_MENU), (HTREEITEM)TVI_ROOT, (HTREEITEM)TVI_LAST, 0);
			treeview_set_item(GetDlgItem(hDlg, IDC_TREE_MENU),
				message_get_res(IDS_ACTION_TREE_NEW), root_item, (HTREEITEM)TVI_LAST, 0);
			TreeView_Expand(GetDlgItem(hDlg, IDC_TREE_MENU), root_item, TVE_EXPAND);
			TreeView_SelectItem(GetDlgItem(hDlg, IDC_TREE_MENU), root_item);

			SetWindowLong(hDlg, GWL_USERDATA, 0);
			set_enable_control(hDlg);
			break;
		}
		ai = (ACTION_INFO *)lParam;
		SendDlgItemMessage(hDlg, IDC_COMBO_ACTION, CB_SETCURSEL, ai->action, 0);
		SendDlgItemMessage(hDlg, IDC_COMBO_TYPE, CB_SETCURSEL, ai->type, 0);
		CheckDlgButton(hDlg, IDC_CHECK_ENABLE, ai->enable);
		CheckDlgButton(hDlg, IDC_CHECK_PASTE, ai->paste);
		CheckDlgButton(hDlg, IDC_CHECK_CARET, ai->caret);

		// ホットキー
		i = 0;
		if (ai->modifiers & MOD_SHIFT) {
			i |= HOTKEYF_SHIFT;
		}
		if (ai->modifiers & MOD_CONTROL) {
			i |= HOTKEYF_CONTROL;
		}
		if (ai->modifiers & MOD_ALT) {
			i |= HOTKEYF_ALT;
		}
		if (ai->modifiers & MOD_WIN) {
			i |= HOTKEYF_WIN;
		}
		if (ai->virtkey != 0) {
			SendDlgItemMessage(hDlg, IDC_HOTKEY, HKM_SETHOTKEY, (WPARAM)MAKEWORD(ai->virtkey, i), 0);
		}

		// メニュー項目の設定
		root_item = treeview_set_item(GetDlgItem(hDlg, IDC_TREE_MENU),
			message_get_res(IDS_ACTION_TREE_MENU), (HTREEITEM)TVI_ROOT, (HTREEITEM)TVI_LAST, 0);

		set_menu_item(hDlg, ai->menu_info, ai->menu_cnt, root_item);

		treeview_set_item(GetDlgItem(hDlg, IDC_TREE_MENU),
			message_get_res(IDS_ACTION_TREE_NEW), root_item, (HTREEITEM)TVI_LAST, 0);
		TreeView_Expand(GetDlgItem(hDlg, IDC_TREE_MENU), root_item, TVE_EXPAND);
		TreeView_SelectItem(GetDlgItem(hDlg, IDC_TREE_MENU), root_item);

		set_enable_control(hDlg);
		SetWindowLong(hDlg, GWL_USERDATA, lParam);
		break;

	case WM_CLOSE:
		free_menu_item(GetDlgItem(hDlg, IDC_TREE_MENU), root_item);
#ifdef OP_XP_STYLE
		if (hThemeUp != 0 && hThemeDown != 0) {
			close_theme(hThemeUp);
			close_theme(hThemeDown);
		}
#endif	// OP_XP_STYLE
		EndDialog(hDlg, FALSE);
		break;

	case WM_DROPFILES:
		if (SendDlgItemMessage(hDlg, IDC_COMBO_ACTION, CB_GETCURSEL, 0, 0) == -1) {
			SendDlgItemMessage(hDlg, IDC_COMBO_ACTION, CB_SETCURSEL, ACTION_POPUPMEMU, 0);
			set_enable_control(hDlg);
		} else if (SendDlgItemMessage(hDlg, IDC_COMBO_ACTION, CB_GETCURSEL, 0, 0) != ACTION_POPUPMEMU) {
			DragFinish((HANDLE)wParam);
			break;
		}

		cnt = DragQueryFile((HANDLE)wParam, 0xFFFFFFFF, NULL, 0);
		for (i = 0; i < cnt; i++) {
			if ((cItem = TreeView_GetChild(GetDlgItem(hDlg, IDC_TREE_MENU), root_item)) == NULL) {
				break;
			}
			while (TreeView_GetNextSibling(GetDlgItem(hDlg, IDC_TREE_MENU), cItem) != NULL) {
				cItem = TreeView_GetNextSibling(GetDlgItem(hDlg, IDC_TREE_MENU), cItem);
			}
			TreeView_SelectItem(GetDlgItem(hDlg, IDC_TREE_MENU), cItem);
			SendDlgItemMessage(hDlg, IDC_COMBO_CONTENT, CB_SETCURSEL, MENU_CONTENT_APP, 0);
			get_menu_control(hDlg, cItem);
			set_enable_control(hDlg);
			DragQueryFile((HANDLE)wParam, i, buf, BUF_SIZE - 1);

			SendDlgItemMessage(hDlg, IDC_EDIT_PATH, WM_SETTEXT, 0, (LPARAM)buf);
			set_app_title(hDlg, buf);
		}
		DragFinish((HANDLE)wParam);
		SetFocus(GetDlgItem(hDlg, IDC_EDIT_PATH));
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_COMBO_ACTION:
		case IDC_COMBO_TYPE:
			if (HIWORD(wParam) == CBN_SELENDOK) {
				set_enable_control(hDlg);
			}
			break;

		case IDC_COMBO_CONTENT:
			if (HIWORD(wParam) == CBN_SELENDOK) {
				get_menu_control(hDlg, TreeView_GetSelection(GetDlgItem(hDlg, IDC_TREE_MENU)));
				set_enable_control(hDlg);
			}
			break;

		case IDC_EDIT_TITLE:
		case IDC_EDIT_PATH:
		case IDC_EDIT_CMD:
		case IDC_EDIT_ICON_FILE:
		case IDC_EDIT_ICON_INDEX:
		case IDC_EDIT_MIN:
		case IDC_EDIT_MAX:
			if (HIWORD(wParam) == EN_KILLFOCUS) {
				get_menu_control(hDlg, TreeView_GetSelection(GetDlgItem(hDlg, IDC_TREE_MENU)));
			}
			break;

		case IDC_BUTTON_UP:
			treeview_move_up(GetDlgItem(hDlg, IDC_TREE_MENU));
			break;

		case IDC_BUTTON_DOWN:
			treeview_move_down(GetDlgItem(hDlg, IDC_TREE_MENU));
			break;

		case IDC_BUTTON_DELETE:
			// 項目削除
			free_menu_item(GetDlgItem(hDlg, IDC_TREE_MENU), TreeView_GetSelection(GetDlgItem(hDlg, IDC_TREE_MENU)));
			TreeView_DeleteItem(GetDlgItem(hDlg, IDC_TREE_MENU), TreeView_GetSelection(GetDlgItem(hDlg, IDC_TREE_MENU)));
			break;

		case IDC_BUTTON_PATH_SELECT:
			if (SendDlgItemMessage(hDlg, IDC_COMBO_CONTENT, CB_GETCURSEL, 0, 0) == MENU_CONTENT_REGIST) {
				HWND CLCLWnd;
				TCHAR err_str[BUF_SIZE];

				// メインウィンドウ検索
				if ((CLCLWnd = FindWindow(MAIN_WND_CLASS, MAIN_WINDOW_TITLE)) != NULL) {
					// 登録アイテム保存要求
					SendMessage(CLCLWnd, WM_REGIST_SAVE, 0, 0);
				}
				wsprintf(buf, TEXT("%s\\%s"), work_path, REGIST_FILENAME);
				*err_str = TEXT('\0');
				if (file_read_data(buf, &regist_data.child, err_str) == FALSE && *err_str != TEXT('\0')) {
					MessageBox(hDlg, err_str, REGIST_FILENAME, MB_ICONERROR);
					break;
				}
				// フォルダの選択
				*buf = TEXT('\0');
				SendDlgItemMessage(hDlg, IDC_EDIT_PATH, WM_GETTEXT, BUF_SIZE - 1, (LPARAM)buf);
				if (select_path(hInst, hDlg, regist_data.child, buf) == TRUE) {
					SendDlgItemMessage(hDlg, IDC_EDIT_PATH, WM_SETTEXT, 0, (LPARAM)buf);
				}
				data_free(regist_data.child);
				regist_data.child = NULL;

			} else if (SendDlgItemMessage(hDlg, IDC_COMBO_CONTENT, CB_GETCURSEL, 0, 0) == MENU_CONTENT_TOOL) {
				if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_SELECT_FUNC), hDlg, select_tool_proc, 0) == TRUE) {
					get_menu_control(hDlg, TreeView_GetSelection(GetDlgItem(hDlg, IDC_TREE_MENU)));
				}

			} else if (SendDlgItemMessage(hDlg, IDC_COMBO_CONTENT, CB_GETCURSEL, 0, 0) == MENU_CONTENT_APP) {
				SetFocus(GetDlgItem(hDlg, IDC_EDIT_PATH));
				// ファイル選択
				if (file_select(hDlg, TEXT("*.*\0*.*\0\0"), 1, buf) == -1) {
					break;
				}
				SendDlgItemMessage(hDlg, IDC_EDIT_PATH, WM_SETTEXT, 0, (LPARAM)buf);
				set_app_title(hDlg, buf);
			}
			break;

		case IDC_BUTTON_ICON_FILE_SELECT:
			*buf = TEXT('\0');
			SendDlgItemMessage(hDlg, IDC_EDIT_ICON_FILE, WM_GETTEXT, BUF_SIZE - 1, (LPARAM)buf);
			i = GetDlgItemInt(hDlg, IDC_EDIT_ICON_INDEX, NULL, FALSE);
			SetFocus(GetDlgItem(hDlg, IDC_EDIT_ICON_FILE));
			// アイコン選択
			if ((i = select_icon(hInst, hDlg, buf, i)) == -1) {
				break;
			}
			SendDlgItemMessage(hDlg, IDC_EDIT_ICON_FILE, WM_SETTEXT, 0, (LPARAM)buf);
			SetDlgItemInt(hDlg, IDC_EDIT_ICON_INDEX, i, FALSE);
			break;

		case IDOK:
			if (SendDlgItemMessage(hDlg, IDC_COMBO_ACTION, CB_GETCURSEL, 0, 0) == -1) {
				MessageBox(hDlg, message_get_res(IDS_ACTION_ERR_ACTION), WINDOW_TITLE, MB_ICONEXCLAMATION);
				SetFocus(GetDlgItem(hDlg, IDC_COMBO_ACTION));
				break;
			}
			if (SendDlgItemMessage(hDlg, IDC_COMBO_TYPE, CB_GETCURSEL, 0, 0) == -1) {
				MessageBox(hDlg, message_get_res(IDS_ACTION_ERR_TYPE), WINDOW_TITLE, MB_ICONEXCLAMATION);
				SetFocus(GetDlgItem(hDlg, IDC_COMBO_TYPE));
				break;
			}

			TreeView_SelectItem(GetDlgItem(hDlg, IDC_TREE_MENU), NULL);

			if ((ai = (ACTION_INFO *)GetWindowLong(hDlg, GWL_USERDATA)) == NULL) {
				ai = mem_calloc(sizeof(ACTION_INFO));
			}
			if (ai != NULL) {
				// 設定取得
				ai->action = SendDlgItemMessage(hDlg, IDC_COMBO_ACTION, CB_GETCURSEL, 0, 0);
				ai->type = SendDlgItemMessage(hDlg, IDC_COMBO_TYPE, CB_GETCURSEL, 0, 0);
				ai->enable = IsDlgButtonChecked(hDlg, IDC_CHECK_ENABLE);
				ai->paste = IsDlgButtonChecked(hDlg, IDC_CHECK_PASTE);
				ai->caret = IsDlgButtonChecked(hDlg, IDC_CHECK_CARET);

				i = SendDlgItemMessage(hDlg,IDC_HOTKEY, HKM_GETHOTKEY, 0, 0);
				ai->virtkey = LOBYTE(i);
				i = HIBYTE(i);
				ai->modifiers = ((i & HOTKEYF_SHIFT) ? MOD_SHIFT : 0) |
					((i & HOTKEYF_CONTROL) ? MOD_CONTROL : 0) |
					((i & HOTKEYF_ALT) ? MOD_ALT : 0) |
					((i & HOTKEYF_WIN) ? MOD_WIN : 0);

				ini_free_menu(ai->menu_info, ai->menu_cnt);
				ai->menu_info = get_menu_item(GetDlgItem(hDlg, IDC_TREE_MENU), root_item, &ai->menu_cnt);
			}

			if (GetWindowLong(hDlg, GWL_USERDATA) == 0) {
				// 新規
				HWND pWnd = PropSheet_GetCurrentPageHwnd(GetParent(hDlg));
				listview_set_action(GetDlgItem(pWnd, IDC_LIST_ACTION), ai, FALSE);
			}
			free_menu_item(GetDlgItem(hDlg, IDC_TREE_MENU), root_item);
#ifdef OP_XP_STYLE
			if (hThemeUp != 0 && hThemeDown != 0) {
				close_theme(hThemeUp);
				close_theme(hThemeDown);
			}
#endif	// OP_XP_STYLE
			EndDialog(hDlg, TRUE);
			break;

		case IDCANCEL:
			free_menu_item(GetDlgItem(hDlg, IDC_TREE_MENU), root_item);
#ifdef OP_XP_STYLE
			if (hThemeUp != 0 && hThemeDown != 0) {
				close_theme(hThemeUp);
				close_theme(hThemeDown);
			}
#endif	// OP_XP_STYLE
			EndDialog(hDlg, FALSE);
			break;
		}
		break;

	case WM_NOTIFY:
		// コントロール通知メッセージ
		if (((NMHDR *)lParam)->hwndFrom == GetDlgItem(hDlg, IDC_TREE_MENU)) {
			return treeview_notify_proc(hDlg, lParam);
		}
		break;

	case WM_DRAWITEM:
		// 描画するフレームコントロールスタイルを設定
		switch ((UINT)wParam) {
		case IDC_BUTTON_UP:
			i = DFCS_SCROLLUP;
			break;

		case IDC_BUTTON_DOWN:
			i = DFCS_SCROLLDOWN;
			break;

		default:
			return FALSE;
		}
		// ボタンの描画
#ifdef OP_XP_STYLE
		if (hThemeUp != 0 && hThemeDown != 0) {
			draw_theme_scroll((LPDRAWITEMSTRUCT)lParam, i, (i == DFCS_SCROLLUP) ? hThemeUp : hThemeDown);
		} else {
			draw_scroll_sontrol((LPDRAWITEMSTRUCT)lParam, i);
		}
#else	// OP_XP_STYLE
		draw_scroll_sontrol((LPDRAWITEMSTRUCT)lParam, i);
#endif	// OP_XP_STYLE
		break;

#ifdef OP_XP_STYLE
	case WM_THEMECHANGED:
		// テーマの変更
		if (hThemeUp != 0 && hThemeDown != 0) {
			close_theme(hThemeUp);
			close_theme(hThemeDown);
		}
		hThemeUp = open_theme(GetDlgItem(hDlg, IDC_BUTTON_UP), L"SCROLLBAR");
		hThemeDown = open_theme(GetDlgItem(hDlg, IDC_BUTTON_DOWN), L"SCROLLBAR");
		break;
#endif	// OP_XP_STYLE

	case WM_TV_EVENT:
		switch (wParam) {
		case TVN_SELCHANGED:
			get_menu_control(hDlg, ((NM_TREEVIEW *)lParam)->itemOld.hItem);
			set_menu_control(hDlg, ((NM_TREEVIEW *)lParam)->itemNew.hItem);
			return TRUE;

		case NM_CUSTOMDRAW:
			// カスタムドロー
			switch (((LPNMTVCUSTOMDRAW)lParam)->nmcd.dwDrawStage) {
			case CDDS_PREPAINT:
				SetWindowLong(hDlg, DWL_MSGRESULT, CDRF_NOTIFYITEMDRAW);
				return TRUE;

			case CDDS_ITEMPREPAINT:
				if (((LPNMTVCUSTOMDRAW)lParam)->nmcd.lItemlParam == 0) {
					if (((LPNMTVCUSTOMDRAW)lParam)->nmcd.uItemState == (CDIS_FOCUS | CDIS_SELECTED)) {
						((LPNMTVCUSTOMDRAW)lParam)->clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
					} else {   
						((LPNMTVCUSTOMDRAW)lParam)->clrText = GetSysColor(COLOR_GRAYTEXT);
					}
				}
				SetWindowLong(hDlg, DWL_MSGRESULT, CDRF_DODEFAULT);
				return TRUE;
			}
			break;
		}
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

/*
 * copy_menu_info - メニュー情報のコピーを作成
 */
static MENU_INFO *copy_menu_info(const MENU_INFO *menu_info, const int menu_cnt)
{
	MENU_INFO *mi;
	int i;

	if ((mi = mem_calloc(sizeof(MENU_INFO) * menu_cnt)) == NULL) {
		return NULL;
	}
	CopyMemory(mi, menu_info, sizeof(MENU_INFO) * menu_cnt);

	for (i = 0; i < menu_cnt; i++) {
		// メニュー情報のコピー
		(mi + i)->title = alloc_copy((menu_info + i)->title);
		(mi + i)->icon_path = alloc_copy((menu_info + i)->icon_path);
		(mi + i)->path = alloc_copy((menu_info + i)->path);
		(mi + i)->cmd = alloc_copy((menu_info + i)->cmd);
		(mi + i)->mi = NULL;
		(mi + i)->mi_cnt = 0;

		if ((menu_info + i)->mi_cnt > 0 &&
			((mi + i)->mi = copy_menu_info((menu_info + i)->mi, (menu_info + i)->mi_cnt)) != NULL) {
			(mi + i)->mi_cnt = (menu_info + i)->mi_cnt;
		}
	}
	return mi;
}

/*
 * copy_action_info - 動作情報のコピーを作成
 */
static ACTION_INFO *copy_action_info(const ACTION_INFO *ai)
{
	ACTION_INFO *new_ai;

	if ((new_ai = mem_calloc(sizeof(ACTION_INFO))) == NULL) {
		return NULL;
	}
	CopyMemory(new_ai, ai, sizeof(ACTION_INFO));
	new_ai->menu_info = copy_menu_info(ai->menu_info, ai->menu_cnt);
	new_ai->menu_cnt = (new_ai->menu_info != NULL) ? ai->menu_cnt : 0;
	return new_ai;
}

/*
 * listview_set_text - ListViewのテキストを設定
 */
static void listview_set_text(const HWND hListView, const int i)
{
	ACTION_INFO *ai;
	TCHAR buf[BUF_SIZE];
	TCHAR *p;

	if ((ai = (ACTION_INFO *)listview_get_lparam(hListView, i)) == NULL) {
		return;
	}
	// 動作
	switch (ai->action) {
	case ACTION_POPUPMEMU:
		p = message_get_res(IDS_ACTION_POPUPMEMU);
		break;

	case ACTION_VIEWER:
		p = message_get_res(IDS_ACTION_VIEWER);
		break;

	case ACTION_OPTION:
		p = message_get_res(IDS_ACTION_OPTION);
		break;

	case ACTION_CLIPBOARD_WATCH:
		p = message_get_res(IDS_ACTION_CLIPBOARD_WATCH);
		break;

	case ACTION_EXIT:
		p = message_get_res(IDS_ACTION_EXIT);
		break;
	}
	ListView_SetItemText(hListView, i, 0, p);

	// 呼び出し方法
	switch (ai->type) {
	case ACTION_TYPE_HOTKEY:
		get_keyname(ai->modifiers, ai->virtkey, buf);
		p = buf;
		break;

	case ACTION_TYPE_CTRL_CTRL:
		p = message_get_res(IDS_ACTION_TYPE_CTRL_CTRL);
		break;

	case ACTION_TYPE_SHIFT_SHIFT:
		p = message_get_res(IDS_ACTION_TYPE_SHIFT_SHIFT);
		break;

	case ACTION_TYPE_ALT_ALT:
		p = message_get_res(IDS_ACTION_TYPE_ALT_ALT);
		break;

	case ACTION_TYPE_TRAY_LEFT:
		p = message_get_res(IDS_ACTION_TYPE_TRAY_LEFT);
		break;

	case ACTION_TYPE_TRAY_LEFT_DBLCLK:
		p = message_get_res(IDS_ACTION_TYPE_TRAY_LEFT_DBLCLK);
		break;

	case ACTION_TYPE_TRAY_RIGHT:
		p = message_get_res(IDS_ACTION_TYPE_TRAY_RIGHT);
		break;

	case ACTION_TYPE_TRAY_RIGHT_DBLCLK:
		p = message_get_res(IDS_ACTION_TYPE_TRAY_RIGHT_DBLCLK);
		break;
	}
	ListView_SetItemText(hListView, i, 1, p);
}

/*
 * listview_set_action - ListViewにAction情報を追加する
 */
static void listview_set_action(const HWND hListView, ACTION_INFO *ai, const BOOL copy)
{
	LV_ITEM lvi;
	ACTION_INFO *new_ai;
	int i;

	if (copy == TRUE) {
		if ((new_ai = copy_action_info(ai)) == NULL) {
			return;
		}
	} else {
		new_ai = ai;
	}

	lvi.mask = LVIF_TEXT | LVIF_PARAM;
	lvi.iItem = ListView_GetItemCount(hListView);
	lvi.iSubItem = 0;
	lvi.pszText = TEXT("");
	lvi.cchTextMax = BUF_SIZE - 1;
	lvi.iImage = 0;
	lvi.lParam = (LPARAM)new_ai;
	i = ListView_InsertItem(hListView, &lvi);

	listview_set_text(hListView, i);
}

/*
 * listview_get_action - Action情報の取得
 */
static ACTION_INFO *listview_get_action(const HWND hListView, int *cnt)
{
	ACTION_INFO *ai;
	ACTION_INFO *new_ai;
	int i;

	if ((*cnt = ListView_GetItemCount(hListView)) == 0) {
		return NULL;
	}

	// 項目の作成
	if ((new_ai = mem_calloc(sizeof(ACTION_INFO) * *cnt)) == NULL) {
		*cnt = 0;
		return NULL;
	}

	for (i = 0; i < *cnt; i++) {
		if ((ai = (ACTION_INFO *)listview_get_lparam(hListView, i)) == NULL) {
			continue;
		}
		CopyMemory(new_ai + i, ai, sizeof(ACTION_INFO));
		(new_ai + i)->menu_info = copy_menu_info(ai->menu_info, ai->menu_cnt);
		(new_ai + i)->menu_cnt = ((new_ai + i)->menu_info != NULL) ? ai->menu_cnt : 0;
	}
	return new_ai;
}

/*
 * listview_free_action - Action情報の解放
 */
static void listview_free_action(const HWND hListView)
{
	ACTION_INFO *ai;
	int i;

	for (i = 0; i < ListView_GetItemCount(hListView); i++) {
		if ((ai = (ACTION_INFO *)listview_get_lparam(hListView, i)) != NULL) {
			ini_free_menu(ai->menu_info, ai->menu_cnt);
			mem_free(&ai);
		}
		listview_set_lparam(hListView, i, 0);
	}
}

/*
 * set_action_proc - Action設定のプロシージャ
 */
BOOL CALLBACK set_action_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LV_COLUMN lvc;
	ACTION_INFO *ai, *new_ai;
	int i;
	BOOL enable;
#ifdef OP_XP_STYLE
	static long hThemeUp, hThemeDown;
#endif	// OP_XP_STYLE

	switch (uMsg) {
	case WM_INITDIALOG:
#ifdef OP_XP_STYLE
		// XP
		hThemeUp = open_theme(GetDlgItem(hDlg, IDC_BUTTON_UP), L"SCROLLBAR");
		hThemeDown = open_theme(GetDlgItem(hDlg, IDC_BUTTON_DOWN), L"SCROLLBAR");
#endif	// OP_XP_STYLE
		// リストビューのカラムの設定
		i = 0;
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = Scale(130);
		lvc.pszText = message_get_res(IDS_ACTION_LIST_HEAD_ACTION);
		lvc.iSubItem = i++;
		ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_ACTION), lvc.iSubItem, &lvc);

		lvc.fmt = LVCFMT_LEFT;
		lvc.cx = Scale(200);
		lvc.pszText = message_get_res(IDS_ACTION_LIST_HEAD_TYPE);
		lvc.iSubItem = i++;
		ListView_InsertColumn(GetDlgItem(hDlg, IDC_LIST_ACTION), lvc.iSubItem, &lvc);

		// リストビューのスタイルの設定
		SetWindowLong(GetDlgItem(hDlg, IDC_LIST_ACTION), GWL_STYLE,
			GetWindowLong(GetDlgItem(hDlg, IDC_LIST_ACTION), GWL_STYLE) | LVS_SHOWSELALWAYS);
		SendDlgItemMessage(hDlg, IDC_LIST_ACTION, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
			LVS_EX_ONECLICKACTIVATE | LVS_EX_FULLROWSELECT |
			SendDlgItemMessage(hDlg, IDC_LIST_ACTION, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0));

		for (i = 0; i < option.action_cnt; i++) {
			listview_set_action(GetDlgItem(hDlg, IDC_LIST_ACTION), option.action_info + i, TRUE);
		}
		SendMessage(hDlg, WM_LV_EVENT, LVN_ITEMCHANGED, 0);
		break;

	case WM_DESTROY:
		listview_free_action(GetDlgItem(hDlg, IDC_LIST_ACTION));
#ifdef OP_XP_STYLE
		// XP
		if (hThemeUp != 0 && hThemeDown != 0) {
			close_theme(hThemeUp);
			close_theme(hThemeDown);
		}
#endif	// OP_XP_STYLE
		break;

	case WM_DRAWITEM:
		switch ((UINT)wParam) {
		case IDC_BUTTON_UP:
			i = DFCS_SCROLLUP;
			break;

		case IDC_BUTTON_DOWN:
			i = DFCS_SCROLLDOWN;
			break;

		default:
			return FALSE;
		}
		// ボタンの描画
#ifdef OP_XP_STYLE
		if (hThemeUp != 0 && hThemeDown != 0) {
			draw_theme_scroll((LPDRAWITEMSTRUCT)lParam, i, (i == DFCS_SCROLLUP) ? hThemeUp : hThemeDown);
		} else {
			draw_scroll_sontrol((LPDRAWITEMSTRUCT)lParam, i);
		}
#else	// OP_XP_STYLE
		draw_scroll_sontrol((LPDRAWITEMSTRUCT)lParam, i);
#endif	// OP_XP_STYLE
		break;

#ifdef OP_XP_STYLE
	case WM_THEMECHANGED:
		// テーマの変更
		if (hThemeUp != 0 && hThemeDown != 0) {
			close_theme(hThemeUp);
			close_theme(hThemeDown);
		}
		hThemeUp = open_theme(GetDlgItem(hDlg, IDC_BUTTON_UP), L"SCROLLBAR");
		hThemeDown = open_theme(GetDlgItem(hDlg, IDC_BUTTON_DOWN), L"SCROLLBAR");
		break;
#endif	// OP_XP_STYLE

	case WM_NOTIFY:
		if (listview_notify_proc(hDlg, lParam, GetDlgItem(hDlg, IDC_LIST_ACTION)) == 0) {
			return OptionNotifyProc(hDlg, uMsg, wParam, lParam);
		}
		break;

	case WM_LV_EVENT:
		switch (wParam) {
		case LVN_ITEMCHANGED:
			enable = (ListView_GetSelectedCount(GetDlgItem(hDlg, IDC_LIST_ACTION)) <= 0) ? FALSE : TRUE;
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_UP), enable);
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_DOWN), enable);
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_EDIT), enable);
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_DELETE), enable);
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_COPY), enable);
			break;

		case NM_CUSTOMDRAW:
			// カスタムドロー
			switch (((LPNMLVCUSTOMDRAW)lParam)->nmcd.dwDrawStage) {
			case CDDS_PREPAINT:
				SetWindowLong(hDlg, DWL_MSGRESULT, CDRF_NOTIFYITEMDRAW);
				return TRUE;

			case CDDS_ITEMPREPAINT:
				ai = (ACTION_INFO *)((LPNMLVCUSTOMDRAW)lParam)->nmcd.lItemlParam;
				if (ai != NULL && ai->enable == 0) {
					((LPNMLVCUSTOMDRAW)lParam)->clrText = GetSysColor(COLOR_GRAYTEXT);
				}
				SetWindowLong(hDlg, DWL_MSGRESULT, CDRF_DODEFAULT);
				return TRUE;
			}
			break;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_BUTTON_UP:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_ACTION), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			if (i == 0) {
				break;
			}
			listview_move_item(GetDlgItem(hDlg, IDC_LIST_ACTION), i, -1);
			SetFocus(GetDlgItem(hDlg, LOWORD(wParam)));
			break;

		case IDC_BUTTON_DOWN:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_ACTION), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			if (i == ListView_GetItemCount(GetDlgItem(hDlg, IDC_LIST_ACTION)) - 1) {
				break;
			}
			listview_move_item(GetDlgItem(hDlg, IDC_LIST_ACTION), i, 1);
			SetFocus(GetDlgItem(hDlg, LOWORD(wParam)));
			break;

		case IDC_BUTTON_ADD:
			DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_ACTION_SET), hDlg, set_action_item_proc, 0);
			break;

		case IDC_BUTTON_EDIT:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_ACTION), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_ACTION_SET), hDlg, set_action_item_proc,
				listview_get_lparam(GetDlgItem(hDlg, IDC_LIST_ACTION), i));
			listview_set_text(GetDlgItem(hDlg, IDC_LIST_ACTION), i);
			break;

		case IDC_BUTTON_DELETE:
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_ACTION), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			if (MessageBox(hDlg, message_get_res(IDS_OPTION_DELETE), WINDOW_TITLE, MB_ICONQUESTION | MB_YESNO) == IDNO) {
				break;
			}
			if ((ai = (ACTION_INFO *)listview_get_lparam(GetDlgItem(hDlg, IDC_LIST_ACTION), i)) != NULL) {
				ini_free_menu(ai->menu_info, ai->menu_cnt);
				mem_free(&ai);
			}
			ListView_DeleteItem(GetDlgItem(hDlg, IDC_LIST_ACTION), i);
			ListView_SetItemState(GetDlgItem(hDlg, IDC_LIST_ACTION), i,
				LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
			break;

		case IDC_BUTTON_COPY:
			// コピーの作成
			if ((i = ListView_GetNextItem(GetDlgItem(hDlg, IDC_LIST_ACTION), -1, LVNI_SELECTED)) == -1) {
				break;
			}
			if ((ai = (ACTION_INFO *)listview_get_lparam(GetDlgItem(hDlg, IDC_LIST_ACTION), i)) == NULL){
				break;
			}
			if((new_ai = copy_action_info(ai)) != NULL) {
				listview_set_action(GetDlgItem(hDlg, IDC_LIST_ACTION), new_ai, FALSE);
				i = ListView_GetItemCount(GetDlgItem(hDlg, IDC_LIST_ACTION)) - 1;
				ListView_SetItemState(GetDlgItem(hDlg, IDC_LIST_ACTION), i,
					LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
				ListView_EnsureVisible(GetDlgItem(hDlg, IDC_LIST_ACTION), i, TRUE);
			}
			break;

		case IDOK:
			// Actionの解放
			for (i = 0; i < option.action_cnt; i++) {
				ini_free_menu((option.action_info + i)->menu_info, (option.action_info + i)->menu_cnt);
			}
			mem_free(&option.action_info);
			// ListViewからActionを取得
			option.action_info = listview_get_action(GetDlgItem(hDlg, IDC_LIST_ACTION), &option.action_cnt);
			listview_free_action(GetDlgItem(hDlg, IDC_LIST_ACTION));
			prop_ret = 1;
			break;

		case IDPCANCEL:
			break;
		}
		break;

	default:
		return FALSE;
	}
	return TRUE;
}
/* End of source */
