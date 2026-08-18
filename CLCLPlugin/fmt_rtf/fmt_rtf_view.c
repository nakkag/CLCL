/*
 * CLCL
 *
 * fmt_text_view.c
 *
 * Copyright (C) 1996-2003 by Nakashima Tomoaki. All rights reserved.
 *		http://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE
#include <richedit.h>

#include "fmt_rtf_view.h"

#include "resource.h"

/* Define */
#define WINDOW_CLASS			TEXT("CLCLFmtRTFView")

#define IDC_EDIT_TEXT			100

#define ID_MENU_TIMER			1

#define MENU_SELECT_ALL			(WM_APP + 1)
#define MENU_SELECT_FONT		(WM_APP + 2)

/* Global Variables */
static LONG src_len;

/* Local Function Prototypes */

/*
 * font_select - フォントの選択
 */
static BOOL font_select(const HWND hWnd)
{
    CHARFORMAT cfm;
	CHOOSEFONT cf;
	LOGFONT lf;
	HDC hdc;

	// 設定取得
	ZeroMemory(&cfm, sizeof(cfm));
	cfm.cbSize = sizeof(cfm);
	cfm.dwMask = CFM_SIZE | CFM_FACE | CFM_CHARSET | CFM_COLOR | CFM_BOLD | CFM_ITALIC | CFM_STRIKEOUT | CFM_UNDERLINE;
	SendMessage(GetDlgItem(hWnd, IDC_EDIT_TEXT), EM_GETCHARFORMAT, TRUE, (LPARAM)&cfm);

	// フォント情報の作成
	ZeroMemory(&lf, sizeof(LOGFONT));
	hdc = GetDC(NULL);
	lf.lfHeight = -(int)(((cfm.yHeight / 20) * GetDeviceCaps(hdc, LOGPIXELSY)) / 72);
	ReleaseDC(NULL, hdc);
	if (cfm.dwEffects & CFE_BOLD) {
		lf.lfWeight = 700;
	}
	if (cfm.dwEffects & CFE_ITALIC){
		lf.lfItalic = TRUE;
	}
	if (cfm.dwEffects & CFE_UNDERLINE){
		lf.lfUnderline = TRUE;
	}
	if (cfm.dwEffects & CFE_STRIKEOUT){
		lf.lfStrikeOut = TRUE;
	}
	lf.lfCharSet = cfm.bCharSet;
	lf.lfPitchAndFamily = cfm.bPitchAndFamily;
	lstrcpy(lf.lfFaceName, cfm.szFaceName);

	// フォント選択ダイアログを表示
	ZeroMemory(&cf, sizeof(CHOOSEFONT));
	cf.lStructSize = sizeof(CHOOSEFONT);
	cf.hwndOwner = hWnd;
	cf.lpLogFont = &lf;
	cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_EFFECTS;
	cf.nFontType = SCREEN_FONTTYPE;
	cf.rgbColors = cfm.crTextColor;
	if (ChooseFont(&cf) == FALSE) {
		return FALSE;
	}

	// フォント情報を反映
	ZeroMemory(&cfm, sizeof(cfm));
	cfm.cbSize = sizeof(cfm);
	cfm.dwMask = CFM_SIZE | CFM_FACE | CFM_CHARSET | CFM_COLOR | CFM_BOLD | CFM_ITALIC | CFM_STRIKEOUT | CFM_UNDERLINE;
	if (lf.lfWeight >= 700) {
	    cfm.dwEffects = cfm.dwEffects | CFE_BOLD;
	}
	if (lf.lfItalic == TRUE) {
	    cfm.dwEffects = cfm.dwEffects | CFE_ITALIC;
	}
	if (lf.lfUnderline == TRUE) {
	    cfm.dwEffects = cfm.dwEffects | CFE_UNDERLINE;
	}
	if (lf.lfStrikeOut == TRUE) {
	    cfm.dwEffects = cfm.dwEffects | CFE_STRIKEOUT;
	}
	cfm.crTextColor = cf.rgbColors;
	cfm.yHeight = cf.iPointSize / 10 * 20;
	cfm.bCharSet = lf.lfCharSet;
	cfm.bPitchAndFamily = lf.lfPitchAndFamily;
	lstrcpy(cfm.szFaceName, lf.lfFaceName);
	SendMessage(GetDlgItem(hWnd, IDC_EDIT_TEXT), EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfm);
	return TRUE;
}

/*
 * rtfview_stream_in_proc - RTF設定プロシージャ
 */
static DWORD CALLBACK rtfview_stream_in_proc(DWORD dwCookie, LPBYTE pbBuf, LONG cb, LONG *pcb)
{
	strncpy((char *)pbBuf, (char *)dwCookie + src_len, cb);
	*pcb = cb;
	src_len += *pcb;
    return 0;
}

/*
 * rtfview_stream_out_proc - RTF取得プロシージャ
 */
static DWORD CALLBACK rtfview_stream_out_proc(DWORD dwCookie, LPBYTE pbBuf, LONG cb, LONG *pcb)
{
	HLOCAL hloc = (HLOCAL)dwCookie;
	char *p;

	if ((hloc = LocalReAlloc(hloc, src_len + cb + 1, LMEM_MOVEABLE)) == NULL) {
		return 1;
	}
	if ((p = LocalLock(hloc)) == NULL) {
		return 1;
	}
	strcpy(p + src_len, (char *)pbBuf);
	LocalUnlock(hloc);
	*pcb = cb;
 	src_len += cb;
   return 0;
}

/*
 * rtfview_proc - ウィンドウのプロシージャ
 */
static LRESULT CALLBACK rtfview_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	RECT window_rect;
	POINT apos;
	HMENU hMenu;
	EDITSTREAM eds;
	HLOCAL hloc;
	DWORD ret;
	DWORD st, en;

	switch (msg) {
	case WM_CREATE:
		// RichEditの作成
		if (CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("RICHEDIT"), NULL,
			WS_TABSTOP | WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
			ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_SAVESEL | ES_NOHIDESEL | ES_DISABLENOSCROLL | ES_WANTRETURN,
			0, 0, 0, 0, hWnd, (HMENU)IDC_EDIT_TEXT, ((LPCREATESTRUCT)lParam)->hInstance, NULL) == NULL) {
			DestroyWindow(hWnd);
			break;
		}
		SendMessage(GetDlgItem(hWnd, IDC_EDIT_TEXT), EM_EXLIMITTEXT, 0, 0xFFFFFF);
		break;

	case WM_CLOSE:
		// ウィンドウを閉じる
		DestroyWindow(hWnd);
		break;

	case WM_DESTROY:
		if (GetDlgItem(hWnd, IDC_EDIT_TEXT) != NULL) {
			DestroyWindow(GetDlgItem(hWnd, IDC_EDIT_TEXT));
		}
		// ウィンドウの破棄
		return DefWindowProc(hWnd, msg, wParam, lParam);

	case WM_SIZE:
		// サイズ変更
		GetClientRect(hWnd, (LPRECT)&window_rect);
		MoveWindow(GetDlgItem(hWnd, IDC_EDIT_TEXT), 0, 0, window_rect.right, window_rect.bottom, TRUE);
		break;

	case WM_EXITSIZEMOVE:
		// サイズ変更完了
		break;

	case WM_SETFOCUS:
		SetFocus(GetDlgItem(hWnd, IDC_EDIT_TEXT));
		break;

	case WM_PARENTNOTIFY:
		if (LOWORD(wParam) == WM_RBUTTONDOWN) {
			SetTimer(hWnd, ID_MENU_TIMER, 100, NULL);
		}
		break;

	case WM_CONTEXTMENU:
		SetTimer(hWnd, ID_MENU_TIMER, 100, NULL);
		break;

	case WM_TIMER:
		// タイマー
		switch (wParam) {
		case ID_MENU_TIMER:
			if (GetKeyState(VK_RBUTTON) >= 0) {
				KillTimer(hWnd, wParam);

				// メニューの作成
				hMenu = CreatePopupMenu();
				AppendMenu(hMenu, MF_STRING | (SendMessage(GetDlgItem(hWnd, IDC_EDIT_TEXT), EM_CANUNDO, 0, 0) == TRUE) ? 0 : MF_GRAYED,
					WM_UNDO, TEXT("&Undo"));
				AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
				SendMessage(GetDlgItem(hWnd, IDC_EDIT_TEXT), EM_GETSEL, (WPARAM)&st, (LPARAM)&en);
				AppendMenu(hMenu, MF_STRING | (st != en) ? 0 : MF_GRAYED, WM_CUT, TEXT("Cu&t"));
				AppendMenu(hMenu, MF_STRING | (st != en) ? 0 : MF_GRAYED, WM_COPY, TEXT("&Copy"));
				AppendMenu(hMenu, MF_STRING, WM_PASTE, TEXT("&Paste"));
				AppendMenu(hMenu, MF_STRING | (st != en) ? 0 : MF_GRAYED, WM_CLEAR, TEXT("&Delete"));
				AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
				AppendMenu(hMenu, MF_STRING, MENU_SELECT_ALL, TEXT("Select &All"));
				AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
				AppendMenu(hMenu, MF_STRING, MENU_SELECT_FONT, TEXT("&Font..."));

				// メニューの表示
				GetCursorPos((LPPOINT)&apos);
				ret = TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_RETURNCMD, apos.x, apos.y, 0, hWnd, NULL);
				DestroyMenu(hMenu);
				if (ret <= 0) {
					break;
				}
				switch (ret) {
				case MENU_SELECT_ALL:
					// すべて選択
					SendMessage(GetDlgItem(hWnd, IDC_EDIT_TEXT), EM_SETSEL, 0, -1);
					break;

				case MENU_SELECT_FONT:
					// フォント選択
					font_select(hWnd);
					break;

				default:
					SendMessage(GetDlgItem(hWnd, IDC_EDIT_TEXT), ret, 0, 0);
					break;
				}
			}
			break;
		}
		break;

	case WM_SETRTF:
		if (lParam == 0) {
			SendMessage(GetDlgItem(hWnd, IDC_EDIT_TEXT), WM_SETTEXT, 0, (LPARAM)TEXT(""));
			break;
		}
		src_len = 0;
		eds.dwCookie = (DWORD)lParam;
		eds.dwError = 0;
		eds.pfnCallback = rtfview_stream_in_proc;
		SendMessage(GetDlgItem(hWnd, IDC_EDIT_TEXT), EM_STREAMIN, SF_RTF, (LPARAM)&eds);
		break;

	case WM_GETRTF:
		if((hloc = LocalAlloc(LMEM_MOVEABLE, 1)) == NULL) {
			break;
		}
		src_len = 0;
		eds.dwCookie = (DWORD)hloc;
		eds.dwError = 0;
		eds.pfnCallback = rtfview_stream_out_proc;
		SendMessage(GetDlgItem(hWnd, IDC_EDIT_TEXT), EM_STREAMOUT, SF_RTF, (LPARAM)&eds);
		if (lParam != 0) {
			*(int *)lParam = src_len;
		}
		return (LRESULT)hloc;

	case EM_GETOPTIONS:
	case EM_SETOPTIONS:
	case EM_GETMODIFY:
	case EM_SETMODIFY:
	case EM_GETSEL:
	case EM_SETSEL:
	case EM_STREAMIN:
	case EM_STREAMOUT:
	case EM_GETCHARFORMAT:
	case EM_SETCHARFORMAT:
	case WM_GETTEXT:
	case WM_GETTEXTLENGTH:
	case WM_SETTEXT:
	case WM_UNDO:
	case WM_CUT:
	case WM_COPY:
	case WM_PASTE:
	case WM_CLEAR:
		return SendMessage(GetDlgItem(hWnd, IDC_EDIT_TEXT), msg, wParam, lParam);

	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

/*
 * rtfview_regist - ウィンドウクラスの登録
 */
BOOL rtfview_regist(const HINSTANCE hInstance)
{
	WNDCLASS wc;

	wc.style = 0;
	wc.lpfnWndProc = (WNDPROC)rtfview_proc;
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
 * rtfview_create - ビューアの作成
 */
HWND rtfview_create(const HINSTANCE hInstance, const HWND pWnd, int id)
{
	HWND hWnd;

	// ウィンドウの作成
	hWnd = CreateWindow(WINDOW_CLASS,
		TEXT(""),
		WS_TABSTOP | WS_CHILD,
		0, 0, 0, 0, pWnd, (HMENU)id, hInstance, NULL);
	return hWnd;
}
/* End of source */
