/*
 * CLCL
 *
 * StatusBar.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_STATUSBAR_H
#define _INC_STATUSBAR_H

/* Include Files */

/* Define */

/* Struct */

/* Function Prototypes */
HWND statusbar_create(const HWND hWnd, const int id);
BOOL statusbar_set_text(const HWND hWnd, const HWND hStatusBar);
LRESULT statusbar_notify_proc(const HWND hStatusBar, LPARAM lParam);

#endif
/* End of source */
