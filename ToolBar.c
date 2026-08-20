/*
 * CLCL
 *
 * ToolBar.c
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
#include "dpi.h"

#include "resource.h"

/* Define */
#define BITMAP_CNT						7

#define TOOLBAR_INDENT					Scale(5)
// ツールバーのビットマップの基準サイズ
#define TOOLBAR_BITMAP_SIZE				16

/* Global Variables */
static TBBUTTON tbb[] = {
	{0,					ID_MENUITEM_CLIPBOARD_TB,		TBSTATE_ENABLED,	TBSTYLE_BUTTON, 0, 0},
	{1,					ID_MENUITEM_PASTE_TB,			TBSTATE_ENABLED,	TBSTYLE_BUTTON, 0, 0},
	{2,					ID_MENUITEM_REGIST_ADD_TB,		TBSTATE_ENABLED,	TBSTYLE_BUTTON, 0, 0},
	{0,					0,								0,					TBSTYLE_SEP, 0, 0},
	{3,					ID_MENUITEM_NEW_ITEM_TB,		TBSTATE_ENABLED,	TBSTYLE_BUTTON, 0, 0},
	{4,					ID_MENUITEM_DELETE_TB,			TBSTATE_ENABLED,	TBSTYLE_BUTTON, 0, 0},
	{0,					0,								0,					TBSTYLE_SEP, 0, 0},
	{5,					ID_MENUITEM_UP_TB,				TBSTATE_ENABLED,	TBSTYLE_BUTTON, 0, 0},
	{6,					ID_MENUITEM_DOWN_TB,			TBSTATE_ENABLED,	TBSTYLE_BUTTON, 0, 0},
};

// extern
extern HINSTANCE hInst;

// オプション
extern OPTION_INFO option;

/* Local Function Prototypes */

/*
 * toolbar_create - StatusBarの作成
 */
HWND toolbar_create(const HWND hWnd, const int id)
{
	HWND hToolBar;
	int bmp_size;
	int bmp_id;

	// ビットマップの選択
	bmp_size = Scale(TOOLBAR_BITMAP_SIZE);
	if (bmp_size >= 40) {
		bmp_size = 48;
		bmp_id = IDR_TOOLBAR48;
	} else if (bmp_size >= 24) {
		bmp_size = 32;
		bmp_id = IDR_TOOLBAR32;
	} else {
		bmp_size = 16;
		bmp_id = IDR_TOOLBAR;
	}
	hToolBar = CreateToolbarEx(hWnd, WS_CHILD | TBSTYLE_TOOLTIPS,
		id, BITMAP_CNT, hInst, bmp_id, tbb, sizeof(tbb) / sizeof(TBBUTTON), 0, 0,
		bmp_size, bmp_size, sizeof(TBBUTTON));
	if (hToolBar == NULL) {
		return NULL;
	}

	SetWindowLong(hToolBar, GWL_STYLE, GetWindowLong(hToolBar, GWL_STYLE) | TBSTYLE_FLAT);
	SendMessage(hToolBar, TB_SETINDENT, TOOLBAR_INDENT, 0);

	if (option.viewer_show_toolbar == 1) {
		ShowWindow(hToolBar, SW_SHOW);
	}
	return hToolBar;
}
/* End of source */
