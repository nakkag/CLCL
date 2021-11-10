/*
 * CLCL
 *
 * Bitmap.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_BITMAP_H
#define _INC_BITMAP_H

/* Include Files */

/* Define */

/* Struct */

/* Function Prototypes */
BYTE *bitmap_to_dib(const HBITMAP hbmp, DWORD *size);
HBITMAP dib_to_bitmap(const BYTE *dib);

#endif
/* End of source */
