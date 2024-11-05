/*
 * CLCL
 *
 * Data.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_DATA_H
#define _INC_DATA_H

/* Include Files */

/* Define */
// data type
#define TYPE_DATA						0
#define TYPE_ITEM						1
#define TYPE_FOLDER						2
#define TYPE_ROOT						3

/* Struct */
// �A�C�e�����
// Item information
typedef struct _DATA_INFO {
	DWORD struct_size;					// �\���̂̃T�C�Y / Structure size

	int type;							// TYPE_
	TCHAR *title;						// �^�C�g�� / title

	TCHAR *format_name;					// �`���� / format name
	int format_name_hash;				// �`�����̃n�b�V�� / format name hash
	UINT format;						// �`���l / format value

	HANDLE data;						// �f�[�^ / data
	DWORD size;							// �T�C�Y / size

	FILETIME modified;					// �X�V���� / timestamp of last update
	TCHAR *window_name;					// �R�s�[�����E�B���h�E�^�C�g�� / copy of window title

	TCHAR *plugin_string;				// �v���O�C���p�f�[�^ / data for plugins
	LPARAM plugin_param;

// �ȉ��ۑ����Ȃ����
// not persistent Information below
	TCHAR *menu_title;					// ���j���[�ɕ\������^�C�g�� (���ݒ�̏ꍇ�͌`����\��) / title to display on menu (if not set, display format)
	BOOL free_title;					// �^�C�g���� TRUE-������� FALSE-������Ȃ� / title TRUE: release, FALSE: do not release
	HICON menu_icon;					// ���j���[�ɕ\������A�C�R���n���h�� / icon handle to display in menu
	BOOL free_icon;						// �A�C�R���n���h���� TRUE-������� FALSE-������Ȃ� / TRUE: release the icon handle, FALSE: do not release the icon handle
	HBITMAP menu_bitmap;				// ���j���[�ɕ\������r�b�g�}�b�v / bitmap to display in menu
	BOOL free_bitmap;					// �r�b�g�}�b�v�n���h���� TRUE-������� FALSE-������Ȃ� / TRUE: release bitmap handle, FALSE: do not release
	int menu_bmp_width;					// ���j���[�ɕ\������r�b�g�}�b�v�̌ʃT�C�Y / individual size of bitmap displayed in menu
	int menu_bmp_height;
	LPARAM param1;						// �v���O�C���p�f�[�^ / data for plugins
	LPARAM param2;

	struct _DATA_INFO *child;
	struct _DATA_INFO *next;

// Ver 1.0.5
	int hkey_id;						// �z�b�g�L�[ / hotkey
	UINT op_modifiers;
	UINT op_virtkey;
	int op_paste;
} DATA_INFO;

/* Function Prototypes */
DATA_INFO *data_create_data(const UINT format, TCHAR *format_name, const HANDLE data, const DWORD size, const BOOL init, TCHAR *err_str);
DATA_INFO *data_create_item(const TCHAR *title, const BOOL set_date, TCHAR *err_str);
DATA_INFO *data_create_folder(const TCHAR *title, TCHAR *err_str);
DATA_INFO *data_item_copy(const DATA_INFO *di, const BOOL next_copy, const BOOL move_flag, TCHAR *err_str);
BOOL data_delete(DATA_INFO **root, DATA_INFO *del_di, const BOOL free_item);
void data_adjust(DATA_INFO **root);
void data_menu_free_item(DATA_INFO *di);
void data_menu_free(DATA_INFO *di);
void data_free(DATA_INFO *di);
DATA_INFO *data_check(DATA_INFO *di, const DATA_INFO *check_di);
void data_set_modified(DATA_INFO *di);
BOOL data_get_modified_string(const DATA_INFO *di, TCHAR *ret);
TCHAR *data_get_title(DATA_INFO *di);

#endif
/* End of source */
