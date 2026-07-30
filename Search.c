/*
 * CLCL
 *
 * Search.c
 *
 * Search popup for clipboard history - filter and select items by keyword
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE
#include <tchar.h>
#include <shlwapi.h>

#include "General.h"
#include "Memory.h"
#include "Data.h"
#include "Ini.h"
#include "Format.h"
#include "Font.h"
#include "Search.h"

/* Define */
#define SEARCH_WIDTH					400
#define SEARCH_HEIGHT					350
#define SEARCH_EDIT_HEIGHT				24
#define SEARCH_MARGIN					4

#define IDC_SEARCH_EDIT					1
#define IDC_SEARCH_LIST					2

#define SEARCH_INIT_CAPACITY			256

/* Global Variables */
extern HINSTANCE hInst;
extern OPTION_INFO option;

/* Local Variables */
static DATA_INFO *search_result;
static DATA_INFO *search_history_root;
static HWND hSearchWnd;
static HWND hEditCtrl;
static HWND hListBox;
static WNDPROC origListBoxProc;
static WNDPROC origEditProc;
static HFONT hSearchFont;

// Dynamic array mapping listbox index to DATA_INFO*
static DATA_INFO **search_items;
static int search_item_count;
static int search_item_capacity;

/* Local Function Prototypes */
static LRESULT CALLBACK search_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK listbox_subproc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK edit_subproc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void search_populate(const TCHAR *filter);
static void search_collect(DATA_INFO *di, const TCHAR *filter);
static void search_add_item(DATA_INFO *di, const TCHAR *title);
static TCHAR *search_get_title(DATA_INFO *di);
static void search_select_item(void);
static HFONT search_create_font(void);

/*
 * search_regist - register window class
 */
BOOL search_regist(const HINSTANCE hInstance)
{
	WNDCLASS wc;

	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = (WNDPROC)search_proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = SEARCH_WND_CLASS;
	return RegisterClass(&wc);
}

/*
 * search_create_font - create font for search controls
 */
static HFONT search_create_font(void)
{
	NONCLIENTMETRICS ncMetrics;

	if (*option.menu_font_name != TEXT('\0')) {
		return font_create(option.menu_font_name, option.menu_font_size, option.menu_font_charset,
			option.menu_font_weight, (option.menu_font_italic == 0) ? FALSE : TRUE, FALSE);
	}

	ncMetrics.cbSize = sizeof(NONCLIENTMETRICS);
	if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
		sizeof(NONCLIENTMETRICS), &ncMetrics, 0) == FALSE) {
		return NULL;
	}
	return CreateFontIndirect(&ncMetrics.lfMenuFont);
}

/*
 * search_get_title - get display title for a DATA_INFO
 */
static TCHAR *search_get_title(DATA_INFO *di)
{
	DATA_INFO *show_di;

	if (di == NULL) {
		return NULL;
	}
	if (di->title != NULL && lstrcmp(di->title, TEXT("-")) != 0) {
		return di->title;
	}
	if (di->type == TYPE_ITEM) {
		show_di = format_get_priority_highest(di);
		if (show_di != NULL) {
			format_get_menu_title(show_di);
			if (show_di->menu_title != NULL) {
				return show_di->menu_title;
			}
			if (show_di->format_name != NULL) {
				return show_di->format_name;
			}
		}
	}
	if (di->menu_title != NULL) {
		return di->menu_title;
	}
	if (di->format_name != NULL) {
		return di->format_name;
	}
	return NULL;
}

/*
 * search_add_item - add an item to the search list and listbox
 */
static void search_add_item(DATA_INFO *di, const TCHAR *title)
{
	DATA_INFO **new_items;

	if (title == NULL || *title == TEXT('\0')) {
		return;
	}
	// Grow array if needed
	if (search_item_count >= search_item_capacity) {
		search_item_capacity *= 2;
		new_items = (DATA_INFO **)mem_alloc(sizeof(DATA_INFO *) * search_item_capacity);
		if (new_items == NULL) {
			return;
		}
		CopyMemory(new_items, search_items, sizeof(DATA_INFO *) * search_item_count);
		mem_free((void **)&search_items);
		search_items = new_items;
	}
	search_items[search_item_count] = di;
	search_item_count++;
	SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)title);
}

/*
 * search_collect - recursively collect matching items from history
 */
static void search_collect(DATA_INFO *di, const TCHAR *filter)
{
	TCHAR *title;

	for (; di != NULL; di = di->next) {
		if (di->type == TYPE_FOLDER) {
			// Recurse into folders
			if (di->child != NULL) {
				search_collect(di->child, filter);
			}
			continue;
		}
		// Skip separators
		if (di->title != NULL && lstrcmp(di->title, TEXT("-")) == 0) {
			continue;
		}
		title = search_get_title(di);
		if (title == NULL) {
			continue;
		}
		// Match filter (empty filter matches all)
		if (*filter == TEXT('\0') || StrStrI(title, filter) != NULL) {
			search_add_item(di, title);
		}
	}
}

/*
 * search_populate - populate listbox with filtered items
 */
static void search_populate(const TCHAR *filter)
{
	SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
	search_item_count = 0;
	search_collect(search_history_root, filter);
	if (search_item_count > 0) {
		SendMessage(hListBox, LB_SETCURSEL, 0, 0);
	}
}

/*
 * search_select_item - select the current listbox item and close
 */
static void search_select_item(void)
{
	int sel;

	sel = (int)SendMessage(hListBox, LB_GETCURSEL, 0, 0);
	if (sel >= 0 && sel < search_item_count) {
		search_result = search_items[sel];
	} else {
		search_result = NULL;
	}
	DestroyWindow(hSearchWnd);
}

/*
 * listbox_subproc - subclassed listbox for vi-style keys
 */
static LRESULT CALLBACK listbox_subproc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int sel, count;

	switch (msg) {
	case WM_KEYDOWN:
		switch (wParam) {
		case VK_RETURN:
			search_select_item();
			return 0;

		case VK_ESCAPE:
			search_result = NULL;
			DestroyWindow(hSearchWnd);
			return 0;

		case VK_OEM_2:
			// '/' key - focus search edit
			SetFocus(hEditCtrl);
			return 0;
		}
		break;

	case WM_CHAR:
		switch (wParam) {
		case TEXT('j'):
			// Move down
			count = (int)SendMessage(hWnd, LB_GETCOUNT, 0, 0);
			sel = (int)SendMessage(hWnd, LB_GETCURSEL, 0, 0);
			if (sel < count - 1) {
				SendMessage(hWnd, LB_SETCURSEL, sel + 1, 0);
			}
			return 0;

		case TEXT('k'):
			// Move up
			sel = (int)SendMessage(hWnd, LB_GETCURSEL, 0, 0);
			if (sel > 0) {
				SendMessage(hWnd, LB_SETCURSEL, sel - 1, 0);
			}
			return 0;

		case TEXT('g'):
			// Go to top
			SendMessage(hWnd, LB_SETCURSEL, 0, 0);
			return 0;

		case TEXT('G'):
			// Go to bottom
			count = (int)SendMessage(hWnd, LB_GETCOUNT, 0, 0);
			if (count > 0) {
				SendMessage(hWnd, LB_SETCURSEL, count - 1, 0);
			}
			return 0;

		case TEXT('/'):
			// Consumed (handled in WM_KEYDOWN)
			return 0;
		}
		break;

	case WM_LBUTTONDBLCLK:
		search_select_item();
		return 0;
	}
	return CallWindowProc(origListBoxProc, hWnd, msg, wParam, lParam);
}

/*
 * edit_subproc - subclassed edit control for navigation keys
 */
static LRESULT CALLBACK edit_subproc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int sel, count;

	switch (msg) {
	case WM_KEYDOWN:
		switch (wParam) {
		case VK_RETURN:
			search_select_item();
			return 0;

		case VK_ESCAPE:
			search_result = NULL;
			DestroyWindow(hSearchWnd);
			return 0;

		case VK_DOWN:
			// Move selection down in listbox
			count = (int)SendMessage(hListBox, LB_GETCOUNT, 0, 0);
			sel = (int)SendMessage(hListBox, LB_GETCURSEL, 0, 0);
			if (sel < count - 1) {
				SendMessage(hListBox, LB_SETCURSEL, sel + 1, 0);
			} else if (sel == -1 && count > 0) {
				SendMessage(hListBox, LB_SETCURSEL, 0, 0);
			}
			return 0;

		case VK_UP:
			// Move selection up in listbox
			sel = (int)SendMessage(hListBox, LB_GETCURSEL, 0, 0);
			if (sel > 0) {
				SendMessage(hListBox, LB_SETCURSEL, sel - 1, 0);
			}
			return 0;
		}
		break;
	}
	return CallWindowProc(origEditProc, hWnd, msg, wParam, lParam);
}

/*
 * search_proc - search popup window procedure
 */
static LRESULT CALLBACK search_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	TCHAR buf[BUF_SIZE];
	int client_w, client_h;
	int edit_h;

	switch (msg) {
	case WM_CREATE:
		client_w = SEARCH_WIDTH - SEARCH_MARGIN * 2;
		edit_h = SEARCH_EDIT_HEIGHT;
		client_h = SEARCH_HEIGHT;

		// Create edit control (search box)
		hEditCtrl = CreateWindowEx(0, TEXT("EDIT"), TEXT(""),
			WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
			SEARCH_MARGIN, SEARCH_MARGIN,
			client_w, edit_h,
			hWnd, (HMENU)IDC_SEARCH_EDIT, hInst, NULL);

		// Create listbox (results)
		hListBox = CreateWindowEx(0, TEXT("LISTBOX"), TEXT(""),
			WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
			LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
			SEARCH_MARGIN, SEARCH_MARGIN + edit_h + SEARCH_MARGIN,
			client_w, client_h - edit_h - SEARCH_MARGIN * 3,
			hWnd, (HMENU)IDC_SEARCH_LIST, hInst, NULL);

		// Set font
		hSearchFont = search_create_font();
		if (hSearchFont != NULL) {
			SendMessage(hEditCtrl, WM_SETFONT, (WPARAM)hSearchFont, TRUE);
			SendMessage(hListBox, WM_SETFONT, (WPARAM)hSearchFont, TRUE);
		}

		// Subclass controls
		origListBoxProc = (WNDPROC)SetWindowLongPtr(hListBox, GWLP_WNDPROC, (LONG_PTR)listbox_subproc);
		origEditProc = (WNDPROC)SetWindowLongPtr(hEditCtrl, GWLP_WNDPROC, (LONG_PTR)edit_subproc);

		// Allocate search items array
		search_item_capacity = SEARCH_INIT_CAPACITY;
		search_items = (DATA_INFO **)mem_alloc(sizeof(DATA_INFO *) * search_item_capacity);
		search_item_count = 0;

		// Populate with all items
		search_populate(TEXT(""));

		// Focus on edit
		SetFocus(hEditCtrl);
		break;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_SEARCH_EDIT && HIWORD(wParam) == EN_CHANGE) {
			// Filter changed - repopulate
			GetWindowText(hEditCtrl, buf, BUF_SIZE);
			search_populate(buf);
		}
		if (LOWORD(wParam) == IDC_SEARCH_LIST && HIWORD(wParam) == LBN_DBLCLK) {
			search_select_item();
		}
		break;

	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE) {
			// Lost focus - close like a menu
			search_result = NULL;
			DestroyWindow(hWnd);
		}
		break;

	case WM_DESTROY:
		// Restore subclassed wndprocs
		if (hListBox != NULL && origListBoxProc != NULL) {
			SetWindowLongPtr(hListBox, GWLP_WNDPROC, (LONG_PTR)origListBoxProc);
		}
		if (hEditCtrl != NULL && origEditProc != NULL) {
			SetWindowLongPtr(hEditCtrl, GWLP_WNDPROC, (LONG_PTR)origEditProc);
		}
		// Free resources
		if (search_items != NULL) {
			mem_free((void **)&search_items);
		}
		search_item_count = 0;
		search_item_capacity = 0;
		if (hSearchFont != NULL) {
			DeleteObject(hSearchFont);
			hSearchFont = NULL;
		}
		hEditCtrl = NULL;
		hListBox = NULL;
		origListBoxProc = NULL;
		origEditProc = NULL;
		hSearchWnd = NULL;
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

/*
 * search_popup_show - show search popup and return selected item
 */
DATA_INFO *search_popup_show(const HWND hWnd, DATA_INFO *history_root)
{
	POINT pos;
	MSG msg;
	int x, y;
	int screen_w, screen_h;

	search_history_root = history_root;
	search_result = NULL;

	// Position at cursor
	GetCursorPos(&pos);
	x = pos.x;
	y = pos.y;

	// Clamp to screen
	screen_w = GetSystemMetrics(SM_CXSCREEN);
	screen_h = GetSystemMetrics(SM_CYSCREEN);
	if (x + SEARCH_WIDTH > screen_w) {
		x = screen_w - SEARCH_WIDTH;
	}
	if (y + SEARCH_HEIGHT > screen_h) {
		y = screen_h - SEARCH_HEIGHT;
	}
	if (x < 0) x = 0;
	if (y < 0) y = 0;

	// Create search popup
	hSearchWnd = CreateWindowEx(
		WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
		SEARCH_WND_CLASS, TEXT(""),
		WS_POPUP | WS_BORDER,
		x, y, SEARCH_WIDTH, SEARCH_HEIGHT,
		hWnd, NULL, hInst, NULL);

	if (hSearchWnd == NULL) {
		return NULL;
	}

	ShowWindow(hSearchWnd, SW_SHOW);
	UpdateWindow(hSearchWnd);

	// Modal message loop
	while (GetMessage(&msg, NULL, 0, 0) == TRUE) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return search_result;
}
/* End of source */
