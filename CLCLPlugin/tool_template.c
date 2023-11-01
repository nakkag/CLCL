/*
 * CLCL
 *
 * tool_template.c
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
		lstrcpy(tgi->title, TEXT("�^�C�g��"));
		lstrcpy(tgi->func_name, TEXT("func_tool"));
		lstrcpy(tgi->cmd_line, TEXT(""));
		tgi->call_type = CALLTYPE_MENU | CALLTYPE_VIEWER;	// CALLTYPE_
		return TRUE;

	case 1:
		return FALSE;
	}
	return FALSE;
}

/*
 * func_tool - �c�[������
 * Tool processing
 *
 *	���� / argument:
 *		hWnd - �Ăяo�����E�B���h�E / the calling window
 *		tei - �c�[�����s��� / tool execution information
 *		tdi - �c�[���p�A�C�e����� / item information for tools
 *
 *	�߂�l / Return value:
 *		TOOL_
 */
__declspec(dllexport) int CALLBACK func_tool(const HWND hWnd, TOOL_EXEC_INFO *tei, TOOL_DATA_INFO *tdi)
{
	return TOOL_SUCCEED;
}

/*
 * func_tool_property - �v���p�e�B�\��
 * Show properties
 *
 *	���� / argument:
 *		hWnd - �I�v�V�����E�B���h�E�̃n���h�� / handle of the options window
 *		tei - �c�[�����s��� / tool execution information
 *
 *	�߂�l / Return value:
 *		TRUE - �v���p�e�B���� / with properties
 *		FALSE - �v���p�e�B�Ȃ� / no property
 */
__declspec(dllexport) BOOL CALLBACK func_tool_property(const HWND hWnd, TOOL_EXEC_INFO *tei)
{
	return FALSE;
}
/* End of source */
