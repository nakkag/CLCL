/*
 * CLCL
 *
 * SendKey.c
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
#include "SendKey.h"

/* Define */
#define SC_LSHIFT					0x2A
#define SC_RSHIFT					0x36
#define SC_LCONTROL					0x1D
#define SC_RCONTROL					0x1D
#define SC_LALT						0x38
#define SC_RALT						0x38
#define SC_LWIN						0x5B
#define SC_RWIN						0x5C

#define SC_PAUSE					0x45

#define SC_NUMPADDEL				0x53
#define SC_NUMPADINS				0x52
#define SC_NUMPADEND				0x4F
#define SC_NUMPADHOME				0x47
#define SC_NUMPADCLEAR				0x4C
#define SC_NUMPADUP					0x48
#define SC_NUMPADDOWN				0x50
#define SC_NUMPADLEFT				0x4B
#define SC_NUMPADRIGHT				0x4D
#define SC_NUMPADPGUP				0x49
#define SC_NUMPADPGDN				0x51

#define SC_NUMPAD0					SC_NUMPADINS
#define SC_NUMPAD1					SC_NUMPADEND
#define SC_NUMPAD2					SC_NUMPADDOWN
#define SC_NUMPAD3					SC_NUMPADPGDN
#define SC_NUMPAD4					SC_NUMPADLEFT
#define SC_NUMPAD5					SC_NUMPADCLEAR
#define SC_NUMPAD6					SC_NUMPADRIGHT
#define SC_NUMPAD7					SC_NUMPADHOME
#define SC_NUMPAD8					SC_NUMPADUP
#define SC_NUMPAD9					SC_NUMPADPGUP
#define SC_NUMPADDOT				SC_NUMPADDEL

#define SC_NUMLOCK					0x45
#define SC_NUMPADDIV				0x35
#define SC_NUMPADMULT				0x37
#define SC_NUMPADSUB				0x4A
#define SC_NUMPADADD				0x4E

#define SC_PRINTSCREEN				0x37

/* Global Variables */
// オプション
extern OPTION_INFO option;

/* Local Function Prototypes */
static int sendkey_window_to_index(const HWND hWnd, const int st);
static void sendkey_send(const int wait, const UINT modifiers, const UINT virtkey);

/*
 * sendkey_window_to_index - ウィンドウにマッチするインデックスを取得
 */
static int sendkey_window_to_index(const HWND hWnd, const int st)
{
	TCHAR title[BUF_SIZE];
	TCHAR class_name[BUF_SIZE];
	int i;

	if (option.sendkey_cnt <= 0) {
		return -1;
	}

	*title = TEXT('\0');
	GetWindowText(hWnd, title, BUF_SIZE - 1);
	*class_name = TEXT('\0');
	GetClassName(hWnd, class_name, BUF_SIZE - 1);

	for (i = st + 1; i < option.sendkey_cnt; i++) {
		// ウィンドウのタイトルとクラス名のチェック
		if (((option.sendkey_info + i)->title == NULL ||
			*(option.sendkey_info + i)->title == TEXT('\0') ||
			str_match((option.sendkey_info + i)->title, title) == TRUE) &&

			((option.sendkey_info + i)->class_name == NULL ||
			*(option.sendkey_info + i)->class_name == TEXT('\0') ||
			str_match((option.sendkey_info + i)->class_name, class_name) == TRUE)) {
			return i;
		}
	}
	return -1;
}

/*
 * get_extended_key - KEYEVENTF_EXTENDEDKEYの付加チェック
 */
static int get_extended_key(BYTE vk)
{
	switch (vk) {
	case VK_APPS:
	case VK_CANCEL:
	case VK_DIVIDE:
	case VK_NUMLOCK:
	case VK_LWIN:
	case VK_LMENU:
	case VK_LCONTROL:
	case VK_LSHIFT:
	case VK_RWIN:
	case VK_RMENU:
	case VK_RCONTROL:
	case VK_RSHIFT:
	case VK_BROWSER_BACK:
	case VK_BROWSER_FORWARD:
	case VK_BROWSER_REFRESH:
	case VK_BROWSER_STOP:
	case VK_BROWSER_SEARCH:
	case VK_BROWSER_FAVORITES:
	case VK_BROWSER_HOME:
	case VK_VOLUME_MUTE:
	case VK_VOLUME_DOWN:
	case VK_VOLUME_UP:
	case VK_MEDIA_NEXT_TRACK:
	case VK_MEDIA_PREV_TRACK:
	case VK_MEDIA_STOP:
	case VK_MEDIA_PLAY_PAUSE:
	case VK_LAUNCH_MAIL:
	case VK_LAUNCH_MEDIA_SELECT:
	case VK_LAUNCH_APP1:
	case VK_LAUNCH_APP2:
	case VK_RETURN:
	case VK_INSERT:
	case VK_DELETE:
	case VK_PRIOR:
	case VK_NEXT:
	case VK_HOME:
	case VK_END:
	case VK_UP:
	case VK_DOWN:
	case VK_LEFT:
	case VK_RIGHT:
	case VK_SNAPSHOT:
		return KEYEVENTF_EXTENDEDKEY;
	}
	return 0;
}

/*
 * get_scan_code - SCANコード取得
 */
static BYTE get_scan_code(BYTE vk)
{
	BYTE sc = 0;

	switch (vk) {
	case VK_LSHIFT:   sc = SC_LSHIFT; break;
	case VK_RSHIFT:   sc = SC_RSHIFT; break;
	case VK_LCONTROL: sc = SC_LCONTROL; break;
	case VK_RCONTROL: sc = SC_RCONTROL; break;
	case VK_LMENU:    sc = SC_LALT; break;
	case VK_RMENU:    sc = SC_RALT; break;
	case VK_LWIN:     sc = SC_LWIN; break;
	case VK_RWIN:     sc = SC_RWIN; break;
	case VK_PAUSE:    sc = SC_PAUSE; break;
	case VK_NUMPAD0:  sc = SC_NUMPAD0; break;
	case VK_NUMPAD1:  sc = SC_NUMPAD1; break;
	case VK_NUMPAD2:  sc = SC_NUMPAD2; break;
	case VK_NUMPAD3:  sc = SC_NUMPAD3; break;
	case VK_NUMPAD4:  sc = SC_NUMPAD4; break;
	case VK_NUMPAD5:  sc = SC_NUMPAD5; break;
	case VK_NUMPAD6:  sc = SC_NUMPAD6; break;
	case VK_NUMPAD7:  sc = SC_NUMPAD7; break;
	case VK_NUMPAD8:  sc = SC_NUMPAD8; break;
	case VK_NUMPAD9:  sc = SC_NUMPAD9; break;
	case VK_DECIMAL:  sc = SC_NUMPADDOT; break;
	case VK_NUMLOCK:  sc = SC_NUMLOCK; break;
	case VK_DIVIDE:   sc = SC_NUMPADDIV; break;
	case VK_MULTIPLY: sc = SC_NUMPADMULT; break;
	case VK_SUBTRACT: sc = SC_NUMPADSUB; break;
	case VK_ADD:      sc = SC_NUMPADADD; break;
	case VK_SNAPSHOT: sc = SC_PRINTSCREEN; break;
	}
	if (sc != 0) {
		return sc;
	}
	return (BYTE)MapVirtualKey(vk, 0);
}

/*
 * sendkey_send - キーを送信
 */
static void sendkey_send(const int wait, const UINT modifiers, const UINT virtkey)
{
	// wait
	Sleep(wait);

	// 組み合わせキー押下
	if (modifiers & MOD_ALT) {
		keybd_event(VK_MENU, get_scan_code(VK_MENU), get_extended_key(VK_MENU), 0);
	}
	if (modifiers & MOD_CONTROL) {
		keybd_event(VK_CONTROL, get_scan_code(VK_CONTROL), get_extended_key(VK_CONTROL), 0);
	}
	if (modifiers & MOD_SHIFT) {
		keybd_event(VK_SHIFT, get_scan_code(VK_SHIFT), get_extended_key(VK_SHIFT), 0);
	}
	if (modifiers & MOD_WIN) {
		keybd_event(VK_LWIN, get_scan_code(VK_LWIN), get_extended_key(VK_LWIN), 0);
	}
	// キー送信
	keybd_event((BYTE)virtkey, get_scan_code(virtkey), get_extended_key(virtkey), 0);
	keybd_event((BYTE)virtkey, get_scan_code(virtkey), get_extended_key(virtkey) | KEYEVENTF_KEYUP, 0);
	// 組み合わせキー解除
	if (modifiers & MOD_ALT) {
		keybd_event(VK_MENU, get_scan_code(VK_MENU), get_extended_key(VK_MENU) | KEYEVENTF_KEYUP, 0);
	}
	if (modifiers & MOD_CONTROL) {
		keybd_event(VK_CONTROL, get_scan_code(VK_CONTROL), get_extended_key(VK_CONTROL) | KEYEVENTF_KEYUP, 0);
	}
	if (modifiers & MOD_SHIFT) {
		keybd_event(VK_SHIFT, get_scan_code(VK_SHIFT), get_extended_key(VK_SHIFT) | KEYEVENTF_KEYUP, 0);
	}
	if (modifiers & MOD_WIN) {
		keybd_event(VK_LWIN, get_scan_code(VK_LWIN), get_extended_key(VK_LWIN) | KEYEVENTF_KEYUP, 0);
	}
}

/*
 * sendkey_copy - コピーキーを送信
 */
BOOL sendkey_copy(const HWND hWnd)
{
	int i;
	BOOL send_flag = FALSE;

	i = sendkey_window_to_index(hWnd, -1);
	while (i != -1) {
		if ((option.sendkey_info + i)->copy_modifiers != 0 ||
			(option.sendkey_info + i)->copy_virtkey != 0) {
			// コピー
			sendkey_send((option.sendkey_info + i)->copy_wait,
				(option.sendkey_info + i)->copy_modifiers,
				(option.sendkey_info + i)->copy_virtkey);
			send_flag = TRUE;
		}
		i = sendkey_window_to_index(hWnd, i);
	}
	if (send_flag == FALSE) {
		// デフォルトのコピー
		sendkey_send(option.def_copy_wait,
			option.def_copy_modifiers, option.def_copy_virtkey);
	}
	return TRUE;
}

/*
 * sendkey_paste - 貼り付けキーを送信
 */
BOOL sendkey_paste(const HWND hWnd)
{
	int i;
	BOOL send_flag = FALSE;

	i = sendkey_window_to_index(hWnd, -1);
	while (i != -1) {
		if ((option.sendkey_info + i)->paste_modifiers != 0 ||
			(option.sendkey_info + i)->paste_virtkey != 0) {
			// 貼り付け
			sendkey_send((option.sendkey_info + i)->paste_wait,
				(option.sendkey_info + i)->paste_modifiers,
				(option.sendkey_info + i)->paste_virtkey);
			send_flag = TRUE;
		}
		i = sendkey_window_to_index(hWnd, i);
	}
	if (send_flag == FALSE) {
		// デフォルトの貼り付け
		sendkey_send(option.def_paste_wait,
			option.def_paste_modifiers, option.def_paste_virtkey);
	}
	return TRUE;
}
/* End of source */
