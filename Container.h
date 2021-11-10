/*
 * CLCL
 *
 * Container.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_CONTAINER_H
#define _INC_CONTAINER_H

/* Include Files */

/* Define */
#define WM_ALLHIDE						(WM_APP + 1)

/* Struct */

/* Function Prototypes */
BOOL container_regist(const HINSTANCE hInstance);
HWND container_create(const HINSTANCE hInstance, const HWND pWnd, int id);

#endif
/* End of source */
