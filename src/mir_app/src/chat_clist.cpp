/*
Chat module plugin for Miranda IM

Copyright (C) 2003 Jörgen Persson

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "stdafx.h"

#include "chat.h"

static MCONTACT FindRoom(const char *pszModule, const wchar_t *pszRoom)
{
	for (auto &hContact : Contacts(pszModule)) {
		if (!Contact::IsGroupChat(hContact, pszModule))
			continue;

		ptrW roomid(Contact::GetInfo(CNF_UNIQUEID, hContact, pszModule));
		if (roomid != nullptr && !mir_wstrcmpi(roomid, pszRoom))
			return hContact;
	}

	return 0;
}

MCONTACT AddRoom(const char *pszModule, const wchar_t *pszRoom, const wchar_t *pszDisplayName, int iType)
{
	ptrW pwszGroup(Chat_GetGroup());
	if (mir_wstrlen(pwszGroup)) {
		MGROUP hGroup = Clist_GroupExists(pwszGroup);
		if (hGroup == 0) {
			hGroup = Clist_GroupCreate(0, pwszGroup);
			if (hGroup)
				Clist_GroupSetExpanded(hGroup, 1);
		}
	}

	MCONTACT hContact = FindRoom(pszModule, pszRoom);
	if (hContact) {
		// contact exists, let's assign the standard group name if it's missing
		if (mir_wstrlen(pwszGroup)) {
			ptrW pwszOldGroup(Clist_GetGroup(hContact));
			if (!mir_wstrlen(pwszOldGroup))
				Clist_SetGroup(hContact, pwszGroup);
		}

		db_set_w(hContact, pszModule, "Status", ID_STATUS_OFFLINE);
		db_set_ws(hContact, pszModule, "Nick", pszDisplayName);
		return hContact;
	}

	// here we create a new one since no one is to be found
	if ((hContact = db_add_contact()) == 0)
		return 0;

	Proto_AddToContact(hContact, pszModule);
	Clist_SetGroup(hContact, pwszGroup);

	if (auto *pa = Proto_GetAccount(pszModule)) {
		if (MBaseProto *pd = g_arProtos.find((MBaseProto *)&pa->szProtoName)) {
			if (pd->iUniqueIdType == DBVT_DWORD)
				db_set_dw(hContact, pszModule, pd->szUniqueId, _wtoi(pszRoom));
			else
				db_set_ws(hContact, pszModule, pd->szUniqueId, pszRoom);
		}
	}

	db_set_ws(hContact, pszModule, "Nick", pszDisplayName);
	db_set_b(hContact, pszModule, "ChatRoom", (uint8_t)iType);
	db_set_w(hContact, pszModule, "Status", ID_STATUS_OFFLINE);
	return hContact;
}

BOOL SetOffline(MCONTACT hContact, BOOL)
{
	if (hContact) {
		char *szProto = Proto_GetBaseAccountName(hContact);
		db_set_w(hContact, szProto, "ApparentMode", 0);
		db_set_w(hContact, szProto, "Status", ID_STATUS_OFFLINE);
		return TRUE;
	}

	return FALSE;
}

BOOL SetAllOffline(BOOL, const char *pszModule)
{
	for (auto &hContact : Contacts(pszModule)) {
		char *szProto = Proto_GetBaseAccountName(hContact);
		if (!MM_FindModule(szProto))
			continue;
		
		if (Contact::IsGroupChat(hContact, szProto)) {
			db_set_w(hContact, szProto, "ApparentMode", 0);
			db_set_w(hContact, szProto, "Status", ID_STATUS_OFFLINE);
		}
	}

	return TRUE;
}

int RoomDoubleclicked(WPARAM hContact, LPARAM)
{
	if (!hContact)
		return 0;

	char *szProto = Proto_GetBaseAccountName(hContact);
	if (MM_FindModule(szProto) == nullptr)
		return 0;
	if (!Contact::IsGroupChat(hContact, szProto))
		return 0;

	ptrW roomid(Contact::GetInfo(CNF_UNIQUEID, hContact, szProto));
	if (roomid == nullptr)
		return 0;

	SESSION_INFO *si = Chat_Find(roomid, szProto);
	if (si) {
		if (si->pDlg != nullptr && !g_clistApi.pfnGetEvent(hContact, 0) && IsWindowVisible(si->pDlg->GetHwnd()) && !IsIconic(si->pDlg->GetHwnd())) {
			si->pDlg->CloseTab();
			return 1;
		}
		g_chatApi.ShowRoom(si);
	}
	return 1;
}
