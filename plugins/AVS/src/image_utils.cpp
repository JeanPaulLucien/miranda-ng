#include "stdafx.h"
#include "image_utils.h"

#define GET_PIXEL(__P__, __X__, __Y__) ( (__P__) + width * 4 * (__Y__) + 4 * (__X__))

// Make a bitmap all transparent, but only if it is a 32bpp
void MakeBmpTransparent(HBITMAP hBitmap)
{
	BITMAP bmp;
	GetObject(hBitmap, sizeof(bmp), &bmp);
	if (bmp.bmBitsPixel != 32)
		return;

	uint32_t dwLen = bmp.bmWidth * bmp.bmHeight * (bmp.bmBitsPixel / 8);
	uint8_t *p = (uint8_t *)malloc(dwLen);
	if (p == nullptr)
		return;

	memset(p, 0, dwLen);
	SetBitmapBits(hBitmap, dwLen, p);

	free(p);
}

/////////////////////////////////////////////////////////////////////////////////////////

HBITMAP CopyBitmapTo32(HBITMAP hBitmap)
{
	BITMAP bmp;
	GetObject(hBitmap, sizeof(bmp), &bmp);

	uint32_t dwLen = bmp.bmWidth * bmp.bmHeight * 4;
	uint8_t *p = (uint8_t *)malloc(dwLen);
	if (p == nullptr)
		return nullptr;

	// Create bitmap
	BITMAPINFO RGB32BitsBITMAPINFO;
	memset(&RGB32BitsBITMAPINFO, 0, sizeof(BITMAPINFO));
	RGB32BitsBITMAPINFO.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	RGB32BitsBITMAPINFO.bmiHeader.biWidth = bmp.bmWidth;
	RGB32BitsBITMAPINFO.bmiHeader.biHeight = bmp.bmHeight;
	RGB32BitsBITMAPINFO.bmiHeader.biPlanes = 1;
	RGB32BitsBITMAPINFO.bmiHeader.biBitCount = 32;

	uint8_t *ptPixels;
	HBITMAP hDirectBitmap = CreateDIBSection(nullptr, (BITMAPINFO *)&RGB32BitsBITMAPINFO, DIB_RGB_COLORS, (void **)&ptPixels, nullptr, 0);

	// Copy data
	if (bmp.bmBitsPixel != 32) {
		HDC hdcOrig = CreateCompatibleDC(nullptr);
		HBITMAP oldOrig = (HBITMAP)SelectObject(hdcOrig, hBitmap);

		HDC hdcDest = CreateCompatibleDC(nullptr);
		HBITMAP oldDest = (HBITMAP)SelectObject(hdcDest, hDirectBitmap);

		BitBlt(hdcDest, 0, 0, bmp.bmWidth, bmp.bmHeight, hdcOrig, 0, 0, SRCCOPY);

		SelectObject(hdcDest, oldDest);
		DeleteObject(hdcDest);
		SelectObject(hdcOrig, oldOrig);
		DeleteObject(hdcOrig);

		// Set alpha
		FreeImage_CorrectBitmap32Alpha(hDirectBitmap, FALSE);
	}
	else {
		GetBitmapBits(hBitmap, dwLen, p);
		SetBitmapBits(hDirectBitmap, dwLen, p);
	}

	free(p);

	return hDirectBitmap;
}

HBITMAP CreateBitmap32(int cx, int cy)
{
	BITMAPINFO RGB32BitsBITMAPINFO;
	memset(&RGB32BitsBITMAPINFO, 0, sizeof(BITMAPINFO));
	RGB32BitsBITMAPINFO.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	RGB32BitsBITMAPINFO.bmiHeader.biWidth = cx;//bm.bmWidth;
	RGB32BitsBITMAPINFO.bmiHeader.biHeight = cy;//bm.bmHeight;
	RGB32BitsBITMAPINFO.bmiHeader.biPlanes = 1;
	RGB32BitsBITMAPINFO.bmiHeader.biBitCount = 32;

	UINT *ptPixels;
	return CreateDIBSection(nullptr, (BITMAPINFO *)&RGB32BitsBITMAPINFO, DIB_RGB_COLORS, (void **)&ptPixels, nullptr, 0);
}

// Set the color of points that are transparent
void SetTranspBkgColor(HBITMAP hBitmap, COLORREF color)
{
	BITMAP bmp;
	GetObject(hBitmap, sizeof(bmp), &bmp);

	if (bmp.bmBitsPixel != 32)
		return;

	uint32_t dwLen = bmp.bmWidth * bmp.bmHeight * (bmp.bmBitsPixel / 8);
	uint8_t *p = (uint8_t *)malloc(dwLen);
	if (p == nullptr)
		return;
	memset(p, 0, dwLen);

	GetBitmapBits(hBitmap, dwLen, p);

	bool changed = false;
	for (int y = 0; y < bmp.bmHeight; ++y) {
		uint8_t *px = p + bmp.bmWidth * 4 * y;

		for (int x = 0; x < bmp.bmWidth; ++x) {
			if (px[3] == 0) {
				px[0] = GetBValue(color);
				px[1] = GetGValue(color);
				px[2] = GetRValue(color);
				changed = true;
			}
			px += 4;
		}
	}

	if (changed)
		SetBitmapBits(hBitmap, dwLen, p);

	free(p);
}


#define HIMETRIC_INCH   2540    // HIMETRIC units per inch

void SetHIMETRICtoDP(HDC hdc, SIZE* sz)
{
	POINT pt;
	int nMapMode = GetMapMode(hdc);
	if (nMapMode < MM_ISOTROPIC && nMapMode != MM_TEXT) {
		// when using a constrained map mode, map against physical inch
		SetMapMode(hdc, MM_HIMETRIC);
		pt.x = sz->cx;
		pt.y = sz->cy;
		LPtoDP(hdc, &pt, 1);
		sz->cx = pt.x;
		sz->cy = pt.y;
		SetMapMode(hdc, nMapMode);
	}
	else {
		// map against logical inch for non-constrained mapping modes
		int cxPerInch, cyPerInch;
		cxPerInch = GetDeviceCaps(hdc, LOGPIXELSX);
		cyPerInch = GetDeviceCaps(hdc, LOGPIXELSY);
		sz->cx = MulDiv(sz->cx, cxPerInch, HIMETRIC_INCH);
		sz->cy = MulDiv(sz->cy, cyPerInch, HIMETRIC_INCH);
	}

	pt.x = sz->cx;
	pt.y = sz->cy;
	DPtoLP(hdc, &pt, 1);
	sz->cx = pt.x;
	sz->cy = pt.y;
}

HBITMAP BmpFilterLoadBitmap(BOOL *bIsTransparent, const wchar_t *ptszFilename)
{
	FIBITMAP *dib = (FIBITMAP*)Image_Load(ptszFilename, IMGL_RETURNDIB);
	if (dib == nullptr)
		return nullptr;

	FIBITMAP *dib32 = nullptr;
	if (FreeImage_GetBPP(dib) != 32) {
		dib32 = FreeImage_ConvertTo32Bits(dib);
		FreeImage_Unload(dib);
	}
	else dib32 = dib;

	if (dib32 == nullptr)
		return nullptr;

	if (FreeImage_IsTransparent(dib32))
		if (bIsTransparent)
			*bIsTransparent = TRUE;

	if (FreeImage_GetWidth(dib32) > 128 || FreeImage_GetHeight(dib32) > 128) {
		FIBITMAP *dib_new = FreeImage_MakeThumbnail(dib32, 128, FALSE);
		FreeImage_Unload(dib32);
		if (dib_new == nullptr)
			return nullptr;
		dib32 = dib_new;
	}

	HBITMAP bitmap = FreeImage_CreateHBITMAPFromDIB(dib32);
	FreeImage_Unload(dib32);
	FreeImage_CorrectBitmap32Alpha(bitmap, FALSE);
	return bitmap;
}

static HWND hwndClui = nullptr;

//
// Save ///////////////////////////////////////////////////////////////////////////////////////////
// PNG and BMP will be saved as 32bit images, jpg as 24bit with default quality (75)
// returns 1 on success, 0 on failure

int BmpFilterSaveBitmap(HBITMAP hBmp, const wchar_t *ptszFile, int flags)
{
	wchar_t tszFilename[MAX_PATH];
	if (!PathToAbsoluteW(ptszFile, tszFilename))
		wcsncpy_s(tszFilename, ptszFile, _TRUNCATE);

	if (mir_wstrlen(tszFilename) <= 4)
		return -1;

	IMGSRVC_INFO i = { 0 };
	i.cbSize = sizeof(IMGSRVC_INFO);
	i.pwszName = tszFilename;
	i.hbm = hBmp;
	i.dwMask = IMGI_HBITMAP;
	i.fif = FIF_UNKNOWN;
	return !Image_Save(&i, flags);
}

// Other utilities ////////////////////////////////////////////////////////////////////////////////

static BOOL ColorsAreTheSame(int colorDiff, uint8_t *px1, uint8_t *px2)
{
	return abs(px1[0] - px2[0]) <= colorDiff
		&& abs(px1[1] - px2[1]) <= colorDiff
		&& abs(px1[2] - px2[2]) <= colorDiff;
}

void AddToStack(int *stack, int *topPos, int x, int y)
{
	// Already is in stack?
	for (int i = 0; i < *topPos; i += 2)
		if (stack[i] == x && stack[i + 1] == y)
			return;

	stack[*topPos] = x;
	(*topPos)++;

	stack[*topPos] = y;
	(*topPos)++;
}

BOOL GetColorForPoint(int colorDiff, uint8_t *p, int width,
	int x0, int y0, int x1, int y1, int x2, int y2, BOOL *foundBkg, uint8_t colors[][3])
{
	uint8_t *px1 = GET_PIXEL(p, x0, y0);
	uint8_t *px2 = GET_PIXEL(p, x1, y1);
	uint8_t *px3 = GET_PIXEL(p, x2, y2);

	// If any of the corners have transparency, forget about it
	// Not using != 255 because some MSN bmps have 254 in some positions
	if (px1[3] < 253 || px2[3] < 253 || px3[3] < 253)
		return FALSE;

	// See if is the same color
	if (ColorsAreTheSame(colorDiff, px1, px2) && ColorsAreTheSame(colorDiff, px3, px2)) {
		*foundBkg = TRUE;
		memmove(colors, px1, 3);
	}
	else *foundBkg = FALSE;

	return TRUE;
}


uint32_t GetImgHash(HBITMAP hBitmap)
{
	BITMAP bmp;
	GetObject(hBitmap, sizeof(bmp), &bmp);

	uint32_t dwLen = bmp.bmWidth * bmp.bmHeight * (bmp.bmBitsPixel / 8);
	uint16_t *p = (uint16_t *)malloc(dwLen);
	if (p == nullptr)
		return 0;
	memset(p, 0, dwLen);

	GetBitmapBits(hBitmap, dwLen, p);

	uint32_t ret = 0;
	for (uint32_t i = 0; i < dwLen / 2; i++)
		ret += p[i];

	free(p);

	return ret;
}

/*
 * Changes the handle to a grayscale image
 */
HBITMAP MakeGrayscale(HBITMAP hBitmap)
{
	if (hBitmap) {
		FIBITMAP *dib = FreeImage_CreateDIBFromHBITMAP(hBitmap);
		if (dib) {
			FIBITMAP *dib_new = FreeImage_ConvertToGreyscale(dib);
			FreeImage_Unload(dib);
			if (dib_new) {
				DeleteObject(hBitmap);
				HBITMAP hbm_new = FreeImage_CreateHBITMAPFromDIB(dib_new);
				FreeImage_Unload(dib_new);
				return hbm_new;
			}
		}
	}
	return hBitmap;
}

/*
 * See if finds a transparent background in image, and set its transparency
 * Return TRUE if found a transparent background
 */
BOOL MakeTransparentBkg(MCONTACT hContact, HBITMAP *hBitmap)
{
	int i, j;

	BITMAP bmp;
	GetObject(*hBitmap, sizeof(bmp), &bmp);
	int width = bmp.bmWidth;
	int height = bmp.bmHeight;
	int colorDiff = db_get_w(hContact, "ContactPhoto", "TranspBkgColorDiff", g_plugin.getWord("TranspBkgColorDiff", 10));

	// Min 5x5 to easy things in loop
	if (width <= 4 || height <= 4)
		return FALSE;

	uint32_t dwLen = width * height * 4;
	uint8_t *p = (uint8_t *)malloc(dwLen);
	if (p == nullptr)
		return FALSE;

	HBITMAP hBmpTmp;
	if (bmp.bmBitsPixel == 32)
		hBmpTmp = *hBitmap;
	else // Convert to 32 bpp
		hBmpTmp = CopyBitmapTo32(*hBitmap);

	GetBitmapBits(hBmpTmp, dwLen, p);

	// **** Get corner colors

	// Top left
	uint8_t colors[8][3];
	BOOL foundBkg[8];
	if (!GetColorForPoint(colorDiff, p, width, 0, 0, 0, 1, 1, 0, &foundBkg[0], &colors[0])) {
		if (hBmpTmp != *hBitmap)
			DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Top center
	if (!GetColorForPoint(colorDiff, p, width, width / 2, 0, width / 2 - 1, 0, width / 2 + 1, 0, &foundBkg[1], &colors[1])) {
		if (hBmpTmp != *hBitmap)
			DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Top Right
	if (!GetColorForPoint(colorDiff, p, width,
		width - 1, 0, width - 1, 1, width - 2, 0, &foundBkg[2], &colors[2])) {
		if (hBmpTmp != *hBitmap)
			DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Center left
	if (!GetColorForPoint(colorDiff, p, width, 0, height / 2, 0, height / 2 - 1, 0, height / 2 + 1, &foundBkg[3], &colors[3])) {
		if (hBmpTmp != *hBitmap)
			DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Center left
	if (!GetColorForPoint(colorDiff, p, width, width - 1, height / 2, width - 1, height / 2 - 1, width - 1, height / 2 + 1, &foundBkg[4], &colors[4])) {
		if (hBmpTmp != *hBitmap)
			DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Bottom left
	if (!GetColorForPoint(colorDiff, p, width, 0, height - 1, 0, height - 2, 1, height - 1, &foundBkg[5], &colors[5])) {
		if (hBmpTmp != *hBitmap)
			DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Bottom center
	if (!GetColorForPoint(colorDiff, p, width, width / 2, height - 1, width / 2 - 1, height - 1, width / 2 + 1, height - 1, &foundBkg[6], &colors[6])) {
		if (hBmpTmp != *hBitmap)
			DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Bottom Right
	if (!GetColorForPoint(colorDiff, p, width, width - 1, height - 1, width - 1, height - 2, width - 2, height - 1, &foundBkg[7], &colors[7])) {
		if (hBmpTmp != *hBitmap)
			DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// **** X corners have to have the same color

	int count = 0;
	for (i = 0; i < 8; i++)
		if (foundBkg[i])
			count++;

	if (count < db_get_w(hContact, "ContactPhoto", "TranspBkgNumPoints", g_plugin.getWord("TranspBkgNumPoints", 5))) {
		if (hBmpTmp != *hBitmap)
			DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Ok, X corners at least have a color, lets compare then
	int maxCount = 0, selectedColor = 0;
	for (i = 0; i < 8; i++) {
		if (foundBkg[i]) {
			count = 0;

			for (j = 0; j < 8; j++) {
				if (foundBkg[j] && ColorsAreTheSame(colorDiff, (uint8_t *)&colors[i], (uint8_t *)&colors[j]))
					count++;
			}

			if (count > maxCount) {
				maxCount = count;
				selectedColor = i;
			}
		}
	}

	if (maxCount < db_get_w(hContact, "ContactPhoto", "TranspBkgNumPoints",
		g_plugin.getWord("TranspBkgNumPoints", 5))) {
		// Not enought corners with the same color
		if (hBmpTmp != *hBitmap) DeleteObject(hBmpTmp);
		free(p);
		return FALSE;
	}

	// Get bkg color as mean of colors
	{
		int bkgColor[3];
		bkgColor[0] = 0;
		bkgColor[1] = 0;
		bkgColor[2] = 0;
		for (i = 0; i < 8; i++) {
			if (foundBkg[i] && ColorsAreTheSame(colorDiff, (uint8_t *)&colors[i], (uint8_t *)&colors[selectedColor])) {
				bkgColor[0] += colors[i][0];
				bkgColor[1] += colors[i][1];
				bkgColor[2] += colors[i][2];
			}
		}
		bkgColor[0] /= maxCount;
		bkgColor[1] /= maxCount;
		bkgColor[2] /= maxCount;

		colors[selectedColor][0] = bkgColor[0];
		colors[selectedColor][1] = bkgColor[1];
		colors[selectedColor][2] = bkgColor[2];
	}

	// **** Set alpha for the background color, from the borders
	if (hBmpTmp != *hBitmap) {
		DeleteObject(*hBitmap);

		*hBitmap = hBmpTmp;

		GetObject(*hBitmap, sizeof(bmp), &bmp);
		GetBitmapBits(*hBitmap, dwLen, p);
	}

	// Set alpha from borders
	bool transpProportional = (g_plugin.getByte("MakeTransparencyProportionalToColorDiff", 0) != 0);

	int *stack = (int *)malloc(width * height * 2 * sizeof(int));
	if (stack == nullptr) {
		free(p);
		return FALSE;
	}

	// Put four corners
	int topPos = 0;
	AddToStack(stack, &topPos, 0, 0);
	AddToStack(stack, &topPos, width / 2, 0);
	AddToStack(stack, &topPos, width - 1, 0);
	AddToStack(stack, &topPos, 0, height / 2);
	AddToStack(stack, &topPos, width - 1, height / 2);
	AddToStack(stack, &topPos, 0, height - 1);
	AddToStack(stack, &topPos, width / 2, height - 1);
	AddToStack(stack, &topPos, width - 1, height - 1);

	int curPos = 0;
	while (curPos < topPos) {
		// Get pos
		int x = stack[curPos]; curPos++;
		int y = stack[curPos]; curPos++;

		// Get pixel
		uint8_t *px1 = GET_PIXEL(p, x, y);

		// It won't change the transparency if one exists
		// (This avoid an endless loop too)
		// Not using == 255 because some MSN bmps have 254 in some positions
		if (px1[3] >= 253) {
			if (ColorsAreTheSame(colorDiff, px1, (uint8_t *)&colors[selectedColor])) {
				px1[3] = (transpProportional) ? min(252,
					(abs(px1[0] - colors[selectedColor][0])
					+ abs(px1[1] - colors[selectedColor][1])
					+ abs(px1[2] - colors[selectedColor][2])) / 3) : 0;

				// Add 4 neighbours
				if (x + 1 < width)
					AddToStack(stack, &topPos, x + 1, y);

				if (x - 1 >= 0)
					AddToStack(stack, &topPos, x - 1, y);

				if (y + 1 < height)
					AddToStack(stack, &topPos, x, y + 1);

				if (y - 1 >= 0)
					AddToStack(stack, &topPos, x, y - 1);
			}
		}
	}

	free(stack);

	SetBitmapBits(*hBitmap, dwLen, p);
	free(p);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Other utils

int SaveAvatar(const char *protocol, const wchar_t *tszFileName)
{
	INT_PTR result = CallProtoService(protocol, PS_SETMYAVATAR, 0, (LPARAM)tszFileName);
	if (result == CALLSERVICE_NOTFOUND)
		result = CallProtoService(protocol, PS_SETMYAVATAR, 0, _T2A(tszFileName));

	return result;
}
