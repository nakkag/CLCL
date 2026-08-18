/*
 * CLCL
 *
 * Search.h
 *
 * Search popup for clipboard history
 */

#ifndef _INC_SEARCH_H
#define _INC_SEARCH_H

/* Include Files */
#include "Data.h"

/* Define */
#define SEARCH_WND_CLASS				TEXT("CLCLSearch")

/* Function Prototypes */
BOOL search_regist(const HINSTANCE hInstance);
DATA_INFO *search_popup_show(const HWND hWnd, DATA_INFO *history_root);

#endif
/* End of source */
