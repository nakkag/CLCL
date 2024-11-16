/*
 * tool_test
 *
 * tool_test.c
 *
 * Copyright (C) 1996-2003 by Nakashima Tomoaki. All rights reserved.
 *		http://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE

#include "..\CLCLPlugin.h"

/* Define */

/* Global Variables */
HINSTANCE hInst;

/* Local Function Prototypes */

/*
 * DllMain - ���C��
 */
int WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved)
{
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		hInst = hInstance;
		break;

	case DLL_PROCESS_DETACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}

/*
 * get_tool_info_w - �c�[�����擾
 * Get tool information
 *
 *	���� / argument:
 *		hWnd - �Ăяo�����E�B���h�E / the calling window
 *		index - �擾�̃C���f�b�N�X (0�`) / the index of the acquisition (from 0)
 *		tgi - �c�[���擾��� / tool retrieval information
 *
 *	�߂�l / Return value:
 *		TRUE - ���Ɏ擾����c�[������ / has tools to get next
 *		FALSE - �擾�̏I�� / end of acquisition
 */
__declspec(dllexport) BOOL CALLBACK get_tool_info_w(const HWND hWnd, const int index, TOOL_GET_INFO *tgi)
{
	switch (index) {
	case 0:
		lstrcpy(tgi->title, TEXT("Remove specific format in item"));
		lstrcpy(tgi->func_name, TEXT("test1"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_MENU | CALLTYPE_VIEWER;
		return TRUE;

	case 1:
		lstrcpy(tgi->title, TEXT("Delete item"));
		lstrcpy(tgi->func_name, TEXT("test2"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_MENU | CALLTYPE_VIEWER;
		return TRUE;

	case 2:
		lstrcpy(tgi->title, TEXT("Save item"));
		lstrcpy(tgi->func_name, TEXT("test3"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_MENU | CALLTYPE_VIEWER;
		return TRUE;

	case 3:
		lstrcpy(tgi->title, TEXT("Create a text item from a file"));
		lstrcpy(tgi->func_name, TEXT("test4"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_MENU | CALLTYPE_VIEWER;
		return TRUE;

	case 4:
		lstrcpy(tgi->title, TEXT("Show item title"));
		lstrcpy(tgi->func_name, TEXT("test5"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_MENU | CALLTYPE_VIEWER;
		return TRUE;

	case 5:
		lstrcpy(tgi->title, TEXT("Save the data in a separate file"));
		lstrcpy(tgi->func_name, TEXT("test6"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_START | CALLTYPE_END;
		return TRUE;

	case 6:
		lstrcpy(tgi->title, TEXT("Test7"));
		lstrcpy(tgi->func_name, TEXT("test7"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_MENU | CALLTYPE_VIEWER;
		return TRUE;
	}
	return FALSE;
}

/*
 * test1 - �A�C�e�����̓���`�����폜
 * test1 - Remove specific format in item 
 *
 *	���� / argument:
 *		hWnd - �Ăяo�����E�B���h�E / the calling window
 *		tei - �c�[�����s��� / tool execution information
 *		tdi - �c�[���p�A�C�e����� / item information for tools
 *
 *	�߂�l / Return value:
 *		TOOL_
 */
__declspec(dllexport) int CALLBACK test1(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	DATA_INFO *cdi;
	DATA_INFO *pdi;

	for (; tdi != NULL; tdi = tdi->next) {
		if (tdi->di->type == TYPE_ITEM) {
			// �`���̌���
			// Format search 
			pdi = NULL;
			for (cdi = tdi->di->child; cdi != NULL && lstrcmpi(cdi->format_name, TEXT("TEXT")) != 0; cdi = cdi->next) {
				pdi = cdi;
			}
			if (cdi == NULL) {
				continue;
			}
			// �`�������X�g����폜
			// Remove format from list
			if (pdi == NULL) {
				tdi->di->child = cdi->next;
			} else {
				pdi->next = cdi->next;
			}
			cdi->next = NULL;
			// �A�C�e���̉��
			// Release items 
			SendMessage(hWnd, WM_ITEM_FREE, 0, (LPARAM)cdi);
		}
	}
	// �A�C�e���̕ω���ʒm
	// Notify item changes 
	if (tei->call_type & CALLTYPE_HISTORY) {
		SendMessage(hWnd, WM_HISTORY_CHANGED, 0, 0);
	} else if (tei->call_type & CALLTYPE_REGIST) {
		SendMessage(hWnd, WM_REGIST_CHANGED, 0, 0);
	}
	return TOOL_SUCCEED;
}

/*
 * test2 - �A�C�e���̍폜
 * test2 - Delete item
 */
__declspec(dllexport) int CALLBACK test2(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	DATA_INFO *di;

	if ((di = (DATA_INFO *)SendMessage(hWnd, WM_ITEM_GET_PARENT, 0, (LPARAM)tdi->di)) == NULL) {
		return TOOL_SUCCEED;
	}

	// �A�C�e�������X�g����폜
	// Remove item from list
	if (di->child == tdi->di) {
		di->child = tdi->di->next;
	} else {
		for (di = di->child; di != NULL && di->next != tdi->di; di = di->next)
			;
		if (di == NULL) {
			return TOOL_SUCCEED;
		}
		di->next = tdi->di->next;
	}
	tdi->di->next = NULL;
	// �A�C�e���̉��
	// Release items 
	SendMessage(hWnd, WM_ITEM_FREE, 0, (LPARAM)tdi->di);

	// �A�C�e���̕ω���ʒm
	// Notify item changes 
	if (tei->call_type & CALLTYPE_HISTORY) {
		SendMessage(hWnd, WM_HISTORY_CHANGED, 0, 0);
	} else if (tei->call_type & CALLTYPE_REGIST) {
		SendMessage(hWnd, WM_REGIST_CHANGED, 0, 0);
	}
	return TOOL_SUCCEED;
}

/*
 * test3 - �A�C�e���̕ۑ�
 * test3 - Save item
 */
__declspec(dllexport) int CALLBACK test3(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	OPENFILENAME of;
	TCHAR file_name[MAX_PATH];
	DATA_INFO *di;

	if ((di = (DATA_INFO *)SendMessage(hWnd, WM_ITEM_GET_PRIORITY_HIGHEST, 0, (LPARAM)tdi->di)) == NULL) {
		return TOOL_SUCCEED;
	}
	ZeroMemory(&of, sizeof(OPENFILENAME));
	of.lStructSize = sizeof(OPENFILENAME);
	of.hInstance = hInst;
	of.hwndOwner = hWnd;
	of.lpstrFilter = TEXT("*.*\0*.*\0\0");
	of.nFilterIndex = 1;
	*file_name = TEXT('\0');
	of.lpstrFile = file_name;
	of.nMaxFile = MAX_PATH - 1;
	of.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	// �ۑ����擾�ƃt�@�C�����I��
	// Get saved information and select file name
	if (SendMessage(hWnd, WM_ITEM_GET_SAVE_INFO, (WPARAM)&of, (LPARAM)di) >= 0 && GetSaveFileName(&of) == FALSE) {
		return TOOL_CANCEL;
	}
	// �A�C�e�����t�@�C���ɕۑ�
	// Save item to file
	SendMessage(hWnd, WM_ITEM_TO_FILE, (WPARAM)file_name, (LPARAM)di);
	return TOOL_SUCCEED;
}

/*
 * test4 - �t�@�C������e�L�X�g�`���̃A�C�e�����쐬
 * test4 - Create a text item from a file
 */
__declspec(dllexport) int CALLBACK test4(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	OPENFILENAME of;
	TCHAR file_name[MAX_PATH];
	DATA_INFO *history_di;
	DATA_INFO *di;

	// �����̎擾
	// Get history
	if ((history_di = (DATA_INFO *)SendMessage(hWnd, WM_HISTORY_GET_ROOT, 0, 0)) == NULL) {
		return TOOL_SUCCEED;
	}

	ZeroMemory(&of, sizeof(OPENFILENAME));
	of.lStructSize = sizeof(OPENFILENAME);
	of.hInstance = hInst;
	of.hwndOwner = hWnd;
	of.lpstrFilter = TEXT("*.*\0*.*\0\0");
	of.nFilterIndex = 1;
	*file_name = TEXT('\0');
	of.lpstrFile = file_name;
	of.nMaxFile = MAX_PATH - 1;
	of.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	// �ۑ����擾�ƃt�@�C�����I��
	// Get saved information and select file name
	if (SendMessage(hWnd, WM_ITEM_GET_OPEN_INFO, (WPARAM)&of, (LPARAM)TEXT("TEXT")) >= 0 && GetOpenFileName(&of) == FALSE) {
		return TOOL_CANCEL;
	}
	// �A�C�e���̍쐬
	// Creating an item
	if ((di = (DATA_INFO *)SendMessage(hWnd, WM_ITEM_CREATE, TYPE_ITEM, 0)) == NULL) {
		return TOOL_CANCEL;
	}
	if ((di->child = (DATA_INFO *)SendMessage(hWnd, WM_ITEM_CREATE, TYPE_DATA, (LPARAM)TEXT("TEXT"))) == NULL) {
		SendMessage(hWnd, WM_ITEM_FREE, 0, (LPARAM)di);
		return TOOL_CANCEL;
	}
	// �t�@�C����ǂݍ���ŃA�C�e���ɐݒ�
	// Load a file and set it as an item
	SendMessage(hWnd, WM_ITEM_FROM_FILE, (WPARAM)file_name, (LPARAM)di->child);

	// �����ɒǉ�
	// Add to history
	di->next = history_di->child;
	history_di->child = di;
	// �����̕ω���ʒm
	// Notify history changes
	SendMessage(hWnd, WM_HISTORY_CHANGED, 0, 0);
	return TOOL_SUCCEED;
}

/*
 * test5 - �A�C�e���̃^�C�g����\��
 * test5 - Show item title
 */
__declspec(dllexport) int CALLBACK test5(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	DATA_INFO *di;
	TCHAR buf[BUF_SIZE];

	// �r���[�A�̑I���A�C�e���擾
	// Get selected items in the viewer
	if ((di = (DATA_INFO *)SendMessage(hWnd, WM_VIEWER_GET_SELECTION, 0, 0)) != NULL) {
		// �I���A�C�e���̃^�C�g����\��
		// Show title of selected item
		SendMessage(hWnd, WM_ITEM_GET_TITLE, (WPARAM)buf, (LPARAM)di);
		MessageBox(hWnd, buf, TEXT("sel item"), 0);
	}

	// �����̎擾
	// Get history
	if ((di = (DATA_INFO *)SendMessage(hWnd, WM_HISTORY_GET_ROOT, 0, 0)) == NULL) {
		return TOOL_SUCCEED;
	}
	for (di = di->child; di != NULL; di = di->next) {
		// �A�C�e���̃^�C�g�����擾
		// Get the title of the item
		SendMessage(hWnd, WM_ITEM_GET_TITLE, (WPARAM)buf, (LPARAM)di);
		if (MessageBox(hWnd, buf, TEXT("title"), MB_OKCANCEL) == IDCANCEL) {
			// �r���[�A�ŃA�C�e����I����Ԃɂ���
			// Select an item in the viewer 
			SendMessage(hWnd, WM_VIEWER_SELECT_ITEM, 0, (LPARAM)di);
			break;
		}
	}
	return TOOL_SUCCEED;
}

/*
 * test6 - �f�[�^��ʃt�@�C���ɕۑ�
 * test6 - Save the data in a separate file
 */
__declspec(dllexport) int CALLBACK test6(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	DATA_INFO *di;
	TCHAR buf[BUF_SIZE];
	TCHAR test_filename[BUF_SIZE];
	TCHAR history_filename[BUF_SIZE];

	SendMessage(hWnd, WM_GET_WORKPATH, 0, (LPARAM)buf);
	wsprintf(test_filename, TEXT("%s\\test.dat"), buf);
	wsprintf(history_filename, TEXT("%s\\history.dat"), buf);

	if (tei->call_type & CALLTYPE_START) {
		if (CopyFile(test_filename, history_filename, FALSE) == TRUE) {
			SendMessage(hWnd, WM_HISTORY_LOAD, 0, 0);
		}

	} else if (tei->call_type & CALLTYPE_END) {
		SendMessage(hWnd, WM_HISTORY_SAVE, 0, 0);
		CopyFile(history_filename, test_filename, FALSE);
		if ((di = (DATA_INFO *)SendMessage(hWnd, WM_HISTORY_GET_ROOT, 0, 0)) != NULL) {
			SendMessage(hWnd, WM_ITEM_FREE, 0, (LPARAM)di->child);
			di->child = NULL;
			SendMessage(hWnd, WM_HISTORY_CHANGED, 0, 0);
		}
	}
	return TOOL_SUCCEED;
}

/*
 * test7 - �e�X�g
 * test7 - Test
 */
__declspec(dllexport) int CALLBACK test7(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	return TOOL_SUCCEED;
}
/* End of source */
