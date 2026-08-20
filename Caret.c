/*
 * CLCL
 *
 * Caret.c
 *
 * Copyright (C) 1996-2026 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#include <windows.h>
#include <tchar.h>
#define COBJMACROS
#include <oleacc.h>
#include <uiautomation.h>

#include "General.h"
#include "Caret.h"

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "uuid.lib")

/* Define */
// ハングしているウィンドウの判定時間
#define CARET_RESPONSE_TIMEOUT			200

#ifndef UIA_TextPatternId
#define UIA_TextPatternId				10014
#endif
#ifndef UIA_TextPattern2Id
#define UIA_TextPattern2Id				10024
#endif

/* Global Variables */
// UI Automation
static IUIAutomation *uia_automation;

/* Local Function Prototypes */
static BOOL caret_point_check(const POINT *pt);
static BOOL caret_get_system_pos(const GUITHREADINFO *gti, POINT *pt);
static BOOL caret_get_msaa_pos(const HWND focus_wnd, POINT *pt);
static BOOL caret_rect_to_pos(IUIAutomationTextRange *range, POINT *pt);
static BOOL caret_range_to_pos(IUIAutomationTextRange *range, POINT *pt);
static BOOL caret_get_uia_pos(POINT *pt);

/*
 * caret_point_check - 位置が画面内にあるかチェック
 */
static BOOL caret_point_check(const POINT *pt)
{
	RECT vrect;

	// 仮想画面全体の矩形
	SetRect(&vrect,
		GetSystemMetrics(SM_XVIRTUALSCREEN),
		GetSystemMetrics(SM_YVIRTUALSCREEN),
		GetSystemMetrics(SM_XVIRTUALSCREEN) + GetSystemMetrics(SM_CXVIRTUALSCREEN),
		GetSystemMetrics(SM_YVIRTUALSCREEN) + GetSystemMetrics(SM_CYVIRTUALSCREEN));
	return PtInRect(&vrect, *pt);
}

/*
 * caret_get_system_pos - システムキャレットから位置を取得
 */
static BOOL caret_get_system_pos(const GUITHREADINFO *gti, POINT *pt)
{
	POINT p;

	if (gti->hwndCaret == NULL || gti->rcCaret.bottom - gti->rcCaret.top <= 0) {
		return FALSE;
	}
	p.x = gti->rcCaret.left;
	p.y = gti->rcCaret.top;
	if (ClientToScreen(gti->hwndCaret, &p) == FALSE || caret_point_check(&p) == FALSE) {
		return FALSE;
	}
	*pt = p;
	return TRUE;
}

/*
 * caret_get_msaa_pos - アクセシビリティ(MSAA)から位置を取得
 */
static BOOL caret_get_msaa_pos(const HWND focus_wnd, POINT *pt)
{
	typedef HRESULT (WINAPI *ACCESSIBLEOBJECTFROMWINDOW)(HWND, DWORD, REFIID, void **);
	static HMODULE oleacc_lib;
	static ACCESSIBLEOBJECTFROMWINDOW _AccessibleObjectFromWindow;
	IAccessible *acc = NULL;
	VARIANT v;
	POINT p;
	long left = 0, top = 0, width = 0, height = 0;
	BOOL ret = FALSE;

	if (focus_wnd == NULL) {
		return FALSE;
	}
	if (oleacc_lib == NULL && (oleacc_lib = LoadLibrary(TEXT("oleacc.dll"))) == NULL) {
		return FALSE;
	}
	if (_AccessibleObjectFromWindow == NULL) {
		_AccessibleObjectFromWindow =
			(ACCESSIBLEOBJECTFROMWINDOW)GetProcAddress(oleacc_lib, "AccessibleObjectFromWindow");
	}
	if (_AccessibleObjectFromWindow == NULL) {
		return FALSE;
	}
	// キャレットのオブジェクトを取得
	if (FAILED(_AccessibleObjectFromWindow(focus_wnd, (DWORD)OBJID_CARET,
		&IID_IAccessible, (void **)&acc)) || acc == NULL) {
		return FALSE;
	}
	VariantInit(&v);
	v.vt = VT_I4;
	v.lVal = CHILDID_SELF;
	if (SUCCEEDED(IAccessible_accLocation(acc, &left, &top, &width, &height, v)) && height > 0) {
		p.x = left;
		p.y = top;
		if (caret_point_check(&p) == TRUE) {
			*pt = p;
			ret = TRUE;
		}
	}
	IAccessible_Release(acc);
	return ret;
}

/*
 * caret_rect_to_pos - テキスト範囲の矩形から位置を取得
 */
static BOOL caret_rect_to_pos(IUIAutomationTextRange *range, POINT *pt)
{
	SAFEARRAY *sa = NULL;
	double *rects = NULL;
	VARTYPE vt = VT_EMPTY;
	LONG lb = 0, ub = -1;
	POINT p;
	BOOL ret = FALSE;

	// 矩形は left, top, width, height の並びで返される
	if (FAILED(IUIAutomationTextRange_GetBoundingRectangles(range, &sa)) || sa == NULL) {
		return FALSE;
	}
	if (SafeArrayGetDim(sa) == 1 &&
		SUCCEEDED(SafeArrayGetVartype(sa, &vt)) && vt == VT_R8 &&
		SUCCEEDED(SafeArrayGetLBound(sa, 1, &lb)) && SUCCEEDED(SafeArrayGetUBound(sa, 1, &ub)) &&
		ub - lb + 1 >= 4 && SUCCEEDED(SafeArrayAccessData(sa, (void **)&rects))) {
		if (*(rects + 3) > 0) {
			p.x = (LONG)*(rects + 0);
			p.y = (LONG)*(rects + 1);
			if (caret_point_check(&p) == TRUE) {
				*pt = p;
				ret = TRUE;
			}
		}
		SafeArrayUnaccessData(sa);
	}
	SafeArrayDestroy(sa);
	return ret;
}

/*
 * caret_range_to_pos - テキスト範囲から位置を取得
 */
static BOOL caret_range_to_pos(IUIAutomationTextRange *range, POINT *pt)
{
	IUIAutomationTextRange *crange = NULL;
	BOOL ret;

	if ((ret = caret_rect_to_pos(range, pt)) == TRUE) {
		return TRUE;
	}
	// 選択されていない場合は1文字分に広げてから位置を取得する
	if (SUCCEEDED(IUIAutomationTextRange_Clone(range, &crange)) && crange != NULL) {
		if (SUCCEEDED(IUIAutomationTextRange_ExpandToEnclosingUnit(crange, TextUnit_Character))) {
			ret = caret_rect_to_pos(crange, pt);
		}
		IUIAutomationTextRange_Release(crange);
	}
	return ret;
}

/*
 * caret_get_uia_pos - アクセシビリティ(UI Automation)から位置を取得
 */
static BOOL caret_get_uia_pos(POINT *pt)
{
	IUIAutomationElement *element = NULL;
	IUIAutomationTextPattern *text = NULL;
	IUIAutomationTextPattern2 *text2 = NULL;
	IUIAutomationTextRangeArray *ranges = NULL;
	IUIAutomationTextRange *range = NULL;
	int cnt = 0;
	BOOL active = FALSE;
	BOOL ret = FALSE;

	if (uia_automation == NULL &&
		(FAILED(CoCreateInstance(&CLSID_CUIAutomation, NULL, CLSCTX_INPROC_SERVER,
			&IID_IUIAutomation, (void **)&uia_automation)) || uia_automation == NULL)) {
		uia_automation = NULL;
		return FALSE;
	}
	// フォーカスを持つ要素の取得
	if (FAILED(IUIAutomation_GetFocusedElement(uia_automation, &element)) || element == NULL) {
		return FALSE;
	}
	// キャレットの位置から取得
	if (SUCCEEDED(IUIAutomationElement_GetCurrentPatternAs(element, UIA_TextPattern2Id,
		&IID_IUIAutomationTextPattern2, (void **)&text2)) && text2 != NULL) {
		if (SUCCEEDED(IUIAutomationTextPattern2_GetCaretRange(text2, &active, &range)) && range != NULL) {
			ret = caret_range_to_pos(range, pt);
			IUIAutomationTextRange_Release(range);
			range = NULL;
		}
		IUIAutomationTextPattern2_Release(text2);
	}
	// 選択範囲の位置から取得
	if (ret == FALSE &&
		SUCCEEDED(IUIAutomationElement_GetCurrentPatternAs(element, UIA_TextPatternId,
			&IID_IUIAutomationTextPattern, (void **)&text)) && text != NULL) {
		if (SUCCEEDED(IUIAutomationTextPattern_GetSelection(text, &ranges)) && ranges != NULL) {
			if (SUCCEEDED(IUIAutomationTextRangeArray_get_Length(ranges, &cnt)) && cnt > 0 &&
				SUCCEEDED(IUIAutomationTextRangeArray_GetElement(ranges, 0, &range)) && range != NULL) {
				ret = caret_range_to_pos(range, pt);
				IUIAutomationTextRange_Release(range);
			}
			IUIAutomationTextRangeArray_Release(ranges);
		}
		IUIAutomationTextPattern_Release(text);
	}
	IUIAutomationElement_Release(element);
	return ret;
}

/*
 * caret_get_pos - キャレットの位置を取得
 */
BOOL caret_get_pos(const HWND active_wnd, const HWND focus_wnd, POINT *pt)
{
	GUITHREADINFO gti;
	HWND wnd = focus_wnd;
	DWORD_PTR res;

	if (active_wnd == NULL || pt == NULL) {
		return FALSE;
	}
	ZeroMemory(&gti, sizeof(GUITHREADINFO));
	gti.cbSize = sizeof(GUITHREADINFO);
	if (GetGUIThreadInfo(GetWindowThreadProcessId(active_wnd, NULL), &gti) != FALSE) {
		// システムキャレットから取得
		if (caret_get_system_pos(&gti, pt) == TRUE) {
			return TRUE;
		}
		if (wnd == NULL) {
			wnd = gti.hwndFocus;
		}
	}
	// システムキャレットを作成しないアプリケーション(Chromeなど)は
	// アクセシビリティから位置を取得する
	if (SendMessageTimeout(active_wnd, WM_NULL, 0, 0,
		SMTO_ABORTIFHUNG | SMTO_BLOCK, CARET_RESPONSE_TIMEOUT, &res) == 0) {
		// 応答しないウィンドウは処理しない
		return FALSE;
	}
	if (caret_get_msaa_pos(wnd, pt) == TRUE) {
		return TRUE;
	}
	return caret_get_uia_pos(pt);
}

/*
 * caret_free - キャレット情報の解放
 */
void caret_free(void)
{
	if (uia_automation != NULL) {
		IUIAutomation_Release(uia_automation);
		uia_automation = NULL;
	}
}
/* End of source */
