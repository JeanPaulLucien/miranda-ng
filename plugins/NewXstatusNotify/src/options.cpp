/*
	NewXstatusNotify YM - Plugin for Miranda IM
	Copyright (c) 2001-2004 Luca Santarelli
	Copyright (c) 2005-2007 Vasilich
	Copyright (c) 2007-2011 yaho

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
	*/

#include "stdafx.h"

OPTIONS opt = { 0 };
TEMPLATES templates = { 0 };
BOOL UpdateListFlag = FALSE;
LIST<PROTOTEMPLATE> ProtoTemplates(10);
int LastItem = 0;

INT_PTR CALLBACK DlgProcAutoDisableOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

void LoadTemplates()
{
	DBGetStringDefault(0, "TPopupXChanged", templates.PopupXstatusChanged, _countof(templates.PopupXstatusChanged), DEFAULT_POPUP_CHANGED);
	DBGetStringDefault(0, "TPopupXRemoved", templates.PopupXstatusRemoved, _countof(templates.PopupXstatusRemoved), DEFAULT_POPUP_REMOVED);
	DBGetStringDefault(0, "TPopupXMsgChanged", templates.PopupXMsgChanged, _countof(templates.PopupXMsgChanged), DEFAULT_POPUP_MSGCHANGED);
	DBGetStringDefault(0, "TPopupXMsgRemoved", templates.PopupXMsgRemoved, _countof(templates.PopupXMsgRemoved), DEFAULT_POPUP_MSGREMOVED);

	DBGetStringDefault(0, "TLogXChanged", templates.LogXstatusChanged, _countof(templates.LogXstatusChanged), DEFAULT_LOG_CHANGED);
	DBGetStringDefault(0, "TLogXRemoved", templates.LogXstatusRemoved, _countof(templates.LogXstatusRemoved), DEFAULT_LOG_REMOVED);
	DBGetStringDefault(0, "TLogXMsgChanged", templates.LogXMsgChanged, _countof(templates.LogXMsgChanged), DEFAULT_LOG_MSGCHANGED);
	DBGetStringDefault(0, "TLogXMsgRemoved", templates.LogXMsgRemoved, _countof(templates.LogXMsgRemoved), DEFAULT_LOG_MSGREMOVED);
	DBGetStringDefault(0, "TLogXOpening", templates.LogXstatusOpening, _countof(templates.LogXstatusOpening), DEFAULT_LOG_OPENING);

	DBGetStringDefault(0, "TLogSMsgChanged", templates.LogSMsgChanged, _countof(templates.LogSMsgChanged), DEFAULT_LOG_SMSGCHANGED);
	DBGetStringDefault(0, "TLogSMsgRemoved", templates.LogSMsgRemoved, _countof(templates.LogSMsgRemoved), DEFAULT_LOG_SMSGREMOVED);
	DBGetStringDefault(0, "TLogSMsgOpening", templates.LogSMsgOpening, _countof(templates.LogSMsgOpening), DEFAULT_LOG_SMSGOPENING);

	templates.PopupXFlags = g_plugin.getByte("TPopupXFlags", NOTIFY_NEW_XSTATUS | NOTIFY_NEW_MESSAGE);
	templates.PopupSMsgFlags = g_plugin.getByte("TPopupSMsgFlags", NOTIFY_NEW_MESSAGE);
	templates.LogXFlags = g_plugin.getByte("TLogXFlags", NOTIFY_NEW_XSTATUS | NOTIFY_NEW_MESSAGE | NOTIFY_OPENING_ML);
	templates.LogSMsgFlags = g_plugin.getByte("TLogSMsgFlags", NOTIFY_NEW_MESSAGE | NOTIFY_OPENING_ML);
}

void LoadOptions()
{
	// IDD_OPT_POPUP
	opt.Colors = g_plugin.getByte("Colors", DEFAULT_COLORS);
	opt.ShowGroup = g_plugin.getByte("ShowGroup", 0);
	opt.ShowStatus = g_plugin.getByte("ShowStatus", 1);
	opt.UseAlternativeText = g_plugin.getByte("UseAlternativeText", 0);
	opt.ShowPreviousStatus = g_plugin.getByte("ShowPreviousStatus", 0);
	opt.ReadAwayMsg = g_plugin.getByte("ReadAwayMsg", 0);
	opt.PopupTimeout = g_plugin.getDword("PopupTimeout", 0);
	opt.PopupConnectionTimeout = g_plugin.getDword("PopupConnectionTimeout", 15);
	opt.LeftClickAction = g_plugin.getByte("LeftClickAction", 5);
	opt.RightClickAction = g_plugin.getByte("RightClickAction", 1);
	
	// IDD_OPT_XPOPUP
	opt.PXOnConnect = g_plugin.getByte("PXOnConnect", 0);
	opt.PXDisableForMusic = g_plugin.getByte("PXDisableForMusic", 1);
	opt.PXMsgTruncate = g_plugin.getByte("PXMsgTruncate", 0);
	opt.PXMsgLen = g_plugin.getDword("PXMsgLen", 64);
	
	// IDD_OPT_GENERAL
	opt.FromOffline = g_plugin.getByte("FromOffline", 1);
	opt.AutoDisable = g_plugin.getByte("AutoDisable", 0);
	opt.HiddenContactsToo = g_plugin.getByte("HiddenContactsToo", 0);
	opt.UseIndSnd = g_plugin.getByte("UseIndSounds", 1);
	opt.BlinkIcon = g_plugin.getByte("BlinkIcon", 0);
	opt.BlinkIcon_Status = g_plugin.getByte("BlinkIcon_Status", 0);
	opt.BlinkIcon_ForMsgs = g_plugin.getByte("BlinkIcon_ForMsgs", 0);
	DBGetStringDefault(0, "LogFilePath", opt.LogFilePath, MAX_PATH, L"");
	
	// IDD_AUTODISABLE
	opt.OnlyGlobalChanges = g_plugin.getByte("OnlyGlobalChanges", 0);
	opt.DisablePopupGlobally = g_plugin.getByte("DisablePopupGlobally", 0);
	opt.DisableSoundGlobally = g_plugin.getByte("DisableSoundGlobally", 0);
	
	// IDD_OPT_LOG
	opt.LogToFile = g_plugin.getByte("LogToFile", 0);
	opt.LogToDB = g_plugin.getByte("LogToDB", 0);
	opt.LogToDB_WinOpen = g_plugin.getByte("LogToDB_WinOpen", 1);
	opt.LogToDB_Remove = g_plugin.getByte("LogToDB_Remove", 0);
	opt.LogPrevious = g_plugin.getByte("LogToPrevious", 0);
	opt.SMsgLogToFile = g_plugin.getByte("SMsgLogToFile", 0);
	opt.SMsgLogToDB = g_plugin.getByte("SMsgLogToDB", 0);
	opt.SMsgLogToDB_WinOpen = g_plugin.getByte("SMsgLogToDB_WinOpen", 1);
	opt.SMsgLogToDB_Remove = g_plugin.getByte("SMsgLogToDB_Remove", 0);
	
	// IDD_OPT_XLOG
	opt.XLogToFile = g_plugin.getByte("XLogToFile", 0);
	opt.XLogToDB = g_plugin.getByte("XLogToDB", 0);
	opt.XLogToDB_WinOpen = g_plugin.getByte("XLogToDB_WinOpen", 1);
	opt.XLogToDB_Remove = g_plugin.getByte("XLogToDB_Remove", 0);
	opt.XLogDisableForMusic = g_plugin.getByte("XLogDisableForMusic", 1);
	
	// IDD_OPT_SMPOPUP
	opt.PSMsgOnConnect = g_plugin.getByte("PSMsgOnConnect", 0);
	opt.PSMsgTruncate = g_plugin.getByte("PSMsgTruncate", 0);
	opt.PSMsgLen = g_plugin.getDword("PSMsgLen", 64);
	
	// OTHER
	opt.EnableLastSeen = g_plugin.getByte("EnableLastSeen", 0);

	LoadTemplates();
}

void SaveTemplates()
{
	g_plugin.setWString("TPopupXChanged", templates.PopupXstatusChanged);
	g_plugin.setWString("TPopupXRemoved", templates.PopupXstatusRemoved);
	g_plugin.setWString("TPopupXMsgChanged", templates.PopupXMsgChanged);
	g_plugin.setWString("TPopupXMsgRemoved", templates.PopupXMsgRemoved);

	g_plugin.setWString("TLogXChanged", templates.LogXstatusChanged);
	g_plugin.setWString("TLogXRemoved", templates.LogXstatusRemoved);
	g_plugin.setWString("TLogXMsgChanged", templates.LogXMsgChanged);
	g_plugin.setWString("TLogXMsgRemoved", templates.LogXMsgRemoved);
	g_plugin.setWString("TLogXOpening", templates.LogXstatusOpening);

	g_plugin.setWString("TLogSMsgChanged", templates.LogSMsgChanged);
	g_plugin.setWString("TLogSMsgRemoved", templates.LogSMsgRemoved);
	g_plugin.setWString("TLogSMsgOpening", templates.LogSMsgOpening);

	g_plugin.setByte("TPopupXFlags", templates.PopupXFlags);
	g_plugin.setByte("TPopupSMsgFlags", templates.PopupSMsgFlags);
	g_plugin.setByte("TLogXFlags", templates.LogXFlags);
	g_plugin.setByte("TLogSMsgFlags", templates.LogSMsgFlags);

	for (auto &it : ProtoTemplates) {
		char str[MAX_PATH];
		mir_snprintf(str, "%s_TPopupSMsgChanged", it->ProtoName);
		g_plugin.setWString(str, it->ProtoTemplateMsg);
		mir_snprintf(str, "%s_TPopupSMsgRemoved", it->ProtoName);
		g_plugin.setWString(str, it->ProtoTemplateRemoved);
	}
}

void SaveOptions()
{
	// IDD_OPT_POPUP
	g_plugin.setByte("Colors", opt.Colors);
	g_plugin.setByte("ShowGroup", opt.ShowGroup);
	g_plugin.setByte("ShowStatus", opt.ShowStatus);
	g_plugin.setByte("UseAlternativeText", opt.UseAlternativeText);
	g_plugin.setByte("ShowPreviousStatus", opt.ShowPreviousStatus);
	g_plugin.setByte("ReadAwayMsg", opt.ReadAwayMsg);
	g_plugin.setDword("PopupTimeout", opt.PopupTimeout);
	g_plugin.setDword("PopupConnectionTimeout", opt.PopupConnectionTimeout);
	g_plugin.setByte("LeftClickAction", opt.LeftClickAction);
	g_plugin.setByte("RightClickAction", opt.RightClickAction);
	// IDD_OPT_XPOPUP
	g_plugin.setByte("PXOnConnect", opt.PXOnConnect);
	g_plugin.setByte("PXDisableForMusic", opt.PXDisableForMusic);
	g_plugin.setByte("PXMsgTruncate", opt.PXMsgTruncate);
	g_plugin.setDword("PXMsgLen", opt.PXMsgLen);
	// IDD_OPT_GENERAL
	g_plugin.setByte("FromOffline", opt.FromOffline);
	g_plugin.setByte("AutoDisable", opt.AutoDisable);
	g_plugin.setByte("HiddenContactsToo", opt.HiddenContactsToo);
	g_plugin.setByte("UseIndSounds", opt.UseIndSnd);
	g_plugin.setByte("BlinkIcon", opt.BlinkIcon);
	g_plugin.setByte("BlinkIcon_Status", opt.BlinkIcon_Status);
	g_plugin.setByte("BlinkIcon_ForMsgs", opt.BlinkIcon_ForMsgs);
	g_plugin.setWString("LogFilePath", opt.LogFilePath);
	// IDD_AUTODISABLE
	g_plugin.setByte("OnlyGlobalChanges", opt.OnlyGlobalChanges);
	g_plugin.setByte("DisablePopupGlobally", opt.DisablePopupGlobally);
	g_plugin.setByte("DisableSoundGlobally", opt.DisableSoundGlobally);
	// IDD_OPT_LOG
	g_plugin.setByte("LogToFile", opt.LogToFile);
	g_plugin.setByte("LogToDB", opt.LogToDB);
	g_plugin.setByte("LogToDB_WinOpen", opt.LogToDB_WinOpen);
	g_plugin.setByte("LogToDB_Remove", opt.LogToDB_Remove);
	g_plugin.setByte("LogPrevious", opt.LogPrevious);
	g_plugin.setByte("SMsgLogToFile", opt.SMsgLogToFile);
	g_plugin.setByte("SMsgLogToDB", opt.SMsgLogToDB);
	g_plugin.setByte("SMsgLogToDB_WinOpen", opt.SMsgLogToDB_WinOpen);
	g_plugin.setByte("SMsgLogToDB_Remove", opt.SMsgLogToDB_Remove);
	// IDD_OPT_XLOG
	g_plugin.setByte("XLogToFile", opt.XLogToFile);
	g_plugin.setByte("XLogToDB", opt.XLogToDB);
	g_plugin.setByte("XLogToDB_WinOpen", opt.XLogToDB_WinOpen);
	g_plugin.setByte("XLogToDB_Remove", opt.XLogToDB_Remove);
	g_plugin.setByte("XLogDisableForMusic", opt.XLogDisableForMusic);
	// IDD_OPT_SMPOPUP
	g_plugin.setByte("PSMsgOnConnect", opt.PSMsgOnConnect);
	g_plugin.setByte("PSMsgTruncate", opt.PSMsgTruncate);
	g_plugin.setDword("PSMsgLen", opt.PSMsgLen);
}

INT_PTR CALLBACK DlgProcGeneralOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);

		CheckDlgButton(hwndDlg, IDC_AUTODISABLE, opt.AutoDisable ? BST_CHECKED : BST_UNCHECKED);
		EnableWindow(GetDlgItem(hwndDlg, IDC_CONFIGUREAUTODISABLE), opt.AutoDisable);

		CheckDlgButton(hwndDlg, IDC_HIDDENCONTACTSTOO, opt.HiddenContactsToo ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_USEINDIVSOUNDS, opt.UseIndSnd ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_BLINKICON, opt.BlinkIcon ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_BLINKICON_STATUS, opt.BlinkIcon_Status ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_BLINKICON_FORMSGS, opt.BlinkIcon_ForMsgs ? BST_CHECKED : BST_UNCHECKED);
		EnableWindow(GetDlgItem(hwndDlg, IDC_BLINKICON_STATUS), opt.BlinkIcon);
		EnableWindow(GetDlgItem(hwndDlg, IDC_BLINKICON_FORMSGS), opt.BlinkIcon);

		SetDlgItemText(hwndDlg, IDC_LOGFILE, opt.LogFilePath);

		for (int i = ID_STATUS_MIN; i <= ID_STATUS_MAX2; i++) {
			//Statuses notified
			char status[8];
			mir_snprintf(status, "%d", i);
			CheckDlgButton(hwndDlg, i, g_plugin.getByte(status, 1) ? BST_CHECKED : BST_UNCHECKED);
		}
		CheckDlgButton(hwndDlg, IDC_CHK_FROMOFFLINE, opt.FromOffline ? BST_CHECKED : BST_UNCHECKED);

		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CONFIGUREAUTODISABLE:
			CreateDialog(g_plugin.getInst(), MAKEINTRESOURCE(IDD_AUTODISABLE), hwndDlg, DlgProcAutoDisableOpts);
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			return FALSE;
		case IDC_AUTODISABLE:
			EnableWindow(GetDlgItem(hwndDlg, IDC_CONFIGUREAUTODISABLE), IsDlgButtonChecked(hwndDlg, IDC_AUTODISABLE));
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		case IDC_BLINKICON:
			EnableWindow(GetDlgItem(hwndDlg, IDC_BLINKICON_STATUS), IsDlgButtonChecked(hwndDlg, IDC_BLINKICON));
			EnableWindow(GetDlgItem(hwndDlg, IDC_BLINKICON_FORMSGS), IsDlgButtonChecked(hwndDlg, IDC_BLINKICON));
			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		case IDC_BT_VIEWLOG:
			ShowLog(opt.LogFilePath);
			break;
		case IDC_BT_CHOOSELOGFILE:
			{
				wchar_t buff[MAX_PATH];
				OPENFILENAME ofn = { 0 };

				GetDlgItemText(hwndDlg, IDC_LOGFILE, buff, _countof(buff));

				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.lpstrFile = buff;
				ofn.nMaxFile = MAX_PATH;
				ofn.hwndOwner = hwndDlg;
				wchar_t filter[MAX_PATH];
				mir_snwprintf(filter, L"%s (*.*)%c*.*%c%s (*.log)%c*.log%c%s (*.txt)%c*.txt%c", TranslateT("All Files"), 0, 0, TranslateT("Log"), 0, 0, TranslateT("Text"), 0, 0);
				ofn.lpstrFilter = filter;
				ofn.nFilterIndex = 2;
				ofn.lpstrInitialDir = buff;
				ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
				ofn.lpstrDefExt = L"log";
				if (GetSaveFileName(&ofn)) {
					SetDlgItemText(hwndDlg, IDC_LOGFILE, buff);
					SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				}
				break;
			}
		default:
			if (HIWORD(wParam) == BN_CLICKED || (HIWORD(wParam) == EN_CHANGE && (HWND)lParam == GetFocus()))
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
		}
		break;

	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->code == PSN_APPLY) {
			opt.AutoDisable = IsDlgButtonChecked(hwndDlg, IDC_AUTODISABLE);
			opt.BlinkIcon = IsDlgButtonChecked(hwndDlg, IDC_BLINKICON);
			opt.BlinkIcon_Status = IsDlgButtonChecked(hwndDlg, IDC_BLINKICON_STATUS);
			opt.BlinkIcon_ForMsgs = IsDlgButtonChecked(hwndDlg, IDC_BLINKICON_FORMSGS);
			opt.HiddenContactsToo = IsDlgButtonChecked(hwndDlg, IDC_HIDDENCONTACTSTOO);
			opt.UseIndSnd = IsDlgButtonChecked(hwndDlg, IDC_USEINDIVSOUNDS);

			GetDlgItemText(hwndDlg, IDC_LOGFILE, opt.LogFilePath, _countof(opt.LogFilePath));

			//Notified statuses
			for (int i = ID_STATUS_MIN; i <= ID_STATUS_MAX2; i++) {
				char status[8];
				mir_snprintf(status, "%d", i);
				g_plugin.setByte(status, (uint8_t)IsDlgButtonChecked(hwndDlg, i));
			}
			opt.FromOffline = IsDlgButtonChecked(hwndDlg, IDC_CHK_FROMOFFLINE);

			SaveOptions();
			return TRUE;
		}
	}

	return FALSE;
}

INT_PTR CALLBACK DlgProcPopupOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);

		CheckDlgButton(hwndDlg, IDC_USEOWNCOLORS, (opt.Colors == POPUP_COLOR_OWN) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_USEPOPUPCOLORS, (opt.Colors == POPUP_COLOR_POPUP) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_USEWINCOLORS, (opt.Colors == POPUP_COLOR_WINDOWS) ? BST_CHECKED : BST_UNCHECKED);

		for (int i = ID_STATUS_MIN; i <= ID_STATUS_MAX2; i++) {
			SendDlgItemMessage(hwndDlg, (i + 2000), CPM_SETCOLOUR, 0, StatusList[Index(i)].colorBack);
			SendDlgItemMessage(hwndDlg, (i + 1000), CPM_SETCOLOUR, 0, StatusList[Index(i)].colorText);
			EnableWindow(GetDlgItem(hwndDlg, (i + 2000)), (opt.Colors == POPUP_COLOR_OWN));
			EnableWindow(GetDlgItem(hwndDlg, (i + 1000)), (opt.Colors == POPUP_COLOR_OWN));
		}

		CheckDlgButton(hwndDlg, IDC_SHOWSTATUS, opt.ShowStatus ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_SHOWALTDESCS, opt.UseAlternativeText ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_READAWAYMSG, opt.ReadAwayMsg ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_SHOWPREVIOUSSTATUS, opt.ShowPreviousStatus ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_SHOWGROUP, opt.ShowGroup ? BST_CHECKED : BST_UNCHECKED);

		SendDlgItemMessage(hwndDlg, IDC_TIMEOUT_VALUE, EM_LIMITTEXT, 3, 0);
		SendDlgItemMessage(hwndDlg, IDC_TIMEOUT_VALUE_SPIN, UDM_SETRANGE32, -1, 999);
		SetDlgItemInt(hwndDlg, IDC_TIMEOUT_VALUE, opt.PopupTimeout, TRUE);

		SendDlgItemMessage(hwndDlg, IDC_CONNECTIONTIMEOUT_VALUE, EM_LIMITTEXT, 3, 0);
		SendDlgItemMessage(hwndDlg, IDC_CONNECTIONTIMEOUT_VALUE_SPIN, UDM_SETRANGE32, 0, 999);
		SetDlgItemInt(hwndDlg, IDC_CONNECTIONTIMEOUT_VALUE, opt.PopupConnectionTimeout, TRUE);

		//Mouse actions
		for (int i = 0; i < _countof(PopupActions); i++) {
			SendDlgItemMessage(hwndDlg, IDC_STATUS_LC, CB_SETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_STATUS_LC, CB_ADDSTRING, 0, (LPARAM)TranslateW(PopupActions[i].Text)), PopupActions[i].Action);
			SendDlgItemMessage(hwndDlg, IDC_STATUS_RC, CB_SETITEMDATA, SendDlgItemMessage(hwndDlg, IDC_STATUS_RC, CB_ADDSTRING, 0, (LPARAM)TranslateW(PopupActions[i].Text)), PopupActions[i].Action);
		}

		SendDlgItemMessage(hwndDlg, IDC_STATUS_LC, CB_SETCURSEL, opt.LeftClickAction, 0);
		SendDlgItemMessage(hwndDlg, IDC_STATUS_RC, CB_SETCURSEL, opt.RightClickAction, 0);

		SendMessage(hwndDlg, WM_USER + 1, (WPARAM)opt.ShowStatus, 0);
		return TRUE;

	case WM_COMMAND:
		{
			uint16_t idCtrl = LOWORD(wParam);
			if (HIWORD(wParam) == CPN_COLOURCHANGED) {
				if (idCtrl >= IDC_CHK_OFFLINE) {
					COLORREF colour = SendDlgItemMessage(hwndDlg, idCtrl, CPM_GETCOLOUR, 0, 0);
					if ((idCtrl >= IDC_OFFLINE_TX) && (idCtrl <= IDC_STATUSMSG_TX)) //Text colour
						StatusList[Index(idCtrl - 1000)].colorText = colour;
					else // Background colour
						StatusList[Index(idCtrl - 2000)].colorBack = colour;
					SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
					return TRUE;
				}
			}

			if (HIWORD(wParam) == BN_CLICKED) {
				switch (idCtrl) {
				case IDC_USEOWNCOLORS:
				case IDC_USEWINCOLORS:
				case IDC_USEPOPUPCOLORS:
					for (int i = ID_STATUS_MIN; i <= ID_STATUS_MAX2; i++) {
						EnableWindow(GetDlgItem(hwndDlg, (i + 2000)), idCtrl == IDC_USEOWNCOLORS); //Background
						EnableWindow(GetDlgItem(hwndDlg, (i + 1000)), idCtrl == IDC_USEOWNCOLORS); //Text
					}
					break;

				case IDC_SHOWSTATUS:
					SendMessage(hwndDlg, WM_USER + 1, (WPARAM)IsDlgButtonChecked(hwndDlg, IDC_SHOWSTATUS), 0);
					break;

				case IDC_PREVIEW:
					wchar_t str[MAX_SECONDLINE];
					for (int i = ID_STATUS_MIN; i <= ID_STATUS_MAX; i++) {
						str[0] = 0;

						if (opt.ShowStatus) {
							if (opt.UseAlternativeText == 1)
								wcsncpy(str, StatusList[Index(i)].lpzUStatusText, _countof(str));
							else
								wcsncpy(str, StatusList[Index(i)].lpzStandardText, _countof(str));

							if (opt.ShowPreviousStatus) {
								wchar_t buff[MAX_STATUSTEXT];
								mir_snwprintf(buff, TranslateW(STRING_SHOWPREVIOUSSTATUS), StatusList[Index(i)].lpzStandardText);
								mir_wstrcat(str, L" ");
								mir_wstrcat(str, buff);
							}
						}

						if (opt.ReadAwayMsg) {
							if (str[0])
								mir_wstrcat(str, L"\n");
							mir_wstrcat(str, TranslateT("This is status message"));
						}

						ShowChangePopup(NULL, Skin_LoadProtoIcon(nullptr, i), i, str);
					}

					ShowChangePopup(NULL, Skin_LoadProtoIcon(nullptr, ID_STATUS_ONLINE), ID_STATUS_EXTRASTATUS, TranslateT("This is extra status"));
					ShowChangePopup(NULL, Skin_LoadProtoIcon(nullptr, ID_STATUS_ONLINE), ID_STATUS_STATUSMSG, TranslateT("This is status message"));
					return FALSE;
				}
			}

			if ((HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == EN_CHANGE) && (HWND)lParam == GetFocus())
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);

			break;
		}
	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->code == PSN_APPLY) {
			char str[8];
			uint32_t ctlColour = 0;
			for (int i = ID_STATUS_MIN; i <= ID_STATUS_MAX2; i++) {
				ctlColour = SendDlgItemMessage(hwndDlg, (i + 2000), CPM_GETCOLOUR, 0, 0);
				StatusList[Index(i)].colorBack = SendDlgItemMessage(hwndDlg, (i + 2000), CPM_GETCOLOUR, 0, 0);
				mir_snprintf(str, "%ibg", i);
				g_plugin.setDword(str, ctlColour);

				ctlColour = SendDlgItemMessage(hwndDlg, (i + 1000), CPM_GETCOLOUR, 0, 0);
				StatusList[Index(i)].colorText = ctlColour;
				mir_snprintf(str, "%itx", i);
				g_plugin.setDword(str, ctlColour);
			}

			if (IsDlgButtonChecked(hwndDlg, IDC_USEOWNCOLORS))
				opt.Colors = POPUP_COLOR_OWN;
			else if (IsDlgButtonChecked(hwndDlg, IDC_USEPOPUPCOLORS))
				opt.Colors = POPUP_COLOR_POPUP;
			else
				opt.Colors = POPUP_COLOR_WINDOWS;

			opt.ShowStatus = IsDlgButtonChecked(hwndDlg, IDC_SHOWSTATUS);
			opt.UseAlternativeText = IsDlgButtonChecked(hwndDlg, IDC_SHOWALTDESCS);
			opt.ReadAwayMsg = IsDlgButtonChecked(hwndDlg, IDC_READAWAYMSG);
			opt.ShowPreviousStatus = IsDlgButtonChecked(hwndDlg, IDC_SHOWPREVIOUSSTATUS);
			opt.ShowGroup = IsDlgButtonChecked(hwndDlg, IDC_SHOWGROUP);
			opt.PopupTimeout = GetDlgItemInt(hwndDlg, IDC_TIMEOUT_VALUE, nullptr, TRUE);
			opt.PopupConnectionTimeout = GetDlgItemInt(hwndDlg, IDC_CONNECTIONTIMEOUT_VALUE, nullptr, TRUE);
			opt.LeftClickAction = (uint8_t)SendDlgItemMessage(hwndDlg, IDC_STATUS_LC, CB_GETCURSEL, 0, 0);
			opt.RightClickAction = (uint8_t)SendDlgItemMessage(hwndDlg, IDC_STATUS_RC, CB_GETCURSEL, 0, 0);

			SaveOptions();
			return TRUE;
		}

		break;

	case WM_USER + 1:
		EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWALTDESCS), wParam);
		EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWPREVIOUSSTATUS), wParam);
		return TRUE;
	}

	return FALSE;
}

INT_PTR CALLBACK DlgProcAutoDisableOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM)
{
	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		CheckDlgButton(hwndDlg, IDC_CHK_PGLOBAL, opt.DisablePopupGlobally ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_CHK_SGLOBAL, opt.DisableSoundGlobally ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_CHK_ONLYGLOBAL, opt.OnlyGlobalChanges ? BST_CHECKED : BST_UNCHECKED);

		for (int i = ID_STATUS_MIN; i <= ID_STATUS_MAX; i++) {
			char str[8];
			mir_snprintf(str, "p%d", i);
			CheckDlgButton(hwndDlg, i, g_plugin.getByte(str, 0) ? BST_CHECKED : BST_UNCHECKED);
		}

		for (int i = ID_STATUS_MIN; i <= ID_STATUS_MAX; i++) {
			char str[8];
			mir_snprintf(str, "s%d", i);
			CheckDlgButton(hwndDlg, (i + 2000), g_plugin.getByte(str, 0) ? BST_CHECKED : BST_UNCHECKED);
		}

		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_OK:
			opt.DisablePopupGlobally = IsDlgButtonChecked(hwndDlg, IDC_CHK_PGLOBAL);
			opt.DisableSoundGlobally = IsDlgButtonChecked(hwndDlg, IDC_CHK_SGLOBAL);
			opt.OnlyGlobalChanges = IsDlgButtonChecked(hwndDlg, IDC_CHK_ONLYGLOBAL);

			for (int i = ID_STATUS_MIN; i <= ID_STATUS_MAX; i++) {
				char str[8];
				mir_snprintf(str, "p%d", i);
				g_plugin.setByte(str, IsDlgButtonChecked(hwndDlg, i));
			}

			for (int i = ID_STATUS_MIN; i <= ID_STATUS_MAX; i++) {
				char str[8];
				mir_snprintf(str, "s%d", i);
				g_plugin.setByte(str, IsDlgButtonChecked(hwndDlg, i + 2000));
			}

			SaveOptions();
			//Fall through
		case IDC_CANCEL:
			DestroyWindow(hwndDlg);
			break;
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hwndDlg);
		break;
	}

	return FALSE;
}

INT_PTR CALLBACK DlgProcXPopupOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);

		SendDlgItemMessage(hwndDlg, IDC_ED_MSGLEN, EM_LIMITTEXT, 3, 0);
		SendDlgItemMessage(hwndDlg, IDC_UD_MSGLEN, UDM_SETRANGE, 0, MAKELONG(999, 1));

		CheckDlgButton(hwndDlg, IDC_XONCONNECT, opt.PXOnConnect ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_CHK_DISABLEMUSIC, opt.PXDisableForMusic ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_CHK_CUTMSG, opt.PXMsgTruncate ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemInt(hwndDlg, IDC_ED_MSGLEN, opt.PXMsgLen, FALSE);

		// Templates
		CheckDlgButton(hwndDlg, IDC_CHK_XSTATUSCHANGE, templates.PopupXFlags & NOTIFY_NEW_XSTATUS ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_CHK_MSGCHANGE, templates.PopupXFlags & NOTIFY_NEW_MESSAGE ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_CHK_REMOVE, templates.PopupXFlags & NOTIFY_REMOVE_XSTATUS ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_CHK_MSGREMOVE, templates.PopupXFlags & NOTIFY_REMOVE_MESSAGE ? BST_CHECKED : BST_UNCHECKED);

		SetDlgItemText(hwndDlg, IDC_ED_TCHANGE, templates.PopupXstatusChanged);
		SetDlgItemText(hwndDlg, IDC_ED_TREMOVE, templates.PopupXstatusRemoved);
		SetDlgItemText(hwndDlg, IDC_ED_TCHANGEMSG, templates.PopupXMsgChanged);
		SetDlgItemText(hwndDlg, IDC_ED_TREMOVEMSG, templates.PopupXMsgRemoved);

		EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TCHANGE), templates.PopupXFlags & NOTIFY_NEW_XSTATUS);
		EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TCHANGEMSG), templates.PopupXFlags & NOTIFY_NEW_MESSAGE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TREMOVE), templates.PopupXFlags & NOTIFY_REMOVE_XSTATUS);
		EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TREMOVEMSG), templates.PopupXFlags & NOTIFY_REMOVE_MESSAGE);

		// Buttons
		SendDlgItemMessage(hwndDlg, IDC_BT_VARIABLES, BUTTONADDTOOLTIP, (WPARAM)LPGENW("Show available variables"), BATF_UNICODE);
		SendDlgItemMessage(hwndDlg, IDC_BT_VARIABLES, BM_SETIMAGE, IMAGE_ICON, (LPARAM)IcoLib_GetIconByHandle(iconList[ICO_VARIABLES].hIcolib));

		SendDlgItemMessage(hwndDlg, IDC_BT_RESET, BUTTONADDTOOLTIP, (WPARAM)LPGENW("Reset all templates to default"), BATF_UNICODE);
		SendDlgItemMessage(hwndDlg, IDC_BT_RESET, BM_SETIMAGE, IMAGE_ICON, (LPARAM)IcoLib_GetIconByHandle(iconList[ICO_RESET].hIcolib));
		return TRUE;

	case WM_COMMAND:
		switch (HIWORD(wParam)) {
		case BN_CLICKED:
			switch (LOWORD(wParam)) {
			case IDC_CHK_CUTMSG:
				EnableWindow(GetDlgItem(hwndDlg, IDC_ED_MSGLEN), IsDlgButtonChecked(hwndDlg, IDC_CHK_CUTMSG));
				break;
			case IDC_CHK_XSTATUSCHANGE:
				EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TCHANGE), IsDlgButtonChecked(hwndDlg, IDC_CHK_XSTATUSCHANGE));
				break;
			case IDC_CHK_REMOVE:
				EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TREMOVE), IsDlgButtonChecked(hwndDlg, IDC_CHK_REMOVE));
				break;
			case IDC_CHK_MSGCHANGE:
				EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TCHANGEMSG), IsDlgButtonChecked(hwndDlg, IDC_CHK_MSGCHANGE));
				break;
			case IDC_CHK_MSGREMOVE:
				EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TREMOVEMSG), IsDlgButtonChecked(hwndDlg, IDC_CHK_MSGREMOVE));
				break;
			case IDC_BT_VARIABLES:
				MessageBox(hwndDlg, VARIABLES_HELP_TEXT, TranslateT("Variables"), MB_OK | MB_ICONINFORMATION);
				break;
			case IDC_BT_RESET:
				if (MessageBox(hwndDlg,
					TranslateT("Do you want to reset all templates to default?"),
					TranslateT("Reset templates"),
					MB_ICONQUESTION | MB_YESNO) == IDYES)
					SendMessage(hwndDlg, WM_USER + 1, 0, 0);
				else
					return FALSE;
				break;
			}

			if (LOWORD(wParam) != IDC_BT_VARIABLES)
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);

			break;
		case EN_CHANGE:
			if ((HWND)lParam == GetFocus())
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		}

		return TRUE;

	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->code == PSN_APPLY) {
			opt.PXOnConnect = IsDlgButtonChecked(hwndDlg, IDC_XONCONNECT);
			opt.PXDisableForMusic = IsDlgButtonChecked(hwndDlg, IDC_CHK_DISABLEMUSIC);
			opt.PXMsgTruncate = IsDlgButtonChecked(hwndDlg, IDC_CHK_CUTMSG);
			opt.PXMsgLen = GetDlgItemInt(hwndDlg, IDC_ED_MSGLEN, nullptr, FALSE);

			templates.PopupXFlags = 0;
			templates.PopupXFlags |= (IsDlgButtonChecked(hwndDlg, IDC_CHK_XSTATUSCHANGE) ? NOTIFY_NEW_XSTATUS : 0) |
				(IsDlgButtonChecked(hwndDlg, IDC_CHK_MSGCHANGE) ? NOTIFY_NEW_MESSAGE : 0) |
				(IsDlgButtonChecked(hwndDlg, IDC_CHK_REMOVE) ? NOTIFY_REMOVE_XSTATUS : 0) |
				(IsDlgButtonChecked(hwndDlg, IDC_CHK_MSGREMOVE) ? NOTIFY_REMOVE_MESSAGE : 0);

			// Templates
			GetDlgItemText(hwndDlg, IDC_ED_TCHANGE, templates.PopupXstatusChanged, _countof(templates.PopupXstatusChanged));
			GetDlgItemText(hwndDlg, IDC_ED_TREMOVE, templates.PopupXstatusRemoved, _countof(templates.PopupXstatusRemoved));
			GetDlgItemText(hwndDlg, IDC_ED_TCHANGEMSG, templates.PopupXMsgChanged, _countof(templates.PopupXMsgChanged));
			GetDlgItemText(hwndDlg, IDC_ED_TREMOVEMSG, templates.PopupXMsgRemoved, _countof(templates.PopupXMsgRemoved));

			// Save options to db
			SaveOptions();
			SaveTemplates();
		}

		return TRUE;

	case WM_USER + 1:
		SetDlgItemText(hwndDlg, IDC_ED_TCHANGE, DEFAULT_POPUP_CHANGED);
		SetDlgItemText(hwndDlg, IDC_ED_TREMOVE, DEFAULT_POPUP_REMOVED);
		SetDlgItemText(hwndDlg, IDC_ED_TCHANGEMSG, DEFAULT_POPUP_MSGCHANGED);
		SetDlgItemText(hwndDlg, IDC_ED_TREMOVEMSG, DEFAULT_POPUP_MSGREMOVED);

		CheckDlgButton(hwndDlg, IDC_CHK_XSTATUSCHANGE, BST_CHECKED);
		CheckDlgButton(hwndDlg, IDC_CHK_REMOVE, BST_CHECKED);
		CheckDlgButton(hwndDlg, IDC_CHK_MSGCHANGE, BST_CHECKED);
		CheckDlgButton(hwndDlg, IDC_CHK_MSGREMOVE, BST_CHECKED);

		EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TCHANGE), TRUE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TREMOVE), TRUE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TCHANGEMSG), TRUE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TREMOVEMSG), TRUE);

		return TRUE;
	}

	return FALSE;
}

bool IsSuitableProto(PROTOACCOUNT *pa)
{
	return (pa != nullptr && !pa->bDynDisabled && pa->bIsEnabled && CallProtoService(pa->szProtoName, PS_GETCAPS, PFLAGNUM_2, 0) != 0 && CallProtoService(pa->szModuleName, PS_GETCAPS, PFLAGNUM_1, 0) & PF1_MODEMSGRECV);
}

INT_PTR CALLBACK DlgProcSMPopupOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);

		SendDlgItemMessage(hwndDlg, IDC_ED_SMSGLEN, EM_LIMITTEXT, 3, 0);
		SendDlgItemMessage(hwndDlg, IDC_UD_SMSGLEN, UDM_SETRANGE, 0, MAKELONG(999, 1));

		CheckDlgButton(hwndDlg, IDC_ONCONNECT, opt.PSMsgOnConnect ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_CHK_CUTSMSG, opt.PSMsgTruncate ? BST_CHECKED : BST_UNCHECKED);
		SetDlgItemInt(hwndDlg, IDC_ED_SMSGLEN, opt.PSMsgLen, FALSE);

		// Templates
		CheckDlgButton(hwndDlg, IDC_CHK_NEWSMSG, templates.PopupSMsgFlags & NOTIFY_NEW_MESSAGE ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_CHK_SMSGREMOVE, templates.PopupSMsgFlags & NOTIFY_REMOVE_MESSAGE ? BST_CHECKED : BST_UNCHECKED);

		EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TNEWSMSG), templates.PopupSMsgFlags & NOTIFY_NEW_MESSAGE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TSMSGREMOVE), templates.PopupSMsgFlags & NOTIFY_REMOVE_MESSAGE);

		// Buttons
		SendDlgItemMessage(hwndDlg, IDC_BT_VARIABLES, BUTTONADDTOOLTIP, (WPARAM)LPGENW("Show available variables"), BATF_UNICODE);
		SendDlgItemMessage(hwndDlg, IDC_BT_VARIABLES, BM_SETIMAGE, IMAGE_ICON, (LPARAM)IcoLib_GetIconByHandle(iconList[ICO_VARIABLES].hIcolib));

		SendDlgItemMessage(hwndDlg, IDC_BT_RESET, BUTTONADDTOOLTIP, (WPARAM)LPGENW("Reset all templates to default"), BATF_UNICODE);
		SendDlgItemMessage(hwndDlg, IDC_BT_RESET, BM_SETIMAGE, IMAGE_ICON, (LPARAM)IcoLib_GetIconByHandle(iconList[ICO_RESET].hIcolib));

		// proto list
		{
			HWND hList = GetDlgItem(hwndDlg, IDC_PROTOCOLLIST);
			SendMessage(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
			LVCOLUMN lvCol = { 0 };
			lvCol.mask = LVCF_WIDTH | LVCF_TEXT;
			lvCol.pszText = TranslateT("Account");
			lvCol.cx = 118;
			ListView_InsertColumn(hList, 0, &lvCol);
			// fill the list
			LVITEM lvItem = { 0 };
			lvItem.mask = LVIF_TEXT | LVIF_PARAM;
			lvItem.iItem = 0;
			lvItem.iSubItem = 0;

			for (auto &pa : Accounts()) {
				if (IsSuitableProto(pa)) {
					UpdateListFlag = TRUE;
					lvItem.pszText = pa->tszAccountName;
					lvItem.lParam = (LPARAM)pa->szModuleName;
					PROTOTEMPLATE *prototemplate = (PROTOTEMPLATE *)mir_alloc(sizeof(PROTOTEMPLATE));
					prototemplate->ProtoName = pa->szModuleName;

					DBVARIANT dbVar = { 0 };
					char protoname[MAX_PATH] = { 0 };
					mir_snprintf(protoname, "%s_TPopupSMsgChanged", pa->szModuleName);
					if (g_plugin.getWString(protoname, &dbVar))
						wcsncpy(prototemplate->ProtoTemplateMsg, DEFAULT_POPUP_SMSGCHANGED, _countof(prototemplate->ProtoTemplateMsg));
					else {
						wcsncpy(prototemplate->ProtoTemplateMsg, dbVar.pwszVal, _countof(prototemplate->ProtoTemplateMsg));
						db_free(&dbVar);
					}

					mir_snprintf(protoname, "%s_TPopupSMsgRemoved", pa->szModuleName);
					if (g_plugin.getWString(protoname, &dbVar))
						wcsncpy(prototemplate->ProtoTemplateRemoved, DEFAULT_POPUP_SMSGREMOVED, _countof(prototemplate->ProtoTemplateRemoved));
					else {
						wcsncpy(prototemplate->ProtoTemplateRemoved, dbVar.pwszVal, _countof(prototemplate->ProtoTemplateRemoved));
						db_free(&dbVar);
					}

					ListView_InsertItem(hList, &lvItem);
					ProtoTemplates.insert(prototemplate, ProtoTemplates.getCount());

					char dbSetting[128];
					mir_snprintf(dbSetting, "%s_enabled", pa->szModuleName);
					ListView_SetCheckState(hList, lvItem.iItem, g_plugin.getByte(dbSetting, TRUE));
					lvItem.iItem++;
				}
			}

			if (lvItem.iItem) {
				ListView_SetItemState(hList, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
				PROTOTEMPLATE *prototemplate = ProtoTemplates[0];
				SetDlgItemText(hwndDlg, IDC_ED_TNEWSMSG, prototemplate->ProtoTemplateMsg);
				SetDlgItemText(hwndDlg, IDC_ED_TSMSGREMOVE, prototemplate->ProtoTemplateRemoved);
			}
			UpdateListFlag = FALSE;
		}
		return TRUE;

	case WM_COMMAND:
		switch (HIWORD(wParam)) {
		case BN_CLICKED:
			switch (LOWORD(wParam)) {
			case IDC_CHK_CUTSMSG:
				EnableWindow(GetDlgItem(hwndDlg, IDC_ED_SMSGLEN), IsDlgButtonChecked(hwndDlg, IDC_CHK_CUTSMSG));
				break;
			case IDC_CHK_NEWSMSG:
				EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TNEWSMSG), IsDlgButtonChecked(hwndDlg, IDC_CHK_NEWSMSG));
				break;
			case IDC_CHK_SMSGREMOVE:
				EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TSMSGREMOVE), IsDlgButtonChecked(hwndDlg, IDC_CHK_SMSGREMOVE));
				break;
			case IDC_BT_VARIABLES:
				MessageBox(hwndDlg, VARIABLES_SM_HELP_TEXT, TranslateT("Variables"), MB_OK | MB_ICONINFORMATION);
				break;
			case IDC_BT_RESET:
				if (MessageBox(hwndDlg,
					TranslateT("Do you want to reset all templates to default?"),
					TranslateT("Reset templates"),
					MB_ICONQUESTION | MB_YESNO) == IDYES)
					SendMessage(hwndDlg, WM_USER + 1, 0, 0);
				else
					return FALSE;
				break;
			}

			if (LOWORD(wParam) != IDC_BT_VARIABLES)
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);

			break;
		case EN_CHANGE:
			if ((HWND)lParam == GetFocus())
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		}
		return TRUE;

	case WM_NOTIFY:
		if (((NMHDR *)lParam)->idFrom == IDC_PROTOCOLLIST) {
			switch (((NMHDR *)lParam)->code) {
			case LVN_ITEMCHANGED:
				NMLISTVIEW *nmlv = (NMLISTVIEW *)lParam;
				if (nmlv->uNewState == 3 && !UpdateListFlag) {
					HWND hList = GetDlgItem(hwndDlg, IDC_PROTOCOLLIST);
					PROTOTEMPLATE *prototemplate;
					if (ListView_GetHotItem(hList) != ListView_GetSelectionMark(hList)) {
						prototemplate = ProtoTemplates[ListView_GetSelectionMark(hList)];
						GetDlgItemText(hwndDlg, IDC_ED_TNEWSMSG, prototemplate->ProtoTemplateMsg, MAX_PATH);
						GetDlgItemText(hwndDlg, IDC_ED_TSMSGREMOVE, prototemplate->ProtoTemplateRemoved, MAX_PATH);
						ProtoTemplates.remove(ListView_GetSelectionMark(hList));
						ProtoTemplates.insert(prototemplate, ListView_GetSelectionMark(hList));
					}

					LastItem = ListView_GetHotItem(hList);
					prototemplate = ProtoTemplates[LastItem];
					SetDlgItemText(hwndDlg, IDC_ED_TNEWSMSG, prototemplate->ProtoTemplateMsg);
					SetDlgItemText(hwndDlg, IDC_ED_TSMSGREMOVE, prototemplate->ProtoTemplateRemoved);
				}
				if ((nmlv->uNewState ^ nmlv->uOldState) & LVIS_STATEIMAGEMASK && !UpdateListFlag)
					SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			}
		}

		if (((LPNMHDR)lParam)->code == PSN_APPLY) {
			opt.PSMsgOnConnect = IsDlgButtonChecked(hwndDlg, IDC_ONCONNECT);
			opt.PSMsgTruncate = IsDlgButtonChecked(hwndDlg, IDC_CHK_CUTSMSG);
			opt.PSMsgLen = GetDlgItemInt(hwndDlg, IDC_ED_SMSGLEN, nullptr, FALSE);

			templates.PopupSMsgFlags = 0;
			templates.PopupSMsgFlags |= (IsDlgButtonChecked(hwndDlg, IDC_CHK_NEWSMSG) ? NOTIFY_NEW_MESSAGE : 0) |
				(IsDlgButtonChecked(hwndDlg, IDC_CHK_SMSGREMOVE) ? NOTIFY_REMOVE_MESSAGE : 0);

			// Templates
			PROTOTEMPLATE *prototemplate = ProtoTemplates[LastItem];
			GetDlgItemText(hwndDlg, IDC_ED_TNEWSMSG, prototemplate->ProtoTemplateMsg, MAX_PATH);
			GetDlgItemText(hwndDlg, IDC_ED_TSMSGREMOVE, prototemplate->ProtoTemplateRemoved, MAX_PATH);
			ProtoTemplates.remove(LastItem);
			ProtoTemplates.insert(prototemplate, LastItem);

			// Save options to db
			SaveOptions();
			SaveTemplates();
			HWND hList = GetDlgItem(hwndDlg, IDC_PROTOCOLLIST);
			LVITEM lvItem = { 0 };
			lvItem.mask = LVIF_PARAM;
			for (int i = 0; i < ListView_GetItemCount(hList); i++) {
				lvItem.iItem = i;
				lvItem.iSubItem = 0;
				ListView_GetItem(hList, &lvItem);

				char dbSetting[128];
				mir_snprintf(dbSetting, "%s_enabled", (char *)lvItem.lParam);
				g_plugin.setByte(dbSetting, (uint8_t)ListView_GetCheckState(hList, lvItem.iItem));
			}
		}

		return TRUE;
	case WM_USER + 1:
		SetDlgItemText(hwndDlg, IDC_ED_TNEWSMSG, DEFAULT_POPUP_SMSGCHANGED);
		SetDlgItemText(hwndDlg, IDC_ED_TSMSGREMOVE, DEFAULT_POPUP_SMSGREMOVED);

		CheckDlgButton(hwndDlg, IDC_CHK_NEWSMSG, BST_CHECKED);
		CheckDlgButton(hwndDlg, IDC_CHK_SMSGREMOVE, BST_CHECKED);

		EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TNEWSMSG), TRUE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TSMSGREMOVE), TRUE);

		return TRUE;

	case WM_DESTROY:
		ProtoTemplates.destroy();
		break;
	}

	return FALSE;
}

INT_PTR CALLBACK DlgProcXLogOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);

		CheckDlgButton(hwndDlg, IDC_XLOGTOFILE, opt.XLogToFile ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_XLOGTODB, opt.XLogToDB ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_XLOGTODB_WINOPEN, opt.XLogToDB_WinOpen ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_XLOGTODB_REMOVE, opt.XLogToDB_Remove ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_CHK_DISABLEMUSIC, opt.XLogDisableForMusic ? BST_CHECKED : BST_UNCHECKED);

		//Templates
		CheckDlgButton(hwndDlg, IDC_CHK_XSTATUSCHANGED, (templates.LogXFlags & NOTIFY_NEW_XSTATUS) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_CHK_XSTATUSREMOVED, (templates.LogXFlags & NOTIFY_REMOVE_XSTATUS) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_CHK_MSGCHANGED, (templates.LogXFlags & NOTIFY_NEW_MESSAGE) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_CHK_MSGREMOVED, (templates.LogXFlags & NOTIFY_REMOVE_MESSAGE) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_CHK_XSTATUSOPENING, (templates.LogXFlags & NOTIFY_OPENING_ML) ? BST_CHECKED : BST_UNCHECKED);

		SetDlgItemText(hwndDlg, IDC_ED_TXSTATUSCHANGED, templates.LogXstatusChanged);
		SetDlgItemText(hwndDlg, IDC_ED_TXSTATUSREMOVED, templates.LogXstatusRemoved);
		SetDlgItemText(hwndDlg, IDC_ED_TMSGCHANGED, templates.LogXMsgChanged);
		SetDlgItemText(hwndDlg, IDC_ED_TMSGREMOVED, templates.LogXMsgRemoved);
		SetDlgItemText(hwndDlg, IDC_ED_TXSTATUSOPENING, templates.LogXstatusOpening);

		// Buttons
		SendDlgItemMessage(hwndDlg, IDC_BT_VARIABLES, BUTTONADDTOOLTIP, (WPARAM)TranslateT("Show available variables"), BATF_UNICODE);
		SendDlgItemMessage(hwndDlg, IDC_BT_VARIABLES, BM_SETIMAGE, IMAGE_ICON, (LPARAM)IcoLib_GetIconByHandle(iconList[ICO_VARIABLES].hIcolib));

		SendDlgItemMessage(hwndDlg, IDC_BT_RESET, BUTTONADDTOOLTIP, (WPARAM)TranslateT("Reset all templates to default"), BATF_UNICODE);
		SendDlgItemMessage(hwndDlg, IDC_BT_RESET, BM_SETIMAGE, IMAGE_ICON, (LPARAM)IcoLib_GetIconByHandle(iconList[ICO_RESET].hIcolib));

		SendMessage(hwndDlg, WM_USER + 2, (WPARAM)opt.XLogToFile || opt.XLogToDB, 0);
		return TRUE;

	case WM_COMMAND:
		switch (HIWORD(wParam)) {
		case BN_CLICKED:
			switch (LOWORD(wParam)) {
			case IDC_XLOGTOFILE:
			case IDC_XLOGTODB:
				SendMessage(hwndDlg, WM_USER + 2, (WPARAM)IsDlgButtonChecked(hwndDlg, IDC_XLOGTOFILE) || IsDlgButtonChecked(hwndDlg, IDC_XLOGTODB), 0);
				break;
			case IDC_XLOGTODB_WINOPEN:
				EnableWindow(GetDlgItem(hwndDlg, IDC_XLOGTODB_REMOVE), IsDlgButtonChecked(hwndDlg, IDC_XLOGTODB_WINOPEN));
				break;
			case IDC_CHK_XSTATUSCHANGED:
				EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TXSTATUSCHANGED), IsDlgButtonChecked(hwndDlg, IDC_CHK_XSTATUSCHANGED));
				break;
			case IDC_CHK_XSTATUSREMOVED:
				EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TXSTATUSREMOVED), IsDlgButtonChecked(hwndDlg, IDC_CHK_XSTATUSREMOVED));
				break;
			case IDC_CHK_MSGCHANGED:
				EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TMSGCHANGED), IsDlgButtonChecked(hwndDlg, IDC_CHK_MSGCHANGED));
				break;
			case IDC_CHK_MSGREMOVED:
				EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TMSGREMOVED), IsDlgButtonChecked(hwndDlg, IDC_CHK_MSGREMOVED));
				break;
			case IDC_CHK_XSTATUSOPENING:
				EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TXSTATUSOPENING), IsDlgButtonChecked(hwndDlg, IDC_CHK_XSTATUSOPENING));
				break;
			case IDC_BT_VARIABLES:
				MessageBox(hwndDlg, VARIABLES_HELP_TEXT, TranslateT("Variables"), MB_OK | MB_ICONINFORMATION);
				break;
			case IDC_BT_RESET:
				if (MessageBox(hwndDlg,
					TranslateT("Do you want to reset all templates to default?"),
					TranslateT("Reset templates"),
					MB_ICONQUESTION | MB_YESNO) == IDYES)
					SendMessage(hwndDlg, WM_USER + 1, 0, 0);
				else
					return FALSE;
				break;
			}

			if (LOWORD(wParam) != IDC_BT_VARIABLES)
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);

			break;

		case EN_CHANGE:
			if ((HWND)lParam == GetFocus())
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		}
		return TRUE;

	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->code == PSN_APPLY) {
			opt.XLogToFile = IsDlgButtonChecked(hwndDlg, IDC_XLOGTOFILE);
			opt.XLogToDB = IsDlgButtonChecked(hwndDlg, IDC_XLOGTODB);
			opt.XLogToDB_WinOpen = IsDlgButtonChecked(hwndDlg, IDC_XLOGTODB_WINOPEN);
			opt.XLogToDB_Remove = IsDlgButtonChecked(hwndDlg, IDC_XLOGTODB_REMOVE);
			opt.XLogDisableForMusic = IsDlgButtonChecked(hwndDlg, IDC_CHK_DISABLEMUSIC);

			templates.LogXFlags = 0;
			templates.LogXFlags |= (IsDlgButtonChecked(hwndDlg, IDC_CHK_XSTATUSCHANGED) ? NOTIFY_NEW_XSTATUS : 0) |
				(IsDlgButtonChecked(hwndDlg, IDC_CHK_XSTATUSREMOVED) ? NOTIFY_REMOVE_XSTATUS : 0) |
				(IsDlgButtonChecked(hwndDlg, IDC_CHK_MSGCHANGED) ? NOTIFY_NEW_MESSAGE : 0) |
				(IsDlgButtonChecked(hwndDlg, IDC_CHK_MSGREMOVED) ? NOTIFY_REMOVE_MESSAGE : 0) |
				(IsDlgButtonChecked(hwndDlg, IDC_CHK_XSTATUSOPENING) ? NOTIFY_OPENING_ML : 0);

			// Templates
			GetDlgItemText(hwndDlg, IDC_ED_TXSTATUSCHANGED, templates.LogXstatusChanged, _countof(templates.LogXstatusChanged));
			GetDlgItemText(hwndDlg, IDC_ED_TXSTATUSREMOVED, templates.LogXstatusRemoved, _countof(templates.LogXstatusRemoved));
			GetDlgItemText(hwndDlg, IDC_ED_TMSGCHANGED, templates.LogXMsgChanged, _countof(templates.LogXMsgChanged));
			GetDlgItemText(hwndDlg, IDC_ED_TMSGREMOVED, templates.LogXMsgRemoved, _countof(templates.LogXMsgRemoved));
			GetDlgItemText(hwndDlg, IDC_ED_TXSTATUSOPENING, templates.LogXstatusOpening, _countof(templates.LogXstatusOpening));

			SaveOptions();
			SaveTemplates();
		}
		return TRUE;

	case WM_USER + 1:
		CheckDlgButton(hwndDlg, IDC_CHK_XSTATUSCHANGED, BST_CHECKED);
		CheckDlgButton(hwndDlg, IDC_CHK_XSTATUSREMOVED, BST_CHECKED);
		CheckDlgButton(hwndDlg, IDC_CHK_MSGCHANGED, BST_CHECKED);
		CheckDlgButton(hwndDlg, IDC_CHK_MSGREMOVED, BST_CHECKED);
		CheckDlgButton(hwndDlg, IDC_CHK_XSTATUSOPENING, BST_CHECKED);

		SetDlgItemText(hwndDlg, IDC_ED_TXSTATUSCHANGED, DEFAULT_LOG_CHANGED);
		SetDlgItemText(hwndDlg, IDC_ED_TXSTATUSREMOVED, DEFAULT_LOG_REMOVED);
		SetDlgItemText(hwndDlg, IDC_ED_TMSGCHANGED, DEFAULT_LOG_MSGCHANGED);
		SetDlgItemText(hwndDlg, IDC_ED_TMSGREMOVED, DEFAULT_LOG_MSGREMOVED);
		SetDlgItemText(hwndDlg, IDC_ED_TXSTATUSOPENING, DEFAULT_LOG_OPENING);

		EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TXSTATUSCHANGED), TRUE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TXSTATUSREMOVED), TRUE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TMSGCHANGED), TRUE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TMSGREMOVED), TRUE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TXSTATUSOPENING), TRUE);
		return TRUE;

	case WM_USER + 2:
		EnableWindow(GetDlgItem(hwndDlg, IDC_XLOGTODB_WINOPEN), IsDlgButtonChecked(hwndDlg, IDC_XLOGTODB));
		EnableWindow(GetDlgItem(hwndDlg, IDC_XLOGTODB_REMOVE), IsDlgButtonChecked(hwndDlg, IDC_XLOGTODB) && IsDlgButtonChecked(hwndDlg, IDC_XLOGTODB_WINOPEN));

		EnableWindow(GetDlgItem(hwndDlg, IDC_CHK_XSTATUSCHANGED), wParam);
		EnableWindow(GetDlgItem(hwndDlg, IDC_CHK_XSTATUSREMOVED), wParam);
		EnableWindow(GetDlgItem(hwndDlg, IDC_CHK_MSGCHANGED), wParam);
		EnableWindow(GetDlgItem(hwndDlg, IDC_CHK_MSGREMOVED), wParam);
		EnableWindow(GetDlgItem(hwndDlg, IDC_CHK_XSTATUSOPENING), wParam);

		EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TXSTATUSCHANGED), wParam && IsDlgButtonChecked(hwndDlg, IDC_CHK_XSTATUSCHANGED));
		EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TXSTATUSREMOVED), wParam && IsDlgButtonChecked(hwndDlg, IDC_CHK_XSTATUSREMOVED));
		EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TMSGCHANGED), wParam && IsDlgButtonChecked(hwndDlg, IDC_CHK_MSGCHANGED));
		EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TMSGREMOVED), wParam && IsDlgButtonChecked(hwndDlg, IDC_CHK_MSGREMOVED));
		EnableWindow(GetDlgItem(hwndDlg, IDC_ED_TXSTATUSOPENING), wParam && IsDlgButtonChecked(hwndDlg, IDC_CHK_XSTATUSOPENING));

		EnableWindow(GetDlgItem(hwndDlg, IDC_BT_VARIABLES), wParam);
		EnableWindow(GetDlgItem(hwndDlg, IDC_BT_RESET), wParam);
		return TRUE;
	}

	return FALSE;
}

INT_PTR CALLBACK DlgProcLogOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);

		CheckDlgButton(hwndDlg, IDC_LOGTOFILE, opt.LogToFile ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_LOGTODB, opt.LogToDB ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_LOGTODB_WINOPEN, opt.LogToDB_WinOpen ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_LOGTODB_REMOVE, opt.LogToDB_Remove ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_LOGPREVIOUS, opt.LogPrevious ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_SMSGLOGTOFILE, opt.SMsgLogToFile ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_SMSGLOGTODB, opt.SMsgLogToDB ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_SMSGLOGTODB_WINOPEN, opt.SMsgLogToDB_WinOpen ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_SMSGLOGTODB_REMOVE, opt.SMsgLogToDB_Remove ? BST_CHECKED : BST_UNCHECKED);

		//Templates
		CheckDlgButton(hwndDlg, IDC_LOG_SMSGCHANGED, (templates.LogSMsgFlags & NOTIFY_NEW_MESSAGE) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_LOG_SMSGREMOVED, (templates.LogSMsgFlags & NOTIFY_REMOVE_MESSAGE) ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_LOG_SMSGOPENING, (templates.LogSMsgFlags & NOTIFY_OPENING_ML) ? BST_CHECKED : BST_UNCHECKED);

		SetDlgItemText(hwndDlg, IDC_LOG_TSMSGCHANGED, templates.LogSMsgChanged);
		SetDlgItemText(hwndDlg, IDC_LOG_TSMSGREMOVED, templates.LogSMsgRemoved);
		SetDlgItemText(hwndDlg, IDC_LOG_TSMSGOPENING, templates.LogSMsgOpening);

		// Buttons
		SendDlgItemMessage(hwndDlg, IDC_BT_VARIABLES, BUTTONADDTOOLTIP, (WPARAM)TranslateT("Show available variables"), BATF_UNICODE);
		SendDlgItemMessage(hwndDlg, IDC_BT_VARIABLES, BM_SETIMAGE, IMAGE_ICON, (LPARAM)IcoLib_GetIconByHandle(iconList[ICO_VARIABLES].hIcolib));

		SendDlgItemMessage(hwndDlg, IDC_BT_RESET, BUTTONADDTOOLTIP, (WPARAM)TranslateT("Reset all templates to default"), BATF_UNICODE);
		SendDlgItemMessage(hwndDlg, IDC_BT_RESET, BM_SETIMAGE, IMAGE_ICON, (LPARAM)IcoLib_GetIconByHandle(iconList[ICO_RESET].hIcolib));

		SendMessage(hwndDlg, WM_USER + 2, (WPARAM)opt.LogToFile || opt.LogToDB, 0);
		SendMessage(hwndDlg, WM_USER + 3, (WPARAM)opt.SMsgLogToFile || opt.SMsgLogToDB, 0);
		return TRUE;

	case WM_COMMAND:
		switch (HIWORD(wParam)) {
		case BN_CLICKED:
			switch (LOWORD(wParam)) {
			case IDC_LOGTOFILE:
			case IDC_LOGTODB:
				SendMessage(hwndDlg, WM_USER + 2, (WPARAM)IsDlgButtonChecked(hwndDlg, IDC_LOGTOFILE) || IsDlgButtonChecked(hwndDlg, IDC_LOGTODB), 0);
				break;
			case IDC_SMSGLOGTOFILE:
			case IDC_SMSGLOGTODB:
				SendMessage(hwndDlg, WM_USER + 3, (WPARAM)IsDlgButtonChecked(hwndDlg, IDC_SMSGLOGTOFILE) || IsDlgButtonChecked(hwndDlg, IDC_SMSGLOGTODB), 0);
				break;
			case IDC_LOGTODB_WINOPEN:
				EnableWindow(GetDlgItem(hwndDlg, IDC_LOGTODB_REMOVE), IsDlgButtonChecked(hwndDlg, IDC_LOGTODB_WINOPEN));
				break;
			case IDC_SMSGLOGTODB_WINOPEN:
				EnableWindow(GetDlgItem(hwndDlg, IDC_SMSGLOGTODB_REMOVE), IsDlgButtonChecked(hwndDlg, IDC_SMSGLOGTODB_WINOPEN));
				break;
			case IDC_LOG_SMSGCHANGED:
				EnableWindow(GetDlgItem(hwndDlg, IDC_LOG_TSMSGCHANGED), IsDlgButtonChecked(hwndDlg, IDC_LOG_SMSGCHANGED));
				break;
			case IDC_LOG_SMSGREMOVED:
				EnableWindow(GetDlgItem(hwndDlg, IDC_LOG_TSMSGREMOVED), IsDlgButtonChecked(hwndDlg, IDC_LOG_SMSGREMOVED));
				break;
			case IDC_LOG_SMSGOPENING:
				EnableWindow(GetDlgItem(hwndDlg, IDC_LOG_TSMSGOPENING), IsDlgButtonChecked(hwndDlg, IDC_LOG_SMSGOPENING));
				break;
			case IDC_BT_VARIABLES:
				MessageBox(hwndDlg, VARIABLES_SM_HELP_TEXT, TranslateT("Variables"), MB_OK | MB_ICONINFORMATION);
				break;
			case IDC_BT_RESET:
				if (MessageBox(hwndDlg,
					TranslateT("Do you want to reset all templates to default?"),
					TranslateT("Reset templates"),
					MB_ICONQUESTION | MB_YESNO) == IDYES)
					SendMessage(hwndDlg, WM_USER + 1, 0, 0);
				else
					return FALSE;
				break;
			}

			if (LOWORD(wParam) != IDC_BT_VARIABLES)
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);

			break;

		case EN_CHANGE:
			if ((HWND)lParam == GetFocus())
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			break;
		}
		return TRUE;

	case WM_NOTIFY:
		if (((LPNMHDR)lParam)->code == PSN_APPLY) {
			opt.LogToFile = IsDlgButtonChecked(hwndDlg, IDC_LOGTOFILE);
			opt.LogToDB = IsDlgButtonChecked(hwndDlg, IDC_LOGTODB);
			opt.LogToDB_WinOpen = IsDlgButtonChecked(hwndDlg, IDC_LOGTODB_WINOPEN);
			opt.LogToDB_Remove = IsDlgButtonChecked(hwndDlg, IDC_LOGTODB_REMOVE);
			opt.LogPrevious = IsDlgButtonChecked(hwndDlg, IDC_LOGPREVIOUS);
			opt.SMsgLogToFile = IsDlgButtonChecked(hwndDlg, IDC_SMSGLOGTOFILE);
			opt.SMsgLogToDB = IsDlgButtonChecked(hwndDlg, IDC_SMSGLOGTODB);
			opt.SMsgLogToDB_WinOpen = IsDlgButtonChecked(hwndDlg, IDC_SMSGLOGTODB_WINOPEN);
			opt.SMsgLogToDB_Remove = IsDlgButtonChecked(hwndDlg, IDC_SMSGLOGTODB_REMOVE);

			templates.LogSMsgFlags = 0;
			templates.LogSMsgFlags |= (IsDlgButtonChecked(hwndDlg, IDC_LOG_SMSGCHANGED) ? NOTIFY_NEW_MESSAGE : 0) |
				(IsDlgButtonChecked(hwndDlg, IDC_LOG_SMSGREMOVED) ? NOTIFY_REMOVE_MESSAGE : 0) |
				(IsDlgButtonChecked(hwndDlg, IDC_LOG_SMSGOPENING) ? NOTIFY_OPENING_ML : 0);

			// Templates
			GetDlgItemText(hwndDlg, IDC_LOG_TSMSGCHANGED, templates.LogSMsgChanged, _countof(templates.LogSMsgChanged));
			GetDlgItemText(hwndDlg, IDC_LOG_TSMSGREMOVED, templates.LogSMsgRemoved, _countof(templates.LogSMsgRemoved));
			GetDlgItemText(hwndDlg, IDC_LOG_TSMSGOPENING, templates.LogSMsgOpening, _countof(templates.LogSMsgOpening));

			SaveOptions();
			SaveTemplates();
		}
		return TRUE;

	case WM_USER + 1:
		CheckDlgButton(hwndDlg, IDC_LOG_SMSGCHANGED, BST_CHECKED);
		CheckDlgButton(hwndDlg, IDC_LOG_SMSGREMOVED, BST_CHECKED);
		CheckDlgButton(hwndDlg, IDC_LOG_SMSGOPENING, BST_CHECKED);

		SetDlgItemText(hwndDlg, IDC_LOG_TSMSGCHANGED, DEFAULT_LOG_SMSGCHANGED);
		SetDlgItemText(hwndDlg, IDC_LOG_TSMSGREMOVED, DEFAULT_LOG_SMSGREMOVED);
		SetDlgItemText(hwndDlg, IDC_LOG_TSMSGOPENING, DEFAULT_LOG_SMSGOPENING);

		EnableWindow(GetDlgItem(hwndDlg, IDC_LOG_TSMSGCHANGED), TRUE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_LOG_TSMSGREMOVED), TRUE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_LOG_TSMSGOPENING), TRUE);

		return TRUE;
	case WM_USER + 2:
		EnableWindow(GetDlgItem(hwndDlg, IDC_LOGTODB_WINOPEN), IsDlgButtonChecked(hwndDlg, IDC_LOGTODB));
		EnableWindow(GetDlgItem(hwndDlg, IDC_LOGTODB_REMOVE), IsDlgButtonChecked(hwndDlg, IDC_LOGTODB) && IsDlgButtonChecked(hwndDlg, IDC_LOGTODB_WINOPEN));
		EnableWindow(GetDlgItem(hwndDlg, IDC_LOGPREVIOUS), wParam);

		return TRUE;
	case WM_USER + 3:
		EnableWindow(GetDlgItem(hwndDlg, IDC_SMSGLOGTODB_WINOPEN), IsDlgButtonChecked(hwndDlg, IDC_SMSGLOGTODB));
		EnableWindow(GetDlgItem(hwndDlg, IDC_SMSGLOGTODB_REMOVE), IsDlgButtonChecked(hwndDlg, IDC_SMSGLOGTODB) && IsDlgButtonChecked(hwndDlg, IDC_SMSGLOGTODB_WINOPEN));

		EnableWindow(GetDlgItem(hwndDlg, IDC_LOG_SMSGCHANGED), wParam);
		EnableWindow(GetDlgItem(hwndDlg, IDC_LOG_SMSGREMOVED), wParam);
		EnableWindow(GetDlgItem(hwndDlg, IDC_LOG_SMSGOPENING), wParam);

		EnableWindow(GetDlgItem(hwndDlg, IDC_LOG_TSMSGCHANGED), wParam && IsDlgButtonChecked(hwndDlg, IDC_LOG_SMSGCHANGED));
		EnableWindow(GetDlgItem(hwndDlg, IDC_LOG_TSMSGREMOVED), wParam && IsDlgButtonChecked(hwndDlg, IDC_LOG_SMSGREMOVED));
		EnableWindow(GetDlgItem(hwndDlg, IDC_LOG_TSMSGOPENING), wParam && IsDlgButtonChecked(hwndDlg, IDC_LOG_SMSGOPENING));

		EnableWindow(GetDlgItem(hwndDlg, IDC_BT_VARIABLES), wParam);
		EnableWindow(GetDlgItem(hwndDlg, IDC_BT_RESET), wParam);

		return TRUE;
	}

	return FALSE;
}

int OptionsInitialize(WPARAM wParam, LPARAM)
{
	OPTIONSDIALOGPAGE odp = {};
	odp.position = -100000000;
	odp.flags = ODPF_BOLDGROUPS;
	odp.szTitle.a = LPGEN("Status Notify");
	odp.szGroup.a = LPGEN("Status");
	odp.szTab.a = LPGEN("General");
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_GENERAL);
	odp.pfnDlgProc = DlgProcGeneralOpts;
	g_plugin.addOptions(wParam, &odp);

	odp.szTab.a = LPGEN("Status logging");
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_LOG);
	odp.pfnDlgProc = DlgProcLogOpts;
	g_plugin.addOptions(wParam, &odp);

	odp.szTab.a = LPGEN("Extra status logging");
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_XLOG);
	odp.pfnDlgProc = DlgProcXLogOpts;
	g_plugin.addOptions(wParam, &odp);

	odp.szTab.a = LPGEN("Filtering");
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_FILTERS);
	odp.pfnDlgProc = DlgProcFiltering;
	g_plugin.addOptions(wParam, &odp);

	// Popups
	odp.szTitle.a = LPGEN("Status Notify");
	odp.szGroup.a = LPGEN("Popups");
	odp.szTab.a = LPGEN("General");
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_POPUP);
	odp.pfnDlgProc = DlgProcPopupOpts;
	g_plugin.addOptions(wParam, &odp);

	odp.szTab.a = LPGEN("Extra status");
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_XPOPUP);
	odp.pfnDlgProc = DlgProcXPopupOpts;
	g_plugin.addOptions(wParam, &odp);

	odp.szTab.a = LPGEN("Status message");
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_SMPOPUP);
	odp.pfnDlgProc = DlgProcSMPopupOpts;
	g_plugin.addOptions(wParam, &odp);
	return 0;
}
