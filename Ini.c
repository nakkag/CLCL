/*
 * CLCL
 *
 * Ini.c
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
#include "Memory.h"
#include "String.h"
#include "Data.h"
#include "Profile.h"
#include "Ini.h"
#include "Message.h"
#include "File.h"
#include "Menu.h"
#include "Format.h"
#include "SendKey.h"
#include "dpi.h"

#include "resource.h"

/* Define */
#define USER_INI_OLD					TEXT("user.ini")

/* Global Variables */
OPTION_INFO option;

extern TCHAR work_path[];

/* Local Function Prototypes */
static FORMAT_NAME *ini_get_format_name(TCHAR *format_name, int *cnt);
static BOOL ini_get_menu(const TCHAR *ini_path, const TCHAR *menu_path, MENU_INFO *mi, const int mcnt, TCHAR *err_str);
static BOOL ini_put_menu(const TCHAR *ini_path, const TCHAR *menu_path, MENU_INFO *mi, const int mcnt);

/*
 * ini_get_format_name - 形式名の取得
 */
static FORMAT_NAME *ini_get_format_name(TCHAR *format_name, int *cnt)
{
	FORMAT_NAME *ret;
	TCHAR buf[BUF_SIZE];
	TCHAR *p, *r;
	int i;

	// 項目数の取得
	p = format_name;
	*cnt = 0;
	while (1) {
		if (*p == TEXT('\0') || *p == TEXT(',')) {
			if (*p == TEXT(',') && *(p + 1) == TEXT(',')) {
				p += 2;
				continue;
			}
			(*cnt)++;
			if (*p == TEXT('\0') || *(p++) == TEXT('\0')) {
				break;
			}
		} else {
			p++;
		}
	}

	// 確保
	if ((ret = mem_calloc(sizeof(FORMAT_NAME) * (*cnt))) == NULL) {
		*cnt = 0;
		return NULL;
	}

	// 項目を切り出す
	p = format_name;
	r = buf;
	i = 0;
	while (1) {
		if (*p == TEXT('\0') || *p == TEXT(',')) {
			if (*p == TEXT(',') && *(p + 1) == TEXT(',')) {
				*(r++) = *(p++);
				p++;
				continue;
			}
			*r = TEXT('\0');
			(ret + i)->format_name = alloc_copy(buf);
			Trim((ret + i)->format_name);
			(ret + i)->format_name_hash = str2hash((ret + i)->format_name);
			if (*p == TEXT('\0') || *(p++) == TEXT('\0')) {
				break;
			}
			r = buf;
			i++;
		} else {
			*(r++) = *(p++);
		}
	}
	return ret;
}

/*
 * ini_get_option - メニューオプションを取得
 */
static BOOL ini_get_menu(const TCHAR *ini_path, const TCHAR *menu_path, MENU_INFO *mi, const int mcnt, TCHAR *err_str)
{
	TCHAR buf[BUF_SIZE];
	int i;

	for (i = 0; i < mcnt; i++) {
		wsprintf(buf, TEXT("%s-content-%d"), menu_path, i);
		(mi + i)->content = profile_get_int(TEXT("action"), buf, 0, ini_path);
		wsprintf(buf, TEXT("%s-title-%d"), menu_path, i);
		(mi + i)->title = profile_alloc_string(TEXT("action"), buf, TEXT(""), ini_path);
		wsprintf(buf, TEXT("%s-icon_path-%d"), menu_path, i);
		(mi + i)->icon_path = profile_alloc_string(TEXT("action"), buf, TEXT(""), ini_path);
		wsprintf(buf, TEXT("%s-icon_index-%d"), menu_path, i);
		(mi + i)->icon_index = profile_get_int(TEXT("action"), buf, 0, ini_path);
		wsprintf(buf, TEXT("%s-path-%d"), menu_path, i);
		(mi + i)->path = profile_alloc_string(TEXT("action"), buf, TEXT(""), ini_path);
		wsprintf(buf, TEXT("%s-cmd-%d"), menu_path, i);
		(mi + i)->cmd = profile_alloc_string(TEXT("action"), buf, TEXT(""), ini_path);
		wsprintf(buf, TEXT("%s-min-%d"), menu_path, i);
		(mi + i)->min = profile_get_int(TEXT("action"), buf, 0, ini_path);
		wsprintf(buf, TEXT("%s-max-%d"), menu_path, i);
		(mi + i)->max = profile_get_int(TEXT("action"), buf, 0, ini_path);
		wsprintf(buf, TEXT("%s-mi_cnt-%d"), menu_path, i);
		(mi + i)->mi_cnt = profile_get_int(TEXT("action"), buf, 0, ini_path);

		if ((mi + i)->mi_cnt > 0) {
			if (((mi + i)->mi = mem_calloc(sizeof(MENU_INFO) * (mi + i)->mi_cnt)) == NULL) {
				message_get_error(GetLastError(), err_str);
				return FALSE;
			}
			wsprintf(buf, TEXT("%s-menu_info-%d"), menu_path, i);
			if (ini_get_menu(ini_path, buf, (mi + i)->mi, (mi + i)->mi_cnt, err_str) == FALSE) {
				return FALSE;
			}
		}
	}
	return TRUE;
}

/*
 * ini_get_option - オプションを取得
 */
BOOL ini_get_option(TCHAR *err_str)
{
	HDC hdc;
	RECT desktop_rect;
	TCHAR ini_path[MAX_PATH];
	TCHAR ini_path_old[MAX_PATH];
	TCHAR buf[BUF_SIZE];
	int char_set;
	int i;
	int version;

	hdc = GetDC(NULL);
	char_set = GetTextCharset(hdc);
	ReleaseDC(NULL, hdc);

	wsprintf(ini_path, TEXT("%s\\%s"), work_path, USER_INI);
	wsprintf(ini_path_old, TEXT("%s\\%s"), work_path, USER_INI_OLD);
	if (file_check_file(ini_path) == FALSE && file_check_file(ini_path_old) == TRUE) {
		// INIファイル名変更 (Ver 1.0.8)
		MoveFile(ini_path_old, ini_path);
	}
	profile_initialize(ini_path, TRUE);

	// main
	version = profile_get_int(TEXT("main"), TEXT("version"), 0, ini_path);
	option.main_clipboard_watch = profile_get_int(TEXT("main"), TEXT("clipboard_watch"), 1, ini_path);
	option.main_clipboard_rechain_minute = profile_get_int(TEXT("main"), TEXT("clipboard_rechain_minute"), 1, ini_path);
	option.main_show_trayicon = profile_get_int(TEXT("main"), TEXT("show_trayicon"), 1, ini_path);
	option.main_show_viewer = profile_get_int(TEXT("main"), TEXT("show_viewer"), 0, ini_path);

	// data
	option.data_date_format = profile_alloc_string(TEXT("data"), TEXT("date_format"), TEXT(""), ini_path);
	option.data_time_format = profile_alloc_string(TEXT("data"), TEXT("time_format"), TEXT(""), ini_path);

	// history
	option.history_add_interval = profile_get_int(TEXT("history"), TEXT("add_interval"), 1, ini_path);
	option.history_save = profile_get_int(TEXT("history"), TEXT("save"), 1, ini_path);
	option.history_always_save = profile_get_int(TEXT("history"), TEXT("always_save"), 0, ini_path);
	option.history_delete = profile_get_int(TEXT("history"), TEXT("delete"), 1, ini_path);
	option.history_max = profile_get_int(TEXT("history"), TEXT("max"), 30, ini_path);
	if (option.history_max <= 0) {
		option.history_max = 30;
	}
	option.history_overlap_check = profile_get_int(TEXT("history"), TEXT("overlap_check"), 1, ini_path);
	option.history_ignore_regist_item = profile_get_int(TEXT("history"), TEXT("ignore_regist_item"), 0, ini_path);

	// menu
	option.menu_text_format = profile_alloc_string(TEXT("menu"), TEXT("text_format"), TEXT("&%1d. %t"), ini_path);
	option.menu_intact_item_title = profile_get_int(TEXT("menu"), TEXT("intact_item_title"), 0, ini_path);
	option.menu_text_margin_left = Scale(profile_get_int(TEXT("menu"), TEXT("text_margin_left"), 4, ini_path));
	option.menu_text_margin_right = Scale(profile_get_int(TEXT("menu"), TEXT("text_margin_right"), 15, ini_path));
	option.menu_text_margin_y = Scale(profile_get_int(TEXT("menu"), TEXT("text_margin_y"), 2, ini_path));
	option.menu_separator_height = Scale(profile_get_int(TEXT("menu"), TEXT("separator_height"), 9, ini_path));
	option.menu_separator_margin_left = Scale(profile_get_int(TEXT("menu"), TEXT("separator_margin_left"), 1, ini_path));
	option.menu_separator_margin_right = Scale(profile_get_int(TEXT("menu"), TEXT("separator_margin_right"), 1, ini_path));
	option.menu_max_width = Scale(profile_get_int(TEXT("menu"), TEXT("max_width"), 200, ini_path));
	if (option.menu_max_width <= 0) {
		option.menu_max_width = Scale(200);
	}
	option.menu_break = profile_get_int(TEXT("menu"), TEXT("break"), 1, ini_path);
	option.menu_show_icon = profile_get_int(TEXT("menu"), TEXT("show_icon"), 1, ini_path);
	option.menu_icon_size = Scale(profile_get_int(TEXT("menu"), TEXT("icon_size"), 16, ini_path));
	if (option.menu_icon_size <= 0) {
		option.menu_icon_size = Scale(16);
	}
	option.menu_icon_margin = Scale(profile_get_int(TEXT("menu"), TEXT("icon_margin"), 2, ini_path));
	option.menu_show_bitmap = profile_get_int(TEXT("menu"), TEXT("show_bitmap"), 1, ini_path);
	option.menu_bitmap_width = Scale(profile_get_int(TEXT("menu"), TEXT("bitmap_width"), 32, ini_path));
	if (option.menu_bitmap_width <= 0) {
		option.menu_bitmap_width = Scale(32);
	}
	option.menu_bitmap_height = Scale(profile_get_int(TEXT("menu"), TEXT("bitmap_height"), 32, ini_path));
	if (option.menu_bitmap_height <= 0) {
		option.menu_bitmap_height = Scale(32);
	}
	option.menu_show_tooltip = profile_get_int(TEXT("menu"), TEXT("show_tooltip"), 1, ini_path);
	option.menu_show_hotkey = profile_get_int(TEXT("menu"), TEXT("show_hotkey"), 1, ini_path);
	option.menu_show_tool_menu = profile_get_int(TEXT("menu"), TEXT("show_tool_menu"), 1, ini_path);
#ifdef MENU_LAYERER
	option.menu_alpha = profile_get_int(TEXT("menu"), TEXT("menu_alpha"), 0, ini_path);
#endif	// MENU_LAYERER
	option.menu_font_name = profile_alloc_string(TEXT("menu"), TEXT("font_name"), TEXT(""), ini_path);
	option.menu_font_size = profile_get_int(TEXT("menu"), TEXT("font_size"), 9, ini_path);
	option.menu_font_weight = profile_get_int(TEXT("menu"), TEXT("font_weight"), 0, ini_path);
	option.menu_font_italic = profile_get_int(TEXT("menu"), TEXT("font_italic"), 0, ini_path);
	option.menu_font_charset = profile_get_int(TEXT("menu"), TEXT("font_charset"), char_set, ini_path);
#ifdef MENU_COLOR
	option.menu_color_back.color_str = profile_alloc_string(TEXT("menu"), TEXT("color_back"), TEXT(""), ini_path);
	option.menu_color_back.color = tx2i(option.menu_color_back.color_str);
	option.menu_color_text.color_str = profile_alloc_string(TEXT("menu"), TEXT("color_text"), TEXT(""), ini_path);
	option.menu_color_text.color = tx2i(option.menu_color_text.color_str);
	option.menu_color_highlight.color_str = profile_alloc_string(TEXT("menu"), TEXT("color_highlight"), TEXT(""), ini_path);
	option.menu_color_highlight.color = tx2i(option.menu_color_highlight.color_str);
	option.menu_color_highlighttext.color_str = profile_alloc_string(TEXT("menu"), TEXT("color_highlighttext"), TEXT(""), ini_path);
	option.menu_color_highlighttext.color = tx2i(option.menu_color_highlighttext.color_str);
	option.menu_color_3d_shadow.color_str = profile_alloc_string(TEXT("menu"), TEXT("color_3d_shadow"), TEXT(""), ini_path);
	option.menu_color_3d_shadow.color = tx2i(option.menu_color_3d_shadow.color_str);
	option.menu_color_3d_highlight.color_str = profile_alloc_string(TEXT("menu"), TEXT("color_3d_highlight"), TEXT(""), ini_path);
	option.menu_color_3d_highlight.color = tx2i(option.menu_color_3d_highlight.color_str);
#endif	// MENU_COLOR

	// tooltip
	option.tooltip_show_delay = profile_get_int(TEXT("tooltip"), TEXT("show_delay"), 500, ini_path);
	option.tooltip_tab_length = profile_get_int(TEXT("tooltip"), TEXT("tab_length"), 4, ini_path);
	option.tooltip_margin_x = Scale(profile_get_int(TEXT("tooltip"), TEXT("margin_x"), 2, ini_path));
	option.tooltip_margin_y = Scale(profile_get_int(TEXT("tooltip"), TEXT("margin_y"), 2, ini_path));
	option.tooltip_font_name = profile_alloc_string(TEXT("tooltip"), TEXT("font_name"), TEXT(""), ini_path);
	option.tooltip_font_size = profile_get_int(TEXT("tooltip"), TEXT("font_size"), 9, ini_path);
	option.tooltip_font_weight = profile_get_int(TEXT("tooltip"), TEXT("font_weight"), 0, ini_path);
	option.tooltip_font_italic = profile_get_int(TEXT("tooltip"), TEXT("font_italic"), 0, ini_path);
	option.tooltip_font_charset = profile_get_int(TEXT("tooltip"), TEXT("font_charset"), char_set, ini_path);
#ifdef TOOLTIP_COLOR
	option.tooltip_color_back.color_str = profile_alloc_string(TEXT("tooltip"), TEXT("color_back"), TEXT(""), ini_path);
	option.tooltip_color_back.color = tx2i(option.tooltip_color_back.color_str);
	option.tooltip_color_text.color_str = profile_alloc_string(TEXT("tooltip"), TEXT("color_text"), TEXT(""), ini_path);
	option.tooltip_color_text.color = tx2i(option.tooltip_color_text.color_str);
#endif	// TOOLTIP_COLOR

	// action
	option.action_double_press_time = profile_get_int(TEXT("action"), TEXT("double_press_time"), GetDoubleClickTime(), ini_path);
	option.action_show_hotkey_error = profile_get_int(TEXT("action"), TEXT("show_hotkey_error"), 1, ini_path);
	option.action_cnt = profile_get_int(TEXT("action"), TEXT("cnt"), -1, ini_path);
	if (option.action_cnt < 0) {
		// Default
		option.action_cnt = 4;
		if ((option.action_info = mem_calloc(sizeof(ACTION_INFO) * option.action_cnt)) == NULL) {
			message_get_error(GetLastError(), err_str);
			return FALSE;
		}

		// tray right click
		i = 0;
		(option.action_info + i)->action = ACTION_VIEWER;
		(option.action_info + i)->type = ACTION_TYPE_TRAY_RIGHT;
		(option.action_info + i)->enable = 1;
		(option.action_info + i)->caret = 1;
		(option.action_info + i)->paste = 1;

		// tray left click
		i++;
		(option.action_info + i)->action = ACTION_POPUPMEMU;
		(option.action_info + i)->type = ACTION_TYPE_TRAY_LEFT;
		(option.action_info + i)->enable = 1;
		(option.action_info + i)->caret = 1;
		(option.action_info + i)->id = 0;
		(option.action_info + i)->modifiers = 0;
		(option.action_info + i)->virtkey = 0;
		(option.action_info + i)->paste = 1;

		(option.action_info + i)->menu_cnt = 8;
		(option.action_info + i)->menu_info = mem_calloc(sizeof(MENU_INFO) * (option.action_info + i)->menu_cnt);
		if ((option.action_info + i)->menu_info == NULL) {
			message_get_error(GetLastError(), err_str);
			return FALSE;
		}
		((option.action_info + i)->menu_info + 0)->content = MENU_CONTENT_HISTORY;
		((option.action_info + i)->menu_info + 1)->content = MENU_CONTENT_SEPARATOR;
			// popup
			((option.action_info + i)->menu_info + 2)->content = MENU_CONTENT_POPUP;
			((option.action_info + i)->menu_info + 2)->title = alloc_copy(message_get_res(IDS_MENU_REGIST));
			((option.action_info + i)->menu_info + 2)->icon_path = alloc_copy(MAIN_EXE);
			((option.action_info + i)->menu_info + 2)->icon_index = 6;
			((option.action_info + i)->menu_info + 2)->mi_cnt = 1;
			((option.action_info + i)->menu_info + 2)->mi = mem_calloc(sizeof(MENU_INFO) * ((option.action_info + i)->menu_info + 2)->mi_cnt);
			if (((option.action_info + i)->menu_info + 2)->mi == NULL) {
				message_get_error(GetLastError(), err_str);
				return FALSE;
			}
			(((option.action_info + i)->menu_info + 2)->mi + 0)->content = MENU_CONTENT_REGIST;
			(((option.action_info + i)->menu_info + 2)->mi + 0)->path = alloc_copy(TEXT("\\"));
		((option.action_info + i)->menu_info + 3)->content = MENU_CONTENT_SEPARATOR;
		((option.action_info + i)->menu_info + 4)->content = MENU_CONTENT_VIEWER;
		((option.action_info + i)->menu_info + 5)->content = MENU_CONTENT_OPTION;
		((option.action_info + i)->menu_info + 6)->content = MENU_CONTENT_SEPARATOR;
		((option.action_info + i)->menu_info + 7)->content = MENU_CONTENT_EXIT;

		// hotkey (Alt + C)
		i++;
		(option.action_info + i)->action = ACTION_POPUPMEMU;
		(option.action_info + i)->type = ACTION_TYPE_HOTKEY;
		(option.action_info + i)->enable = 1;
		(option.action_info + i)->caret = 1;
		(option.action_info + i)->id = HKEY_ID + i;
		(option.action_info + i)->modifiers = MOD_ALT;
		(option.action_info + i)->virtkey = 'C';
		(option.action_info + i)->paste = 1;
		(option.action_info + i)->menu_cnt = 5;
		(option.action_info + i)->menu_info = mem_calloc(sizeof(MENU_INFO) * (option.action_info + i)->menu_cnt);
		if ((option.action_info + i)->menu_info == NULL) {
			message_get_error(GetLastError(), err_str);
			return FALSE;
		}
		((option.action_info + i)->menu_info + 0)->content = MENU_CONTENT_HISTORY;
		((option.action_info + i)->menu_info + 1)->content = MENU_CONTENT_SEPARATOR;
			// popup
			((option.action_info + i)->menu_info + 2)->content = MENU_CONTENT_POPUP;
			((option.action_info + i)->menu_info + 2)->title = alloc_copy(message_get_res(IDS_MENU_REGIST));
			((option.action_info + i)->menu_info + 2)->icon_path = alloc_copy(MAIN_EXE);
			((option.action_info + i)->menu_info + 2)->icon_index = 6;
			((option.action_info + i)->menu_info + 2)->mi_cnt = 1;
			((option.action_info + i)->menu_info + 2)->mi = mem_calloc(sizeof(MENU_INFO) * ((option.action_info + i)->menu_info + 2)->mi_cnt);
			if (((option.action_info + i)->menu_info + 2)->mi == NULL) {
				message_get_error(GetLastError(), err_str);
				return FALSE;
			}
			(((option.action_info + i)->menu_info + 2)->mi + 0)->content = MENU_CONTENT_REGIST;
			(((option.action_info + i)->menu_info + 2)->mi + 0)->path = alloc_copy(TEXT("\\"));
		((option.action_info + i)->menu_info + 3)->content = MENU_CONTENT_SEPARATOR;
		((option.action_info + i)->menu_info + 4)->content = MENU_CONTENT_CANCEL;

		// hotkey (Alt + T)
		i++;
		(option.action_info + i)->action = ACTION_POPUPMEMU;
		(option.action_info + i)->type = ACTION_TYPE_HOTKEY;
		(option.action_info + i)->enable = 1;
		(option.action_info + i)->caret = 1;
		(option.action_info + i)->id = HKEY_ID + i;
		(option.action_info + i)->modifiers = MOD_ALT;
		(option.action_info + i)->virtkey = 'T';
		(option.action_info + i)->paste = 1;
		(option.action_info + i)->menu_cnt = 3;
		(option.action_info + i)->menu_info = mem_calloc(sizeof(MENU_INFO) * (option.action_info + i)->menu_cnt);
		if ((option.action_info + i)->menu_info == NULL) {
			message_get_error(GetLastError(), err_str);
			return FALSE;
		}
		((option.action_info + i)->menu_info + 0)->content = MENU_CONTENT_TOOL;
		((option.action_info + i)->menu_info + 1)->content = MENU_CONTENT_SEPARATOR;
		((option.action_info + i)->menu_info + 2)->content = MENU_CONTENT_CANCEL;
	} else {
		if ((option.action_info = mem_calloc(sizeof(ACTION_INFO) * option.action_cnt)) == NULL) {
			message_get_error(GetLastError(), err_str);
			return FALSE;
		}
		for (i = 0; i < option.action_cnt; i++) {
			wsprintf(buf, TEXT("action-%d"), i);
			(option.action_info + i)->action = profile_get_int(TEXT("action"), buf, ACTION_VIEWER, ini_path);
			wsprintf(buf, TEXT("type-%d"), i);
			(option.action_info + i)->type = profile_get_int(TEXT("action"), buf, ACTION_TYPE_TRAY_LEFT, ini_path);
			wsprintf(buf, TEXT("enable-%d"), i);
			(option.action_info + i)->enable = profile_get_int(TEXT("action"), buf, 1, ini_path);
			wsprintf(buf, TEXT("caret-%d"), i);
			(option.action_info + i)->caret = profile_get_int(TEXT("action"), buf, 1, ini_path);
			(option.action_info + i)->id = HKEY_ID + i;
			wsprintf(buf, TEXT("modifiers-%d"), i);
			(option.action_info + i)->modifiers = profile_get_int(TEXT("action"), buf, 0, ini_path);
			wsprintf(buf, TEXT("virtkey-%d"), i);
			(option.action_info + i)->virtkey = profile_get_int(TEXT("action"), buf, 0, ini_path);
			wsprintf(buf, TEXT("paste-%d"), i);
			(option.action_info + i)->paste = profile_get_int(TEXT("action"), buf, 0, ini_path);
			wsprintf(buf, TEXT("menu_cnt-%d"), i);
			(option.action_info + i)->menu_cnt = profile_get_int(TEXT("action"), buf, 0, ini_path);

			if ((option.action_info + i)->menu_cnt > 0) {
				(option.action_info + i)->menu_info = mem_calloc(sizeof(MENU_INFO) * (option.action_info + i)->menu_cnt);
				if ((option.action_info + i)->menu_info == NULL) {
					return FALSE;
				}
				wsprintf(buf, TEXT("menu_info-%d"), i);
				if (ini_get_menu(ini_path, buf, (option.action_info + i)->menu_info, (option.action_info + i)->menu_cnt, err_str) == FALSE) {
					return FALSE;
				}
			}
		}
	}

	// format
	option.format_cnt = profile_get_int(TEXT("format"), TEXT("cnt"), -1, ini_path);
	if (option.format_cnt < 0) {
		// Default
		option.format_cnt = 4;
		if ((option.format_info = mem_calloc(sizeof(FORMAT_INFO) * option.format_cnt)) == NULL) {
			message_get_error(GetLastError(), err_str);
			return FALSE;
		}
		(option.format_info + 0)->format_name = alloc_copy(TEXT("UNICODE TEXT"));
		(option.format_info + 0)->func_header = alloc_copy(TEXT("text_"));

		(option.format_info + 1)->format_name = alloc_copy(TEXT("TEXT"));
		(option.format_info + 1)->func_header = alloc_copy(TEXT("text_"));

		(option.format_info + 2)->format_name = alloc_copy(TEXT("BITMAP, DIB"));
		(option.format_info + 2)->func_header = alloc_copy(TEXT("bitmap_"));

		(option.format_info + 3)->format_name = alloc_copy(TEXT("DROP FILE LIST"));
		(option.format_info + 3)->func_header = alloc_copy(TEXT("file_"));

		for (i = 0; i < option.format_cnt; i++) {
			// 形式名の取得
			(option.format_info + i)->fn = ini_get_format_name((option.format_info + i)->format_name,
				&(option.format_info + i)->fn_cnt);
		}
	} else {
		if ((option.format_info = mem_calloc(sizeof(FORMAT_INFO) * option.format_cnt)) == NULL) {
			message_get_error(GetLastError(), err_str);
			return FALSE;
		}
		for (i = 0; i < option.format_cnt; i++) {
			wsprintf(buf, TEXT("format_name-%d"), i);
			(option.format_info + i)->format_name = profile_alloc_string(TEXT("format"), buf, TEXT(""), ini_path);
			wsprintf(buf, TEXT("lib_file_path-%d"), i);
			(option.format_info + i)->lib_file_path = profile_alloc_string(TEXT("format"), buf, TEXT(""), ini_path);
			wsprintf(buf, TEXT("func_header-%d"), i);
			(option.format_info + i)->func_header = profile_alloc_string(TEXT("format"), buf, TEXT(""), ini_path);

			// 形式名の取得
			(option.format_info + i)->fn = ini_get_format_name((option.format_info + i)->format_name,
				&(option.format_info + i)->fn_cnt);
		}
		if (version < 120) {
			FORMAT_INFO *tmp_format_list;
			int format_cnt = option.format_cnt + 1;

			if ((tmp_format_list = mem_calloc(sizeof(FORMAT_INFO) * format_cnt)) == NULL) {
				message_get_error(GetLastError(), err_str);
				return FALSE;
			}
			(tmp_format_list + 0)->format_name = alloc_copy(TEXT("UNICODE TEXT"));
			(tmp_format_list + 0)->func_header = alloc_copy(TEXT("text_"));
			(tmp_format_list + 0)->fn = ini_get_format_name((tmp_format_list + 0)->format_name,
				&(tmp_format_list + 0)->fn_cnt);
			for (i = 0; i < option.format_cnt; i++) {
				(tmp_format_list + i + 1)->format_name = (option.format_info + i)->format_name;
				(tmp_format_list + i + 1)->lib_file_path = (option.format_info + i)->lib_file_path;
				(tmp_format_list + i + 1)->func_header = (option.format_info + i)->func_header;
				(tmp_format_list + i + 1)->fn = (option.format_info + i)->fn;
				(tmp_format_list + i + 1)->fn_cnt = (option.format_info + i)->fn_cnt;
			}
			mem_free(&option.format_info);
			option.format_info = tmp_format_list;
			option.format_cnt = format_cnt;
		}
	}

	// filter
	option.filter_all_action = profile_get_int(TEXT("filter"), TEXT("all_action"), FILTER_ACTION_IGNORE, ini_path);
	option.filter_cnt = profile_get_int(TEXT("filter"), TEXT("cnt"), -1, ini_path);
	if (option.filter_cnt < 0) {
		// Default
		option.filter_cnt = 3;
		if ((option.filter_info = mem_calloc(sizeof(FILTER_INFO) * option.filter_cnt)) == NULL) {
			message_get_error(GetLastError(), err_str);
			return FALSE;
		}
		(option.filter_info + 0)->format_name = alloc_copy(TEXT("UNICODE TEXT"));
		(option.filter_info + 0)->action = FILTER_ACTION_ADD;
		(option.filter_info + 0)->save = FILTER_SAVE_SAVE;
		(option.filter_info + 0)->limit_size = 0;

		(option.filter_info + 1)->format_name = alloc_copy(TEXT("BITMAP"));
		(option.filter_info + 1)->action = FILTER_ACTION_ADD;
		(option.filter_info + 1)->save = FILTER_SAVE_SAVE;
		(option.filter_info + 1)->limit_size = 0;

		(option.filter_info + 2)->format_name = alloc_copy(TEXT("DROP FILE LIST"));
		(option.filter_info + 2)->action = FILTER_ACTION_ADD;
		(option.filter_info + 2)->save = FILTER_SAVE_SAVE;
		(option.filter_info + 2)->limit_size = 0;

		for (i = 0; i < option.filter_cnt; i++) {
			// 形式名の取得
			(option.filter_info + i)->fn = ini_get_format_name((option.filter_info + i)->format_name,
				&(option.filter_info + i)->fn_cnt);
		}
	} else {
		if ((option.filter_info = mem_calloc(sizeof(FILTER_INFO) * option.filter_cnt)) == NULL) {
			message_get_error(GetLastError(), err_str);
			return FALSE;
		}
		for (i = 0; i < option.filter_cnt; i++) {
			wsprintf(buf, TEXT("format_name-%d"), i);
			(option.filter_info + i)->format_name = profile_alloc_string(TEXT("filter"), buf, TEXT(""), ini_path);
			wsprintf(buf, TEXT("action-%d"), i);
			(option.filter_info + i)->action = profile_get_int(TEXT("filter"), buf, FILTER_ACTION_ADD, ini_path);
			wsprintf(buf, TEXT("save-%d"), i);
			(option.filter_info + i)->save = profile_get_int(TEXT("filter"), buf, FILTER_SAVE_SAVE, ini_path);
			wsprintf(buf, TEXT("limit_size-%d"), i);
			(option.filter_info + i)->limit_size = profile_get_int(TEXT("filter"), buf, 0, ini_path);

			// 形式名の取得
			(option.filter_info + i)->fn = ini_get_format_name((option.filter_info + i)->format_name,
				&(option.filter_info + i)->fn_cnt);
		}
		if (version < 120) {
			FILTER_INFO *tmp_filter_list;
			int filter_cnt = option.filter_cnt + 1;

			if ((tmp_filter_list = mem_calloc(sizeof(FILTER_INFO) * filter_cnt)) == NULL) {
				message_get_error(GetLastError(), err_str);
				return FALSE;
			}
			(tmp_filter_list + 0)->format_name = alloc_copy(TEXT("UNICODE TEXT"));
			(tmp_filter_list + 0)->action = FILTER_ACTION_ADD;
			(tmp_filter_list + 0)->save = FILTER_SAVE_SAVE;
			(tmp_filter_list + 0)->limit_size = 0;
			(tmp_filter_list + 0)->fn = ini_get_format_name((tmp_filter_list + 0)->format_name,
				&(tmp_filter_list + 0)->fn_cnt);
			for (i = 0; i < option.filter_cnt; i++) {
				(tmp_filter_list + i + 1)->format_name = (option.filter_info + i)->format_name;
				(tmp_filter_list + i + 1)->action = (option.filter_info + i)->action;
				(tmp_filter_list + i + 1)->save = (option.filter_info + i)->save;
				(tmp_filter_list + i + 1)->limit_size = (option.filter_info + i)->limit_size;
				(tmp_filter_list + i + 1)->fn = (option.filter_info + i)->fn;
				(tmp_filter_list + i + 1)->fn_cnt = (option.filter_info + i)->fn_cnt;
			}
			mem_free(&option.filter_info);
			option.filter_info = tmp_filter_list;
			option.filter_cnt = filter_cnt;
		}
	}

	// window filter
	option.window_filter_cnt = profile_get_int(TEXT("window_filter"), TEXT("cnt"), -1, ini_path);
	if (option.window_filter_cnt < 0) {
		option.window_filter_cnt = 0;
	} else {
		if ((option.window_filter_info = mem_calloc(sizeof(WINDOW_FILTER_INFO) * option.window_filter_cnt)) == NULL) {
			message_get_error(GetLastError(), err_str);
			return FALSE;
		}
		for (i = 0; i < option.window_filter_cnt; i++) {
			wsprintf(buf, TEXT("title-%d"), i);
			(option.window_filter_info + i)->title = profile_alloc_string(TEXT("window_filter"), buf, TEXT(""), ini_path);
			wsprintf(buf, TEXT("class_name-%d"), i);
			(option.window_filter_info + i)->class_name = profile_alloc_string(TEXT("window_filter"), buf, TEXT(""), ini_path);

			wsprintf(buf, TEXT("ignore-%d"), i);
			(option.window_filter_info + i)->ignore = profile_get_int(TEXT("window_filter"), buf, 1, ini_path);
			wsprintf(buf, TEXT("focus-%d"), i);
			(option.window_filter_info + i)->focus = profile_get_int(TEXT("window_filter"), buf, 1, ini_path);
			wsprintf(buf, TEXT("paste-%d"), i);
			(option.window_filter_info + i)->paste = profile_get_int(TEXT("window_filter"), buf, 0, ini_path);
		}
	}

	// send key
	option.def_copy_modifiers = profile_get_int(TEXT("sendkey"), TEXT("def_copy_modifiers"), MOD_CONTROL, ini_path);
	option.def_copy_virtkey = profile_get_int(TEXT("sendkey"), TEXT("def_copy_virtkey"), 'C', ini_path);
	option.def_copy_wait = profile_get_int(TEXT("sendkey"), TEXT("def_copy_wait"), DEFAULT_COPY_WAIT, ini_path);

	option.def_paste_modifiers = profile_get_int(TEXT("sendkey"), TEXT("def_paste_modifiers"), MOD_CONTROL, ini_path);
	option.def_paste_virtkey = profile_get_int(TEXT("sendkey"), TEXT("def_paste_virtkey"), 'V', ini_path);
	option.def_paste_wait = profile_get_int(TEXT("sendkey"), TEXT("def_paste_wait"), DEFAULT_PASTE_WAIT, ini_path);

	option.sendkey_cnt = profile_get_int(TEXT("sendkey"), TEXT("cnt"), -1, ini_path);
	if (option.sendkey_cnt < 0) {
		option.sendkey_cnt = 0;
	} else {
		if ((option.sendkey_info = mem_calloc(sizeof(SENDKEY_INFO) * option.sendkey_cnt)) == NULL) {
			message_get_error(GetLastError(), err_str);
			return FALSE;
		}
		for (i = 0; i < option.sendkey_cnt; i++) {
			wsprintf(buf, TEXT("title-%d"), i);
			(option.sendkey_info + i)->title = profile_alloc_string(TEXT("sendkey"), buf, TEXT(""), ini_path);
			wsprintf(buf, TEXT("class_name-%d"), i);
			(option.sendkey_info + i)->class_name = profile_alloc_string(TEXT("sendkey"), buf, TEXT(""), ini_path);

			wsprintf(buf, TEXT("copy_modifiers-%d"), i);
			(option.sendkey_info + i)->copy_modifiers = profile_get_int(TEXT("sendkey"), buf, 0, ini_path);
			wsprintf(buf, TEXT("copy_virtkey-%d"), i);
			(option.sendkey_info + i)->copy_virtkey = profile_get_int(TEXT("sendkey"), buf, 0, ini_path);
			wsprintf(buf, TEXT("copy_wait-%d"), i);
			(option.sendkey_info + i)->copy_wait = profile_get_int(TEXT("sendkey"), buf, DEFAULT_COPY_WAIT, ini_path);

			wsprintf(buf, TEXT("paste_modifiers-%d"), i);
			(option.sendkey_info + i)->paste_modifiers = profile_get_int(TEXT("sendkey"), buf, 0, ini_path);
			wsprintf(buf, TEXT("paste_virtkey-%d"), i);
			(option.sendkey_info + i)->paste_virtkey = profile_get_int(TEXT("sendkey"), buf, 0, ini_path);
			wsprintf(buf, TEXT("paste_wait-%d"), i);
			(option.sendkey_info + i)->paste_wait = profile_get_int(TEXT("sendkey"), buf, DEFAULT_PASTE_WAIT, ini_path);
		}
	}

	// tool
	option.tool_valid_interval = profile_get_int(TEXT("tool"), TEXT("valid_interval"), 1000 * 5, ini_path);
	option.tool_cnt = profile_get_int(TEXT("tool"), TEXT("cnt"), -1, ini_path);
	if (option.tool_cnt < 0) {
		option.tool_cnt = 0;
	} else {
		if ((option.tool_info = mem_calloc(sizeof(TOOL_INFO) * option.tool_cnt)) == NULL) {
			message_get_error(GetLastError(), err_str);
			return FALSE;
		}
		for (i = 0; i < option.tool_cnt; i++) {
			wsprintf(buf, TEXT("title-%d"), i);
			(option.tool_info + i)->title = profile_alloc_string(TEXT("tool"), buf, TEXT(""), ini_path);
			wsprintf(buf, TEXT("lib_file_path-%d"), i);
			(option.tool_info + i)->lib_file_path = profile_alloc_string(TEXT("tool"), buf, TEXT(""), ini_path);
			wsprintf(buf, TEXT("func_name-%d"), i);
			(option.tool_info + i)->func_name = profile_alloc_string(TEXT("tool"), buf, TEXT(""), ini_path);
			wsprintf(buf, TEXT("cmd_line-%d"), i);
			(option.tool_info + i)->cmd_line = profile_alloc_string(TEXT("tool"), buf, TEXT(""), ini_path);
			wsprintf(buf, TEXT("call_type-%d"), i);
			(option.tool_info + i)->call_type = profile_get_int(TEXT("tool"), buf, 0, ini_path);
			wsprintf(buf, TEXT("copy_paste-%d"), i);
			(option.tool_info + i)->copy_paste = profile_get_int(TEXT("tool"), buf, 1, ini_path);

			(option.tool_info + i)->id = HKEY_ID + option.action_cnt + i;
			wsprintf(buf, TEXT("modifiers-%d"), i);
			(option.tool_info + i)->modifiers = profile_get_int(TEXT("tool"), buf, 0, ini_path);
			wsprintf(buf, TEXT("virtkey-%d"), i);
			(option.tool_info + i)->virtkey = profile_get_int(TEXT("tool"), buf, 0, ini_path);

			wsprintf(buf, TEXT("old-%d"), i);
			(option.tool_info + i)->old = profile_get_int(TEXT("tool"), buf, 0, ini_path);
		}
	}

	// viewer
	option.viewer_toggle = profile_get_int(TEXT("viewer"), TEXT("toggle"), 0, ini_path);
	option.viewer_show_bin = profile_get_int(TEXT("viewer"), TEXT("show_bin"), 0, ini_path);
	option.viewer_show_toolbar = profile_get_int(TEXT("viewer"), TEXT("show_toolbar"), 1, ini_path);
	option.viewer_show_statusbar = profile_get_int(TEXT("viewer"), TEXT("show_statusbar"), 1, ini_path);
	option.viewer_delete_confirm = profile_get_int(TEXT("viewer"), TEXT("delete_confirm"), 1, ini_path);

	GetWindowRect(GetDesktopWindow(), &desktop_rect);
	option.viewer_rect.left = Scale(profile_get_int(TEXT("viewer"), TEXT("left"), 0, ini_path));
	option.viewer_rect.top = Scale(profile_get_int(TEXT("viewer"), TEXT("top"), 0, ini_path));
	option.viewer_rect.right = Scale(profile_get_int(TEXT("viewer"), TEXT("right"), 550, ini_path));
	option.viewer_rect.bottom = Scale(profile_get_int(TEXT("viewer"), TEXT("bottom"), 400, ini_path));
	option.viewer_sep_size = Scale(profile_get_int(TEXT("viewer"), TEXT("sep_size"), 150, ini_path));
	if (option.viewer_sep_size < 1 || (option.viewer_rect.right > 0 && option.viewer_sep_size > option.viewer_rect.right)) {
		option.viewer_sep_size = Scale(150);
	}

	// treeview
	option.tree_show_format = profile_get_int(TEXT("treeview"), TEXT("show_format"), 1, ini_path);
	option.tree_root_order = profile_alloc_string(TEXT("treeview"), TEXT("root_order"), TEXT("012"), ini_path);
	option.tree_clip_expand = profile_get_int(TEXT("treeview"), TEXT("clip_expand"), 1, ini_path);
	option.tree_history_expand = profile_get_int(TEXT("treeview"), TEXT("history_expand"), 1, ini_path);
	option.tree_regist_expand = profile_get_int(TEXT("treeview"), TEXT("regist_expand"), 1, ini_path);
	option.tree_folder_expand = profile_get_int(TEXT("treeview"), TEXT("folder_expand"), 0, ini_path);
	option.tree_font_name = profile_alloc_string(TEXT("treeview"), TEXT("font_name"), TEXT(""), ini_path);
	option.tree_font_size = profile_get_int(TEXT("treeview"), TEXT("font_size"), 9, ini_path);
	option.tree_font_weight = profile_get_int(TEXT("treeview"), TEXT("font_weight"), 0, ini_path);
	option.tree_font_italic = profile_get_int(TEXT("treeview"), TEXT("font_italic"), 0, ini_path);
	option.tree_font_charset = profile_get_int(TEXT("treeview"), TEXT("font_charset"), char_set, ini_path);

	// listview
	option.list_default_action = profile_get_int(TEXT("listview"), TEXT("default_action"), 0, ini_path);
	option.list_column_data = Scale(profile_get_int(TEXT("listview"), TEXT("column_data"), 150, ini_path));
	option.list_column_size = Scale(profile_get_int(TEXT("listview"), TEXT("column_size"), 100, ini_path));
	option.list_column_date = Scale(profile_get_int(TEXT("listview"), TEXT("column_date"), 100, ini_path));
	option.list_column_window = Scale(profile_get_int(TEXT("listview"), TEXT("column_window"), 100, ini_path));
	option.list_font_name = profile_alloc_string(TEXT("listview"), TEXT("font_name"), TEXT(""), ini_path);
	option.list_font_size = profile_get_int(TEXT("listview"), TEXT("font_size"), 9, ini_path);
	option.list_font_weight = profile_get_int(TEXT("listview"), TEXT("font_weight"), 0, ini_path);
	option.list_font_italic = profile_get_int(TEXT("listview"), TEXT("font_italic"), 0, ini_path);
	option.list_font_charset = profile_get_int(TEXT("listview"), TEXT("font_charset"), char_set, ini_path);

	// bin view
	option.bin_lock = profile_get_int(TEXT("binview"), TEXT("lock"), 1, ini_path);
	option.bin_font_name = profile_alloc_string(TEXT("binview"), TEXT("font_name"), TEXT("FixedSys"), ini_path);
	option.bin_font_size = profile_get_int(TEXT("binview"), TEXT("font_size"), 9, ini_path);
	option.bin_font_weight = profile_get_int(TEXT("binview"), TEXT("font_weight"), 0, ini_path);
	option.bin_font_italic = profile_get_int(TEXT("binview"), TEXT("font_italic"), 0, ini_path);
	option.bin_font_charset = profile_get_int(TEXT("binview"), TEXT("font_charset"), char_set, ini_path);

	// text format
	option.fmt_txt_menu_tooltip_size = profile_get_int(TEXT("fmt_text"), TEXT("menu_tooltip_size"), 1024, ini_path);
	option.fmt_txt_viewer_word_wrap = profile_get_int(TEXT("fmt_text"), TEXT("viewer_word_wrap"), 0, ini_path);
	option.fmt_txt_tab_size = profile_get_int(TEXT("fmt_text"), TEXT("tab_size"), 8, ini_path);
	option.fmt_txt_font_name = profile_alloc_string(TEXT("fmt_text"), TEXT("font_name"), TEXT(""), ini_path);
	option.fmt_txt_font_size = profile_get_int(TEXT("fmt_text"), TEXT("font_size"), 9, ini_path);
	option.fmt_txt_font_weight = profile_get_int(TEXT("fmt_text"), TEXT("font_weight"), 0, ini_path);
	option.fmt_txt_font_italic = profile_get_int(TEXT("fmt_text"), TEXT("font_italic"), 0, ini_path);
	option.fmt_txt_font_charset = profile_get_int(TEXT("fmt_text"), TEXT("font_charset"), char_set, ini_path);

	// bitmap format
	option.fmt_bmp_stretch_mode = profile_get_int(TEXT("fmt_bitmap"), TEXT("stretch_mode"), 0, ini_path);

	// file format
	option.fmt_file_column_name = Scale(profile_get_int(TEXT("fmt_file"), TEXT("column_name"), 200, ini_path));
	option.fmt_file_column_folder = Scale(profile_get_int(TEXT("fmt_file"), TEXT("column_folder"), 200, ini_path));
	option.fmt_file_column_type = Scale(profile_get_int(TEXT("fmt_file"), TEXT("column_type"), 100, ini_path));
	option.fmt_file_font_name = profile_alloc_string(TEXT("fmt_file"), TEXT("font_name"), TEXT(""), ini_path);
	option.fmt_file_font_size = profile_get_int(TEXT("fmt_file"), TEXT("font_size"), 9, ini_path);
	option.fmt_file_font_weight = profile_get_int(TEXT("fmt_file"), TEXT("font_weight"), 0, ini_path);
	option.fmt_file_font_italic = profile_get_int(TEXT("fmt_file"), TEXT("font_italic"), 0, ini_path);
	option.fmt_file_font_charset = profile_get_int(TEXT("fmt_file"), TEXT("font_charset"), char_set, ini_path);

	profile_free();
	return TRUE;
}

/*
 * ini_put_menu - メニューオプションを書きこむ
 */
static BOOL ini_put_menu(const TCHAR *ini_path, const TCHAR *menu_path, MENU_INFO *mi, const int mcnt)
{
	TCHAR buf[BUF_SIZE];
	int i;

	if (mi == NULL) {
		return TRUE;
	}

	for (i = 0; i < mcnt; i++) {
		wsprintf(buf, TEXT("%s-content-%d"), menu_path, i);
		profile_write_int(TEXT("action"), buf, (mi + i)->content, ini_path);
		wsprintf(buf, TEXT("%s-title-%d"), menu_path, i);
		profile_write_string(TEXT("action"), buf, (mi + i)->title, ini_path);
		wsprintf(buf, TEXT("%s-icon_path-%d"), menu_path, i);
		profile_write_string(TEXT("action"), buf, (mi + i)->icon_path, ini_path);
		wsprintf(buf, TEXT("%s-icon_index-%d"), menu_path, i);
		profile_write_int(TEXT("action"), buf, (mi + i)->icon_index, ini_path);
		wsprintf(buf, TEXT("%s-path-%d"), menu_path, i);
		profile_write_string(TEXT("action"), buf, (mi + i)->path, ini_path);
		wsprintf(buf, TEXT("%s-cmd-%d"), menu_path, i);
		profile_write_string(TEXT("action"), buf, (mi + i)->cmd, ini_path);
		wsprintf(buf, TEXT("%s-min-%d"), menu_path, i);
		profile_write_int(TEXT("action"), buf, (mi + i)->min, ini_path);
		wsprintf(buf, TEXT("%s-max-%d"), menu_path, i);
		profile_write_int(TEXT("action"), buf, (mi + i)->max, ini_path);
		wsprintf(buf, TEXT("%s-mi_cnt-%d"), menu_path, i);
		profile_write_int(TEXT("action"), buf, (mi + i)->mi_cnt, ini_path);

		if ((mi + i)->mi != NULL) {
			wsprintf(buf, TEXT("%s-menu_info-%d"), menu_path, i);
			if (ini_put_menu(ini_path, buf, (mi + i)->mi, (mi + i)->mi_cnt) == FALSE) {
				return FALSE;
			}
		}
	}
	return TRUE;
}

/*
 * ini_put_option - オプションを書きこむ
 */
BOOL ini_put_option(void)
{
	TCHAR ini_path[MAX_PATH];
	TCHAR buf[BUF_SIZE];
	int i;

	wsprintf(ini_path, TEXT("%s\\%s"), work_path, USER_INI);
	profile_initialize(ini_path, TRUE);

	// main
	profile_write_int(TEXT("main"), TEXT("version"), APP_VAR, ini_path);
#ifndef OPTION_SET
	profile_write_int(TEXT("main"), TEXT("clipboard_watch"), option.main_clipboard_watch, ini_path);
#endif	// OPTION_SET
	profile_write_int(TEXT("main"), TEXT("clipboard_rechain_minute"), option.main_clipboard_rechain_minute, ini_path);
	profile_write_int(TEXT("main"), TEXT("show_trayicon"), option.main_show_trayicon, ini_path);
	profile_write_int(TEXT("main"), TEXT("show_viewer"), option.main_show_viewer, ini_path);

	// data
	profile_write_string(TEXT("data"), TEXT("date_format"), option.data_date_format, ini_path);
	profile_write_string(TEXT("data"), TEXT("time_format"), option.data_time_format, ini_path);

	// history
	profile_write_int(TEXT("history"), TEXT("add_interval"), option.history_add_interval, ini_path);
	profile_write_int(TEXT("history"), TEXT("save"), option.history_save, ini_path);
	profile_write_int(TEXT("history"), TEXT("always_save"), option.history_always_save, ini_path);
	profile_write_int(TEXT("history"), TEXT("delete"), option.history_delete, ini_path);
	profile_write_int(TEXT("history"), TEXT("max"), option.history_max, ini_path);
	profile_write_int(TEXT("history"), TEXT("overlap_check"), option.history_overlap_check, ini_path);
	profile_write_int(TEXT("history"), TEXT("ignore_regist_item"), option.history_ignore_regist_item, ini_path);

	// menu
	profile_write_string(TEXT("menu"), TEXT("text_format"), option.menu_text_format, ini_path);
	profile_write_int(TEXT("menu"), TEXT("intact_item_title"), option.menu_intact_item_title, ini_path);
	profile_write_int(TEXT("menu"), TEXT("text_margin_left"), UnScale(option.menu_text_margin_left), ini_path);
	profile_write_int(TEXT("menu"), TEXT("text_margin_right"), UnScale(option.menu_text_margin_right), ini_path);
	profile_write_int(TEXT("menu"), TEXT("text_margin_y"), UnScale(option.menu_text_margin_y), ini_path);
	profile_write_int(TEXT("menu"), TEXT("separator_height"), UnScale(option.menu_separator_height), ini_path);
	profile_write_int(TEXT("menu"), TEXT("separator_margin_left"), UnScale(option.menu_separator_margin_left), ini_path);
	profile_write_int(TEXT("menu"), TEXT("separator_margin_right"), UnScale(option.menu_separator_margin_right), ini_path);
	profile_write_int(TEXT("menu"), TEXT("max_width"), UnScale(option.menu_max_width), ini_path);
	profile_write_int(TEXT("menu"), TEXT("break"), option.menu_break, ini_path);
	profile_write_int(TEXT("menu"), TEXT("show_icon"), option.menu_show_icon, ini_path);
	profile_write_int(TEXT("menu"), TEXT("icon_size"), UnScale(option.menu_icon_size), ini_path);
	profile_write_int(TEXT("menu"), TEXT("icon_margin"), UnScale(option.menu_icon_margin), ini_path);
	profile_write_int(TEXT("menu"), TEXT("show_bitmap"), option.menu_show_bitmap, ini_path);
	profile_write_int(TEXT("menu"), TEXT("bitmap_width"), UnScale(option.menu_bitmap_width), ini_path);
	profile_write_int(TEXT("menu"), TEXT("bitmap_height"), UnScale(option.menu_bitmap_height), ini_path);
	profile_write_int(TEXT("menu"), TEXT("show_tooltip"), option.menu_show_tooltip, ini_path);
	profile_write_int(TEXT("menu"), TEXT("show_hotkey"), option.menu_show_hotkey, ini_path);
	profile_write_int(TEXT("menu"), TEXT("show_tool_menu"), option.menu_show_tool_menu, ini_path);
#ifdef MENU_LAYERER
	profile_write_int(TEXT("menu"), TEXT("menu_alpha"), option.menu_alpha, ini_path);
#endif	// MENU_LAYERER
	profile_write_string(TEXT("menu"), TEXT("font_name"), option.menu_font_name, ini_path);
	profile_write_int(TEXT("menu"), TEXT("font_size"), option.menu_font_size, ini_path);
	profile_write_int(TEXT("menu"), TEXT("font_weight"), option.menu_font_weight, ini_path);
	profile_write_int(TEXT("menu"), TEXT("font_italic"), option.menu_font_italic, ini_path);
	profile_write_int(TEXT("menu"), TEXT("font_charset"), option.menu_font_charset, ini_path);
#ifdef MENU_COLOR
	profile_write_string(TEXT("menu"), TEXT("color_back"), option.menu_color_back.color_str, ini_path);
	profile_write_string(TEXT("menu"), TEXT("color_text"), option.menu_color_text.color_str, ini_path);
	profile_write_string(TEXT("menu"), TEXT("color_highlight"), option.menu_color_highlight.color_str, ini_path);
	profile_write_string(TEXT("menu"), TEXT("color_highlighttext"), option.menu_color_highlighttext.color_str, ini_path);
	profile_write_string(TEXT("menu"), TEXT("color_3d_shadow"), option.menu_color_3d_shadow.color_str, ini_path);
	profile_write_string(TEXT("menu"), TEXT("color_3d_highlight"), option.menu_color_3d_highlight.color_str, ini_path);
#endif	// MENU_COLOR

	// tooltip
	profile_write_int(TEXT("tooltip"), TEXT("show_delay"), option.tooltip_show_delay, ini_path);
	profile_write_int(TEXT("tooltip"), TEXT("tab_length"), option.tooltip_tab_length, ini_path);
	profile_write_int(TEXT("tooltip"), TEXT("margin_x"), UnScale(option.tooltip_margin_x), ini_path);
	profile_write_int(TEXT("tooltip"), TEXT("margin_y"), UnScale(option.tooltip_margin_y), ini_path);
	profile_write_string(TEXT("tooltip"), TEXT("font_name"), option.tooltip_font_name, ini_path);
	profile_write_int(TEXT("tooltip"), TEXT("font_size"), option.tooltip_font_size, ini_path);
	profile_write_int(TEXT("tooltip"), TEXT("font_weight"), option.tooltip_font_weight, ini_path);
	profile_write_int(TEXT("tooltip"), TEXT("font_italic"), option.tooltip_font_italic, ini_path);
	profile_write_int(TEXT("tooltip"), TEXT("font_charset"), option.tooltip_font_charset, ini_path);
#ifdef TOOLTIP_COLOR
	profile_write_string(TEXT("tooltip"), TEXT("color_back"), option.tooltip_color_back.color_str, ini_path);
	profile_write_string(TEXT("tooltip"), TEXT("color_text"), option.tooltip_color_text.color_str, ini_path);
#endif	// TOOLTIP_COLOR

	// action
#ifdef OPTION_SET
	profile_write_string(TEXT("action"), NULL, NULL, ini_path);
#endif	// OPTION_SET
	profile_write_int(TEXT("action"), TEXT("double_press_time"), option.action_double_press_time, ini_path);
	profile_write_int(TEXT("action"), TEXT("show_hotkey_error"), option.action_show_hotkey_error, ini_path);
	profile_write_int(TEXT("action"), TEXT("cnt"), option.action_cnt, ini_path);
	for (i = 0; i < option.action_cnt; i++) {
		wsprintf(buf, TEXT("action-%d"), i);
		profile_write_int(TEXT("action"), buf, (option.action_info + i)->action, ini_path);
		wsprintf(buf, TEXT("type-%d"), i);
		profile_write_int(TEXT("action"), buf, (option.action_info + i)->type, ini_path);
		wsprintf(buf, TEXT("enable-%d"), i);
		profile_write_int(TEXT("action"), buf, (option.action_info + i)->enable, ini_path);
		wsprintf(buf, TEXT("caret-%d"), i);
		profile_write_int(TEXT("action"), buf, (option.action_info + i)->caret, ini_path);
		wsprintf(buf, TEXT("modifiers-%d"), i);
		profile_write_int(TEXT("action"), buf, (option.action_info + i)->modifiers, ini_path);
		wsprintf(buf, TEXT("virtkey-%d"), i);
		profile_write_int(TEXT("action"), buf, (option.action_info + i)->virtkey, ini_path);
		wsprintf(buf, TEXT("paste-%d"), i);
		profile_write_int(TEXT("action"), buf, (option.action_info + i)->paste, ini_path);
		wsprintf(buf, TEXT("menu_cnt-%d"), i);
		profile_write_int(TEXT("action"), buf, (option.action_info + i)->menu_cnt, ini_path);

		wsprintf(buf, TEXT("menu_info-%d"), i);
		ini_put_menu(ini_path, buf, (option.action_info + i)->menu_info, (option.action_info + i)->menu_cnt);
	}

	// format
#ifdef OPTION_SET
	profile_write_string(TEXT("format"), NULL, NULL, ini_path);
#endif	// OPTION_SET
	profile_write_int(TEXT("format"), TEXT("cnt"), option.format_cnt, ini_path);
	for (i = 0; i < option.format_cnt; i++) {
		wsprintf(buf, TEXT("format_name-%d"), i);
		profile_write_string(TEXT("format"), buf, (option.format_info + i)->format_name, ini_path);
		wsprintf(buf, TEXT("lib_file_path-%d"), i);
		profile_write_string(TEXT("format"), buf, (option.format_info + i)->lib_file_path, ini_path);
		wsprintf(buf, TEXT("func_header-%d"), i);
		profile_write_string(TEXT("format"), buf, (option.format_info + i)->func_header, ini_path);
	}

	// filter
#ifdef OPTION_SET
	profile_write_string(TEXT("filter"), NULL, NULL, ini_path);
#endif	// OPTION_SET
	profile_write_int(TEXT("filter"), TEXT("all_action"), option.filter_all_action, ini_path);
	profile_write_int(TEXT("filter"), TEXT("cnt"), option.filter_cnt, ini_path);
	for (i = 0; i < option.filter_cnt; i++) {
		wsprintf(buf, TEXT("format_name-%d"), i);
		profile_write_string(TEXT("filter"), buf, (option.filter_info + i)->format_name, ini_path);
		wsprintf(buf, TEXT("action-%d"), i);
		profile_write_int(TEXT("filter"), buf, (option.filter_info + i)->action, ini_path);
		wsprintf(buf, TEXT("save-%d"), i);
		profile_write_int(TEXT("filter"), buf, (option.filter_info + i)->save, ini_path);
		wsprintf(buf, TEXT("limit_size-%d"), i);
		profile_write_int(TEXT("filter"), buf, (option.filter_info + i)->limit_size, ini_path);
	}

	// window filter
#ifdef OPTION_SET
	profile_write_string(TEXT("window_filter"), NULL, NULL, ini_path);
#endif	// OPTION_SET
	profile_write_int(TEXT("window_filter"), TEXT("cnt"), option.window_filter_cnt, ini_path);
	for (i = 0; i < option.window_filter_cnt; i++) {
		wsprintf(buf, TEXT("title-%d"), i);
		profile_write_string(TEXT("window_filter"), buf, (option.window_filter_info + i)->title, ini_path);
		wsprintf(buf, TEXT("class_name-%d"), i);
		profile_write_string(TEXT("window_filter"), buf, (option.window_filter_info + i)->class_name, ini_path);

		wsprintf(buf, TEXT("ignore-%d"), i);
		profile_write_int(TEXT("window_filter"), buf, (option.window_filter_info + i)->ignore, ini_path);
		wsprintf(buf, TEXT("focus-%d"), i);
		profile_write_int(TEXT("window_filter"), buf, (option.window_filter_info + i)->focus, ini_path);
		wsprintf(buf, TEXT("paste-%d"), i);
		profile_write_int(TEXT("window_filter"), buf, (option.window_filter_info + i)->paste, ini_path);
	}

	// send key
#ifdef OPTION_SET
	profile_write_string(TEXT("sendkey"), NULL, NULL, ini_path);
#endif	// OPTION_SET
	profile_write_int(TEXT("sendkey"), TEXT("def_copy_modifiers"), option.def_copy_modifiers, ini_path);
	profile_write_int(TEXT("sendkey"), TEXT("def_copy_virtkey"), option.def_copy_virtkey, ini_path);
	profile_write_int(TEXT("sendkey"), TEXT("def_copy_wait"), option.def_copy_wait, ini_path);

	profile_write_int(TEXT("sendkey"), TEXT("def_paste_modifiers"), option.def_paste_modifiers, ini_path);
	profile_write_int(TEXT("sendkey"), TEXT("def_paste_virtkey"), option.def_paste_virtkey, ini_path);
	profile_write_int(TEXT("sendkey"), TEXT("def_paste_wait"), option.def_paste_wait, ini_path);

	profile_write_int(TEXT("sendkey"), TEXT("cnt"), option.sendkey_cnt, ini_path);
	for (i = 0; i < option.sendkey_cnt; i++) {
		wsprintf(buf, TEXT("title-%d"), i);
		profile_write_string(TEXT("sendkey"), buf, (option.sendkey_info + i)->title, ini_path);
		wsprintf(buf, TEXT("class_name-%d"), i);
		profile_write_string(TEXT("sendkey"), buf, (option.sendkey_info + i)->class_name, ini_path);

		wsprintf(buf, TEXT("copy_modifiers-%d"), i);
		profile_write_int(TEXT("sendkey"), buf, (option.sendkey_info + i)->copy_modifiers, ini_path);
		wsprintf(buf, TEXT("copy_virtkey-%d"), i);
		profile_write_int(TEXT("sendkey"), buf, (option.sendkey_info + i)->copy_virtkey, ini_path);
		wsprintf(buf, TEXT("copy_wait-%d"), i);
		profile_write_int(TEXT("sendkey"), buf, (option.sendkey_info + i)->copy_wait, ini_path);

		wsprintf(buf, TEXT("paste_modifiers-%d"), i);
		profile_write_int(TEXT("sendkey"), buf, (option.sendkey_info + i)->paste_modifiers, ini_path);
		wsprintf(buf, TEXT("paste_virtkey-%d"), i);
		profile_write_int(TEXT("sendkey"), buf, (option.sendkey_info + i)->paste_virtkey, ini_path);
		wsprintf(buf, TEXT("paste_wait-%d"), i);
		profile_write_int(TEXT("sendkey"), buf, (option.sendkey_info + i)->paste_wait, ini_path);
	}

	// tool
#ifdef OPTION_SET
	profile_write_string(TEXT("tool"), NULL, NULL, ini_path);
#endif	// OPTION_SET
	profile_write_int(TEXT("tool"), TEXT("valid_interval"), option.tool_valid_interval, ini_path);
	profile_write_int(TEXT("tool"), TEXT("cnt"), option.tool_cnt, ini_path);
	for (i = 0; i < option.tool_cnt; i++) {
		wsprintf(buf, TEXT("title-%d"), i);
		profile_write_string(TEXT("tool"), buf, (option.tool_info + i)->title, ini_path);
		wsprintf(buf, TEXT("lib_file_path-%d"), i);
		profile_write_string(TEXT("tool"), buf, (option.tool_info + i)->lib_file_path, ini_path);
		wsprintf(buf, TEXT("func_name-%d"), i);
		profile_write_string(TEXT("tool"), buf, (option.tool_info + i)->func_name, ini_path);
		wsprintf(buf, TEXT("cmd_line-%d"), i);
		profile_write_string(TEXT("tool"), buf, (option.tool_info + i)->cmd_line, ini_path);
		wsprintf(buf, TEXT("call_type-%d"), i);
		profile_write_int(TEXT("tool"), buf, (option.tool_info + i)->call_type, ini_path);
		wsprintf(buf, TEXT("copy_paste-%d"), i);
		profile_write_int(TEXT("tool"), buf, (option.tool_info + i)->copy_paste, ini_path);

		wsprintf(buf, TEXT("modifiers-%d"), i);
		profile_write_int(TEXT("tool"), buf, (option.tool_info + i)->modifiers, ini_path);
		wsprintf(buf, TEXT("virtkey-%d"), i);
		profile_write_int(TEXT("tool"), buf, (option.tool_info + i)->virtkey, ini_path);

		wsprintf(buf, TEXT("old-%d"), i);
		profile_write_int(TEXT("tool"), buf, (option.tool_info + i)->old, ini_path);
	}

	// viewer
	profile_write_int(TEXT("viewer"), TEXT("toggle"), option.viewer_toggle, ini_path);
#ifndef OPTION_SET
	profile_write_int(TEXT("viewer"), TEXT("show_bin"), option.viewer_show_bin, ini_path);
	profile_write_int(TEXT("viewer"), TEXT("show_toolbar"), option.viewer_show_toolbar, ini_path);
	profile_write_int(TEXT("viewer"), TEXT("show_statusbar"), option.viewer_show_statusbar, ini_path);
	profile_write_int(TEXT("viewer"), TEXT("delete_confirm"), option.viewer_delete_confirm, ini_path);
	profile_write_int(TEXT("viewer"), TEXT("left"), UnScale(option.viewer_rect.left), ini_path);
	profile_write_int(TEXT("viewer"), TEXT("top"), UnScale(option.viewer_rect.top), ini_path);
	profile_write_int(TEXT("viewer"), TEXT("right"), UnScale(option.viewer_rect.right), ini_path);
	profile_write_int(TEXT("viewer"), TEXT("bottom"), UnScale(option.viewer_rect.bottom), ini_path);
	profile_write_int(TEXT("viewer"), TEXT("sep_size"), UnScale(option.viewer_sep_size), ini_path);
#endif	// OPTION_SET

	// treeview
#ifndef OPTION_SET
	profile_write_int(TEXT("treeview"), TEXT("show_format"), option.tree_show_format, ini_path);
#endif	// OPTION_SET
	profile_write_string(TEXT("treeview"), TEXT("root_order"), option.tree_root_order, ini_path);
	profile_write_int(TEXT("treeview"), TEXT("clip_expand"), option.tree_clip_expand, ini_path);
	profile_write_int(TEXT("treeview"), TEXT("history_expand"), option.tree_history_expand, ini_path);
	profile_write_int(TEXT("treeview"), TEXT("regist_expand"), option.tree_regist_expand, ini_path);
	profile_write_int(TEXT("treeview"), TEXT("folder_expand"), option.tree_folder_expand, ini_path);
	profile_write_string(TEXT("treeview"), TEXT("font_name"), option.tree_font_name, ini_path);
	profile_write_int(TEXT("treeview"), TEXT("font_size"), option.tree_font_size, ini_path);
	profile_write_int(TEXT("treeview"), TEXT("font_weight"), option.tree_font_weight, ini_path);
	profile_write_int(TEXT("treeview"), TEXT("font_italic"), option.tree_font_italic, ini_path);
	profile_write_int(TEXT("treeview"), TEXT("font_charset"), option.tree_font_charset, ini_path);

	// listview
	profile_write_int(TEXT("listview"), TEXT("default_action"), option.list_default_action, ini_path);
#ifndef OPTION_SET
	profile_write_int(TEXT("listview"), TEXT("column_data"), UnScale(option.list_column_data), ini_path);
	profile_write_int(TEXT("listview"), TEXT("column_size"), UnScale(option.list_column_size), ini_path);
	profile_write_int(TEXT("listview"), TEXT("column_date"), UnScale(option.list_column_date), ini_path);
	profile_write_int(TEXT("listview"), TEXT("column_window"), UnScale(option.list_column_window), ini_path);
#endif	// OPTION_SET
	profile_write_string(TEXT("listview"), TEXT("font_name"), option.list_font_name, ini_path);
	profile_write_int(TEXT("listview"), TEXT("font_size"), option.list_font_size, ini_path);
	profile_write_int(TEXT("listview"), TEXT("font_weight"), option.list_font_weight, ini_path);
	profile_write_int(TEXT("listview"), TEXT("font_italic"), option.list_font_italic, ini_path);
	profile_write_int(TEXT("listview"), TEXT("font_charset"), option.list_font_charset, ini_path);

	// bin view
#ifndef OPTION_SET
	profile_write_int(TEXT("binview"), TEXT("lock"), option.bin_lock, ini_path);
	profile_write_string(TEXT("binview"), TEXT("font_name"), option.bin_font_name, ini_path);
	profile_write_int(TEXT("binview"), TEXT("font_size"), option.bin_font_size, ini_path);
	profile_write_int(TEXT("binview"), TEXT("font_weight"), option.bin_font_weight, ini_path);
	profile_write_int(TEXT("binview"), TEXT("font_italic"), option.bin_font_italic, ini_path);
	profile_write_int(TEXT("binview"), TEXT("font_charset"), option.bin_font_charset, ini_path);
#endif	// OPTION_SET

	// text format
	profile_write_int(TEXT("fmt_text"), TEXT("menu_tooltip_size"), option.fmt_txt_menu_tooltip_size, ini_path);
#ifndef OPTION_SET
	profile_write_int(TEXT("fmt_text"), TEXT("viewer_word_wrap"), option.fmt_txt_viewer_word_wrap, ini_path);
	profile_write_int(TEXT("fmt_text"), TEXT("tab_size"), option.fmt_txt_tab_size, ini_path);
	profile_write_string(TEXT("fmt_text"), TEXT("font_name"), option.fmt_txt_font_name, ini_path);
	profile_write_int(TEXT("fmt_text"), TEXT("font_size"), option.fmt_txt_font_size, ini_path);
	profile_write_int(TEXT("fmt_text"), TEXT("font_weight"), option.fmt_txt_font_weight, ini_path);
	profile_write_int(TEXT("fmt_text"), TEXT("font_italic"), option.fmt_txt_font_italic, ini_path);
	profile_write_int(TEXT("fmt_text"), TEXT("font_charset"), option.fmt_txt_font_charset, ini_path);
#endif	// OPTION_SET

	// bitmap format
#ifndef OPTION_SET
	profile_write_int(TEXT("fmt_bitmap"), TEXT("stretch_mode"), option.fmt_bmp_stretch_mode, ini_path);
#endif	// OPTION_SET

	// file format
#ifndef OPTION_SET
	profile_write_int(TEXT("fmt_file"), TEXT("column_data"), UnScale(option.fmt_file_column_name), ini_path);
	profile_write_int(TEXT("fmt_file"), TEXT("column_size"), UnScale(option.fmt_file_column_folder), ini_path);
	profile_write_int(TEXT("fmt_file"), TEXT("column_type"), UnScale(option.fmt_file_column_type), ini_path);
#endif	// OPTION_SET
	profile_write_string(TEXT("fmt_file"), TEXT("font_name"), option.fmt_file_font_name, ini_path);
	profile_write_int(TEXT("fmt_file"), TEXT("font_size"), option.fmt_file_font_size, ini_path);
	profile_write_int(TEXT("fmt_file"), TEXT("font_weight"), option.fmt_file_font_weight, ini_path);
	profile_write_int(TEXT("fmt_file"), TEXT("font_italic"), option.fmt_file_font_italic, ini_path);
	profile_write_int(TEXT("fmt_file"), TEXT("font_charset"), option.fmt_file_font_charset, ini_path);

	profile_flush(ini_path);
	profile_free();
	return TRUE;
}

/*
 * ini_free_format_name - 形式名を解放
 */
void ini_free_format_name(FORMAT_NAME *fn, const int fn_cnt)
{
	int i;

	if (fn != NULL) {
		for (i = 0; i < fn_cnt; i++) {
			mem_free(&(fn + i)->format_name);
		}
		mem_free(&fn);
	}
}

/*
 * ini_free_menu - メニューオプションを解放
 */
void ini_free_menu(MENU_INFO *mi, const int mcnt)
{
	int i;

	if (mi == NULL) {
		return;
	}

	for (i = 0; i < mcnt; i++) {
		mem_free(&((mi + i)->title));
		mem_free(&((mi + i)->icon_path));
		mem_free(&((mi + i)->path));
		mem_free(&((mi + i)->cmd));

		if ((mi + i)->mi != NULL) {
			ini_free_menu((mi + i)->mi, (mi + i)->mi_cnt);
		}
	}
	mem_free(&mi);
}

/*
 * ini_free - オプションを解放
 */
BOOL ini_free(void)
{
	int i;

	// data
	mem_free(&option.data_date_format);
	mem_free(&option.data_time_format);

	// menu
	mem_free(&option.menu_text_format);
	mem_free(&option.menu_font_name);
#ifdef MENU_COLOR
	mem_free(&option.menu_color_back.color_str);
	mem_free(&option.menu_color_text.color_str);
	mem_free(&option.menu_color_highlight.color_str);
	mem_free(&option.menu_color_highlighttext.color_str);
	mem_free(&option.menu_color_3d_shadow.color_str);
	mem_free(&option.menu_color_3d_highlight.color_str);
#endif	// MENU_COLOR

	// tooltip
	mem_free(&option.tooltip_font_name);
#ifdef TOOLTIP_COLOR
	mem_free(&option.tooltip_color_back.color_str);
	mem_free(&option.tooltip_color_text.color_str);
#endif	// TOOLTIP_COLOR

	// action
	for (i = 0; i < option.action_cnt; i++) {
		ini_free_menu((option.action_info + i)->menu_info, (option.action_info + i)->menu_cnt);
	}
	mem_free(&option.action_info);
	option.action_cnt = 0;

	// format
	for (i = 0; i < option.format_cnt; i++) {
		mem_free(&((option.format_info + i)->format_name));
		ini_free_format_name((option.format_info + i)->fn, (option.format_info + i)->fn_cnt);
		mem_free(&((option.format_info + i)->lib_file_path));
		mem_free(&((option.format_info + i)->func_header));
		if ((option.format_info + i)->lib != NULL) {
			FreeLibrary((option.format_info + i)->lib);
		}
	}
	mem_free(&option.format_info);
	option.format_cnt = 0;

	// filter
	for (i = 0; i < option.filter_cnt; i++) {
		mem_free(&((option.filter_info + i)->format_name));
		ini_free_format_name((option.filter_info + i)->fn, (option.filter_info + i)->fn_cnt);
	}
	mem_free(&option.filter_info);
	option.filter_cnt = 0;

	// window filter
	for (i = 0; i < option.window_filter_cnt; i++) {
		mem_free(&((option.window_filter_info + i)->title));
		mem_free(&((option.window_filter_info + i)->class_name));
	}
	mem_free(&option.window_filter_info);
	option.window_filter_cnt = 0;

	// send key
	for (i = 0; i < option.sendkey_cnt; i++) {
		mem_free(&((option.sendkey_info + i)->title));
		mem_free(&((option.sendkey_info + i)->class_name));
	}
	mem_free(&option.sendkey_info);
	option.sendkey_cnt = 0;

	// tool
	for (i = 0; i < option.tool_cnt; i++) {
		mem_free(&((option.tool_info + i)->title));
		mem_free(&((option.tool_info + i)->lib_file_path));
		mem_free(&((option.tool_info + i)->func_name));
		mem_free(&((option.tool_info + i)->cmd_line));
		if ((option.tool_info + i)->lib != NULL) {
			FreeLibrary((option.tool_info + i)->lib);
		}
	}
	mem_free(&option.tool_info);
	option.tool_cnt = 0;

	// treeview
	mem_free(&option.tree_root_order);
	mem_free(&option.tree_font_name);

	// listview
	mem_free(&option.list_font_name);

	// bin view
	mem_free(&option.bin_font_name);

	// text format
	mem_free(&option.fmt_txt_font_name);

	// file format
	mem_free(&option.fmt_file_font_name);
	return TRUE;
}
/* End of source */
