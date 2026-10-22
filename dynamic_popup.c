/*
 * CLCL
 *
 * dynamic_popup.c
 *
 * Copyright (C) 2026 by Wilf Zimmermann. MIT License.
 *		https://linguversa.de/clcl
 *		https://github.com/wilfz/clcl
 */

/* Include Files */
#include "dynamic_popup.h"
#include <tchar.h>
#include <stdio.h>
#include <windowsx.h>

#define SUBCLASS_ID_POPUP 202
#define ITEM_HEIGHT 20
#define MAX_VISIBLE_ITEMS 10
#define TOOLTIP_DELAY_MS 800
#ifndef IDC_TOOLTIP_WINDOW
#define IDC_TOOLTIP_WINDOW 51004
#endif

// Data after select or losing focus
typedef struct {
    UINT_PTR selectedItemData;
    BOOL selectionMade;
} ModalPopupState;

// State control for the dynamic popup
typedef struct {
    HWND hwndFrame;           // The new invisible container window (has the shadow!)
    HWND hwndEdit;            // The actual edit control (as child)
    HWND hwndList;            // The listbox control (as child)
    HWND hwndTooltip;         // Tooltip window
    HWND hwndOwner;
    HFONT hFont;
    OnPopupPopulateCallback populateCallback;
    OnPopupSelectCallback selectCallback;
    OnPopupTooltipCallback tooltipCallback;  // New callback for multiline tooltips
    unsigned int max_visible_items;
	ModalPopupState* pModalState;
    void* pUserData;
    BOOL isClosing;
    BOOL listAboveEdit;
    RECT monitorRect;
    HIMAGELIST hImageList;    // Stored ImageList for drawing icons
    int icon_size;
    int icon_margin;
    int text_margin;
    int item_height;
    int lastHoveredItem;      // Tracking of last displayed tooltip element
    UINT_PTR uiTooltipTimer;  // Timer ID for tooltip delay
    TCHAR* currentTooltipText; // Current tooltip text for WM_PAINT
} DynamicPopupData;

// Forward declarations
LRESULT CALLBACK PopupFrameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK DynamicEditSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
LRESULT CALLBACK DynamicListSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
LRESULT CALLBACK TooltipWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Helper function: Hides the tooltip window
static void HideTooltip(DynamicPopupData* pData)
{
    if (pData && pData->hwndTooltip) {
        ShowWindow(pData->hwndTooltip, SW_HIDE);
        if (pData->uiTooltipTimer) {
            KillTimer(pData->hwndList, pData->uiTooltipTimer);
            pData->uiTooltipTimer = 0;
        }
        pData->lastHoveredItem = -1;
    }
}

// Helper function: Displays a multiline tooltip
static void ShowTooltipForItem(DynamicPopupData* pData, int itemIndex, POINT ptMouse)
{
    if (!pData || !pData->hwndList || !pData->tooltipCallback) {
        return;
    }

    if (itemIndex == LB_ERR) {
        pData->tooltipCallback(ptMouse, NULL, pData->pUserData);
        HideTooltip(pData);
        return;
    }

    // End existing timer
    if (pData->uiTooltipTimer) {
        KillTimer(pData->hwndList, pData->uiTooltipTimer);
        pData->uiTooltipTimer = 0;
    }

    // Don't show again if over the same element
    if (pData->lastHoveredItem == itemIndex) {
        return;
    }

    pData->lastHoveredItem = itemIndex;

    PopupItemData* pItem = (PopupItemData*)SendMessage(pData->hwndList, LB_GETITEMDATA, itemIndex, 0);
    if (!pItem || pItem == (PopupItemData*)LB_ERR) {
        pData->tooltipCallback(ptMouse, NULL, pData->pUserData);
        HideTooltip(pData);
        return;
    }

    // Get tooltip text from callback, respectively let the host application show the tooltip
    TCHAR* tooltipText = pData->tooltipCallback(ptMouse, pItem, pData->pUserData);
    if (!tooltipText || *tooltipText == TEXT('\0')) {
        HideTooltip(pData);
        return;
    }

    // Either the hosting application handles tooltips itself, 
    // then the callback returned NULL and we are already done,
    // or it returned us a tooltip text, 
    // then we have to do the tooltip handling right here.

    // Create tooltip window if not present
    if (!pData->hwndTooltip) {
        const TCHAR* szTooltipClass = TEXT("HCP_TooltipClass");
        static BOOL tooltipClassRegistered = FALSE;

        if (!tooltipClassRegistered) {
            WNDCLASS wc = { 0 };
            wc.lpfnWndProc = TooltipWindowProc;
            wc.hInstance = (HINSTANCE)GetWindowLongPtr(pData->hwndList, GWLP_HINSTANCE);
            wc.hbrBackground = (HBRUSH)(COLOR_INFOBK + 1);
            wc.lpszClassName = szTooltipClass;
            wc.style = CS_SAVEBITS;
            if (RegisterClass(&wc)) {
                tooltipClassRegistered = TRUE;
            }
        }

        HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(pData->hwndList, GWLP_HINSTANCE);
        pData->hwndTooltip = CreateWindowEx(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            szTooltipClass, NULL,
            WS_POPUP | WS_BORDER,
            ptMouse.x + 15, ptMouse.y + 15, 300, 100,
            NULL, NULL, hInstance, NULL
        );

        if (!pData->hwndTooltip) {
            return;
        }

        // Store pointer to pData in the window
        SetWindowLongPtr(pData->hwndTooltip, GWLP_USERDATA, (LONG_PTR)pData);

        HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        SendMessage(pData->hwndTooltip, WM_SETFONT, (WPARAM)hFont, FALSE);
    }

    // Store tooltip text in pData
    if (pData->currentTooltipText) {
        HeapFree(GetProcessHeap(), 0, pData->currentTooltipText);
    }
    size_t len = _tcslen(tooltipText) + 1;
    pData->currentTooltipText = (TCHAR*)HeapAlloc(GetProcessHeap(), 0, len * sizeof(TCHAR));
    if (pData->currentTooltipText) {
        _tcscpy_s(pData->currentTooltipText, len, tooltipText);
    }

    // Calculate tooltip size
    HDC hdc = GetDC(pData->hwndTooltip);
    HFONT hFont = (HFONT)SendMessage(pData->hwndTooltip, WM_GETFONT, 0, 0);
    HFONT oldFont = hFont ? (HFONT)SelectObject(hdc, hFont) : NULL;

    if (!pData->currentTooltipText) {
        ReleaseDC(pData->hwndTooltip, hdc);
        return;
	}
    RECT rcText = { 0, 0, 280, 1000 };
    DrawText(hdc, pData->currentTooltipText, -1, &rcText, DT_CALCRECT | DT_WORDBREAK | DT_LEFT);

    if (oldFont) SelectObject(hdc, oldFont);
    ReleaseDC(pData->hwndTooltip, hdc);

    int tooltipWidth = rcText.right - rcText.left + 10;
    int tooltipHeight = rcText.bottom - rcText.top + 10;

    // Position and show tooltip
    SetWindowPos(pData->hwndTooltip, HWND_TOPMOST,
        ptMouse.x + 15, ptMouse.y + 15,
        tooltipWidth, tooltipHeight,
        SWP_NOACTIVATE | SWP_SHOWWINDOW);
 
    // Force repaint
    InvalidateRect(pData->hwndTooltip, NULL, TRUE);
    UpdateWindow(pData->hwndTooltip);
}

// Custom window procedure for tooltip
LRESULT CALLBACK TooltipWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DynamicPopupData* pData = (DynamicPopupData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (uMsg) {
    case WM_PAINT: {
        PAINTSTRUCT ps = { 0 };
        HDC hdc = BeginPaint(hWnd, &ps);

        if (pData && pData->currentTooltipText) {
            RECT rcClient;
            GetClientRect(hWnd, &rcClient);

            // Fill background
            HBRUSH hBg = GetSysColorBrush(COLOR_INFOBK);
            FillRect(hdc, &rcClient, hBg);

            // Draw text
            HFONT hFont = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
            HFONT oldFont = hFont ? (HFONT)SelectObject(hdc, hFont) : NULL;

            COLORREF oldTextColor = SetTextColor(hdc, GetSysColor(COLOR_INFOTEXT));
            int oldBkMode = SetBkMode(hdc, TRANSPARENT);

            RECT rcText = rcClient;
            rcText.left += 5;
            rcText.top += 5;
            rcText.right -= 5;
            rcText.bottom -= 5;

            DrawText(hdc, pData->currentTooltipText, -1, &rcText, DT_WORDBREAK | DT_LEFT);

            SetBkMode(hdc, oldBkMode);
            SetTextColor(hdc, oldTextColor);
            if (oldFont) SelectObject(hdc, oldFont);
        }

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        if (pData && pData->currentTooltipText) {
            HeapFree(GetProcessHeap(), 0, pData->currentTooltipText);
            pData->currentTooltipText = NULL;
        }
        break;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// Helper function: Frees memory of all listbox entries
static void ClearListBoxItems(HWND hwndList) 
{
    int count = (int)SendMessage(hwndList, LB_GETCOUNT, 0, 0);
    if (count == LB_ERR) return;

    for (int i = 0; i < count; i++) {
        PopupItemData* pItem = (PopupItemData*)SendMessage(hwndList, LB_GETITEMDATA, i, 0);
        if (pItem && pItem != (PopupItemData*)LB_ERR) {
            if (pItem->pszText) HeapFree(GetProcessHeap(), 0, pItem->pszText);
            HeapFree(GetProcessHeap(), 0, pItem);
        }
    }
    SendMessage(hwndList, LB_RESETCONTENT, 0, 0);
}

void PopupAddString(HWND hwndListBox, const TCHAR* pszText, int iIconIndex, UINT_PTR itemData) 
{
    PopupItemData* pItem = (PopupItemData*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PopupItemData));
    if (!pItem) return;

    if (pszText) {
        size_t len = _tcslen(pszText) + 1;
        pItem->pszText = (TCHAR*)HeapAlloc(GetProcessHeap(), 0, len * sizeof(TCHAR));
        if (pItem->pszText) {
            _tcscpy_s(pItem->pszText, len, pszText);
        }
    }
    pItem->iIconIndex = iIconIndex;
    pItem->itemData = itemData;

    // Since LBS_OWNERDRAWFIXED is active, we pass the pointer as string parameter.
    // Windows automatically stores this as ItemData since LBS_HASSTRINGS is NOT set.
    SendMessage(hwndListBox, LB_ADDSTRING, 0, (LPARAM)pItem);
}

static void DestroyPopupLayout(DynamicPopupData* pData) 
{
    if (!pData || pData->isClosing) return;
    pData->isClosing = TRUE;

    HWND hFrame = pData->hwndFrame;
    HWND hEdit = pData->hwndEdit;
    HWND hList = pData->hwndList;
    HWND hTooltip = pData->hwndTooltip;

    // Hide internal tooltip if active.
    HideTooltip(pData);
    if (hTooltip == NULL && pData->tooltipCallback) {
        POINT pt = { 0, 0 };
        // If host application handles tooltip, this call will hide it.
        pData->tooltipCallback(pt, NULL, pData->pUserData);
    }

    RemoveWindowSubclass(hEdit, DynamicEditSubclass, SUBCLASS_ID_POPUP);
    RemoveWindowSubclass(hList, DynamicListSubclass, SUBCLASS_ID_POPUP);

    ClearListBoxItems(hList);

    if (hTooltip) DestroyWindow(hTooltip);
    ShowWindow(hFrame, SW_HIDE);
    DestroyWindow(hFrame);

    if (pData->currentTooltipText) {
        HeapFree(GetProcessHeap(), 0, pData->currentTooltipText);
    }

    HeapFree(GetProcessHeap(), 0, pData);
}

static void RepositionListbox(DynamicPopupData* pData, int editHeight) 
{
	if (!pData || !pData->hwndFrame || !pData->hwndEdit || !pData->hwndList)
        return;

    int itemCount = (int)SendMessage(pData->hwndList, LB_GETCOUNT, 0, 0);
    int visibleItems = itemCount > MAX_VISIBLE_ITEMS ? MAX_VISIBLE_ITEMS : itemCount;
    int listHeight = 0;
	int maxListHeight = ITEM_HEIGHT * MAX_VISIBLE_ITEMS;
    if (visibleItems > 0 && pData && pData->item_height > 0) {
        listHeight = pData->item_height * visibleItems;
		maxListHeight = pData->item_height * MAX_VISIBLE_ITEMS;
    }
    
    // Get screen coordinates of the frame window
    RECT frameRect;
    GetWindowRect(pData->hwndFrame, &frameRect);
    int width = frameRect.right - frameRect.left + 1;
    
    RECT editRect;
    GetWindowRect(pData->hwndEdit, &editRect);

    // Get monitor info to check available space
    //HMONITOR hMonitor = MonitorFromRect(&frameRect, MONITOR_DEFAULTTONEAREST);
    //MONITORINFO miInfo = { 0 };
    //miInfo.cbSize = sizeof(MONITORINFO);
    //GetMonitorInfo(hMonitor, &miInfo);
    int monitorBottom = pData->monitorRect.bottom;

    if (frameRect.left < pData->monitorRect.left) {
        // Move the frame window to the right to fit within the monitor
		frameRect.left = pData->monitorRect.left;
		frameRect.right = pData->monitorRect.left + width + 1;
    } 
    else if (frameRect.right > pData->monitorRect.right) {
		// Move the frame window to the left to fit within the monitor
        frameRect.left = pData->monitorRect.right - width  - 1;
		frameRect.right = pData->monitorRect.right;
	}
    if (editRect.top < pData->monitorRect.top) {
        // Move the frame window down to fit within the monitor
        frameRect.top = pData->monitorRect.top;
    } else if (editRect.bottom > pData->monitorRect.bottom) {
        // Move the frame window up to fit within the monitor
        frameRect.bottom = pData->monitorRect.bottom;
	}  
    
    // Adjust frame window size
    int totalHeight = editHeight + listHeight;

    // Check if listbox fits below the edit control
    BOOL fitsBelowEdit = (monitorBottom >= editRect.top + editHeight + maxListHeight);
    
    // If doesn't fit below and there's more space above, position above
    if (!fitsBelowEdit) {
        // Position listbox above the edit control
        SetWindowPos(pData->hwndList, NULL, 0, 0, width, listHeight, SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(pData->hwndEdit, NULL, 0, listHeight + 1, width, editHeight, SWP_NOZORDER);
        pData->listAboveEdit = TRUE;
    } else {
        // Position listbox below the edit control (default)
        SetWindowPos(pData->hwndEdit, NULL, 0, 0, width, editHeight, SWP_NOZORDER);
        SetWindowPos(pData->hwndList, NULL, 0, editHeight + 1, width, listHeight, SWP_NOZORDER | SWP_NOACTIVATE);
        pData->listAboveEdit = FALSE;
    }
    
    int frameY = pData->listAboveEdit ? frameRect.bottom - totalHeight : frameRect.top;
    SetWindowPos(pData->hwndFrame, NULL, frameRect.left, frameY, width, totalHeight, SWP_NOZORDER | SWP_NOACTIVATE);
}

static void UpdateListContent(DynamicPopupData* pData) 
{
    int len = GetWindowTextLength(pData->hwndEdit);
    TCHAR* text = (TCHAR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (len + 1) * sizeof(TCHAR));
    if (text) {
        GetWindowText(pData->hwndEdit, text, len + 1);

        // Delete old content and its heap objects before populating
        ClearListBoxItems(pData->hwndList);
        HideTooltip(pData);

        if (pData->populateCallback) {
            pData->populateCallback(text, pData->hwndList, pData->pUserData);
        }
        HeapFree(GetProcessHeap(), 0, text);
    }

    if (SendMessage(pData->hwndList, LB_GETCOUNT, 0, 0) > 0) {
        SendMessage(pData->hwndList, LB_SETCURSEL, 0, 0);
    }
    
    // Reposition listbox based on available space
    RepositionListbox(pData, 26);
}

HWND CreateDynamicPopupMenu(HWND hwndOwner, int x, int y, int width)
 {
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(hwndOwner, GWLP_HINSTANCE);
    const TCHAR* szClassName = TEXT("HCP_PopupMenuFrameClass");

    // Register own, clean window class (ONLY ONCE)
    static BOOL classRegistered = FALSE;
    if (!classRegistered) {
        WNDCLASS wc = { 0 };
        wc.lpfnWndProc = PopupFrameWndProc;
        wc.hInstance = hInstance;
        wc.hbrBackground = (HBRUSH)(COLOR_MENU + 1); // Menu background color
        wc.lpszClassName = szClassName;
        wc.style = CS_DROPSHADOW;

        if (RegisterClass(&wc)) {
            classRegistered = TRUE;
        }
    }

    // Get monitor info to check available space
    POINT pt = { x, y };
    HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    MONITORINFO miInfo = { 0 };
    miInfo.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(hMonitor, &miInfo);

    int editHeight = 26;
    int initialListHeight = 0; //ITEM_HEIGHT * 3;
    int totalHeight = editHeight + initialListHeight;

    // 1. Create the parent POPUP window
    HWND hwndFrame = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        szClassName, NULL,
        WS_POPUP | WS_BORDER,
        x, y, width, totalHeight,
        hwndOwner, NULL, hInstance, NULL
    );

    if (!hwndFrame) return NULL;

    // Allocate memory for data
    DynamicPopupData* pData = (DynamicPopupData*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DynamicPopupData));
    if (!pData) {
        DestroyWindow(hwndFrame);
        return NULL;
    }

    // 2. Create the edit control as CHILD in the frame
    HWND hwndEdit = CreateWindowEx(
        0, TEXT("EDIT"), TEXT(""),
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        0, 0, width, editHeight, 
        hwndFrame, (HMENU)IDC_DYNAMIC_EDIT, hInstance, NULL
    );

    // IMPORTANT: LBS_OWNERDRAWFIXED without LBS_HASSTRINGS. This makes the "string" the pointer address.
    HWND hwndList = CreateWindowEx(
        0, TEXT("LISTBOX"), NULL,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_OWNERDRAWFIXED,
        0, editHeight, width, initialListHeight,
        hwndFrame, (HMENU)IDC_DYNAMIC_LISTBOX, hInstance, NULL
    );

    if (!hwndEdit || !hwndList) {
        DestroyWindow(hwndFrame);
        HeapFree(GetProcessHeap(), 0, pData);
        return NULL;
    }

    // Store pointer to data structure in frame window
    SetWindowLongPtr(hwndFrame, GWLP_USERDATA, (LONG_PTR)pData);

    pData->hwndFrame = hwndFrame;
    pData->hwndEdit = hwndEdit;
    pData->hwndList = hwndList;
    pData->hwndTooltip = NULL;
    pData->hwndOwner = hwndOwner;
    pData->hFont = (HFONT)NULL;
    pData->isClosing = FALSE;
    pData->listAboveEdit = FALSE;
    pData->monitorRect = miInfo.rcWork;
    pData->populateCallback = NULL;
    pData->selectCallback = NULL;
    pData->tooltipCallback = NULL;
	pData->pModalState = NULL;
    pData->max_visible_items = MAX_VISIBLE_ITEMS;
    pData->hImageList = (HIMAGELIST) NULL;
    pData->icon_size = 0;
    pData->item_height = ITEM_HEIGHT;
    pData->text_margin = 4;
	pData->pUserData = NULL;
    pData->lastHoveredItem = -1;
    pData->uiTooltipTimer = 0;
    pData->currentTooltipText = NULL;

    // Activate subclassing for controls
    SetWindowSubclass(pData->hwndEdit, DynamicEditSubclass, SUBCLASS_ID_POPUP, (DWORD_PTR)pData);
    SetWindowSubclass(pData->hwndList, DynamicListSubclass, SUBCLASS_ID_POPUP, (DWORD_PTR)pData);

    return hwndFrame;
}

void SetPopulateCallback(HWND hwndFrame, OnPopupPopulateCallback populateCallback)
{
    DynamicPopupData* pData = (DynamicPopupData*)GetWindowLongPtr(hwndFrame, GWLP_USERDATA);
    if (pData) {
        pData->populateCallback = populateCallback;
    }
}

void SetSelectCallback(HWND hwndFrame, OnPopupSelectCallback selectCallback)
{
    DynamicPopupData* pData = (DynamicPopupData*)GetWindowLongPtr(hwndFrame, GWLP_USERDATA);
    if (pData) {
        pData->selectCallback = selectCallback;
    }
}

void SetTooltipCallback(HWND hwndFrame, OnPopupTooltipCallback tooltipCallback)
{
    DynamicPopupData* pData = (DynamicPopupData*)GetWindowLongPtr(hwndFrame, GWLP_USERDATA);
    if (pData) {
        pData->tooltipCallback = tooltipCallback;
    }
}

void SetImageList(HWND hwndFrame, HIMAGELIST hImageList, int icon_size)
{
    DynamicPopupData* pData = (DynamicPopupData*)GetWindowLongPtr(hwndFrame, GWLP_USERDATA);
    if (!pData)
        return;

        pData->hImageList = hImageList;
        pData->icon_size = icon_size;

    if (icon_size == 0) {
        int cx = 0, cy = 0;
        if (pData && pData->hImageList && ImageList_GetIconSize(pData->hImageList, &cx, &cy))
            icon_size = (cy > 0) ? cy : 16;
    }

    pData->hImageList = hImageList;
    pData->icon_size = icon_size;
}

void SetIconSize(HWND hwndFrame, int icon_size)
{
    DynamicPopupData* pData = (DynamicPopupData*)GetWindowLongPtr(hwndFrame, GWLP_USERDATA);
    if (pData)
        pData->icon_size = icon_size;
}

void SetIconMargin(HWND hwndFrame, int icon_margin)
{
    DynamicPopupData* pData = (DynamicPopupData*)GetWindowLongPtr(hwndFrame, GWLP_USERDATA);
    if (pData)
        pData->icon_margin = icon_margin;
}

void SetItemHeight(HWND hwndFrame, int item_height) 
{
    DynamicPopupData* pData = (DynamicPopupData*)GetWindowLongPtr(hwndFrame, GWLP_USERDATA);
    if (pData)
        pData->item_height = item_height;
}

void SetTextMargin(HWND hwndFrame, int text_margin)
{
    DynamicPopupData* pData = (DynamicPopupData*)GetWindowLongPtr(hwndFrame, GWLP_USERDATA);
    if (pData)
        pData->text_margin = text_margin;
}

void SetMaxVisibleItems(HWND hwndFrame, unsigned int max_visible_items)
{
    DynamicPopupData* pData = (DynamicPopupData*)GetWindowLongPtr(hwndFrame, GWLP_USERDATA);
    if (pData)
        pData->max_visible_items = max_visible_items;
}

void SetMenuFont(HWND hwndFrame, HFONT hFont)
{
    DynamicPopupData* pData = (DynamicPopupData*)GetWindowLongPtr(hwndFrame, GWLP_USERDATA);
    if (pData) {
        pData->hFont = hFont;
    }
}

void SetUserData(HWND hwndFrame, void* pUserData) 
{
    DynamicPopupData* pData = (DynamicPopupData*)GetWindowLongPtr(hwndFrame, GWLP_USERDATA);
    if (pData) {
        pData->pUserData = pUserData;
    }
}

void ActivateDynamicPopup(HWND hwndFrame) 
{
    DynamicPopupData* pData = (DynamicPopupData*)GetWindowLongPtr(hwndFrame, GWLP_USERDATA);
	if (pData == NULL)
        return;

    // Assign modern system font
    HFONT hFont = pData->hFont ? pData->hFont : (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    SendMessage(pData->hwndEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(pData->hwndList, WM_SETFONT, (WPARAM)hFont, TRUE);

    if (pData->item_height < pData->icon_margin + pData->icon_size + pData->icon_margin) {
        // Height of listbox item based on icon size
        pData->item_height = pData->icon_margin + pData->icon_size + pData->icon_margin;
        }
    SendMessage(pData->hwndList, LB_SETITEMHEIGHT, 0, (LPARAM)pData->item_height);

    // Populate initially
    UpdateListContent(pData);

    ShowWindow(pData->hwndFrame, SW_SHOW);
    SetFocus(pData->hwndEdit);

    // Select the first item
    int sel = 0;
    LRESULT ret = SendMessage(pData->hwndList, LB_SETCURSEL, (WPARAM)sel, (LPARAM)0);
    if (ret >= 0 && pData->tooltipCallback != NULL) {
        // Show tooltip for the selected item
        RECT itemrect;
        ret = SendMessage(pData->hwndList, LB_GETITEMRECT, (WPARAM)sel, (LPARAM)&itemrect);
        if (ret >= 0) {
            POINT pt;
            pt.x = (itemrect.left + itemrect.right) / 2;
            pt.y = itemrect.bottom + 1;
            if (ClientToScreen(pData->hwndList, &pt))
                ShowTooltipForItem(pData, sel, pt);
        }
    }

    return;
}

static void SetModalState(DynamicPopupData* pData, const PopupItemData* pSelectedItem)
{
    if (!pData || !pData->pModalState) return;
    if (pSelectedItem) {
        pData->pModalState->selectedItemData = pSelectedItem->itemData;
        pData->pModalState->selectionMade = TRUE;
    }
}

// Window procedure for the outer frame window
LRESULT CALLBACK PopupFrameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    DynamicPopupData* pData = (DynamicPopupData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (uMsg) {
        case WM_COMMAND:
            if (pData && pData->hwndEdit
                && LOWORD(wParam) == IDC_DYNAMIC_EDIT
                && HIWORD(wParam) == EN_CHANGE)
            {
                UpdateListContent(pData);
            }
            break;

        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* pdis = (DRAWITEMSTRUCT*)lParam;
            if (pdis->CtlID == IDC_DYNAMIC_LISTBOX && pdis->itemID != -1) {
                PopupItemData* pItem = (PopupItemData*)pdis->itemData;
                if (!pItem) return TRUE;

                HDC hdc = pdis->hDC;
                RECT rc = pdis->rcItem;
                BOOL isSelected = (pdis->itemState & ODS_SELECTED);

                // 1. Draw background (selected vs. standard)
                HBRUSH hBg = GetSysColorBrush(isSelected ? COLOR_HIGHLIGHT : COLOR_MENU);
                FillRect(hdc, &rc, hBg);

                // 2. Draw icon (if ImageList and valid index present)
                if (pData && pData->hImageList && pItem->iIconIndex >= 0 && pData->icon_size > 0) {
                    // Center vertically
                    int cy = rc.top + (rc.bottom - rc.top - pData->icon_size) / 2;
                    ImageList_Draw(pData->hImageList, pItem->iIconIndex, hdc, rc.left + pData->icon_margin, cy, ILD_TRANSPARENT);
                }
                int iconOffset = (pData && pData->icon_size > 0) ? pData->icon_margin + pData->icon_size + pData->icon_margin : 0;

                // 3. Draw text
                COLORREF oldTextCol = SetTextColor(hdc, GetSysColor(isSelected ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT));
                int oldBkMode = SetBkMode(hdc, TRANSPARENT);

                RECT rcText = rc;
                rcText.left += iconOffset + (pData ? pData->text_margin : 2);

                HFONT hFont = (HFONT)SendMessage(pdis->hwndItem, WM_GETFONT, 0, 0);
                HFONT oldFont = hFont ? (HFONT)SelectObject(hdc, hFont) : NULL;

                DrawText(hdc, pItem->pszText, -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                if (oldFont) SelectObject(hdc, oldFont);
                SetBkMode(hdc, oldBkMode);
                SetTextColor(hdc, oldTextCol);

                // 4. Suppress focus rectangle (menus don't have dashed borders)
                return TRUE;
            }
            break;
        }

        case WM_SETFOCUS:
            if (pData) SetFocus(pData->hwndEdit);
            return 0;

        case WM_KILLFOCUS: {
            HWND hwndNewFocus = (HWND)wParam;
            if (pData && hwndNewFocus != pData->hwndFrame &&
                hwndNewFocus != pData->hwndEdit && hwndNewFocus != pData->hwndList) 
            {
                DestroyPopupLayout(pData);
            }
            return 0;
        }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// Subclass the edit control
LRESULT CALLBACK DynamicEditSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    DynamicPopupData* pData = (DynamicPopupData*)dwRefData;

    switch (uMsg) {
        case WM_COMMAND:
            if (HIWORD(wParam) == EN_CHANGE) {
                UpdateListContent(pData);
            }
            break;

        case WM_KILLFOCUS: {
            HWND hwndNewFocus = (HWND)wParam;
            if (hwndNewFocus != pData->hwndFrame && hwndNewFocus != pData->hwndList && hwndNewFocus != hWnd) {
                DestroyPopupLayout(pData);
            }
            break;
        }

        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                DestroyPopupLayout(pData);
                return 0;
            }
            if (wParam == VK_DOWN || wParam == VK_UP) {
                HideTooltip(pData);
                SendMessage(pData->hwndList, WM_KEYDOWN, wParam, lParam);
                int sel = (int)SendMessage(pData->hwndList, LB_GETCURSEL, 0, 0);
                if (pData->tooltipCallback == NULL || sel < 0)
                    return 0;

                // Show tooltip for the new selected item
                RECT itemrect;
                LRESULT ret = SendMessage(pData->hwndList, LB_GETITEMRECT, (WPARAM)sel, (LPARAM)&itemrect);
                if (ret >= 0) {
                    POINT pt;
                    pt.x = (itemrect.left + itemrect.right) / 2;
                    pt.y = itemrect.bottom + 1;
                    if (ClientToScreen(pData->hwndList,&pt))
                         ShowTooltipForItem(pData, sel, pt);
                }

                return 0;
            }
            if (wParam == VK_RETURN) {
                int index = (int)SendMessage(pData->hwndList, LB_GETCURSEL, 0, 0);
                PopupItemData* pItem = NULL;
                if (index != LB_ERR) {
                    pItem = (PopupItemData*)SendMessage(pData->hwndList, LB_GETITEMDATA, index, 0);
                }
                SetModalState(pData, pItem);
                if (pItem && pData->selectCallback) {
                    pData->selectCallback(pItem, pData->pUserData);
                }
                DestroyPopupLayout(pData);
                return 0;
            }
            break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// Subclass the listbox
LRESULT CALLBACK DynamicListSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    DynamicPopupData* pData = (DynamicPopupData*)dwRefData;

    switch (uMsg) {
    case WM_MOUSEMOVE: {
        // Apparently this message comes in even when mouse has not been moved.
        // Therefore we compare with the previous position, to avoid false alarms. 
        static POINT oldpos = { 0, 0 };
        POINT pt;
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
        ClientToScreen(hWnd, &pt);
        if (pt.x == oldpos.x && pt.y == oldpos.y)
            break;
        // Memorize the position
        oldpos = pt;
        int sel = (int)SendMessage(hWnd, LB_GETCURSEL, 0, 0);
        int index = (int)SendMessage(hWnd, LB_ITEMFROMPOINT, 0, lParam);
        // If mouse has really been moved and a different item is under the mouse cursor, select that item.
        if (index != LB_ERR && index != sel) {
            SendMessage(hWnd, LB_SETCURSEL, index, 0);
            // Show tooltip for the new selected item
            if (pData && pData->tooltipCallback && index != LB_ERR) {
                ShowTooltipForItem(pData, index, pt);
            }
        }
        break;
    }

    case WM_MOUSELEAVE: {
        HideTooltip(pData);
        break;
    }

    case WM_LBUTTONUP: {
        LRESULT res = DefSubclassProc(hWnd, uMsg, wParam, lParam);
        int index = (int)SendMessage(hWnd, LB_GETCURSEL, 0, 0);
        PopupItemData* pItem = NULL;
        if (index != LB_ERR) {
            pItem = (PopupItemData*)SendMessage(pData->hwndList, LB_GETITEMDATA, index, 0);
        }
        SetModalState(pData, pItem);
        if (pItem && pData->selectCallback) {
            pData->selectCallback(pItem, pData->pUserData);
        }
        DestroyPopupLayout(pData);
        return res;
    }

    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

UINT_PTR TrackDynamicPopup(HWND hwndFrame)
{
    if (!IsWindow(hwndFrame)) {
        return 0;
    }

    DynamicPopupData* pData = (DynamicPopupData*)GetWindowLongPtr(hwndFrame, GWLP_USERDATA);
    if (!pData) {
        return 0;
    }

    // Local variable stays alive in this scope, even after pUserData is destroyed
    ModalPopupState modalState = { 0, FALSE };
    modalState.selectedItemData = 0;
    modalState.selectionMade = FALSE;

    // Pointer to local variable modalState
	pData->pModalState = &modalState;

    ActivateDynamicPopup(hwndFrame);

    MSG msg = { 0 };
    BOOL bContinue = TRUE;

    while (bContinue && GetMessage(&msg, NULL, 0, 0)) {
        if (!IsWindow(hwndFrame)) {
            // The popup window has been destroyed, exit the loop
            bContinue = FALSE;
            break;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (modalState.selectionMade) {
            bContinue = FALSE;
            break;
        }
    }

    if (IsWindow(hwndFrame)) {
        DestroyWindow(hwndFrame);
    }

    UINT_PTR result = modalState.selectedItemData;
        
    return result;
}
