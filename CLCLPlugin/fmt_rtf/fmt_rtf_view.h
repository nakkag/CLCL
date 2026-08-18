/*
 * CLCL
 *
 * fmt_rtf_view.h
 *
 * Copyright (C) 1996-2003 by Nakashima Tomoaki. All rights reserved.
 *		http://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_FMT_RTF_VIEW_H
#define _INC_FMT_RTF_VIEW_H

/* Include Files */

/* Define */
#define WM_SETRTF				(WM_APP + 1)
#define WM_GETRTF				(WM_APP + 2)

/* Struct */

/* Function Prototypes */
BOOL rtfview_regist(const HINSTANCE hInstance);
HWND rtfview_create(const HINSTANCE hInstance, const HWND pWnd, int id);

#endif
/* End of source */
