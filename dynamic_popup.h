/*
 * CLCL
 *
 * dynamic_popup.h
 *
 * Copyright (C) 2026 by Wilf Zimmermann. MIT License.
 *		https://linguversa.de/clcl
 *		https://github.com/wilfz/clcl
 */

#pragma once

#ifndef DYNAMIC_POPUP_H
#define DYNAMIC_POPUP_H

#include <windows.h>
#include <commctrl.h>

// Unique IDs for controls
#define IDC_DYNAMIC_EDIT    51001
#define IDC_DYNAMIC_LISTBOX 51002

// Structure for each element in the owner-draw listbox
typedef struct {
    TCHAR* pszText;       // Text to be displayed
    int iIconIndex;       // Index of the icon in an ImageList (or -1 for no icon)
    UINT_PTR itemData;    // Application-specific data (e.g., ID or pointer)
} PopupItemData;

// Callback to display tooltips when hovering
// Return: Pointer to tooltip text (not freed by the caller)
// or NULL if no tooltip
typedef TCHAR* (*OnPopupTooltipCallback)(POINT pt, const PopupItemData* pItem, void* pUserData);

// Callback for selecting an element
typedef void (*OnPopupSelectCallback)(const PopupItemData* pSelectedItem, void* pUserData);

// Callback to dynamically populate the listbox based on edit text
typedef void (*OnPopupPopulateCallback)(const TCHAR* editText, HWND hwndListBox, void* pUserData);

/**
 * Helper function: Adds an entry to the owner-draw listbox.
 * Must be called from within the OnPopupPopulateCallback.
 */
void PopupAddString(HWND hwndListBox, const TCHAR* pszText, int iIconIndex, UINT_PTR itemData);

/**
 * Creates a temporary, context-dependent popup menu with edit field and listbox.
 *
 * @param hwndOwner         The main window that owns this popup.
 * @param x                 X-coordinate on screen (screen coordinates).
 * @param y                 Y-coordinate on screen.
 * @param width             Width of the popup.
 * @return HWND             The handle of the created edit popup.
 */
HWND CreateDynamicPopupMenu(HWND hwndOwner, int x, int y, int width);

void SetPopulateCallback(HWND hwndFrame, OnPopupPopulateCallback populateCallback);

void SetSelectCallback(HWND hwndFrame, OnPopupSelectCallback selectCallback);

void SetTooltipCallback(HWND hwndFrame, OnPopupTooltipCallback tooltipCallback);

void SetImageList(HWND hwndFrame, HIMAGELIST hImageList, int icon_size);

void SetIconSize(HWND hwndFrame, int icon_size);

void SetIconMargin(HWND hwndFrame, int icon_margin);

void SetTextMargin(HWND hwndFrame, int text_margin);

void SetMaxVisibleItems(HWND hwndFrame, unsigned int max_visible_items);

void SetMenuFont(HWND hwndFrame, HFONT hFont);

void SetUserData(HWND hwndFrame, void* pUserData);

void ActivateDynamicPopup(HWND hwndFrame);

/**
 * Displays a popup menu modally and waits for a selection or cancellation.
 * This function blocks until the user makes a selection or closes the popup.
 * 
 * The workflow:
 * 1. Call CreateDynamicPopupMenu() to create the popup
 * 2: Set the callback that populates the listbox, optionally callback for tooltips
 * 3. Use SetImageList(), SetIconSize(), SetIconMargin(), etc. to configure GUI
 * 4. Call TrackDynamicPopup() to wait modally for a selection
 *
 * @param hwndPopup         The window handle returned by CreateDynamicPopupMenu().
 * @return UINT_PTR         The itemData of the selected row or 0 if cancelled.
 *
 * Example:
 *   HWND hwndPopup = CreateDynamicPopupMenu(hWnd, x, y, width);
 *   SetPopulateCallback(hwndPopup, MyPopupPopulateHandler);
 *   SetTooltipCallback(hwndPopup, MyPopupTooltipHandler);
 *   SetImageList(hwndPopup, hImageList, 16);
 *   SetIconSize(hwndPopup, 16);
 *   UINT_PTR result = TrackDynamicPopup(hwndPopup);
 */
UINT_PTR TrackDynamicPopup(HWND hwndFrame);

#endif // DYNAMIC_POPUP_H
