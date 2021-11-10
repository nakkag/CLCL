/*
 * CLCL
 *
 * Container.c
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE

#include "Viewer.h"
#include "Container.h"

/* Define */
#define WINDOW_CLASS					TEXT("CLCLContainer")

/* Global Variables */

/* Local Function Prototypes */

/*
 * binview_proc - ウィンドウのプロシージャ
 */
static LRESULT CALLBACK container_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	RECT window_rect;
	HWND cWnd;

	switch (msg) {
	case WM_CREATE:
		// ウィンドウ作成
		break;

	case WM_CLOSE:
		// ウィンドウを閉じる
		DestroyWindow(hWnd);
		break;

	case WM_DESTROY:
		// ウィンドウの破棄
		return DefWindowProc(hWnd, msg, wParam, lParam);

	case WM_SETFOCUS:
		if (wParam != 0 && (HWND)wParam != GetDlgItem(GetParent(hWnd), ID_TREE)) {
			SetFocus(GetDlgItem(GetParent(hWnd), ID_TREE));
			break;
		}
		cWnd = GetWindow(hWnd, GW_CHILD);
		while (cWnd != NULL) {
			if (IsWindowVisible(cWnd) == TRUE) {
				SetFocus(cWnd);
				break;
			}
			cWnd = GetWindow(cWnd, GW_HWNDNEXT);
		}
		break;

	case WM_KILLFOCUS:
		break;

	case WM_SIZE:
		// サイズ変更
		GetClientRect(hWnd, (LPRECT)&window_rect);

		// 子ウィンドウを検索してサイズを設定
		cWnd = GetWindow(hWnd, GW_CHILD);
		while (cWnd != NULL) {
			if (IsWindowVisible(cWnd) == TRUE) {
				MoveWindow(cWnd, 0, 0,
					window_rect.right, window_rect.bottom, TRUE);
				UpdateWindow(cWnd);
			} else {
				MoveWindow(cWnd, 0, 0,
					window_rect.right, window_rect.bottom, FALSE);
			}
			cWnd = GetWindow(cWnd, GW_HWNDNEXT);
		}
		break;

	case WM_NOTIFY:
		return SendMessage(GetParent(hWnd), WM_NOTIFY, wParam, lParam);

	case WM_ALLHIDE:
		cWnd = GetWindow(hWnd, GW_CHILD);
		while (cWnd != NULL) {
			ShowWindow(cWnd, SW_HIDE);
			cWnd = GetWindow(cWnd, GW_HWNDNEXT);
		}
		break;

	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

/*
 * container_regist - ウィンドウクラスの登録
 */
BOOL container_regist(const HINSTANCE hInstance)
{
	WNDCLASS wc;

	wc.style = 0;
	wc.lpfnWndProc = (WNDPROC)container_proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = WINDOW_CLASS;
	// ウィンドウクラスの登録
	return RegisterClass(&wc);
}

/*
 * container_create - バイナリビューアの作成
 */
HWND container_create(const HINSTANCE hInstance, const HWND pWnd, int id)
{
	HWND hWnd;

	// ウィンドウの作成
	hWnd = CreateWindow(WINDOW_CLASS,
		TEXT(""),
		WS_CHILD | WS_VISIBLE | WS_TABSTOP,
		0, 0, 0, 0, pWnd, (HMENU)id, hInstance, NULL);
	return hWnd;
}
/* End of source */
