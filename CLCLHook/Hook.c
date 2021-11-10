/*
 * CLCLHook
 *
 * Hook.c
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE

/* Define */

/* Global Variables */
#pragma data_seg("my_shared_section")

static HHOOK next_hook = NULL;
static HWND call_wnd = NULL;
static int msg_id = 0;

#pragma data_seg()

HINSTANCE hInstDLL;

/* Local Function Prototypes **/

/*
 * DllMain - メイン
 */
int WINAPI DllMain(HINSTANCE hInstance, DWORD dwNotification, LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(lpReserved);

	hInstDLL = hInstance;
	return TRUE;
}

/*
 * key_hook_proc - フックプロシージャ
 */
LRESULT CALLBACK key_hook_proc(INT nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0) {
		SendMessage(call_wnd, msg_id, wParam, lParam);
	}
	return CallNextHookEx(next_hook, nCode, wParam, lParam);
}

/*
 * set_hook - フックの開始
 */
__declspec(dllexport) BOOL CALLBACK SetHook(const HWND hWnd, const int msg)
{
	call_wnd = hWnd;
	msg_id = msg;

	//フックを開始する
	next_hook = SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)key_hook_proc, hInstDLL, 0);
	if (next_hook == NULL) {
		return FALSE;
	}
	return TRUE;
}

/*
 * UnHook - フックの解除
 */
__declspec(dllexport) void CALLBACK UnHook(void)
{
	if (next_hook != NULL) {
		//フックを解除する
		UnhookWindowsHookEx(next_hook);
	}
}
/* End of source */
