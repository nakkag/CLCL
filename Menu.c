/*
 * CLCL
 *
 * Menu.c
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE
#include <tchar.h>

#include "General.h"
#include "Memory.h"
#include "Data.h"
#include "Ini.h"
#include "Message.h"
#include "Menu.h"
#include "Regist.h"
#include "ClipBoard.h"
#include "Format.h"
#include "Font.h"
#include "dpi.h"

#include "resource.h"

/* Define */
#define LICONSIZE			32
#define SICONSIZE			16

/* Global Variables */
static MENU_ITEM_INFO *menu_item_info;
static int menu_item_cnt;

extern HINSTANCE hInst;

extern HICON icon_menu_default;
extern HICON icon_menu_folder;

// オプション
extern OPTION_INFO option;

/* Local Function Prototypes */
static void menu_item_free(MENU_ITEM_INFO *mii, int cnt);
static MENU_ITEM_INFO *menu_id_to_menuitem(MENU_ITEM_INFO *mii, const int mcnt, const UINT id);
static HICON menu_read_icon(const TCHAR *file_name, const int index, const int icon_size);
static HFONT menu_create_font(void);
static int menu_get_item_size(const MENU_ITEM_INFO *mii, int *width);
static void menu_create_text(const int index, const TCHAR *buf, TCHAR *ret);
static BOOL menu_create_datainfo(DATA_INFO *set_di,
								MENU_ITEM_INFO *mii, int menu_index, int *id,
								const int step, const int min, const int max);
static MENU_ITEM_INFO *menu_create_info(MENU_INFO *menu_info, const int menu_cnt,
								DATA_INFO *history_di, DATA_INFO *regist_di, int *id, int *ret_cnt);
static BOOL menu_set_item(const HDC hdc, const HMENU hMenu, MENU_ITEM_INFO *mii, const int cnt);
static int menu_draw_bitmap(const HDC draw_dc, const DATA_INFO *di, const int height);
static BOOL menu_draw_ckeck(const HDC draw_dc, const int left, const int top, const int right, const int bottom);
static TCHAR menu_get_accelerator(TCHAR *str);

/*
 * menu_item_free - メニュー情報の解放
 */
static void menu_item_free(MENU_ITEM_INFO *mii, int cnt)
{
	int i;

	if (mii == NULL) {
		return;
	}
	for (i = 0; i < cnt; i++) {
		if ((mii + i)->mii != NULL) {
			menu_item_free((mii + i)->mii, (mii + i)->mii_cnt);
		}
		mem_free(&((mii + i)->text));
		mem_free(&((mii + i)->hkey));
		if ((mii + i)->free_icon == TRUE && (mii + i)->icon != NULL) {
			DestroyIcon((mii + i)->icon);
		}
	}
	mem_free(&mii);
}

/*
 * menu_free - メニュー情報の解放
 */
void menu_free(void)
{
	menu_item_free(menu_item_info, menu_item_cnt);
	menu_item_info = NULL;
	menu_item_cnt = 0;
}

/*
 * menu_show - マウスの位置にメニューを表示する
 */
int menu_show(const HWND hWnd, const HMENU hMenu, const POINT *mpos)
{
	POINT apos;
	DWORD ret;
	int x = 0, y = 0;

	if (mpos == NULL ||
		(mpos->x < 0 || mpos->x > GetSystemMetrics(SM_CXSCREEN) ||
		mpos->y < 0 || mpos->y > GetSystemMetrics(SM_CYSCREEN))) {
		GetCursorPos((LPPOINT)&apos);
		x = apos.x;
		y = apos.y;
	} else {
		x = mpos->x;
		y = mpos->y;
	}
	ret = TrackPopupMenu(hMenu,
		TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_RETURNCMD,
		x, y, 0, hWnd, NULL);
	PostMessage(hWnd, WM_NULL, 0, 0);
	return ret;
}

/*
 * menu_id_to_menuitem - メニューIDからメニュー情報を検索
 */
static MENU_ITEM_INFO *menu_id_to_menuitem(MENU_ITEM_INFO *mii, const int mcnt, const UINT id)
{
	MENU_ITEM_INFO *ret;
	int i;

	if (mii == NULL) {
		return NULL;
	}
	for (i = 0; i < mcnt; i++) {
		if ((mii + i)->mii != NULL) {
			ret = menu_id_to_menuitem((mii + i)->mii, (mii + i)->mii_cnt, id);
			if (ret != NULL) {
				return ret;
			}
		}
		if ((mii + i)->id == id) {
			return (mii + i);
		}
	}
	return NULL;
}

/*
 * menu_get_info - メニューIDからメニュー情報を取得
 */
MENU_ITEM_INFO *menu_get_info(const UINT id)
{
	return menu_id_to_menuitem(menu_item_info, menu_item_cnt, id);
}

/*
 * menu_read_icon - アイコン取得
 */
static HICON menu_read_icon(const TCHAR *file_name, const int index, const int icon_size)
{
	SHFILEINFO shfi;
	HICON hIcon = NULL;
	HICON hsIcon = NULL;
	int icon_flag;

	if (file_name == NULL || *file_name == TEXT('\0')) {
		return NULL;
	}
	// ファイルからアイコン取得
	ExtractIconEx(file_name, index, &hIcon, &hsIcon, 1);
	if (icon_size >= LICONSIZE) {
		DestroyIcon(hsIcon);
	} else {
		DestroyIcon(hIcon);
		hIcon = hsIcon;
	}
	if (hIcon == NULL) {
		// 関連付けからアイコン取得
		icon_flag = SHGFI_ICON | ((icon_size == SICONSIZE) ? SHGFI_SMALLICON : SHGFI_LARGEICON);
		SHGetFileInfo(file_name, SHGFI_USEFILEATTRIBUTES, &shfi, sizeof(SHFILEINFO), icon_flag);
		hIcon = shfi.hIcon;
	}
	return hIcon;
}

/*
 * menu_create_font - メニュー用フォントの作成
 */
static HFONT menu_create_font(void)
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
 * menu_get_item_size - オーナードローメニュー項目のサイズを取得
 */
static int menu_get_item_size(const MENU_ITEM_INFO *mii, int *width)
{
	int text_x, text_y;
	int bmp_x, bmp_y;
	int ret_x, ret_y;

	text_x = mii->text_x;
	text_y = mii->text_y;

	if (mii->flag & MF_SEPARATOR) {
		// 区切り
		ret_x = 0;
		ret_y = option.menu_separator_height;

	} else if (option.menu_show_icon != 1) {
		// テキストのみ
		text_x += (option.menu_icon_margin + option.menu_icon_size + option.menu_text_margin_left + option.menu_text_margin_right);
		ret_x = (text_x > option.menu_max_width) ? option.menu_max_width : text_x;

		text_y += (option.menu_text_margin_y * 2);
		ret_y = text_y;

	} else if (mii->show_bitmap == TRUE) {
		// ビットマップ表示
		if (mii->show_di->menu_bmp_width == 0 && mii->show_di->menu_bmp_height == 0) {
			bmp_x = option.menu_bitmap_width;
			bmp_y = option.menu_bitmap_height;
		} else {
			bmp_x = mii->show_di->menu_bmp_width;
			bmp_y = mii->show_di->menu_bmp_height;
		}
		text_x += (option.menu_icon_margin + bmp_x + option.menu_text_margin_left + option.menu_text_margin_right);
		ret_x = (text_x > option.menu_max_width) ? option.menu_max_width : text_x;

		text_y += (option.menu_text_margin_y * 2);
		ret_y = (bmp_y + (option.menu_icon_margin * 2) > text_y)
			? bmp_y + (option.menu_icon_margin * 2) : text_y;

	} else {
		// アイコン表示
		text_x += (option.menu_icon_margin + option.menu_icon_size + option.menu_text_margin_left + option.menu_text_margin_right);
		ret_x = (text_x > option.menu_max_width) ? option.menu_max_width : text_x;

		text_y += (option.menu_text_margin_y * 2);
		ret_y = (text_y > option.menu_icon_margin + option.menu_icon_size + option.menu_icon_margin)
			? text_y : (option.menu_icon_margin + option.menu_icon_size + option.menu_icon_margin);
	}
	if (width != NULL) {
		*width = ret_x;
	}
	return ret_y;
}

/*
 * menu_create_text - アクセラレータ付き文字文字列の作成
 */
static void menu_create_text(const int index, const TCHAR *buf, TCHAR *ret)
{
	TCHAR *p = option.menu_text_format;
	TCHAR *r;
	int base;
	int num;
	int i;

	if (*p == TEXT('\0')) {
		lstrcpy(ret, buf);
		return;
	}
	while (*p != TEXT('\0')) {
#ifndef UNICODE
		if (IsDBCSLeadByte((BYTE)*p) == TRUE) {
			*(ret++) = *(p++);
			if (*p == TEXT('\0')) {
				break;
			}
			*(ret++) = *(p++);
			continue;
		}
#endif	// UNICODE
		if (*p != TEXT('%')) {
			*(ret++) = *(p++);
			continue;
		}
		r = p;
		p++;

		// ベース値
		if (*p >= TEXT('0') && *p <= TEXT('9')) {
			base = _ttoi(p);
			for (; *p >= TEXT('0') && *p <= TEXT('9'); p++)
				;
		} else {
			base = 0;
		}
		num = index + base;

		switch (*p) {
		case TEXT('d'):
		case TEXT('D'):
			// 数字 (10進数)
			_itot_s(num, ret, BUF_SIZE, 10);
			ret += lstrlen(ret);
			break;

		case TEXT('x'):
			// 数字 (16進数) (小文字)
			_itot_s(num, ret, BUF_SIZE, 16);
			CharLower(ret);
			ret += lstrlen(ret);
			break;

		case TEXT('X'):
			// 数字 (16進数)
			_itot_s(num, ret, BUF_SIZE, 16);
			CharUpper(ret);
			ret += lstrlen(ret);
			break;

		case TEXT('n'):
		case TEXT('N'):
			// １桁の数字
			*(ret++) = TEXT('0') + num % 10;
			break;

		case TEXT('a'):
			// アルファベット (小文字)
			*(ret++) = TEXT('a') + num % 26;
			break;

		case TEXT('A'):
			// アルファベット
			*(ret++) = TEXT('A') + num % 26;
			break;

		case TEXT('b'):
			// アルファベット + 数字 (小文字)
			i = num % (26 + 10);
			*(ret++) = (i < 26) ? TEXT('a') + i : TEXT('0') + i - 26;
			break;

		case TEXT('B'):
			// アルファベット + 数字
			i = num % (26 + 10);
			*(ret++) = (i < 26) ? TEXT('A') + i : TEXT('0') + i - 26;
			break;

		case TEXT('c'):
			// 数字 + アルファベット (小文字)
			i = num % (26 + 10);
			*(ret++) = (i < 10) ? TEXT('0') + i : TEXT('a') + i - 10;
			break;

		case TEXT('C'):
			// 数字 + アルファベット
			i = num % (26 + 10);
			*(ret++) = (i < 10) ? TEXT('0') + i : TEXT('A') + i - 10;
			break;

		case TEXT('t'):
		case TEXT('T'):
			// タイトル
			lstrcpyn(ret, buf, BUF_SIZE);
			ret += lstrlen(ret);
			break;

		case TEXT('%'):
			// %
			*(ret++) = *p;
			break;

		default:
			*(ret++) = *r;
			p = r;
			break;

		}
		if (*p == TEXT('\0')) {
			break;
		}
		p++;
	}
	*ret = TEXT('\0');
}

/*
 * menu_get_keyname - キー名を取得
 */
TCHAR *menu_get_keyname(const UINT modifiers, const UINT virtkey)
{
	TCHAR buf[BUF_SIZE];
	UINT scan_code;
	int ext_flag = 0;

	*buf = TEXT('\0');
	if (modifiers & MOD_CONTROL) {
		lstrcat(buf, TEXT("Ctrl+"));
	}
	if (modifiers & MOD_SHIFT) {
		lstrcat(buf, TEXT("Shift+"));
	}
	if (modifiers & MOD_ALT) {
		lstrcat(buf, TEXT("Alt+"));
	}
	if (modifiers & MOD_WIN) {
		lstrcat(buf, TEXT("Win+"));
	}
	if (virtkey == 0 || (scan_code = MapVirtualKey(virtkey, 0)) <= 0) {
		// なし
		return NULL;
	}
	if (virtkey == VK_APPS ||
		virtkey == VK_PRIOR ||
		virtkey == VK_NEXT ||
		virtkey == VK_END ||
		virtkey == VK_HOME ||
		virtkey == VK_LEFT ||
		virtkey == VK_UP ||
		virtkey == VK_RIGHT ||
		virtkey == VK_DOWN ||
		virtkey == VK_INSERT ||
		virtkey == VK_DELETE ||
		virtkey == VK_NUMLOCK) {
		ext_flag = 1 << 24;
	}
	GetKeyNameText((scan_code << 16) | ext_flag, buf + lstrlen(buf), BUF_SIZE - lstrlen(buf) - 1);
	return alloc_copy(buf);
}

/*
 * menu_create_datainfo - メニュー情報にデータを展開
 */
static BOOL menu_create_datainfo(DATA_INFO *set_di,
								MENU_ITEM_INFO *mii, int menu_index, int *id,
								const int step, const int min, const int max)
{
	DATA_INFO *di;
	DATA_INFO *cdi;
	MENU_ITEM_INFO *cmi;
	TCHAR buf[BUF_SIZE * 2];
	TCHAR tmp[BUF_SIZE];
	TCHAR *p;
	int cnt;
	int i, j;
	int m, n;

	// 初期位置移動
	for (m = 0; set_di != NULL && min > 0 && m < min - 1; set_di = set_di->next, m++)
		;
	if (step < 0) {
		// 降順
		for (di = set_di, i = 0, n = m; di != NULL && (max <= 0 || n < max); di = di->next, i++, n++)
			;
		i += menu_index - 1;
	} else {
		// 昇順
		i = menu_index;
	}

	for (di = set_di,j = 0; di != NULL && (max <= 0 || m < max); di = di->next, i += step, m++) {
		(mii + i)->id = ID_MENUITEM_DATA + ((*id)++);
		(mii + i)->item = (LPCTSTR)(mii + i);
		(mii + i)->set_di = di;

		switch (di->type) {
		case TYPE_FOLDER:
			// 階層表示
			(mii + i)->flag = MF_POPUP | MF_OWNERDRAW;
			(mii + i)->show_di = di;

			for (cdi = di->child, cnt = 0; cdi != NULL; cdi = cdi->next, cnt++)
				;
			// メニュー項目情報の確保
			if ((cmi = mem_calloc(sizeof(MENU_ITEM_INFO) * cnt)) == NULL) {
				return FALSE;
			}
			(mii + i)->mii = cmi;
			(mii + i)->mii_cnt = cnt;
			menu_create_datainfo(di->child, cmi, 0, id, step, 0, 0);
			break;

		case TYPE_ITEM:
			// アイテム
			(mii + i)->flag = MF_OWNERDRAW;
			(mii + i)->show_di = format_get_priority_highest(di);
			break;

		case TYPE_DATA:
			// データ
			(mii + i)->flag = MF_OWNERDRAW;
			(mii + i)->show_di = di;
			break;
		}

		// メニューに表示するタイトルを取得
		format_get_menu_title((mii + i)->show_di);
		// タイトルを設定
		if (di->title != NULL) {
			if (lstrcmp(di->title, TEXT("-")) == 0) {
				// 区切り
				(mii + i)->id = 0;
				(mii + i)->flag = MF_SEPARATOR | MF_OWNERDRAW;
				(mii + i)->item = (LPCTSTR)(mii + i);
				continue;
			} else if (option.menu_intact_item_title == 0) {
				menu_create_text(j++, di->title, buf);
				(mii + i)->text = alloc_copy(buf);
			} else {
				(mii + i)->text = alloc_copy(di->title);
			}

		} else if ((mii + i)->show_di->menu_title != NULL) {
			menu_create_text(j++, (mii + i)->show_di->menu_title, buf);
			(mii + i)->text = alloc_copy(buf);

		} else if ((mii + i)->show_di->format_name != NULL) {
			// 形式名
			p = tmp;
			*(p++) = TEXT('(');
			lstrcpyn(p, (mii + i)->show_di->format_name, BUF_SIZE - 3);
			p += lstrlen(p);
			*(p++) = TEXT(')');
			*(p++) = TEXT('\0');
			menu_create_text(j++, tmp, buf);
			(mii + i)->text = alloc_copy(buf);
			(mii + i)->show_format = TRUE;

		} else {
			(mii + i)->text = alloc_copy(TEXT(""));
		}

		if (option.menu_show_hotkey == 1) {
			// ホットキー取得
			(mii + i)->hkey = menu_get_keyname(di->op_modifiers, di->op_virtkey);
		}

		if (option.menu_show_icon == 1) {
			// メニューに表示するアイコンを取得
			format_get_menu_icon((mii + i)->show_di);
			if ((mii + i)->show_di->menu_icon == NULL) {
				(mii + i)->icon = (di->type == TYPE_FOLDER) ? icon_menu_folder : icon_menu_default;
			} else {
				(mii + i)->icon = (mii + i)->show_di->menu_icon;
			}
			(mii + i)->free_icon = FALSE;

			// メニューに表示するビットマップを取得
			if (option.menu_show_bitmap == 1) {
				format_get_menu_bitmap((mii + i)->show_di);
			}
			(mii + i)->show_bitmap = (option.menu_show_bitmap == 1 &&
				(mii + i)->show_di->menu_bitmap != NULL) ? TRUE : FALSE;
		}
	}
	return TRUE;
}

/*
 * menu_create_info - メニュー情報の作成
 */
static MENU_ITEM_INFO *menu_create_info(MENU_INFO *menu_info, const int menu_cnt,
										DATA_INFO *history_di, DATA_INFO *regist_di,
										int *id, int *ret_cnt)
{
	MENU_ITEM_INFO *mii;
	DATA_INFO *di;
	int i, j, t;
	int cnt;

	// メニュー項目数の取得
	for (i = 0, *ret_cnt = 0; i < menu_cnt; i++) {
		switch ((menu_info + i)->content) {
		case MENU_CONTENT_SEPARATOR:
		case MENU_CONTENT_POPUP:
		case MENU_CONTENT_VIEWER:
		case MENU_CONTENT_OPTION:
		case MENU_CONTENT_CLIPBOARD_WATCH:
		case MENU_CONTENT_APP:
		case MENU_CONTENT_CANCEL:
		case MENU_CONTENT_EXIT:
			(*ret_cnt)++;
			break;

		case MENU_CONTENT_HISTORY:
		case MENU_CONTENT_HISTORY_DESC:
			for (di = history_di, cnt = 0; di != NULL &&
				(menu_info + i)->min > 0 && cnt < (menu_info + i)->min - 1; di = di->next, cnt++);
			for (; di != NULL &&
				((menu_info + i)->max <= 0 || cnt < (menu_info + i)->max); di = di->next, (*ret_cnt)++, cnt++);
			break;

		case MENU_CONTENT_REGIST:
		case MENU_CONTENT_REGIST_DESC:
			di = regist_path_to_item(regist_di, (menu_info + i)->path);
			for (; di != NULL; di = di->next, (*ret_cnt)++)
				;
			break;

		case MENU_CONTENT_TOOL:
			if ((menu_info + i)->path != NULL && *(menu_info + i)->path != TEXT('\0')) {
				if (tool_title_to_index((menu_info + i)->path) != -1) {
					(*ret_cnt)++;
				}
			} else {
				for (t = 0; t < option.tool_cnt; t++) {
					if ((option.tool_info + t)->call_type & CALLTYPE_MENU) {
						(*ret_cnt)++;
					}
				}
			}
			break;
		}
	}

	// メニュー項目情報の確保
	if ((mii = mem_calloc(sizeof(MENU_ITEM_INFO) * (*ret_cnt))) == NULL) {
		*ret_cnt = 0;
		return NULL;
	}

	// メニュー項目情報の作成
	for (i = 0, j = 0; i < menu_cnt; i++) {
		switch ((menu_info + i)->content) {
		case MENU_CONTENT_SEPARATOR:
			// 区切り
			(mii + j)->id = 0;
			(mii + j)->flag = MF_SEPARATOR | MF_OWNERDRAW;
			(mii + j)->item = (LPCTSTR)(mii + j);
			j++;
			break;

		case MENU_CONTENT_HISTORY:
			// 履歴 (昇順)
			if (menu_create_datainfo(history_di, mii, j, id, 1, (menu_info + i)->min, (menu_info + i)->max) == TRUE) {
				for (di = history_di, cnt = 0; di != NULL &&
					(menu_info + i)->min > 0 && cnt < (menu_info + i)->min - 1; di = di->next, cnt++);
				for (; di != NULL &&
					((menu_info + i)->max <= 0 || cnt < (menu_info + i)->max); di = di->next, j++, cnt++);
			}
			break;

		case MENU_CONTENT_HISTORY_DESC:
			// 履歴 (降順)
			if (menu_create_datainfo(history_di, mii, j, id, -1, (menu_info + i)->min, (menu_info + i)->max) == TRUE) {
				for (di = history_di, cnt = 0; di != NULL &&
					(menu_info + i)->min > 0 && cnt < (menu_info + i)->min - 1; di = di->next, cnt++)
					;
				for (; di != NULL &&
					((menu_info + i)->max <= 0 || cnt < (menu_info + i)->max); di = di->next, j++, cnt++)
					;
			}
			break;

		case MENU_CONTENT_REGIST:
			// 登録アイテム (昇順)
			di = regist_path_to_item(regist_di, (menu_info + i)->path);
			if (di != NULL && menu_create_datainfo(di, mii, j, id, 1, 0, 0) == TRUE) {
				for (; di != NULL; di = di->next, j++)
					;
			}
			break;

		case MENU_CONTENT_REGIST_DESC:
			// 登録アイテム (降順)
			di = regist_path_to_item(regist_di, (menu_info + i)->path);
			if (di != NULL && menu_create_datainfo(di, mii, j, id, -1, 0, 0) == TRUE) {
				for (; di != NULL; di = di->next, j++)
					;
			}
			break;

		case MENU_CONTENT_POPUP:
			// ポップアップメニュー
			(mii + j)->flag = MF_POPUP | MF_OWNERDRAW;
			(mii + j)->item = (LPCTSTR)(mii + j);
			(mii + j)->text = alloc_copy((menu_info + i)->title);
			(mii + j)->icon = menu_read_icon((menu_info + i)->icon_path, (menu_info + i)->icon_index, option.menu_icon_size);
			(mii + j)->free_icon = TRUE;
			(mii + j)->mii = menu_create_info(
				(menu_info + i)->mi, (menu_info + i)->mi_cnt,
				history_di, regist_di, id, &(mii + j)->mii_cnt);
			j++;
			break;

		case MENU_CONTENT_VIEWER:
			// ビューア
			(mii + j)->id = ID_MENUITEM_VIEWER;
			(mii + j)->flag = MF_OWNERDRAW;
			(mii + j)->item = (LPCTSTR)(mii + j);
			(mii + j)->text = alloc_copy(((menu_info + i)->title == NULL || *(menu_info + i)->title == TEXT('\0')) ?
				message_get_res(IDS_MENU_VIEWER) : (menu_info + i)->title);
			(mii + j)->icon = menu_read_icon((menu_info + i)->icon_path, (menu_info + i)->icon_index, option.menu_icon_size);
			(mii + j)->free_icon = TRUE;
			j++;
			break;

		case MENU_CONTENT_OPTION:
			// オプション
			(mii + j)->id = ID_MENUITEM_OPTION;
			(mii + j)->flag = MF_OWNERDRAW;
			(mii + j)->item = (LPCTSTR)(mii + j);
			(mii + j)->text = alloc_copy(((menu_info + i)->title == NULL || *(menu_info + i)->title == TEXT('\0')) ?
				message_get_res(IDS_MENU_OPTION) : (menu_info + i)->title);
			(mii + j)->icon = menu_read_icon((menu_info + i)->icon_path, (menu_info + i)->icon_index, option.menu_icon_size);
			(mii + j)->free_icon = TRUE;
			j++;
			break;

		case MENU_CONTENT_CLIPBOARD_WATCH:
			// クリップボード監視切り替え
			(mii + j)->id = ID_MENUITEM_CLIPBOARD_WATCH;
			(mii + j)->flag = MF_OWNERDRAW | ((option.main_clipboard_watch == 1) ? MF_CHECKED : 0);
			(mii + j)->item = (LPCTSTR)(mii + j);
			(mii + j)->text = alloc_copy(((menu_info + i)->title == NULL || *(menu_info + i)->title == TEXT('\0')) ?
				message_get_res(IDS_MENU_CLIPBOARD_WATCH) : (menu_info + i)->title);
			(mii + j)->icon = menu_read_icon((menu_info + i)->icon_path, (menu_info + i)->icon_index, option.menu_icon_size);
			(mii + j)->free_icon = TRUE;
			j++;
			break;

		case MENU_CONTENT_TOOL:
			// ツール
			if ((menu_info + i)->path != NULL && *(menu_info + i)->path != TEXT('\0')) {
				if ((t = tool_title_to_index((menu_info + i)->path)) != -1) {
					(mii + j)->id = ID_MENUITEM_DATA + ((*id)++);
					(mii + j)->flag = MF_OWNERDRAW;
					(mii + j)->item = (LPCTSTR)(mii + j);
					if ((menu_info + i)->title != NULL && *(menu_info + i)->title != TEXT('\0')) {
						(mii + j)->text = alloc_copy((menu_info + i)->title);
					} else {
						(mii + j)->text = alloc_copy((option.tool_info + t)->title);
					}
					if (option.menu_show_hotkey == 1) {
						(mii + j)->hkey = menu_get_keyname((option.tool_info + t)->modifiers, (option.tool_info + t)->virtkey);
					}
					(mii + j)->icon = menu_read_icon((menu_info + i)->icon_path, (menu_info + i)->icon_index, option.menu_icon_size);
					(mii + j)->free_icon = TRUE;
					(mii + j)->ti = option.tool_info + t;
					j++;
				}
			} else {
				for (t = 0; t < option.tool_cnt; t++) {
					if (!((option.tool_info + t)->call_type & CALLTYPE_MENU)) {
						continue;
					}
					if (lstrcmp((option.tool_info + t)->title, TEXT("-")) == 0) {
						(mii + j)->id = 0;
						(mii + j)->flag = MF_SEPARATOR | MF_OWNERDRAW;
						(mii + j)->item = (LPCTSTR)(mii + j);
					} else {
						(mii + j)->id = ID_MENUITEM_DATA + ((*id)++);
						(mii + j)->flag = MF_OWNERDRAW;
						(mii + j)->item = (LPCTSTR)(mii + j);
						(mii + j)->text = alloc_copy((option.tool_info + t)->title);
						if (option.menu_show_hotkey == 1) {
							(mii + j)->hkey = menu_get_keyname((option.tool_info + t)->modifiers, (option.tool_info + t)->virtkey);
						}
						(mii + j)->ti = option.tool_info + t;
					}
					j++;
				}
			}
			break;

		case MENU_CONTENT_APP:
			// アプリケーション実行
			(mii + j)->id = ID_MENUITEM_DATA + ((*id)++);
			(mii + j)->flag = MF_OWNERDRAW;
			(mii + j)->item = (LPCTSTR)(mii + j);
			(mii + j)->text = alloc_copy((menu_info + i)->title);
			if (*(menu_info + i)->icon_path != TEXT('\0')) {
				(mii + j)->icon = menu_read_icon((menu_info + i)->icon_path, (menu_info + i)->icon_index, option.menu_icon_size);
			} else {
				(mii + j)->icon = menu_read_icon((menu_info + i)->path, 0, option.menu_icon_size);
			}
			(mii + j)->free_icon = TRUE;
			// メニュー情報を設定
			(mii + j)->mi = menu_info + i;
			j++;
			break;

		case MENU_CONTENT_CANCEL:
			// キャンセル
			(mii + j)->id = IDCANCEL;
			(mii + j)->flag = MF_OWNERDRAW;
			(mii + j)->item = (LPCTSTR)(mii + j);
			(mii + j)->text = alloc_copy(((menu_info + i)->title == NULL || *(menu_info + i)->title == TEXT('\0')) ?
				message_get_res(IDS_MENU_CANCEL) : (menu_info + i)->title);
			(mii + j)->icon = menu_read_icon((menu_info + i)->icon_path, (menu_info + i)->icon_index, option.menu_icon_size);
			(mii + j)->free_icon = TRUE;
			j++;
			break;

		case MENU_CONTENT_EXIT:
			// 終了
			(mii + j)->id = ID_MENUITEM_EXIT;
			(mii + j)->flag = MF_OWNERDRAW;
			(mii + j)->item = (LPCTSTR)(mii + j);
			(mii + j)->text = alloc_copy(((menu_info + i)->title == NULL || *(menu_info + i)->title == TEXT('\0')) ?
				message_get_res(IDS_MENU_EXIT) : (menu_info + i)->title);
			(mii + j)->icon = menu_read_icon((menu_info + i)->icon_path, (menu_info + i)->icon_index, option.menu_icon_size);
			(mii + j)->free_icon = TRUE;
			j++;
			break;
		}
	}
	return mii;
}

/*
 * menu_set_item - メニューに項目を設定
 */
static BOOL menu_set_item(const HDC hdc, const HMENU hMenu, MENU_ITEM_INFO *mii, const int cnt)
{
	HMENU hPopupMenu;
	SIZE size;
	int height = 0;
	int item_height;
	int menu_flag;
	int i;

	// メニュー項目の追加
	for (i = 0; i < cnt; i++) {
		// メニューの高さを取得
		if ((mii + i)->flag & MF_SEPARATOR) {
			item_height = option.menu_separator_height;
		} else if ((mii + i)->flag & MF_OWNERDRAW) {
			if ((mii + i)->text != NULL && GetTextExtentPoint32(hdc,
				(mii + i)->text, lstrlen((mii + i)->text), &size) == TRUE) {
				(mii + i)->text_x = size.cx;
				(mii + i)->text_y = size.cy;
			}
			if ((mii + i)->hkey != NULL && GetTextExtentPoint32(hdc,
				(mii + i)->hkey, lstrlen((mii + i)->hkey), &size) == TRUE) {
				(mii + i)->text_x += size.cx;
			}
			item_height = menu_get_item_size(mii + i, NULL);
		} else {
			item_height = GetSystemMetrics(SM_CYMENU);
		}
		// 折り返し設定
		menu_flag = 0;
		height += item_height;
		if (option.menu_break == 1 && height >= GetSystemMetrics(SM_CYSCREEN)) {
			height = item_height;
			menu_flag = MF_MENUBARBREAK;
		}

		if ((mii + i)->flag & MF_POPUP) {
			hPopupMenu = CreatePopupMenu();
			menu_set_item(hdc, hPopupMenu, (mii + i)->mii, (mii + i)->mii_cnt);
			// メニュー項目の追加
			AppendMenu(hMenu, (mii + i)->flag | menu_flag, (UINT)hPopupMenu, (mii + i)->item);
		} else {
			// メニュー項目の追加
			AppendMenu(hMenu, (mii + i)->flag | menu_flag, (mii + i)->id, (mii + i)->item);
		}
	}
	return TRUE;
}

/*
 * menu_create - メニューの作成
 */
HMENU menu_create(const HWND hWnd, MENU_INFO *menu_info, const int menu_cnt,
				  DATA_INFO *history_di, DATA_INFO *regist_di)
{
	HMENU hMenu;
	HDC hdc;
	HFONT hFont, hRetFont;
	int id = 0;

	// メニュー作成
	if ((hMenu = CreatePopupMenu()) == NULL) {
		return NULL;
	}
	menu_item_info = menu_create_info(menu_info, menu_cnt, history_di, regist_di, &id, &menu_item_cnt);

	if ((hdc = GetDC(hWnd)) == NULL) {
		DestroyMenu(hMenu);
		return NULL;
	}
	// フォント設定
	hFont = menu_create_font();
	hRetFont = SelectObject(hdc, hFont);
	// メニューに項目を設定
	menu_set_item(hdc, hMenu, menu_item_info, menu_item_cnt);
	SelectObject(hdc, hRetFont);
	DeleteObject(hFont);
	ReleaseDC(hWnd, hdc);
	return hMenu;
}

/*
 * menu_destory - メニューの破棄
 */
void menu_destory(HMENU hMenu)
{
	HMENU hSubMenu;
	int cnt, i;

	cnt = GetMenuItemCount(hMenu);
	for (i = 0; i < cnt; i++) {
		if ((hSubMenu = GetSubMenu(hMenu, i)) != NULL) {
			menu_destory(hSubMenu);
			ModifyMenu(hMenu, i, MF_BYPOSITION, 0, NULL);
		}
	}
	DestroyMenu(hMenu);
}

/*
 * menu_set_drawitem - メニュー描画設定
 */
BOOL menu_set_drawitem(MEASUREITEMSTRUCT *ms)
{
	ms->itemHeight = menu_get_item_size((MENU_ITEM_INFO *)ms->itemData, &ms->itemWidth);
	return TRUE;
}

/*
 * menu_draw_bitmap - メニューにビットマップを描画
 */
static int menu_draw_bitmap(const HDC draw_dc, const DATA_INFO *di, const int height)
{
	HDC hdc;
	HBITMAP hRetBmp;
	int bmp_width;
	int bmp_height;

	if ((hdc = CreateCompatibleDC(draw_dc)) == NULL) {
		return -1;
	}
	if ((hRetBmp = SelectObject(hdc, di->menu_bitmap)) == NULL) {
		DeleteDC(hdc);
		return -1;
	}

	if (di->menu_bmp_width == 0 && di->menu_bmp_height == 0) {
		bmp_width = option.menu_bitmap_width;
		bmp_height = option.menu_bitmap_height;
	} else {
		bmp_width = di->menu_bmp_width;
		bmp_height = di->menu_bmp_height;
	}

	BitBlt(draw_dc,
		option.menu_icon_margin,
		height / 2 - bmp_height / 2,
		bmp_width, height,
		hdc, 0, 0, SRCCOPY);

	SelectObject(hdc, hRetBmp);
	DeleteDC(hdc);
	return option.menu_icon_margin + bmp_width;
}

/*
 * menu_draw_ckeck - メニューのチェックマークを描画
 */
static BOOL menu_draw_ckeck(const HDC draw_dc, const int left, const int top, const int right, const int bottom)
{
	HDC hdc;
	HBITMAP hbmp, ret_hbmp;
	HDC wk_dc;
	HBITMAP wk_hbmp, wk_ret_hbmp;
	HANDLE hBrush;
	RECT draw_rect;

	// 作業用DCの作成
	if ((hdc = CreateCompatibleDC(draw_dc)) == NULL) {
		return FALSE;
	}
	if ((hbmp = CreateCompatibleBitmap(draw_dc, right - left, bottom - top)) == NULL) {
		DeleteDC(hdc);
		return FALSE;
	}
	ret_hbmp = SelectObject(hdc, hbmp);

	if ((wk_dc = CreateCompatibleDC(draw_dc)) == NULL) {
		SelectObject(hdc, ret_hbmp);
		DeleteObject(hbmp);
		DeleteDC(hdc);
		return FALSE;
	}
	if ((wk_hbmp = CreateCompatibleBitmap(draw_dc, right - left, bottom - top)) == NULL) {
		SelectObject(hdc, ret_hbmp);
		DeleteObject(hbmp);
		DeleteDC(hdc);
		DeleteDC(wk_dc);
		return FALSE;
	}
	wk_ret_hbmp = SelectObject(wk_dc, wk_hbmp);

	SetRect(&draw_rect, 0, 0, right - left, bottom - top);
	
	// マスクの描画
	DrawFrameControl(hdc, &draw_rect, DFC_MENU, DFCS_MENUCHECK);
	BitBlt(hdc, 0, 0, right - left, bottom - top, hdc, 0, 0, DSTINVERT);
	BitBlt(draw_dc, left, top, right, bottom, hdc, 0, 0, SRCPAINT);

	// チェックマークの描画
	hBrush = CreateSolidBrush(GetTextColor(draw_dc));
	FillRect(hdc, &draw_rect, hBrush);
	DeleteObject(hBrush);
	DrawFrameControl(wk_dc, &draw_rect, DFC_MENU, DFCS_MENUCHECK);
	BitBlt(hdc, 0, 0, right - left, bottom - top, wk_dc, 0, 0, SRCPAINT);
	BitBlt(draw_dc, left, top, right, bottom, hdc, 0, 0, SRCAND);

	SelectObject(hdc, ret_hbmp);
	DeleteObject(hbmp);
	DeleteDC(hdc);
	SelectObject(wk_dc, wk_ret_hbmp);
	DeleteObject(wk_hbmp);
	DeleteDC(wk_dc);
	return TRUE;
}

/*
 * menu_drawitem - メニュー項目を描画
 */
BOOL menu_drawitem(const DRAWITEMSTRUCT *ds)
{
	MENU_ITEM_INFO *mii;
	HDC draw_dc;
	HBITMAP hDrawBmp, hrBmp;
	HANDLE hBrush;
	HFONT hFont, hRetFont;
	HPEN hPen, hRetPen;
	RECT draw_rect;
	SIZE sz;
	int left_margin;
	int width, height;
#ifdef MENU_COLOR
	DWORD menu_color_back = (*option.menu_color_back.color_str != TEXT('\0')) ?
		option.menu_color_back.color : GetSysColor(COLOR_MENU);
	DWORD menu_color_text = (*option.menu_color_text.color_str != TEXT('\0')) ?
		option.menu_color_text.color : GetSysColor(COLOR_MENUTEXT);
	DWORD menu_color_highlight = (*option.menu_color_highlight.color_str != TEXT('\0')) ?
		option.menu_color_highlight.color : GetSysColor(COLOR_HIGHLIGHT);
	DWORD menu_color_highlighttext = (*option.menu_color_highlighttext.color_str != TEXT('\0')) ?
		option.menu_color_highlighttext.color : GetSysColor(COLOR_HIGHLIGHTTEXT);
	DWORD menu_color_3d_shadow = (*option.menu_color_3d_shadow.color_str != TEXT('\0')) ?
		option.menu_color_3d_shadow.color : GetSysColor(COLOR_3DSHADOW);
	DWORD menu_color_3d_highlight = (*option.menu_color_3d_highlight.color_str != TEXT('\0')) ?
		option.menu_color_3d_highlight.color : GetSysColor(COLOR_3DHIGHLIGHT);
#else	// MENU_COLOR
	DWORD menu_color_back = GetSysColor(COLOR_MENU);
	DWORD menu_color_text = GetSysColor(COLOR_MENUTEXT);
	DWORD menu_color_highlight = GetSysColor(COLOR_HIGHLIGHT);
	DWORD menu_color_highlighttext = GetSysColor(COLOR_HIGHLIGHTTEXT);
	DWORD menu_color_3d_shadow = GetSysColor(COLOR_3DSHADOW);
	DWORD menu_color_3d_highlight = GetSysColor(COLOR_3DHIGHLIGHT);
#endif	// MENU_COLOR

	mii = (MENU_ITEM_INFO *)ds->itemData;

	width = ds->rcItem.right - ds->rcItem.left;
	height = ds->rcItem.bottom - ds->rcItem.top;

	// 描画用DCの作成
	if ((draw_dc = CreateCompatibleDC(ds->hDC)) == NULL) {
		return FALSE;
	}
	if ((hDrawBmp = CreateCompatibleBitmap(ds->hDC, width, height)) == NULL) {
		DeleteDC(draw_dc);
		return FALSE;
	}
	hrBmp = SelectObject(draw_dc, hDrawBmp);

	// 背景
	SetRect(&draw_rect, 0, 0, width, height);
	if (ds->itemState & ODS_SELECTED) {
		hBrush = CreateSolidBrush(menu_color_highlight);
		FillRect(draw_dc, &draw_rect, hBrush);
		DeleteObject(hBrush);

		SetTextColor(draw_dc, menu_color_highlighttext);
		SetBkColor(draw_dc, menu_color_highlight);
	} else {
		hBrush = CreateSolidBrush(menu_color_back);
		FillRect(draw_dc, &draw_rect, hBrush);
		DeleteObject(hBrush);

		if (mii->show_format == TRUE) {
			SetTextColor(draw_dc, menu_color_highlight);
		} else {
			SetTextColor(draw_dc, menu_color_text);
		}
		SetBkColor(draw_dc, menu_color_back);
	}

	if (option.menu_show_icon == 1) {
		left_margin = -1;
		if (mii->show_bitmap == TRUE &&
			mii->show_di->menu_bitmap != NULL) {
			// ビットマップ
			left_margin = menu_draw_bitmap(draw_dc, mii->show_di, height);
		}
		if (left_margin == -1) {
			// アイコン
			if (mii->icon != NULL) {
				DrawIconEx(draw_dc, option.menu_icon_margin,
					height / 2 - option.menu_icon_size / 2, mii->icon,
					option.menu_icon_size, option.menu_icon_size, 0, NULL, DI_NORMAL);
			} else if (mii->flag & MF_CHECKED) {
				menu_draw_ckeck(draw_dc, option.menu_icon_margin,
					height / 2 - option.menu_icon_size / 2,
					option.menu_icon_size, option.menu_icon_size);
			}
			left_margin = option.menu_icon_margin + option.menu_icon_size;
		}
	} else {
		if (mii->flag & MF_CHECKED) {
			menu_draw_ckeck(draw_dc, option.menu_icon_margin, 0,
				option.menu_icon_size, height);
		}
		left_margin = option.menu_icon_margin + GetSystemMetrics(SM_CXMENUCHECK);
	}

	if (mii->text != NULL) {
		// テキスト
		hFont = menu_create_font();
		hRetFont = SelectObject(draw_dc, hFont);

		left_margin += option.menu_text_margin_left;
		SetRect(&draw_rect, left_margin, 0, width - option.menu_text_margin_right, height);
		if (mii->hkey == NULL) {
			DrawText(draw_dc,
				mii->text, lstrlen(mii->text),
				&draw_rect, DT_VCENTER | DT_SINGLELINE | DT_NOCLIP | DT_WORD_ELLIPSIS);
		} else {
			GetTextExtentPoint32(draw_dc, mii->hkey, lstrlen(mii->hkey), &sz);
			draw_rect.right -= (sz.cx + 10);
			DrawText(draw_dc,
				mii->text, lstrlen(mii->text),
				&draw_rect, DT_VCENTER | DT_SINGLELINE | DT_NOCLIP | DT_WORD_ELLIPSIS);
			// ホットキー表示
			if (!(ds->itemState & ODS_SELECTED) && mii->show_format == TRUE) {
				SetTextColor(draw_dc, menu_color_text);
			}
			draw_rect.right = width - option.menu_text_margin_right;
			DrawText(draw_dc,
				mii->hkey, lstrlen(mii->hkey),
				&draw_rect, DT_VCENTER | DT_SINGLELINE | DT_NOCLIP | DT_RIGHT);
		}
		SelectObject(draw_dc, hRetFont);
		DeleteObject(hFont);

	} else if (mii->flag & MF_SEPARATOR) {
		// 区切り
		hPen = CreatePen(PS_SOLID, 1, menu_color_3d_shadow);
		hRetPen = SelectObject(draw_dc, hPen);
		MoveToEx(draw_dc, option.menu_separator_margin_left,
			option.menu_separator_height / 2 - 1, NULL);
		LineTo(draw_dc, width - option.menu_separator_margin_right,
			option.menu_separator_height / 2 - 1);
		SelectObject(draw_dc, hRetPen);
		DeleteObject(hPen);

		hPen = CreatePen(PS_SOLID, 1, menu_color_3d_highlight);
		hRetPen = SelectObject(draw_dc, hPen);
		MoveToEx(draw_dc, option.menu_separator_margin_left,
			option.menu_separator_height / 2, NULL);
		LineTo(draw_dc, width - option.menu_separator_margin_right,
			option.menu_separator_height / 2);
		SelectObject(draw_dc, hRetPen);
		DeleteObject(hPen);
	}

	// メニューに描画
	BitBlt(ds->hDC,
		ds->rcItem.left, ds->rcItem.top,
		ds->rcItem.right, ds->rcItem.bottom,
		draw_dc, 0, 0, SRCCOPY);

	SelectObject(draw_dc, hrBmp);
	DeleteObject(hDrawBmp);
	DeleteDC(draw_dc);
	return TRUE;
}

/*
 * menu_get_accelerator - メニューのアクセラレータキーを取得
 */
static TCHAR menu_get_accelerator(TCHAR *str)
{
	TCHAR ret = TEXT('\0');
	TCHAR *p;

	for (p = str; *p != TEXT('\0'); p++) {
#ifndef UNICODE
		if (IsDBCSLeadByte((BYTE)*p) == TRUE) {
			p++;
			continue;
		}
#endif
		if (*p != TEXT('&')) {
			continue;
		}
		if (*(p + 1) == TEXT('&')) {
			p++;
		} else {
			// アクセラレータキー
			ret = *(p + 1);
		}
	}
	return ret;
}

/*
 * menu_accelerator - メニューアクセラレータ
 */
LRESULT menu_accelerator(const HMENU hMenu, const TCHAR key)
{
#define ToLower(c)		((c >= TEXT('A') && c <= TEXT('Z')) ? (c - TEXT('A') + TEXT('a')) : c)
	MENUITEMINFO mii;
	int i, sel;
	int cnt;
	int ret = -1;

	cnt = GetMenuItemCount(hMenu);

	// 選択位置取得
	for (sel = 0; sel < cnt; sel++) {
		if (GetMenuState(hMenu, sel, MF_BYPOSITION) & MF_HILITE) {
			break;
		}
	}
	if (sel >= cnt) {
		sel = -1;
	}

	// アクセラレータ位置取得
	for (i = sel + 1; i < cnt; i++) {
		ZeroMemory(&mii, sizeof(mii));
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_TYPE | MIIM_DATA;
		if (GetMenuItemInfo(hMenu, i, TRUE, &mii) == 0 ||
			!(mii.fType & MFT_OWNERDRAW) ||
			mii.dwItemData == 0 || 
			((MENU_ITEM_INFO *)mii.dwItemData)->text == NULL) {
			continue;
		}
		if (ToLower(menu_get_accelerator(((MENU_ITEM_INFO *)mii.dwItemData)->text)) != ToLower(key)) {
			continue;
		}
		if (ret != -1) {
			// 選択
			return MAKELRESULT(ret, MNC_SELECT);
		}
		ret = i;
	}
	for (i = 0; i <= sel; i++) {
		ZeroMemory(&mii, sizeof(mii));
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_TYPE | MIIM_DATA;
		if (GetMenuItemInfo(hMenu, i, TRUE, &mii) == 0 ||
			!(mii.fType & MFT_OWNERDRAW) ||
			mii.dwItemData == 0 || 
			((MENU_ITEM_INFO *)mii.dwItemData)->text == NULL) {
			continue;
		}
		if (ToLower(menu_get_accelerator(((MENU_ITEM_INFO *)mii.dwItemData)->text)) != ToLower(key)) {
			continue;
		}
		if (ret != -1) {
			// 選択
			return MAKELRESULT(ret, MNC_SELECT);
		}
		ret = i;
	}
	return ((ret != -1) ? MAKELRESULT(ret, MNC_EXECUTE) : 0);
}
/* End of source */
