/*
 * CLCL
 *
 * dpi.h
  *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
*/

#pragma once

 /* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE

/* Define */

/* Struct */
typedef enum PROCESS_DPI_AWARENESS {
	PROCESS_DPI_UNAWARE = 0,
	PROCESS_SYSTEM_DPI_AWARE = 1,
	PROCESS_PER_MONITOR_DPI_AWARE = 2
} PROCESS_DPI_AWARENESS;

/* Function Prototypes */
int Scale(int x);
int UnScale(int x);
UINT GetScale();
void SetScale(UINT iDPI);
PROCESS_DPI_AWARENESS GetAwareness();
void SetAwareness(PROCESS_DPI_AWARENESS awareness);
void ScaleRect(RECT* pRect);
void UnScaleRect(RECT* pRect);
void ScalePoint(POINT* pPoint);
void UnScalePoint(POINT* pPoint);
void InitDpi();
/* End of source */
