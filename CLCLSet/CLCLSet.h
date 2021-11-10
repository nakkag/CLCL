/*
 * CLCLSet
 *
 * CLCLSet.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_CLCLSET_H
#define _INC_CLCLSET_H

/* Include Files */

/* Define */
#define WINDOW_TITLE					TEXT("CLCLSet")

#define IDPCANCEL						(WM_APP + 1000)
#define WM_LV_EVENT						(WM_APP + 1001)

#ifndef WM_THEMECHANGED
#define WM_THEMECHANGED					0x031A
#endif

/* Struct */

/* Function Prototypes */
#ifdef OP_XP_STYLE
long open_theme(const HWND hWnd, const WCHAR *class_name);
void close_theme(long hTheme);
BOOL draw_theme_scroll(LPDRAWITEMSTRUCT lpDrawItem, UINT i, long hTheme);
#endif	// OP_XP_STYLE
void alloc_get_text(const HWND hEdit, TCHAR **buf);
int file_select(const HWND hDlg, const TCHAR *oFilter, const int Index, TCHAR *ret);
void draw_scroll_sontrol(LPDRAWITEMSTRUCT lpDrawItem, UINT i);
BOOL CALLBACK enum_windows_proc(const HWND hWnd, const LPARAM lParam);
BOOL listview_set_lparam(const HWND hListView, const int i, const LPARAM lParam);
LPARAM listview_get_lparam(const HWND hListView, const int i);
void listview_move_item(const HWND hListView, int index, const int Move);
LRESULT listview_notify_proc(const HWND hWnd, const LPARAM lParam, const HWND hListView);
LRESULT listview_header_notify_proc(const HWND hListView, const LPARAM lParam);
void get_keyname(const UINT modifiers, const UINT virtkey, TCHAR *ret);
LRESULT OptionNotifyProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif
/* End of source */
