/*
 * CLCL
 *
 * ViewerOLEDnD.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_VIEWER_OLEDND_H
#define _INC_VIEWER_OLEDND_H

/* Include Files */

/* Define */

/* Struct */

/* Function Prototypes */
BOOL viewer_ole_get_drag_effect(const HWND hWnd, const LPIDROPTARGET_NOTIFY pdtn);
BOOL viewer_ole_create_drop_item(const HWND hWnd, const LPIDROPTARGET_NOTIFY pdtn, const BOOL keystate);
BOOL viewer_ole_start_drag(const HWND hWnd, const HTREEITEM sel_item);
HANDLE viewer_ole_get_drag_data(const HWND hWnd, const UINT format);

#endif
/* End of source */
