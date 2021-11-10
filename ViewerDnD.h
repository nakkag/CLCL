/*
 * CLCL
 *
 * ViewerDnD.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_VIEWER_DND_H
#define _INC_VIEWER_DND_H

/* Include Files */

/* Define */
#define DRAG_MODE_NONE					0
#define DRAG_MODE_COPY					1
#define DRAG_MODE_MOVE					2

/* Struct */

/* Function Prototypes */
int dragdrop_show_menu(const HWND hWnd, const int mode, const BOOL def_move);
HTREEITEM dragdrop_get_drop_folder(const HWND hWnd, const int x, const int y);
BOOL dragdrop_start_drag(const HWND hWnd, const HTREEITEM sel_item);
int dragdrop_set_drag_item(const HWND hWnd);
BOOL dragdrop_drop_item(const HWND hWnd, const UINT msg);

#endif
/* End of source */
