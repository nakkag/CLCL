/*
 * CLCL
 *
 * Font.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_FONT_H
#define _INC_FONT_H

/* Include Files */

/* Define */

/* Struct */

/* Function Prototypes */
HFONT font_create(const TCHAR *FontName, const int FontSize, const int Charset, const int weight, const BOOL italic, const BOOL fixed);

#endif
/* End of source */
