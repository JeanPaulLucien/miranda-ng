/*
Copyright (C) 2006-2007 Scott Ellis
Copyright (C) 2007-2011 Jan Holub

This is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this file; see the file license.txt.  If
not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  
*/

#include "stdafx.h"

__inline void AddRow(PopupWindowData *pwd, wchar_t *swzLabel, wchar_t *swzValue, char *szProto, bool bParseSmileys, bool bNewline, bool bLineAbove, bool bIsTitle = false, HICON hIcon = nullptr)
{
	RowData *pRows = (RowData *)mir_realloc(pwd->rows, sizeof(RowData) * (pwd->iRowCount + 1));
	if (pRows == nullptr)
		return;
	pwd->rows = pRows;
	pwd->rows[pwd->iRowCount].swzLabel = swzLabel ? mir_wstrdup(swzLabel) : nullptr;
	pwd->rows[pwd->iRowCount].swzValue = swzValue ? mir_wstrdup(swzValue) : nullptr;
	pwd->rows[pwd->iRowCount].spi = bParseSmileys ? Smileys_PreParse(swzValue, (int)mir_wstrlen(swzValue), szProto) : nullptr;
	pwd->rows[pwd->iRowCount].bValueNewline = bNewline;
	pwd->rows[pwd->iRowCount].bLineAbove = bLineAbove;
	pwd->rows[pwd->iRowCount].bIsTitle = bIsTitle;
	pwd->rows[pwd->iRowCount++].hIcon = hIcon;
}

LRESULT CALLBACK PopupWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	POINT pt;
	RECT rc;
	PopupWindowData *pwd = (PopupWindowData *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch (uMsg) {
	case WM_CREATE:
		{
			CREATESTRUCT *cs = (CREATESTRUCT *)lParam;
			pwd = (PopupWindowData *)mir_calloc(sizeof(PopupWindowData));
			pwd->clcit = *(CLCINFOTIPEX *)cs->lpCreateParams;
			pwd->iIconIndex = -1;
			pwd->hpenBorder = CreatePen(PS_SOLID, 1, opt.bBorder ? opt.colBorder : opt.colBg);
			pwd->hpenDivider = CreatePen(PS_SOLID, 1, opt.colDivider);
			pwd->iTrans = (int)(opt.iOpacity / 100.0 * 255);

			// load icons order
			for (int i = 0; i < EXICONS_COUNT; i++)
				pwd->bIsIconVisible[opt.exIconsOrder[i]] = opt.exIconsVis[i] ? true : false;

			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pwd);

			// work around bug hiding tips
			GetCursorPos(&pwd->ptCursorStartPos);
			SetTimer(hwnd, ID_TIMER_CHECKMOUSE, CHECKMOUSE_ELAPSE, nullptr);

			// register copy menu hotkey (CTRL+C)
			pwd->iHotkeyId = GlobalAddAtom(L"Tipper");
			RegisterHotKey(hwnd, pwd->iHotkeyId, MOD_CONTROL, 0x43);

			if (pwd->clcit.szProto) {
				pwd->bIsTextTip = false;
				pwd->iIndent = opt.iTextIndent;
				pwd->iSidebarWidth = opt.iSidebarWidth;

				PROTOACCOUNT *pa = Proto_GetAccount(pwd->clcit.szProto);
				if (pa) {
					mir_wstrcpy(pwd->swzTitle, pa->tszAccountName);
					if (pa->IsLocked())
						mir_snwprintf(pwd->swzTitle, TranslateT("%s (locked)"), pwd->swzTitle);
				}
				else a2t(pwd->clcit.szProto, pwd->swzTitle, TITLE_TEXT_LEN);

				// protocol status
				int wStatus = Proto_GetStatus(pwd->clcit.szProto);

				// get status icon
				if (pwd->bIsIconVisible[0]) {
					pwd->extraIcons[0].hIcon = Skin_LoadProtoIcon(pwd->clcit.szProto, wStatus);
					pwd->extraIcons[0].bDestroy = false;
				}

				// get activity icon
				if (pwd->bIsIconVisible[2]) {
					pwd->extraIcons[2].hIcon = GetJabberActivityIcon(0, pwd->clcit.szProto);
					pwd->extraIcons[2].bDestroy = false;
				}

				// uid info
				wchar_t swzUid[256], swzUidName[256];
				if (Uid(0, pwd->clcit.szProto, swzUid, 256) && UidName(pwd->clcit.szProto, swzUidName, 253)) {
					mir_wstrcat(swzUidName, L": ");
					AddRow(pwd, swzUidName, swzUid, nullptr, false, false, false);
				}

				// logon info
				wchar_t swzLogon[64];
				if (TimestampToTimeDifference(NULL, pwd->clcit.szProto, "LogonTS", swzLogon, 59)) {
					wchar_t ago[96];
					mir_snwprintf(ago, TranslateT("%s ago"), swzLogon);
					AddRow(pwd, TranslateT("Log on:"), ago, nullptr, false, false, false);
				}

				// number of unread emails
				wchar_t swzEmailCount[64];
				if (ProtoServiceExists(pwd->clcit.szProto, PS_GETUNREADEMAILCOUNT)) {
					int iCount = (int)CallProtoService(pwd->clcit.szProto, PS_GETUNREADEMAILCOUNT, 0, 0);
					if (iCount > 0) {
						_itow(iCount, swzEmailCount, 10);
						AddRow(pwd, TranslateT("Unread emails:"), swzEmailCount, nullptr, false, false, false);
					}
				}

				wchar_t *swzText = Clist_GetStatusModeDescription(wStatus, 0);
				if (swzText)
					AddRow(pwd, TranslateT("Status:"), swzText, nullptr, false, false, false);

				if (wStatus >= ID_STATUS_ONLINE && wStatus <= ID_STATUS_MAX) {
					// status message
					ptrW ptszStatus(GetProtoStatusMessage(pwd->clcit.szProto, wStatus));
					if (ptszStatus) {
						StripBBCodesInPlace(ptszStatus);
						AddRow(pwd, TranslateT("Status message:"), ptszStatus, pwd->clcit.szProto, true, true, true);
					}

					// jabber mood or icq xstatus
					wchar_t *swzAdvTitle = GetJabberAdvStatusText(pwd->clcit.szProto, "mood", "title");
					if (swzAdvTitle) {
						StripBBCodesInPlace(swzAdvTitle);
						AddRow(pwd, TranslateT("Mood:"), swzAdvTitle, pwd->clcit.szProto, true, false, true);

						ptrW swzAdvText(GetJabberAdvStatusText(pwd->clcit.szProto, "mood", "text"));
						if (swzAdvText) {
							StripBBCodesInPlace(swzAdvText);
							AddRow(pwd, L"", swzAdvText, pwd->clcit.szProto, true, true, false);
						}
					}
					else {
						if (db_get_b(0, pwd->clcit.szProto, "XStatusId", 0)) {
							// xstatus title
							swzAdvTitle = GetProtoExtraStatusTitle(pwd->clcit.szProto);
							if (swzAdvTitle) {
								StripBBCodesInPlace(swzAdvTitle);
								AddRow(pwd, TranslateT("xStatus:"), swzAdvTitle, pwd->clcit.szProto, true, false, true);
							}

							// xstatus message
							ptrW swzAdvText(GetProtoExtraStatusMessage(pwd->clcit.szProto));
							if (swzAdvText) {
								StripBBCodesInPlace(swzAdvText);
								AddRow(pwd, L"", swzAdvText, pwd->clcit.szProto, true, true, false);
							}
						}
					}

					if (swzAdvTitle) {
						mir_free(swzAdvTitle);
						// get advanced status icon
						if (pwd->bIsIconVisible[1]) {
							pwd->extraIcons[1].hIcon = (HICON)CallProtoService(pwd->clcit.szProto, PS_GETCUSTOMSTATUSICON, 0, LR_SHARED);
							pwd->extraIcons[1].bDestroy = false;
						}
					}

					// jabber activity
					ptrW swzActTitle(GetJabberAdvStatusText(pwd->clcit.szProto, "activity", "title"));
					if (swzActTitle) {
						StripBBCodesInPlace(swzActTitle);
						AddRow(pwd, TranslateT("Activity:"), swzActTitle, pwd->clcit.szProto, true, false, true);
					}

					ptrW swzActText(GetJabberAdvStatusText(pwd->clcit.szProto, "activity", "text"));
					if (swzActText) {
						StripBBCodesInPlace(swzActText);
						AddRow(pwd, L"", swzActText, pwd->clcit.szProto, true, true, false);
					}

					// listening to
					ptrW swzListening(GetListeningTo(pwd->clcit.szProto));
					if (swzListening) {
						StripBBCodesInPlace(swzListening);
						AddRow(pwd, TranslateT("Listening to:"), swzListening, nullptr, false, true, true);
					}
				}
			}
			else if (pwd->clcit.swzText) {
				pwd->bIsTextTip = true;
				pwd->iIndent = 0;
				pwd->iSidebarWidth = 0;

				rc = pwd->clcit.rcItem;
				bool mirandaTrayTip = ((rc.right - rc.left) == 20) && ((rc.bottom - rc.top) == 20) ? true : false;

				if (mirandaTrayTip && !opt.bTraytip) {
					MyDestroyWindow(hwnd);
					return 0;
				}

				if (mirandaTrayTip && opt.bHandleByTipper) { // extended tray tooltip
					pwd->bIsTrayTip = true;
					pwd->iIndent = opt.iTextIndent;
					pwd->iSidebarWidth = opt.iSidebarWidth;

					SendMessage(hwnd, PUM_REFRESHTRAYTIP, 0, 0);

					if (opt.bExpandTraytip)
						SetTimer(hwnd, ID_TIMER_TRAYTIP, opt.iExpandTime, nullptr);
				}
				else {
					wchar_t buff[2048], *swzText = pwd->clcit.swzText;
					size_t iBuffPos, i = 0, iSize = mir_wstrlen(pwd->clcit.swzText);
					bool bTopMessage = false;

					while (i < iSize && swzText[i] != '<') {
						iBuffPos = 0;
						while (swzText[i] != '\n' && swzText[i] != '\r' && i < iSize && iBuffPos < 2048) {
							if (swzText[i] != '\t')
								buff[iBuffPos++] = swzText[i];
							i++;
						}

						buff[iBuffPos] = 0;

						if (iBuffPos) {
							AddRow(pwd, L"", buff, nullptr, false, true, false);
							bTopMessage = true;
						}

						while (i < iSize && (swzText[i] == '\n' || swzText[i] == '\r'))
							i++;
					}

					// parse bold bits into labels and the rest into items
					while (i < iSize) {
						while (i + 2 < iSize && (swzText[i] != '<' || swzText[i + 1] != 'b' || swzText[i + 2] != '>'))
							i++;

						i += 3;

						iBuffPos = 0;
						while (i + 3 < iSize && iBuffPos < 2048 && (swzText[i] != '<'
							|| swzText[i + 1] != '/' || swzText[i + 2] != 'b' || swzText[i + 3] != '>')) {
							if (swzText[i] != '\t')
								buff[iBuffPos++] = swzText[i];
							i++;
						}

						i += 4;

						buff[iBuffPos] = 0;

						if (iBuffPos) {
							pwd->rows = (RowData *)mir_realloc(pwd->rows, sizeof(RowData)* (pwd->iRowCount + 1));
							pwd->rows[pwd->iRowCount].bValueNewline = false;
							pwd->rows[pwd->iRowCount].swzLabel = mir_wstrdup(buff);
							if (pwd->iRowCount == 1 && bTopMessage)
								pwd->rows[pwd->iRowCount].bLineAbove = true;
							else
								pwd->rows[pwd->iRowCount].bLineAbove = false;

							iBuffPos = 0;
							while (i < iSize && iBuffPos < 2048 && swzText[i] != '\n') {
								if (swzText[i] != '\t' && swzText[i] != '\r')
									buff[iBuffPos++] = swzText[i];
								i++;
							}
							buff[iBuffPos] = 0;

							pwd->rows[pwd->iRowCount].swzValue = mir_wstrdup(buff);
							pwd->rows[pwd->iRowCount].spi = nullptr;
							pwd->iRowCount++;
						}

						i++;
					}

					if (pwd->iRowCount == 0) {
						// single item
						pwd->iRowCount = 1;
						pwd->rows = (RowData *)mir_alloc(sizeof(RowData));
						pwd->rows[0].bLineAbove = pwd->rows[0].bValueNewline = false;
						pwd->rows[0].swzLabel = nullptr;
						pwd->rows[0].swzValue = swzText;
						pwd->rows[0].spi = nullptr;
					}
				}
			}
			else {
				pwd->bIsTextTip = false;
				pwd->iIndent = opt.iTextIndent;
				pwd->iSidebarWidth = opt.iSidebarWidth;
				pwd->hContact = (DWORD_PTR)pwd->clcit.hItem;
				pwd->iIconIndex = Clist_GetContactIcon(pwd->hContact);

				// don't use stored status message
				if (!opt.bWaitForContent)
					g_plugin.delSetting(pwd->hContact, "TempStatusMsg");

				wcsncpy_s(pwd->swzTitle, Clist_GetContactDisplayName(pwd->hContact), _TRUNCATE);

				char *szProto = Proto_GetBaseAccountName(pwd->hContact);
				pwd->spiTitle = Smileys_PreParse(pwd->swzTitle, -1, szProto);

				// get extra icons
				int i;

				if (szProto) {
					// status icon
					if (pwd->bIsIconVisible[0]) {
						for (i = 0; opt.exIconsOrder[i] != 0; i++);
						pwd->extraIcons[i].hIcon = ImageList_GetIcon(Clist_GetImageList(), pwd->iIconIndex, 0);
						pwd->extraIcons[i].bDestroy = true;
					}

					// xstatus icon
					if (pwd->bIsIconVisible[1]) {
						for (i = 0; opt.exIconsOrder[i] != 1; i++);
						int iXstatus = db_get_b(pwd->hContact, szProto, "XStatusId", 0);
						if (iXstatus) {
							char szIconProto[64];
							if (mir_strcmp(szProto, META_PROTO) != 0)
								strncpy_s(szIconProto, szProto, _TRUNCATE);
							else
								strncpy_s(szIconProto, ptrA(db_get_sa(pwd->hContact, szProto, "XStatusProto", "")), _TRUNCATE);

							pwd->extraIcons[i].hIcon = (HICON)CallProtoService(szIconProto, PS_GETCUSTOMSTATUSICON, (WPARAM)iXstatus, LR_SHARED);
							pwd->extraIcons[i].bDestroy = false;
						}
					}

					// activity icon
					if (pwd->bIsIconVisible[2]) {
						for (i = 0; opt.exIconsOrder[i] != 2; i++);
						pwd->extraIcons[i].hIcon = GetJabberActivityIcon(pwd->hContact, szProto);
						pwd->extraIcons[i].bDestroy = false;
					}

					// gender icon
					if (pwd->bIsIconVisible[3]) {
						for (i = 0; opt.exIconsOrder[i] != 3; i++);
						int iGender = db_get_b(pwd->hContact, "UserInfo", "Gender", 0);
						if (iGender == 0)
							iGender = db_get_b(pwd->hContact, szProto, "Gender", 0);

						if (iGender == GEN_FEMALE)
							pwd->extraIcons[i].hIcon = IcoLib_GetIcon("gender_female");
						else if (iGender == GEN_MALE)
							pwd->extraIcons[i].hIcon = IcoLib_GetIcon("gender_male");
						pwd->extraIcons[i].bDestroy = false;
					}

					// flags icon
					if (pwd->bIsIconVisible[4]) {
						for (i = 0; opt.exIconsOrder[i] != 4; i++);

						INT_PTR iCountry = CallService(MS_FLAGS_DETECTCONTACTORIGINCOUNTRY, pwd->hContact, 0);
						if (iCountry == CALLSERVICE_NOTFOUND)
							iCountry = CallService(MS_FLAGS_GETCONTACTORIGINCOUNTRY, pwd->hContact, 0);

						if (iCountry != CALLSERVICE_NOTFOUND && iCountry != 0 && iCountry != CTRY_UNKNOWN && iCountry != CTRY_ERROR) {
							pwd->extraIcons[i].hIcon = LoadFlagIcon(iCountry);
							pwd->extraIcons[i].bDestroy = false;
						}
					}

					// fingerprint icon
					if (pwd->bIsIconVisible[5]) {
						for (i = 0; opt.exIconsOrder[i] != 5; i++);
						if (ServiceExists(MS_FP_GETCLIENTICONT)) {
							ptrW tszVersion(db_get_wsa(pwd->hContact, szProto, "MirVer"));
							if (tszVersion != NULL) {
								pwd->extraIcons[i].hIcon = Finger_GetClientIcon(tszVersion, 0);
								pwd->extraIcons[i].bDestroy = true;
							}
						}
					}
				}

				SendMessage(hwnd, PUM_REFRESH_VALUES, FALSE, 0);
			}

			SendMessage(hwnd, PUM_GETHEIGHT, 0, 0);
			SendMessage(hwnd, PUM_CALCPOS, 0, 0);

			// transparency
			SetWindowLongPtr(hwnd, GWL_EXSTYLE, GetWindowLongPtr(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);

			if (opt.bDropShadow && opt.skinMode == SM_COLORFILL)
				SetClassLongPtr(hwnd, GCL_STYLE, CS_DROPSHADOW);
			else
				SetClassLongPtr(hwnd, GCL_STYLE, 0);

			if (!skin.bNeedLayerUpdate)
				SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_ALPHA);

			if (opt.showEffect)
				SetTimer(hwnd, ID_TIMER_ANIMATE, ANIM_ELAPSE, nullptr);

			ShowWindow(hwnd, SW_SHOWNOACTIVATE);
			InvalidateRect(hwnd, nullptr, FALSE);

			// since tipper win is topmost, this should put it at top of topmost windows
			SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		}
		return 0;

	case WM_ERASEBKGND:
		if (!skin.bNeedLayerUpdate) {
			HDC hdc = (HDC)wParam;
			GetClientRect(hwnd, &rc);

			BitBlt(hdc, 0, 0, skin.iWidth, skin.iHeight, skin.hdc, 0, 0, SRCCOPY);

			// border
			if (opt.bBorder) {
				HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
				HPEN hOldPen = (HPEN)SelectObject(hdc, pwd->hpenBorder);

				int h = 0;
				if (opt.bRound) {
					int v;
					int w = 14;
					h = (rc.right - rc.left) > (w * 2) ? w : (rc.right - rc.left);
					v = (rc.bottom - rc.top) > (w * 2) ? w : (rc.bottom - rc.top);
					h = (h < v) ? h : v;
				}

				RoundRect(hdc, 0, 0, (rc.right - rc.left), (rc.bottom - rc.top), h, h);

				SelectObject(hdc, hOldBrush);
				SelectObject(hdc, hOldPen);
			}
		}
		return TRUE;

	case WM_PAINT:
		PAINTSTRUCT ps;
		{
			RECT r, r2;
			BeginPaint(hwnd, &ps);
			HDC hdc = skin.bNeedLayerUpdate ? skin.hdc : ps.hdc;

			GetClientRect(hwnd, &r);
			r2 = r;
			HFONT hOldFont = (HFONT)GetCurrentObject(hdc, OBJ_FONT);

			// text background
			SetBkMode(hdc, TRANSPARENT);

			BLENDFUNCTION blend;
			blend.BlendOp = AC_SRC_OVER;
			blend.BlendFlags = 0;
			blend.SourceConstantAlpha = 255;
			blend.AlphaFormat = AC_SRC_ALPHA;

			// avatar
			if (!pwd->bIsTextTip && opt.avatarLayout != PAV_NONE && pwd->iAvatarHeight) {
				RECT rcAvatar;
				rcAvatar.top = opt.iOuterAvatarPadding;

				if (opt.avatarLayout == PAV_LEFT) {
					rcAvatar.left = r.left + opt.iOuterAvatarPadding;
					rcAvatar.right = rcAvatar.left + pwd->iRealAvatarWidth;
					r2.left += pwd->iRealAvatarWidth + (opt.iOuterAvatarPadding + opt.iInnerAvatarPadding - opt.iPadding); // padding re-added for text
				}
				else {
					rcAvatar.right = r.right - opt.iOuterAvatarPadding;
					rcAvatar.left = rcAvatar.right - pwd->iRealAvatarWidth;
					r2.right -= pwd->iRealAvatarWidth + (opt.iOuterAvatarPadding + opt.iInnerAvatarPadding - opt.iPadding);
				}

				rcAvatar.bottom = rcAvatar.top + pwd->iRealAvatarHeight;

				AVATARCACHEENTRY *ace = nullptr;
				if (pwd->hContact)
					ace = (AVATARCACHEENTRY *)CallService(MS_AV_GETAVATARBITMAP, pwd->hContact, 0);
				else
					ace = (AVATARCACHEENTRY *)CallService(MS_AV_GETMYAVATAR, 0, (LPARAM)pwd->clcit.szProto);

				if (ace && ace->hbmPic && (ace->dwFlags & AVS_BITMAP_VALID) && !(ace->dwFlags & AVS_HIDEONCLIST)) {
					HBITMAP hbmpAvatar = Image_Resize(ace->hbmPic, RESIZEBITMAP_STRETCH | RESIZEBITMAP_KEEP_PROPORTIONS, pwd->iRealAvatarWidth, pwd->iRealAvatarHeight);
					if (hbmpAvatar) {
						HRGN hrgnAvatar = nullptr;
						if (opt.bAvatarRound) {
							hrgnAvatar = CreateRoundRectRgn(rcAvatar.left, rcAvatar.top, rcAvatar.right + 1, rcAvatar.bottom + 1, 9, 9);
							SelectClipRgn(hdc, hrgnAvatar);
						}

						BITMAP bm;
						GetObject(hbmpAvatar, sizeof(bm), &bm);
						HDC hdcMem = CreateCompatibleDC(hdc);
						SelectObject(hdcMem, hbmpAvatar);

						blend.SourceConstantAlpha = (uint8_t)(opt.iAvatarOpacity / 100.0 * 255);
						AlphaBlend(hdc, rcAvatar.left, rcAvatar.top, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, blend);
						blend.SourceConstantAlpha = 255;

						if (opt.bAvatarBorder) {
							mir_ptr<COLOR32> pBits(SaveAlpha(&rcAvatar));
							HBRUSH hbrBorder = CreateSolidBrush(opt.colAvatarBorder);
							if (opt.bAvatarRound)
								FrameRgn(hdc, hrgnAvatar, hbrBorder, 1, 1);
							else
								FrameRect(hdc, &rcAvatar, hbrBorder);

							DeleteObject(hbrBorder);
							RestoreAlpha(&rcAvatar, pBits, (uint8_t)(opt.iAvatarOpacity / 100.0 * 255));
						}

						if (hrgnAvatar) {
							SelectClipRgn(hdc, nullptr);
							DeleteObject(hrgnAvatar);
						}

						if (hbmpAvatar != ace->hbmPic)
							DeleteObject(hbmpAvatar);

						DeleteDC(hdcMem);
					}
				}
			}

			RECT tr;
			tr.left = r2.left + opt.iPadding + opt.iTitleIndent;
			tr.right = r2.right - opt.iPadding;
			tr.top = 0;
			tr.bottom = opt.iPadding;

			if (!pwd->bIsTextTip) {
				if (opt.titleIconLayout != PTL_NOICON) {
					// draw icons
					int iIconX, iIconY;
					iIconY = opt.iPadding + opt.iTextPadding;

					if (opt.titleIconLayout == PTL_RIGHTICON)
						iIconX = r2.right - 16 - opt.iPadding;
					else
						iIconX = r2.left + opt.iPadding;

					for (int i = 0; i < EXICONS_COUNT; i++) {
						if (pwd->extraIcons[i].hIcon) {
							DrawIconExAlpha(hdc, iIconX, iIconY, pwd->extraIcons[i].hIcon, 16, 16, 0, nullptr, DI_NORMAL, false);
							iIconY += 20;
						}
					}
				}

				// title text
				if (opt.bShowTitle) {
					if (hFontTitle)
						SelectObject(hdc, (HGDIOBJ)hFontTitle);
					SetTextColor(hdc, opt.colTitle);
					tr.top = opt.iPadding;
					tr.bottom = tr.top + pwd->iTitleHeight - opt.iPadding;
					UINT uTextFormat = DT_TOP | DT_LEFT | DT_WORDBREAK | DT_WORD_ELLIPSIS | DT_END_ELLIPSIS | DT_NOPREFIX;
					DrawTextExt(hdc, pwd->swzTitle, -1, &tr, uTextFormat, pwd->spiTitle);
				}
			}

			// values
			pwd->iTextHeight = 0;
			bool bIconPainted, bUseRect = true;
			int iRowHeight;
			for (int i = 0; i < pwd->iRowCount; i++) {
				tr.top = tr.bottom;
				bUseRect = (tr.top + opt.iTextPadding >= pwd->iAvatarHeight);
				bIconPainted = false;
				if (bUseRect) {
					if (pwd->rows[i].bLineAbove) {
						HPEN hOldPen = (HPEN)SelectObject(hdc, pwd->hpenDivider);
						tr.top += opt.iTextPadding;
						RECT rec;
						SetRect(&rec, r.left + opt.iPadding + pwd->iIndent, tr.top, r.right - opt.iPadding, tr.top + 1);
						mir_ptr<COLOR32> pBits(SaveAlpha(&rec));
						Rectangle(hdc, rec.left, rec.top, rec.right, rec.bottom);
						RestoreAlpha(&rec, pBits);
						SelectObject(hdc, hOldPen);
					}

					tr.left = r.left + opt.iPadding + pwd->iIndent;
					if (pwd->rows[i].bValueNewline)
						tr.right = r.right - opt.iPadding;
					else
						tr.right = r.left + opt.iPadding + pwd->iIndent + pwd->iLabelWidth;
				}
				else {
					if (pwd->rows[i].bLineAbove) {
						HPEN hOldPen = (HPEN)SelectObject(hdc, pwd->hpenDivider);
						tr.top += opt.iTextPadding;
						RECT rec;
						SetRect(&rec, r2.left + opt.iPadding + pwd->iIndent, tr.top, r2.right - opt.iPadding, tr.top + 1);
						mir_ptr<COLOR32> pBits(SaveAlpha(&rec));
						Rectangle(hdc, rec.left, rec.top, rec.right, rec.bottom);
						RestoreAlpha(&rec, pBits);
						SelectObject(hdc, hOldPen);
					}

					tr.left = r2.left + opt.iPadding + pwd->iIndent;
					if (pwd->rows[i].bValueNewline)
						tr.right = r2.right - opt.iPadding;
					else
						tr.right = r2.left + opt.iPadding + pwd->iIndent + pwd->iLabelWidth;
				}

				if (pwd->rows[i].bValueNewline)
					iRowHeight = pwd->rows[i].iLabelHeight;
				else
					iRowHeight = max(pwd->rows[i].iLabelHeight, pwd->rows[i].iValueHeight);

				if (pwd->rows[i].iLabelHeight) {
					tr.top += opt.iTextPadding;
					tr.bottom = tr.top + iRowHeight;

					if (pwd->bIsTrayTip && pwd->rows[i].bIsTitle) {
						if (hFontTrayTitle)
							SelectObject(hdc, (HGDIOBJ)hFontTrayTitle);
						SetTextColor(hdc, opt.colTrayTitle);
					}
					else {
						if (hFontLabels)
							SelectObject(hdc, (HGDIOBJ)hFontLabels);
						SetTextColor(hdc, opt.colLabel);
					}

					// status icon in tray tooltip
					if (opt.titleIconLayout != PTL_NOICON && pwd->bIsTrayTip && pwd->rows[i].hIcon) {
						DrawIconExAlpha(hdc, opt.iPadding, tr.top + (pwd->rows[i].iLabelHeight - 16) / 2, pwd->rows[i].hIcon, 16, 16, 0, nullptr, DI_NORMAL, false);
						bIconPainted = true;
					}

					DrawTextAlpha(hdc, pwd->rows[i].swzLabel, -1, &tr, opt.iLabelValign | ((opt.iLabelHalign == DT_RIGHT && !pwd->rows[i].bValueNewline) ? DT_RIGHT : DT_LEFT) | DT_END_ELLIPSIS | DT_SINGLELINE | DT_NOPREFIX);
					if (pwd->rows[i].bValueNewline)
						tr.top = tr.bottom;
				}
				else tr.bottom = tr.top;

				if (pwd->rows[i].bValueNewline)
					iRowHeight = pwd->rows[i].iValueHeight;

				if (hFontValues)
					SelectObject(hdc, (HGDIOBJ)hFontValues);
				SetTextColor(hdc, opt.colValue);
				if (bUseRect) {
					tr.left = r.left + opt.iPadding + pwd->iIndent;
					if (!pwd->rows[i].bValueNewline)
						tr.left += pwd->iLabelWidth + opt.iValueIndent;

					tr.right = r.right - opt.iPadding;
				}
				else {
					tr.left = r2.left + opt.iPadding + pwd->iIndent;
					if (!pwd->rows[i].bValueNewline)
						tr.left += pwd->iLabelWidth + opt.iValueIndent;

					tr.right = r2.right - opt.iPadding;
				}

				if (pwd->rows[i].iValueHeight) {
					if (pwd->rows[i].bValueNewline || !pwd->rows[i].iLabelHeight) tr.top += opt.iTextPadding;
					if (pwd->rows[i].iLabelHeight > pwd->rows[i].iValueHeight && pwd->bIsTextTip&& pwd->rows[i].bIsTitle)
						tr.top = tr.bottom - pwd->rows[i].iValueHeight - 2;
					else
						tr.bottom = tr.top + iRowHeight;

					if (opt.titleIconLayout != PTL_NOICON && pwd->bIsTrayTip && pwd->rows[i].hIcon && !bIconPainted)
						DrawIconExAlpha(hdc, opt.iPadding, tr.top + (pwd->rows[i].iValueHeight - 16) / 2, pwd->rows[i].hIcon, 16, 16, 0, nullptr, DI_NORMAL, false);

					UINT uFormat = opt.iValueValign | opt.iValueHalign | DT_WORDBREAK | DT_WORD_ELLIPSIS | DT_END_ELLIPSIS | DT_NOPREFIX;
					DrawTextExt(hdc, pwd->rows[i].swzValue, -1, &tr, uFormat, pwd->rows[i].spi);
				}
			}

			PremultipleChannels();

			if (opt.showEffect == PSE_NONE) {
				if (skin.bNeedLayerUpdate) {
					POINT ptSrc = { 0, 0 };
					SIZE szTip = { r.right - r.left, r.bottom - r.top };
					blend.SourceConstantAlpha = pwd->iTrans;
					UpdateLayeredWindow(hwnd, nullptr, nullptr, &szTip, skin.hdc, &ptSrc, 0xffffffff, &blend, LWA_ALPHA);

					if (opt.bAeroGlass && MyDwmEnableBlurBehindWindow) {
						DWM_BLURBEHIND bb = { 0 };
						bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
						bb.fEnable = TRUE;
						bb.hRgnBlur = CreateOpaqueRgn(25, true);
						MyDwmEnableBlurBehindWindow(hwnd, &bb);
					}
				}
				else SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), pwd->iTrans, LWA_ALPHA);
			}

			SelectObject(hdc, hOldFont);
			EndPaint(hwnd, &ps);
			pwd->bIsPainted = true;
		}
		return 0;

	case WM_HOTKEY:
		if (LOWORD(lParam) == MOD_CONTROL && HIWORD(lParam) == 0x43) { // CTRL+C 
			if (pwd->iRowCount == 0)
				return 0;

			ShowWindow(hwnd, SW_HIDE);
			HMENU hMenu = CreatePopupMenu();
			if (!hMenu)
				return 0;

			HICON hIcon = IcoLib_GetIconByHandle(iconList[1].hIcolib);
			if (!hIcon) {
				DestroyMenu(hMenu);
				return 0;
			}

			ICONINFO iconInfo;
			GetIconInfo(hIcon, &iconInfo);
			HBITMAP hbmpAllItems = iconInfo.hbmColor;

			AppendMenu(hMenu, MF_STRING, COPYMENU_ALLITEMS_LABELS, LPGENW("Copy all items with labels"));
			AppendMenu(hMenu, MF_STRING, COPYMENU_ALLITEMS, LPGENW("Copy all items"));
			if (pwd->clcit.szProto || pwd->hContact)
				AppendMenu(hMenu, MF_STRING, COPYMENU_AVATAR, LPGENW("Copy avatar"));
			AppendMenu(hMenu, MF_SEPARATOR, 2000, nullptr);
			TranslateMenu(hMenu);

			SetMenuItemBitmaps(hMenu, COPYMENU_ALLITEMS_LABELS, MF_BYCOMMAND, hbmpAllItems, hbmpAllItems);
			SetMenuItemBitmaps(hMenu, COPYMENU_ALLITEMS, MF_BYCOMMAND, hbmpAllItems, hbmpAllItems);
			SetMenuItemBitmaps(hMenu, COPYMENU_AVATAR, MF_BYCOMMAND, hbmpAllItems, hbmpAllItems);

			hIcon = IcoLib_GetIconByHandle(iconList[0].hIcolib);
			if (!hIcon) {
				DeleteObject(hbmpAllItems);
				DestroyMenu(hMenu);
				return 0;
			}

			GetIconInfo(hIcon, &iconInfo);
			HBITMAP hbmpItem = iconInfo.hbmColor;

			for (int i = 0; i < pwd->iRowCount; i++) {
				if (pwd->rows[i].swzValue) {
					wchar_t buff[128];
					int iLen = (int)mir_wstrlen(pwd->rows[i].swzValue);
					if (iLen) {
						if (iLen > MAX_VALUE_LEN) {
							wcsncpy(buff, pwd->rows[i].swzValue, MAX_VALUE_LEN);
							buff[MAX_VALUE_LEN] = 0;
							mir_wstrcat(buff, L"...");
						}
						else mir_wstrcpy(buff, pwd->rows[i].swzValue);

						AppendMenu(hMenu, MF_STRING, i + 1, buff);  // first id = 1, because no select have id = 0
						SetMenuItemBitmaps(hMenu, i + 1, MF_BYCOMMAND, hbmpItem, hbmpItem);
					}
					else AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
				}
			}

			GetCursorPos(&pt);
			SetForegroundWindow(hwnd);
			int iSelItem = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, nullptr);
			DeleteObject(hbmpAllItems);
			DeleteObject(hbmpItem);
			DestroyMenu(hMenu);

			if (iSelItem == 0)
				return 0;	// no item was selected

			if (OpenClipboard(nullptr)) {
				EmptyClipboard();
				if (iSelItem == COPYMENU_AVATAR) { // copy avatar
					AVATARCACHEENTRY *ace = nullptr;
					if (pwd->hContact)
						ace = (AVATARCACHEENTRY *)CallService(MS_AV_GETAVATARBITMAP, pwd->hContact, 0);
					else
						ace = (AVATARCACHEENTRY *)CallService(MS_AV_GETMYAVATAR, 0, (LPARAM)pwd->clcit.szProto);
					if (ace && ace->hbmPic && (ace->dwFlags & AVS_BITMAP_VALID) && !(ace->dwFlags & AVS_HIDEONCLIST)) {
						HDC hdc = GetDC(hwnd);
						HDC hdcSrc = CreateCompatibleDC(hdc);
						HDC hdcDes = CreateCompatibleDC(hdc);
						HBITMAP hbmAvr = CreateCompatibleBitmap(hdc, ace->bmWidth, ace->bmHeight);
						SelectObject(hdcSrc, ace->hbmPic);
						SelectObject(hdcDes, hbmAvr);
						BitBlt(hdcDes, 0, 0, ace->bmWidth, ace->bmHeight, hdcSrc, 0, 0, SRCCOPY);
						SetClipboardData(CF_BITMAP, hbmAvr);
						ReleaseDC(hwnd, hdc);
						DeleteDC(hdcSrc);
						DeleteDC(hdcDes);
					}
				}
				else { // copy text
					HGLOBAL hClipboardData = GlobalAlloc(GMEM_DDESHARE, 4096);
					wchar_t *pchData = (wchar_t *)GlobalLock(hClipboardData);
					pchData[0] = 0;
					if (iSelItem == COPYMENU_ALLITEMS_LABELS) { // copy all items with labels
						for (int i = 0; i < pwd->iRowCount; i++) {
							if ((pwd->rows[i].swzLabel && pwd->rows[i].swzLabel[0]) || (pwd->rows[i].swzValue && pwd->rows[i].swzValue[0])) {
								if (pwd->rows[i].swzLabel && pwd->rows[i].swzLabel[0]) {
									mir_wstrcat(pchData, pwd->rows[i].swzLabel);
									mir_wstrcat(pchData, L" ");
								}
								else mir_wstrcat(pchData, TranslateT("<No Label>: "));

								if (pwd->rows[i].swzValue && pwd->rows[i].swzValue[0])
									mir_wstrcat(pchData, pwd->rows[i].swzValue);
								else
									mir_wstrcat(pchData, TranslateT("<No Value>"));

								mir_wstrcat(pchData, L"\r\n");
							}
						}
					}
					else if (iSelItem == COPYMENU_ALLITEMS) { // copy all items		
						for (int i = 0; i < pwd->iRowCount; i++) {
							if (pwd->rows[i].swzValue && pwd->rows[i].swzValue[0]) {
								mir_wstrcat(pchData, pwd->rows[i].swzValue);
								mir_wstrcat(pchData, L"\r\n");
							}
						}
					}
					// single row
					else mir_wstrcpy(pchData, pwd->rows[iSelItem - 1].swzValue);

					GlobalUnlock(hClipboardData);
					SetClipboardData(CF_UNICODETEXT, hClipboardData);
				}

				CloseClipboard();
			}
		}
		break;

	case PUM_FADEOUTWINDOW:
		// kill timers
		KillTimer(hwnd, ID_TIMER_ANIMATE);
		KillTimer(hwnd, ID_TIMER_CHECKMOUSE);
		KillTimer(hwnd, ID_TIMER_TRAYTIP);

		if (opt.showEffect != PSE_NONE) {
			if (skin.bNeedLayerUpdate) {
				POINT ptSrc = { 0, 0 };
				SIZE sz = { pwd->rcWindow.right - pwd->rcWindow.left, pwd->rcWindow.bottom - pwd->rcWindow.top };

				BLENDFUNCTION blend;
				blend.BlendOp = AC_SRC_OVER;
				blend.BlendFlags = 0;
				blend.SourceConstantAlpha = pwd->iTrans;
				blend.AlphaFormat = AC_SRC_ALPHA;

				while (blend.SourceConstantAlpha != 0) {
					UpdateLayeredWindow(hwnd, nullptr, nullptr, &sz, skin.hdc, &ptSrc, 0xffffffff, &blend, LWA_ALPHA);
					blend.SourceConstantAlpha = max(blend.SourceConstantAlpha - opt.iAnimateSpeed, 0);
					Sleep(ANIM_ELAPSE);
				}
			}
			else {
				int iTrans = pwd->iTrans;
				while (iTrans != 0) {
					SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), iTrans, LWA_ALPHA);
					iTrans = max(iTrans - opt.iAnimateSpeed, 0);
					Sleep(ANIM_ELAPSE);
				}
			}
		}
		break;

	case WM_DESTROY:
		ShowWindow(hwnd, SW_HIDE);

		if (skin.hBitmap)
			DeleteObject(skin.hBitmap);
		if (skin.hdc)
			DeleteDC(skin.hdc);
		skin.hBitmap = nullptr;
		skin.hdc = nullptr;

		if (pwd == nullptr)
			break;
		// unregister hotkey
		UnregisterHotKey(hwnd, pwd->iHotkeyId);

		DeleteObject(pwd->hpenBorder);
		DeleteObject(pwd->hpenDivider);

		if (pwd->hrgnAeroGlass)
			DeleteObject(pwd->hrgnAeroGlass);

		Smileys_FreeParse(pwd->spiTitle);

		for (int i = 0; i < pwd->iRowCount && pwd->rows != nullptr; i++) {
			mir_free(pwd->rows[i].swzLabel);
			mir_free(pwd->rows[i].swzValue);
			Smileys_FreeParse(pwd->rows[i].spi);
		}
		mir_free(pwd->rows);

		// destroy icons
		for (int i = 0; i < EXICONS_COUNT; i++) {
			if (pwd->extraIcons[i].hIcon == nullptr)
				continue;

			if (pwd->extraIcons[i].bDestroy)
				DestroyIcon(pwd->extraIcons[i].hIcon);
			else
				IcoLib_ReleaseIcon(pwd->extraIcons[i].hIcon);
		}

		mir_free(pwd->clcit.swzText);
		mir_free(pwd);
		pwd = nullptr;

		SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
		break;

	case WM_TIMER:
		switch (wParam) {
		case ID_TIMER_ANIMATE:
			pwd->iAnimStep++;
			if (pwd->iAnimStep == ANIM_STEPS)
				KillTimer(hwnd, ID_TIMER_ANIMATE);

			SendMessage(hwnd, PUM_UPDATERGN, 1, 0);
			break;
		case ID_TIMER_CHECKMOUSE:
			{
				// workaround for tips that just won't go away
				GetCursorPos(&pt);
				// mouse has moved beyond tollerance
				if (abs(pt.x - pwd->ptCursorStartPos.x) > opt.iMouseTollerance || abs(pt.y - pwd->ptCursorStartPos.y) > opt.iMouseTollerance)
					PostMPMessage(MUM_DELETEPOPUP, 0, 0);
			}
			break;
		case ID_TIMER_TRAYTIP:
			KillTimer(hwnd, ID_TIMER_TRAYTIP);
			SendMessage(hwnd, PUM_EXPANDTRAYTIP, 0, 0);
			break;
		}
		break;

	case PUM_SETSTATUSTEXT:
		if (pwd && wParam == pwd->hContact) {
			g_plugin.setWString(pwd->hContact, "TempStatusMsg", (wchar_t *)lParam);
			pwd->bIsPainted = false;
			pwd->bNeedRefresh = true;
			SendMessage(hwnd, PUM_REFRESH_VALUES, TRUE, 0);
			InvalidateRect(hwnd, nullptr, TRUE);
		}
		mir_free((void *)lParam);
		return TRUE;

	case PUM_SHOWXSTATUS:
		if (pwd && wParam == pwd->hContact) {
			// in case we have retrieve xstatus
			pwd->bIsPainted = false;
			SendMessage(hwnd, PUM_REFRESH_VALUES, TRUE, 0);
			InvalidateRect(hwnd, nullptr, TRUE);
		}
		return TRUE;

	case PUM_SETAVATAR:
		if (pwd && wParam == pwd->hContact) {
			pwd->bIsPainted = false;
			SendMessage(hwnd, PUM_GETHEIGHT, 0, 0);
			SendMessage(hwnd, PUM_CALCPOS, 0, 0);
			InvalidateRect(hwnd, nullptr, TRUE);
		}
		return TRUE;

	case PUM_EXPANDTRAYTIP:
		pwd->bIsPainted = false;
		if (skin.bNeedLayerUpdate) {
			RECT r = pwd->rcWindow;
			POINT ptSrc = { 0, 0 };
			SIZE sz = { r.right - r.left, r.bottom - r.top };

			BLENDFUNCTION blend;
			blend.BlendOp = AC_SRC_OVER;
			blend.BlendFlags = 0;
			blend.SourceConstantAlpha = 0;
			blend.AlphaFormat = AC_SRC_ALPHA;

			UpdateLayeredWindow(hwnd, nullptr, nullptr, &sz, skin.hdc, &ptSrc, 0xffffffff, &blend, LWA_ALPHA);
		}
		else SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_ALPHA);

		SendMessage(hwnd, PUM_REFRESHTRAYTIP, 1, 0);
		SendMessage(hwnd, PUM_GETHEIGHT, 0, 0);
		SendMessage(hwnd, PUM_CALCPOS, 0, 0);
		InvalidateRect(hwnd, nullptr, TRUE);

		if (opt.showEffect) {
			KillTimer(hwnd, ID_TIMER_ANIMATE);
			SetTimer(hwnd, ID_TIMER_ANIMATE, ANIM_ELAPSE, nullptr);
			pwd->iAnimStep = 0;
			pwd->iCurrentTrans = 0;
		}
		return TRUE;

	case PUM_REFRESH_VALUES:
		if (pwd && pwd->clcit.szProto == nullptr && !pwd->bIsTextTip) {
			for (int i = 0; i < pwd->iRowCount; i++) {
				mir_free(pwd->rows[i].swzLabel);
				mir_free(pwd->rows[i].swzValue);
				Smileys_FreeParse(pwd->rows[i].spi);
			}

			mir_free(pwd->rows);
			pwd->rows = nullptr;
			pwd->iRowCount = 0;

			DIListNode *node = opt.diList;
			wchar_t buff_label[LABEL_LEN], buff[VALUE_LEN];
			while (node) {
				if (node->di.bIsVisible && CheckContactType(pwd->hContact, node->di)) {
					if (GetLabelText(pwd->hContact, node->di, buff_label, LABEL_LEN) && GetValueText(pwd->hContact, node->di, buff, VALUE_LEN)) {
						if (node->di.bLineAbove // we have a line above
							&& pwd->iRowCount > 0 // and we're not the first row
							&& pwd->rows[pwd->iRowCount - 1].bLineAbove // and above us there's a line above
							&& pwd->rows[pwd->iRowCount - 1].swzLabel[0] == 0 // with no label
							&& pwd->rows[pwd->iRowCount - 1].swzValue[0] == 0) // and no value
						{
							// overwrite item above
							pwd->iRowCount--;
							mir_free(pwd->rows[pwd->iRowCount].swzLabel);
							mir_free(pwd->rows[pwd->iRowCount].swzValue);
							Smileys_FreeParse(pwd->rows[pwd->iRowCount].spi);	//prevent possible mem leak
						}
						else pwd->rows = (RowData *)mir_realloc(pwd->rows, sizeof(RowData)* (pwd->iRowCount + 1));

						char *szProto = Proto_GetBaseAccountName(pwd->hContact);

						pwd->rows[pwd->iRowCount].swzLabel = mir_wstrdup(buff_label);
						pwd->rows[pwd->iRowCount].swzValue = mir_wstrdup(buff);
						pwd->rows[pwd->iRowCount].spi = Smileys_PreParse(buff, (int)mir_wstrlen(buff), szProto);
						pwd->rows[pwd->iRowCount].bValueNewline = node->di.bValueNewline;
						pwd->rows[pwd->iRowCount].bLineAbove = node->di.bLineAbove;
						pwd->iRowCount++;
					}
				}
				node = node->next;
			}

			// if the last item is just a divider, remove it
			if (pwd->iRowCount > 0
				&& pwd->rows[pwd->iRowCount - 1].bLineAbove // and above us there's a line above
				&& pwd->rows[pwd->iRowCount - 1].swzLabel[0] == 0 // with no label
				&& pwd->rows[pwd->iRowCount - 1].swzValue[0] == 0) // and no value
			{
				pwd->iRowCount--;
				mir_free(pwd->rows[pwd->iRowCount].swzLabel);
				mir_free(pwd->rows[pwd->iRowCount].swzValue);
				Smileys_FreeParse(pwd->rows[pwd->iRowCount].spi);	//prevent possible mem leak

				if (pwd->iRowCount == 0) {
					mir_free(pwd->rows);
					pwd->rows = nullptr;
				}
			}

			if (wParam == TRUE) {
				SendMessage(hwnd, PUM_GETHEIGHT, 0, 0);
				SendMessage(hwnd, PUM_CALCPOS, 0, 0);
			}
		}
		return TRUE;

	case PUM_GETHEIGHT:
		RECT smr;
		{
			int *pHeight = (int *)wParam;
			HDC hdc = GetDC(hwnd);
			SIZE sz;
			rc.top = rc.left = 0;
			rc.right = opt.iWinWidth;
			int iWidth = opt.iPadding;
			int iWinAvatarHeight = 0;
			bool bStatusMsg = false;
			HFONT hOldFont = (HFONT)GetCurrentObject(hdc, OBJ_FONT);

			// avatar height
			pwd->iAvatarHeight = 0;
			if (!pwd->bIsTextTip && opt.avatarLayout != PAV_NONE && ServiceExists(MS_AV_GETAVATARBITMAP)) {
				AVATARCACHEENTRY *ace = nullptr;
				if (pwd->hContact) ace = (AVATARCACHEENTRY *)CallService(MS_AV_GETAVATARBITMAP, (WPARAM)pwd->hContact, 0);
				else ace = (AVATARCACHEENTRY *)CallService(MS_AV_GETMYAVATAR, 0, (LPARAM)pwd->clcit.szProto);

				if (ace && (ace->dwFlags & AVS_BITMAP_VALID) && !(ace->dwFlags & AVS_HIDEONCLIST)) {
					if (opt.bOriginalAvatarSize && max(ace->bmWidth, ace->bmHeight) <= opt.iAvatarSize) {
						pwd->iRealAvatarHeight = ace->bmHeight;
						pwd->iRealAvatarWidth = ace->bmWidth;
					}
					else if (ace->bmHeight >= ace->bmWidth) {
						pwd->iRealAvatarHeight = opt.iAvatarSize;
						pwd->iRealAvatarWidth = (int)(opt.iAvatarSize * (ace->bmWidth / (double)ace->bmHeight));
					}
					else {
						pwd->iRealAvatarHeight = (int)(opt.iAvatarSize * (ace->bmHeight / (double)ace->bmWidth));
						pwd->iRealAvatarWidth = opt.iAvatarSize;
					}

					pwd->iAvatarHeight = opt.iOuterAvatarPadding + opt.iInnerAvatarPadding + pwd->iRealAvatarHeight;
					iWinAvatarHeight = 2 * opt.iOuterAvatarPadding + pwd->iRealAvatarHeight;
					iWidth += pwd->iRealAvatarWidth + (opt.iOuterAvatarPadding + opt.iInnerAvatarPadding - opt.iPadding);
				}
			}

			// titlebar height
			if (!pwd->bIsTextTip && opt.bShowTitle) {
				smr.top = smr.bottom = 0;
				smr.left = rc.left + opt.iPadding + pwd->iIndent;
				smr.right = rc.right;

				if (pwd->iRealAvatarWidth > 0)
					smr.right -= pwd->iRealAvatarWidth + opt.iOuterAvatarPadding + opt.iInnerAvatarPadding;
				else
					smr.right -= opt.iPadding;

				if (hFontTitle) SelectObject(hdc, (HGDIOBJ)hFontTitle);
				DrawTextExt(hdc, pwd->swzTitle, -1, &smr, DT_CALCRECT | DT_LEFT | DT_WORDBREAK | DT_END_ELLIPSIS | DT_NOPREFIX, pwd->spiTitle);

				iWidth += opt.iPadding + opt.iTitleIndent + smr.right;
				pwd->iTitleHeight = opt.iPadding + smr.bottom;
			}
			else pwd->iTitleHeight = opt.iPadding;

			// icon height
			int i, iCount = 0;
			if (pwd->hContact || pwd->clcit.szProto) {
				for (i = 0; i < EXICONS_COUNT; i++) {
					if ((INT_PTR)pwd->extraIcons[i].hIcon == CALLSERVICE_NOTFOUND)
						pwd->extraIcons[i].hIcon = nullptr;

					if (pwd->extraIcons[i].hIcon)
						iCount++;
				}
			}
			pwd->iIconsHeight = (iCount * 20) + 20;

			// text height
			pwd->iTextHeight = pwd->iLabelWidth = 0;
			// iterate once to find max label width for items with label and value on same line, but don't consider width of labels on a new line
			for (i = 0; i < pwd->iRowCount; i++) {
				if (pwd->rows[i].swzLabel && !pwd->rows[i].bValueNewline) {
					if (pwd->bIsTrayTip && pwd->rows[i].bIsTitle) {
						if (hFontTrayTitle)
							SelectObject(hdc, (HGDIOBJ)hFontTrayTitle);
					}
					else if (hFontLabels)
						SelectObject(hdc, (HGDIOBJ)hFontLabels);

					GetTextExtentPoint32(hdc, pwd->rows[i].swzLabel, (int)mir_wstrlen(pwd->rows[i].swzLabel), &sz);
					if (sz.cx > pwd->iLabelWidth)
						pwd->iLabelWidth = sz.cx;
				}
			}

			for (i = 0; i < pwd->iRowCount; i++) {
				if (pwd->bIsTrayTip && pwd->rows[i].bIsTitle) {
					if (hFontTrayTitle)
						SelectObject(hdc, (HGDIOBJ)hFontTrayTitle);
				}
				else if (hFontLabels)
					SelectObject(hdc, (HGDIOBJ)hFontLabels);

				if (pwd->rows[i].swzLabel && pwd->rows[i].swzLabel[0])
					GetTextExtentPoint32(hdc, pwd->rows[i].swzLabel, (int)mir_wstrlen(pwd->rows[i].swzLabel), &sz);
				else
					sz.cy = sz.cx = 0;

				// save so we don't have to recalculate
				pwd->rows[i].iLabelHeight = sz.cy;

				smr.top = smr.bottom = 0;
				smr.left = rc.left + opt.iPadding + pwd->iIndent;
				smr.right = rc.right;
				if (hFontValues) SelectObject(hdc, (HGDIOBJ)hFontValues);
				if (pwd->iTitleHeight + pwd->iTextHeight + opt.iTextPadding < pwd->iAvatarHeight)
					smr.right -= pwd->iRealAvatarWidth + opt.iOuterAvatarPadding + opt.iInnerAvatarPadding;
				else
					smr.right -= opt.iPadding;

				if (!pwd->rows[i].bValueNewline)
					smr.right -= pwd->iLabelWidth + opt.iValueIndent;

				if (pwd->rows[i].swzValue && pwd->rows[i].swzValue[0]) {
					if (!bStatusMsg && opt.bGetNewStatusMsg) {
						if (!mir_wstrcmp(pwd->rows[i].swzValue, L"%sys:status_msg%"))
							bStatusMsg = true;
					}

					DrawTextExt(hdc, pwd->rows[i].swzValue, -1, &smr, DT_CALCRECT | DT_LEFT | DT_WORDBREAK | DT_END_ELLIPSIS | DT_NOPREFIX, pwd->rows[i].spi);
				}
				else smr.left = smr.right = 0;

				// save so we don't have to recalculate
				pwd->rows[i].iValueHeight = smr.bottom;

				pwd->rows[i].iTotalHeight = (pwd->rows[i].bLineAbove ? opt.iTextPadding : 0);
				if (pwd->rows[i].bValueNewline) {
					if (sz.cy) pwd->rows[i].iTotalHeight += sz.cy + opt.iTextPadding;
					if (smr.bottom) pwd->rows[i].iTotalHeight += smr.bottom + opt.iTextPadding;
				}
				else {
					int maxheight = max(sz.cy, smr.bottom);
					if (maxheight) pwd->rows[i].iTotalHeight += maxheight + opt.iTextPadding;
				}

				// only consider this item's width, and include it's height, if it doesn't make the window too big
				if (max(pwd->iTitleHeight + pwd->iTextHeight + opt.iPadding + pwd->rows[i].iTotalHeight, pwd->iAvatarHeight) <= opt.iWinMaxHeight || pwd->bIsTrayTip) {
					if (iWidth < opt.iWinWidth) {
						int wid = opt.iPadding + pwd->iIndent + (pwd->rows[i].bValueNewline ? max(sz.cx, smr.right - smr.left) : pwd->iLabelWidth + opt.iValueIndent + (smr.right - smr.left));
						if (pwd->iTitleHeight + pwd->iTextHeight + opt.iTextPadding < pwd->iAvatarHeight)
							iWidth = max(iWidth, wid + pwd->iRealAvatarWidth + opt.iOuterAvatarPadding + opt.iInnerAvatarPadding);
						else
							iWidth = max(iWidth, wid + opt.iPadding);
					}

					pwd->iTextHeight += pwd->rows[i].iTotalHeight;
				}
			}

			SelectObject(hdc, hOldFont);
			ReleaseDC(hwnd, hdc);

			int iHeight = max(pwd->iTitleHeight + pwd->iTextHeight + opt.iPadding, iWinAvatarHeight);
			iHeight = max(pwd->iIconsHeight, iHeight);
			if (bStatusMsg) iHeight += 50;

			if (iHeight < opt.iMinHeight) iHeight = opt.iMinHeight;
			// ignore minwidth for text tips
			if (!pwd->bIsTextTip && iWidth < opt.iMinWidth) iWidth = opt.iMinWidth;

			// ignore maxheight for tray tip
			if (!pwd->bIsTrayTip && iHeight > opt.iWinMaxHeight) iHeight = opt.iWinMaxHeight;
			if (iWidth > opt.iWinWidth) iWidth = opt.iWinWidth;

			CreateSkinBitmap(iWidth, iHeight, pwd->bIsTextTip && !pwd->bIsTrayTip);

			GetWindowRect(hwnd, &rc);
			if (rc.right - rc.left != iWidth || rc.bottom - rc.top != iHeight) {
				SetWindowPos(hwnd, nullptr, 0, 0, iWidth, iHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
				GetWindowRect(hwnd, &pwd->rcWindow);
				SendMessage(hwnd, PUM_UPDATERGN, 0, 0);
				InvalidateRect(hwnd, nullptr, TRUE);
			}

			if (pHeight)
				*pHeight = iHeight;
		}
		return TRUE;

	case PUM_UPDATERGN:
		HRGN hRgn;
		{
			RECT r = pwd->rcWindow;
			int w = 11;

			r.right -= r.left;
			r.left = 0;
			r.bottom -= r.top;
			r.top = 0;

			if (opt.showEffect == PSE_FADE && wParam == 1) {
				if (skin.bNeedLayerUpdate) {
					POINT ptSrc = { 0, 0 };
					SIZE sz = { r.right - r.left, r.bottom - r.top };

					BLENDFUNCTION blend;
					blend.BlendOp = AC_SRC_OVER;
					blend.BlendFlags = 0;
					blend.SourceConstantAlpha = pwd->iCurrentTrans;
					blend.AlphaFormat = AC_SRC_ALPHA;

					pwd->iCurrentTrans += opt.iAnimateSpeed;
					if (pwd->iCurrentTrans > pwd->iTrans) {
						pwd->iCurrentTrans = pwd->iTrans;
						pwd->iAnimStep = ANIM_STEPS;
					}

					UpdateLayeredWindow(hwnd, nullptr, nullptr, &sz, skin.hdc, &ptSrc, 0xffffffff, &blend, LWA_ALPHA);

				}
				else {
					pwd->iCurrentTrans += opt.iAnimateSpeed;
					if (pwd->iCurrentTrans > pwd->iTrans) {
						pwd->iCurrentTrans = pwd->iTrans;
						pwd->iAnimStep = ANIM_STEPS;
					}

					SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), pwd->iCurrentTrans, LWA_ALPHA);
				}
			}
			else if (opt.showEffect == PSE_ANIMATE && pwd->bIsPainted) {
				if (pwd->iAnimStep <= (ANIM_STEPS / opt.iAnimateSpeed)) {
					float frac = 1.0f - pwd->iAnimStep / ((float)ANIM_STEPS / opt.iAnimateSpeed);
					int wi = r.right, hi = r.bottom;

					r.left += (int)(wi / 2.0f * frac + 0.5f);
					r.right -= (int)(wi / 2.0f * frac + 0.5f);
					r.top += (int)(hi / 2.0f * frac + 0.5f);
					r.bottom -= (int)(hi / 2.0f * frac + 0.5f);
				}
				else pwd->iAnimStep = ANIM_STEPS;

				if (skin.bNeedLayerUpdate) {
					RECT r2 = pwd->rcWindow;
					POINT ptPos = { r.left + r2.left, r.top + r2.top };
					POINT ptSrc = { r.left, r.top };

					SIZE sz = { r.right - r.left, r.bottom - r.top };

					BLENDFUNCTION blend;
					blend.BlendOp = AC_SRC_OVER;
					blend.BlendFlags = 0;
					blend.SourceConstantAlpha = pwd->iTrans;
					blend.AlphaFormat = AC_SRC_ALPHA;

					UpdateLayeredWindow(hwnd, nullptr, &ptPos, &sz, skin.hdc, &ptSrc, 0xffffffff, &blend, LWA_ALPHA);
				}
			}

			if (!skin.bNeedLayerUpdate) {
				// round corners
				int v, h;
				if (opt.bRound) {
					h = (r.right - r.left) > (w * 2) ? w : (r.right - r.left);
					v = (r.bottom - r.top) > (w * 2) ? w : (r.bottom - r.top);
					h = (h < v) ? h : v;
				}
				else h = 0;

				hRgn = CreateRoundRectRgn(r.left, r.top, r.right + 1, r.bottom + 1, h, h);
				SetWindowRgn(hwnd, hRgn, FALSE);

				if (opt.showEffect == PSE_ANIMATE && pwd->iAnimStep == 1)
					SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), pwd->iTrans, LWA_ALPHA);
			}
			else {
				// aero glass (vista+)
				if (opt.bAeroGlass && MyDwmEnableBlurBehindWindow && pwd->iAnimStep > 5) {
					if (pwd->hrgnAeroGlass) {
						DeleteObject(pwd->hrgnAeroGlass);
						pwd->hrgnAeroGlass = nullptr;
					}

					pwd->hrgnAeroGlass = CreateOpaqueRgn(25, true);

					if (pwd->hrgnAeroGlass) {
						DWM_BLURBEHIND bb = { 0 };
						bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
						bb.fEnable = TRUE;
						bb.hRgnBlur = pwd->hrgnAeroGlass;
						MyDwmEnableBlurBehindWindow(hwnd, &bb);
					}
				}
			}
		}
		return TRUE;

	case PUM_CALCPOS:
		RECT rcWork;
		{
			SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, FALSE);

			HMONITOR hMon = MonitorFromPoint(pwd->clcit.ptCursor, MONITOR_DEFAULTTONEAREST);
			MONITORINFO mi;
			mi.cbSize = sizeof(mi);
			if (GetMonitorInfo(hMon, &mi))
				rcWork = mi.rcWork;

			GetWindowRect(hwnd, &rc);

			int x = 0, y = 0, iWidth = (rc.right - rc.left), iHeight = (rc.bottom - rc.top);

			switch (opt.pos) {
			case PP_BOTTOMRIGHT:
				x = pwd->clcit.ptCursor.x + GetSystemMetrics(SM_CXSMICON); // cursor size is too large - use small icon size
				y = pwd->clcit.ptCursor.y + GetSystemMetrics(SM_CYSMICON);
				break;
			case PP_BOTTOMLEFT:
				x = pwd->clcit.ptCursor.x - iWidth - GetSystemMetrics(SM_CXSMICON);
				y = pwd->clcit.ptCursor.y + GetSystemMetrics(SM_CYSMICON);
				break;
			case PP_TOPRIGHT:
				x = pwd->clcit.ptCursor.x + GetSystemMetrics(SM_CXSMICON);
				y = pwd->clcit.ptCursor.y - iHeight - GetSystemMetrics(SM_CYSMICON);
				break;
			case PP_TOPLEFT:
				x = pwd->clcit.ptCursor.x - iWidth - GetSystemMetrics(SM_CXSMICON);
				y = pwd->clcit.ptCursor.y - iHeight - GetSystemMetrics(SM_CYSMICON);
				break;
			}


			if (x + iWidth + 8 > rcWork.right)
				x = rcWork.right - iWidth - 8;
			if (x - 8 < rcWork.left)
				x = rcWork.left + 8;

			if (pwd->bAllowReposition || !pwd->bNeedRefresh) {
				if (y + iHeight > rcWork.bottom) {
					y = pwd->clcit.ptCursor.y - iHeight - 8;
					pwd->bAllowReposition = true;
				}

				if (y - 8 < rcWork.top) {
					y = pwd->clcit.ptCursor.y + GetSystemMetrics(SM_CYSMICON);
					pwd->bAllowReposition = true;
				}
			}

			SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
			GetWindowRect(hwnd, &pwd->rcWindow);
		}
		return TRUE;

	case PUM_REFRESHTRAYTIP:
		for (int i = 0; i < pwd->iRowCount; i++) {
			mir_free(pwd->rows[i].swzLabel);
			mir_free(pwd->rows[i].swzValue);
			Smileys_FreeParse(pwd->rows[i].spi);
		}

		mir_free(pwd->rows);
		pwd->rows = nullptr;
		pwd->iRowCount = 0;

		uint32_t dwItems = (wParam == 0) ? opt.iFirstItems : opt.iSecondItems;
		bool bFirstItem = true;
		wchar_t buff[64];

		int oldOrder = -1, iProtoCount = 0;
		PROTOACCOUNT **accs;
		Proto_EnumAccounts(&iProtoCount, &accs);

		for (int j = 0; j < iProtoCount; j++) {
			PROTOACCOUNT *pa = nullptr;
			for (int i = 0; i < iProtoCount; i++)
				if (accs[i]->iOrder > oldOrder && (pa == nullptr || accs[i]->iOrder < pa->iOrder))
					pa = accs[i];

			if (pa == nullptr)
				continue;

			oldOrder = pa->iOrder;

			if (opt.bHideOffline && pa->iRealStatus == ID_STATUS_OFFLINE)
				continue;

			if (!pa->IsEnabled() || !IsTrayProto(pa->tszAccountName, (BOOL)wParam))
				continue;

			if (dwItems & TRAYTIP_NUMCONTACTS) {
				int iCount = 0, iCountOnline = 0;
				for (auto &hContact : Contacts(pa->szModuleName)) {
					if (db_get_w(hContact, pa->szModuleName, "Status", ID_STATUS_OFFLINE) != ID_STATUS_OFFLINE)
						iCountOnline++;
					iCount++;
				}
				mir_snwprintf(buff, L"(%d/%d)", iCountOnline, iCount);
			}
			else buff[0] = 0;

			wchar_t swzProto[256];
			mir_wstrcpy(swzProto, pa->tszAccountName);
			if (dwItems & TRAYTIP_LOCKSTATUS)
				if (pa->IsLocked())
					mir_snwprintf(swzProto, TranslateT("%s (locked)"), pa->tszAccountName);

			AddRow(pwd, swzProto, buff, nullptr, false, false, !bFirstItem, true, Skin_LoadProtoIcon(pa->szModuleName, pa->iRealStatus));
			bFirstItem = false;

			if (dwItems & TRAYTIP_LOGON) {
				if (TimestampToTimeDifference(NULL, pa->szModuleName, "LogonTS", buff, 59)) {
					wchar_t ago[96];
					mir_snwprintf(ago, TranslateT("%s ago"), buff);
					AddRow(pwd, TranslateT("Log on:"), ago, nullptr, false, false, false);
				}
			}

			if (dwItems & TRAYTIP_UNREAD_EMAILS && ProtoServiceExists(pa->szModuleName, PS_GETUNREADEMAILCOUNT)) {
				int iCount = (int)CallProtoService(pa->szModuleName, PS_GETUNREADEMAILCOUNT, 0, 0);
				if (iCount > 0) {
					_itow(iCount, buff, 10);
					AddRow(pwd, TranslateT("Unread emails:"), buff, nullptr, false, false, false);
				}
			}

			if (dwItems & TRAYTIP_STATUS) {
				wchar_t *swzText = Clist_GetStatusModeDescription(pa->iRealStatus, 0);
				if (swzText)
					AddRow(pwd, TranslateT("Status:"), swzText, nullptr, false, false, false);
			}

			if (pa->iRealStatus >= ID_STATUS_ONLINE && pa->iRealStatus <= ID_STATUS_MAX) {
				if (dwItems & TRAYTIP_STATUS_MSG) {
					wchar_t *swzText = GetProtoStatusMessage(pa->szModuleName, pa->iRealStatus);
					if (swzText) {
						StripBBCodesInPlace(swzText);
						AddRow(pwd, TranslateT("Status message:"), swzText, pa->szModuleName, true, true, false);
						mir_free(swzText);
					}
				}

				if (dwItems & TRAYTIP_EXTRA_STATUS) {
					// jabber mood or icq xstatus
					wchar_t *swzAdvTitle = GetJabberAdvStatusText(pa->szModuleName, "mood", "title");
					if (swzAdvTitle) {
						StripBBCodesInPlace(swzAdvTitle);
						AddRow(pwd, TranslateT("Mood:"), swzAdvTitle, pa->szModuleName, true, false, false);
						mir_free(swzAdvTitle);

						wchar_t *swzAdvText = GetJabberAdvStatusText(pa->szModuleName, "mood", "text");
						if (swzAdvText) {
							StripBBCodesInPlace(swzAdvText);
							AddRow(pwd, L"", swzAdvText, pa->szModuleName, true, true, false);
							mir_free(swzAdvText);
						}
					}
					else {
						if (db_get_b(0, pa->szModuleName, "XStatusId", 0)) {
							// xstatus title
							swzAdvTitle = GetProtoExtraStatusTitle(pa->szModuleName);
							if (swzAdvTitle) {
								StripBBCodesInPlace(swzAdvTitle);
								AddRow(pwd, TranslateT("xStatus:"), swzAdvTitle, pa->szModuleName, true, false, false);
								mir_free(swzAdvTitle);
							}

							// xstatus message
							wchar_t *swzAdvText = GetProtoExtraStatusMessage(pa->szModuleName);
							if (swzAdvText) {
								StripBBCodesInPlace(swzAdvText);
								AddRow(pwd, L"", swzAdvText, pa->szModuleName, true, true, false);
								mir_free(swzAdvText);
							}
						}
					}

					wchar_t *swzActTitle = GetJabberAdvStatusText(pa->szModuleName, "activity", "title");
					if (swzActTitle) {
						StripBBCodesInPlace(swzActTitle);
						AddRow(pwd, TranslateT("Activity:"), swzActTitle, pa->szModuleName, true, false, false);
						mir_free(swzActTitle);
					}

					wchar_t *swzActText = GetJabberAdvStatusText(pa->szModuleName, "activity", "text");
					if (swzActText) {
						StripBBCodesInPlace(swzActText);
						AddRow(pwd, L"", swzActText, pa->szModuleName, true, true, false);
						mir_free(swzActText);
					}
				}

				if (dwItems & TRAYTIP_LISTENINGTO) {
					wchar_t *swzListening = GetListeningTo(pa->szModuleName);
					if (swzListening) {
						StripBBCodesInPlace(swzListening);
						AddRow(pwd, TranslateT("Listening to:"), swzListening, nullptr, false, true, false);
						mir_free(swzListening);
					}
				}
			}
		}

		if (dwItems & TRAYTIP_FAVCONTACTS) {
			if (g_plugin.getDword("FavouriteContactsCount", 0)) {
				wchar_t swzName[256];
				wchar_t swzStatus[256];
				bool bTitlePainted = false;
				int iCount = 0, iCountOnline = 0;

				for (auto &hContact : Contacts()) {
					if (g_plugin.getByte(hContact, "FavouriteContact")) {
						char *proto = Proto_GetBaseAccountName(hContact);
						if (proto == nullptr)
							continue;

						uint16_t wStatus = db_get_w(hContact, proto, "Status", ID_STATUS_OFFLINE);
						WordToStatusDesc(hContact, proto, "Status", swzStatus, 256);

						if (wStatus != ID_STATUS_OFFLINE)
							iCountOnline++;

						iCount++;

						if (!(opt.iFavoriteContFlags & FAVCONT_HIDE_OFFLINE && wStatus == ID_STATUS_OFFLINE)) {
							if (!bTitlePainted) {
								AddRow(pwd, TranslateT("Fav. contacts"), nullptr, nullptr, false, false, !bFirstItem, true, nullptr);
								bFirstItem = false;
								bTitlePainted = true;
							}

							wchar_t *swzNick = Clist_GetContactDisplayName(hContact);
							if (opt.iFavoriteContFlags & FAVCONT_APPEND_PROTO) {
								wchar_t *swzProto = a2t(proto);
								mir_snwprintf(swzName, L"%s (%s)", swzNick, swzProto);
								mir_free(swzProto);
							}
							else mir_wstrcpy(swzName, swzNick);

							AddRow(pwd, swzName, swzStatus, nullptr, false, false, false);
						}
					}
				}

				int index = pwd->iRowCount - 1;
				if (opt.iFavoriteContFlags & FAVCONT_HIDE_OFFLINE)
					index -= iCountOnline;
				else
					index -= iCount;

				if (index >= 0 && (dwItems & TRAYTIP_NUMCONTACTS) && !((opt.iFavoriteContFlags & FAVCONT_HIDE_OFFLINE) && iCountOnline == 0)) {
					mir_snwprintf(buff, L"(%d/%d)", iCountOnline, iCount);
					pwd->rows[index].swzValue = mir_wstrdup(buff);
				}
			}
		}

		if (dwItems & TRAYTIP_MIRANDA_UPTIME) {
			if (TimestampToTimeDifference(0, MODULENAME, "MirandaStartTS", buff, 64)) {
				AddRow(pwd, TranslateT("Other"), L"", nullptr, false, false, !bFirstItem, true, nullptr);
				AddRow(pwd, TranslateT("Miranda uptime:"), buff, nullptr, false, false, false);
			}
		}

		if (dwItems & TRAYTIP_CLIST_EVENT && pwd->clcit.swzText) {
			wchar_t *pchBr = wcschr(pwd->clcit.swzText, '\n');
			wchar_t *pchBold = wcsstr(pwd->clcit.swzText, L"<b>");

			if (!pchBold || pchBold != pwd->clcit.swzText) {
				wchar_t swzText[256];
				mir_wstrcpy(swzText, pwd->clcit.swzText);
				if (pchBr) swzText[pchBr - pwd->clcit.swzText] = 0;
				AddRow(pwd, swzText, L"", nullptr, false, true, false, true, Skin_LoadIcon(SKINICON_OTHER_FILLEDBLOB));
			}
		}

		return TRUE;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
