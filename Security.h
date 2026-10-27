/*
 * CLCL
 *
 * Security.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_SECURITY_H
#define _INC_SECURITY_H

/* Include Files */

/* Define */
// プラグインの初期化でエラーメッセージに使用するバッファのサイズ
#define SECURITY_ERR_SIZE				(BUF_SIZE + MAX_PATH)

/* Struct */

/* Function Prototypes */
void security_initialize(void);
BOOL security_is_elevated(void);
BOOL path_join(TCHAR *buf, const int size, const TCHAR *dir, const TCHAR *name);
HMODULE security_load_plugin(const TCHAR *lib_path, BOOL *blocked);
void security_add_blocked(TCHAR *list, const int size, const TCHAR *lib_path);
void security_get_blocked_message(TCHAR *err_str, const int size, const TCHAR *list);
HMODULE security_load_plugin_ui(const HWND hWnd, const TCHAR *lib_path);
BOOL security_is_internal_message(const UINT msg);
void security_trust_begin(void);
void security_trust_end(void);
BOOL security_is_foreign_send(void);
BOOL security_is_foreign_message(const UINT msg);
LRESULT security_call_window_proc(const WNDPROC proc, const HWND hWnd, const UINT msg, const WPARAM wParam, const LPARAM lParam);
#ifndef OPTION_SET
BOOL security_shell_open_as_user(const TCHAR *file_name, const TCHAR *command_line);
#endif	// OPTION_SET

#endif
/* End of source */
