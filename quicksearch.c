/*
 * CLCL
 *
 * search.c
 *
 * Copyright (C) 2026 by Wilf Zimmermann. MIT License.
 *		https://linguversa.de/clcl
 *		https://github.com/wilfz/clcl
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE
#include <commctrl.h>
#include <tchar.h>
#include <strsafe.h>
#include <shlwapi.h>

#include "quicksearch.h"
#include "Data.h"
#include "Memory.h"
#include "ImageList.h"
#include "Ini.h"
#include "Profile.h"
#include "dpi.h"
#include "Font.h"
#include "ToolTip.h"

#include "dynamic_popup.h"


#define EM_HIDE_LISTBOX        (WM_USER + 86)

/* Global Variables */
HWND hWndClcl = NULL; // the one and only application window handle
HWND hWndTooltip = NULL; // the tooltip window handle
TCHAR ini_path[MAX_PATH] = TEXT("");

// extern
extern HINSTANCE hInst;
extern DATA_INFO history_data;
extern DATA_INFO regist_data;
extern OPTION_INFO option;
extern TCHAR work_path[];

/* Item data structure for owner-drawn listbox */
typedef struct {
	DATA_INFO* data_info;
	int icon_index;
	TCHAR text[BUF_SIZE];
} LISTBOX_ITEM_DATA;

/* Local Function Prototypes */
static int get_icon_index_for_data(DATA_INFO* di);
static int listbox_add_matches(HWND hListBox, DATA_INFO* start, const TCHAR* srch, int max_cnt);

static int listbox_add_matches(HWND hListBox, DATA_INFO* start, const TCHAR* srch, int max_cnt)
{
	int cnt = 0;
	int idx = 0;
	for (DATA_INFO* item = start; item != NULL; item = item->next)
	{
		switch (item->type) {
		case TYPE_ROOT:
		case TYPE_FOLDER:
			if (item->child != NULL) {
				// recursive call
				cnt = listbox_add_matches(hListBox, item->child, srch, max_cnt);
				if (cnt >= max_cnt)
					return cnt;
			}
			break;
		case TYPE_ITEM:
			// Look for case-insensitive matches in title
			if (item->title && item->title[0] != TEXT('\0') 
				&& (srch[0] == 0 || StrStrI(item->title, srch) != NULL))
			{
				//listbox_add_item_with_icon(hListBox, item->title, item);
				int icon_index = get_icon_index_for_data(item);
				PopupAddString(hListBox, item->title, icon_index, (UINT_PTR) item);
				if ((cnt = SendMessage(hListBox, LB_GETCOUNT, 0, 0)) >= max_cnt)
					return cnt;
			}
			else if (item->menu_title && item->menu_title[0] != TEXT('\0')
				&& (srch[0] == 0 || StrStrI(item->menu_title, srch) != NULL))
			{
				//listbox_add_item_with_icon(hListBox, item->menu_title, item);
				int icon_index = get_icon_index_for_data(item);
				PopupAddString(hListBox, item->menu_title, icon_index, (UINT_PTR)item);
				if ((cnt = SendMessage(hListBox, LB_GETCOUNT, 0, 0)) >= max_cnt)
					return cnt;
			}
			else {
				// Look for case-insensitive matches in textual content
				BOOL bMatches = FALSE;
				DATA_INFO* di = NULL;
				TCHAR* mem;

#ifdef UNICODE
				di = (DATA_INFO*)SendMessage(hWndClcl, WM_ITEM_GET_FORMAT_TO_ITEM, (WPARAM)TEXT("UNICODE TEXT"), (LPARAM)item);
#else
				di = (DATA_INFO*)SendMessage(hWndClcl, WM_ITEM_GET_FORMAT_TO_ITEM, (WPARAM)TEXT("TEXT"), (LPARAM)item);
#endif

				if (di == NULL || di->data == NULL || (mem = GlobalLock(di->data)) == NULL) {
					break;
				}

				switch (di->format) {
				case CF_UNICODETEXT:
					if (*(WCHAR*)mem == L'\0') {
						GlobalUnlock(di->data);
						continue;
					}
					break;

				case CF_TEXT:
					if (*(char*)mem == '\0') {
						GlobalUnlock(di->data);
						continue;
					}
					break;

				default:
					GlobalUnlock(di->data);
					continue;
				}

				TCHAR* s = (TCHAR*)mem;
				// Check case-insensititive whether srch is substring of s
				// (empty srch is always to be considered as match).
				bMatches = srch[0] == 0 || (StrStrI(s, srch) != NULL);
				if (bMatches) {
					TCHAR buf[BUF_SIZE];
					// ltrim whitespace etc.
					int i = 0;
					for (i = 0; i < _tcslen(s); i++) {
						if (s[i] != TEXT(' ') && s[i] != TEXT('\t') 
								&& s[i] != TEXT('\r') && s[i] != TEXT('\n') || s[i] == 0)
							break;
					}
					StringCchCopy(buf, BUF_SIZE, s + i);
					buf[BUF_SIZE-1] = TEXT('\0');
					int icon_index = get_icon_index_for_data(item);
					PopupAddString(hListBox, buf, icon_index, (UINT_PTR)item);
				}

				GlobalUnlock(di->data);

				if ((cnt = SendMessage(hListBox, LB_GETCOUNT, 0, 0)) >= max_cnt)
					return cnt;
			}
			break;
		}
	}

	cnt = (int)SendMessage(hListBox, LB_GETCOUNT, 0, 0);
	return cnt;
}

/*
 * get_icon_index_for_data
 * Determine the icon index based on the data format
 */
static int get_icon_index_for_data(DATA_INFO* di)
{
	if (di == NULL)
		return 5; // Default icon index

	// Based on data format, return appropriate icon index
	// Adjust these indices to match your ImageList structure
	switch (di->type) {
	case TYPE_ROOT:
		return 1; // Main icon
	case TYPE_FOLDER:
		return 3; // Folder icon
	case TYPE_ITEM:
		{
			// find default format
			DATA_INFO* highest_di = format_get_priority_highest(di);
			if (highest_di == NULL)
				return 5; // Default icon
			int icon_idx = format_get_index(highest_di->format_name, highest_di->format_name_hash);
			if (icon_idx >= 0 && icon_idx < option.format_cnt) {
				return 6 + icon_idx;  // Format-specific icons start at index 6
			}
		}
		return 5; // Default icon

	default:
		return 5; // Default icon
	}
}

/*
 * listbox_add_item_with_icon
 * Add a listbox item with icon and data
 */
static int listbox_add_item_with_icon(HWND hListBox, const TCHAR* text, DATA_INFO* di)
{
	LISTBOX_ITEM_DATA* item_data = (LISTBOX_ITEM_DATA*)malloc(sizeof(LISTBOX_ITEM_DATA));
	if (item_data == NULL)
		return -1;

	BOOL bIconFound = di ? format_get_menu_icon(di) : FALSE;
	item_data->data_info = di;
	item_data->icon_index = get_icon_index_for_data(di);
	StringCchCopy(item_data->text, BUF_SIZE, text);

	int idx = SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)item_data->text);
	if (idx >= 0) {
		SendMessage(hListBox, LB_SETITEMDATA, (WPARAM)idx, (LPARAM)item_data);
	} else {
		free(item_data);
	}

	return idx;
}

void MyPopupPopulateHandler(const TCHAR* editText, HWND hwndListBox, void* pUserData);
void MyPopupSelectionHandler(const PopupItemData* pSelectedItem, void* pUserData);

// The callback can be used in two ways:
// - Either it uses the tooltip handling of the hosting application (hWndTooltip != NULL)
//   -> then it must always return NULL.
// - Or it uses internal tooltip handling of dynamic_popup control 
//   -> then it should return the appropriate tooltip text for pSelectItem, 
//      respectively an empty string or NULL to hide the tooltip window.
// For CLCL we return NULL ands thus inherit all the nice features of CLCL's tooltips.
TCHAR* MyPopupTooltipHandler(POINT pt, const PopupItemData* pSelectedItem, void* pUserData)
{
	if (!pSelectedItem) {
		if (hWndTooltip)
			tooltip_hide(hWndTooltip);
		return NULL;
	}

	DATA_INFO* di = (DATA_INFO*)pSelectedItem->itemData;
	if (!di) {
		if (hWndTooltip)
			tooltip_hide(hWndTooltip);
		return NULL;
	}

	if (hWndTooltip) {
		// In CLCL menu items selected by mouse require 0 for x and y coordinates.
		POINT cp;
		GetCursorPos(&cp);
		// If pt is identical to the current mouse position, it's a mouse selection 
		// and we set pt to { 0, 0 }, so that CLCL handles tooltip positioning.
		if (cp.x == pt.x && cp.y == pt.y)
			pt.x = pt.y = 0;
	}

	// If it's of  TYPE_ITEM we can get additional information.
	DATA_INFO* highest_di = NULL;
	if (di->type == TYPE_ITEM && (highest_di = format_get_priority_highest(di)) != NULL) {
		TCHAR* buf = format_get_tooltip_text(highest_di);
		if (hWndTooltip) {
			// Global tooltip window is set and the hosting application CLCL handles the tooltips.
			tooltip_show(hWndTooltip, buf ? buf : TEXT(""), pt.x, pt.y, 0);
			mem_free(&buf);
			return NULL;
		}
		else {
			// Global tooltip windows is not set, we return a text buffer.
			// DynamicPopupMenu will do the tooltip handling.
			static TCHAR tooltip_buffer[512];
			StringCchCopy(tooltip_buffer, 512, buf ? buf : TEXT(""));
			mem_free(&buf);
			return tooltip_buffer;
		}
	}

	// If no special tooltip text is available, use the title.
	if (hWndTooltip) {
		tooltip_show(hWndTooltip, pSelectedItem->pszText, pt.x, pt.y, 0);
		return NULL;
	}
	else {
		return pSelectedItem->pszText;
	}
}

UINT_PTR quicksearch(HWND hWnd, POINT pt, HWND hToolTip)
{
	hWndClcl = hWnd;
	hWndTooltip = hToolTip;
	int icon_size = option.menu_icon_size ? Scale(option.menu_icon_size) : Scale(16);
	int icon_margin = option.menu_icon_margin ? Scale(option.menu_icon_margin) : Scale(2);
	int text_margin = option.menu_text_margin_left ? Scale(option.menu_text_margin_left) : Scale(4);
	int menu_width = option.menu_max_width ? Scale(option.menu_max_width) : Scale(200);
	int font_size = option.menu_font_size;

	NONCLIENTMETRICS ncMetrics;
	HFONT menu_font = NULL;
	if (option.menu_font_name && option.menu_font_size && option.menu_font_charset)
		menu_font = font_create(option.menu_font_name, option.menu_font_size, option.menu_font_charset, option.menu_font_weight, (option.menu_font_italic == 0) ? FALSE : TRUE, FALSE);
	else if (GetNonClientMetricsDpi(&ncMetrics) != FALSE)
		menu_font = CreateFontIndirect(&ncMetrics.lfMenuFont);

	StringCbPrintf(ini_path, BUF_SIZE, TEXT("%s\\%s"), work_path, USER_INI);
	static unsigned int max_visible_items = 0;
	if (max_visible_items == 0)
		max_visible_items = (unsigned int)profile_get_int(TEXT("quicksearch"), TEXT("max_visible_items"), 10, ini_path);
	HIMAGELIST hImageList = create_imagelist(hInst);
	// Step 1: Create the popup window
	HWND hwndPopup = CreateDynamicPopupMenu(hWnd, pt.x, pt.y, menu_width);
	if (!hwndPopup) {
		return 0;
	}

	// Step 2: Configure the popup using setter functions
	// Callbacks:
	SetPopulateCallback(hwndPopup, MyPopupPopulateHandler);
	SetTooltipCallback(hwndPopup, MyPopupTooltipHandler);

	// Layout:
	SetImageList(hwndPopup, hImageList, icon_size);
	SetIconMargin(hwndPopup, icon_margin);
	SetTextMargin(hwndPopup, text_margin);
	SetMaxVisibleItems(hwndPopup, max_visible_items);
	SetUserData(hwndPopup, (void*)NULL);
	if (menu_font)
		SetMenuFont(hwndPopup, menu_font);

	// Step 3: Track the popup modally and get the result
	UINT_PTR itemData = TrackDynamicPopup(hwndPopup);

	// Step 4: Clean-up and return the result
	if (hImageList)
		ImageList_Destroy(hImageList);
	if (menu_font)
		DeleteObject(menu_font);

	return itemData; // Return the selected item's data or 0 if no selection is made
}

// 1. THIS CALLBACK POPULATES THE LISTBOX CONTROL DYNAMICALLY ACCORDING TO editText.
void MyPopupPopulateHandler(const TCHAR* editText, HWND hwndListBox, void* pUserData) 
{
	// Add matching items to the listbox
	static int max_cnt = 0;
	if (max_cnt == 0)
		max_cnt = profile_get_int(TEXT("quicksearch"), TEXT("max_item_count"), 30, ini_path);

	// Add from history
	int item_count = listbox_add_matches(hwndListBox, &history_data, editText, max_cnt);
	if (item_count < max_cnt) {
		// Add from templates
		item_count = listbox_add_matches(hwndListBox, &regist_data, editText, max_cnt);
	}
}

// 2. THIS CALLBACK PROCESSES THE FINAL SELECTION
void MyPopupSelectionHandler(const PopupItemData* pSelectedItem, void* pUserData) 
{
	if (pSelectedItem) {
		DATA_INFO* di = (DATA_INFO*) pSelectedItem->itemData;
		if (di) {
			LRESULT res = SendMessage(hWndClcl, WM_ITEM_TO_CLIPBOARD, 0, (LPARAM)di);
		}
	}
}
