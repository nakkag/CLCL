/*
 * CLCL
 *
 * Message.h
 *
 * Copyright (C) 1996-2003 by Nakashima Tomoaki. All rights reserved.
 *		http://www.nakka.com/
 *		nakka@nakka.com
 */

#ifndef _INC_MESSAGE_H
#define _INC_MESSAGE_H

/* Include Files */

/* Define */

/* Struct */

/* Function Prototypes */
BOOL message_get_error(const int err_code, TCHAR *err_str);
TCHAR *message_get_res(const UINT id);

#endif
/* End of source */
