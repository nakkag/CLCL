/*
 * CLCL
 *
 * ToolTip.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_TOOLTIP_H
#define _INC_TOOLTIP_H

/* Include Files */

/* Define */

/* Struct */

/* Function Prototypes */
BOOL tooltip_show(const HWND hToolTip, TCHAR *tip_text, const long x, const long y, const long top);
void tooltip_hide(const HWND hToolTip);
void tooltip_close(const HWND hToolTip);
BOOL tooltip_regist(const HINSTANCE hInstance);
HWND tooltip_create(const HINSTANCE hInstance);

#endif
/* End of source */
