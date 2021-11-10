/*
 * CLCL
 *
 * fmt_bitmap_view.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_FMT_BITMAP_VIEW_H
#define _INC_FMT_BITMAP_VIEW_H

/* Include Files */

/* Define */
#define WM_SET_BMPDATA					(WM_APP + 1)
#define WM_SET_STRETCH_MODE				(WM_APP + 2)

/* Struct */

/* Function Prototypes */
BOOL bmpview_regist(const HINSTANCE hInstance);
HWND bmpview_create(const HINSTANCE hInstance, const HWND pWnd, int id);

#endif
/* End of source */
