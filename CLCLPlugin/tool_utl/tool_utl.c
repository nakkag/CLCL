/*
 * tool_utl
 *
 * tool_utl.c
 *
 * Copyright (C) 1996-2003 by Nakashima Tomoaki. All rights reserved.
 *		http://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE
#include <wchar.h>
#include <shlobj.h>
#include <Shlwapi.h>

#include "..\CLCLPlugin.h"
#include "resource.h"

/* Define */
#define	INI_FILE_NAME				TEXT("tool_utl.ini")

/* Global Variables */
HINSTANCE hInst;
TCHAR ini_path[MAX_PATH];

static TCHAR wave_file[BUF_SIZE];

TCHAR multi_save_format[BUF_SIZE];
TCHAR multi_save_file_name[BUF_SIZE];

/* Local Function Prototypes */
static BOOL dll_initialize(void);
static BOOL dll_uninitialize(void);

/*
 * DllMain - メイン
 */
int WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved)
{
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		hInst = hInstance;
		dll_initialize();
		break;

	case DLL_PROCESS_DETACH:
		dll_uninitialize();
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}

/*
 * dll_initialize - 初期化
 */
static BOOL dll_initialize(void)
{
	TCHAR dll_path[MAX_PATH];
	TCHAR exe_path[MAX_PATH];
	TCHAR *p, *r;

	// First look if it is portable installation.
	// Handle NULL instead of hInst indicates to look 
	// for the path of the exe instead of the current dll.
	GetModuleFileName(NULL, exe_path, MAX_PATH - 1);
	for (p = r = exe_path; *p != TEXT('\0'); p++) {
#ifndef UNICODE
		if (IsDBCSLeadByte((BYTE)*p) == TRUE) {
			p++;
			continue;
		}
#endif	// UNICODE
		if (*p == TEXT('\\') || *p == TEXT('/')) {
			r = p;
		}
	}
	*r = TEXT('\0');

	UINT portable = 0;
	TCHAR clcl_ini_path[MAX_PATH];
	swprintf_s(clcl_ini_path, MAX_PATH, TEXT("%s\\%s"), exe_path, TEXT("clcl.ini"));
	if (PathFileExists(clcl_ini_path)) {
		portable = GetPrivateProfileInt(TEXT("GENERAL"), TEXT("GENERAL"), 0, clcl_ini_path);
	}

	// get the dll folder
	GetModuleFileName(hInst, dll_path, MAX_PATH - 1);
	for (p = r = dll_path; *p != TEXT('\0'); p++) {
#ifndef UNICODE
		if (IsDBCSLeadByte((BYTE)*p) == TRUE) {
			p++;
			continue;
		}
#endif	// UNICODE
		if (*p == TEXT('\\') || *p == TEXT('/')) {
			r = p;
		}
	}
	*r = TEXT('\0');

	if (portable == 1) { // If portable installation
		swprintf_s(ini_path, MAX_PATH, TEXT("%s\\%s"), dll_path, INI_FILE_NAME); // locate ini in dll folder
		// If ini file does not yet exist in dll folder we locate it 
		// in the exe folder where clcl.ini also resides.
		if (PathFileExists(ini_path) == FALSE)
			swprintf_s(ini_path, MAX_PATH, TEXT("%s\\%s"), exe_path, INI_FILE_NAME); // same folder as clcl.exe and clcl.ini
	}
	else {
		// For normal installation (not portable app) set ini_path to the same folder as %LOCALAPPDATA%\CLCL\clcl.ini
		// so that we have write access.
		// See Clcl\main.c line 2234 ff (get_work_path())
		TCHAR local_app_data[MAX_PATH];
		if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, local_app_data))) {
			swprintf_s(ini_path, MAX_PATH, TEXT("%s\\clcl\\%s"), local_app_data, INI_FILE_NAME);
			// If ini file not yet exists here, we may read the settings form old location in dll folder
			// but I don't consider it necessary.
			//if (PathFileExists(ini_path) == FALSE) {
			//	TCHAR old_ini[MAX_PATH];
			//	wsprintf(old_ini, TEXT("%s\\%s"), dll_path, INI_FILE_NAME);
			//	if (PathFileExists(old_ini)) {
			//		// TODO: read values from old_ini
			//		// and return
			//	}
			//}
		}
	}

	GetPrivateProfileString(TEXT("play_sound"), TEXT("wave_file"), TEXT(""), wave_file, BUF_SIZE - 1, ini_path);

	GetPrivateProfileString(TEXT("multi_save"), TEXT("format"), TEXT("TEXT"), multi_save_format, BUF_SIZE - 1, ini_path);
	GetPrivateProfileString(TEXT("multi_save"), TEXT("file_name"), TEXT("%04d.txt"), multi_save_file_name, BUF_SIZE - 1, ini_path);
	return TRUE;
}

/*
 * dll_uninitialize - 終了処理
 */
static BOOL dll_uninitialize(void)
{
	return TRUE;
}

/*
 * get_tool_info_w - ツール情報取得
 */
__declspec(dllexport) BOOL CALLBACK get_tool_info_w(const HWND hWnd, const int index, TOOL_GET_INFO *tgi)
{
	switch (index) {
	case 0:
		LoadString(hInst, IDS_CLEAR_HISTORY, tgi->title, BUF_SIZE - 1);
		lstrcpy(tgi->func_name, TEXT("clear_history"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_MENU | CALLTYPE_VIEWER;
		return TRUE;

	case 1:
		LoadString(hInst, IDS_CLEAR_CLIPBOARD, tgi->title, BUF_SIZE - 1);
		lstrcpy(tgi->func_name, TEXT("clear_clipboard"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_MENU | CALLTYPE_VIEWER;
		return TRUE;

	case 2:
		LoadString(hInst, IDS_PLAY_SOUND, tgi->title, BUF_SIZE - 1);
		lstrcpy(tgi->func_name, TEXT("play_sound"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_ADD_HISTORY;
		return TRUE;

	case 3:
		LoadString(hInst, IDS_ALWAYS_ON_TOP, tgi->title, BUF_SIZE - 1);
		lstrcpy(tgi->func_name, TEXT("ztop"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_VIEWER;
		return TRUE;

	case 4:
		LoadString(hInst, IDS_UN_TOP, tgi->title, BUF_SIZE - 1);
		lstrcpy(tgi->func_name, TEXT("unztop"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_VIEWER;
		return TRUE;

	case 5:
		LoadString(hInst, IDS_SAVE_SEVERAL, tgi->title, BUF_SIZE - 1);
		lstrcpy(tgi->func_name, TEXT("multi_save"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_VIEWER;
		return TRUE;
	}
	return FALSE;
}

/*
 * clear_history - 履歴のクリア
 */
__declspec(dllexport) int CALLBACK clear_history(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	DATA_INFO *di;

	TCHAR clr_hist_question[BUF_SIZE];
	LoadString(hInst, IDS_CLR_HIST_QUESTION, clr_hist_question, BUF_SIZE - 1);
	TCHAR clr_hist_caption[BUF_SIZE];
	LoadString(hInst, IDS_CLR_HIST_CAPTION, clr_hist_caption, BUF_SIZE - 1);

	if (MessageBox(hWnd, clr_hist_question, clr_hist_caption,
		MB_ICONQUESTION | MB_YESNO | MB_TOPMOST) == IDNO) {
		return TOOL_SUCCEED;
	}
	// 履歴の取得
	di = (DATA_INFO *)SendMessage(hWnd, WM_HISTORY_GET_ROOT, 0, 0);
	if (di != NULL) {
		// 履歴の解放
		SendMessage(hWnd, WM_ITEM_FREE, 0, (LPARAM)di->child);
		di->child = NULL;
		// 履歴の変化を通知
		SendMessage(hWnd, WM_HISTORY_CHANGED, 0, 0);
	}
	return TOOL_SUCCEED;
}

/*
 * clear_history_property - プロパティ表示
 */
__declspec(dllexport) BOOL CALLBACK clear_history_property(const HWND hWnd, TOOL_EXEC_INFO *tei)
{
	return FALSE;
}

/*
 * clear_clipboard - クリップボードのクリア
 */
__declspec(dllexport) int CALLBACK clear_clipboard(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	if (OpenClipboard(hWnd) == TRUE) {
		EmptyClipboard();
		CloseClipboard();
	}
	return TOOL_SUCCEED;
}

/*
 * clear_clipboard_property - プロパティ表示
 */
__declspec(dllexport) BOOL CALLBACK clear_clipboard_property(const HWND hWnd, TOOL_EXEC_INFO *tei)
{
	return FALSE;
}

/*
 * play_sound - WAVEファイルを鳴らす
 */
__declspec(dllexport) int CALLBACK play_sound(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	if (*wave_file == TEXT('\0') || sndPlaySound(wave_file, SND_ASYNC | SND_NODEFAULT) == FALSE) {
		MessageBeep(MB_ICONASTERISK);
	}
	return TOOL_SUCCEED;
}

/*
 * play_sound_property - プロパティ表示
 */
__declspec(dllexport) BOOL CALLBACK play_sound_property(const HWND hWnd, TOOL_EXEC_INFO *tei)
{
	OPENFILENAME of;
	TCHAR buf[MAX_PATH];
	TCHAR title[BUF_SIZE];
	LoadString(hInst, IDS_WAVE_FILE, title, BUF_SIZE - 1);

	// ファイルの選択
	ZeroMemory(&of, sizeof(OPENFILENAME));
	of.lStructSize = sizeof(OPENFILENAME);
	of.hInstance = hInst;
	of.hwndOwner = hWnd;
	of.lpstrTitle = title;
	of.lpstrFilter = TEXT("*.wav\0*.wav\0*.*\0*.*\0\0");
	of.nFilterIndex = 1;
	lstrcpy(buf, wave_file);
	of.lpstrFile = buf;
	of.nMaxFile = MAX_PATH - 1;
	of.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	if (GetOpenFileName(&of) == TRUE) {
		lstrcpy(wave_file, buf);
		WritePrivateProfileString(TEXT("play_sound"), TEXT("wave_file"), wave_file, ini_path);
	}
	return TRUE;
}

/*
 * ztop - 最前面に表示
 */
__declspec(dllexport) int CALLBACK ztop(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	HWND hViewerWnd;

	hViewerWnd = (HWND)SendMessage(hWnd, WM_VIEWER_GET_HWND, 0, 0);
	if (hViewerWnd != NULL) {
		// 最前面に表示
		SetWindowPos(hViewerWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	}
	return TOOL_SUCCEED;
}

/*
 * ztop_property - プロパティ表示
 */
__declspec(dllexport) BOOL CALLBACK ztop_property(const HWND hWnd, TOOL_EXEC_INFO *tei)
{
	return FALSE;
}

/*
 * unztop - 最前面に表示の解除
 */
__declspec(dllexport) int CALLBACK unztop(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	HWND hViewerWnd;

	hViewerWnd = (HWND)SendMessage(hWnd, WM_VIEWER_GET_HWND, 0, 0);
	if (hViewerWnd != NULL) {
		// 最前面に表示
		SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	}
	return TOOL_SUCCEED;
}

/*
 * unztop_property - プロパティ表示
 */
__declspec(dllexport) BOOL CALLBACK unztop_property(const HWND hWnd, TOOL_EXEC_INFO *tei)
{
	return FALSE;
}
/* End of source */
