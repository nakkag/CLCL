/*
 * CLCL
 *
 * BinView.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_BINVIEW_H
#define _INC_BINVIEW_H

/* Include Files */

/* Define */
#define WM_SET_BINDATA					(WM_APP + 1)
#define WM_SAVE_BINDATA					(WM_APP + 2)

/* Struct */

/* Function Prototypes */
BOOL binview_regist(const HINSTANCE hInstance);
HWND binview_create(const HINSTANCE hInstance, const HWND pWnd, int id);

#endif
/* End of source */
