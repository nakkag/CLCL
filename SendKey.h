/*
 * CLCL
 *
 * SendKey.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_SENDKEY_H
#define _INC_SENDKEY_H

/* Include Files */

/* Define */
#define DEFAULT_COPY_WAIT				100
#define DEFAULT_PASTE_WAIT				100

/* Struct */
typedef struct _SENDKEY_INFO {
	TCHAR *title;						// Window title
	TCHAR *class_name;					// Window class name

	UINT copy_modifiers;
	UINT copy_virtkey;
	int copy_wait;

	UINT paste_modifiers;
	UINT paste_virtkey;
	int paste_wait;
} SENDKEY_INFO;

/* Function Prototypes */
BOOL sendkey_copy(const HWND hWnd);
BOOL sendkey_paste(const HWND hWnd);

#endif
/* End of source */
