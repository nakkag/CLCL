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
 * DllMain - ���C��
 */
int WINAPI DllMain(HINSTANCE hInstance, DWORD dwNotification, LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(lpReserved);

	hInstDLL = hInstance;
	return TRUE;
}

/*
 * key_hook_proc - low-level keyboard hook procedure
 *
 * WH_KEYBOARD_LL receives:
 *   wParam = WM_KEYDOWN / WM_KEYUP / WM_SYSKEYDOWN / WM_SYSKEYUP
 *   lParam = pointer to KBDLLHOOKSTRUCT
 *
 * We translate to the format the main window expects (same as old WH_KEYBOARD):
 *   msg wParam = virtual key code
 *   msg lParam = bit 31 set on key-up, clear on key-down
 */
LRESULT CALLBACK key_hook_proc(INT nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0) {
		KBDLLHOOKSTRUCT *kb = (KBDLLHOOKSTRUCT *)lParam;
		LPARAM flags = 0;
		if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
			flags = 0x80000000;
		}
		PostMessage(call_wnd, msg_id, (WPARAM)kb->vkCode, flags);
	}
	return CallNextHookEx(next_hook, nCode, wParam, lParam);
}

/*
 * set_hook - �t�b�N�̊J�n
 */
__declspec(dllexport) BOOL CALLBACK SetHook(const HWND hWnd, const int msg)
{
	call_wnd = hWnd;
	msg_id = msg;

	//�t�b�N���J�n����
	// Use WH_KEYBOARD_LL (low-level) so the hook works across all processes
	// regardless of 32/64-bit. No DLL injection required.
	next_hook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)key_hook_proc, hInstDLL, 0);
	if (next_hook == NULL) {
		return FALSE;
	}
	return TRUE;
}

/*
 * UnHook - �t�b�N�̉���
 */
__declspec(dllexport) void CALLBACK UnHook(void)
{
	if (next_hook != NULL) {
		//�t�b�N����������
		UnhookWindowsHookEx(next_hook);
	}
}
/* End of source */
