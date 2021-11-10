/*
 * CLCL
 *
 * Window.c
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE

#include "General.h"
#include "String.h"
#include "Ini.h"
#include "Window.h"

/* Define */

/* Global Variables */
// オプション
extern OPTION_INFO option;

/* ocal Function Prototypes */

/*
 * window_to_index - ウィンドウ情報のインデックスを取得
 */
static int window_to_index(const HWND hWnd)
{
	TCHAR title[BUF_SIZE];
	TCHAR class_name[BUF_SIZE];
	int i;

	if (option.window_filter_cnt <= 0) {
		return -1;
	}

	*title = TEXT('\0');
	GetWindowText(hWnd, title, BUF_SIZE - 1);
	*class_name = TEXT('\0');
	GetClassName(hWnd, class_name, BUF_SIZE - 1);

	for (i = 0; i < option.window_filter_cnt; i++) {
		// ウィンドウのタイトルとクラス名のチェック
		if (((option.window_filter_info + i)->title == NULL ||
			*(option.window_filter_info + i)->title == TEXT('\0') ||
			str_match((option.window_filter_info + i)->title, title) == TRUE) &&

			((option.window_filter_info + i)->class_name == NULL ||
			*(option.window_filter_info + i)->class_name == TEXT('\0') ||
			str_match((option.window_filter_info + i)->class_name, class_name) == TRUE)) {
			return i;
		}
	}
	return -1;
}

/*
 * window_ignore_check - 除外ウィンドウのチェック
 */
BOOL window_ignore_check(const HWND hWnd)
{
	int i;

	if ((i = window_to_index(hWnd)) == -1) {
		return TRUE;
	}
	return (((option.window_filter_info + i)->ignore == 0) ? TRUE : FALSE);
}

/*
 * window_focus_check - フォーカス設定のチェック
 */
BOOL window_focus_check(const HWND hWnd)
{
	int i;

	if ((i = window_to_index(hWnd)) == -1) {
		return TRUE;
	}
	return (((option.window_filter_info + i)->focus == 0) ? FALSE : TRUE);
}

/*
 * window_paste_check - 貼り付けのチェック
 */
BOOL window_paste_check(const HWND hWnd)
{
	int i;

	if ((i = window_to_index(hWnd)) == -1) {
		return FALSE;
	}
	return (((option.window_filter_info + i)->paste == 0) ? FALSE : TRUE);
}
/* End of source */
