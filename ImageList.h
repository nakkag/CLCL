/*
 * CLCL
 *
 * ImageList.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_IMAGELIST_H
#define _INC_IMAGELIST_H

/* Include Files */
#include <commctrl.h>

/* Define */
#define ICON_CLIPBOARD					0
#define ICON_HISTORY					1
#define ICON_REGIST						2
#define ICON_FOLDER						3
#define ICON_FOLDER_OPEN				4
#define ICON_NO_FORMAT					5
#define ICON_FORMAT						6

/* Struct */

/* Function Prototypes */
HIMAGELIST create_imagelist(const HINSTANCE hInstance);

#endif
/* End of source */
