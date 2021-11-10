/*
 * CLCL
 *
 * Frame.c
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef	_INC_OLE

#include "Memory.h"
#include "Frame.h"
#include "dpi.h"

/* Define */
#define NOMOVESIZE		Scale(6)		// フレームの移動制限値

/* Global Variables */
static RECT *frame_rect;		// フレームの位置情報

/* ocal Function Prototypes */

/*
 * frame_initialize - フレーム描画用構造体の初期化
 */
BOOL frame_initialize(const HWND hWnd)
{
	if (frame_rect == NULL) {
		frame_rect = (RECT *)mem_calloc(sizeof(RECT) * FRAME_CNT);
		if (frame_rect == NULL) {
			return FALSE;
		}
	}
	SetCapture(hWnd);
	return TRUE;
}

/*
 * frame_free - フレーム描画用構造体の解放
 */
void frame_free(void)
{
	if (frame_rect != NULL) {
		mem_free(&frame_rect);
	}
	ReleaseCapture();
}

/*
 * frame_draw - フレームの描画
 */
int frame_draw(const HWND hWnd, const HWND hTreeView)
{
	RECT window_rect, treeview_rect;
	POINT apos;
	HDC hdc;
	int draw_cnt;

	GetCursorPos((LPPOINT)&apos);
	GetWindowRect(hWnd, (LPRECT)&window_rect);
	GetWindowRect(hTreeView, (LPRECT)&treeview_rect);

	// フレームの移動制限
	if (apos.x <= (window_rect.left + NOMOVESIZE + GetSystemMetrics(SM_CXFRAME))) {
		apos.x = window_rect.left + NOMOVESIZE + GetSystemMetrics(SM_CXFRAME);

	} else if (apos.x >= (window_rect.right - (NOMOVESIZE + (FRAME_CNT * 2)) - GetSystemMetrics(SM_CXFRAME))) {
		apos.x = window_rect.right - (NOMOVESIZE + (FRAME_CNT * 2)) - GetSystemMetrics(SM_CXFRAME);
	}

	// 前回の位置と比較
	if (apos.x == frame_rect[0].left) {
		return 1;
	}

	hdc = GetWindowDC(hWnd);

	// 前回描画分を消去
	for (draw_cnt = 0;draw_cnt < FRAME_CNT;draw_cnt++) {
		DrawFocusRect(hdc, (LPRECT)&frame_rect[draw_cnt]);
	}

	// フレームの描画
	for (draw_cnt = 0;draw_cnt < FRAME_CNT;draw_cnt++) {
		(frame_rect + draw_cnt)->left = apos.x + draw_cnt - window_rect.left;
		(frame_rect + draw_cnt)->right = (frame_rect + draw_cnt)->left + FRAME_CNT + 1;
		(frame_rect + draw_cnt)->top = treeview_rect.top - window_rect.top;
		(frame_rect + draw_cnt)->bottom = treeview_rect.bottom - window_rect.top;

		DrawFocusRect(hdc, (LPRECT)(frame_rect + draw_cnt));
	}
	ReleaseDC(hWnd, hdc);
	return 0;
}

/*
 * frame_draw_end - フレームの描画終了、フレームの最終位置を返す
 */
int frame_draw_end(const HWND hWnd)
{
	HDC hdc;
	int draw_cnt;
	int ret;

	if (frame_rect[0].left == 0 && frame_rect[0].right == 0 &&
		frame_rect[0].top == 0 && frame_rect[0].bottom == 0) {
		frame_free();
		return -1;
	}

	// 前回描画分を消去
	hdc = GetWindowDC(hWnd);
	for (draw_cnt = 0;draw_cnt < FRAME_CNT;draw_cnt++) {
		DrawFocusRect(hdc, (LPRECT)&frame_rect[draw_cnt]);
	}
	ReleaseDC(hWnd, hdc);

	// 境界位置の取得
	ret = frame_rect[0].left - GetSystemMetrics(SM_CXFRAME);

	frame_free();
	return ret;
}
/* End of source */
