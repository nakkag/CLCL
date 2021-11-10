/*
 * CLCL
 *
 * fmt_text_view.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_FMT_TEXT_VIEW_H
#define _INC_FMT_TEXT_VIEW_H

/* Include Files */
#include <windows.h>
#include <tchar.h>
#ifdef OP_XP_STYLE
#include <uxtheme.h>
#include <vssym32.h>
#endif	// OP_XP_STYLE

/* Define */
#define NEDIT_WND_CLASS					TEXT("nEdit")

#define WM_GETBUFFERINFO				(WM_APP + 1)
#define WM_REFLECT						(WM_APP + 2)
#define WM_GETWORDWRAP					(WM_APP + 3)
#define WM_SETWORDWRAP					(WM_APP + 4)
#define WM_GETMEMSIZE					(WM_APP + 5)
#define WM_GETMEM						(WM_APP + 6)
#define WM_SETMEM						(WM_APP + 7)

#define EM_GETREADONLY					(WM_APP + 100)

#ifndef EM_REDO
#define EM_REDO							(WM_USER + 84)
#endif
#ifndef EM_CANREDO
#define EM_CANREDO						(WM_USER + 85)
#endif

#define UNDO_TYPE_INPUT					1
#define UNDO_TYPE_DELETE				2

/* Struct */
typedef struct _UNDO {
	BYTE type;

	DWORD st;
	DWORD len;
	TCHAR *buf;
} UNDO;

typedef struct _BUFFER {
	// 保持している内容
	TCHAR *buf;
	DWORD buf_size;
	DWORD buf_len;

	// 入力バッファ
	TCHAR *input_buf;
	DWORD input_size;
	DWORD input_len;

	// 表示行頭のオフセット
	DWORD *line;
	int line_size;
	int line_len;
	int line_add_index;
	int line_add_len;

	// UNDOバッファ
	UNDO *undo;
	int undo_size;
	int undo_len;
	int undo_pos;

	// 入力開始位置
	TCHAR *ip;
	DWORD ip_len;
	// 削除開始位置
	TCHAR *dp;
	DWORD del_len;

	// キャレットの位置
	DWORD cp;
	// 選択位置
	DWORD sp;
	// 上下移動時のキャレットのX座標
	int cpx;

	// 1行の文字数
	int line_max;
	// 行の最大幅
	int line_width;
	// ウィンドウの幅
	int width;

	// コントロール識別子
	int id;

	// スクロール バー
	int pos_x;
	int max_x;
	int pos_y;
	int max_y;

	// タブストップ
	int tab_stop;
	// 左マージン
	int left_margin;
	// 上マージン
	int top_margin;
	// 右マージン
	int right_margin;
	// 下マージン
	int bottom_margin;
	// 行間
	int spacing;

	// 描画用情報
	HDC mdc;
	HBITMAP ret_bmp;
	HRGN hrgn;
	HFONT hfont;
	HFONT ret_font;

	// フォント
	int font_height;
	int char_width;

	// IME
	HIMC himc;

	// 折り返しフラグ
	BOOL wordwrap;
	// フォーカスが無くても選択表示
	BOOL no_hide_sel;
	// 小文字に変換
	BOOL lowercase;
	// 大文字に変換
	BOOL uppercase;
	// ロックフラグ
	BOOL lock;
	// 修正フラグ
	BOOL modified;
	// 入力モード
	BOOL insert_mode;
	// 選択フラグ
	BOOL sel;
	// マウス情報フラグ
	BOOL mousedown;
	// 入力長制限
	DWORD limit_len;

	// 文字の幅
	BYTE cwidth[256];

#ifdef OP_XP_STYLE
	// XP
	HMODULE hModThemes;
	HTHEME hTheme;
#endif	// OP_XP_STYLE
} BUFFER;

/* Function Prototypes */
BOOL txtview_regist(const HINSTANCE hInstance);

#endif
/* End of source */
