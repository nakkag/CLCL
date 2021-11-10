/*
 * CLCL
 *
 * General.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_CLCL_GENERAL_H
#define _INC_CLCL_GENERAL_H

/* Include Files */
#ifdef OP_XP_STYLE
#include <uxtheme.h>
#include <vssym32.h>
#endif	// OP_XP_STYLE

/* Define */
#define BUF_SIZE						256

#define APP_NAME						TEXT("CLCL")
#define APP_VAR							210

#define MAIN_WND_CLASS					TEXT("CLCLMain")
#define MAIN_WINDOW_TITLE				APP_NAME

#define DEFAULT_USER					TEXT("DEFAULT")
#define GENERAL_INI						TEXT("general.ini")
#define APP_INI							TEXT("clcl_app.ini")
#define USER_INI						TEXT("clcl.ini")
#define HISTORY_FILENAME				TEXT("history.dat")
#define REGIST_FILENAME					TEXT("regist.dat")

#define MAIN_EXE						TEXT("CLCL.exe")
#define OPTION_EXE						TEXT("CLCLSet.exe")
#define HOOK_LIB						TEXT("CLCLHook.dll")

#define HKEY_ID							0x0300			// ホットキーID

// general
#define WM_GET_VERSION					(WM_APP + 100)
#define WM_GET_WORKPATH					(WM_APP + 101)
#define WM_GET_CLIPBOARD_WATCH			(WM_APP + 102)
#define WM_SET_CLIPBOARD_WATCH			(WM_APP + 103)
#define WM_GET_FORMAT_ICON				(WM_APP + 104)
#define WM_ENABLE_ACCELERATOR			(WM_APP + 105)
#define WM_REGIST_HOTKEY				(WM_APP + 106)
#define WM_UNREGIST_HOTKEY				(WM_APP + 107)
// option
#define WM_OPTION_SHOW					(WM_APP + 200)
#define WM_OPTION_GET					(WM_APP + 201)
#define WM_OPTION_LOAD					(WM_APP + 202)
#define WM_OPTION_SAVE					(WM_APP + 203)
// history & regist
#define WM_HISTORY_CHANGED				(WM_APP + 300)
#define WM_HISTORY_GET_ROOT				(WM_APP + 301)
#define WM_HISTORY_LOAD					(WM_APP + 302)
#define WM_HISTORY_SAVE					(WM_APP + 303)
#define WM_REGIST_CHANGED				(WM_APP + 350)
#define WM_REGIST_GET_ROOT				(WM_APP + 351)
#define WM_REGIST_LOAD					(WM_APP + 352)
#define WM_REGIST_SAVE					(WM_APP + 353)
// item
#define WM_ITEM_TO_CLIPBOARD			(WM_APP + 400)
#define WM_ITEM_CREATE					(WM_APP + 401)
#define WM_ITEM_COPY					(WM_APP + 402)
#define WM_ITEM_FREE					(WM_APP + 403)
#define WM_ITEM_FREE_DATA				(WM_APP + 404)
#define WM_ITEM_CHECK					(WM_APP + 405)
#define WM_ITEM_TO_BYTES				(WM_APP + 406)
#define WM_ITEM_FROM_BYTES				(WM_APP + 407)
#define WM_ITEM_TO_FILE					(WM_APP + 408)
#define WM_ITEM_FROM_FILE				(WM_APP + 409)
#define WM_ITEM_GET_PARENT				(WM_APP + 410)
#define WM_ITEM_GET_FORMAT_TO_ITEM		(WM_APP + 411)
#define WM_ITEM_GET_PRIORITY_HIGHEST	(WM_APP + 412)
#define WM_ITEM_GET_TITLE				(WM_APP + 413)
#define WM_ITEM_GET_OPEN_INFO			(WM_APP + 414)
#define WM_ITEM_GET_SAVE_INFO			(WM_APP + 415)
// viewer
#define WM_VIEWER_SHOW					(WM_APP + 500)
#define WM_VIEWER_GET_HWND				(WM_APP + 501)
#define WM_VIEWER_GET_MAIN_HWND			(WM_APP + 504)
#define WM_VIEWER_GET_SELECTION			(WM_APP + 502)
#define WM_VIEWER_SELECT_ITEM			(WM_APP + 503)

/* Struct */

/* Function Prototypes */
#ifdef OP_XP_STYLE
HTHEME theme_open(const HWND hWnd);
void theme_close(const HTHEME hTheme);
void theme_free(void);
BOOL theme_draw(const HWND hWnd, const HRGN draw_hrgn, const HTHEME hTheme);
#endif	// OP_XP_STYLE

BOOL _SetForegroundWindow(const HWND hWnd);

#endif
/* End of source */
