/*
 * CLCLSet
 *
 * SetViewer.c
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
#include "..\Memory.h"
#include "..\Ini.h"
#include "..\Message.h"

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
 * set_viewer_proc - ビューア設定のプロシージャ
 */
BOOL CALLBACK set_viewer_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	TCHAR buf[BUF_SIZE];
	TCHAR tmp[BUF_SIZE];
	TCHAR *p, *r;
	int i;

	switch (uMsg) {
	case WM_INITDIALOG:
		CheckDlgButton(hDlg, IDC_CHECK_TOGGLE, option.viewer_toggle);
		CheckDlgButton(hDlg, IDC_CHECK_CLIP_EXPAND, option.tree_clip_expand);
		CheckDlgButton(hDlg, IDC_CHECK_HISTORY_EXPAND, option.tree_history_expand);
		CheckDlgButton(hDlg, IDC_CHECK_REGIST_EXPAND, option.tree_regist_expand);
		CheckDlgButton(hDlg, IDC_CHECK_FOLDER_EXPAND, option.tree_folder_expand);

		SendMessage(GetDlgItem(hDlg, IDC_LIST_FROM), LB_ADDSTRING, 0,
			(LPARAM)message_get_res(IDS_VIEWER_CLIPBOARD));
		SendMessage(GetDlgItem(hDlg, IDC_LIST_FROM), LB_ADDSTRING, 0,
			(LPARAM)message_get_res(IDS_VIEWER_HISTORY));
		SendMessage(GetDlgItem(hDlg, IDC_LIST_FROM), LB_ADDSTRING, 0,
			(LPARAM)message_get_res(IDS_VIEWER_REGIST));

		for (p = option.tree_root_order; *p != TEXT('\0'); p++) {
			switch (*p) {
			// クリップボード
			case TEXT('0'):
				r = message_get_res(IDS_VIEWER_CLIPBOARD);
				break;
			// 履歴
			case TEXT('1'):
				r = message_get_res(IDS_VIEWER_HISTORY);
				break;
			// 登録アイテム
			case TEXT('2'):
				r = message_get_res(IDS_VIEWER_REGIST);
				break;
			}
			if (SendMessage(GetDlgItem(hDlg, IDC_LIST_TO), LB_FINDSTRING, -1, (LPARAM)r) == -1) {
				SendMessage(GetDlgItem(hDlg, IDC_LIST_TO), LB_ADDSTRING, 0, (LPARAM)r);
			}
		}
		if (SendMessage(GetDlgItem(hDlg, IDC_LIST_TO), LB_GETCOUNT, 0, 0) <= 0) {
			SendMessage(GetDlgItem(hDlg, IDC_LIST_TO), LB_ADDSTRING, 0,
				(LPARAM)message_get_res(IDS_VIEWER_CLIPBOARD));
			SendMessage(GetDlgItem(hDlg, IDC_LIST_TO), LB_ADDSTRING, 0,
				(LPARAM)message_get_res(IDS_VIEWER_HISTORY));
			SendMessage(GetDlgItem(hDlg, IDC_LIST_TO), LB_ADDSTRING, 0,
				(LPARAM)message_get_res(IDS_VIEWER_REGIST));
		}

		switch (option.list_default_action) {
		case 0:
		default:
			CheckDlgButton(hDlg, IDC_RADIO_DEF_OPEN, 1);
			break;
		case 1:
			CheckDlgButton(hDlg, IDC_RADIO_DEF_CLIPBOARD, 1);
			break;
		case 2:
			CheckDlgButton(hDlg, IDC_RADIO_DEF_SAVE, 1);
			break;
		}
		break;

	case WM_NOTIFY:
		return OptionNotifyProc(hDlg, uMsg, wParam, lParam);

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_LIST_FROM:
			if (HIWORD(wParam) == LBN_DBLCLK) {
				SendMessage(hDlg, WM_COMMAND, IDC_BUTTON_ADD, 0);
			}
			break;

		case IDC_LIST_TO:
			if (HIWORD(wParam) == LBN_DBLCLK) {
				SendMessage(hDlg, WM_COMMAND, IDC_BUTTON_DELETE, 0);
			}
			break;

		case IDC_BUTTON_ADD:
			if ((i = SendMessage(GetDlgItem(hDlg, IDC_LIST_FROM), LB_GETCURSEL, 0, 0)) == -1) {
				break;
			}
			SendMessage(GetDlgItem(hDlg, IDC_LIST_FROM), LB_GETTEXT, i, (LPARAM)buf);

			if ((i = SendMessage(GetDlgItem(hDlg, IDC_LIST_TO), LB_FINDSTRING, -1, (LPARAM)buf)) != -1) {
				SendMessage(GetDlgItem(hDlg, IDC_LIST_TO), LB_DELETESTRING, i, 0);
			}
			SendMessage(GetDlgItem(hDlg, IDC_LIST_TO), LB_ADDSTRING, 0, (LPARAM)buf);
			break;

		case IDC_BUTTON_DELETE:
			if ((i = SendMessage(GetDlgItem(hDlg, IDC_LIST_TO), LB_GETCURSEL, 0, 0)) == -1) {
				break;
			}
			SendMessage(GetDlgItem(hDlg, IDC_LIST_TO), LB_DELETESTRING, i, 0);
			break;

		case IDOK:
			option.viewer_toggle = IsDlgButtonChecked(hDlg, IDC_CHECK_TOGGLE);
			option.tree_clip_expand = IsDlgButtonChecked(hDlg, IDC_CHECK_CLIP_EXPAND);
			option.tree_history_expand = IsDlgButtonChecked(hDlg, IDC_CHECK_HISTORY_EXPAND);
			option.tree_regist_expand = IsDlgButtonChecked(hDlg, IDC_CHECK_REGIST_EXPAND);
			option.tree_folder_expand = IsDlgButtonChecked(hDlg, IDC_CHECK_FOLDER_EXPAND);

			p = tmp;
			for (i = 0; i < SendMessage(GetDlgItem(hDlg, IDC_LIST_TO), LB_GETCOUNT, 0, 0); i++) {
				SendMessage(GetDlgItem(hDlg, IDC_LIST_TO), LB_GETTEXT, i, (LPARAM)buf);
				if (lstrcmp(buf, message_get_res(IDS_VIEWER_CLIPBOARD)) == 0) {
					*(p++) = TEXT('0');
				} else if (lstrcmp(buf, message_get_res(IDS_VIEWER_HISTORY)) == 0) {
					*(p++) = TEXT('1');
				} else if (lstrcmp(buf, message_get_res(IDS_VIEWER_REGIST)) == 0) {
					*(p++) = TEXT('2');
				}
			}
			*p = TEXT('\0');
			mem_free(&option.tree_root_order);
			option.tree_root_order = alloc_copy(tmp);

			if (IsDlgButtonChecked(hDlg, IDC_RADIO_DEF_CLIPBOARD) == 1) {
				option.list_default_action = 1;
			} else if (IsDlgButtonChecked(hDlg, IDC_RADIO_DEF_SAVE) == 1) {
				option.list_default_action = 2;
			} else {
				option.list_default_action = 0;
			}
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
