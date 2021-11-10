/*
 * CLCL
 *
 * Viewer.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_VIEWER_H
#define _INC_VIEWER_H

/* Include Files */
#include <commctrl.h>

#include "Data.h"

/* Define */
#define ID_TREE							100
#define ID_CONTAINER					101
#define ID_LIST							102
#define ID_BINVIEW						103
#define ID_TOOLBAR						105
#define ID_STATUSBAR					104

#define WM_VIEWER_CHANGE_CLIPBOARD		(WM_APP + 1100)
#define WM_VIEWER_CHANGE_WATCH			(WM_APP + 1101)
#define WM_VIEWER_REFRESH_STATUS		(WM_APP + 1102)

#define WM_DRAGDROP						(WM_APP + 1110)
#define WM_GETDATA						(WM_APP + 1111)

#define WM_VIEWER_NOTIFY_CLOSE			(WM_APP + 1120)

/* Struct */

/* Function Prototypes */
HTREEITEM viewer_item_copy(const HWND hWnd, HTREEITEM from_item, HTREEITEM to_item, const BOOL move_flag, TCHAR *err_str);
void treeview_to_listview(const HWND hTreeView, const HTREEITEM parent_item, const HWND hListView);
void viewer_get_clipboard_data(const HWND hWnd, const HTREEITEM hItem);

BOOL viewer_regist(const HINSTANCE hInstance);
HWND viewer_create(const HWND pWnd, const int CmdShow);

#endif
/* End of source */
