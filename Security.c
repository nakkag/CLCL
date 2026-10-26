/*
 * CLCL
 *
 * Security.c
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define COBJMACROS
#include <windows.h>
#include <tchar.h>
#include <shlobj.h>
#include <exdisp.h>
#include <shldisp.h>
#include <shlwapi.h>
#include <aclapi.h>

#include "General.h"
#include "Message.h"
#include "Security.h"

#ifndef OPTION_SET
#include "Viewer.h"
#include "DarkMode.h"
#endif	// OPTION_SET

// CLCL と CLCLSet で同じ ID を使用する
#include "resource.h"

#pragma comment(lib, "shlwapi.lib")

/* Define */
#ifndef LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR
#define LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR	0x00000100
#endif
#ifndef LOAD_LIBRARY_SEARCH_DEFAULT_DIRS
#define LOAD_LIBRARY_SEARCH_DEFAULT_DIRS	0x00001000
#endif

#define SEC_PATH_SIZE					(MAX_PATH * 2)

// 一般ユーザーに許されていると差し替えが可能になる権限
#define RIGHTS_FILE						(FILE_WRITE_DATA | FILE_APPEND_DATA | DELETE | WRITE_DAC | WRITE_OWNER)
#define RIGHTS_DLL_DIR					(FILE_ADD_FILE | FILE_DELETE_CHILD | DELETE | WRITE_DAC | WRITE_OWNER)
#define RIGHTS_PARENT_DIR				(FILE_DELETE_CHILD | DELETE | WRITE_DAC | WRITE_OWNER)

typedef BOOL (WINAPI *SET_DEFAULT_DLL_DIRECTORIES)(DWORD);

/* Global Variables */
extern HINSTANCE hInst;

static BOOL search_flags_supported = FALSE;
static int elevated = -1;
// 信頼する外側のメッセージの処理中の深さ (メインスレッドのみで使用)
static int trusted_depth = 0;

/* Local Function Prototypes */
static BOOL get_module_dir(TCHAR *buf, const int size);
static BOOL get_final_path(const HANDLE hFile, TCHAR *buf, const int size);
static BOOL path_is_under(const TCHAR *dir, const TCHAR *path);
static HANDLE get_limited_token(void);
static BOOL limited_user_has_rights(const HANDLE hToken, const HANDLE hObject, const DWORD rights);
static BOOL path_is_protected(const HANDLE hToken, const TCHAR *final_path);
static BOOL plugin_file_is_safe(const HANDLE hToken, const HANDLE hFile, const TCHAR *final_path, const TCHAR *app_dir);
static HMODULE load_library(const TCHAR *path);

/*
 * security_initialize - DLL の検索先をシステムフォルダとアプリケーションのフォルダに限定する
 */
void security_initialize(void)
{
	HMODULE kernel32;
	SET_DEFAULT_DLL_DIRECTORIES func;

	// カレントディレクトリを DLL の検索先から外す
	SetDllDirectory(TEXT(""));

	kernel32 = GetModuleHandle(TEXT("kernel32.dll"));
	if (kernel32 == NULL) {
		return;
	}
	func = (SET_DEFAULT_DLL_DIRECTORIES)GetProcAddress(kernel32, "SetDefaultDllDirectories");
	if (func != NULL && func(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS) != FALSE) {
		search_flags_supported = TRUE;
	}
}

/*
 * security_is_elevated - UAC で昇格して実行しているか
 *
 *	昇格前のトークンを持つ (同じユーザーの昇格していないプロセスが存在しうる) 場合のみ TRUE
 */
BOOL security_is_elevated(void)
{
	HANDLE hToken;
	TOKEN_ELEVATION_TYPE type;
	DWORD len;

	if (elevated != -1) {
		return (elevated == 1) ? TRUE : FALSE;
	}
	elevated = 0;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken) == FALSE) {
		return FALSE;
	}
	if (GetTokenInformation(hToken, TokenElevationType, &type, sizeof(type), &len) != FALSE &&
		type == TokenElevationTypeFull) {
		elevated = 1;
	}
	CloseHandle(hToken);
	return (elevated == 1) ? TRUE : FALSE;
}

/*
 * path_join - フォルダとファイル名を連結 (バッファに収まらない場合は FALSE)
 */
BOOL path_join(TCHAR *buf, const int size, const TCHAR *dir, const TCHAR *name)
{
	int dir_len, name_len;
	BOOL sep;

	if (buf == NULL || size <= 0 || dir == NULL || name == NULL) {
		return FALSE;
	}
	dir_len = lstrlen(dir);
	name_len = lstrlen(name);
	sep = (dir_len > 0 && *(dir + dir_len - 1) != TEXT('\\')) ? TRUE : FALSE;
	if (dir_len + ((sep == TRUE) ? 1 : 0) + name_len + 1 > size) {
		*buf = TEXT('\0');
		return FALSE;
	}
	lstrcpy(buf, dir);
	if (sep == TRUE) {
		lstrcat(buf, TEXT("\\"));
	}
	lstrcat(buf, name);
	return TRUE;
}

/*
 * security_add_blocked - 読み込まなかった DLL を一覧に追加 (収まらない分は切り捨てる)
 */
void security_add_blocked(TCHAR *list, const int size, const TCHAR *lib_path)
{
	int len = lstrlen(list);

	if (lib_path == NULL || len + 2 >= size) {
		return;
	}
	lstrcpyn(list + len, TEXT("\r\n"), size - len);
	len += 2;
	lstrcpyn(list + len, lib_path, size - len);
	// 切り詰めでサロゲートペアを分断しない
	len = lstrlen(list);
	if (len > 0 && IS_HIGH_SURROGATE(*(list + len - 1))) {
		*(list + len - 1) = TEXT('\0');
	}
}

/*
 * security_get_blocked_message - 読み込まなかった DLL の一覧からエラーメッセージを作成
 */
void security_get_blocked_message(TCHAR *err_str, const int size, const TCHAR *list)
{
	TCHAR msg[BUF_SIZE * 2];

	*msg = TEXT('\0');
	LoadString(hInst, IDS_ERROR_PLUGIN_BLOCKED, msg, BUF_SIZE * 2 - 1);
	wnsprintf(err_str, size, TEXT("%s%s"), msg, list);
}

/*
 * get_module_dir - 実行ファイルのフォルダを取得
 */
static BOOL get_module_dir(TCHAR *buf, const int size)
{
	DWORD len;
	TCHAR *p;

	len = GetModuleFileName(NULL, buf, size);
	if (len == 0 || len >= (DWORD)size) {
		return FALSE;
	}
	if ((p = StrRChr(buf, NULL, TEXT('\\'))) == NULL) {
		return FALSE;
	}
	*p = TEXT('\0');
	return TRUE;
}

/*
 * get_final_path - ハンドルからリンクなどを解決した実際のパスを取得 (\\?\ を除いた形式)
 */
static BOOL get_final_path(const HANDLE hFile, TCHAR *buf, const int size)
{
	TCHAR tmp[SEC_PATH_SIZE];
	DWORD len;

	len = GetFinalPathNameByHandle(hFile, tmp, SEC_PATH_SIZE, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
	if (len == 0 || len >= SEC_PATH_SIZE) {
		return FALSE;
	}
	if (StrCmpNI(tmp, TEXT("\\\\?\\UNC\\"), 8) == 0) {
		if (lstrlen(tmp) - 6 >= size) {
			return FALSE;
		}
		lstrcpy(buf, TEXT("\\\\"));
		lstrcat(buf, tmp + 8);
	} else if (StrCmpNI(tmp, TEXT("\\\\?\\"), 4) == 0) {
		if (lstrlen(tmp) - 4 >= size) {
			return FALSE;
		}
		lstrcpy(buf, tmp + 4);
	} else {
		if (lstrlen(tmp) >= size) {
			return FALSE;
		}
		lstrcpy(buf, tmp);
	}
	return TRUE;
}

/*
 * path_is_under - path が dir の配下にあるか
 */
static BOOL path_is_under(const TCHAR *dir, const TCHAR *path)
{
	int len = lstrlen(dir);

	if (len == 0 || lstrlen(path) <= len) {
		return FALSE;
	}
	if (CompareStringOrdinal(dir, len, path, len, TRUE) != CSTR_EQUAL) {
		return FALSE;
	}
	return (*(dir + len - 1) == TEXT('\\') || *(path + len) == TEXT('\\')) ? TRUE : FALSE;
}

/*
 * get_limited_token - 昇格前のトークン (同じユーザーの通常権限) を取得
 */
static HANDLE get_limited_token(void)
{
	HANDLE hToken;
	HANDLE hImpToken = NULL;
	TOKEN_LINKED_TOKEN tlt;
	DWORD len;

	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken) == FALSE) {
		return NULL;
	}
	ZeroMemory(&tlt, sizeof(tlt));
	if (GetTokenInformation(hToken, TokenLinkedToken, &tlt, sizeof(tlt), &len) != FALSE &&
		tlt.LinkedToken != NULL) {
		if (DuplicateToken(tlt.LinkedToken, SecurityIdentification, &hImpToken) == FALSE) {
			hImpToken = NULL;
		}
		CloseHandle(tlt.LinkedToken);
	}
	CloseHandle(hToken);
	return hImpToken;
}

/*
 * limited_user_has_rights - 通常権限のトークンに rights のいずれかが許可されているか
 *
 *	判定できない場合は許可されているものとして扱う
 */
static BOOL limited_user_has_rights(const HANDLE hToken, const HANDLE hObject, const DWORD rights)
{
	PSECURITY_DESCRIPTOR psd = NULL;
	GENERIC_MAPPING gm = { FILE_GENERIC_READ, FILE_GENERIC_WRITE, FILE_GENERIC_EXECUTE, FILE_ALL_ACCESS };
	BYTE ps_buf[sizeof(PRIVILEGE_SET) + sizeof(LUID_AND_ATTRIBUTES) * 8];
	DWORD ps_len = sizeof(ps_buf);
	DWORD granted = 0;
	BOOL status = FALSE;
	BOOL ret = TRUE;

	if (GetSecurityInfo(hObject, SE_FILE_OBJECT,
		OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION,
		NULL, NULL, NULL, NULL, &psd) != ERROR_SUCCESS) {
		return TRUE;
	}
	if (AccessCheck(psd, hToken, MAXIMUM_ALLOWED, &gm, (PPRIVILEGE_SET)ps_buf, &ps_len, &granted, &status) != FALSE) {
		ret = (status != FALSE && (granted & rights) != 0) ? TRUE : FALSE;
	}
	LocalFree(psd);
	return ret;
}

/*
 * path_is_protected - ファイルとその上位のフォルダを一般ユーザーが変更できないか
 */
static BOOL path_is_protected(const HANDLE hToken, const TCHAR *final_path)
{
	TCHAR dir[SEC_PATH_SIZE];
	HANDLE hDir;
	TCHAR *p;
	BOOL first = TRUE;
	BOOL ret = TRUE;

	// ネットワーク上のファイルはアクセス権を正しく判定できない
	if (*final_path == TEXT('\\') || lstrlen(final_path) >= SEC_PATH_SIZE) {
		return FALSE;
	}
	lstrcpy(dir, final_path);
	while ((p = StrRChr(dir, NULL, TEXT('\\'))) != NULL) {
		if (p == dir + 2 && *(dir + 1) == TEXT(':')) {
			// ドライブ直下
			*(p + 1) = TEXT('\0');
		} else {
			*p = TEXT('\0');
		}
		hDir = CreateFile(dir, READ_CONTROL, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (hDir == INVALID_HANDLE_VALUE) {
			return FALSE;
		}
		if (limited_user_has_rights(hToken, hDir, (first == TRUE) ? RIGHTS_DLL_DIR : RIGHTS_PARENT_DIR) == TRUE) {
			ret = FALSE;
		}
		CloseHandle(hDir);
		if (ret == FALSE || *(dir + lstrlen(dir) - 1) == TEXT('\\')) {
			break;
		}
		first = FALSE;
	}
	return ret;
}

/*
 * plugin_file_is_safe - 開いている DLL を一般ユーザー (hToken) が差し替えられないか
 *
 *	アプリケーションのフォルダ配下は許可する
 */
static BOOL plugin_file_is_safe(const HANDLE hToken, const HANDLE hFile, const TCHAR *final_path, const TCHAR *app_dir)
{
	TCHAR app_final[SEC_PATH_SIZE];
	HANDLE hDir;
	BOOL safe = FALSE;

	hDir = CreateFile(app_dir, READ_CONTROL, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (hDir != INVALID_HANDLE_VALUE) {
		if (get_final_path(hDir, app_final, SEC_PATH_SIZE) == TRUE && path_is_under(app_final, final_path) == TRUE) {
			safe = TRUE;
		}
		CloseHandle(hDir);
	}
	if (safe == TRUE) {
		return TRUE;
	}
	return (limited_user_has_rights(hToken, hFile, RIGHTS_FILE) == FALSE &&
		path_is_protected(hToken, final_path) == TRUE) ? TRUE : FALSE;
}

/*
 * load_library - DLL のフォルダとシステムフォルダから依存 DLL を探して読み込む
 */
static HMODULE load_library(const TCHAR *path)
{
	HMODULE lib;

	if (search_flags_supported == TRUE) {
		lib = LoadLibraryEx(path, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
		if (lib != NULL || GetLastError() != ERROR_INVALID_PARAMETER) {
			return lib;
		}
	}
	return LoadLibraryEx(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
}

/*
 * security_load_plugin - プラグインの DLL を読み込む
 *
 *	相対パスはアプリケーションのフォルダを基準にする。
 *	昇格して実行している場合は、アプリケーションのフォルダ配下か一般ユーザーが変更できない場所の
 *	DLL のみ読み込み、それ以外は blocked に TRUE を設定して NULL を返す。
 */
HMODULE security_load_plugin(const TCHAR *lib_path, BOOL *blocked)
{
	TCHAR app_dir[SEC_PATH_SIZE];
	TCHAR tmp[SEC_PATH_SIZE];
	TCHAR full_path[SEC_PATH_SIZE];
	TCHAR final_path[SEC_PATH_SIZE];
	HANDLE hFile, hToken;
	HMODULE lib = NULL;
	DWORD len;
	BOOL safe = FALSE;
	DWORD err;

	if (blocked != NULL) {
		*blocked = FALSE;
	}
	if (lib_path == NULL || *lib_path == TEXT('\0') || get_module_dir(app_dir, SEC_PATH_SIZE) == FALSE) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return NULL;
	}
	// 絶対パスにする
	if (PathIsRelative(lib_path) == TRUE) {
		if (path_join(tmp, SEC_PATH_SIZE, app_dir, lib_path) == FALSE) {
			SetLastError(ERROR_FILENAME_EXCED_RANGE);
			return NULL;
		}
	} else {
		lstrcpyn(tmp, lib_path, SEC_PATH_SIZE);
	}
	len = GetFullPathName(tmp, SEC_PATH_SIZE, full_path, NULL);
	if (len == 0 || len >= SEC_PATH_SIZE) {
		SetLastError(ERROR_FILENAME_EXCED_RANGE);
		return NULL;
	}

	if (security_is_elevated() == FALSE) {
		return load_library(full_path);
	}
	if (search_flags_supported == FALSE) {
		// 検索先を限定できない環境では PATH 上の DLL が読まれるため、昇格中は読み込まない
		if (blocked != NULL) {
			*blocked = TRUE;
		}
		SetLastError(ERROR_ACCESS_DENIED);
		return NULL;
	}

	// 判定から読み込みまでの間に書き換えられないように書き込みと削除を禁止して開いておく
	hFile = CreateFile(full_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		return NULL;
	}
	if (get_final_path(hFile, final_path, SEC_PATH_SIZE) == FALSE) {
		// 実際の場所を確認できない (ドライブ文字のないボリュームなど) ため読み込まない
		CloseHandle(hFile);
		if (blocked != NULL) {
			*blocked = TRUE;
		}
		SetLastError(ERROR_ACCESS_DENIED);
		return NULL;
	}
	if ((hToken = get_limited_token()) != NULL) {
		safe = plugin_file_is_safe(hToken, hFile, final_path, app_dir);
		CloseHandle(hToken);
	}
	if (safe == FALSE) {
		CloseHandle(hFile);
		if (blocked != NULL) {
			*blocked = TRUE;
		}
		SetLastError(ERROR_ACCESS_DENIED);
		return NULL;
	}
	// 判定したパスそのものを読み込む
	lib = load_library(final_path);
	err = GetLastError();
	CloseHandle(hFile);
	SetLastError(err);
	return lib;
}

/*
 * security_load_plugin_ui - プラグインの DLL を読み込み、失敗した場合はメッセージを表示
 */
HMODULE security_load_plugin_ui(const HWND hWnd, const TCHAR *lib_path)
{
	TCHAR err_str[SECURITY_ERR_SIZE];
	TCHAR list[SECURITY_ERR_SIZE];
	HMODULE lib;
	BOOL blocked;

	if ((lib = security_load_plugin(lib_path, &blocked)) != NULL) {
		return lib;
	}
	if (blocked == TRUE) {
		*list = TEXT('\0');
		security_add_blocked(list, SECURITY_ERR_SIZE, lib_path);
		security_get_blocked_message(err_str, SECURITY_ERR_SIZE, list);
	} else {
		message_get_error(GetLastError(), err_str);
	}
	if (*err_str != TEXT('\0')) {
		MessageBox(hWnd, err_str, (lib_path != NULL) ? lib_path : TEXT(""), MB_ICONERROR);
	}
	return NULL;
}

/*
 * security_is_internal_message - 同じスレッドからの SendMessage でのみ受け付けるメッセージか
 *
 *	ポインタやハンドルを受け渡すプラグイン用・内部用のメッセージと、システムが同じスレッドへ送る
 *	メニューやコントロールのメッセージが対象。別のスレッドやプロセスから送られたり、キューに
 *	投稿されたりした場合は処理しない。
 */
BOOL security_is_internal_message(const UINT msg)
{
	switch (msg) {
	case WM_GET_WORKPATH:
#ifndef OPTION_SET
	case WM_GET_FORMAT_ICON:
	case WM_OPTION_SHOW:
	case WM_OPTION_GET:
	case WM_HISTORY_GET_ROOT:
	case WM_REGIST_GET_ROOT:
	case WM_VIEWER_GET_SELECTION:
	case WM_VIEWER_SELECT_ITEM:
	case WM_DRAGDROP:
	case WM_GETDATA:
	case WM_MENUCHAR:
	case WM_UAHDRAWMENU:
	case WM_UAHDRAWMENUITEM:
	case WM_MEASUREITEM:
	case WM_DRAWITEM:
	case WM_NOTIFY:
#endif	// OPTION_SET
		return TRUE;
	}
#ifndef OPTION_SET
	// 各ウィンドウが WM_APP + 1 〜 7 で使う内部メッセージ (データや構造体のポインタを渡す)
	if (msg >= WM_APP + 1 && msg <= WM_APP + 7) {
		return TRUE;
	}
	return (msg >= WM_ITEM_TO_CLIPBOARD && msg <= WM_ITEM_GET_SAVE_INFO) ? TRUE : FALSE;
#else	// OPTION_SET
	return FALSE;
#endif	// OPTION_SET
}

/*
 * security_trust_begin - 本体が信頼する外側のメッセージの処理開始
 *
 *	別のプロセスから送られた設定の再読み込みや終了要求の処理中に、本体がプラグインを呼び出すと、
 *	プラグインが同じスレッドで本体へ送るメッセージも「送信中」と判定されてしまう。
 *	その間は入れ子のメッセージを受け付ける。
 */
void security_trust_begin(void)
{
	trusted_depth++;
}

void security_trust_end(void)
{
	if (trusted_depth > 0) {
		trusted_depth--;
	}
}

/*
 * security_is_foreign_send - 現在のメッセージが別のスレッドやプロセスから送られたものか
 */
BOOL security_is_foreign_send(void)
{
	if (trusted_depth > 0) {
		return FALSE;
	}
	return (InSendMessageEx(NULL) != ISMEX_NOSEND) ? TRUE : FALSE;
}

/*
 * security_is_foreign_message - 処理してはいけない送られ方をしたメッセージか
 */
BOOL security_is_foreign_message(const UINT msg)
{
	return (security_is_internal_message(msg) == TRUE && security_is_foreign_send() == TRUE) ? TRUE : FALSE;
}

#ifndef OPTION_SET
/*
 * security_shell_open_as_user - エクスプローラ経由で起動する (管理者権限を引き継がない)
 */
BOOL security_shell_open_as_user(const TCHAR *file_name, const TCHAR *command_line)
{
	IShellWindows *psw = NULL;
	IDispatch *pdisp = NULL;
	IServiceProvider *psp = NULL;
	IShellBrowser *psb = NULL;
	IShellView *psv = NULL;
	IDispatch *pdisp_view = NULL;
	IShellFolderViewDual *psfvd = NULL;
	IDispatch *pdisp_app = NULL;
	IShellDispatch2 *psd = NULL;
	VARIANT v_empty, v_args, v_dir, v_show;
	BSTR bs_file = NULL, bs_args = NULL, bs_dir = NULL;
	TCHAR cur_dir[MAX_PATH];
	long hwnd = 0;
	BOOL ret = FALSE;

	if (file_name == NULL || *file_name == TEXT('\0')) {
		return FALSE;
	}
	VariantInit(&v_empty);
	VariantInit(&v_args);
	VariantInit(&v_dir);
	VariantInit(&v_show);

	if (FAILED(CoCreateInstance(&CLSID_ShellWindows, NULL, CLSCTX_LOCAL_SERVER,
			&IID_IShellWindows, (void **)&psw)) ||
		FAILED(IShellWindows_FindWindowSW(psw, &v_empty, &v_empty,
			SWC_DESKTOP, &hwnd, SWFO_NEEDDISPATCH, &pdisp)) ||
		pdisp == NULL ||
		FAILED(IDispatch_QueryInterface(pdisp, &IID_IServiceProvider, (void **)&psp)) ||
		FAILED(IServiceProvider_QueryService(psp, &SID_STopLevelBrowser,
			&IID_IShellBrowser, (void **)&psb)) ||
		FAILED(IShellBrowser_QueryActiveShellView(psb, &psv)) ||
		FAILED(IShellView_GetItemObject(psv, SVGIO_BACKGROUND,
			&IID_IDispatch, (void **)&pdisp_view)) ||
		FAILED(IDispatch_QueryInterface(pdisp_view,
			&IID_IShellFolderViewDual, (void **)&psfvd)) ||
		FAILED(IShellFolderViewDual_get_Application(psfvd, &pdisp_app)) ||
		FAILED(IDispatch_QueryInterface(pdisp_app, &IID_IShellDispatch2, (void **)&psd))) {
		goto end;
	}
	if ((bs_file = SysAllocString(file_name)) == NULL) {
		goto end;
	}
	// 作業フォルダは ShellExecuteEx と同じくカレントディレクトリにする
	if (GetCurrentDirectory(MAX_PATH, cur_dir) > 0 && (bs_dir = SysAllocString(cur_dir)) != NULL) {
		v_dir.vt = VT_BSTR;
		v_dir.bstrVal = bs_dir;
	}
	if (command_line != NULL && *command_line != TEXT('\0')) {
		if ((bs_args = SysAllocString(command_line)) == NULL) {
			goto end;
		}
		v_args.vt = VT_BSTR;
		v_args.bstrVal = bs_args;
	}
	v_show.vt = VT_I4;
	v_show.lVal = SW_SHOWNORMAL;
	ret = SUCCEEDED(IShellDispatch2_ShellExecute(psd, bs_file,
		v_args, v_dir, v_empty, v_show)) ? TRUE : FALSE;

end:
	if (bs_file != NULL) {
		SysFreeString(bs_file);
	}
	if (bs_args != NULL) {
		SysFreeString(bs_args);
	}
	if (bs_dir != NULL) {
		SysFreeString(bs_dir);
	}
	if (psd != NULL) {
		IShellDispatch2_Release(psd);
	}
	if (pdisp_app != NULL) {
		IDispatch_Release(pdisp_app);
	}
	if (psfvd != NULL) {
		IShellFolderViewDual_Release(psfvd);
	}
	if (pdisp_view != NULL) {
		IDispatch_Release(pdisp_view);
	}
	if (psv != NULL) {
		IShellView_Release(psv);
	}
	if (psb != NULL) {
		IShellBrowser_Release(psb);
	}
	if (psp != NULL) {
		IServiceProvider_Release(psp);
	}
	if (pdisp != NULL) {
		IDispatch_Release(pdisp);
	}
	if (psw != NULL) {
		IShellWindows_Release(psw);
	}
	return ret;
}
#endif	// OPTION_SET
/* End of source */
