/*
 * CLCL
 *
 * TreeView.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_TREEVIEW_H
#define _INC_TREEVIEW_H

/* Include Files */
#include <commctrl.h>

#include "Data.h"

/* Define */
#define WM_TV_EVENT						(WM_APP + 1300)

/* Struct */

/* Function Prototypes */
HWND treeview_create(const HINSTANCE hInstance, const HWND hWnd, const int id, const HIMAGELIST icon_list);
BOOL treeview_set_init_item(const HWND hTreeView);
void treeview_close(const HWND hTreeView);
LRESULT treeview_notify_proc(const HWND hWnd, LPARAM lParam);
void treeview_scroll(const HWND hTreeView);
HTREEITEM treeview_set_item(const HWND hTreeView, TCHAR *buf, const HTREEITEM hParent, const HTREEITEM After,
							const int icon, const int select_icon, LPARAM lParam);
void treeview_delete_child(const HWND hTreeView, const HTREEITEM parent_item);
BOOL treeview_set_text(const HWND hTreeView, const HTREEITEM hItem, TCHAR *text);
BOOL treeview_get_text(const HWND hTreeView, const HTREEITEM hItem, TCHAR *text);
BOOL treeview_set_icon(const HWND hTreeView, const HTREEITEM hItem, const int icon, const int select_icon);
int treeview_get_icon(const HWND hTreeView, const HTREEITEM hItem);
BOOL treeview_set_lparam(const HWND hTreeView, const HTREEITEM hItem, const LPARAM lParam);
LPARAM treeview_get_lparam(const HWND hTreeView, const HTREEITEM hItem);
HTREEITEM treeview_lparam_to_item(const HWND hTreeView, const HTREEITEM hParent, const LPARAM lParam);
HTREEITEM treeview_get_rootitem(const HWND hTreeView, const HTREEITEM hItem);
HTREEITEM treeview_get_hitest(const HWND hTreeView);
HTREEITEM treeview_copy_item(const HWND hTreeView, const HTREEITEM parent_item, const HTREEITEM hItem, const HTREEITEM After);
HTREEITEM treeview_datainfo_to_treeitem(const HWND hTreeView, const HTREEITEM parent_item, DATA_INFO *di);
BOOL treeview_sync_datainfo(const HWND hTreeView, const HTREEITEM parent_item, DATA_INFO *di);
BOOL treeview_title_refresh(const HWND hTreeView, HTREEITEM hItem);
BOOL treeview_delete_item(const HWND hTreeView, const HTREEITEM hItem);
HTREEITEM treeview_move_up(const HWND hTreeView, const HTREEITEM hItem);
HTREEITEM treeview_move_down(const HWND hTreeView, const HTREEITEM hItem);

#endif
/* End of source */
