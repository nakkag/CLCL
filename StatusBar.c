/*
 * CLCL
 *
 * StatusBar.c
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
#include <limits.h>

#include "General.h"
#include "Memory.h"
#include "Data.h"
#include "Ini.h"
#include "Message.h"
#include "Menu.h"
#include "Viewer.h"
#include "TreeView.h"
#include "StatusBar.h"
#include "dpi.h"

#include "resource.h"

/* Define */
#ifndef SBT_TOOLTIPS
#define SBT_TOOLTIPS					0x0800
#endif

/* Global Variables */
extern HTREEITEM clip_treeitem;
extern HTREEITEM history_treeitem;
extern HTREEITEM regist_treeitem;

extern DATA_INFO history_data;
extern DATA_INFO regist_data;

// オプション
extern OPTION_INFO option;

/* Local Function Prototypes */
static ULONGLONG statusbar_get_allsize(DATA_INFO *di);
static void statusbar_get_size_text(const ULONGLONG size, TCHAR *ret);

/*
 * statusbar_create - StatusBarの作成
 */
HWND statusbar_create(const HWND hWnd, const int id)
{
	HWND hStatusBar;

	if ((hStatusBar = CreateStatusWindow(WS_CHILD | SBT_TOOLTIPS, TEXT(""), hWnd, id)) == NULL) {
		return NULL;
	}
	if (option.viewer_show_statusbar == 1) {
		// ステータスバー表示
		ShowWindow(hStatusBar, SW_SHOW);
	}
	// パーツの設定
	statusbar_reset_parts(hStatusBar);
	return hStatusBar;
}

/*
 * statusbar_reset_parts - StatusBarのパーツを現在のDPIに合わせる
 */
void statusbar_reset_parts(const HWND hStatusBar)
{
	int width[] = {Scale(150), Scale(270), -1};

	if (hStatusBar == NULL) {
		return;
	}
	SendMessage(hStatusBar, SB_SETPARTS, (WPARAM)sizeof(width) / sizeof(int), (LPARAM)width);
}

/*
 * statusbar_get_allsize - データサイズの合計
 */
static ULONGLONG statusbar_get_allsize(DATA_INFO *di)
{
	DATA_INFO *cdi = di;
	ULONGLONG cnt = 0;

	for (; cdi != NULL; cdi = cdi->next) {
		switch (cdi->type) {
		case TYPE_FOLDER:
			break;

		case TYPE_ITEM:
			cnt += statusbar_get_allsize(cdi->child);
			break;

		case TYPE_DATA:
			cnt += cdi->size;
			break;
		}
	}
	return cnt;
}

/*
 * statusbar_set_text - サイズ文字列の取得
 */
static void statusbar_get_size_text(const ULONGLONG size, TCHAR *ret)
{
	ULONGLONG kb;

	if (size < 1024) {
		wsprintf(ret, message_get_res(IDS_STATUSBAR_BYTES), (int)size);
	} else {
		// 書式は %d のため int の範囲に収める
		kb = size / 1024;
		wsprintf(ret, message_get_res(IDS_STATUSBAR_KB), (kb > INT_MAX) ? INT_MAX : (int)kb);
	}
}

/*
 * statusbar_set_text - StatusBarのテキストを設定
 */
BOOL statusbar_set_text(const HWND hWnd, const HWND hStatusBar)
{
	HWND hTreeView = GetDlgItem(hWnd, ID_TREE);
	HWND hListView = GetDlgItem(GetDlgItem(hWnd, ID_CONTAINER), ID_LIST);
	HTREEITEM hItem;
	DATA_INFO *di;
	DATA_INFO *cdi;
	TCHAR buf[BUF_SIZE];
	TCHAR *str_hkey;
	ULONGLONG size;
	int cnt;

	// 初期化
	SendMessage(hStatusBar, SB_SETTEXT, (WPARAM)0 | 0, (LPARAM)TEXT(""));
	SendMessage(hStatusBar, SB_SETTEXT, (WPARAM)1 | 0, (LPARAM)TEXT(""));
	SendMessage(hStatusBar, SB_SETTEXT, (WPARAM)2 | 0, (LPARAM)TEXT(""));

	// 選択アイテムの取得
	if ((hItem = TreeView_GetSelection(hTreeView)) == NULL || hItem == clip_treeitem) {
		return FALSE;
	}
	if (hItem == history_treeitem || hItem == regist_treeitem) {
		cnt = ListView_GetItemCount(hListView);
		wsprintf(buf, message_get_res(IDS_STATUSBAR_ITEM_CNT), cnt);
		SendMessage(hStatusBar, SB_SETTEXT, (WPARAM)0 | 0, (LPARAM)buf);

		if (hItem == history_treeitem) {
			size = statusbar_get_allsize(history_data.child);
		} else {
			size = statusbar_get_allsize(regist_data.child);
		}
		statusbar_get_size_text(size, buf);
		SendMessage(hStatusBar, SB_SETTEXT, (WPARAM)1 | 0, (LPARAM)buf);
		return TRUE;
	}

	// データの取得
	if ((di = (DATA_INFO *)treeview_get_lparam(hTreeView, hItem)) == NULL) {
		return FALSE;
	}
	switch (di->type) {
	case TYPE_FOLDER:
		// フォルダ
		cnt = ListView_GetItemCount(hListView);
		wsprintf(buf, message_get_res(IDS_STATUSBAR_ITEM_CNT), cnt);
		SendMessage(hStatusBar, SB_SETTEXT, (WPARAM)0 | 0, (LPARAM)buf);

		size = statusbar_get_allsize(di->child);
		statusbar_get_size_text(size, buf);
		SendMessage(hStatusBar, SB_SETTEXT, (WPARAM)1 | 0, (LPARAM)buf);
		break;

	case TYPE_ITEM:
		// アイテム
		// 更新日時
		data_get_modified_string(di, buf);
		SendMessage(hStatusBar, SB_SETTEXT, (WPARAM)0 | 0, (LPARAM)buf);

		// 合計サイズ
		size = 0;
		for (cdi = di->child; cdi != NULL; cdi = cdi->next) {
			size += cdi->size;
		}
		statusbar_get_size_text(size, buf);
		SendMessage(hStatusBar, SB_SETTEXT, (WPARAM)1 | 0, (LPARAM)buf);

		if (di->window_name != NULL) {
			// ウィンドウタイトル
			SendMessage(hStatusBar, SB_SETTEXT, (WPARAM)2 | 0, (LPARAM)di->window_name);
		} else {
			// ホットキー
			if ((str_hkey = menu_get_keyname(di->op_modifiers, di->op_virtkey)) != NULL) {
				SendMessage(hStatusBar, SB_SETTEXT, (WPARAM)2 | 0, (LPARAM)str_hkey);
				mem_free(&str_hkey);
			}
		}
		break;

	case TYPE_DATA:
		// データ
		statusbar_get_size_text(di->size, buf);
		SendMessage(hStatusBar, SB_SETTEXT, (WPARAM)1 | 0, (LPARAM)buf);
		break;
	}
	return TRUE;
}

/*
 * statusbar_notify_proc - ステータスバーのツールチップの設定
 */
LRESULT statusbar_notify_proc(const HWND hStatusBar, LPARAM lParam)
{
	NMHDR *nmhdr = (NMHDR *)lParam;
	TOOLTIPTEXT *tt = (TOOLTIPTEXT *)lParam;
	static TCHAR tooltip_string[BUF_SIZE];
	TCHAR *tmp;
	int len;

	*tooltip_string = TEXT('\0');
	len = LOWORD(SendMessage(hStatusBar, SB_GETTEXTLENGTH, (WPARAM)nmhdr->idFrom, 0));
	if (len < BUF_SIZE) {
		SendMessage(hStatusBar, SB_GETTEXT, (WPARAM)nmhdr->idFrom, (LPARAM)tooltip_string);
	} else if ((tmp = mem_alloc(sizeof(TCHAR) * (len + 1))) != NULL) {
		// 収まらない場合は先頭の BUF_SIZE - 1 文字だけ使う
		SendMessage(hStatusBar, SB_GETTEXT, (WPARAM)nmhdr->idFrom, (LPARAM)tmp);
		lstrcpyn(tooltip_string, tmp, BUF_SIZE);
		mem_free(&tmp);
	}

	tt->hinst = NULL;
	tt->lpszText = tooltip_string;
	return TRUE;
}
/* End of source */
