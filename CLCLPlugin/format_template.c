/*
 * CLCL
 *
 * format_template.c
 *
 * Copyright (C) 1996-2003 by Nakashima Tomoaki. All rights reserved.
 *		http://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE

#include "CLCLPlugin.h"

/* Define */

/* Global Variables */

/* Local Function Prototypes */

/*
 * get_format_header - 内部形式を処理するヘッダの取得
 *
 *	引数:
 *		hWnd - 呼び出し元ウィンドウ
 *		index - 取得のインデックス (0～)
 *		fgi - 形式取得情報
 *
 *	戻り値:
 *		TRUE - 次に取得する形式あり
 *		FALSE - 取得の終了
 */
__declspec(dllexport) BOOL CALLBACK get_format_header(const HWND hWnd, const int index, FORMAT_GET_INFO *fgi)
{
	switch (index) {
	case 0:
		lstrcpy(fgi->format_name, TEXT("FORMAT"));
		lstrcpy(fgi->func_header, TEXT("func_"));
		lstrcpy(fgi->comment, TEXT("コメント"));
		return TRUE;

	case 1:
		return FALSE;
	}
	return FALSE;
}

/*
 * func_show_property - プロパティ表示
 *
 *	引数:
 *		hWnd - オプションウィンドウのハンドル
 *
 *	戻り値:
 *		TRUE - プロパティあり
 *		FALSE - プロパティなし
 */
__declspec(dllexport) BOOL CALLBACK func_show_property(const HWND hWnd)
{
	return FALSE;
}

/*
 * func_initialize - 初期化
 *
 *	引数:
 *		なし
 *
 *	戻り値:
 *		TRUE - 初期化あり
 *		FALSE - 初期化なし
 */
__declspec(dllexport) BOOL CALLBACK func_initialize(void)
{
	return TRUE;
}

/*
 * func_get_icon - 形式用のアイコンを取得
 *
 *	引数:
 *		icon_size - 取得するアイコンのサイズ
 *		*free_icon - (OUT) TRUE - 解放する、FALSE - 解放しない
 *					 初期値は TRUE になっています。
 *
 *	戻り値:
 *		アイコンのハンドル
 *		NULL の場合はアイコンなし
 *		*free_icon に TRUE を設定するとアイコンのハンドルは本体側で解放されます
 */
__declspec(dllexport) HICON CALLBACK func_get_icon(const int icon_size, BOOL *free_icon)
{
	return NULL;
}

/*
 * func_free - 終了処理
 *
 *	引数:
 *		なし
 *
 *	戻り値:
 *		TRUE - 終了処理あり
 *		FALSE - 終了処理なし
 */
__declspec(dllexport) BOOL CALLBACK func_free(void)
{
	return TRUE;
}

/*
 * func_initialize_item - アイテム情報の初期化
 *	
 *	引数:
 *		di - 初期化するアイテム情報
 *		set_init_data - TRUE の時は di->data と di->size を設定可能です。
 *		                FALSE の時は設定しないでください。
 *
 *	戻り値:
 *		TRUE - 初期化処理あり
 *		FALSE - 初期化処理なし
 */
__declspec(dllexport) BOOL CALLBACK func_initialize_item(DATA_INFO *di, const BOOL set_init_data)
{
	return FALSE;
}

/*
 * func_copy_data - データのコピー
 *
 *	引数:
 *		format_name - 形式名
 *		data - コピーするデータ
 *		ret_size - データのサイズ (out)
 *
 *	戻り値:
 *		コピーしたデータ
 *		NULL の場合はコピー処理なし
 */
__declspec(dllexport) HANDLE CALLBACK func_copy_data(const TCHAR *format_name, const HANDLE data, DWORD *ret_size)
{
	return NULL;
}

/*
 * func_data_to_bytes - データをバイト列に変換
 *
 *	引数:
 *		di - 変換するアイテム情報
 *		ret_size - データのサイズ (out)
 *
 *	戻り値:
 *		変換したバイト列
 *		NULL の場合は変換処理なし
 */
__declspec(dllexport) BYTE* CALLBACK func_data_to_bytes(const DATA_INFO *di, DWORD *ret_size)
{
	return NULL;
}

/*
 * func_bytes_to_data - バイト列をデータに変換
 *
 *	引数:
 *		format_name - 形式名
 *		data - 変換するバイト列
 *		size - バイト列のサイズ (in/out)
 *		       値を設定するとデータのサイズになります。
 *
 *	戻り値:
 *		変換したデータ
 *		NULL の場合は変換処理なし
 */
__declspec(dllexport) HANDLE CALLBACK func_bytes_to_data(const TCHAR *format_name, const BYTE *data, DWORD *size)
{
	return NULL;
}

/*
 * func_get_file_info - コモンダイアログ情報の取得
 *
 *	引数:
 *		format_name - 形式名
 *		di - アイテム情報 (modeがTRUEの場合は NULL)
 *		of - コモンダイアログのファイル情報
 *		mode - TRUE - open、FALSE - save
 *
 *	戻り値:
 *		-1 - コモンダイアログを表示しない
 *		0 - 未設定
 *		1 - 設定済み
 */
__declspec(dllexport) int CALLBACK func_get_file_info(const TCHAR *format_name, const DATA_INFO *di, OPENFILENAME *of, const BOOL mode)
{
	return 0;
}

/*
 * func_data_to_file - データをファイルに保存
 *
 *	引数:
 *		di - 保存するアイテム情報
 *		file_name - ファイル名
 *		filter_index - ファイル選択時のフィルタインデックス
 *		               ファイル選択されていない場合は 0
 *		err_str - エラー文字列 (out)
 *
 *	戻り値:
 *		TRUE - 成功
 *		FALSE - 失敗 (err_str を空で返すと保存処理なし)
 */
__declspec(dllexport) BOOL CALLBACK func_data_to_file(DATA_INFO *di, const TCHAR *file_name, const int filter_index, TCHAR *err_str)
{
	return FALSE;
}

/*
 * func_file_to_data - ファイルからデータを作成
 *
 *	引数:
 *		file_name - ファイル名
 *		format_name - 形式名
 *		ret_size - データのサイズ (out)
 *		           NULLの場合あり
 *		err_str - エラー文字列 (out)
 *
 *	戻り値:
 *		読み込んだデータ
 *		NULL の場合は失敗 (err_str を空で返すと読み込み処理なし)
 */
__declspec(dllexport) HANDLE CALLBACK func_file_to_data(const TCHAR *file_name, const TCHAR *format_name, DWORD *ret_size, TCHAR *err_str)
{
	return NULL;
}

/*
 * func_free_data - データの解放
 *
 *	引数:
 *		format_name - 形式名
 *		data - 解放するデータ
 *
 *	戻り値:
 *		TRUE - 解放済み
 *		FALSE - 解放処理なし
 */
__declspec(dllexport) BOOL CALLBACK func_free_data(const TCHAR *format_name, HANDLE data)
{
	return FALSE;
}

/*
 * func_free_item - アイテム情報の解放
 *
 *	plugin_param, param1, param2 に設定したメモリの解放などをしてください
 *	その他の情報を解放した場合はメンバにNULLを設定してください
 *
 *	引数:
 *		di - アイテム情報
 *
 *	戻り値:
 *		TRUE - 解放処理あり
 *		FALSE - 解放処理なし
 */
__declspec(dllexport) BOOL CALLBACK func_free_item(DATA_INFO *di)
{
	return FALSE;
}

/*
 * func_get_menu_title - メニュータイトルの取得
 *
 *	引数:
 *		di - アイテム情報 (menu_title と free_title を編集)
 *
 *	戻り値:
 *		TRUE - タイトルあり
 *		FALSE - タイトルなし
 */
__declspec(dllexport) BOOL CALLBACK func_get_menu_title(DATA_INFO *di)
{
	return FALSE;
}

/*
 * func_get_menu_icon - メニュー用アイコンの取得
 *
 *	引数:
 *		di - アイテム情報 (menu_icon と free_icon を編集)
 *		icon_size - メニューに表示するアイコンのサイズ
 *
 *	戻り値:
 *		TRUE - アイコンあり
 *		FALSE - アイコンなし
 */
__declspec(dllexport) BOOL CALLBACK func_get_menu_icon(DATA_INFO *di, const int icon_size)
{
	return FALSE;
}

/*
 * func_get_menu_bitmap - メニュー用ビットマップの取得
 *
 *	引数:
 *		di - アイテム情報 (menu_bitmap と free_bitmap を編集)
 *		width - ビットマップの横幅
 *		height - ビットマップの縦幅
 *
 *	戻り値:
 *		TRUE - ビットマップあり
 *		FALSE - ビットマップなし
 *
 *	個別にサイズを指定する場合は di->menu_bmp_width と di->menu_bmp_height に
 *	ビットマップのサイズを設定する。
 */
__declspec(dllexport) BOOL CALLBACK func_get_menu_bitmap(DATA_INFO *di, const int width, const int height)
{
	return FALSE;
}

/*
 * func_get_tooltip_text - メニュー用ツールチップテキストの取得
 *	
 *	引数:
 *		di - アイテム情報
 *
 *	戻り値:
 *		ツールチップに表示するテキスト
 *		NULL の場合はツールチップを表示しない
 */
__declspec(dllexport) TCHAR* CALLBACK func_get_tooltip_text(DATA_INFO *di)
{
	return NULL;
}

/*
 * func_window_create - データ表示ウィンドウの作成
 *
 *	引数:
 *		parent_wnd - 親ウィンドウ
 *
 *	戻り値:
 *		作成したウィンドウハンドル
 *		NULL の場合はウィンドウなし
 */
__declspec(dllexport) HWND CALLBACK func_window_create(const HWND parent_wnd)
{
	return NULL;
}

/*
 * func_window_destroy - データ表示ウィンドウの破棄
 *
 *	引数:
 *		hWnd - データ表示ウィンドウ
 *
 *	戻り値:
 *		TRUE - 破棄処理あり
 *		FALSE - 破棄処理なし
 */
__declspec(dllexport) BOOL CALLBACK func_window_destroy(const HWND hWnd)
{
	return FALSE;
}

/*
 * func_window_show_data - データの表示
 *
 *	引数:
 *		hWnd - データ表示ウィンドウ
 *		di - 表示するアイテム情報
 *		lock - TRUE - 変更不可、FALSE - 変更可
 *
 *	戻り値:
 *		TRUE - データ表示処理あり
 *		FALSE - データ表示処理なし
 */
__declspec(dllexport) BOOL CALLBACK func_window_show_data(const HWND hWnd, DATA_INFO *di, const BOOL lock)
{
	return FALSE;
}

/*
 * func_window_save_data - データの保存
 *
 *	引数:
 *		hWnd - データ表示ウィンドウ
 *		di - 保存するアイテム情報
 *
 *	戻り値:
 *		TRUE - データ保存処理あり
 *		FALSE - データ保存処理なし
 */
__declspec(dllexport) BOOL CALLBACK func_window_save_data(const HWND hWnd, DATA_INFO *di)
{
	return FALSE;
}

/*
 * func_window_hide_data - データの非表示
 *
 *	引数:
 *		hWnd - データ表示ウィンドウ
 *		di - 非表示にするアイテム情報
 *
 *	戻り値:
 *		TRUE - データ非表示処理あり
 *		FALSE - データ非表示処理なし
 */
__declspec(dllexport) BOOL CALLBACK func_window_hide_data(const HWND hWnd, DATA_INFO *di)
{
	return FALSE;
}
/* End of source */
