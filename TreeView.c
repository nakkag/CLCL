/*
 * CLCL
 *
 * TreeView.c
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
#include "Memory.h"
#include "String.h"
#include "Data.h"
#include "Ini.h"
#include "Message.h"
#include "File.h"
#include "ClipBoard.h"
#include "Regist.h"
#include "Format.h"
#include "Font.h"
#include "Viewer.h"
#include "TreeView.h"
#include "ImageList.h"

#include "resource.h"

/* Define */
#define ERROR_TITLE						TEXT("CLCL - Error")

#define SICONSIZE						16

/* Global Variables */
static HFONT tree_font;

extern HTREEITEM clip_treeitem;
extern HTREEITEM history_treeitem;
extern HTREEITEM regist_treeitem;

extern DATA_INFO clip_di;
extern DATA_INFO history_data;
extern DATA_INFO regist_data;

// オプション
extern OPTION_INFO option;

/* Local Function Prototypes */

/*
 * treeview_create - ツリービューの作成
 */
HWND treeview_create(const HINSTANCE hInstance, const HWND hWnd, const int id, const HIMAGELIST icon_list)
{
	HWND hTreeView;

	// ツリービューの作成
	hTreeView = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_CLIENTEDGE,
		WC_TREEVIEW, NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP |
		TVS_HASLINES | TVS_SHOWSELALWAYS | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_EDITLABELS,
		0, 0, option.viewer_sep_size, 100, hWnd, (HMENU)id, hInstance, NULL);
	if (hTreeView == NULL) {
		return NULL;
	}

	// フォント設定
	if (*option.tree_font_name != TEXT('\0')) {
		if (tree_font != NULL) {
			DeleteObject(tree_font);
		}
		tree_font = font_create(option.tree_font_name, option.tree_font_size, option.tree_font_charset,
			option.tree_font_weight, (option.tree_font_italic == 0) ? FALSE : TRUE, FALSE);
		SendMessage(hTreeView, WM_SETFONT, (WPARAM)tree_font, MAKELPARAM(TRUE, 0));
	}

	// イメージリストの設定
	TreeView_SetImageList(hTreeView, icon_list, TVSIL_NORMAL);
	return hTreeView;
}

/*
 * treeview_set_init_item - ツリービューの初期アイテム設定
 */
BOOL treeview_set_init_item(const HWND hTreeView)
{
	HTREEITEM sel_item = NULL;
	TCHAR *p;

	SendMessage(hTreeView, WM_SETREDRAW, (WPARAM)FALSE, 0);
	TreeView_DeleteAllItems(hTreeView);

	// ルートアイテムを追加
	clip_treeitem = history_treeitem = regist_treeitem = NULL;
	for (p = option.tree_root_order; *p != TEXT('\0'); p++) {
		switch (*p) {
		case TEXT('0'):
			// クリップボード
			if (clip_treeitem != NULL) {
				break;
			}
			clip_treeitem = treeview_set_item(hTreeView, message_get_res(IDS_TREEITEM_CLIPBOARD),
				(HTREEITEM)TVI_ROOT, (HTREEITEM)TVI_LAST, ICON_CLIPBOARD, ICON_CLIPBOARD, (LPARAM)&clip_di);
			if (sel_item == NULL) {
				sel_item = clip_treeitem;
			}
			break;

		case TEXT('1'):
			// 履歴
			if (history_treeitem != NULL) {
				break;
			}
			history_treeitem = treeview_set_item(hTreeView, message_get_res(IDS_TREEITEM_HISTORY),
				(HTREEITEM)TVI_ROOT, (HTREEITEM)TVI_LAST, ICON_HISTORY, ICON_HISTORY, 0);
			if (sel_item == NULL) {
				sel_item = history_treeitem;
			}
			break;

		case TEXT('2'):
			// 登録アイテム
			if (regist_treeitem != NULL) {
				break;
			}
			regist_treeitem = treeview_set_item(hTreeView, message_get_res(IDS_TREEITEM_REGIST),
				(HTREEITEM)TVI_ROOT, (HTREEITEM)TVI_LAST, ICON_REGIST, ICON_REGIST, 0);
			if (sel_item == NULL) {
				sel_item = regist_treeitem;
			}
			break;
		}
	}
	if (sel_item == NULL) {
		clip_treeitem = treeview_set_item(hTreeView, message_get_res(IDS_TREEITEM_CLIPBOARD),
			(HTREEITEM)TVI_ROOT, (HTREEITEM)TVI_LAST, ICON_CLIPBOARD, ICON_CLIPBOARD, (LPARAM)&clip_di);
		sel_item = clip_treeitem;
		history_treeitem = treeview_set_item(hTreeView, message_get_res(IDS_TREEITEM_HISTORY),
			(HTREEITEM)TVI_ROOT, (HTREEITEM)TVI_LAST, ICON_HISTORY, ICON_HISTORY, 0);
		regist_treeitem = treeview_set_item(hTreeView, message_get_res(IDS_TREEITEM_REGIST),
			(HTREEITEM)TVI_ROOT, (HTREEITEM)TVI_LAST, ICON_REGIST, ICON_REGIST, 0);
	}

	// クリップボード
	if (clip_treeitem != NULL) {
		treeview_datainfo_to_treeitem(hTreeView, clip_treeitem, clip_di.child);
		if (option.tree_clip_expand == 1) {
			TreeView_Expand(hTreeView, clip_treeitem, TVE_EXPAND);
		}
	}

	// 履歴
	if (history_treeitem != NULL) {
		treeview_datainfo_to_treeitem(hTreeView, history_treeitem, history_data.child);
		if (option.tree_history_expand == 1) {
			TreeView_Expand(hTreeView, history_treeitem, TVE_EXPAND);
		}
	}

	// 登録アイテム
	if (regist_treeitem != NULL) {
		treeview_datainfo_to_treeitem(hTreeView, regist_treeitem, regist_data.child);
		if (option.tree_regist_expand == 1) {
			TreeView_Expand(hTreeView, regist_treeitem, TVE_EXPAND);
		}
	}

	// 一番上のアイテムを選択
	TreeView_SelectItem(hTreeView, sel_item);
	TreeView_EnsureVisible(hTreeView, sel_item);

	SendMessage(hTreeView, WM_SETREDRAW, (WPARAM)TRUE, 0);
	UpdateWindow(hTreeView);
	return TRUE;
}

/*
 * treeview_close - ツリービューの解放
 */
void treeview_close(const HWND hTreeView)
{
	// フォント解放
	if (tree_font != NULL) {
		DeleteObject(tree_font);
		tree_font = NULL;
	}
}

/*
 * treeview_notify_proc - ツリービューイベント
 */
LRESULT treeview_notify_proc(const HWND hWnd, LPARAM lParam)
{
	TV_DISPINFO *tvd = (TV_DISPINFO *)lParam;
	NM_TREEVIEW *nmtv = (NM_TREEVIEW *)lParam;
	TV_KEYDOWN *tvk = (TV_KEYDOWN *)lParam;
	NMHDR *nmhdr = (NMHDR *)lParam;

	switch (tvd->hdr.code) {
	case TVN_BEGINLABELEDIT:
	case TVN_ENDLABELEDIT:
		return SendMessage(hWnd, WM_TV_EVENT, tvd->hdr.code, lParam);
	}

	switch (nmtv->hdr.code) {
	case TVN_BEGINDRAG:
	case TVN_BEGINRDRAG:
	case TVN_SELCHANGING:
	case TVN_SELCHANGED:
	case NM_CUSTOMDRAW:
		return SendMessage(hWnd, WM_TV_EVENT, nmtv->hdr.code, lParam);
	}

	switch (tvk->hdr.code) {
	case TVN_KEYDOWN:			// キーダウン
		return SendMessage(hWnd, WM_TV_EVENT, tvk->hdr.code, lParam);
	}

	switch (nmhdr->code) {
	case NM_SETFOCUS:
	case NM_RCLICK:
	case NM_DBLCLK:
		return SendMessage(hWnd, WM_TV_EVENT, nmhdr->code, lParam);
	}
	return FALSE;
}

/*
 * treeview_scroll - ツリービューをスクロールしてアイテムを表示
 */
void treeview_scroll(const HWND hTreeView)
{
	RECT rect, item_rect;
	POINT pt;

	// ツリービューのサイズ取得
	GetClientRect(hTreeView, &rect);
	// マウス位置取得
	GetCursorPos(&pt);
	ScreenToClient(hTreeView, &pt);
	//アイテムの高さ取得
	TreeView_GetItemRect(hTreeView, treeview_get_hitest(hTreeView), &item_rect, FALSE);

	// スクロール
	if (pt.y < rect.top + (item_rect.bottom - item_rect.top)) {
		SendMessage(hTreeView, WM_VSCROLL, SB_LINEUP, 0);
	} else if (pt.y > rect.bottom - (item_rect.bottom - item_rect.top)) {
		SendMessage(hTreeView, WM_VSCROLL, SB_LINEDOWN, 0);
	}
}

/*
 * treeview_set_item - ツリービューアイテムの追加
 */
HTREEITEM treeview_set_item(const HWND hTreeView, TCHAR *buf, const HTREEITEM hParent, const HTREEITEM After,
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
 * treeview_delete_child - 子アイテムを削除
 */
void treeview_delete_child(const HWND hTreeView, const HTREEITEM parent_item)
{
	HTREEITEM hItem;

	hItem = TreeView_GetNextItem(hTreeView, parent_item, TVGN_CHILD);
	while (hItem != NULL) {
		TreeView_DeleteItem(hTreeView, hItem);
		hItem = TreeView_GetNextItem(hTreeView, parent_item, TVGN_CHILD);
	}
}

/*
 * treeview_set_text - アイテムのテキストを設定
 */
BOOL treeview_set_text(const HWND hTreeView, const HTREEITEM hItem, TCHAR *text)
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
 * treeview_get_text - アイテムのテキストを取得
 */
BOOL treeview_get_text(const HWND hTreeView, const HTREEITEM hItem, TCHAR *text)
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
 * treeview_set_icon - アイテムのアイコンを設定
 */
BOOL treeview_set_icon(const HWND hTreeView, const HTREEITEM hItem, const int icon, const int select_icon)
{
	TV_ITEM tvit;

	if (hItem == NULL) {
		return FALSE;
	}
	tvit.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvit.hItem = hItem;
	tvit.iImage = icon;
	tvit.iSelectedImage = select_icon;
	return TreeView_SetItem(hTreeView, &tvit);
}

/*
 * treeview_get_icon - アイテムのアイコンを取得
 */
int treeview_get_icon(const HWND hTreeView, const HTREEITEM hItem)
{
	TV_ITEM tvit;

	if (hItem == NULL) {
		return FALSE;
	}
	tvit.mask = TVIF_IMAGE;
	tvit.hItem = hItem;
	tvit.iImage = 0;
	TreeView_GetItem(hTreeView, &tvit);
	return tvit.iImage;
}

/*
 * treeview_set_lparam - アイテムに情報を関連つける
 */
BOOL treeview_set_lparam(const HWND hTreeView, const HTREEITEM hItem, const LPARAM lParam)
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
LPARAM treeview_get_lparam(const HWND hTreeView, const HTREEITEM hItem)
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
 * treeview_lparam_to_item - LPARAMからアイテムを検索
 */
HTREEITEM treeview_lparam_to_item(const HWND hTreeView, const HTREEITEM hParent, const LPARAM lParam)
{
	HTREEITEM hItem;
	HTREEITEM ret_item;

	if (hParent == NULL) {
		return NULL;
	}
	hItem = TreeView_GetChild(hTreeView, hParent);
	while (hItem != NULL) {
		if (treeview_get_lparam(hTreeView, hItem) == lParam) {
			return hItem;
		}
		if ((ret_item = treeview_lparam_to_item(hTreeView, hItem, lParam)) != NULL) {
			return ret_item;
		}
		hItem = TreeView_GetNextSibling(hTreeView, hItem);
	}
	return NULL;
}

/*
 * treeview_get_rootitem - アイテムのルートを取得
 */
HTREEITEM treeview_get_rootitem(const HWND hTreeView, const HTREEITEM hItem)
{
	HTREEITEM ret_item = hItem;
	HTREEITEM wk_item = hItem;

	while ((wk_item = TreeView_GetParent(hTreeView, wk_item)) != NULL) {
		ret_item = wk_item;
	}
	return ret_item;
}

/*
 * treeview_get_hitest - マウスの下のアイテムを取得
 */
HTREEITEM treeview_get_hitest(const HWND hTreeView)
{
	TV_HITTESTINFO tvht;
	POINT apos;
	RECT treeview_rect;

	// マウスの位置を取得
	GetCursorPos((LPPOINT)&apos);
	GetWindowRect(hTreeView, (LPRECT)&treeview_rect);
	apos.x = apos.x - treeview_rect.left;
	apos.y = apos.y - treeview_rect.top;

	tvht.pt = apos;
	tvht.flags = TVHT_NOWHERE;
	tvht.hItem = NULL;
	return TreeView_HitTest(hTreeView, &tvht);
}

/*
 * treeview_copy_item - ツリビューアイテムのコピーを作成
 */
HTREEITEM treeview_copy_item(const HWND hTreeView, const HTREEITEM parent_item, const HTREEITEM hItem, const HTREEITEM After)
{
	TV_INSERTSTRUCT tvitn;
	TV_ITEM tvit;
	HTREEITEM new_item;
	HTREEITEM cItem;
	TCHAR buf[BUF_SIZE];

	tvit.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
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
	return new_item;
}

/*
 * treeview_datainfo_to_treeitem - ツリービューにデータを設定
 */
HTREEITEM treeview_datainfo_to_treeitem(const HWND hTreeView, const HTREEITEM parent_item, DATA_INFO *di)
{
	HTREEITEM hItem;
	HTREEITEM ret_item = NULL;
	HTREEITEM pos_item;
	HTREEITEM prev_item;
	DATA_INFO *wk_di;
	int icon;

	for (; di != NULL; di = di->next) {
		switch (di->type) {
		case TYPE_FOLDER:
			// フォルダ
			pos_item = (HTREEITEM)TVI_LAST;
			prev_item = NULL;
			hItem = TreeView_GetChild(hTreeView, parent_item);
			while (hItem != NULL) {
				wk_di = (DATA_INFO *)treeview_get_lparam(hTreeView, hItem);
				if (wk_di == di) {
					pos_item = (prev_item != NULL) ? prev_item : (HTREEITEM)TVI_FIRST;
					TreeView_DeleteItem(hTreeView, hItem);
					break;
				}
				prev_item = hItem;
				hItem = TreeView_GetNextSibling(hTreeView, hItem);
			}
			hItem = treeview_set_item(hTreeView, di->title, parent_item, pos_item,
				ICON_FOLDER, ICON_FOLDER_OPEN, (LPARAM)di);
			if (hItem == NULL) {
				return NULL;
			}
			treeview_datainfo_to_treeitem(hTreeView, hItem, di->child);
			if (option.tree_folder_expand == 1) {
				TreeView_Expand(hTreeView, hItem, TVE_EXPAND);
			}
			if (ret_item == NULL) {
				ret_item = hItem;
			}
			if (pos_item != (HTREEITEM)TVI_LAST) {
				return ret_item;
			}
			break;

		case TYPE_ITEM:
			// アイテム
			wk_di = format_get_priority_highest(di);
			icon = ICON_FORMAT + format_get_index(wk_di->format_name, wk_di->format_name_hash);
			hItem = treeview_set_item(hTreeView, data_get_title(di),
				parent_item, (HTREEITEM)TVI_LAST, icon, icon, (LPARAM)di);
			if (hItem == NULL) {
				return NULL;
			}
			if (option.tree_show_format == 1) {
				treeview_datainfo_to_treeitem(hTreeView, hItem, di->child);
			}
			if (ret_item == NULL) {
				ret_item = hItem;
			}
			break;

		case TYPE_DATA:
			// データ
			icon = ICON_FORMAT + format_get_index(di->format_name, di->format_name_hash);
			hItem = treeview_set_item(hTreeView, di->format_name, parent_item, (HTREEITEM)TVI_LAST, icon, icon, (LPARAM)di);
			if (ret_item == NULL) {
				ret_item = hItem;
			}
			break;
		}
	}
	return ret_item;
}

/*
 * treeview_sync_datainfo - データとツリーアイテムの同期
 */
BOOL treeview_sync_datainfo(const HWND hTreeView, const HTREEITEM parent_item, DATA_INFO *di)
{
	HTREEITEM hItem;
	HTREEITEM delete_item;
	HTREEITEM insert_item;
	DATA_INFO *cdi;
	DATA_INFO *wk_di;
	int icon;

	// ツリーアイテムの削除
	hItem = TreeView_GetNextItem(hTreeView, parent_item, TVGN_CHILD);
	while (hItem != NULL) {
		wk_di = (DATA_INFO *)treeview_get_lparam(hTreeView, hItem);
		for (cdi = di; cdi != NULL && cdi != wk_di; cdi = cdi->next)
			;
		if (cdi == NULL) {
			// ツリーからアイテムを削除
			delete_item = hItem;
			hItem = TreeView_GetNextItem(hTreeView, hItem, TVGN_NEXT);
			TreeView_DeleteItem(hTreeView, delete_item);
		} else {
			hItem = TreeView_GetNextItem(hTreeView, hItem, TVGN_NEXT);
		}
	}

	// ツリーアイテムの追加
	insert_item = (HTREEITEM)TVI_FIRST;
	hItem = TreeView_GetNextItem(hTreeView, parent_item, TVGN_CHILD);
	cdi = (hItem == NULL) ? NULL : (DATA_INFO *)treeview_get_lparam(hTreeView, hItem);
	for (; di != NULL; di = di->next) {
		if (di == cdi) {
			switch (cdi->type) {
			case TYPE_FOLDER:
				treeview_set_text(hTreeView, hItem, cdi->title);
				if (treeview_sync_datainfo(hTreeView, hItem, cdi->child) == FALSE) {
					return FALSE;
				}
				break;

			case TYPE_ITEM:
				wk_di = format_get_priority_highest(cdi);
				icon = ICON_FORMAT + format_get_index(wk_di->format_name, wk_di->format_name_hash);
				treeview_set_icon(hTreeView, hItem, icon, icon);
				treeview_set_text(hTreeView, hItem, data_get_title(cdi));
				if (option.tree_show_format == 1 &&
					treeview_sync_datainfo(hTreeView, hItem, cdi->child) == FALSE) {
					return FALSE;
				}
				break;

			case TYPE_DATA:
				icon = ICON_FORMAT + format_get_index(cdi->format_name, cdi->format_name_hash);
				treeview_set_icon(hTreeView, hItem, icon, icon);
				treeview_set_text(hTreeView, hItem, cdi->format_name);
				break;
			}
		} else {
			switch (di->type) {
			case TYPE_FOLDER:
				// フォルダ
				hItem = treeview_set_item(hTreeView, di->title,
					parent_item, insert_item, ICON_FOLDER, ICON_FOLDER_OPEN, (LPARAM)di);
				if (hItem == NULL) {
					return FALSE;
				}
				treeview_datainfo_to_treeitem(hTreeView, hItem, di->child);
				break;

			case TYPE_ITEM:
				// アイテム
				wk_di = format_get_priority_highest(di);
				icon = ICON_FORMAT + format_get_index(wk_di->format_name, wk_di->format_name_hash);
				hItem = treeview_set_item(hTreeView, data_get_title(di),
					parent_item, insert_item, icon, icon, (LPARAM)di);
				if (option.tree_show_format == 1 &&
					hItem != NULL && treeview_datainfo_to_treeitem(hTreeView, hItem, di->child) == NULL) {
					return FALSE;
				}
				break;

			case TYPE_DATA:
				// データ
				icon = ICON_FORMAT + format_get_index(di->format_name, di->format_name_hash);
				hItem = treeview_set_item(hTreeView, di->format_name,
					parent_item, insert_item, icon, icon, (LPARAM)di);
				break;
			}
		}
		insert_item = hItem;
		if (hItem != (HTREEITEM)TVI_LAST) {
			if ((hItem = TreeView_GetNextItem(hTreeView, hItem, TVGN_NEXT)) != NULL) {
				cdi = (DATA_INFO *)treeview_get_lparam(hTreeView, hItem);
			} else {
				hItem = (HTREEITEM)TVI_LAST;
				cdi = NULL;
			}
		}
	}
	while (hItem != NULL && hItem != (HTREEITEM)TVI_LAST) {
		// ツリーからアイテムを削除
		delete_item = hItem;
		hItem = TreeView_GetNextItem(hTreeView, hItem, TVGN_NEXT);
		TreeView_DeleteItem(hTreeView, delete_item);
	}
	return TRUE;
}

/*
 * treeview_refresh_title - ツリービューアイテムのタイトルを更新
 */
BOOL treeview_title_refresh(const HWND hTreeView, HTREEITEM hItem)
{
	DATA_INFO *di;
	DATA_INFO *wk_di;
	int icon;

	if ((di = (DATA_INFO *)treeview_get_lparam(hTreeView, hItem)) == NULL) {
		return FALSE;
	}
	switch (di->type) {
	case TYPE_DATA:
		if ((hItem = TreeView_GetParent(hTreeView, hItem)) == NULL) {
			return FALSE;
		}
		if ((di = (DATA_INFO *)treeview_get_lparam(hTreeView, hItem)) == NULL) {
			return FALSE;
		}
	case TYPE_ITEM:
		wk_di = format_get_priority_highest(di);
		break;

	default:
		return FALSE;
	}
	// タイトルを設定
	data_menu_free_item(wk_di);
	treeview_set_text(hTreeView, hItem, data_get_title(di));
	// アイコンを設定
	icon = ICON_FORMAT + format_get_index(wk_di->format_name, wk_di->format_name_hash);
	treeview_set_icon(hTreeView, hItem, icon, icon);
	return TRUE;
}

/*
 * treeview_delete_item - ツリービューからアイテムの削除
 */
BOOL treeview_delete_item(const HWND hTreeView, const HTREEITEM hItem)
{
	HTREEITEM pItem;
	DATA_INFO *di, *wk_di;
	DATA_INFO **pdi;

	// データの取得
	if ((di = (DATA_INFO *)treeview_get_lparam(hTreeView, hItem)) == NULL) {
		return FALSE;
	}

	// ルートデータの取得
	pItem = TreeView_GetParent(hTreeView, hItem);
	wk_di = NULL;
	if (pItem == NULL || pItem == clip_treeitem) {
		return FALSE;
	} else if (pItem == history_treeitem) {
		pdi = &history_data.child;
	} else if (pItem == regist_treeitem) {
		pdi = &regist_data.child;
	} else {
		wk_di = (DATA_INFO *)treeview_get_lparam(hTreeView, pItem);
		pdi = &wk_di->child;
	}
	if (pdi == NULL || *pdi == NULL) {
		return FALSE;
	}

	if (hItem == TreeView_GetSelection(hTreeView)) {
		// 選択中のアイテムはデータの関連つけを解除
		TreeView_SelectItem(hTreeView, NULL);
		treeview_set_lparam(hTreeView, hItem, 0);
		TreeView_SelectItem(hTreeView, hItem);
	}

	switch (di->type) {
	case TYPE_DATA:
		// データの削除
		data_delete(pdi, di, TRUE);
		if (*pdi == NULL) {
			treeview_delete_item(hTreeView, pItem);
		} else {
			treeview_title_refresh(hTreeView, pItem);
		}
		break;

	case TYPE_ITEM:
	case TYPE_FOLDER:
		// アイテムの削除
		data_delete(pdi, di, TRUE);
		break;
	}

	// ツリービューアイテムの削除
	TreeView_DeleteItem(hTreeView, hItem);
	return TRUE;
}

/*
 * treeview_move_up - アイテムを上に移動
 */
HTREEITEM treeview_move_up(const HWND hTreeView, const HTREEITEM hItem)
{
	HTREEITEM parent_item, prev_item, ret_item;
	DATA_INFO *di, *cdi, *prev_di, *wk_di;
	DATA_INFO **pdi;

	// 前のアイテムを取得
	if ((prev_item = TreeView_GetPrevSibling(hTreeView, hItem)) == NULL) {
		return NULL;
	}
	if ((di = (DATA_INFO *)treeview_get_lparam(hTreeView, prev_item)) == NULL) {
		return NULL;
	}

	// 親データを取得
	if ((parent_item = TreeView_GetParent(hTreeView, hItem)) == history_treeitem) {
		pdi = &history_data.child;
	} else if (parent_item == regist_treeitem) {
		pdi = &regist_data.child;
	} else if (parent_item == NULL || parent_item == clip_treeitem) {
		return NULL;
	} else {
		di = (DATA_INFO *)treeview_get_lparam(hTreeView, parent_item);
		if (di == NULL) {
			return NULL;
		}
		pdi = &di->child;
	}

	// データ入れ替え
	if ((di = (DATA_INFO *)treeview_get_lparam(hTreeView, hItem)) == NULL) {
		return NULL;
	}
	prev_di = NULL;
	for (cdi = *pdi; cdi != NULL; cdi = cdi->next) {
		if (cdi->next == di) {
			wk_di = di->next;
			di->next = cdi;
			cdi->next = wk_di;

			if (prev_di == NULL) {
				*pdi = di;
			} else {
				prev_di->next = di;
			}
			break;
		}
		prev_di = cdi;
	}

	// ツリービューアイテムの入れ替え
	if ((prev_item = TreeView_GetPrevSibling(hTreeView, prev_item)) == NULL) {
		prev_item = (HTREEITEM)TVI_FIRST;
	}
	if ((ret_item = treeview_copy_item(hTreeView, parent_item, hItem, prev_item)) == NULL) {
		return NULL;
	}
	if (TreeView_GetSelection(hTreeView) == hItem) {
		TreeView_SelectItem(hTreeView, ret_item);
	}
	TreeView_DeleteItem(hTreeView, hItem);
	return ret_item;
}

/*
 * treeview_move_down - アイテムを下に移動
 */
HTREEITEM treeview_move_down(const HWND hTreeView, const HTREEITEM hItem)
{
	HTREEITEM parent_item, next_item, ret_item;
	DATA_INFO *di, *cdi, *prev_di, *wk_di;
	DATA_INFO **pdi;

	// 前のアイテムを取得
	if ((next_item = TreeView_GetNextSibling(hTreeView, hItem)) == NULL) {
		return NULL;
	}

	// 親データを取得
	if ((parent_item = TreeView_GetParent(hTreeView, hItem)) == history_treeitem) {
		pdi = &history_data.child;
	} else if (parent_item == regist_treeitem) {
		pdi = &regist_data.child;
	} else if (parent_item == NULL || parent_item == clip_treeitem) {
		return NULL;
	} else {
		if ((di = (DATA_INFO *)treeview_get_lparam(hTreeView, parent_item)) == NULL) {
			return NULL;
		}
		pdi = &di->child;
	}

	// データ入れ替え
	if ((di = (DATA_INFO *)treeview_get_lparam(hTreeView, hItem)) == NULL || di->next == NULL) {
		return NULL;
	}
	prev_di = NULL;
	for (cdi = *pdi; cdi != NULL; cdi = cdi->next) {
		if (cdi == di) {
			cdi = cdi->next;

			wk_di = cdi->next;
			cdi->next = di;
			di->next = wk_di;

			if (prev_di == NULL) {
				*pdi = cdi;
			} else {
				prev_di->next = cdi;
			}
			break;
		}
		prev_di = cdi;
	}

	// ツリービューアイテムの入れ替え
	if ((ret_item = treeview_copy_item(hTreeView, parent_item, hItem, next_item)) == NULL) {
		return NULL;
	}
	if (TreeView_GetSelection(hTreeView) == hItem) {
		TreeView_SelectItem(hTreeView, ret_item);
	}
	TreeView_DeleteItem(hTreeView, hItem);
	return ret_item;
}
/* End of source */
