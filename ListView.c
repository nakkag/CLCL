/*
 * CLCL
 *
 * ListView.c
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
#include "Ini.h"
#include "Message.h"
#include "Font.h"
#include "TreeView.h"
#include "ListView.h"

#include "resource.h"

/* Define */
#ifndef LVS_EX_INFOTIP
#define LVS_EX_INFOTIP					0x400
#endif

#define SICONSIZE						16

#define ABS(n)							((n < 0) ? (n * -1) : n)

/* Global Variables */
static HFONT list_font;
static HWND gTreeView;

// オプション
extern OPTION_INFO option;

/* Local Function Prototypes */

/*
 * listview_create - リストビューの作成
 */
HWND listview_create(const HINSTANCE hInstance, const HWND hWnd, const int id, const HIMAGELIST icon_list)
{
	HWND hListView;
	LV_COLUMN lvc;

	// リストビューの作成
	hListView = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_CLIENTEDGE,
		WC_LISTVIEW, NULL, WS_TABSTOP | WS_CHILD |
		LVS_REPORT | LVS_AUTOARRANGE | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS | LVS_EDITLABELS,
		0, 0, 500, 500, hWnd, (HMENU)id, hInstance, NULL);
	if (hListView == NULL) {
		return NULL;
	}
	ListView_SetExtendedListViewStyle(hListView, LVS_EX_INFOTIP);

	// フォント設定
	if (*option.list_font_name != TEXT('\0')) {
		if (list_font != NULL) {
			DeleteObject(list_font);
		}
		list_font = font_create(option.list_font_name, option.list_font_size, option.list_font_charset,
			option.list_font_weight, (option.list_font_italic == 0) ? FALSE : TRUE, FALSE);
		SendMessage(hListView, WM_SETFONT, (WPARAM)list_font, MAKELPARAM(TRUE, 0));
	}
	
	// イメージリストの設定
	ListView_SetImageList(hListView, icon_list, LVSIL_SMALL);

	// カラム設定
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = option.list_column_data;
	lvc.pszText = message_get_res(IDS_LISTCOLUMN_DATA);
	lvc.iSubItem = 0;
	ListView_InsertColumn(hListView, lvc.iSubItem, &lvc);

	lvc.fmt = LVCFMT_RIGHT;
	lvc.cx = option.list_column_size;
	lvc.pszText = message_get_res(IDS_LISTCOLUMN_SIZE);
	lvc.iSubItem = 1;
	ListView_InsertColumn(hListView, lvc.iSubItem, &lvc);

	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = option.list_column_date;
	lvc.pszText = message_get_res(IDS_LISTCOLUMN_DATE);
	lvc.iSubItem = 2;
	ListView_InsertColumn(hListView, lvc.iSubItem, &lvc);

	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = option.list_column_window;
	lvc.pszText = message_get_res(IDS_LISTCOLUMN_WINDOW);
	lvc.iSubItem = 3;
	ListView_InsertColumn(hListView, lvc.iSubItem, &lvc);
	return hListView;
}

/*
 * listview_close - リストビューの解放
 */
void listview_close(const HWND hListView)
{
	// フォント解放
	if (list_font != NULL) {
		DeleteObject(list_font);
		list_font = NULL;
	}
}

/*
 * ListView_NotifyProc - リストビューイベント
 */
LRESULT listview_notify_proc(const HWND hWnd, const LPARAM lParam)
{
	LV_DISPINFO *lvd = (LV_DISPINFO *)lParam;
	NM_LISTVIEW *nmlv = (NM_LISTVIEW *)lParam;
	LV_KEYDOWN *lvk = (LV_KEYDOWN *)lParam;
	NMHDR *nmhdr = (NMHDR *)lParam;

	switch (lvd->hdr.code) {
	case LVN_ITEMCHANGED:		// アイテムの選択状態の変更
	case LVN_BEGINLABELEDIT:	// タイトル編集開始
	case LVN_ENDLABELEDIT:		// タイトル編集終了
	case LVN_GETDISPINFO:		// 表示アイテムの要求
	case LVN_ITEMACTIVATE:
		return SendMessage(hWnd, WM_LV_EVENT, lvd->hdr.code, lParam);
	}

	switch (nmlv->hdr.code) {
	case LVN_BEGINDRAG:			// ドラッグの開始 (マウスの左ボタン)
	case LVN_BEGINRDRAG:		// ドラッグの開始 (マウスの右ボタン)
	case NM_CUSTOMDRAW:
		return SendMessage(hWnd, WM_LV_EVENT, nmlv->hdr.code, lParam);
	}

	switch (lvk->hdr.code) {
	case LVN_KEYDOWN:			// キーダウン
		return SendMessage(hWnd, WM_LV_EVENT, lvk->hdr.code, lParam);
	}

	switch (nmhdr->code) {
	case NM_SETFOCUS:			// フォーカスの変更
	case NM_CLICK:				// クリック
	case NM_DBLCLK:				// ダブルクリック
	case NM_RCLICK:				// 右クリック
		return SendMessage(hWnd, WM_LV_EVENT, nmhdr->code, lParam);
	}
	return FALSE;
}

/*
 * compare_func - ソート用文字列比較
 */
static int CALLBACK compare_func(LPARAM lParam1, LPARAM lParam2, LPARAM colum)
{
	DATA_INFO *d1, *d2;
	DATA_INFO *cdi;
	TCHAR buf1[BUF_SIZE];
	TCHAR buf2[BUF_SIZE];
	int len1 = 0, len2 = 0;
	int order;
	int header;
	int ret;

	// ソート情報
	order = (colum < 0) ? 1 : 0;
	header = ABS(colum) - 1;

	// データ取得
	if (lParam1 == 0 || lParam2 == 0 ||
		(d1 = (DATA_INFO *)treeview_get_lparam(gTreeView, (HTREEITEM)lParam1)) == NULL ||
		(d2 = (DATA_INFO *)treeview_get_lparam(gTreeView, (HTREEITEM)lParam2)) == NULL) {
		return 0;
	}

	*buf1 = TEXT('\0');
	*buf2 = TEXT('\0');

	switch (header) {
	case 0:
		// データ
		lstrcpy(buf1, data_get_title(d1));
		lstrcpy(buf2, data_get_title(d2));
		ret = lstrcmpi(buf1, buf2);
		break;

	case 1:
		// サイズ
		len1 = 0;
		for (cdi = d1->child; cdi != NULL; cdi = cdi->next) {
			len1 += cdi->size;
		}
		len2 = 0;
		for (cdi = d2->child; cdi != NULL; cdi = cdi->next) {
			len2 += cdi->size;
		}
		ret = len1 - len2;
		break;

	case 2:
		// 更新日時
		ret = CompareFileTime(&d1->modified, &d2->modified);
		break;

	case 3:
		// ウィンドウ名
		if (d1->window_name != NULL) {
			lstrcpy(buf1, d1->window_name);
		}
		if (d2->window_name != NULL) {
			lstrcpy(buf2, d2->window_name);
		}
		ret = lstrcmpi(buf1, buf2);
		break;
	}
	return (((ret < 0 && order == 1) || (ret > 0 && order == 0)) ? 1 : -1);
}

/*
 * listview_header_notify_proc - リストビューヘッダメッセージ
 */
LRESULT listview_header_notify_proc(const HWND hListView, const HWND hTreeView, const LPARAM lParam)
{
	HD_NOTIFY *hdn = (HD_NOTIFY *)lParam;
	static int colum;

	switch (hdn->hdr.code) {
	case HDN_ITEMCLICK:
		// ソートの設定
		colum = (ABS(colum) == (hdn->iItem + 1)) ? (colum * -1) : (hdn->iItem + 1);
		gTreeView = hTreeView;
		// ソート
		ListView_SortItems(hListView, compare_func, colum);
		break;
	}
	return FALSE;
}

/*
 * listview_set_lparam - アイテムのLPARAMを設定
 */
BOOL listview_set_lparam(const HWND hListView, const int i, const LPARAM lParam)
{
	LV_ITEM lvi;

	ZeroMemory(&lvi, sizeof(LVITEM));
	lvi.mask = LVIF_PARAM;
	lvi.iItem = i;
	lvi.lParam = lParam;
	return ListView_SetItem(hListView, &lvi);
}

/*
 * listview_get_lparam - アイテムのLPARAMを取得
 */
LPARAM listview_get_lparam(const HWND hListView, const int i)
{
	LV_ITEM lvi;

	ZeroMemory(&lvi, sizeof(LVITEM));
	lvi.mask = LVIF_PARAM;
	lvi.iItem = i;
	ListView_GetItem(hListView, &lvi);
	return lvi.lParam;
}

/*
 * listview_lparam_to_item - LPARAMからアイテムを検索
 */
int listview_lparam_to_item(const HWND hListView, const LPARAM lParam)
{
	int cnt;
	int i;

	cnt = ListView_GetItemCount(hListView);
	for (i = 0; i < cnt ; i++) {
		if (listview_get_lparam(hListView, i) == lParam) {
			return i;
		}
	}
	return -1;
}

/*
 * listview_lparam_select - LPARAMからアイテムを選択
 */
BOOL listview_lparam_select(const HWND hListView, const LPARAM lParam)
{
	int i;

	if ((i = listview_lparam_to_item(hListView, lParam)) == -1) {
		return FALSE;
	}
	ListView_SetItemState(hListView, -1, 0, LVIS_SELECTED);
	ListView_SetItemState(hListView, i,
		LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	ListView_EnsureVisible(hListView, i, TRUE);
	return TRUE;
}

/*
 * listview_get_icon - アイテムのアイコンインデックスを取得
 */
int listview_get_icon(const HWND hListView, const int i)
{
	LV_ITEM lvi;

	ZeroMemory(&lvi, sizeof(LVITEM));
	lvi.mask = TVIF_IMAGE;
	lvi.iItem = i;
	ListView_GetItem(hListView, &lvi);
	return lvi.iImage;
}

/*
 * listview_get_hitest - マウスの下のアイテムのインデックスを取得
 */
int listview_get_hitest(const HWND hListView)
{
	LV_HITTESTINFO lvht;
	POINT apos;
	RECT listview_rect;

	GetCursorPos((LPPOINT)&apos);
	GetWindowRect(hListView, (LPRECT)&listview_rect);
	apos.x = apos.x - listview_rect.left;
	apos.y = apos.y - listview_rect.top;

	lvht.pt = apos;
	lvht.flags = LVHT_ONITEMICON | LVHT_ONITEMLABEL | LVHT_ONITEMSTATEICON;
	lvht.iItem = 0;
	return ListView_HitTest(hListView, &lvht);
}

/*
 * listview_move_item - リストビューのアイテムを移動
 */
void listview_move_item(const HWND hListView, int index, const int Move)
{
	LV_ITEM lvi;
	LPARAM lp;

	lp = listview_get_lparam(hListView, index);
	ListView_DeleteItem(hListView, index);

	index += Move;

	lvi.mask = LVIF_TEXT | TVIF_IMAGE | LVIF_PARAM;
	lvi.iItem = index;
	lvi.iSubItem = 0;
	lvi.pszText = LPSTR_TEXTCALLBACK;
	lvi.cchTextMax = 0;
	lvi.iImage = I_IMAGECALLBACK;
	lvi.lParam = lp;
	ListView_InsertItem(hListView, &lvi);

	ListView_SetItemState(hListView, index,
		LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	ListView_EnsureVisible(hListView, index, TRUE);
}
/* End of source */
