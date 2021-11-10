/*
 * CLCL
 *
 * Ini.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_INI_H
#define _INC_INI_H

/* Include Files */

#include "Menu.h"
#include "Format.h"
#include "Filter.h"
#include "Window.h"
#include "SendKey.h"
#include "Tool.h"

/* Define */
#define ACTION_POPUPMEMU				0
#define ACTION_VIEWER					1
#define ACTION_OPTION					2
#define ACTION_CLIPBOARD_WATCH			3
#define ACTION_EXIT						4

#define ACTION_TYPE_HOTKEY				0
#define ACTION_TYPE_CTRL_CTRL			1
#define ACTION_TYPE_SHIFT_SHIFT			2
#define ACTION_TYPE_ALT_ALT				3
#define ACTION_TYPE_TRAY_LEFT			4
#define ACTION_TYPE_TRAY_LEFT_DBLCLK	5
#define ACTION_TYPE_TRAY_RIGHT			6
#define ACTION_TYPE_TRAY_RIGHT_DBLCLK	7

#define OPTION_SHOW_HISTORY				TEXT("0")
#define OPTION_SHOW_MENU				TEXT("1")
#define OPTION_SHOW_VIEWER				TEXT("2")
#define OPTION_SHOW_ACTION				TEXT("3")
#define OPTION_SHOW_FORMAT				TEXT("4")
#define OPTION_SHOW_FILTER				TEXT("5")
#define OPTION_SHOW_WINDOW				TEXT("6")
#define OPTION_SHOW_SENDKEY				TEXT("7")
#define OPTION_SHOW_TOOL				TEXT("8")

/* Struct */
// color info
typedef struct _COLOR_INFO {
	TCHAR *color_str;
	int color;
} COLOR_INFO;

// action info
typedef struct _ACTION_INFO {
	int action;							// ACTION_
	int type;							// ACTION_TYPE_

	int enable;							// 0-無効 1-有効
	int caret;							// Caret位置に表示

	// hot key
	int id;
	UINT modifiers;
	UINT virtkey;
	int paste;

	struct _MENU_INFO *menu_info;
	int menu_cnt;
} ACTION_INFO;

// option
typedef struct _OPTION_INFO {
	// main
	int main_clipboard_watch;			// クリップボード監視
	int main_clipboard_rechain_minute;	// クリップボード再監視時間
	int main_show_trayicon;				// タスクトレイにアイコンを表示
	int main_show_viewer;				// 起動時にビューアを表示

	// data
	TCHAR *data_date_format;
	TCHAR *data_time_format;

	// history
	int history_add_interval;			// 履歴に追加するまでのインターバル
	int history_save;					// 終了時に履歴を保存
	int history_always_save;			// 変更があれば常に保存
	int history_delete;					// クリップボードに送ると履歴から削除
	int history_max;					// 履歴件数
	int history_overlap_check;			// 履歴の重複チェック 0-チェックしない 1-最近の１件のみチェック 2-全ての履歴をチェック
	int history_ignore_regist_item;		// 登録アイテムをクリップボードに送った時に履歴に入れない

	// menu
	TCHAR *menu_text_format;			// メニュータイトルの表示形式
	int menu_intact_item_title;			// アイテムタイトルがある場合はそのまま表示する
	int menu_text_margin_left;			// テキストの左マージン
	int menu_text_margin_right;			// テキストの右マージン
	int menu_text_margin_y;				// テキストの上下マージン
	int menu_separator_height;			// 区切りの高さ
	int menu_separator_margin_left;		// 区切りの左マージン
	int menu_separator_margin_right;	// 区切りの右マージン
	int menu_max_width;					// メニューの最大幅
	int menu_break;						// 画面からはみ出た場合の折り返し
	int menu_show_icon;					// アイコン表示
	int menu_icon_size;					// アイコンのサイズ
	int menu_icon_margin;				// アイコンのマージン
	int menu_show_bitmap;				// ビットマップ表示
	int menu_bitmap_width;				// ビットマップの横幅
	int menu_bitmap_height;				// ビットマップの縦幅
	int menu_show_tooltip;				// ツールチップ表示
	int menu_show_hotkey;				// ホットキーを表示
	int menu_show_tool_menu;			// ツールメニュー表示
#ifdef MENU_LAYERER
	int menu_alpha;						// 透明度
#endif
	TCHAR *menu_font_name;				// メニューのフォント
	int menu_font_size;
	int menu_font_weight;
	int menu_font_italic;
	int menu_font_charset;
#ifdef MENU_COLOR
	COLOR_INFO menu_color_back;			// メニューの色
	COLOR_INFO menu_color_text;
	COLOR_INFO menu_color_highlight;
	COLOR_INFO menu_color_highlighttext;
	COLOR_INFO menu_color_3d_shadow;
	COLOR_INFO menu_color_3d_highlight;
#endif

	// tooltip
	int tooltip_show_delay;				// ツールチップを表示するまでの待ち時間(ミリ秒)
	int tooltip_tab_length;				// ツールチップのタブ幅
	int tooltip_margin_x;				// ツールチップ内に表示する文字の横マージン
	int tooltip_margin_y;				// ツールチップ内に表示する文字の縦マージン
	TCHAR *tooltip_font_name;			// ツールチップのフォント
	int tooltip_font_size;
	int tooltip_font_weight;
	int tooltip_font_italic;
	int tooltip_font_charset;
#ifdef TOOLTIP_COLOR
	COLOR_INFO tooltip_color_back;		// ツールチップの色
	COLOR_INFO tooltip_color_text;
#endif

	// action
	int action_double_press_time;		// CTRL or Shift or Altの1回目と2回目のキーを押す間に経過する時間(ミリ秒)
	int action_show_hotkey_error;		// ホットキーのエラーを表示
	ACTION_INFO *action_info;
	int action_cnt;

	// format
	FORMAT_INFO *format_info;
	int format_cnt;

	// filter
	int filter_all_action;
	FILTER_INFO *filter_info;
	int filter_cnt;

	// window filter
	WINDOW_FILTER_INFO *window_filter_info;
	int window_filter_cnt;

	// send key
	UINT def_copy_modifiers;
	UINT def_copy_virtkey;
	int def_copy_wait;					// コピーまでの待ち時間

	UINT def_paste_modifiers;
	UINT def_paste_virtkey;
	int def_paste_wait;					// 貼り付けまでの待ち時間

	SENDKEY_INFO *sendkey_info;
	int sendkey_cnt;

	// tool
	int tool_valid_interval;			// 動作メニューからの選択でツールが有効な時間
	TOOL_INFO *tool_info;
	int tool_cnt;

	// viewer
	int viewer_toggle;					// 表示をトグルする
	int viewer_show_bin;				// 常にバイナリ表示
	int viewer_show_toolbar;
	int viewer_show_statusbar;
	int viewer_delete_confirm;			// 削除確認 0-しない 1-する
	RECT viewer_rect;
	int viewer_sep_size;

	// treeview
	int tree_show_format;				// ツリーに形式を表示
	TCHAR *tree_root_order;				// ルートの表示と並び順 0-clip 1-history 2-regist
	int tree_clip_expand;				// クリップボードを展開
	int tree_history_expand;			// 履歴を展開
	int tree_regist_expand;				// 登録アイテムを展開
	int tree_folder_expand;				// フォルダを展開
	TCHAR *tree_font_name;
	int tree_font_size;
	int tree_font_weight;
	int tree_font_italic;
	int tree_font_charset;

	// listview
	int list_default_action;			// デフォルト動作 0-表示 1-クリップボードに送る 2-名前を付けて保存
	int list_column_data;
	int list_column_size;
	int list_column_date;
	int list_column_window;
	TCHAR *list_font_name;
	int list_font_size;
	int list_font_weight;
	int list_font_italic;
	int list_font_charset;

	// bin view
	int bin_lock;
	TCHAR *bin_font_name;
	int bin_font_size;
	int bin_font_weight;
	int bin_font_italic;
	int bin_font_charset;

	// text format
	int fmt_txt_menu_tooltip_size;		// ツールチップに表示する文字数
	int fmt_txt_viewer_word_wrap;		// 右端で折り返す
	int fmt_txt_tab_size;				// TABサイズ
	TCHAR *fmt_txt_font_name;
	int fmt_txt_font_size;
	int fmt_txt_font_weight;
	int fmt_txt_font_italic;
	int fmt_txt_font_charset;

	// bitmap format
	int fmt_bmp_stretch_mode;			// 画像をウィンドウの大きさに合わせて表示

	// file format
	int fmt_file_column_name;
	int fmt_file_column_folder;
	int fmt_file_column_type;
	TCHAR *fmt_file_font_name;
	int fmt_file_font_size;
	int fmt_file_font_weight;
	int fmt_file_font_italic;
	int fmt_file_font_charset;
} OPTION_INFO;

/* Function Prototypes */
BOOL ini_get_option(TCHAR *err_str);
BOOL ini_put_option(void);
void ini_free_format_name(FORMAT_NAME *fn, const int fn_cnt);
void ini_free_menu(MENU_INFO *mi, const int mcnt);
BOOL ini_free(void);

#endif
/* End of source */
