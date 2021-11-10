/*
 * CLCL
 *
 * fmt_file_view.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_FMT_FILE_VIEW_H
#define _INC_FMT_FILE_VIEW_H

/* Include Files */

/* Define */
#define WM_SET_FILEDATA					(WM_APP + 1)
#define WM_GET_FILEDATA					(WM_APP + 2)

/* Struct */

/* Function Prototypes */
HDROP create_dropfile(const TCHAR **FileName, const int cnt, DWORD *ret_size);
BOOL fileview_regist(const HINSTANCE hInstance);
HWND fileview_create(const HINSTANCE hInstance, const HWND pWnd, int id);

#endif
/* End of source */
