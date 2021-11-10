/*
 * CLCLSet
 *
 * SetMenu.c
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE
#include <commctrl.h>

#include "..\General.h"
#include "..\Ini.h"
#include "..\Message.h"
#include "..\dpi.h"

#include "CLCLSet.h"

#include "resource.h"

/* Define */

/* Global Variables */
extern HINSTANCE hInst;
extern int prop_ret;

// オプション
extern OPTION_INFO option;

/* Local Function Prototypes */

/*
 * set_menu_proc - メニュー設定のプロシージャ
 */
BOOL CALLBACK set_menu_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HMENU hMenu;
	RECT button_rect;
	TCHAR buf[BUF_SIZE];
	DWORD ret;
#ifdef OP_XP_STYLE
	static long hTheme;
#endif	// OP_XP_STYLE

	switch (uMsg) {
	case WM_INITDIALOG:
#ifdef OP_XP_STYLE
		// XP
		hTheme = open_theme(GetDlgItem(hDlg, IDC_BUTTON_FORMAT_SELECT), L"SCROLLBAR");
#endif	// OP_XP_STYLE
		// スピンコントロールの設定
		SendDlgItemMessage(hDlg, IDC_SPIN_ICON_SIZE, UDM_SETRANGE, 0, (LPARAM)MAKELONG(UD_MAXVAL, 1));
		SendDlgItemMessage(hDlg, IDC_SPIN_BMP_WIDTH, UDM_SETRANGE, 0, (LPARAM)MAKELONG(UD_MAXVAL, 1));
		SendDlgItemMessage(hDlg, IDC_SPIN_BMP_HEIGHT, UDM_SETRANGE, 0, (LPARAM)MAKELONG(UD_MAXVAL, 1));
		SendDlgItemMessage(hDlg, IDC_SPIN_SHOW_DELAY, UDM_SETRANGE, 0, (LPARAM)MAKELONG(UD_MAXVAL, 0));
		SendDlgItemMessage(hDlg, IDC_SPIN_MAX_WIDTH, UDM_SETRANGE, 0, (LPARAM)MAKELONG(UD_MAXVAL, 1));

		CheckDlgButton(hDlg, IDC_CHECK_SHOW_ICON, option.menu_show_icon);
		SetDlgItemInt(hDlg, IDC_EDIT_ICON_SIZE, UnScale(option.menu_icon_size), FALSE);

		CheckDlgButton(hDlg, IDC_CHECK_SHOW_BITMAP, option.menu_show_bitmap);
		SetDlgItemInt(hDlg, IDC_EDIT_BMP_WIDTH, UnScale(option.menu_bitmap_width), FALSE);
		SetDlgItemInt(hDlg, IDC_EDIT_BMP_HEIGHT, UnScale(option.menu_bitmap_height), FALSE);

		CheckDlgButton(hDlg, IDC_CHECK_SHOW_TOOLTIP, option.menu_show_tooltip);
		SetDlgItemInt(hDlg, IDC_EDIT_SHOW_DELAY, option.tooltip_show_delay, FALSE);

		CheckDlgButton(hDlg, IDC_CHECK_SHOW_HOTKEY, option.menu_show_hotkey);
		CheckDlgButton(hDlg, IDC_CHECK_SHOW_TOOL_MENU, option.menu_show_tool_menu);
		CheckDlgButton(hDlg, IDC_CHECK_BREAK, option.menu_break);
		SendDlgItemMessage(hDlg, IDC_EDIT_TEXT_FORMAT, WM_SETTEXT, 0, (LPARAM)option.menu_text_format);
		SetDlgItemInt(hDlg, IDC_EDIT_MAX_WIDTH, UnScale(option.menu_max_width), FALSE);

		SendMessage(hDlg, WM_COMMAND, IDC_CHECK_SHOW_ICON, 0);
		SendMessage(hDlg, WM_COMMAND, IDC_CHECK_SHOW_TOOLTIP, 0);
		break;

	case WM_DESTROY:
#ifdef OP_XP_STYLE
		// XP
		if (hTheme != 0) {
			close_theme(hTheme);
		}
#endif	// OP_XP_STYLE
		break;

	case WM_DRAWITEM:
		// ボタンの描画
#ifdef OP_XP_STYLE
		if (hTheme != 0) {
			draw_theme_scroll((LPDRAWITEMSTRUCT)lParam, DFCS_SCROLLRIGHT, hTheme);
		} else {
			draw_scroll_sontrol((LPDRAWITEMSTRUCT)lParam, DFCS_SCROLLRIGHT);
		}
#else	// OP_XP_STYLE
		draw_scroll_sontrol((LPDRAWITEMSTRUCT)lParam, DFCS_SCROLLRIGHT);
#endif	// OP_XP_STYLE
		break;

#ifdef OP_XP_STYLE
	case WM_THEMECHANGED:
		// テーマの変更
		if (hTheme != 0) {
			close_theme(hTheme);
		}
		hTheme = open_theme(GetDlgItem(hDlg, IDC_BUTTON_FORMAT_SELECT), L"SCROLLBAR");
		break;
#endif	// OP_XP_STYLE

	case WM_NOTIFY:
		return OptionNotifyProc(hDlg, uMsg, wParam, lParam);

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CHECK_SHOW_ICON:
		case IDC_CHECK_SHOW_BITMAP:
			if (IsDlgButtonChecked(hDlg, IDC_CHECK_SHOW_ICON) == 0) {
				EnableWindow(GetDlgItem(hDlg, IDC_EDIT_ICON_SIZE), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_CHECK_SHOW_BITMAP), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BMP_WIDTH), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BMP_HEIGHT), FALSE);
			} else {
				EnableWindow(GetDlgItem(hDlg, IDC_EDIT_ICON_SIZE), TRUE);
				EnableWindow(GetDlgItem(hDlg, IDC_CHECK_SHOW_BITMAP), TRUE);
				if (IsDlgButtonChecked(hDlg, IDC_CHECK_SHOW_BITMAP) == 0) {
					EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BMP_WIDTH), FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BMP_HEIGHT), FALSE);
				} else {
					EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BMP_WIDTH), TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BMP_HEIGHT), TRUE);
				}
			}
			break;

		case IDC_CHECK_SHOW_TOOLTIP:
			EnableWindow(GetDlgItem(hDlg, IDC_EDIT_SHOW_DELAY),
				IsDlgButtonChecked(hDlg, IDC_CHECK_SHOW_TOOLTIP));
			break;

		case IDC_BUTTON_FORMAT_SELECT:
			// メニューの作成
			hMenu = CreatePopupMenu();
			AppendMenu(hMenu, MF_STRING, 1, message_get_res(IDS_MENU_FORMAT_1));
			AppendMenu(hMenu, MF_STRING, 2, message_get_res(IDS_MENU_FORMAT_2));
			AppendMenu(hMenu, MF_STRING, 3, message_get_res(IDS_MENU_FORMAT_3));
			AppendMenu(hMenu, MF_STRING, 4, message_get_res(IDS_MENU_FORMAT_4));
			AppendMenu(hMenu, MF_STRING, 5, message_get_res(IDS_MENU_FORMAT_5));
			AppendMenu(hMenu, MF_STRING, 6, message_get_res(IDS_MENU_FORMAT_6));
			AppendMenu(hMenu, MF_STRING, 7, message_get_res(IDS_MENU_FORMAT_7));
			AppendMenu(hMenu, MF_STRING, 8, message_get_res(IDS_MENU_FORMAT_8));
			AppendMenu(hMenu, MF_STRING, 9, message_get_res(IDS_MENU_FORMAT_9));
			AppendMenu(hMenu, MF_STRING, 10, message_get_res(IDS_MENU_FORMAT_10));
			AppendMenu(hMenu, MF_STRING, 11, message_get_res(IDS_MENU_FORMAT_11));

			// メニューの表示
			GetWindowRect(GetDlgItem(hDlg, LOWORD(wParam)), (LPRECT)&button_rect);
			ret = TrackPopupMenu(hMenu, TPM_TOPALIGN | TPM_RETURNCMD, button_rect.right, button_rect.top, 0, hDlg, NULL);
			DestroyMenu(hMenu);
			if (ret <= 0) {
				break;
			}

			// 文字列の置き換え
			SetFocus(GetDlgItem(hDlg, IDC_EDIT_TEXT_FORMAT));
			switch (ret) {
			case 1:
				lstrcpy(buf, TEXT("%1d"));
				break;
			case 2:
				lstrcpy(buf, TEXT("%1x"));
				break;
			case 3:
				lstrcpy(buf, TEXT("%1X"));
				break;
			case 4:
				lstrcpy(buf, TEXT("%n"));
				break;
			case 5:
				lstrcpy(buf, TEXT("%a"));
				break;
			case 6:
				lstrcpy(buf, TEXT("%A"));
				break;
			case 7:
				lstrcpy(buf, TEXT("%b"));
				break;
			case 8:
				lstrcpy(buf, TEXT("%B"));
				break;
			case 9:
				lstrcpy(buf, TEXT("%c"));
				break;
			case 10:
				lstrcpy(buf, TEXT("%C"));
				break;
			case 11:
				lstrcpy(buf, TEXT("%t"));
				break;
			default:
				*buf = '\0';
				break;
			}
			SendDlgItemMessage(hDlg, IDC_EDIT_TEXT_FORMAT, EM_REPLACESEL, 0, (LPARAM)buf);
			break;

		case IDOK:
			option.menu_show_icon = IsDlgButtonChecked(hDlg, IDC_CHECK_SHOW_ICON);
			option.menu_icon_size = Scale(GetDlgItemInt(hDlg, IDC_EDIT_ICON_SIZE, NULL, FALSE));

			option.menu_show_bitmap = IsDlgButtonChecked(hDlg, IDC_CHECK_SHOW_BITMAP);
			option.menu_bitmap_width = Scale(GetDlgItemInt(hDlg, IDC_EDIT_BMP_WIDTH, NULL, FALSE));
			option.menu_bitmap_height = Scale(GetDlgItemInt(hDlg, IDC_EDIT_BMP_HEIGHT, NULL, FALSE));

			option.menu_show_tooltip = IsDlgButtonChecked(hDlg, IDC_CHECK_SHOW_TOOLTIP);
			option.tooltip_show_delay = GetDlgItemInt(hDlg, IDC_EDIT_SHOW_DELAY, NULL, FALSE);

			option.menu_show_hotkey = IsDlgButtonChecked(hDlg, IDC_CHECK_SHOW_HOTKEY);
			option.menu_show_tool_menu = IsDlgButtonChecked(hDlg, IDC_CHECK_SHOW_TOOL_MENU);
			option.menu_break = IsDlgButtonChecked(hDlg, IDC_CHECK_BREAK);
			alloc_get_text(GetDlgItem(hDlg, IDC_EDIT_TEXT_FORMAT), &option.menu_text_format);
			option.menu_max_width = Scale(GetDlgItemInt(hDlg, IDC_EDIT_MAX_WIDTH, NULL, FALSE));
			prop_ret = 1;
			break;

		case IDPCANCEL:
			break;
		}
		break;

	default:
		return FALSE;
	}
	return TRUE;
}
/* End of source */
