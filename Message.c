/*
 * CLCL
 *
 * Message.c
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */

#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE

#include "General.h"
#include "Memory.h"
#include "String.h"

/* Define */

/* Global Variables */

extern HINSTANCE hInst;

/* Local Function Prototypes */

/*
 * message_get_error - エラー値からメッセージを取得
 */
BOOL message_get_error(const int err_code, TCHAR *err_str)
{
	if (err_str == NULL) {
		return FALSE;
	}
	*err_str = TEXT('\0');
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err_code, 0, err_str, BUF_SIZE - 1, NULL);
	return TRUE;
}

/*
 * message_get_res - リソースからメッセージを取得
 */
TCHAR *message_get_res(const UINT id)
{
	static TCHAR buf[BUF_SIZE];

	LoadString(hInst, id, buf, BUF_SIZE - 1);
	return buf;
}
/* End of source */
