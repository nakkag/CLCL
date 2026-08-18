/*
 * CLCL
 *
 * Menu.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_MENU_H
#define _INC_MENU_H

/* Include Files */
#include "Data.h"
#include "Tool.h"

/* Define */
#define ID_MENUITEM_DATA				50000

#define MENU_CONTENT_SEPARATOR			0
#define MENU_CONTENT_HISTORY			1
#define MENU_CONTENT_HISTORY_DESC		2
#define MENU_CONTENT_REGIST				3
#define MENU_CONTENT_REGIST_DESC		4
#define MENU_CONTENT_POPUP				5
#define MENU_CONTENT_VIEWER				6
#define MENU_CONTENT_OPTION				7
#define MENU_CONTENT_CLIPBOARD_WATCH	8
#define MENU_CONTENT_TOOL				9
#define MENU_CONTENT_APP				10
#define MENU_CONTENT_CANCEL				11
#define MENU_CONTENT_EXIT				12
#define MENU_CONTENT_HELP				13

/* Struct */
// menu item
typedef struct _MENU_ITEM_INFO {
	UINT id;						// メニューID         / MenuID
	UINT flag;						// メニューフラグ     / MenuFlag
	LPCTSTR item;					// メニュー項目の内容 / Menu item content

	TCHAR *text;					// メニューに表示するテキスト / menu item text
	int text_x;						// テキストの位置             / text position
	int text_y;

	HICON icon;						// メニューに表示するアイコン / menu item icon
	BOOL free_icon;

	TCHAR *hkey;

	BOOL show_format;				// 形式表示           / whether to show format
	BOOL show_bitmap;				// ビットマップ表示   / wether to show bitmap

	DATA_INFO *set_di;				// データ情報         / data information
	DATA_INFO *show_di;				// 表示するデータ情報 / data information to display

	TOOL_INFO *ti;					// ツール情報         / tool information

	struct _MENU_INFO *mi;			// 元となるMENU_INFO構造体  / original MENU_INFO structure

	// popup
	struct _MENU_ITEM_INFO *mii;	// ポップアップメニューの子アイテム     / Popup menu child items
	int mii_cnt;					// ポップアップメニューの子アイテムの数 / Number of child items in popup menu
} MENU_ITEM_INFO;

// menu info
typedef struct _MENU_INFO {
	// MENU_CONTENT_
	int content;
	// メニューに表示するタイトル 
	// menu title 
	TCHAR *title;

	// icon
	// メニューに表示するアイコンのパス (空の場合は本体)
	// Path of icon to be displayed in menu (body if empty)
	TCHAR *icon_path;
	// メニューに表示するアイコンのインデックス
	// index of the icon to be displayed in the menu
	int icon_index;

	// content
	// パス (MENU_CONTENT_REGIST, MENU_CONTENT_APP)
	// Path (MENU_CONTENT_REGIST, MENU_CONTENT_APP)
	TCHAR *path;
	// コマンドライン (MENU_CONTENT_APP)
	// Command line (MENU_CONTENT_APP) 
	TCHAR *cmd;

	// 履歴の表示開始値 (MENU_CONTENT_HISTORY)
	// Value of first history display (MENU_CONTENT_HISTORY)
	int min;
	// 履歴の表示終了値 (MENU_CONTENT_HISTORY)
	// Value of last history display (MENU_CONTENT_HISTORY)
	int max;

	// popup
	// ポップアップメニューの子アイテム (MENU_CONTENT_POPUP)
	// Popup menu child items (MENU_CONTENT_POPUP)
	struct _MENU_INFO *mi;
	// ポップアップメニューの子アイテムの数
	// Number of child items in the popup menu
	int mi_cnt;
} MENU_INFO;

/* Function Prototypes */
void menu_free(void);
int menu_show(const HWND hWnd, const HMENU hMenu, const POINT *mpos);
MENU_ITEM_INFO *menu_get_info(const UINT id);
TCHAR *menu_get_keyname(const UINT modifiers, const UINT virtkey);
HMENU menu_create(const HWND hWnd, MENU_INFO *menu_info, const int menu_cnt,
				  DATA_INFO *history_di, DATA_INFO *regist_di);
void menu_destory(HMENU hMenu);
BOOL menu_set_drawitem(MEASUREITEMSTRUCT *ms);
BOOL menu_drawitem(const DRAWITEMSTRUCT *ds);
LRESULT menu_accelerator(const HMENU hMenu, const TCHAR key);

#endif
/* End of source */
