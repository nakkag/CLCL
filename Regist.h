/*
 * CLCL
 *
 * Regist.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_REGIST_H
#define _INC_REGIST_H

/* Include Files */

/* Define */

/* Struct */

/* Function Prototypes */
DATA_INFO *regist_path_to_item(DATA_INFO *di, TCHAR *path);
DATA_INFO *regist_create_folder(DATA_INFO **root, const TCHAR *title, TCHAR *err_str);
DATA_INFO *regist_merge_item(DATA_INFO **root, const DATA_INFO *from_di, const BOOL move_flag, TCHAR *err_str);
BOOL regist_regist_hotkey(const HWND hWnd, DATA_INFO *di, int *id);
void regist_unregist_hotkey(const HWND hWnd, DATA_INFO *di);
DATA_INFO *regist_hotkey_to_item(DATA_INFO *di, const int id);

#endif
/* End of source */
