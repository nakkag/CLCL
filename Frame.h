/*
 * CLCL
 *
 * Frame.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_FRAME_H
#define _INC_FRAME_H

/* Include Files */
#include "dpi.h"

/* Define */
#define FRAME_CNT						Scale(2)				// ã´äEÉtÉåÅ[ÉÄêî

/* Struct */

/* Function Prototypes */
BOOL frame_initialize(const HWND hWnd);
void frame_free(void);
int frame_draw(const HWND hWnd, const HWND hTreeView);
int frame_draw_end(const HWND hWnd);

#endif
/* End of source */
