////////////////////////////////////////////////////////////////////////////////
// Gadu-Gadu Plugin for Miranda IM
//
// Copyright (c) 2009-2012 Bartosz Białek
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
////////////////////////////////////////////////////////////////////////////////

#include "gg.h"
#include <io.h>
#include <fcntl.h>
#include "protocol.h"

//////////////////////////////////////////////////////////
// Avatars support
//
void GaduProto::getAvatarFilename(MCONTACT hContact, wchar_t *pszDest, int cbLen)
{
	CMStringW wszPath(GetAvatarPath());

	const wchar_t *avatartype = ProtoGetAvatarExtension(getByte(hContact, GG_KEY_AVATARTYPE, GG_KEYDEF_AVATARTYPE));

	if (hContact != NULL) {
		DBVARIANT dbv;
		if (!getString(hContact, GG_KEY_AVATARHASH, &dbv)) {
			wszPath.AppendFormat(L"\\%S%s", dbv.pszVal, avatartype);
			db_free(&dbv);
		}
	}
	else wszPath.AppendFormat(L"\\%S avatar%s", m_szModuleName, avatartype);

	wcsncpy_s(pszDest, cbLen, wszPath, _TRUNCATE);
}

bool GaduProto::getAvatarFileInfo(uin_t uin, char **avatarurl, char **avatarts)
{
	*avatarurl = *avatarts = nullptr;

	char szUrl[128];
	mir_snprintf(szUrl, "http://api.gadu-gadu.pl/avatars/%d/0.xml", uin);

	NETLIBHTTPREQUEST req = { sizeof(req) };
	req.requestType = REQUEST_GET;
	req.szUrl = szUrl;
	req.flags = NLHRF_NODUMP | NLHRF_HTTP11 | NLHRF_REDIRECT;

	NLHR_PTR resp(Netlib_HttpTransaction(m_hNetlibUser, &req));
	if (resp == nullptr) {
		debugLogA("getAvatarFileInfo(): No response from HTTP request");
		return false;
	}

	if (resp->resultCode != 200 || !resp->dataLength || !resp->pData) {
		debugLogA("getAvatarFileInfo(): Invalid response code from HTTP request");
		return false;
	}

	if ((strncmp(resp->pData, "<result>", 8) == 0) || (strncmp(resp->pData, "<?xml", 5) == 0)) {
		// if this url returned xml data (before and after 11.2013 gg convention)
		TiXmlDocument doc;
		if (doc.Parse(resp->pData) == 0) {
			auto *node = TiXmlConst(doc.FirstChildElement("result"))["users"]["user"]["avatars"]["avatar"].ToElement();
			const char *blank = (node != nullptr) ? node->Attribute("blank") : nullptr;
			if (mir_strcmp(blank, "1")) {
				auto *p = node->FirstChildElement("timestamp");
				if (p)
					*avatarts = mir_strdup(p->GetText());

				p = node->FirstChildElement("bigavatar"); // new gg convention
				if (p)
					*avatarurl = mir_strdup(p->GetText());
			}
		}
	}
	else if (strncmp(resp->pData, "{\"result\":", 10) == 0) {
		// if this url returns json data (11.2013 gg convention)
		JSONNode root = JSONNode::parse(resp->pData);
		if (root) {
			const JSONNode &respJSONavatars = root["result"].at("users").at("user").at("avatars");
			if (respJSONavatars) {
				const JSONNode &respJSONavatar = *respJSONavatars.begin();
				std::string respJSON_blank = respJSONavatar["_blank"].as_string();
				std::string respJSONoriginBigAvatar = respJSONavatar["originBigAvatar"].as_string();
				std::string respJSONtimestamp = respJSONavatar["timestamp"].as_string();
				if (respJSON_blank == "1" && !respJSONoriginBigAvatar.empty() && !respJSONtimestamp.empty()) {
					*avatarurl = mir_strdup(respJSONoriginBigAvatar.c_str());
					*avatarts = mir_strdup(respJSONtimestamp.c_str());
				}
			}
		}
	}
	else {
		debugLogA("getAvatarFileInfo(): Invalid response code from HTTP request, unknown format");
		return false;
	}

	return true;
}

char *gg_avatarhash(char *param)
{
	char *result;

	if (param == nullptr || (result = (char *)mir_alloc(MIR_SHA1_HASH_SIZE * 2 + 1)) == nullptr)
		return nullptr;

	uint8_t digest[MIR_SHA1_HASH_SIZE];
	mir_sha1_hash((uint8_t*)param, (int)mir_strlen(param), digest);
	return bin2hex(digest, sizeof(digest), result);
}

void GaduProto::requestAvatarTransfer(MCONTACT hContact, char *szAvatarURL)
{
	if (pth_avatar.dwThreadId == NULL) {
		debugLogA("requestAvatarTransfer(): Can not list_add element to avatar_transfers list. No pth_avatar.dwThreadId");
		return;
	}

	gg_EnterCriticalSection(&avatar_mutex, "requestAvatarTransfer", 1, "avatar_mutex", 1);
	if (avatar_transfers.getIndex((GGGETAVATARDATA*)&hContact) == -1) {
		GGGETAVATARDATA *data = (GGGETAVATARDATA*)mir_alloc(sizeof(GGGETAVATARDATA) + mir_strlen(szAvatarURL) + 1);
		data->hContact = hContact;
		data->szAvatarURL = mir_strcpy((char*)(data + 1), szAvatarURL);
		avatar_transfers.insert(data);
	}
	gg_LeaveCriticalSection(&avatar_mutex, "requestAvatarTransfer", 1, 1, "avatar_mutex", 1);
}

void GaduProto::requestAvatarInfo(MCONTACT hContact, int iWaitFor)
{
	if (pth_avatar.dwThreadId == NULL) {
		debugLogA("requestAvatarInfo(): Can not list_add element to avatar_requests list. No pth_avatar.dwThreadId");
		return;
	}

	if (!getByte(GG_KEY_ENABLEAVATARS, GG_KEYDEF_ENABLEAVATARS))
		return;

	GGREQUESTAVATARDATA *data = nullptr;
	gg_EnterCriticalSection(&avatar_mutex, "requestAvatarInfo", 2, "avatar_mutex", 1);
	if (avatar_requests.getIndex((GGREQUESTAVATARDATA*)&hContact) == -1) {
		data = (GGREQUESTAVATARDATA*)mir_alloc(sizeof(GGREQUESTAVATARDATA));
		data->hContact = hContact;
		data->iWaitFor = iWaitFor;
		avatar_requests.insert(data);
	}
	gg_LeaveCriticalSection(&avatar_mutex, "requestAvatarInfo", 2, 1, "avatar_mutex", 1);

	if (data != nullptr)
		setByte(hContact, GG_KEY_AVATARREQUESTED, 1);
}

void __cdecl GaduProto::avatarrequestthread(void*)
{
	debugLogA("avatarrequestthread() started. Avatar Request Thread Starting");
	while (pth_avatar.dwThreadId)
	{
		gg_EnterCriticalSection(&avatar_mutex, "avatarrequestthread", 3, "avatar_mutex", 1);
		if (avatar_requests.getCount()) {
			GGREQUESTAVATARDATA *data = avatar_requests[0];
			int iWaitFor = data->iWaitFor;
			MCONTACT hContact = data->hContact;
			avatar_requests.remove(0);
			mir_free(data);
			gg_LeaveCriticalSection(&avatar_mutex, "avatarrequestthread", 3, 1, "avatar_mutex", 1);

			uin_t uin = (uin_t)getDword(hContact, GG_KEY_UIN, 0);
			debugLogA("avatarrequestthread() new avatar_requests item for uin=%d.", uin);

			char *AvatarURL, *AvatarTs;
			if (!getAvatarFileInfo(uin, &AvatarURL, &AvatarTs)) {
				if (iWaitFor)
					ProtoBroadcastAck(hContact, ACKTYPE_AVATAR, ACKRESULT_FAILED, nullptr);
			}
			else {
				if (AvatarURL == nullptr && AvatarTs == nullptr) {
					delSetting(hContact, GG_KEY_AVATARURL);
					delSetting(hContact, GG_KEY_AVATARTS);
					if (iWaitFor)
						ProtoBroadcastAck(hContact, ACKTYPE_AVATAR, ACKRESULT_SUCCESS, nullptr);
				}
				else {
					setString(hContact, GG_KEY_AVATARURL, AvatarURL);
					setString(hContact, GG_KEY_AVATARTS, AvatarTs);
					mir_free(AvatarURL); mir_free(AvatarTs);

					if (iWaitFor) {
						PROTO_AVATAR_INFORMATION ai = { 0 };
						ai.hContact = hContact;
						INT_PTR res = getavatarinfo((WPARAM)GAIF_FORCE, (LPARAM)&ai);
						if (res == GAIR_NOAVATAR)
							ProtoBroadcastAck(hContact, ACKTYPE_AVATAR, ACKRESULT_SUCCESS, nullptr);
						else if (res == GAIR_SUCCESS)
							ProtoBroadcastAck(hContact, ACKTYPE_AVATAR, ACKRESULT_SUCCESS, &ai);
					}
					else ProtoBroadcastAck(hContact, ACKTYPE_AVATAR, ACKRESULT_STATUS, nullptr);
					delSetting(hContact, GG_KEY_AVATARREQUESTED);
				}
			}
		}
		else gg_LeaveCriticalSection(&avatar_mutex, "avatarrequestthread", 3, 2, "avatar_mutex", 1);

		gg_EnterCriticalSection(&avatar_mutex, "avatarrequestthread", 4, "avatar_mutex", 1);
		if (avatar_transfers.getCount()) {
			GGGETAVATARDATA *data = avatar_transfers[0];
			avatar_transfers.remove(0);
			gg_LeaveCriticalSection(&avatar_mutex, "avatarrequestthread", 4, 1, "avatar_mutex", 1);
			debugLogA("avatarrequestthread() new avatar_transfers item for url=%s.", data->szAvatarURL);

			int result = 0;

			PROTO_AVATAR_INFORMATION ai = { 0 };
			ai.hContact = data->hContact;
			ai.format = getByte(ai.hContact, GG_KEY_AVATARTYPE, GG_KEYDEF_AVATARTYPE);

			NETLIBHTTPREQUEST req = { sizeof(req) };
			req.requestType = REQUEST_GET;
			req.szUrl = data->szAvatarURL;
			req.flags = NLHRF_NODUMP | NLHRF_HTTP11 | NLHRF_REDIRECT;

			NLHR_PTR resp(Netlib_HttpTransaction(m_hNetlibUser, &req));
			if (resp) {
				if (resp->resultCode == 200 && resp->dataLength > 0 && resp->pData) {
					int file_fd;

					int avatarType = PA_FORMAT_UNKNOWN;
					if (strncmp(resp->pData, "\xFF\xD8", 2) == 0) avatarType = PA_FORMAT_JPEG;
					if (strncmp(resp->pData, "\x47\x49\x46\x38", 4) == 0) avatarType = PA_FORMAT_GIF;
					if (strncmp(resp->pData, "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A", 8) == 0) avatarType = PA_FORMAT_PNG;
					setByte(data->hContact, GG_KEY_AVATARTYPE, (uint8_t)avatarType);

					getAvatarFilename(ai.hContact, ai.filename, _countof(ai.filename));
					file_fd = _wopen(ai.filename, _O_WRONLY | _O_TRUNC | _O_BINARY | _O_CREAT, _S_IREAD | _S_IWRITE);
					if (file_fd != -1) {
						_write(file_fd, resp->pData, resp->dataLength);
						_close(file_fd);
						result = 1;
						debugLogW(L"avatarrequestthread() new avatar_transfers item. Saved data to file=%s.", ai.filename);
					}
					else {
						debugLogW(L"avatarrequestthread(): _wopen file %s error. errno=%d: %s", ai.filename, errno, ws_strerror(errno));
						wchar_t error[512];
						mir_snwprintf(error, TranslateT("Cannot create avatar file. ERROR: %d: %s\n%s"), errno, ws_strerror(errno), ai.filename);
						showpopup(m_tszUserName, error, GG_POPUP_ERROR);
					}
				}
				else debugLogA("avatarrequestthread(): Invalid response code from HTTP request");
			}
			else debugLogA("avatarrequestthread(): No response from HTTP request");

			ProtoBroadcastAck(ai.hContact, ACKTYPE_AVATAR, result ? ACKRESULT_SUCCESS : ACKRESULT_FAILED, &ai);

			if (!ai.hContact)
				ReportSelfAvatarChanged();

			mir_free(data);
		}
		else gg_LeaveCriticalSection(&avatar_mutex, "avatarrequestthread", 4, 2, "avatar_mutex", 1);

		gg_sleep(100, FALSE, "avatarrequestthread", 101, 1);
	}

	for (auto &it : avatar_requests)
		mir_free(it);
	avatar_requests.destroy();

	for (auto &it : avatar_transfers)
		mir_free(it);
	avatar_transfers.destroy();

	debugLogA("avatarrequestthread(): end. Avatar Request Thread Ending");
}

void GaduProto::initavatarrequestthread()
{
	DWORD exitCode = 0;
	GetExitCodeThread(pth_avatar.hThread, &exitCode);

	if (exitCode != STILL_ACTIVE) {
		avatar_requests.destroy();
		avatar_transfers.destroy();
#ifdef DEBUGMODE
		debugLogA("initavatarrequestthread(): ForkThreadEx 1 GaduProto::avatarrequestthread");
#endif
		pth_avatar.hThread = ForkThreadEx(&GaduProto::avatarrequestthread, nullptr, &pth_avatar.dwThreadId);
	}
}

void __cdecl GaduProto::getOwnAvatarThread(void*)
{
	debugLogA("getOwnAvatarThread() started");

	char *AvatarURL, *AvatarTs;
	if (getAvatarFileInfo(getDword(GG_KEY_UIN, 0), &AvatarURL, &AvatarTs)) {
		if (AvatarURL != nullptr && AvatarTs != nullptr) {
			setString(GG_KEY_AVATARURL, AvatarURL);
			setString(GG_KEY_AVATARTS, AvatarTs);
			mir_free(AvatarURL); mir_free(AvatarTs);
		}
		else {
			delSetting(GG_KEY_AVATARURL);
			delSetting(GG_KEY_AVATARTS);
		}
		setByte(GG_KEY_AVATARREQUESTED, 1);

		PROTO_AVATAR_INFORMATION ai = { 0 };
		getavatarinfo((WPARAM)GAIF_FORCE, (LPARAM)&ai);
	}
#ifdef DEBUGMODE
	debugLogA("getOwnAvatarThread(): end");
#endif
}

void GaduProto::getOwnAvatar()
{
	if (getByte(GG_KEY_ENABLEAVATARS, GG_KEYDEF_ENABLEAVATARS) && getDword(GG_KEY_UIN, 0)) {
#ifdef DEBUGMODE
		debugLogA("getOwnAvatar(): ForkThread 2 GaduProto::getOwnAvatarThread");
#endif
		ForkThread(&GaduProto::getOwnAvatarThread, nullptr);
	}
}

void __cdecl GaduProto::setavatarthread(void *param)
{
	debugLogA("setavatarthread(): started. Trying to set user avatar.");

	//read file
	wchar_t *szFilename = (wchar_t*)param;
	int file_fd = _wopen(szFilename, _O_RDONLY | _O_BINARY, _S_IREAD);
	if (file_fd == -1) {
		debugLogW(L"setavatarthread(): Failed to open avatar file errno=%d: %s", errno, ws_strerror(errno));
		wchar_t error[512];
		mir_snwprintf(error, TranslateT("Cannot open avatar file. ERROR: %d: %s\n%s"), errno, ws_strerror(errno), szFilename);
		showpopup(m_tszUserName, error, GG_POPUP_ERROR);
		mir_free(szFilename);
		int prevType = getByte(GG_KEY_AVATARTYPEPREV, -1);
		if (prevType != -1)
			setByte(GG_KEY_AVATARTYPE, prevType);
		delSetting(GG_KEY_AVATARTYPEPREV);
		getOwnAvatar();
#ifdef DEBUGMODE
		debugLogA("setavatarthread(): end. err1");
#endif
		return;
	}

	long avatarFileLen = _filelength(file_fd);
	char* avatarFile = (char*)mir_alloc(avatarFileLen);
	_read(file_fd, avatarFile, avatarFileLen);
	_close(file_fd);

	ptrA avatarFileB64(mir_base64_encode(avatarFile, avatarFileLen));
	mir_free(avatarFile);

	CMStringA avatarFileB64Enc(mir_urlEncode(avatarFileB64));

	char dataPrefix[64];
	mir_snprintf(dataPrefix, "uin=%d&photo=", getDword(GG_KEY_UIN, 0));
	size_t dataPrefixLen = mir_strlen(dataPrefix);

	size_t dataLen = dataPrefixLen + avatarFileB64Enc.GetLength();
	char* data = (char*)mir_alloc(dataLen);
	memcpy(data, dataPrefix, dataPrefixLen);
	memcpy(data + dataPrefixLen, avatarFileB64Enc, avatarFileB64Enc.GetLength());

	//check if we have token, if no - receive new one
	oauth_checktoken(0);
	char* token = getStringA(GG_KEY_TOKEN);

	//construct request
	NETLIBHTTPREQUEST req = { 0 };
	req.cbSize = sizeof(req);
	req.requestType = REQUEST_POST;
	req.szUrl = "http://avatars.nowe.gg/upload";
	req.flags = NLHRF_NODUMP | NLHRF_HTTP11;
	req.headersCount = 10;
	NETLIBHTTPHEADER httpHeaders[10];
	httpHeaders[0].szName = "X-Request";
	httpHeaders[0].szValue = "JSON";
	httpHeaders[1].szName = "Authorization";
	httpHeaders[1].szValue = token;
	httpHeaders[2].szName = "X-Requested-With";
	httpHeaders[2].szValue = "XMLHttpRequest";
	httpHeaders[3].szName = "From";
	httpHeaders[3].szValue = "avatars to avatars";
	httpHeaders[4].szName = "X-IM-Web-App-Version";
	httpHeaders[4].szValue = "10,5,2,13164";
	httpHeaders[5].szName = "User-Agent";
	httpHeaders[5].szValue = "avatars to avatars";
	httpHeaders[6].szName = "From";
	httpHeaders[6].szValue = NETLIB_USER_AGENT;
	httpHeaders[7].szName = "Content-type";
	httpHeaders[7].szValue = "application/x-www-form-urlencoded; charset=utf-8";
	httpHeaders[8].szName = "Accept";
	httpHeaders[8].szValue = "application/json";
	httpHeaders[9].szName = "Referer";
	httpHeaders[9].szValue = "http://avatars.nowe.gg/.static/index_new_22.0.2_595nwh.html";
	req.headers = httpHeaders;
	req.pData = data;
	req.dataLength = int(dataLen);

	// send request
	int resultSuccess = 0;
	int needRepeat = 0;
	{
		NLHR_PTR resp(Netlib_HttpTransaction(m_hNetlibUser, &req));
		if (resp) {
			if (resp->resultCode == 200 && resp->dataLength > 0 && resp->pData) {
				debugLogA("setavatarthread(): 1 resp.data= %s", resp->pData);
				resultSuccess = 1;
			}
			else {
				debugLogA("setavatarthread() Invalid response code from HTTP request [%d]", resp->resultCode);
				if (resp->resultCode == 399 || resp->resultCode == 403 || resp->resultCode == 401) {
					needRepeat = 1;
				}
			}
		}
		else {
			debugLogA("setavatarthread(): No response from HTTP request");
		}
	}

	// check if we should repeat request
	if (needRepeat) {
		// Access Token expired - force obtain new
		oauth_checktoken(1);
		mir_free(token);
		token = getStringA(GG_KEY_TOKEN);
		httpHeaders[1].szValue = token;

		//construct 2nd request
		memset(&req, 0, sizeof(req));
		req.cbSize = sizeof(req);
		req.requestType = REQUEST_POST;
		req.szUrl = "http://avatars.nowe.gg/upload";
		req.flags = NLHRF_NODUMP | NLHRF_HTTP11;
		req.headersCount = 10;
		req.headers = httpHeaders;
		req.pData = data;
		req.dataLength = int(dataLen);

		NLHR_PTR resp(Netlib_HttpTransaction(m_hNetlibUser, &req));
		if (resp) {
			if (resp->resultCode == 200 && resp->dataLength > 0 && resp->pData) {
				debugLogA("setavatarthread(): 2 resp.data= %s", resp->pData);
				resultSuccess = 1;
			}
			else debugLogA("setavatarthread(): Invalid response code from HTTP request [%d]", resp->resultCode);
		}
		else debugLogA("setavatarthread(): No response from HTTP request");
	}

	//clean and end thread
	mir_free(token);
	mir_free(data);

	if (resultSuccess) {
		debugLogA("setavatarthread(): User avatar set successfully.");
	}
	else {
		int prevType = getByte(GG_KEY_AVATARTYPEPREV, -1);
		if (prevType != -1)
			setByte(GG_KEY_AVATARTYPE, prevType);
		debugLogA("setavatarthread(): Failed to set user avatar.");
	}
	delSetting(GG_KEY_AVATARTYPEPREV);

	mir_free(szFilename);
	getOwnAvatar();
#ifdef DEBUGMODE
	debugLogA("setavatarthread(): end.");
#endif

}

void GaduProto::setAvatar(const wchar_t *szFilename)
{
#ifdef DEBUGMODE
	debugLogA("setAvatar(): ForkThread 3 GaduProto::setavatarthread");
#endif
	ForkThread(&GaduProto::setavatarthread, mir_wstrdup(szFilename));
}
