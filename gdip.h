/*
 * CLCL
 *
 * gdip.h
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
#include <tchar.h>

#ifdef __cplusplus
extern "C" {
#endif

	/* Include Files */

	/* Define */

	/* Struct */

	/* Function Prototypes */
	void init_gdip();
	void shutdown_gdip();

	int save_jpeg(HBITMAP hBmp, LPCWSTR lpszFilename, ULONG uQuality);
	int save_png(HBITMAP hBmp, LPCWSTR lpszFilename);

	HBITMAP image_to_bitmap(HDC hdc, LPCWSTR lpszFilename);
#ifdef __cplusplus
}
#endif
/* End of source */
