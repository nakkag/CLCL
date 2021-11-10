/*
 * CLCL
 *
 * ListView.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_LISTVIEW_H
#define _INC_LISTVIEW_H

/* Include Files */
#include <commctrl.h>

/* Define */
#define WM_LV_EVENT						(WM_APP + 1301)

/* Struct */

/* Function Prototypes */
HWND listview_create(const HINSTANCE hInstance, const HWND hWnd, const int id, const HIMAGELIST icon_list);
void listview_close(const HWND hListView);
LRESULT listview_notify_proc(const HWND hWnd, const LPARAM lParam);
LRESULT listview_header_notify_proc(const HWND hListView, const HWND hTreeView, const LPARAM lParam);
BOOL listview_set_lparam(const HWND hListView, const int i, const LPARAM lParam);
LPARAM listview_get_lparam(const HWND hListView, const int i);
int listview_lparam_to_item(const HWND hListView, const LPARAM lParam);
BOOL listview_lparam_select(const HWND hListView, const LPARAM lParam);
int listview_get_icon(const HWND hListView, const int i);
int listview_get_hitest(const HWND hListView);
void listview_move_item(const HWND hListView, int index, const int Move);

#endif
/* End of source */
