/*
 * CLCL
 *
 * DarkMode.h
 *
 * Copyright (C) 1996-2026 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_DARKMODE_H
#define _INC_DARKMODE_H

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE

/* Define */
// メニューバーの描画に使用する非公開メッセージ
#ifndef WM_UAHDRAWMENU
#define WM_UAHDRAWMENU					0x0091
#define WM_UAHDRAWMENUITEM				0x0092
#endif

/* Struct */

/* Function Prototypes */
void dark_mode_init(void);
void dark_mode_free(void);
BOOL dark_mode_is_dark(void);
BOOL dark_mode_update(void);
BOOL dark_mode_is_color_change(const UINT msg, const LPARAM lParam);

COLORREF dark_mode_get_color(const int index);
COLORREF dark_mode_get_accent_color(void);
HBRUSH dark_mode_get_brush(const int index);

void dark_mode_set_window(const HWND hWnd);
void dark_mode_set_control(const HWND hWnd);
void dark_mode_set_children(const HWND hWnd);
void dark_mode_set_dialog(const HWND hDlg);
void dark_mode_set_tab(const HWND hTab);
void dark_mode_set_statusbar(const HWND hStatusBar);
void dark_mode_refresh_window(const HWND hWnd);

BOOL dark_mode_ctl_color(const UINT msg, const WPARAM wParam, const LPARAM lParam, HBRUSH *ret);
BOOL dark_mode_menubar_message(const HWND hWnd, const UINT msg, const WPARAM wParam, const LPARAM lParam, LRESULT *ret);
void dark_mode_draw_arrow_button(const DRAWITEMSTRUCT *ds, const UINT type);
BOOL dark_mode_toolbar_customdraw(const LPARAM lParam, LRESULT *ret);

#endif
/* End of source */
