/*
 * CLCL
 *
 * DarkMode.c
 *
 * Copyright (C) 1996-2026 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE
#include <commctrl.h>
#include <tchar.h>

#include "DarkMode.h"
#include "dpi.h"

/* Define */
#define DARK_BUF_SIZE					256

// ダークモードに対応する最小のビルド番号 (Windows 10 1809)
#define DARK_MODE_MIN_BUILD				17763
// DWMWA_USE_IMMERSIVE_DARK_MODE が現在の値になった最小のビルド番号
#define DARK_MODE_DWMATTR_BUILD			18985

// uxtheme.dll の未公開APIの序数
#define ORD_REFRESHIMMERSIVECOLORPOLICYSTATE	104
#define ORD_ALLOWDARKMODEFORWINDOW				133
// 序数135は Windows 10 1809 では AllowDarkModeForApp(BOOL)、
// 1903以降は SetPreferredAppMode(int) で引数の意味は同じ
#define ORD_SETPREFERREDAPPMODE					135
#define ORD_FLUSHMENUTHEMES						136

// SetPreferredAppMode に渡す値
#define APPMODE_ALLOW_DARK				1
#define APPMODE_DEFAULT					0

// DwmSetWindowAttribute のダークモード属性
#define DWMWA_DARK_MODE_OLD				19
#define DWMWA_DARK_MODE					20

// ラジオボタンのグリフの基準サイズ
#define RADIO_GLYPH_SIZE				12
// ラジオボタンの文字の基準オフセット
#define RADIO_TEXT_MARGIN				17

// ダークモードの配色
#define DARK_COLOR_WINDOW				RGB(32, 32, 32)
#define DARK_COLOR_FACE					RGB(43, 43, 43)
#define DARK_COLOR_TEXT					RGB(240, 240, 240)
#define DARK_COLOR_GRAYTEXT				RGB(140, 140, 140)
#define DARK_COLOR_HIGHLIGHT			RGB(0, 120, 215)
#define DARK_COLOR_HIGHLIGHTTEXT		RGB(255, 255, 255)
#define DARK_COLOR_HOTLIGHT				RGB(102, 178, 255)
#define DARK_COLOR_SHADOW				RGB(64, 64, 64)
#define DARK_COLOR_LIGHT				RGB(85, 85, 85)
#define DARK_COLOR_DKSHADOW				RGB(16, 16, 16)

// ブラシのキャッシュ数 (COLOR_ 定数の最大値 + 1)
#define BRUSH_CACHE_CNT					32

// サブクラスID
#define SUBCLASS_ID_DIALOG				1
#define SUBCLASS_ID_TAB					2
#define SUBCLASS_ID_GROUPBOX			3
#define SUBCLASS_ID_STATUSBAR			4
#define SUBCLASS_ID_HEADER				5
#define SUBCLASS_ID_NOTIFY				6
#define SUBCLASS_ID_RADIO				7

// 多重適用の防止用プロパティ
#define DARK_MODE_PROP					TEXT("CLCLDarkMode")

// 標準コントロールのクラス名
#define CLASS_EDIT						TEXT("Edit")
#define CLASS_COMBOBOX					TEXT("ComboBox")
#define CLASS_LISTBOX					TEXT("ListBox")
#define CLASS_BUTTON					TEXT("Button")

#ifndef ODS_HOTLIGHT
#define ODS_HOTLIGHT					0x0040
#endif
#ifndef ODS_NOACCEL
#define ODS_NOACCEL						0x0100
#endif

/* Struct */
typedef void (WINAPI *REFRESHIMMERSIVECOLORPOLICYSTATE_PROC)(void);
typedef BOOL (WINAPI *ALLOWDARKMODEFORWINDOW_PROC)(HWND, BOOL);
typedef int (WINAPI *SETPREFERREDAPPMODE_PROC)(int);
typedef void (WINAPI *FLUSHMENUTHEMES_PROC)(void);
typedef HRESULT (WINAPI *SETWINDOWTHEME_PROC)(HWND, LPCWSTR, LPCWSTR);
typedef HRESULT (WINAPI *DWMSETWINDOWATTRIBUTE_PROC)(HWND, DWORD, LPCVOID, DWORD);
typedef LONG (WINAPI *RTLGETVERSION_PROC)(OSVERSIONINFOW *);

// メニューバーの描画に使用する非公開の構造体
typedef union tagUAHMENUITEMMETRICS {
	struct {
		DWORD cx;
		DWORD cy;
	} rgsizeBar[2];
	struct {
		DWORD cx;
		DWORD cy;
	} rgsizePopup[4];
} UAHMENUITEMMETRICS;

typedef struct tagUAHMENUPOPUPMETRICS {
	DWORD rgcx[4];
	DWORD fUpdateMaxWidths : 2;
} UAHMENUPOPUPMETRICS;

typedef struct tagUAHMENU {
	HMENU hmenu;
	HDC hdc;
	DWORD dwFlags;
} UAHMENU;

typedef struct tagUAHMENUITEM {
	int iPosition;
	UAHMENUITEMMETRICS umim;
	UAHMENUPOPUPMETRICS umpm;
} UAHMENUITEM;

typedef struct tagUAHDRAWMENUITEM {
	DRAWITEMSTRUCT dis;
	UAHMENU um;
	UAHMENUITEM umi;
} UAHDRAWMENUITEM;

/* Global Variables */
static HMODULE dark_mode_uxtheme_lib;
static HMODULE dark_mode_dwmapi_lib;

static REFRESHIMMERSIVECOLORPOLICYSTATE_PROC _RefreshImmersiveColorPolicyState;
static ALLOWDARKMODEFORWINDOW_PROC _AllowDarkModeForWindow;
static SETPREFERREDAPPMODE_PROC _SetPreferredAppMode;
static FLUSHMENUTHEMES_PROC _FlushMenuThemes;
static SETWINDOWTHEME_PROC _SetWindowTheme;
static DWMSETWINDOWATTRIBUTE_PROC _DwmSetWindowAttribute;

// OSがダークモードに対応しているか
static BOOL dark_mode_support;
// 現在ダークモードで表示しているか
static BOOL dark_mode_dark;
// 初期化済みか
static BOOL dark_mode_initialized;
// OSのビルド番号
static DWORD dark_mode_build_number;

static HBRUSH dark_mode_brush[BRUSH_CACHE_CNT];

/* Local Function Prototypes */
static DWORD dark_mode_get_build_number(void);
static BOOL dark_mode_is_high_contrast(void);
static BOOL dark_mode_get_os_setting(void);
static void dark_mode_set_app_mode(void);
static void dark_mode_free_brush(void);
static void dark_mode_allow_window(const HWND hWnd);
static void dark_mode_set_theme(const HWND hWnd, const WCHAR *theme);
static void dark_mode_set_titlebar(const HWND hWnd);
static BOOL CALLBACK dark_mode_enum_child_proc(HWND hWnd, LPARAM lParam);
static void dark_mode_set_groupbox(const HWND hWnd);
static void dark_mode_set_radio(const HWND hWnd);
static void dark_mode_draw_radio(const HWND hWnd);
static void dark_mode_set_header(const HWND hHeader);
static void dark_mode_set_tooltip(const HWND hToolTip);
static void dark_mode_set_notify(const HWND hWnd);
static void dark_mode_draw_header(const HWND hWnd);
static void dark_mode_draw_groupbox(const HWND hWnd);
static void dark_mode_draw_tab(const HWND hWnd);
static void dark_mode_draw_statusbar(const HWND hWnd);
static LRESULT CALLBACK dark_mode_dialog_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
static LRESULT CALLBACK dark_mode_tab_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
static LRESULT CALLBACK dark_mode_groupbox_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
static LRESULT CALLBACK dark_mode_radio_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
static LRESULT CALLBACK dark_mode_statusbar_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
static LRESULT CALLBACK dark_mode_header_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
static LRESULT CALLBACK dark_mode_notify_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
static BOOL dark_mode_draw_menubar(const HWND hWnd, const UAHMENU *pum);
static BOOL dark_mode_draw_menubar_item(const UAHDRAWMENUITEM *pumdi);
static void dark_mode_draw_menubar_border(const HWND hWnd);

/*
 * dark_mode_get_build_number - OSのビルド番号の取得
 */
static DWORD dark_mode_get_build_number(void)
{
	RTLGETVERSION_PROC _RtlGetVersion;
	HMODULE hModule;
	OSVERSIONINFOW osvi;

	if ((hModule = GetModuleHandle(TEXT("ntdll.dll"))) == NULL) {
		return 0;
	}
	if ((_RtlGetVersion = (RTLGETVERSION_PROC)GetProcAddress(hModule, "RtlGetVersion")) == NULL) {
		return 0;
	}
	ZeroMemory(&osvi, sizeof(osvi));
	osvi.dwOSVersionInfoSize = sizeof(osvi);
	if (_RtlGetVersion(&osvi) != 0) {
		return 0;
	}
	if (osvi.dwMajorVersion < 10) {
		return 0;
	}
	return osvi.dwBuildNumber;
}

/*
 * dark_mode_is_high_contrast - ハイコントラストが有効か
 */
static BOOL dark_mode_is_high_contrast(void)
{
	HIGHCONTRAST hc;

	ZeroMemory(&hc, sizeof(hc));
	hc.cbSize = sizeof(hc);
	if (SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, 0) == FALSE) {
		return FALSE;
	}
	return (hc.dwFlags & HCF_HIGHCONTRASTON) ? TRUE : FALSE;
}

/*
 * dark_mode_get_os_setting - OSのダークモード設定の取得
 */
static BOOL dark_mode_get_os_setting(void)
{
	HKEY hKey;
	DWORD value = 1;
	DWORD size = sizeof(DWORD);
	DWORD type = REG_DWORD;
	BOOL ret = FALSE;

	if (dark_mode_support == FALSE || dark_mode_is_high_contrast() == TRUE) {
		return FALSE;
	}
	if (RegOpenKeyEx(HKEY_CURRENT_USER,
		TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
		0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS) {
		return FALSE;
	}
	if (RegQueryValueEx(hKey, TEXT("AppsUseLightTheme"), NULL, &type,
		(LPBYTE)&value, &size) == ERROR_SUCCESS && type == REG_DWORD) {
		ret = (value == 0) ? TRUE : FALSE;
	}
	RegCloseKey(hKey);
	return ret;
}

/*
 * dark_mode_set_app_mode - プロセスのダークモードの設定
 */
static void dark_mode_set_app_mode(void)
{
	if (_SetPreferredAppMode != NULL) {
		_SetPreferredAppMode((dark_mode_dark == TRUE) ? APPMODE_ALLOW_DARK : APPMODE_DEFAULT);
	}
	if (_RefreshImmersiveColorPolicyState != NULL) {
		_RefreshImmersiveColorPolicyState();
	}
	if (_FlushMenuThemes != NULL) {
		_FlushMenuThemes();
	}
}

/*
 * dark_mode_init - ダークモードの初期化
 */
void dark_mode_init(void)
{
	if (dark_mode_initialized == TRUE) {
		return;
	}
	dark_mode_initialized = TRUE;

	dark_mode_build_number = dark_mode_get_build_number();
	if (dark_mode_build_number < DARK_MODE_MIN_BUILD) {
		return;
	}
	if ((dark_mode_uxtheme_lib = LoadLibrary(TEXT("uxtheme.dll"))) == NULL) {
		return;
	}
	_RefreshImmersiveColorPolicyState = (REFRESHIMMERSIVECOLORPOLICYSTATE_PROC)
		GetProcAddress(dark_mode_uxtheme_lib, MAKEINTRESOURCEA(ORD_REFRESHIMMERSIVECOLORPOLICYSTATE));
	_AllowDarkModeForWindow = (ALLOWDARKMODEFORWINDOW_PROC)
		GetProcAddress(dark_mode_uxtheme_lib, MAKEINTRESOURCEA(ORD_ALLOWDARKMODEFORWINDOW));
	_SetPreferredAppMode = (SETPREFERREDAPPMODE_PROC)
		GetProcAddress(dark_mode_uxtheme_lib, MAKEINTRESOURCEA(ORD_SETPREFERREDAPPMODE));
	_FlushMenuThemes = (FLUSHMENUTHEMES_PROC)
		GetProcAddress(dark_mode_uxtheme_lib, MAKEINTRESOURCEA(ORD_FLUSHMENUTHEMES));
	_SetWindowTheme = (SETWINDOWTHEME_PROC)GetProcAddress(dark_mode_uxtheme_lib, "SetWindowTheme");

	if (_AllowDarkModeForWindow == NULL || _SetPreferredAppMode == NULL) {
		FreeLibrary(dark_mode_uxtheme_lib);
		dark_mode_uxtheme_lib = NULL;
		return;
	}
	if ((dark_mode_dwmapi_lib = LoadLibrary(TEXT("dwmapi.dll"))) != NULL) {
		_DwmSetWindowAttribute = (DWMSETWINDOWATTRIBUTE_PROC)
			GetProcAddress(dark_mode_dwmapi_lib, "DwmSetWindowAttribute");
	}

	dark_mode_support = TRUE;
	dark_mode_dark = dark_mode_get_os_setting();
	dark_mode_set_app_mode();
}

/*
 * dark_mode_free - ダークモードの解放
 */
void dark_mode_free(void)
{
	dark_mode_free_brush();
	if (dark_mode_dwmapi_lib != NULL) {
		FreeLibrary(dark_mode_dwmapi_lib);
		dark_mode_dwmapi_lib = NULL;
		_DwmSetWindowAttribute = NULL;
	}
	if (dark_mode_uxtheme_lib != NULL) {
		FreeLibrary(dark_mode_uxtheme_lib);
		dark_mode_uxtheme_lib = NULL;
		_RefreshImmersiveColorPolicyState = NULL;
		_AllowDarkModeForWindow = NULL;
		_SetPreferredAppMode = NULL;
		_FlushMenuThemes = NULL;
		_SetWindowTheme = NULL;
	}
	dark_mode_support = FALSE;
	dark_mode_dark = FALSE;
	dark_mode_initialized = FALSE;
	dark_mode_build_number = 0;
}

/*
 * dark_mode_is_dark - ダークモードで表示しているかの取得
 */
BOOL dark_mode_is_dark(void)
{
	return dark_mode_dark;
}

/*
 * dark_mode_update - OSの設定を読み直す、変更があればTRUEを返す
 */
BOOL dark_mode_update(void)
{
	BOOL dark;

	if (dark_mode_support == FALSE) {
		return FALSE;
	}
	dark = dark_mode_get_os_setting();
	if (dark == dark_mode_dark) {
		return FALSE;
	}
	dark_mode_dark = dark;
	dark_mode_free_brush();
	dark_mode_set_app_mode();
	return TRUE;
}

/*
 * dark_mode_is_color_change - 配色の変更通知かの判定
 */
BOOL dark_mode_is_color_change(const UINT msg, const LPARAM lParam)
{
	if (msg == WM_THEMECHANGED || msg == WM_SYSCOLORCHANGE) {
		return TRUE;
	}
	if (msg == WM_SETTINGCHANGE && lParam != 0 &&
		lstrcmpi((const TCHAR *)lParam, TEXT("ImmersiveColorSet")) == 0) {
		return TRUE;
	}
	return FALSE;
}

/*
 * dark_mode_get_color - システムカラーの取得
 */
COLORREF dark_mode_get_color(const int index)
{
	if (dark_mode_dark == FALSE) {
		return GetSysColor(index);
	}
	switch (index) {
	case COLOR_WINDOW:
		return DARK_COLOR_WINDOW;

	case COLOR_WINDOWTEXT:
	case COLOR_BTNTEXT:
	case COLOR_MENUTEXT:
	case COLOR_INFOTEXT:
	case COLOR_CAPTIONTEXT:
	case COLOR_INACTIVECAPTIONTEXT:
		return DARK_COLOR_TEXT;

	case COLOR_BTNFACE:
	case COLOR_MENU:
	case COLOR_MENUBAR:
	case COLOR_INFOBK:
	case COLOR_SCROLLBAR:
	case COLOR_APPWORKSPACE:
	case COLOR_ACTIVECAPTION:
	case COLOR_INACTIVECAPTION:
	case COLOR_BACKGROUND:
		return DARK_COLOR_FACE;

	case COLOR_HIGHLIGHT:
	case COLOR_MENUHILIGHT:
		return DARK_COLOR_HIGHLIGHT;

	case COLOR_HIGHLIGHTTEXT:
		return DARK_COLOR_HIGHLIGHTTEXT;

	case COLOR_GRAYTEXT:
		return DARK_COLOR_GRAYTEXT;

	case COLOR_HOTLIGHT:
		return DARK_COLOR_HOTLIGHT;

	case COLOR_BTNSHADOW:
		return DARK_COLOR_SHADOW;

	case COLOR_BTNHIGHLIGHT:
	case COLOR_3DLIGHT:
	case COLOR_WINDOWFRAME:
	case COLOR_ACTIVEBORDER:
	case COLOR_INACTIVEBORDER:
		return DARK_COLOR_LIGHT;

	case COLOR_3DDKSHADOW:
		return DARK_COLOR_DKSHADOW;
	}
	return GetSysColor(index);
}

/*
 * dark_mode_get_accent_color - 強調表示の文字色の取得
 */
COLORREF dark_mode_get_accent_color(void)
{
	// 選択色をそのまま文字色に使うと暗い背景では読みにくいため明るい青にする
	return dark_mode_get_color((dark_mode_dark == TRUE) ? COLOR_HOTLIGHT : COLOR_HIGHLIGHT);
}

/*
 * dark_mode_get_brush - システムカラーのブラシの取得
 */
HBRUSH dark_mode_get_brush(const int index)
{
	if (dark_mode_dark == FALSE) {
		return GetSysColorBrush(index);
	}
	if (index < 0 || index >= BRUSH_CACHE_CNT) {
		return GetSysColorBrush(index);
	}
	if (dark_mode_brush[index] == NULL) {
		dark_mode_brush[index] = CreateSolidBrush(dark_mode_get_color(index));
	}
	return dark_mode_brush[index];
}

/*
 * dark_mode_free_brush - ブラシの解放
 */
static void dark_mode_free_brush(void)
{
	int i;

	for (i = 0; i < BRUSH_CACHE_CNT; i++) {
		if (dark_mode_brush[i] != NULL) {
			DeleteObject(dark_mode_brush[i]);
			dark_mode_brush[i] = NULL;
		}
	}
}

/*
 * dark_mode_allow_window - ウィンドウのダークモードを許可する
 */
static void dark_mode_allow_window(const HWND hWnd)
{
	if (_AllowDarkModeForWindow == NULL || hWnd == NULL) {
		return;
	}
	_AllowDarkModeForWindow(hWnd, dark_mode_dark);
}

/*
 * dark_mode_set_theme - ウィンドウのビジュアルスタイルの設定
 */
static void dark_mode_set_theme(const HWND hWnd, const WCHAR *theme)
{
	if (_SetWindowTheme == NULL || hWnd == NULL) {
		return;
	}
	_SetWindowTheme(hWnd, (dark_mode_dark == TRUE) ? theme : NULL, NULL);
}

/*
 * dark_mode_set_titlebar - タイトルバーの配色の設定
 */
static void dark_mode_set_titlebar(const HWND hWnd)
{
	BOOL dark = dark_mode_dark;
	DWORD attr;

	if (_DwmSetWindowAttribute == NULL || hWnd == NULL) {
		return;
	}
	attr = (dark_mode_build_number >= DARK_MODE_DWMATTR_BUILD) ?
		DWMWA_DARK_MODE : DWMWA_DARK_MODE_OLD;
	if (_DwmSetWindowAttribute(hWnd, attr, &dark, sizeof(dark)) != S_OK) {
		_DwmSetWindowAttribute(hWnd, DWMWA_DARK_MODE_OLD, &dark, sizeof(dark));
	}
}

/*
 * dark_mode_set_control - コントロールに配色を設定する
 */
void dark_mode_set_control(const HWND hWnd)
{
	TCHAR class_name[DARK_BUF_SIZE];

	if (dark_mode_support == FALSE || hWnd == NULL) {
		return;
	}
	dark_mode_allow_window(hWnd);

	*class_name = TEXT('\0');
	GetClassName(hWnd, class_name, DARK_BUF_SIZE - 1);

	if (lstrcmpi(class_name, WC_LISTVIEW) == 0) {
		dark_mode_set_theme(hWnd, L"DarkMode_Explorer");
		ListView_SetBkColor(hWnd, dark_mode_get_color(COLOR_WINDOW));
		ListView_SetTextBkColor(hWnd, dark_mode_get_color(COLOR_WINDOW));
		ListView_SetTextColor(hWnd, dark_mode_get_color(COLOR_WINDOWTEXT));
		dark_mode_set_header(ListView_GetHeader(hWnd));
		dark_mode_set_tooltip(ListView_GetToolTips(hWnd));
		dark_mode_set_notify(hWnd);
	} else if (lstrcmpi(class_name, WC_TREEVIEW) == 0) {
		dark_mode_set_theme(hWnd, L"DarkMode_Explorer");
		TreeView_SetBkColor(hWnd, dark_mode_get_color(COLOR_WINDOW));
		TreeView_SetTextColor(hWnd, dark_mode_get_color(COLOR_WINDOWTEXT));
		dark_mode_set_tooltip(TreeView_GetToolTips(hWnd));
		dark_mode_set_notify(hWnd);
	} else if (lstrcmpi(class_name, WC_HEADER) == 0) {
		dark_mode_set_header(hWnd);
	} else if (lstrcmpi(class_name, TOOLTIPS_CLASS) == 0) {
		dark_mode_set_tooltip(hWnd);
	} else if (lstrcmpi(class_name, CLASS_EDIT) == 0 || lstrcmpi(class_name, CLASS_COMBOBOX) == 0 ||
		lstrcmpi(class_name, WC_COMBOBOXEX) == 0 || lstrcmpi(class_name, CLASS_LISTBOX) == 0) {
		dark_mode_set_theme(hWnd, L"DarkMode_CFD");
	} else if (lstrcmpi(class_name, CLASS_BUTTON) == 0) {
		switch (GetWindowLong(hWnd, GWL_STYLE) & BS_TYPEMASK) {
		case BS_RADIOBUTTON:
		case BS_AUTORADIOBUTTON:
			// ダークのテーマにはラジオボタンの配色がなく文字が黒くなるため独自に描画する
			dark_mode_set_radio(hWnd);
			break;

		case BS_GROUPBOX:
			dark_mode_set_groupbox(hWnd);
			break;

		default:
			dark_mode_set_theme(hWnd, L"DarkMode_Explorer");
			break;
		}
	} else if (lstrcmpi(class_name, WC_TABCONTROL) == 0) {
		dark_mode_set_tab(hWnd);
	} else if (lstrcmpi(class_name, STATUSCLASSNAME) == 0) {
		dark_mode_set_statusbar(hWnd);
		dark_mode_set_notify(hWnd);
	} else if (lstrcmpi(class_name, TOOLBARCLASSNAME) == 0) {
		// 独自の配色を使うためビジュアルスタイルを無効にする
		if (_SetWindowTheme != NULL) {
			_SetWindowTheme(hWnd, (dark_mode_dark == TRUE) ? L"" : NULL, (dark_mode_dark == TRUE) ? L"" : NULL);
		}
		dark_mode_set_tooltip((HWND)SendMessage(hWnd, TB_GETTOOLTIPS, 0, 0));
		dark_mode_set_notify(hWnd);
	} else {
		dark_mode_set_theme(hWnd, L"DarkMode_Explorer");
	}
	InvalidateRect(hWnd, NULL, TRUE);
}

/*
 * dark_mode_enum_child_proc - 子ウィンドウの列挙
 */
static BOOL CALLBACK dark_mode_enum_child_proc(HWND hWnd, LPARAM lParam)
{
	dark_mode_set_control(hWnd);
	return TRUE;
}

/*
 * dark_mode_set_children - 子ウィンドウに配色を設定する
 */
void dark_mode_set_children(const HWND hWnd)
{
	if (dark_mode_support == FALSE || hWnd == NULL) {
		return;
	}
	EnumChildWindows(hWnd, dark_mode_enum_child_proc, 0);
}

/*
 * dark_mode_set_window - ウィンドウに配色を設定する
 */
void dark_mode_set_window(const HWND hWnd)
{
	if (dark_mode_support == FALSE || hWnd == NULL) {
		return;
	}
	dark_mode_allow_window(hWnd);
	dark_mode_set_titlebar(hWnd);
	dark_mode_set_children(hWnd);
}

/*
 * dark_mode_set_dialog - ダイアログに配色を設定する
 */
void dark_mode_set_dialog(const HWND hDlg)
{
	if (dark_mode_support == FALSE || hDlg == NULL) {
		return;
	}
	if (GetProp(hDlg, DARK_MODE_PROP) == NULL) {
		SetProp(hDlg, DARK_MODE_PROP, (HANDLE)1);
		SetWindowSubclass(hDlg, dark_mode_dialog_proc, SUBCLASS_ID_DIALOG, 0);
	}
	dark_mode_allow_window(hDlg);
	dark_mode_set_titlebar(hDlg);
	dark_mode_set_children(hDlg);
	RedrawWindow(hDlg, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);
}

/*
 * dark_mode_set_tab - タブコントロールに配色を設定する
 */
void dark_mode_set_tab(const HWND hTab)
{
	if (dark_mode_support == FALSE || hTab == NULL) {
		return;
	}
	dark_mode_allow_window(hTab);
	if (GetProp(hTab, DARK_MODE_PROP) == NULL) {
		SetProp(hTab, DARK_MODE_PROP, (HANDLE)1);
		SetWindowSubclass(hTab, dark_mode_tab_proc, SUBCLASS_ID_TAB, 0);
	}
	InvalidateRect(hTab, NULL, TRUE);
}

/*
 * dark_mode_set_statusbar - ステータスバーに配色を設定する
 */
void dark_mode_set_statusbar(const HWND hStatusBar)
{
	if (dark_mode_support == FALSE || hStatusBar == NULL) {
		return;
	}
	dark_mode_allow_window(hStatusBar);
	if (GetProp(hStatusBar, DARK_MODE_PROP) == NULL) {
		SetProp(hStatusBar, DARK_MODE_PROP, (HANDLE)1);
		SetWindowSubclass(hStatusBar, dark_mode_statusbar_proc, SUBCLASS_ID_STATUSBAR, 0);
	}
	InvalidateRect(hStatusBar, NULL, TRUE);
}

/*
 * dark_mode_set_header - ヘッダに配色を設定する
 */
static void dark_mode_set_header(const HWND hHeader)
{
	if (hHeader == NULL) {
		return;
	}
	dark_mode_allow_window(hHeader);
	dark_mode_set_theme(hHeader, L"DarkMode_ItemsView");
	if (GetProp(hHeader, DARK_MODE_PROP) == NULL) {
		SetProp(hHeader, DARK_MODE_PROP, (HANDLE)1);
		SetWindowSubclass(hHeader, dark_mode_header_proc, SUBCLASS_ID_HEADER, 0);
	}
	InvalidateRect(hHeader, NULL, TRUE);
}

/*
 * dark_mode_set_tooltip - ツールチップに配色を設定する
 */
static void dark_mode_set_tooltip(const HWND hToolTip)
{
	const HANDLE mode = (HANDLE)((dark_mode_dark == TRUE) ? 1 : 2);

	// 表示の度に呼ばれるため配色が変わっていない場合は何もしない
	if (hToolTip == NULL || GetProp(hToolTip, DARK_MODE_PROP) == mode) {
		return;
	}
	SetProp(hToolTip, DARK_MODE_PROP, mode);
	dark_mode_allow_window(hToolTip);
	// 配色を指定するためビジュアルスタイルを無効にする
	if (_SetWindowTheme != NULL) {
		_SetWindowTheme(hToolTip,
			(dark_mode_dark == TRUE) ? L"" : NULL, (dark_mode_dark == TRUE) ? L"" : NULL);
	}
	SendMessage(hToolTip, TTM_SETTIPBKCOLOR, (WPARAM)dark_mode_get_color(COLOR_INFOBK), 0);
	SendMessage(hToolTip, TTM_SETTIPTEXTCOLOR, (WPARAM)dark_mode_get_color(COLOR_INFOTEXT), 0);
}

/*
 * dark_mode_set_notify - 子コントロールの通知を処理するように設定する
 */
static void dark_mode_set_notify(const HWND hWnd)
{
	if (hWnd == NULL) {
		return;
	}
	// 同じプロシージャとIDの登録は上書きされるため多重にはならない
	SetWindowSubclass(hWnd, dark_mode_notify_proc, SUBCLASS_ID_NOTIFY, 0);
}

/*
 * dark_mode_set_groupbox - グループボックスに配色を設定する
 */
static void dark_mode_set_groupbox(const HWND hWnd)
{
	if ((GetWindowLong(hWnd, GWL_STYLE) & BS_TYPEMASK) != BS_GROUPBOX) {
		return;
	}
	if (GetProp(hWnd, DARK_MODE_PROP) != NULL) {
		return;
	}
	SetProp(hWnd, DARK_MODE_PROP, (HANDLE)1);
	SetWindowSubclass(hWnd, dark_mode_groupbox_proc, SUBCLASS_ID_GROUPBOX, 0);
}

/*
 * dark_mode_set_radio - ラジオボタンに配色を設定する
 */
static void dark_mode_set_radio(const HWND hWnd)
{
	if (GetProp(hWnd, DARK_MODE_PROP) != NULL) {
		return;
	}
	SetProp(hWnd, DARK_MODE_PROP, (HANDLE)1);
	SetWindowSubclass(hWnd, dark_mode_radio_proc, SUBCLASS_ID_RADIO, 0);
}

/*
 * dark_mode_refresh_window - ウィンドウの配色を設定し直す
 */
void dark_mode_refresh_window(const HWND hWnd)
{
	if (dark_mode_support == FALSE || hWnd == NULL) {
		return;
	}
	dark_mode_allow_window(hWnd);
	dark_mode_set_titlebar(hWnd);
	dark_mode_set_children(hWnd);
	SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	RedrawWindow(hWnd, NULL, NULL,
		RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
}

/*
 * dark_mode_ctl_color - コントロールの配色の設定
 */
BOOL dark_mode_ctl_color(const UINT msg, const WPARAM wParam, const LPARAM lParam, HBRUSH *ret)
{
	const HDC hdc = (HDC)wParam;
	const HWND hCtl = (HWND)lParam;

	if (dark_mode_dark == FALSE || ret == NULL) {
		return FALSE;
	}
	switch (msg) {
	case WM_CTLCOLORDLG:
		SetTextColor(hdc, dark_mode_get_color(COLOR_BTNTEXT));
		SetBkColor(hdc, dark_mode_get_color(COLOR_BTNFACE));
		*ret = dark_mode_get_brush(COLOR_BTNFACE);
		return TRUE;

	case WM_CTLCOLORBTN:
	case WM_CTLCOLORSTATIC:
		SetTextColor(hdc, (hCtl != NULL && IsWindowEnabled(hCtl) == FALSE) ?
			dark_mode_get_color(COLOR_GRAYTEXT) : dark_mode_get_color(COLOR_BTNTEXT));
		SetBkColor(hdc, dark_mode_get_color(COLOR_BTNFACE));
		*ret = dark_mode_get_brush(COLOR_BTNFACE);
		return TRUE;

	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
		SetTextColor(hdc, dark_mode_get_color(COLOR_WINDOWTEXT));
		SetBkColor(hdc, dark_mode_get_color(COLOR_WINDOW));
		*ret = dark_mode_get_brush(COLOR_WINDOW);
		return TRUE;
	}
	return FALSE;
}

/*
 * dark_mode_dialog_proc - ダイアログのサブクラスプロシージャ
 */
static LRESULT CALLBACK dark_mode_dialog_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	HBRUSH hBrush;

	switch (msg) {
	case WM_NCDESTROY:
		RemoveProp(hWnd, DARK_MODE_PROP);
		RemoveWindowSubclass(hWnd, dark_mode_dialog_proc, uIdSubclass);
		break;

	case WM_CTLCOLORDLG:
	case WM_CTLCOLORBTN:
	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
		if (dark_mode_ctl_color(msg, wParam, lParam, &hBrush) == TRUE) {
			return (LRESULT)hBrush;
		}
		break;

	case WM_SETTINGCHANGE:
		if (dark_mode_is_color_change(msg, lParam) == TRUE) {
			dark_mode_update();
			dark_mode_refresh_window(hWnd);
		}
		break;
	}
	return DefSubclassProc(hWnd, msg, wParam, lParam);
}

/*
 * dark_mode_draw_tab - タブコントロールの描画
 */
static void dark_mode_draw_tab(const HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hdc, mdc;
	HBITMAP hBmp, hRetBmp;
	HFONT hFont, hRetFont;
	RECT client_rect, item_rect, page_rect;
	TCHAR buf[DARK_BUF_SIZE];
	TC_ITEM tci;
	int width, height;
	int sel, cnt, i;

	if ((hdc = BeginPaint(hWnd, &ps)) == NULL) {
		return;
	}
	GetClientRect(hWnd, &client_rect);
	width = client_rect.right - client_rect.left;
	height = client_rect.bottom - client_rect.top;

	if ((mdc = CreateCompatibleDC(hdc)) == NULL) {
		EndPaint(hWnd, &ps);
		return;
	}
	if ((hBmp = CreateCompatibleBitmap(hdc, width, height)) == NULL) {
		DeleteDC(mdc);
		EndPaint(hWnd, &ps);
		return;
	}
	hRetBmp = SelectObject(mdc, hBmp);

	// 背景
	FillRect(mdc, &client_rect, dark_mode_get_brush(COLOR_BTNFACE));

	// 表示領域の枠
	page_rect = client_rect;
	TabCtrl_AdjustRect(hWnd, FALSE, &page_rect);
	InflateRect(&page_rect, 1, 1);
	FrameRect(mdc, &page_rect, dark_mode_get_brush(COLOR_3DLIGHT));

	hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
	hRetFont = (hFont != NULL) ? SelectObject(mdc, hFont) : NULL;
	SetBkMode(mdc, TRANSPARENT);

	sel = TabCtrl_GetCurSel(hWnd);
	cnt = TabCtrl_GetItemCount(hWnd);
	for (i = 0; i < cnt; i++) {
		if (TabCtrl_GetItemRect(hWnd, i, &item_rect) == FALSE) {
			continue;
		}
		*buf = TEXT('\0');
		ZeroMemory(&tci, sizeof(tci));
		tci.mask = TCIF_TEXT;
		tci.pszText = buf;
		tci.cchTextMax = DARK_BUF_SIZE - 1;
		TabCtrl_GetItem(hWnd, i, &tci);

		if (i == sel) {
			InflateRect(&item_rect, 1, 1);
			FillRect(mdc, &item_rect, dark_mode_get_brush(COLOR_BTNFACE));
		} else {
			FillRect(mdc, &item_rect, dark_mode_get_brush(COLOR_WINDOW));
		}
		FrameRect(mdc, &item_rect, dark_mode_get_brush(COLOR_3DLIGHT));
		if (i == sel) {
			RECT edge_rect;

			// 選択中のタブは表示領域とつなげる
			SetRect(&edge_rect, item_rect.left + 1, item_rect.bottom - 2,
				item_rect.right - 1, item_rect.bottom);
			FillRect(mdc, &edge_rect, dark_mode_get_brush(COLOR_BTNFACE));
		}

		SetTextColor(mdc, dark_mode_get_color(COLOR_BTNTEXT));
		DrawText(mdc, buf, lstrlen(buf), &item_rect,
			DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
	}
	if (hRetFont != NULL) {
		SelectObject(mdc, hRetFont);
	}

	BitBlt(hdc, 0, 0, width, height, mdc, 0, 0, SRCCOPY);
	SelectObject(mdc, hRetBmp);
	DeleteObject(hBmp);
	DeleteDC(mdc);
	EndPaint(hWnd, &ps);
}

/*
 * dark_mode_tab_proc - タブコントロールのサブクラスプロシージャ
 */
static LRESULT CALLBACK dark_mode_tab_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (msg) {
	case WM_NCDESTROY:
		RemoveProp(hWnd, DARK_MODE_PROP);
		RemoveWindowSubclass(hWnd, dark_mode_tab_proc, uIdSubclass);
		break;

	case WM_ERASEBKGND:
		if (dark_mode_dark == TRUE) {
			return TRUE;
		}
		break;

	case WM_PAINT:
		if (dark_mode_dark == TRUE) {
			dark_mode_draw_tab(hWnd);
			return 0;
		}
		break;
	}
	return DefSubclassProc(hWnd, msg, wParam, lParam);
}

/*
 * dark_mode_draw_statusbar - ステータスバーの描画
 */
static void dark_mode_draw_statusbar(const HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hdc;
	HFONT hFont, hRetFont;
	RECT client_rect, item_rect;
	TCHAR buf[DARK_BUF_SIZE];
	int cnt, i, len;

	if ((hdc = BeginPaint(hWnd, &ps)) == NULL) {
		return;
	}
	GetClientRect(hWnd, &client_rect);
	FillRect(hdc, &client_rect, dark_mode_get_brush(COLOR_BTNFACE));

	hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
	hRetFont = (hFont != NULL) ? SelectObject(hdc, hFont) : NULL;
	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, dark_mode_get_color(COLOR_BTNTEXT));

	cnt = (int)SendMessage(hWnd, SB_GETPARTS, 0, 0);
	for (i = 0; i < cnt; i++) {
		if (SendMessage(hWnd, SB_GETRECT, i, (LPARAM)&item_rect) == FALSE) {
			continue;
		}
		// 区切り
		if (i < cnt - 1) {
			RECT sep_rect;

			SetRect(&sep_rect, item_rect.right, item_rect.top + 2, item_rect.right + 1, item_rect.bottom - 2);
			FillRect(hdc, &sep_rect, dark_mode_get_brush(COLOR_3DLIGHT));
		}
		len = (int)SendMessage(hWnd, SB_GETTEXTLENGTH, i, 0);
		if (LOWORD(len) == 0 || LOWORD(len) >= DARK_BUF_SIZE || (HIWORD(len) & SBT_OWNERDRAW)) {
			continue;
		}
		*buf = TEXT('\0');
		SendMessage(hWnd, SB_GETTEXT, i, (LPARAM)buf);
		if (*buf == TEXT('\0')) {
			continue;
		}
		item_rect.left += 2;
		item_rect.right -= 2;
		DrawText(hdc, buf, lstrlen(buf), &item_rect,
			DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
	}
	if (hRetFont != NULL) {
		SelectObject(hdc, hRetFont);
	}
	EndPaint(hWnd, &ps);
}

/*
 * dark_mode_statusbar_proc - ステータスバーのサブクラスプロシージャ
 */
static LRESULT CALLBACK dark_mode_statusbar_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (msg) {
	case WM_NCDESTROY:
		RemoveProp(hWnd, DARK_MODE_PROP);
		RemoveWindowSubclass(hWnd, dark_mode_statusbar_proc, uIdSubclass);
		break;

	case WM_ERASEBKGND:
		if (dark_mode_dark == TRUE) {
			return TRUE;
		}
		break;

	case WM_PAINT:
		if (dark_mode_dark == TRUE) {
			dark_mode_draw_statusbar(hWnd);
			return 0;
		}
		break;
	}
	return DefSubclassProc(hWnd, msg, wParam, lParam);
}

/*
 * dark_mode_draw_header - ヘッダの描画
 */
static void dark_mode_draw_header(const HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hdc, mdc;
	HBITMAP hBmp, hRetBmp;
	HFONT hFont, hRetFont;
	TEXTMETRIC tm;
	RECT client_rect, item_rect, sep_rect;
	TCHAR buf[DARK_BUF_SIZE];
	HDITEM hdi;
	DWORD flags;
	int width, height;
	int margin;
	int cnt, i;

	if ((hdc = BeginPaint(hWnd, &ps)) == NULL) {
		return;
	}
	GetClientRect(hWnd, &client_rect);
	width = client_rect.right - client_rect.left;
	height = client_rect.bottom - client_rect.top;

	if ((mdc = CreateCompatibleDC(hdc)) == NULL) {
		EndPaint(hWnd, &ps);
		return;
	}
	if ((hBmp = CreateCompatibleBitmap(hdc, width, height)) == NULL) {
		DeleteDC(mdc);
		EndPaint(hWnd, &ps);
		return;
	}
	hRetBmp = SelectObject(mdc, hBmp);

	// 背景
	FillRect(mdc, &client_rect, dark_mode_get_brush(COLOR_BTNFACE));

	hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
	hRetFont = (hFont != NULL) ? SelectObject(mdc, hFont) : NULL;
	GetTextMetrics(mdc, &tm);
	margin = tm.tmAveCharWidth;
	SetBkMode(mdc, TRANSPARENT);
	SetTextColor(mdc, dark_mode_get_color(COLOR_BTNTEXT));

	cnt = Header_GetItemCount(hWnd);
	for (i = 0; i < cnt; i++) {
		if (Header_GetItemRect(hWnd, i, &item_rect) == FALSE) {
			continue;
		}
		// 区切り
		SetRect(&sep_rect, item_rect.right - 1, item_rect.top + margin / 2,
			item_rect.right, item_rect.bottom - margin / 2);
		FillRect(mdc, &sep_rect, dark_mode_get_brush(COLOR_3DLIGHT));

		*buf = TEXT('\0');
		ZeroMemory(&hdi, sizeof(hdi));
		hdi.mask = HDI_TEXT | HDI_FORMAT;
		hdi.pszText = buf;
		hdi.cchTextMax = DARK_BUF_SIZE - 1;
		if (Header_GetItem(hWnd, i, &hdi) == FALSE || *buf == TEXT('\0')) {
			continue;
		}
		switch (hdi.fmt & HDF_JUSTIFYMASK) {
		case HDF_RIGHT:
			flags = DT_RIGHT;
			break;

		case HDF_CENTER:
			flags = DT_CENTER;
			break;

		default:
			flags = DT_LEFT;
			break;
		}
		item_rect.left += margin;
		item_rect.right -= margin;
		DrawText(mdc, buf, lstrlen(buf), &item_rect,
			flags | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
	}
	if (hRetFont != NULL) {
		SelectObject(mdc, hRetFont);
	}

	BitBlt(hdc, 0, 0, width, height, mdc, 0, 0, SRCCOPY);
	SelectObject(mdc, hRetBmp);
	DeleteObject(hBmp);
	DeleteDC(mdc);
	EndPaint(hWnd, &ps);
}

/*
 * dark_mode_header_proc - ヘッダのサブクラスプロシージャ
 */
static LRESULT CALLBACK dark_mode_header_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (msg) {
	case WM_NCDESTROY:
		RemoveProp(hWnd, DARK_MODE_PROP);
		RemoveWindowSubclass(hWnd, dark_mode_header_proc, uIdSubclass);
		break;

	case WM_ERASEBKGND:
		if (dark_mode_dark == TRUE) {
			return TRUE;
		}
		break;

	case WM_PAINT:
		if (dark_mode_dark == TRUE) {
			dark_mode_draw_header(hWnd);
			return 0;
		}
		break;
	}
	return DefSubclassProc(hWnd, msg, wParam, lParam);
}

/*
 * dark_mode_notify_proc - 子コントロールの通知を処理するサブクラスプロシージャ
 */
static LRESULT CALLBACK dark_mode_notify_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (msg) {
	case WM_NCDESTROY:
		RemoveWindowSubclass(hWnd, dark_mode_notify_proc, uIdSubclass);
		break;

	case WM_NOTIFY:
		// ツールチップは表示の直前に作成されることがあるため都度設定する
		if (((NMHDR *)lParam)->code == TTN_SHOW) {
			dark_mode_set_tooltip(((NMHDR *)lParam)->hwndFrom);
		}
		break;
	}
	return DefSubclassProc(hWnd, msg, wParam, lParam);
}

/*
 * dark_mode_draw_groupbox - グループボックスの描画
 */
static void dark_mode_draw_groupbox(const HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hdc;
	HFONT hFont, hRetFont;
	RECT client_rect, frame_rect, text_rect;
	TCHAR buf[DARK_BUF_SIZE];
	SIZE sz;
	int len;

	if ((hdc = BeginPaint(hWnd, &ps)) == NULL) {
		return;
	}
	GetClientRect(hWnd, &client_rect);

	hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
	hRetFont = (hFont != NULL) ? SelectObject(hdc, hFont) : NULL;

	*buf = TEXT('\0');
	GetWindowText(hWnd, buf, DARK_BUF_SIZE - 1);
	len = lstrlen(buf);
	sz.cx = 0;
	sz.cy = 0;
	if (len > 0) {
		GetTextExtentPoint32(hdc, buf, len, &sz);
	}

	// 枠
	frame_rect = client_rect;
	frame_rect.top += sz.cy / 2;
	FrameRect(hdc, &frame_rect, dark_mode_get_brush(COLOR_3DLIGHT));

	// タイトル
	if (len > 0) {
		SetRect(&text_rect, client_rect.left + sz.cy / 2, client_rect.top,
			client_rect.left + sz.cy / 2 + sz.cx + sz.cy / 2, client_rect.top + sz.cy);
		FillRect(hdc, &text_rect, dark_mode_get_brush(COLOR_BTNFACE));

		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, (IsWindowEnabled(hWnd) == FALSE) ?
			dark_mode_get_color(COLOR_GRAYTEXT) : dark_mode_get_color(COLOR_BTNTEXT));
		text_rect.left += sz.cy / 4;
		DrawText(hdc, buf, len, &text_rect, DT_LEFT | DT_TOP | DT_SINGLELINE);
	}
	if (hRetFont != NULL) {
		SelectObject(hdc, hRetFont);
	}
	EndPaint(hWnd, &ps);
}

/*
 * dark_mode_draw_radio - ラジオボタンの描画
 */
static void dark_mode_draw_radio(const HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hdc;
	HFONT hFont, hRetFont;
	HBRUSH hBrush, hRetBrush;
	HPEN hPen, hRetPen;
	RECT client_rect, glyph_rect, text_rect;
	TCHAR buf[DARK_BUF_SIZE];
	COLORREF text_color;
	DWORD flags = DT_LEFT | DT_VCENTER | DT_SINGLELINE;
	int size;
	int len;

	if ((hdc = BeginPaint(hWnd, &ps)) == NULL) {
		return;
	}
	GetClientRect(hWnd, &client_rect);

	// 背景
	FillRect(hdc, &client_rect, dark_mode_get_brush(COLOR_BTNFACE));

	text_color = (IsWindowEnabled(hWnd) == FALSE) ?
		dark_mode_get_color(COLOR_GRAYTEXT) : dark_mode_get_color(COLOR_BTNTEXT);

	// グリフ
	size = Scale(RADIO_GLYPH_SIZE);
	SetRect(&glyph_rect, 0,
		(client_rect.top + client_rect.bottom) / 2 - size / 2, size,
		(client_rect.top + client_rect.bottom) / 2 - size / 2 + size);
	hPen = CreatePen(PS_SOLID, 1, text_color);
	hRetPen = SelectObject(hdc, hPen);
	hRetBrush = SelectObject(hdc, dark_mode_get_brush(COLOR_WINDOW));
	Ellipse(hdc, glyph_rect.left, glyph_rect.top, glyph_rect.right, glyph_rect.bottom);
	SelectObject(hdc, hRetBrush);
	SelectObject(hdc, hRetPen);
	DeleteObject(hPen);
	if (SendMessage(hWnd, BM_GETCHECK, 0, 0) == BST_CHECKED) {
		// チェックの点
		COLORREF check_color = (IsWindowEnabled(hWnd) == FALSE) ?
			dark_mode_get_color(COLOR_GRAYTEXT) : dark_mode_get_accent_color();

		InflateRect(&glyph_rect, -size / 4, -size / 4);
		hBrush = CreateSolidBrush(check_color);
		hPen = CreatePen(PS_SOLID, 1, check_color);
		hRetBrush = SelectObject(hdc, hBrush);
		hRetPen = SelectObject(hdc, hPen);
		Ellipse(hdc, glyph_rect.left, glyph_rect.top, glyph_rect.right, glyph_rect.bottom);
		SelectObject(hdc, hRetPen);
		SelectObject(hdc, hRetBrush);
		DeleteObject(hPen);
		DeleteObject(hBrush);
	}

	// テキスト
	hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
	hRetFont = (hFont != NULL) ? SelectObject(hdc, hFont) : NULL;
	SetBkMode(hdc, TRANSPARENT);
	SetTextColor(hdc, text_color);

	*buf = TEXT('\0');
	GetWindowText(hWnd, buf, DARK_BUF_SIZE - 1);
	len = lstrlen(buf);
	text_rect = client_rect;
	text_rect.left += Scale(RADIO_TEXT_MARGIN);
	if (LOWORD(SendMessage(hWnd, WM_QUERYUISTATE, 0, 0)) & UISF_HIDEACCEL) {
		flags |= DT_HIDEPREFIX;
	}
	DrawText(hdc, buf, len, &text_rect, flags);

	// フォーカス
	if (GetFocus() == hWnd &&
		!(LOWORD(SendMessage(hWnd, WM_QUERYUISTATE, 0, 0)) & UISF_HIDEFOCUS)) {
		RECT focus_rect = text_rect;

		DrawText(hdc, buf, len, &focus_rect, flags | DT_CALCRECT);
		focus_rect.top = client_rect.top;
		focus_rect.bottom = client_rect.bottom;
		InflateRect(&focus_rect, 1, 0);
		if (focus_rect.right > client_rect.right) {
			focus_rect.right = client_rect.right;
		}
		DrawFocusRect(hdc, &focus_rect);
	}
	if (hRetFont != NULL) {
		SelectObject(hdc, hRetFont);
	}
	EndPaint(hWnd, &ps);
}

/*
 * dark_mode_radio_proc - ラジオボタンのサブクラスプロシージャ
 */
static LRESULT CALLBACK dark_mode_radio_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (msg) {
	case WM_NCDESTROY:
		RemoveProp(hWnd, DARK_MODE_PROP);
		RemoveWindowSubclass(hWnd, dark_mode_radio_proc, uIdSubclass);
		break;

	case WM_PAINT:
		if (dark_mode_dark == TRUE) {
			dark_mode_draw_radio(hWnd);
			return 0;
		}
		break;

	case WM_SETFOCUS:
	case WM_KILLFOCUS:
		// フォーカス枠を描き直す
		if (dark_mode_dark == TRUE) {
			InvalidateRect(hWnd, NULL, TRUE);
		}
		break;
	}
	return DefSubclassProc(hWnd, msg, wParam, lParam);
}

/*
 * dark_mode_groupbox_proc - グループボックスのサブクラスプロシージャ
 */
static LRESULT CALLBACK dark_mode_groupbox_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
	UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (msg) {
	case WM_NCDESTROY:
		RemoveProp(hWnd, DARK_MODE_PROP);
		RemoveWindowSubclass(hWnd, dark_mode_groupbox_proc, uIdSubclass);
		break;

	case WM_PAINT:
		if (dark_mode_dark == TRUE) {
			dark_mode_draw_groupbox(hWnd);
			return 0;
		}
		break;
	}
	return DefSubclassProc(hWnd, msg, wParam, lParam);
}

/*
 * dark_mode_draw_menubar - メニューバーの背景の描画
 */
static BOOL dark_mode_draw_menubar(const HWND hWnd, const UAHMENU *pum)
{
	MENUBARINFO mbi;
	RECT window_rect, rect;

	ZeroMemory(&mbi, sizeof(mbi));
	mbi.cbSize = sizeof(mbi);
	if (GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi) == FALSE) {
		return FALSE;
	}
	GetWindowRect(hWnd, &window_rect);

	rect = mbi.rcBar;
	OffsetRect(&rect, -window_rect.left, -window_rect.top);
	FillRect(pum->hdc, &rect, dark_mode_get_brush(COLOR_MENU));
	return TRUE;
}

/*
 * dark_mode_draw_menubar_item - メニューバーの項目の描画
 */
static BOOL dark_mode_draw_menubar_item(const UAHDRAWMENUITEM *pumdi)
{
	MENUITEMINFO mii;
	RECT rect;
	TCHAR buf[DARK_BUF_SIZE];
	DWORD flags = DT_CENTER | DT_SINGLELINE | DT_VCENTER;

	*buf = TEXT('\0');
	ZeroMemory(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STRING;
	mii.dwTypeData = buf;
	mii.cch = DARK_BUF_SIZE - 1;
	if (GetMenuItemInfo(pumdi->um.hmenu, pumdi->umi.iPosition, TRUE, &mii) == FALSE) {
		return FALSE;
	}
	if (pumdi->dis.itemState & ODS_NOACCEL) {
		flags |= DT_HIDEPREFIX;
	}

	rect = pumdi->dis.rcItem;
	if (pumdi->dis.itemState & (ODS_HOTLIGHT | ODS_SELECTED)) {
		FillRect(pumdi->um.hdc, &rect, dark_mode_get_brush(COLOR_MENUHILIGHT));
		SetTextColor(pumdi->um.hdc, dark_mode_get_color(COLOR_HIGHLIGHTTEXT));
	} else {
		FillRect(pumdi->um.hdc, &rect, dark_mode_get_brush(COLOR_MENU));
		SetTextColor(pumdi->um.hdc, (pumdi->dis.itemState & (ODS_GRAYED | ODS_DISABLED)) ?
			dark_mode_get_color(COLOR_GRAYTEXT) : dark_mode_get_color(COLOR_MENUTEXT));
	}
	SetBkMode(pumdi->um.hdc, TRANSPARENT);
	DrawText(pumdi->um.hdc, buf, lstrlen(buf), &rect, flags);
	return TRUE;
}

/*
 * dark_mode_draw_menubar_border - メニューバー下の境界線の描画
 */
static void dark_mode_draw_menubar_border(const HWND hWnd)
{
	MENUBARINFO mbi;
	RECT window_rect, rect;
	HDC hdc;

	ZeroMemory(&mbi, sizeof(mbi));
	mbi.cbSize = sizeof(mbi);
	if (GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi) == FALSE) {
		return;
	}
	GetWindowRect(hWnd, &window_rect);

	rect = mbi.rcBar;
	OffsetRect(&rect, -window_rect.left, -window_rect.top);
	rect.top = rect.bottom;
	rect.bottom += 1;

	if ((hdc = GetWindowDC(hWnd)) == NULL) {
		return;
	}
	FillRect(hdc, &rect, dark_mode_get_brush(COLOR_MENU));
	ReleaseDC(hWnd, hdc);
}

/*
 * dark_mode_menubar_message - メニューバーの描画メッセージの処理
 */
BOOL dark_mode_menubar_message(const HWND hWnd, const UINT msg, const WPARAM wParam, const LPARAM lParam, LRESULT *ret)
{
	if (dark_mode_dark == FALSE || ret == NULL) {
		return FALSE;
	}
	switch (msg) {
	case WM_UAHDRAWMENU:
		if (dark_mode_draw_menubar(hWnd, (UAHMENU *)lParam) == FALSE) {
			return FALSE;
		}
		*ret = TRUE;
		return TRUE;

	case WM_UAHDRAWMENUITEM:
		if (dark_mode_draw_menubar_item((UAHDRAWMENUITEM *)lParam) == FALSE) {
			return FALSE;
		}
		*ret = TRUE;
		return TRUE;

	case WM_NCPAINT:
	case WM_NCACTIVATE:
		*ret = DefWindowProc(hWnd, msg, wParam, lParam);
		dark_mode_draw_menubar_border(hWnd);
		return TRUE;
	}
	return FALSE;
}

/*
 * dark_mode_toolbar_customdraw - ツールバーのカスタムドローの処理
 */
BOOL dark_mode_toolbar_customdraw(const LPARAM lParam, LRESULT *ret)
{
	LPNMTBCUSTOMDRAW nmtb = (LPNMTBCUSTOMDRAW)lParam;

	if (dark_mode_dark == FALSE || ret == NULL) {
		return FALSE;
	}
	switch (nmtb->nmcd.dwDrawStage) {
	case CDDS_PREPAINT:
		FillRect(nmtb->nmcd.hdc, &nmtb->nmcd.rc, dark_mode_get_brush(COLOR_BTNFACE));
		*ret = CDRF_NOTIFYITEMDRAW;
		return TRUE;

	case CDDS_ITEMPREPAINT:
		nmtb->clrText = dark_mode_get_color(COLOR_BTNTEXT);
		nmtb->clrTextHighlight = dark_mode_get_color(COLOR_HIGHLIGHTTEXT);
		nmtb->clrBtnFace = dark_mode_get_color(COLOR_BTNFACE);
		nmtb->clrBtnHighlight = dark_mode_get_color(COLOR_BTNSHADOW);
		nmtb->clrHighlightHotTrack = dark_mode_get_color(COLOR_BTNSHADOW);
		nmtb->nStringBkMode = TRANSPARENT;
		nmtb->nHLStringBkMode = TRANSPARENT;
		*ret = TBCDRF_USECDCOLORS | TBCDRF_HILITEHOTTRACK | CDRF_DODEFAULT;
		return TRUE;
	}
	return FALSE;
}

/*
 * dark_mode_draw_arrow_button - 矢印ボタンの描画
 */
void dark_mode_draw_arrow_button(const DRAWITEMSTRUCT *ds, const UINT type)
{
	HDC hdc = ds->hDC;
	RECT rect = ds->rcItem;
	POINT pt[3];
	HBRUSH hBrush, hRetBrush;
	HPEN hPen, hRetPen;
	COLORREF color;
	int cx, cy, size;

	// 背景
	FillRect(hdc, &rect,
		dark_mode_get_brush((ds->itemState & ODS_SELECTED) ? COLOR_BTNSHADOW : COLOR_BTNFACE));
	FrameRect(hdc, &rect, dark_mode_get_brush(COLOR_3DLIGHT));

	// 矢印
	color = (ds->itemState & ODS_DISABLED) ?
		dark_mode_get_color(COLOR_GRAYTEXT) : dark_mode_get_color(COLOR_BTNTEXT);
	cx = (rect.left + rect.right) / 2;
	cy = (rect.top + rect.bottom) / 2;
	size = ((rect.right - rect.left) < (rect.bottom - rect.top)) ?
		(rect.right - rect.left) : (rect.bottom - rect.top);
	size = size / 4;
	if (size < 2) {
		size = 2;
	}
	switch (type) {
	case DFCS_SCROLLUP:
		pt[0].x = cx;			pt[0].y = cy - size;
		pt[1].x = cx + size;	pt[1].y = cy + size;
		pt[2].x = cx - size;	pt[2].y = cy + size;
		break;

	case DFCS_SCROLLDOWN:
		pt[0].x = cx;			pt[0].y = cy + size;
		pt[1].x = cx - size;	pt[1].y = cy - size;
		pt[2].x = cx + size;	pt[2].y = cy - size;
		break;

	case DFCS_SCROLLLEFT:
		pt[0].x = cx - size;	pt[0].y = cy;
		pt[1].x = cx + size;	pt[1].y = cy - size;
		pt[2].x = cx + size;	pt[2].y = cy + size;
		break;

	default:
		pt[0].x = cx + size;	pt[0].y = cy;
		pt[1].x = cx - size;	pt[1].y = cy + size;
		pt[2].x = cx - size;	pt[2].y = cy - size;
		break;
	}
	hBrush = CreateSolidBrush(color);
	hPen = CreatePen(PS_SOLID, 1, color);
	hRetBrush = SelectObject(hdc, hBrush);
	hRetPen = SelectObject(hdc, hPen);
	Polygon(hdc, pt, 3);
	SelectObject(hdc, hRetPen);
	SelectObject(hdc, hRetBrush);
	DeleteObject(hPen);
	DeleteObject(hBrush);

	// フォーカス
	if (ds->itemState & ODS_FOCUS) {
		InflateRect(&rect, -3, -3);
		DrawFocusRect(hdc, &rect);
	}
}
/* End of source */
