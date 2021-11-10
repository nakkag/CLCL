/*
 * CLCL
 *
 * Window.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_WINDOW_H
#define _INC_WINDOW_H

/* Include Files */

/* Define */

/* Struct */
// ウィンドウフィルタ
typedef struct _WINDOW_FILTER_INFO {
	TCHAR *title;						// Window title
	TCHAR *class_name;					// Window class name

	int ignore;
	int focus;
	int paste;
} WINDOW_FILTER_INFO;

/* Function Prototypes */
BOOL window_ignore_check(const HWND hWnd);
BOOL window_focus_check(const HWND hWnd);
BOOL window_paste_check(const HWND hWnd);

#endif
/* End of source */
