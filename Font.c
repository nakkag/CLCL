/*
 * CLCL
 *
 * Font.c
 *
 * Copyright (C) 1996-2019 by Ohno Tomoaki. All rights reserved.
 *		https://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE

/* Define */

/* Global Variables */

/* Local Function Prototypes */

/*
 * font_create - ÉtÉHÉìÉgÇçÏê¨Ç∑ÇÈ
 */
HFONT font_create(const TCHAR *FontName, const int FontSize, const int Charset, const int weight, const BOOL italic, const BOOL fixed)
{
	LOGFONT lf;
	HDC hdc;

	ZeroMemory(&lf, sizeof(LOGFONT));

	hdc = GetDC(NULL);
	lf.lfHeight = -(int)((FontSize * GetDeviceCaps(hdc, LOGPIXELSY)) / 72);
	ReleaseDC(NULL, hdc);

	lf.lfWidth = 0;
	lf.lfEscapement = 0;
	lf.lfOrientation = 0;
	lf.lfWeight = weight;
	lf.lfItalic = italic;
	lf.lfUnderline = FALSE;
	lf.lfStrikeOut = FALSE;
	lf.lfCharSet = Charset;
	lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
	lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lf.lfQuality = DEFAULT_QUALITY;
	lf.lfPitchAndFamily = (BYTE)(((fixed == TRUE) ? FIXED_PITCH : DEFAULT_PITCH) | FF_DONTCARE);
	if (*FontName == TEXT('\0')) {
		NONCLIENTMETRICS ncm;
		ncm.cbSize = sizeof(NONCLIENTMETRICS);
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);
		lstrcpy(lf.lfFaceName, ncm.lfCaptionFont.lfFaceName);
	} else {
		lstrcpy(lf.lfFaceName, FontName);
	}
	return CreateFontIndirect((CONST LOGFONT *)&lf);
}
/* End of source */
