/*
 * CLCL
 *
 * Tool.h
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_TOOL_H
#define _INC_TOOL_H

/* Include Files */
#include "General.h"
#include "Data.h"

/* Define */
// �c�[�������s����^�C�~���O
// When to run the tool
#define CALLTYPE_MENU					1				// ���상�j���[ / Operation menu
#define CALLTYPE_VIEWER					2				// �r���[�A�̃��j���[ / Viewer menu
#define CALLTYPE_VIEWER_OPEN			4				// �r���[�A���J������ / when viewer is opened
#define CALLTYPE_VIEWER_CLOSE			8				// �r���[�A����鎞 / when viewer is closed
#define CALLTYPE_ADD_HISTORY			16				// �f�[�^�������ɒǉ�����鎞 / when data is added to history
#define CALLTYPE_ITEM_TO_CLIPBOARD		32				// �f�[�^���N���b�v�{�[�h�ɑ��鎞 / when sending data to the clipboard
#define CALLTYPE_START					64				// �N���� / at startup
#define CALLTYPE_END					128				// �I���� / when finished
// option only
#define CALLTYPE_MENU_COPY_PASTE		256				// �R�s�[�Ɠ\��t���𑗂� / send copy and paste
// execute only
#define CALLTYPE_HISTORY				512				// ��������̌Ăяo�� / call from history
#define CALLTYPE_REGIST					1024			// �o�^�A�C�e������̌Ăяo�� / call from registered item

// �c�[���߂�l
// tool return value
#define TOOL_ERROR						0				// �c�[���̃G���[ / tool error
#define TOOL_SUCCEED					1				// �c�[���̐���I�� / tool successfully completed
#define TOOL_CANCEL						2				// �ȍ~�̏������L�����Z�� / cancel further processing
#define TOOL_DATA_MODIFIED				4				// �f�[�^�ύX���� / data changed


// �c�[���Ăяo�����@ (��ver)
// Tool calling method (old ver)
#define OLD_CALLTYPE_VIEWER				0
#define OLD_CALLTYPE_ADD_HISTORY		1
#define OLD_CALLTYPE_ITEM_TO_CLIPBOARD	2
#define OLD_CALLTYPE_NOITEM				4
#define OLD_CALLTYPE_MENU				8
#define OLD_CALLTYPE_START				16
#define OLD_CALLTYPE_END				32

// ���c�[���̊֐��`��
// Function format of old tool 
typedef int (__cdecl *OLD_TOOL_FUNC)(HWND, void *, int, int);
typedef int (__cdecl *OLD_GET_FUNC)(int, TCHAR *, TCHAR *, long *);

/* Struct */
// �c�[�����
// Tool information
typedef struct _TOOL_INFO {
	TCHAR *title;
	TCHAR *lib_file_path;
	TCHAR *func_name;
	TCHAR *cmd_line;
	int call_type;						// CALLTYPE_
	int copy_paste;

	// hot key
	int id;
	UINT modifiers;
	UINT virtkey;

	HANDLE lib;
	FARPROC func;
	OLD_TOOL_FUNC old_func;

	int old;

	LPARAM lParam;						// �c�[���ɑΉ�����long�l / Long value corresponding to the tool
} TOOL_INFO;

// �c�[���擾���
// Tool acquisition information
typedef struct _TOOL_GET_INFO {
	DWORD struct_size;					// �\���̂̃T�C�Y / Structure size

	TCHAR title[BUF_SIZE];
	TCHAR func_name[BUF_SIZE];
	TCHAR cmd_line[BUF_SIZE];
	int call_type;						// CALLTYPE_
} TOOL_GET_INFO;
typedef struct _TOOL_GET_INFO_A {
	DWORD struct_size;					// �\���̂̃T�C�Y / Structure size

	char title[BUF_SIZE];
	char func_name[BUF_SIZE];
	char cmd_line[BUF_SIZE];
	int call_type;						// CALLTYPE_
} TOOL_GET_INFO_A;

// �c�[�����s���
// Tool execution information
typedef struct _TOOL_EXEC_INFO {
	DWORD struct_size;					// �\���̂̃T�C�Y / Structure size

	int call_type;						// CALLTYPE_
	TCHAR *cmd_line;					// �c�[���ݒ�Ŏw�肵���R�}���h���C�� / Command line specified in tool settings
	LPARAM lParam;						// �c�[���ɑΉ�����long�l / long value corresponding to the tool
} TOOL_EXEC_INFO;

// �c�[���p�A�C�e�����
// Item information for tools
typedef struct _TOOL_DATA_INFO {
	DWORD struct_size;					// �\���̂̃T�C�Y / Structure size

	struct _DATA_INFO *di;				// �A�C�e����� / Item information

	struct _TOOL_DATA_INFO *child;
	struct _TOOL_DATA_INFO *next;
} TOOL_DATA_INFO;

/* Function Prototypes */
int tool_title_to_index(const TCHAR *title);
BOOL tool_initialize(TCHAR *err_str);
TOOL_DATA_INFO *tool_data_copy(DATA_INFO *di, const BOOL next_copy);
void tool_data_free(TOOL_DATA_INFO *tdi);
int tool_execute(const HWND hWnd, TOOL_INFO *ti, const int call_type, DATA_INFO *di, TOOL_DATA_INFO *tdi);
int tool_execute_all(const HWND hWnd, const int call_type, DATA_INFO *di);

#endif
/* End of source */
