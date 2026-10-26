/*
 * CLCL Installer
 *
 * Installer.c
 *
 * Copyright (C) 1996-2026 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define COBJMACROS
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <exdisp.h>
#include <shldisp.h>
#include <shlwapi.h>
#include <aclapi.h>
#include <sddl.h>

#include "instinfo.h"
#include "resource.h"
#include "Installer.h"
#include "unzip.h"

#pragma comment(lib, "shlwapi.lib")

/* Define */
#define MSG_SIZE					1024
#define REG_TARGET_MAX				8
#define LOG_BOM						0xfeff
// 廃止したファイル (前回インストールした記録がある場合のみ削除)
#define OBSOLETE_HOOK_LIB			TEXT("CLCLHook.dll")
// Program Files 以外に作成するインストール先のアクセス許可 (一般ユーザーは読み取りと実行のみ)
#define INSTALL_DIR_SDDL			TEXT("D:P(A;OICI;FA;;;SY)(A;OICI;FA;;;BA)(A;OICI;0x1200a9;;;BU)")
// 一般ユーザーに許されているとファイルの差し替えが可能になる権限
#define RIGHTS_INSTALL_DIR			(FILE_ADD_FILE | FILE_ADD_SUBDIRECTORY | FILE_DELETE_CHILD | DELETE | WRITE_DAC | WRITE_OWNER)
#define RIGHTS_PARENT_DIR			(FILE_DELETE_CHILD | DELETE | WRITE_DAC | WRITE_OWNER)

typedef BOOL (WINAPI *SET_DEFAULT_DLL_DIRECTORIES)(DWORD);

/* Global Variables */
static HINSTANCE hInst;
// インストールするファイル
static ZIP_ARCHIVE zip;
static DWORD total_size;
// バージョン文字列
static TCHAR ver_str[BUF_SIZE];
// メッセージボックスのタイトル
static TCHAR title_str[BUF_SIZE];
// インストール先
static TCHAR install_path[MAX_PATH];
// 上書きインストール
static BOOL update_mode;
// アプリの一覧の登録場所
static REG_TARGET reg_target[REG_TARGET_MAX];
static int reg_target_cnt;
// インストールしたファイルの一覧
static STR_BUF log_buf;
// インストール完了
static BOOL install_end;
// ダイアログ用のフォント
static HFONT hDlgFont;
// タイトル用のフォント
static HFONT hTitleFont;

/* Local Function Prototypes */
static TCHAR *str_load(const UINT id, TCHAR *buf, const int size);
static int msg_box(const HWND hWnd, const UINT id, const UINT type, const TCHAR *arg);
static void set_ctrl_text(const HWND hDlg, const int ctrl_id, const UINT str_id);
static BOOL sb_add(STR_BUF *sb, const TCHAR *str);
static void sb_free(STR_BUF *sb);
static void get_module_dir(TCHAR *path);
static BOOL build_path(TCHAR *buf, const int len, const TCHAR *dir, const TCHAR *name);
static BOOL path_has_dotdot(const TCHAR *path);
static BOOL path_normalize(const TCHAR *path, TCHAR *buf);
static BOOL path_is_under(const TCHAR *dir, const TCHAR *path);
static BOOL create_dir(const TCHAR *path, const BOOL log, SECURITY_ATTRIBUTES *sa);
static BOOL install_path_in_program_files(void);
static HANDLE get_user_token(void);
static BOOL user_has_rights(const HANDLE hToken, const TCHAR *path, const DWORD rights);
static BOOL install_path_is_protected(void);
static void remove_obsolete_file(const TCHAR *old_log, const TCHAR *name);
static void delete_dir_tree(const TCHAR *path);
static BOOL move_in_use_file(const TCHAR *path);
static BOOL write_file(const HWND hWnd, const TCHAR *path, const BYTE *buf, const DWORD size);
static void log_add(const TCHAR type, const TCHAR *path);
static BOOL is_running(void);
static BOOL check_running(const HWND hWnd);
static void set_language(void);

/*
 * str_load - 文字列リソースの取得
 */
static TCHAR *str_load(const UINT id, TCHAR *buf, const int size)
{
	*buf = TEXT('\0');
	LoadString(hInst, id, buf, size);
	return buf;
}

/*
 * msg_box - メッセージの表示
 */
static int msg_box(const HWND hWnd, const UINT id, const UINT type, const TCHAR *arg)
{
	TCHAR fmt[MSG_SIZE];
	TCHAR msg[MSG_SIZE + MAX_PATH];

	str_load(id, fmt, MSG_SIZE);
	if (arg == NULL) {
		lstrcpy(msg, fmt);
	} else {
		wsprintf(msg, fmt, arg);
	}
	return MessageBox(hWnd, msg, title_str, type);
}

/*
 * set_ctrl_text - コントロールに文字列リソースを設定
 */
static void set_ctrl_text(const HWND hDlg, const int ctrl_id, const UINT str_id)
{
	TCHAR buf[MSG_SIZE];

	SetDlgItemText(hDlg, ctrl_id, str_load(str_id, buf, MSG_SIZE));
}

/*
 * sb_add - 文字列バッファに追加
 */
static BOOL sb_add(STR_BUF *sb, const TCHAR *str)
{
	TCHAR *buf;
	DWORD len = lstrlen(str);
	DWORD size;

	if (sb->len + len + 1 > sb->size) {
		size = sb->size + ((len + 1 > 4096) ? len + 1 : 4096);
		buf = (sb->buf == NULL) ?
			(TCHAR *)HeapAlloc(GetProcessHeap(), 0, size * sizeof(TCHAR)) :
			(TCHAR *)HeapReAlloc(GetProcessHeap(), 0, sb->buf, size * sizeof(TCHAR));
		if (buf == NULL) {
			return FALSE;
		}
		sb->buf = buf;
		sb->size = size;
	}
	lstrcpy(sb->buf + sb->len, str);
	sb->len += len;
	return TRUE;
}

/*
 * sb_free - 文字列バッファの解放
 */
static void sb_free(STR_BUF *sb)
{
	if (sb->buf != NULL) {
		HeapFree(GetProcessHeap(), 0, sb->buf);
		sb->buf = NULL;
	}
	sb->len = 0;
	sb->size = 0;
}

/*
 * get_module_dir - 実行ファイルのあるフォルダを取得
 */
static void get_module_dir(TCHAR *path)
{
	TCHAR *p;

	*path = TEXT('\0');
	GetModuleFileName(NULL, path, MAX_PATH);
	for (p = path + lstrlen(path); p > path; p--) {
		if (*(p - 1) == TEXT('\\')) {
			*(p - 1) = TEXT('\0');
			break;
		}
	}
}

/*
 * build_path - フォルダ名とファイル名の連結 (バッファに収まらない場合は FALSE)
 */
static BOOL build_path(TCHAR *buf, const int len, const TCHAR *dir, const TCHAR *name)
{
	if (wnsprintf(buf, len, TEXT("%s\\%s"), dir, name) < 0) {
		*buf = TEXT('\0');
		return FALSE;
	}
	return TRUE;
}

/*
 * path_has_dotdot - 親フォルダを参照する ".." の要素を含むかどうか
 */
static BOOL path_has_dotdot(const TCHAR *path)
{
	const TCHAR *p;

	for (p = path; *p != TEXT('\0'); p++) {
		if (*p != TEXT('.') || *(p + 1) != TEXT('.')) {
			continue;
		}
		if ((p == path || *(p - 1) == TEXT('\\')) &&
			(*(p + 2) == TEXT('\0') || *(p + 2) == TEXT('\\'))) {
			return TRUE;
		}
	}
	return FALSE;
}

/*
 * path_normalize - 絶対パスに正規化 (末尾の '\' は除く)
 */
static BOOL path_normalize(const TCHAR *path, TCHAR *buf)
{
	DWORD len;

	if (*path == TEXT('\0')) {
		return FALSE;
	}
	len = GetFullPathName(path, MAX_PATH, buf, NULL);
	if (len == 0 || len >= MAX_PATH) {
		*buf = TEXT('\0');
		return FALSE;
	}
	// "\\?\" や "\\.\" で始まるパスは ".." が解決されないため対象外
	if (*buf == TEXT('\\') && *(buf + 1) == TEXT('\\') &&
		(*(buf + 2) == TEXT('?') || *(buf + 2) == TEXT('.')) && *(buf + 3) == TEXT('\\')) {
		*buf = TEXT('\0');
		return FALSE;
	}
	if (path_has_dotdot(buf) == TRUE) {
		*buf = TEXT('\0');
		return FALSE;
	}
	for (; len > 3 && *(buf + len - 1) == TEXT('\\'); len--) {
		*(buf + len - 1) = TEXT('\0');
	}
	return TRUE;
}

/*
 * path_is_under - path が dir の配下 (dir 自身は含まない) かどうか
 */
static BOOL path_is_under(const TCHAR *dir, const TCHAR *path)
{
	int len = lstrlen(dir);

	if (len == 0 ||
		CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, path, len, dir, len) != CSTR_EQUAL) {
		return FALSE;
	}
	if (*(dir + len - 1) == TEXT('\\')) {
		// ドライブ直下
		return (*(path + len) != TEXT('\0')) ? TRUE : FALSE;
	}
	return (*(path + len) == TEXT('\\') && *(path + len + 1) != TEXT('\0')) ? TRUE : FALSE;
}

/*
 * create_dir - フォルダの作成 (親フォルダも作成、log が TRUE なら作成したフォルダを記録)
 *
 *	sa を指定した場合は作成するすべてのフォルダにそのアクセス許可を設定する
 */
static BOOL create_dir(const TCHAR *path, const BOOL log, SECURITY_ATTRIBUTES *sa)
{
	TCHAR buf[MAX_PATH];
	TCHAR *p;

	if (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES) {
		return TRUE;
	}
	lstrcpyn(buf, path, MAX_PATH);
	for (p = buf; *p != TEXT('\0'); p++) {
		if (*p != TEXT('\\') || p == buf || *(p - 1) == TEXT(':') || *(p - 1) == TEXT('\\')) {
			continue;
		}
		*p = TEXT('\0');
		if (GetFileAttributes(buf) == INVALID_FILE_ATTRIBUTES &&
			CreateDirectory(buf, sa) != FALSE && log == TRUE) {
			log_add(TEXT('D'), buf);
		}
		*p = TEXT('\\');
	}
	if (CreateDirectory(path, sa) != FALSE && log == TRUE) {
		log_add(TEXT('D'), path);
	}
	return (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES) ? TRUE : FALSE;
}

/*
 * install_path_in_program_files - インストール先が Program Files の配下かどうか
 */
static BOOL install_path_in_program_files(void)
{
	static const int csidl_list[] = { CSIDL_PROGRAM_FILES, CSIDL_PROGRAM_FILESX86 };
	TCHAR target[MAX_PATH];
	TCHAR dir[MAX_PATH];
	TCHAR norm[MAX_PATH];
	DWORD len;
	int i;

	if (path_normalize(install_path, target) == FALSE) {
		return FALSE;
	}
	for (i = 0; i < sizeof(csidl_list) / sizeof(int); i++) {
		if (SUCCEEDED(SHGetFolderPath(NULL, csidl_list[i], NULL, SHGFP_TYPE_CURRENT, dir)) &&
			path_normalize(dir, norm) == TRUE && path_is_under(norm, target) == TRUE) {
			return TRUE;
		}
	}
	// 32bit のプロセスからは 64bit 用の Program Files を環境変数で取得する
	len = GetEnvironmentVariable(TEXT("ProgramW6432"), dir, MAX_PATH);
	if (len > 0 && len < MAX_PATH &&
		path_normalize(dir, norm) == TRUE && path_is_under(norm, target) == TRUE) {
		return TRUE;
	}
	return FALSE;
}

/*
 * get_user_token - デスクトップを実行しているユーザーの昇格していないトークンを取得
 */
static HANDLE get_user_token(void)
{
	HANDLE hProcess, hToken, hImpToken = NULL;
	TOKEN_LINKED_TOKEN tlt;
	HWND hShell;
	DWORD pid = 0;
	DWORD len;

	// 別の管理者アカウントで昇格している場合もログオン中のユーザーで判定する
	if ((hShell = GetShellWindow()) != NULL) {
		GetWindowThreadProcessId(hShell, &pid);
	}
	if (pid != 0 && (hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid)) != NULL) {
		if (OpenProcessToken(hProcess, TOKEN_QUERY | TOKEN_DUPLICATE, &hToken) != FALSE) {
			if (DuplicateToken(hToken, SecurityIdentification, &hImpToken) == FALSE) {
				hImpToken = NULL;
			}
			CloseHandle(hToken);
		}
		CloseHandle(hProcess);
	}
	if (hImpToken != NULL) {
		return hImpToken;
	}
	// 昇格前のトークン
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
 * user_has_rights - トークンに rights のいずれかが許可されているか (判定できない場合は TRUE)
 */
static BOOL user_has_rights(const HANDLE hToken, const TCHAR *path, const DWORD rights)
{
	PSECURITY_DESCRIPTOR psd = NULL;
	GENERIC_MAPPING gm = { FILE_GENERIC_READ, FILE_GENERIC_WRITE, FILE_GENERIC_EXECUTE, FILE_ALL_ACCESS };
	BYTE ps_buf[sizeof(PRIVILEGE_SET) + sizeof(LUID_AND_ATTRIBUTES) * 8];
	DWORD ps_len = sizeof(ps_buf);
	DWORD granted = 0;
	BOOL status = FALSE;
	BOOL ret = TRUE;
	HANDLE hDir;

	hDir = CreateFile(path, READ_CONTROL, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
	if (hDir == INVALID_HANDLE_VALUE) {
		return TRUE;
	}
	if (GetSecurityInfo(hDir, SE_FILE_OBJECT,
		OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION,
		NULL, NULL, NULL, NULL, &psd) == ERROR_SUCCESS) {
		if (AccessCheck(psd, hToken, MAXIMUM_ALLOWED, &gm, (PPRIVILEGE_SET)ps_buf, &ps_len, &granted, &status) != FALSE) {
			ret = (status != FALSE && (granted & rights) != 0) ? TRUE : FALSE;
		}
		LocalFree(psd);
	}
	CloseHandle(hDir);
	return ret;
}

/*
 * install_path_is_protected - インストール先とその上位のフォルダを一般ユーザーが変更できないか
 */
static BOOL install_path_is_protected(void)
{
	TCHAR dir[MAX_PATH];
	HANDLE hToken;
	DWORD attr;
	TCHAR *p;
	BOOL first = TRUE;
	BOOL ret = TRUE;

	if (path_normalize(install_path, dir) == FALSE || (hToken = get_user_token()) == NULL) {
		return FALSE;
	}
	for (;;) {
		// リンクの先は判定できないため使用しない
		attr = GetFileAttributes(dir);
		if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_REPARSE_POINT) != 0 ||
			user_has_rights(hToken, dir, (first == TRUE) ? RIGHTS_INSTALL_DIR : RIGHTS_PARENT_DIR) == TRUE) {
			ret = FALSE;
			break;
		}
		if (lstrlen(dir) <= 3 || (p = StrRChr(dir, NULL, TEXT('\\'))) == NULL) {
			break;
		}
		if (p == dir + 2 && *(dir + 1) == TEXT(':')) {
			// ドライブ直下
			*(p + 1) = TEXT('\0');
		} else {
			*p = TEXT('\0');
		}
		first = FALSE;
	}
	CloseHandle(hToken);
	return ret;
}

/*
 * remove_obsolete_file - 前回インストールした記録がある廃止したファイルを削除
 */
static void remove_obsolete_file(const TCHAR *old_log, const TCHAR *name)
{
	TCHAR path[MAX_PATH];
	const TCHAR *p, *r;
	int len;

	if (old_log == NULL || build_path(path, MAX_PATH, install_path, name) == FALSE) {
		return;
	}
	len = lstrlen(path);
	for (p = old_log; *p != TEXT('\0'); p = r) {
		for (r = p; *r != TEXT('\0') && *r != TEXT('\r') && *r != TEXT('\n'); r++)
			;
		if (*p == TEXT('F') && r - p - 1 == len &&
			CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, p + 1, len, path, len) == CSTR_EQUAL) {
			if (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES) {
				SetFileAttributes(path, FILE_ATTRIBUTE_NORMAL);
				if (DeleteFile(path) == FALSE) {
					move_in_use_file(path);
				}
			}
			return;
		}
		for (; *r == TEXT('\r') || *r == TEXT('\n'); r++)
			;
	}
}

/*
 * delete_dir_tree - フォルダを中身ごと削除 (リパースポイントの先はたどらない)
 */
static void delete_dir_tree(const TCHAR *path)
{
	WIN32_FIND_DATA fd;
	HANDLE hFind;
	TCHAR buf[MAX_PATH];
	DWORD attr;

	attr = GetFileAttributes(path);
	if (attr == INVALID_FILE_ATTRIBUTES ||
		(attr & FILE_ATTRIBUTE_DIRECTORY) == 0 || (attr & FILE_ATTRIBUTE_REPARSE_POINT) != 0) {
		return;
	}
	if (build_path(buf, MAX_PATH, path, TEXT("*")) == FALSE) {
		return;
	}
	if ((hFind = FindFirstFile(buf, &fd)) != INVALID_HANDLE_VALUE) {
		do {
			if (lstrcmp(fd.cFileName, TEXT(".")) == 0 || lstrcmp(fd.cFileName, TEXT("..")) == 0) {
				continue;
			}
			if (build_path(buf, MAX_PATH, path, fd.cFileName) == FALSE) {
				continue;
			}
			if ((fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) {
				// リンク自体のみ削除
				if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
					RemoveDirectory(buf);
				} else {
					DeleteFile(buf);
				}
			} else if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
				delete_dir_tree(buf);
			} else {
				SetFileAttributes(buf, FILE_ATTRIBUTE_NORMAL);
				DeleteFile(buf);
			}
		} while (FindNextFile(hFind, &fd) != FALSE);
		FindClose(hFind);
	}
	RemoveDirectory(path);
}

/*
 * move_in_use_file - 使用中のファイルを退避して再起動時に削除
 */
static BOOL move_in_use_file(const TCHAR *path)
{
	TCHAR old_path[MAX_PATH];

	if (wnsprintf(old_path, MAX_PATH, TEXT("%s%s"), path, OLD_FILE_EXT) < 0 ||
		MoveFileEx(path, old_path, MOVEFILE_REPLACE_EXISTING) == FALSE) {
		return FALSE;
	}
	MoveFileEx(old_path, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
	log_add(TEXT('F'), old_path);
	return TRUE;
}

/*
 * write_file - ファイルの書き込み (使用中の場合は退避、できなければ終了を促す)
 */
static BOOL write_file(const HWND hWnd, const TCHAR *path, const BYTE *buf, const DWORD size)
{
	HANDLE hFile;
	DWORD ret_size;
	BOOL moved = FALSE;

	for (;;) {
		if (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES) {
			SetFileAttributes(path, FILE_ATTRIBUTE_NORMAL);
		}
		hFile = CreateFile(path, GENERIC_WRITE, 0, NULL,
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE) {
			break;
		}
		if ((GetLastError() == ERROR_SHARING_VIOLATION || GetLastError() == ERROR_USER_MAPPED_FILE) &&
			moved == FALSE) {
			moved = move_in_use_file(path);
			if (moved == TRUE) {
				continue;
			}
		}
		// 起動中で上書きできない場合は終了を促す
		if (msg_box(hWnd, IDS_FILE_LOCKED, MB_RETRYCANCEL | MB_ICONWARNING, path) != IDRETRY) {
			return FALSE;
		}
	}
	if (size > 0 && WriteFile(hFile, buf, size, &ret_size, NULL) == FALSE) {
		CloseHandle(hFile);
		return FALSE;
	}
	CloseHandle(hFile);
	return TRUE;
}

/*
 * is_running - CLCLが起動中か
 */
static BOOL is_running(void)
{
	return (FindWindow(MAIN_WND_CLASS, MAIN_WINDOW_TITLE) != NULL) ? TRUE : FALSE;
}

/*
 * check_running - CLCLが起動中なら終了を促す
 */
static BOOL check_running(const HWND hWnd)
{
	while (is_running() == TRUE) {
		if (msg_box(hWnd, IDS_RUNNING, MB_RETRYCANCEL | MB_ICONWARNING, NULL) != IDRETRY) {
			return FALSE;
		}
	}
	return TRUE;
}

/*
 * set_language - UI言語の設定 (リソースがある言語のみ)
 */
static void set_language(void)
{
	LANGID langid = GetUserDefaultUILanguage();
	LANGID res_langid = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);

	switch (PRIMARYLANGID(langid)) {
	case LANG_JAPANESE:
		res_langid = MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT);
		break;
	case LANG_GERMAN:
		res_langid = MAKELANGID(LANG_GERMAN, SUBLANG_GERMAN);
		break;
	case LANG_UKRAINIAN:
		res_langid = MAKELANGID(LANG_UKRAINIAN, SUBLANG_DEFAULT);
		break;
	case LANG_CHINESE:
		if (SUBLANGID(langid) == SUBLANG_CHINESE_SIMPLIFIED ||
			SUBLANGID(langid) == SUBLANG_CHINESE_SINGAPORE) {
			res_langid = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
		}
		break;
	}
	SetThreadUILanguage(res_langid);
}

/*
 * is_wow64 - 64bit OS 上で動作しているか
 */
static BOOL is_wow64(void)
{
	BOOL wow64 = FALSE;

	if (IsWow64Process(GetCurrentProcess(), &wow64) == FALSE) {
		return FALSE;
	}
	return wow64;
}

/*
 * reg_open_uninstall - アプリの一覧のキーを開く
 */
static LONG reg_open_uninstall(const HKEY root, const REGSAM view, const REGSAM sam, HKEY *hKey)
{
	return RegOpenKeyEx(root, UNINSTALL_KEY, 0, sam | view, hKey);
}

/*
 * reg_get_string - レジストリから文字列を取得
 */
static BOOL reg_get_string(const HKEY hKey, const TCHAR *name, TCHAR *buf, const DWORD size)
{
	DWORD type;
	DWORD len = size * sizeof(TCHAR);

	*buf = TEXT('\0');
	if (RegQueryValueEx(hKey, name, NULL, &type, (BYTE *)buf, &len) != ERROR_SUCCESS ||
		(type != REG_SZ && type != REG_EXPAND_SZ)) {
		*buf = TEXT('\0');
		return FALSE;
	}
	// 終端が含まれていない場合に備えて終端を付ける
	len /= sizeof(TCHAR);
	buf[(len < size - 1) ? len : size - 1] = TEXT('\0');
	return TRUE;
}

/*
 * reg_is_clcl - CLCLの登録かどうか
 */
static BOOL reg_is_clcl(const TCHAR *display_name)
{
	int len = lstrlen(APP_NAME);

	// 「CLCL」または「CLCL 」で始まる表示名を CLCL の登録とみなす
	if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE,
		display_name, len, APP_NAME, len) != CSTR_EQUAL) {
		return FALSE;
	}
	return (*(display_name + len) == TEXT('\0') || *(display_name + len) == TEXT(' ')) ? TRUE : FALSE;
}

/*
 * reg_find_install - 既存のインストールを検索
 */
static int reg_find_install(REG_TARGET *list, const int max_cnt, TCHAR *install_loc)
{
	HKEY root_list[3];
	REGSAM view_list[3];
	HKEY hKey, hSubKey;
	TCHAR subkey[BUF_SIZE];
	TCHAR buf[MAX_PATH];
	DWORD index, len;
	int list_cnt = 0;
	int i;
	int cnt = 0;

	if (install_loc != NULL) {
		*install_loc = TEXT('\0');
	}
	// 64bit OS では 32bit と 64bit の両方のビューを検索する (32bit OS ではビューの指定は無視される)
	if (is_wow64() == TRUE) {
		root_list[list_cnt] = HKEY_LOCAL_MACHINE;
		view_list[list_cnt++] = KEY_WOW64_32KEY;
		root_list[list_cnt] = HKEY_LOCAL_MACHINE;
		view_list[list_cnt++] = KEY_WOW64_64KEY;
	} else {
		root_list[list_cnt] = HKEY_LOCAL_MACHINE;
		view_list[list_cnt++] = 0;
	}
	root_list[list_cnt] = HKEY_CURRENT_USER;
	view_list[list_cnt++] = 0;
	for (i = 0; i < list_cnt && cnt < max_cnt; i++) {
		if (reg_open_uninstall(root_list[i], view_list[i], KEY_READ, &hKey) != ERROR_SUCCESS) {
			continue;
		}
		for (index = 0; cnt < max_cnt; index++) {
			len = BUF_SIZE;
			if (RegEnumKeyEx(hKey, index, subkey, &len, NULL, NULL, NULL, NULL) != ERROR_SUCCESS) {
				break;
			}
			if (RegOpenKeyEx(hKey, subkey, 0, KEY_READ | view_list[i], &hSubKey) != ERROR_SUCCESS) {
				continue;
			}
			if (reg_get_string(hSubKey, TEXT("DisplayName"), buf, MAX_PATH) == TRUE &&
				reg_is_clcl(buf) == TRUE) {
				(list + cnt)->root = root_list[i];
				(list + cnt)->view = view_list[i];
				lstrcpyn((list + cnt)->subkey, subkey, BUF_SIZE);
				if (cnt == 0 && install_loc != NULL) {
					if (reg_get_string(hSubKey, TEXT("InstallLocation"), buf, MAX_PATH) == TRUE) {
						lstrcpyn(install_loc, buf, MAX_PATH);
					}
				}
				cnt++;
			}
			RegCloseKey(hSubKey);
		}
		RegCloseKey(hKey);
	}
	return cnt;
}

/*
 * reg_set_string - レジストリに文字列を設定
 */
static void reg_set_string(const HKEY hKey, const TCHAR *name, const TCHAR *value)
{
	RegSetValueEx(hKey, name, 0, REG_SZ, (const BYTE *)value, (lstrlen(value) + 1) * sizeof(TCHAR));
}

/*
 * reg_set_dword - レジストリに数値を設定
 */
static void reg_set_dword(const HKEY hKey, const TCHAR *name, const DWORD value)
{
	RegSetValueEx(hKey, name, 0, REG_DWORD, (const BYTE *)&value, sizeof(DWORD));
}

/*
 * reg_delete_install - アプリの一覧から削除
 */
static void reg_delete_install(const REG_TARGET *rt)
{
	HKEY hKey;

	if (reg_open_uninstall(rt->root, rt->view, KEY_READ | KEY_WRITE, &hKey) != ERROR_SUCCESS) {
		return;
	}
	RegDeleteKeyEx(hKey, rt->subkey, rt->view, 0);
	RegCloseKey(hKey);
}

/*
 * reg_clear_value - キーの値をすべて削除
 */
static void reg_clear_value(const HKEY hKey)
{
	TCHAR name[BUF_SIZE];
	DWORD len;

	// 以前のインストーラが設定した値が残らないようにする
	for (;;) {
		len = BUF_SIZE;
		if (RegEnumValue(hKey, 0, name, &len, NULL, NULL, NULL, NULL) != ERROR_SUCCESS ||
			RegDeleteValue(hKey, name) != ERROR_SUCCESS) {
			break;
		}
	}
}

/*
 * reg_write_install - アプリの一覧に登録
 */
static BOOL reg_write_install(const REG_TARGET *rt)
{
	HKEY hKey, hSubKey;
	SYSTEMTIME st;
	TCHAR buf[MAX_PATH * 2];
	int i;

	if (reg_open_uninstall(rt->root, rt->view, KEY_READ | KEY_WRITE, &hKey) != ERROR_SUCCESS) {
		return FALSE;
	}
	if (RegCreateKeyEx(hKey, rt->subkey, 0, NULL, REG_OPTION_NON_VOLATILE,
		KEY_READ | KEY_WRITE | rt->view, NULL, &hSubKey, NULL) != ERROR_SUCCESS) {
		RegCloseKey(hKey);
		return FALSE;
	}
	reg_clear_value(hSubKey);
	reg_set_string(hSubKey, TEXT("DisplayName"), APP_NAME);
	reg_set_string(hSubKey, TEXT("DisplayVersion"), ver_str);
	reg_set_string(hSubKey, TEXT("Publisher"), APP_PUBLISHER);
	reg_set_string(hSubKey, TEXT("URLInfoAbout"), APP_URL);
	reg_set_string(hSubKey, TEXT("InstallLocation"), install_path);
	wsprintf(buf, TEXT("%s\\%s,0"), install_path, APP_EXE);
	reg_set_string(hSubKey, TEXT("DisplayIcon"), buf);
	wsprintf(buf, TEXT("\"%s\\%s\" %s"), install_path, UNINSTALL_EXE, CMD_UNINSTALL);
	reg_set_string(hSubKey, TEXT("UninstallString"), buf);
	reg_set_dword(hSubKey, TEXT("NoModify"), 1);
	reg_set_dword(hSubKey, TEXT("NoRepair"), 1);
	reg_set_dword(hSubKey, TEXT("EstimatedSize"), (total_size + 1023) / 1024);
	reg_set_dword(hSubKey, TEXT("VersionMajor"), INST_VER_MAJOR);
	reg_set_dword(hSubKey, TEXT("VersionMinor"), INST_VER_MINOR);
	GetLocalTime(&st);
	wsprintf(buf, TEXT("%04d%02d%02d"), st.wYear, st.wMonth, st.wDay);
	reg_set_string(hSubKey, TEXT("InstallDate"), buf);
	RegCloseKey(hSubKey);
	RegCloseKey(hKey);

	// 二重に登録されている場合は取り除く
	for (i = 0; i < reg_target_cnt; i++) {
		if ((reg_target + i)->root == rt->root && (reg_target + i)->view == rt->view &&
			lstrcmpi((reg_target + i)->subkey, rt->subkey) == 0) {
			continue;
		}
		reg_delete_install(reg_target + i);
	}
	return TRUE;
}

/*
 * create_shortcut - ショートカットの作成
 */
static BOOL create_shortcut(const TCHAR *link_path, const TCHAR *target, const TCHAR *work_dir)
{
	IShellLinkW *psl;
	IPersistFile *ppf;
	BOOL ret = FALSE;

	if (FAILED(CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
		&IID_IShellLinkW, (void **)&psl))) {
		return FALSE;
	}
	IShellLinkW_SetPath(psl, target);
	IShellLinkW_SetWorkingDirectory(psl, work_dir);
	IShellLinkW_SetIconLocation(psl, target, 0);
	IShellLinkW_SetDescription(psl, APP_NAME);
	if (SUCCEEDED(IShellLinkW_QueryInterface(psl, &IID_IPersistFile, (void **)&ppf))) {
		ret = SUCCEEDED(IPersistFile_Save(ppf, link_path, TRUE)) ? TRUE : FALSE;
		IPersistFile_Release(ppf);
	}
	IShellLinkW_Release(psl);
	return ret;
}

/*
 * exec_as_user - エクスプローラ経由で起動 (管理者権限を引き継がない)
 */
static BOOL exec_as_user(const TCHAR *path, const TCHAR *dir)
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
	VARIANT v_empty, v_dir, v_show;
	BSTR bs_path = NULL, bs_dir = NULL;
	long hwnd = 0;
	BOOL ret = FALSE;

	VariantInit(&v_empty);
	VariantInit(&v_dir);
	VariantInit(&v_show);

	if (FAILED(CoCreateInstance(&CLSID_ShellWindows, NULL, CLSCTX_LOCAL_SERVER,
			&IID_IShellWindows, (void **)&psw)) ||
		FAILED(IShellWindows_FindWindowSW(psw, &v_empty, &v_empty,
			SWC_DESKTOP, &hwnd, SWFO_NEEDDISPATCH, &pdisp)) ||
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
	if ((bs_path = SysAllocString(path)) == NULL || (bs_dir = SysAllocString(dir)) == NULL) {
		goto end;
	}
	v_dir.vt = VT_BSTR;
	v_dir.bstrVal = bs_dir;
	v_show.vt = VT_I4;
	v_show.lVal = SW_SHOWNORMAL;
	ret = SUCCEEDED(IShellDispatch2_ShellExecute(psd, bs_path,
		v_empty, v_dir, v_empty, v_show)) ? TRUE : FALSE;

end:
	if (bs_path != NULL) {
		SysFreeString(bs_path);
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

/*
 * exec_app - CLCLの起動
 */
static BOOL exec_app(void)
{
	TCHAR path[MAX_PATH];

	if (build_path(path, MAX_PATH, install_path, APP_EXE) == FALSE) {
		return FALSE;
	}
	if (exec_as_user(path, install_path) == TRUE) {
		return TRUE;
	}
	return ((INT_PTR)ShellExecute(NULL, NULL, path, NULL, install_path, SW_SHOWNORMAL) > 32) ? TRUE : FALSE;
}

/*
 * load_payload - リソースからインストールするファイルを取得
 */
static BOOL load_payload(void)
{
	HRSRC hRes;
	HGLOBAL hMem;
	const BYTE *data;
	ZIP_ENTRY ze;
	DWORD size;
	DWORD i;

	if ((hRes = FindResource(hInst, MAKEINTRESOURCE(IDR_PAYLOAD), RT_RCDATA)) == NULL ||
		(size = SizeofResource(hInst, hRes)) == 0 ||
		(hMem = LoadResource(hInst, hRes)) == NULL ||
		(data = (const BYTE *)LockResource(hMem)) == NULL) {
		return FALSE;
	}
	if (zip_open(data, size, &zip) == FALSE || zip.count == 0) {
		return FALSE;
	}
	// インストールサイズの取得
	total_size = 0;
	for (i = 0; i < zip.count; i++) {
		if (zip_get_entry(&zip, i, &ze) == TRUE) {
			total_size += ze.orig_size;
		}
	}
	return TRUE;
}

/*
 * log_add - インストールしたファイルを記録
 */
static void log_add(const TCHAR type, const TCHAR *path)
{
	TCHAR buf[MAX_PATH + 4];

	wsprintf(buf, TEXT("%c%s\r\n"), type, path);
	sb_add(&log_buf, buf);
}

/*
 * log_load - インストールしたファイルの一覧を読み込む (戻り値は HeapFree で解放)
 */
static TCHAR *log_load(const TCHAR *dir_path)
{
	HANDLE hFile;
	TCHAR path[MAX_PATH];
	BYTE *buf;
	TCHAR *ret;
	DWORD size, read_size;

	if (build_path(path, MAX_PATH, dir_path, UNINSTALL_LOG) == FALSE) {
		return NULL;
	}
	if ((hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
		return NULL;
	}
	size = GetFileSize(hFile, NULL);
	if (size == 0xffffffff || size < sizeof(TCHAR) ||
		(buf = (BYTE *)HeapAlloc(GetProcessHeap(), 0, size + sizeof(TCHAR))) == NULL) {
		CloseHandle(hFile);
		return NULL;
	}
	if (ReadFile(hFile, buf, size, &read_size, NULL) == FALSE) {
		HeapFree(GetProcessHeap(), 0, buf);
		CloseHandle(hFile);
		return NULL;
	}
	CloseHandle(hFile);
	*((TCHAR *)(buf + (read_size & ~1))) = TEXT('\0');

	// BOM は取り除く
	ret = (TCHAR *)buf;
	if (*ret == LOG_BOM) {
		MoveMemory(ret, ret + 1, (lstrlen(ret + 1) + 1) * sizeof(TCHAR));
	}
	return ret;
}

/*
 * log_save - インストールしたファイルの一覧を保存
 */
static BOOL log_save(void)
{
	TCHAR path[MAX_PATH];
	TCHAR bom = LOG_BOM;
	HANDLE hFile;
	DWORD ret_size;

	if (build_path(path, MAX_PATH, install_path, UNINSTALL_LOG) == FALSE) {
		return FALSE;
	}
	if (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES) {
		SetFileAttributes(path, FILE_ATTRIBUTE_NORMAL);
	}
	if ((hFile = CreateFile(path, GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
		return FALSE;
	}
	WriteFile(hFile, &bom, sizeof(TCHAR), &ret_size, NULL);
	if (log_buf.buf != NULL) {
		WriteFile(hFile, log_buf.buf, log_buf.len * sizeof(TCHAR), &ret_size, NULL);
	}
	CloseHandle(hFile);
	return TRUE;
}

/*
 * str_to_dword - 文字列を数値に変換
 */
static DWORD str_to_dword(const TCHAR *p)
{
	DWORD ret = 0;

	for (; *p >= TEXT('0') && *p <= TEXT('9'); p++) {
		ret = ret * 10 + (DWORD)(*p - TEXT('0'));
	}
	return ret;
}

/*
 * path_is_safe - 展開先として安全なパスかどうか
 */
static BOOL path_is_safe(const TCHAR *name)
{
	if (*name == TEXT('\\') || *name == TEXT('\0') || *(name + 1) == TEXT(':')) {
		return FALSE;
	}
	return (path_has_dotdot(name) == TRUE) ? FALSE : TRUE;
}

/*
 * create_parent_dir - 親フォルダの作成
 */
static void create_parent_dir(const TCHAR *path)
{
	TCHAR buf[MAX_PATH];
	TCHAR *p;

	lstrcpyn(buf, path, MAX_PATH);
	for (p = buf + lstrlen(buf); p > buf; p--) {
		if (*(p - 1) == TEXT('\\')) {
			*(p - 1) = TEXT('\0');
			break;
		}
	}
	if (p > buf && *buf != TEXT('\0')) {
		create_dir(buf, TRUE, NULL);
	}
}

/*
 * get_shortcut_path - ショートカットのパスを取得
 */
static BOOL get_shortcut_path(const int csidl, TCHAR *path)
{
	TCHAR dir[MAX_PATH];

	if (FAILED(SHGetFolderPath(NULL, csidl, NULL, SHGFP_TYPE_CURRENT, dir))) {
		return FALSE;
	}
	if (lstrlen(dir) + lstrlen(APP_NAME) + 6 > MAX_PATH) {
		return FALSE;
	}
	wsprintf(path, TEXT("%s\\%s.lnk"), dir, APP_NAME);
	return TRUE;
}

/*
 * shortcut_exists - ショートカットがあるかどうか
 */
static BOOL shortcut_exists(const int csidl)
{
	TCHAR path[MAX_PATH];

	if (get_shortcut_path(csidl, path) == FALSE) {
		return FALSE;
	}
	return (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES) ? TRUE : FALSE;
}

/*
 * set_shortcut - ショートカットの作成と削除
 */
static void set_shortcut(const int csidl, const BOOL create)
{
	TCHAR path[MAX_PATH];
	TCHAR target[MAX_PATH];

	if (get_shortcut_path(csidl, path) == FALSE) {
		return;
	}
	if (create == FALSE) {
		DeleteFile(path);
		return;
	}
	if (build_path(target, MAX_PATH, install_path, APP_EXE) == FALSE) {
		return;
	}
	if (create_shortcut(path, target, install_path) == TRUE) {
		log_add(TEXT('F'), path);
	}
}

/*
 * copy_file - ファイルのコピー (使用中の場合は終了を促す)
 */
static BOOL copy_file(const HWND hWnd, const TCHAR *from, const TCHAR *to)
{
	for (;;) {
		if (GetFileAttributes(to) != INVALID_FILE_ATTRIBUTES) {
			SetFileAttributes(to, FILE_ATTRIBUTE_NORMAL);
		}
		if (CopyFile(from, to, FALSE) != FALSE) {
			return TRUE;
		}
		if (msg_box(hWnd, IDS_FILE_LOCKED, MB_RETRYCANCEL | MB_ICONWARNING, to) != IDRETRY) {
			return FALSE;
		}
	}
}

/*
 * extract_files - ファイルの展開とアンインストーラの配置
 */
static BOOL extract_files(const HWND hDlg)
{
	ZIP_ENTRY ze;
	BYTE *buf;
	DWORD size;
	DWORD i;
	TCHAR path[MAX_PATH];
	TCHAR self[MAX_PATH];
	BOOL ret;

	set_ctrl_text(hDlg, IDC_STATIC_STATUS, IDS_STATUS_EXTRACT);
	SendDlgItemMessage(hDlg, IDC_PROGRESS, PBM_SETRANGE32, 0, (LPARAM)zip.count);
	for (i = 0; i < zip.count; i++) {
		SendDlgItemMessage(hDlg, IDC_PROGRESS, PBM_SETPOS, (WPARAM)i, 0);
		if (zip_get_entry(&zip, i, &ze) == FALSE || path_is_safe(ze.name) == FALSE) {
			continue;
		}
		if (build_path(path, MAX_PATH, install_path, ze.name) == FALSE) {
			msg_box(hDlg, IDS_ERR_EXTRACT, MB_OK | MB_ICONEXCLAMATION, ze.name);
			return FALSE;
		}
		if (ze.is_dir == TRUE) {
			create_dir(path, TRUE, NULL);
			continue;
		}
		if (zip_extract(&zip, &ze, &buf, &size) != UNZIP_OK) {
			msg_box(hDlg, IDS_ERR_EXTRACT, MB_OK | MB_ICONEXCLAMATION, ze.name);
			return FALSE;
		}
		create_parent_dir(path);
		ret = write_file(hDlg, path, buf, size);
		HeapFree(GetProcessHeap(), 0, buf);
		if (ret == FALSE) {
			return FALSE;
		}
		log_add(TEXT('F'), path);
	}
	SendDlgItemMessage(hDlg, IDC_PROGRESS, PBM_SETPOS, (WPARAM)zip.count, 0);

	// 以前のインストーラがインストール先に残したファイルも削除対象にする
	if (build_path(path, MAX_PATH, install_path, OLD_INSTALL_LOG) == TRUE &&
		GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES) {
		log_add(TEXT('F'), path);
	}

	// アンインストーラの配置
	GetModuleFileName(NULL, self, MAX_PATH);
	if (build_path(path, MAX_PATH, install_path, UNINSTALL_EXE) == FALSE) {
		msg_box(hDlg, IDS_ERR_EXTRACT, MB_OK | MB_ICONEXCLAMATION, UNINSTALL_EXE);
		return FALSE;
	}
	return copy_file(hDlg, self, path);
}

/*
 * do_install - インストールの実行
 */
static BOOL do_install(const HWND hDlg)
{
	REG_TARGET rt;
	TCHAR *old_log;
	BOOL ret;

	// CLCLが起動中なら終了を促す
	if (check_running(hDlg) == FALSE) {
		return FALSE;
	}
	// インストール先の作成
	if (install_path_in_program_files() == TRUE) {
		if (create_dir(install_path, FALSE, NULL) == FALSE) {
			msg_box(hDlg, IDS_ERR_CREATE_DIR, MB_OK | MB_ICONEXCLAMATION, install_path);
			return FALSE;
		}
	} else {
		// 全ユーザーのスタートアップや管理者で実行するアンインストーラから起動されるため
		// 一般ユーザーが CLCL を差し替えられないフォルダにのみインストールする
		SECURITY_ATTRIBUTES sa;
		PSECURITY_DESCRIPTOR psd = NULL;
		BOOL created;

		if (ConvertStringSecurityDescriptorToSecurityDescriptor(INSTALL_DIR_SDDL, SDDL_REVISION_1, &psd, NULL) == FALSE) {
			msg_box(hDlg, IDS_ERR_CREATE_DIR, MB_OK | MB_ICONEXCLAMATION, install_path);
			return FALSE;
		}
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = psd;
		sa.bInheritHandle = FALSE;
		created = (GetFileAttributes(install_path) == INVALID_FILE_ATTRIBUTES) ? TRUE : FALSE;
		ret = create_dir(install_path, FALSE, &sa);
		LocalFree(psd);
		if (ret == FALSE) {
			msg_box(hDlg, IDS_ERR_CREATE_DIR, MB_OK | MB_ICONEXCLAMATION, install_path);
			return FALSE;
		}
		if (*install_path == TEXT('\\')) {
			// ネットワーク上のフォルダはアクセス許可を確認できない
			if (msg_box(hDlg, IDS_CONFIRM_PATH_NETWORK, MB_YESNO | MB_ICONWARNING, NULL) != IDYES) {
				return FALSE;
			}
		} else if (install_path_is_protected() == FALSE) {
			if (created == TRUE) {
				RemoveDirectory(install_path);
			}
			msg_box(hDlg, IDS_ERR_PATH_UNSAFE, MB_OK | MB_ICONEXCLAMATION, install_path);
			return FALSE;
		}
	}
	// 再試行の場合は前回の記録を破棄する (保存済みの一覧から引き継ぐ)
	sb_free(&log_buf);
	// 前回インストールしたファイルの一覧を引き継ぐ
	old_log = log_load(install_path);

	// ファイルの展開
	ret = extract_files(hDlg);
	if (ret == TRUE) {
		// 廃止したファイルの削除
		remove_obsolete_file(old_log, OBSOLETE_HOOK_LIB);

		// ショートカットの作成
		set_ctrl_text(hDlg, IDC_STATIC_STATUS, IDS_STATUS_SHORTCUT);
		set_shortcut(CSIDL_COMMON_STARTUP,
			(IsDlgButtonChecked(hDlg, IDC_CHECK_STARTUP) == BST_CHECKED) ? TRUE : FALSE);
		set_shortcut(CSIDL_COMMON_PROGRAMS,
			(IsDlgButtonChecked(hDlg, IDC_CHECK_STARTMENU) == BST_CHECKED) ? TRUE : FALSE);
		set_shortcut(CSIDL_COMMON_DESKTOPDIRECTORY,
			(IsDlgButtonChecked(hDlg, IDC_CHECK_DESKTOP) == BST_CHECKED) ? TRUE : FALSE);

		// アプリの一覧に登録
		set_ctrl_text(hDlg, IDC_STATIC_STATUS, IDS_STATUS_REGIST);
		if (reg_target_cnt > 0) {
			// 二重に登録されないように既存の登録を使用する
			rt = *reg_target;
		} else {
			rt.root = HKEY_LOCAL_MACHINE;
			rt.view = (is_wow64() == TRUE) ? KEY_WOW64_32KEY : 0;
			lstrcpy(rt.subkey, UNINSTALL_SUBKEY);
		}
		if (reg_write_install(&rt) == FALSE) {
			msg_box(hDlg, IDS_ERR_REGIST, MB_OK | MB_ICONEXCLAMATION, NULL);
			ret = FALSE;
		}
	}

	// 途中で失敗した場合も配置済みのファイルを一覧に保存する
	if (old_log != NULL) {
		sb_add(&log_buf, old_log);
		HeapFree(GetProcessHeap(), 0, old_log);
	}
	log_save();
	return ret;
}

/*
 * set_font_proc - 子ウィンドウにフォントを設定
 */
static BOOL CALLBACK set_font_proc(HWND hWnd, LPARAM lParam)
{
	SendMessage(hWnd, WM_SETFONT, (WPARAM)lParam, MAKELPARAM(TRUE, 0));
	return TRUE;
}

/*
 * set_dialog_font - システムの既定のフォントを設定
 */
static void set_dialog_font(const HWND hDlg)
{
	NONCLIENTMETRICS ncm;
	LOGFONT lf;

	// リソースのフォントは文字セットが環境に依存するため、
	// システムが実際に使用しているフォントを設定する
	ZeroMemory(&ncm, sizeof(NONCLIENTMETRICS));
	ncm.cbSize = sizeof(NONCLIENTMETRICS);
	if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
		sizeof(NONCLIENTMETRICS), &ncm, 0) != FALSE) {
		lf = ncm.lfMessageFont;
		if ((hDlgFont = CreateFontIndirect(&lf)) != NULL) {
			SendMessage(hDlg, WM_SETFONT, (WPARAM)hDlgFont, MAKELPARAM(TRUE, 0));
			EnumChildWindows(hDlg, set_font_proc, (LPARAM)hDlgFont);
		}
	} else if (GetObject((HFONT)SendMessage(hDlg, WM_GETFONT, 0, 0),
		sizeof(LOGFONT), &lf) == 0) {
		return;
	}
	// タイトルは太字で表示
	lf.lfWeight = FW_BOLD;
	if ((hTitleFont = CreateFontIndirect(&lf)) != NULL) {
		SendDlgItemMessage(hDlg, IDC_STATIC_TITLE, WM_SETFONT,
			(WPARAM)hTitleFont, MAKELPARAM(TRUE, 0));
	}
}

/*
 * browse_proc - フォルダ選択のコールバック
 */
static int CALLBACK browse_proc(HWND hWnd, UINT msg, LPARAM lParam, LPARAM lpData)
{
	if (msg == BFFM_INITIALIZED) {
		SendMessage(hWnd, BFFM_SETSELECTION, (WPARAM)TRUE, lpData);
	}
	return 0;
}

/*
 * browse_folder - インストール先の選択
 */
static void browse_folder(const HWND hDlg)
{
	BROWSEINFO bi;
	LPITEMIDLIST pidl;
	TCHAR path[MAX_PATH];
	TCHAR title[MSG_SIZE];
	TCHAR *p;

	GetDlgItemText(hDlg, IDC_EDIT_PATH, path, MAX_PATH);
	ZeroMemory(&bi, sizeof(BROWSEINFO));
	bi.hwndOwner = hDlg;
	bi.lpszTitle = str_load(IDS_SELECT_FOLDER, title, MSG_SIZE);
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	bi.lpfn = browse_proc;
	bi.lParam = (LPARAM)path;
	if ((pidl = SHBrowseForFolder(&bi)) == NULL) {
		return;
	}
	if (SHGetPathFromIDList(pidl, path) != FALSE) {
		// 選択したフォルダ名が異なる場合はフォルダ名を付加
		for (p = path + lstrlen(path); p > path && *(p - 1) != TEXT('\\'); p--)
			;
		if (lstrcmpi(p, DEFAULT_FOLDER) != 0 &&
			lstrlen(path) + lstrlen(DEFAULT_FOLDER) + 2 <= MAX_PATH) {
			if (*(path + lstrlen(path) - 1) != TEXT('\\')) {
				lstrcat(path, TEXT("\\"));
			}
			lstrcat(path, DEFAULT_FOLDER);
		}
		SetDlgItemText(hDlg, IDC_EDIT_PATH, path);
	}
	CoTaskMemFree(pidl);
}

/*
 * enable_ctrl - コントロールの有効と無効
 */
static void enable_ctrl(const HWND hDlg, const BOOL enable)
{
	static const int ctrl_list[] = {
		IDC_EDIT_PATH, IDC_BUTTON_BROWSE, IDC_CHECK_STARTUP,
		IDC_CHECK_STARTMENU, IDC_CHECK_DESKTOP, IDOK, IDCANCEL };
	int i;

	for (i = 0; i < (int)(sizeof(ctrl_list) / sizeof(ctrl_list[0])); i++) {
		EnableWindow(GetDlgItem(hDlg, ctrl_list[i]), enable);
	}
}

/*
 * show_complete - インストール完了の表示
 */
static void show_complete(const HWND hDlg)
{
	static const int ctrl_list[] = {
		IDC_STATIC_FOLDER, IDC_EDIT_PATH, IDC_BUTTON_BROWSE, IDC_CHECK_STARTUP,
		IDC_CHECK_STARTMENU, IDC_CHECK_DESKTOP, IDC_PROGRESS, IDC_STATIC_STATUS, IDCANCEL };
	int i;

	for (i = 0; i < (int)(sizeof(ctrl_list) / sizeof(ctrl_list[0])); i++) {
		ShowWindow(GetDlgItem(hDlg, ctrl_list[i]), SW_HIDE);
	}
	set_ctrl_text(hDlg, IDC_STATIC_DESC, IDS_DESC_COMPLETE);
	set_ctrl_text(hDlg, IDOK, IDS_BUTTON_FINISH);
	// 起動するかの確認
	ShowWindow(GetDlgItem(hDlg, IDC_CHECK_RUN), SW_SHOW);
	CheckDlgButton(hDlg, IDC_CHECK_RUN, BST_CHECKED);
	EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
	SetFocus(GetDlgItem(hDlg, IDOK));
}

/*
 * get_name_len_max - インストール先に付加するファイル名の最大長
 */
static int get_name_len_max(void)
{
	static const TCHAR * const name_list[] = { APP_EXE, UNINSTALL_EXE, UNINSTALL_LOG, OLD_INSTALL_LOG };
	ZIP_ENTRY ze;
	DWORD i;
	int len;
	int max_len = 0;

	for (i = 0; i < sizeof(name_list) / sizeof(name_list[0]); i++) {
		if ((len = lstrlen(name_list[i])) > max_len) {
			max_len = len;
		}
	}
	for (i = 0; i < zip.count; i++) {
		if (zip_get_entry(&zip, i, &ze) == TRUE && (len = lstrlen(ze.name)) > max_len) {
			max_len = len;
		}
	}
	// 使用中のファイルを退避するときに付加する拡張子の分
	return max_len + lstrlen(OLD_FILE_EXT);
}

/*
 * check_install_path - インストール先として使えるパスかどうか (エラーの文字列IDを返す)
 */
static UINT check_install_path(const TCHAR *path)
{
	const TCHAR *p;

	if (((*path >= TEXT('A') && *path <= TEXT('Z')) || (*path >= TEXT('a') && *path <= TEXT('z'))) &&
		*(path + 1) == TEXT(':') && *(path + 2) == TEXT('\\')) {
		// X:\...
		p = path + 3;
	} else if (*path == TEXT('\\') && *(path + 1) == TEXT('\\') &&
		*(path + 2) != TEXT('\\') && *(path + 2) != TEXT('?') && *(path + 2) != TEXT('.') &&
		*(path + 2) != TEXT('\0')) {
		// \\server\share\...
		for (p = path + 2; *p != TEXT('\0') && *p != TEXT('\\'); p++)
			;
		if (*p == TEXT('\0') || *(p + 1) == TEXT('\\') || *(p + 1) == TEXT('\0')) {
			return IDS_ERR_PATH_INVALID;
		}
		for (p++; *p != TEXT('\0') && *p != TEXT('\\'); p++)
			;
		if (*p == TEXT('\\')) {
			p++;
		}
	} else {
		return IDS_ERR_PATH_INVALID;
	}
	// ドライブや共有の直下にはインストールしない
	if (*p == TEXT('\0')) {
		return IDS_ERR_PATH_ROOT;
	}
	if (path_has_dotdot(path) == TRUE) {
		return IDS_ERR_PATH_INVALID;
	}
	if (lstrlen(path) + 1 + get_name_len_max() > MAX_PATH - 1) {
		return IDS_ERR_PATH_LONG;
	}
	return 0;
}

/*
 * dlg_proc - インストールのダイアログ
 */
static INT_PTR CALLBACK dlg_proc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	TCHAR buf[MSG_SIZE];
	TCHAR fmt[MSG_SIZE];
	TCHAR *p;
	UINT err_id;

	switch (msg) {
	case WM_INITDIALOG:
		SendMessage(hDlg, WM_SETICON, ICON_BIG,
			(LPARAM)LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON_MAIN)));
		SendMessage(hDlg, WM_SETICON, ICON_SMALL,
			(LPARAM)LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON_MAIN)));
		SetWindowText(hDlg, title_str);

		set_dialog_font(hDlg);

		wsprintf(buf, str_load(IDS_HEADER, fmt, MSG_SIZE), ver_str);
		SetDlgItemText(hDlg, IDC_STATIC_TITLE, buf);
		set_ctrl_text(hDlg, IDC_STATIC_DESC,
			(update_mode == TRUE) ? IDS_DESC_UPDATE : IDS_DESC_INSTALL);
		set_ctrl_text(hDlg, IDC_STATIC_FOLDER, IDS_FOLDER);
		set_ctrl_text(hDlg, IDC_BUTTON_BROWSE, IDS_BROWSE);
		set_ctrl_text(hDlg, IDC_CHECK_STARTUP, IDS_CHECK_STARTUP);
		set_ctrl_text(hDlg, IDC_CHECK_STARTMENU, IDS_CHECK_STARTMENU);
		set_ctrl_text(hDlg, IDC_CHECK_DESKTOP, IDS_CHECK_DESKTOP);
		set_ctrl_text(hDlg, IDC_CHECK_RUN, IDS_CHECK_RUN);
		set_ctrl_text(hDlg, IDOK,
			(update_mode == TRUE) ? IDS_BUTTON_UPDATE : IDS_BUTTON_INSTALL);
		set_ctrl_text(hDlg, IDCANCEL, IDS_BUTTON_CANCEL);

		SetDlgItemText(hDlg, IDC_EDIT_PATH, install_path);
		SendDlgItemMessage(hDlg, IDC_EDIT_PATH, EM_LIMITTEXT, MAX_PATH - 1, 0);
		// 更新の場合は現在の状態を引き継ぐ
		CheckDlgButton(hDlg, IDC_CHECK_STARTUP,
			(update_mode == FALSE || shortcut_exists(CSIDL_COMMON_STARTUP) == TRUE) ?
			BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_CHECK_STARTMENU,
			(update_mode == FALSE || shortcut_exists(CSIDL_COMMON_PROGRAMS) == TRUE) ?
			BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_CHECK_DESKTOP,
			(update_mode == FALSE || shortcut_exists(CSIDL_COMMON_DESKTOPDIRECTORY) == TRUE) ?
			BST_CHECKED : BST_UNCHECKED);

		ShowWindow(GetDlgItem(hDlg, IDC_CHECK_RUN), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDC_PROGRESS), SW_HIDE);
		SetDlgItemText(hDlg, IDC_STATIC_STATUS, TEXT(""));
		return TRUE;

	case WM_CLOSE:
		SendMessage(hDlg, WM_COMMAND, IDCANCEL, 0);
		return TRUE;

	case WM_DESTROY:
		if (hTitleFont != NULL) {
			DeleteObject(hTitleFont);
			hTitleFont = NULL;
		}
		if (hDlgFont != NULL) {
			DeleteObject(hDlgFont);
			hDlgFont = NULL;
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_BUTTON_BROWSE:
			browse_folder(hDlg);
			break;

		case IDOK:
			if (install_end == TRUE) {
				// 確認してCLCLを起動
				if (IsDlgButtonChecked(hDlg, IDC_CHECK_RUN) == BST_CHECKED &&
					exec_app() == FALSE) {
					msg_box(hDlg, IDS_ERR_RUN, MB_OK | MB_ICONEXCLAMATION, NULL);
				}
				EndDialog(hDlg, IDOK);
				break;
			}
			GetDlgItemText(hDlg, IDC_EDIT_PATH, install_path, MAX_PATH);
			for (p = install_path; *p != TEXT('\0'); p++) {
				if (*p == TEXT('/')) {
					*p = TEXT('\\');
				}
			}
			// 末尾の空白と '\' を取り除く (ドライブ直下を示す '\' は残す)
			for (p = install_path + lstrlen(install_path);
				p > install_path && (*(p - 1) == TEXT(' ') || *(p - 1) == TEXT('\\')); p--) {
				if (*(p - 1) == TEXT('\\') && p - 1 == install_path + 2 && *(install_path + 1) == TEXT(':')) {
					break;
				}
				*(p - 1) = TEXT('\0');
			}
			if (*install_path == TEXT('\0')) {
				msg_box(hDlg, IDS_ERR_PATH_EMPTY, MB_OK | MB_ICONEXCLAMATION, NULL);
				break;
			}
			if ((err_id = check_install_path(install_path)) != 0) {
				msg_box(hDlg, err_id, MB_OK | MB_ICONEXCLAMATION, NULL);
				break;
			}
			SetDlgItemText(hDlg, IDC_EDIT_PATH, install_path);

			enable_ctrl(hDlg, FALSE);
			ShowWindow(GetDlgItem(hDlg, IDC_PROGRESS), SW_SHOW);
			SendDlgItemMessage(hDlg, IDC_PROGRESS, PBM_SETPOS, 0, 0);
			SetCursor(LoadCursor(NULL, IDC_WAIT));
			install_end = do_install(hDlg);
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			if (install_end == FALSE) {
				enable_ctrl(hDlg, TRUE);
				ShowWindow(GetDlgItem(hDlg, IDC_PROGRESS), SW_HIDE);
				SetDlgItemText(hDlg, IDC_STATIC_STATUS, TEXT(""));
				break;
			}
			show_complete(hDlg);
			break;

		case IDCANCEL:
			if (install_end == TRUE) {
				EndDialog(hDlg, IDOK);
				break;
			}
			if (IsWindowEnabled(GetDlgItem(hDlg, IDCANCEL)) == FALSE) {
				break;
			}
			if (msg_box(hDlg, IDS_CONFIRM_CANCEL, MB_YESNO | MB_ICONQUESTION, NULL) == IDYES) {
				EndDialog(hDlg, IDCANCEL);
			}
			break;
		}
		return TRUE;
	}
	return FALSE;
}

/*
 * log_split - 一覧を行ごとに分割 (行数を返す)
 */
static int log_split(TCHAR *log, TCHAR ***lines)
{
	TCHAR *p;
	int cnt = 0;
	int i;

	*lines = NULL;
	// 行数を数える
	for (p = log; *p != TEXT('\0');) {
		for (; *p == TEXT('\r') || *p == TEXT('\n'); p++)
			;
		if (*p == TEXT('\0')) {
			break;
		}
		cnt++;
		for (; *p != TEXT('\0') && *p != TEXT('\r') && *p != TEXT('\n'); p++)
			;
	}
	if (cnt == 0 ||
		(*lines = (TCHAR **)HeapAlloc(GetProcessHeap(), 0, cnt * sizeof(TCHAR *))) == NULL) {
		return 0;
	}
	// 行の区切りを終端に置き換える
	for (p = log, i = 0; i < cnt; i++) {
		for (; *p == TEXT('\r') || *p == TEXT('\n'); p++) {
			*p = TEXT('\0');
		}
		*(*lines + i) = p;
		for (; *p != TEXT('\0') && *p != TEXT('\r') && *p != TEXT('\n'); p++)
			;
	}
	*p = TEXT('\0');
	return cnt;
}

/*
 * uninstall_file_check - アンインストールで削除してよいファイルかどうか (正規化したパスを buf に返す)
 */
static BOOL uninstall_file_check(const TCHAR *dir, const TCHAR *path, TCHAR *buf)
{
	static const int csidl_list[] = {
		CSIDL_COMMON_STARTUP, CSIDL_COMMON_PROGRAMS, CSIDL_COMMON_DESKTOPDIRECTORY };
	TCHAR folder[MAX_PATH];
	TCHAR link_dir[MAX_PATH];
	const TCHAR *name;
	int len;
	int i;

	if (path_normalize(path, buf) == FALSE) {
		return FALSE;
	}
	// インストール先の配下
	if (path_is_under(dir, buf) == TRUE) {
		return TRUE;
	}
	// ショートカットの配置先の直下にある CLCL*.lnk
	for (name = buf + lstrlen(buf); name > buf && *(name - 1) != TEXT('\\'); name--)
		;
	len = lstrlen(APP_NAME);
	if (name == buf ||
		CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, name, len, APP_NAME, len) != CSTR_EQUAL ||
		(len = lstrlen(name)) < 4 || lstrcmpi(name + len - 4, TEXT(".lnk")) != 0) {
		return FALSE;
	}
	for (i = 0; i < (int)(sizeof(csidl_list) / sizeof(csidl_list[0])); i++) {
		if (FAILED(SHGetFolderPath(NULL, csidl_list[i], NULL, SHGFP_TYPE_CURRENT, folder)) ||
			path_normalize(folder, link_dir) == FALSE) {
			continue;
		}
		if (path_is_under(link_dir, buf) == TRUE && name == buf + lstrlen(link_dir) + 1) {
			return TRUE;
		}
	}
	return FALSE;
}

/*
 * do_uninstall - アンインストールの実行
 */
static void do_uninstall(const TCHAR *dir_path, const DWORD pid, const BOOL delete_data)
{
	HANDLE hProcess;
	TCHAR *log;
	TCHAR **lines;
	TCHAR dir[MAX_PATH];
	TCHAR path[MAX_PATH];
	int cnt;
	int i;

	// 呼び出し元の終了を待つ
	if (pid != 0 && (hProcess = OpenProcess(SYNCHRONIZE, FALSE, pid)) != NULL) {
		WaitForSingleObject(hProcess, 30000);
		CloseHandle(hProcess);
	}
	// インストール先として扱えないパスの場合はファイルを削除しない
	if (path_normalize(dir_path, dir) == TRUE) {
		// インストールしたファイルの削除 (インストール先の配下とショートカットのみ)
		if ((log = log_load(dir)) != NULL) {
			cnt = log_split(log, &lines);
			for (i = 0; i < cnt; i++) {
				if (**(lines + i) == TEXT('F') &&
					uninstall_file_check(dir, *(lines + i) + 1, path) == TRUE) {
					SetFileAttributes(path, FILE_ATTRIBUTE_NORMAL);
					DeleteFile(path);
				}
			}
			// 作成したフォルダは作成時と逆の順に削除 (空の場合のみ削除される)
			for (i = cnt - 1; i >= 0; i--) {
				if (**(lines + i) == TEXT('D') &&
					path_normalize(*(lines + i) + 1, path) == TRUE && path_is_under(dir, path) == TRUE) {
					RemoveDirectory(path);
				}
			}
			if (lines != NULL) {
				HeapFree(GetProcessHeap(), 0, lines);
			}
			HeapFree(GetProcessHeap(), 0, log);
		}
		// アンインストーラの削除
		if (build_path(path, MAX_PATH, dir, UNINSTALL_LOG) == TRUE) {
			SetFileAttributes(path, FILE_ATTRIBUTE_NORMAL);
			DeleteFile(path);
		}
		if (build_path(path, MAX_PATH, dir, UNINSTALL_EXE) == TRUE) {
			SetFileAttributes(path, FILE_ATTRIBUTE_NORMAL);
			DeleteFile(path);
		}
		RemoveDirectory(dir);
	}

	// アプリの一覧から削除
	reg_target_cnt = reg_find_install(reg_target, REG_TARGET_MAX, NULL);
	for (i = 0; i < reg_target_cnt; i++) {
		reg_delete_install(reg_target + i);
	}
	// 設定データの削除
	if (delete_data == TRUE &&
		SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path)) &&
		lstrlen(path) + lstrlen(APP_DATA_FOLDER) + 2 <= MAX_PATH) {
		lstrcat(path, TEXT("\\"));
		lstrcat(path, APP_DATA_FOLDER);
		delete_dir_tree(path);
	}
	msg_box(NULL, IDS_UNINSTALL_COMPLETE, MB_OK | MB_ICONINFORMATION, NULL);

	// 一時フォルダにコピーした自分自身を削除
	GetModuleFileName(NULL, path, MAX_PATH);
	MoveFileEx(path, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
}

/*
 * get_uninstall_dir - アンインストールするフォルダの取得
 */
static BOOL get_uninstall_dir(TCHAR *dir_path)
{
	TCHAR path[MAX_PATH];

	// 実行ファイルと同じ場所にファイルの一覧があるか
	get_module_dir(dir_path);
	if (build_path(path, MAX_PATH, dir_path, UNINSTALL_LOG) == TRUE &&
		GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES) {
		return TRUE;
	}
	// 登録されているインストール先を使用する
	reg_target_cnt = reg_find_install(reg_target, REG_TARGET_MAX, path);
	if (reg_target_cnt > 0 && *path != TEXT('\0') &&
		GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES) {
		lstrcpyn(dir_path, path, MAX_PATH);
		return TRUE;
	}
	return FALSE;
}

/*
 * start_uninstall - アンインストールの開始
 */
static BOOL start_uninstall(void)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	TCHAR dir_path[MAX_PATH];
	TCHAR self[MAX_PATH];
	TCHAR temp_dir[MAX_PATH];
	TCHAR temp_path[MAX_PATH];
	TCHAR cmd[MAX_PATH * 3];
	DWORD len;
	BOOL delete_data = FALSE;

	if (get_uninstall_dir(dir_path) == FALSE) {
		msg_box(NULL, IDS_ERR_NOT_INSTALLED, MB_OK | MB_ICONEXCLAMATION, NULL);
		return FALSE;
	}
	if (msg_box(NULL, IDS_CONFIRM_UNINSTALL, MB_YESNO | MB_ICONQUESTION, NULL) != IDYES) {
		return FALSE;
	}
	// CLCLが起動中なら終了を促す
	if (check_running(NULL) == FALSE) {
		return FALSE;
	}
	if (msg_box(NULL, IDS_CONFIRM_DELETE_DATA, MB_YESNO | MB_ICONQUESTION, NULL) == IDYES) {
		delete_data = TRUE;
	}
	// 実行中のファイルを削除できるように一時フォルダから実行する
	// 管理者で実行するため、ユーザーの一時フォルダではなく一般ユーザーが他のファイルを
	// 変更できない Windows の一時フォルダにコピーする
	GetModuleFileName(NULL, self, MAX_PATH);
	len = GetWindowsDirectory(temp_dir, MAX_PATH);
	if (len == 0 || len + 6 >= MAX_PATH) {
		return FALSE;
	}
	if (*(temp_dir + len - 1) != TEXT('\\')) {
		lstrcat(temp_dir, TEXT("\\"));
	}
	lstrcat(temp_dir, TEXT("Temp\\"));
	len = lstrlen(temp_dir);
	if (GetFileAttributes(temp_dir) == INVALID_FILE_ATTRIBUTES ||
		wnsprintf(temp_path, MAX_PATH, TEXT("%sclcl_uninst_%08x_%08x.exe"),
			temp_dir, GetCurrentProcessId(), GetTickCount()) < 0 ||
		CopyFile(self, temp_path, TRUE) == FALSE) {
		return FALSE;
	}
	wsprintf(cmd, TEXT("\"%s\" %s \"%s\" %s%u"),
		temp_path, CMD_UNINSTALL_RUN, dir_path, CMD_PID, GetCurrentProcessId());
	if (delete_data == TRUE) {
		lstrcat(cmd, TEXT(" "));
		lstrcat(cmd, CMD_DELETE_DATA);
	}
	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	if (CreateProcess(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi) == FALSE) {
		DeleteFile(temp_path);
		return FALSE;
	}
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return TRUE;
}

/*
 * WinMain - メイン
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	INITCOMMONCONTROLSEX ic;
	SET_DEFAULT_DLL_DIRECTORIES set_default_dll_directories;
	LPWSTR *argv;
	TCHAR dir_path[MAX_PATH];
	TCHAR buf[MAX_PATH];
	DWORD pid = 0;
	int argc = 0;
	int i;
	int mode = 0;
	BOOL delete_data = FALSE;

	// 管理者で実行するため、インストーラと同じフォルダ (ダウンロードフォルダなど) や
	// カレントディレクトリにある DLL を読み込まないようにシステムフォルダに限定する
	SetDllDirectory(TEXT(""));
	set_default_dll_directories = (SET_DEFAULT_DLL_DIRECTORIES)GetProcAddress(
		GetModuleHandle(TEXT("kernel32.dll")), "SetDefaultDllDirectories");
	if (set_default_dll_directories != NULL) {
		set_default_dll_directories(LOAD_LIBRARY_SEARCH_SYSTEM32);
	}

	hInst = hInstance;
	set_language();
	wsprintf(ver_str, TEXT("%d.%d.%d"), INST_VER_MAJOR, INST_VER_MINOR, INST_VER_REVISION);

	*dir_path = TEXT('\0');
	if ((argv = CommandLineToArgvW(GetCommandLineW(), &argc)) != NULL) {
		for (i = 1; i < argc; i++) {
			if (lstrcmpi(*(argv + i), CMD_UNINSTALL) == 0) {
				mode = 1;
			} else if (lstrcmpi(*(argv + i), CMD_UNINSTALL_RUN) == 0) {
				mode = 2;
				if (i + 1 < argc) {
					lstrcpyn(dir_path, *(argv + (++i)), MAX_PATH);
				}
			} else if (lstrcmpi(*(argv + i), CMD_DELETE_DATA) == 0) {
				delete_data = TRUE;
			} else if (CompareString(LOCALE_INVARIANT, NORM_IGNORECASE,
				*(argv + i), lstrlen(CMD_PID), CMD_PID, lstrlen(CMD_PID)) == CSTR_EQUAL) {
				pid = str_to_dword(*(argv + i) + lstrlen(CMD_PID));
			}
		}
		LocalFree(argv);
	}

	str_load((mode == 0) ? IDS_SETUP_TITLE : IDS_UNINSTALL_TITLE, title_str, BUF_SIZE);
	CoInitialize(NULL);
	ic.dwSize = sizeof(INITCOMMONCONTROLSEX);
	ic.dwICC = ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS;
	InitCommonControlsEx(&ic);

	switch (mode) {
	case 1:
		start_uninstall();
		break;

	case 2:
		if (*dir_path != TEXT('\0')) {
			do_uninstall(dir_path, pid, delete_data);
		}
		break;

	default:
		// インストールするファイルの取得
		if (load_payload() == FALSE) {
			msg_box(NULL, IDS_ERR_PAYLOAD, MB_OK | MB_ICONEXCLAMATION, NULL);
			break;
		}
		// 既にインストールされている場合は更新
		reg_target_cnt = reg_find_install(reg_target, REG_TARGET_MAX, buf);
		if (reg_target_cnt > 0) {
			update_mode = TRUE;
		}
		if (*buf != TEXT('\0')) {
			lstrcpyn(install_path, buf, MAX_PATH);
		} else {
			if (FAILED(SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL,
				SHGFP_TYPE_CURRENT, install_path))) {
				lstrcpy(install_path, TEXT("C:\\Program Files"));
			}
			lstrcat(install_path, TEXT("\\"));
			lstrcat(install_path, DEFAULT_FOLDER);
		}
		DialogBox(hInst, MAKEINTRESOURCE(IDD_MAIN), NULL, dlg_proc);
		sb_free(&log_buf);
		break;
	}
	CoUninitialize();
	return 0;
}
/* End of source */
