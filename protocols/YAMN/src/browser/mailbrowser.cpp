/*
 * This code implements window handling (new mail)
 *
 * (c) majvan 2002-2004
 */
 /* There can be problems when compiling this file, because in this file
  * we are using both unicode and no-unicode functions and compiler does not
  * like it in one file
  * When you got errors, try to comment the #define <stdio.h> and compile, then
  * put it back to uncommented and compile again :)
  */

#include "../stdafx.h" 

#define	TIMER_FLASHING 0x09061979
#define MAILBROWSER_MINXSIZE	200		//min size of mail browser window
#define MAILBROWSER_MINYSIZE	130

#define MAILBROWSERTITLE LPGEN("%s - %d new mail messages, %d total")

void __cdecl ShowEmailThread(void *Param);

//--------------------------------------------------------------------------------------------------
char *s_MonthNames[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
bool bDate = false, bSub = false, bSize = false, bFrom = false;
int PosX = 0, PosY = 0, SizeX = 460, SizeY = 100;
int HeadSizeX = 0x2b2, HeadSizeY = 0x0b5, HeadPosX = 100, HeadPosY = 100;
int HeadSplitPos = 250; // per-mils of the size 
static int FromWidth = 250, SubjectWidth = 280, SizeWidth = 50, SizeDate = 205;
unsigned char optDateTime = (SHOWDATELONG | SHOWDATENOTODAY);

struct CMailNumbersSub
{
	int Total;		//any mail
	int New;			//uses YAMN_MSG_NEW flag
	int UnSeen;			//uses YAMN_MSG_UNSEEN flag
	//	int Browser;		//uses YAMN_MSG_BROWSER flag
	int BrowserUC;		//uses YAMN_MSG_BROWSER flag and YAMN_MSG_UNSEEN flag
	int Display;		//uses YAMN_MSG_DISPLAY flag
	int DisplayTC;		//uses YAMN_MSG_DISPLAY flag and YAMN_MSG_DISPLAYC flag
	int DisplayUC;		//uses YAMN_MSG_DISPLAY flag and YAMN_MSG_DISPLAYC flag and YAMN_MSG_UNSEEN flag
	int Popup;			//uses YAMN_MSG_POPUP flag
	int PopupTC;		//uses YAMN_MSG_POPUPC flag
	int PopupNC;		//uses YAMN_MSG_POPUPC flag and YAMN_MSG_NEW flag
	int PopupRun;		//uses YAMN_MSG_POPUP flag and YAMN_MSG_NEW flag
	int PopupSL2NC;		//uses YAMN_MSG_SPAML2 flag and YAMN_MSG_NEW flag
	int PopupSL3NC;		//uses YAMN_MSG_SPAML3 flag and YAMN_MSG_NEW flag
	//	int SysTray;		//uses YAMN_MSG_SYSTRAY flag
	int SysTrayUC;		//uses YAMN_MSG_SYSTRAY flag and YAMN_MSG_UNSEEN flag
	//	int Sound;		//uses YAMN_MSG_SOUND flag
	int SoundNC;		//uses YAMN_MSG_SOUND flag and YAMN_MSG_NEW flag
	//	int App;		//uses YAMN_MSG_APP flag
	int AppNC;		//uses YAMN_MSG_APP flag and YAMN_MSG_NEW flag
	int EventNC;		//uses YAMN_MSG_NEVENT flag and YAMN_MSG_NEW flag
};

struct CMailNumbers
{
	struct CMailNumbersSub Real;
	struct CMailNumbersSub Virtual;
};

struct CMailWinUserInfo
{
	CAccount *Account;
	int TrayIconState;
	BOOL UpdateMailsMessagesAccess;
	BOOL Seen;
	BOOL RunFirstTime;
};

struct CChangeContent
{
	uint32_t nflags;
	uint32_t nnflags;
};

struct CUpdateMails
{
	struct CChangeContent *Flags;
	BOOL Waiting;
	HANDLE Copied;
};

struct CSortList
{
	HWND hDlg;
	int	iSubItem;
};

// Retrieves CAccount *, whose mails are displayed in ListMails
// hLM- handle of dialog window
// returns handle of account
inline CAccount *GetWindowAccount(HWND hDialog);

// Looks to mail flags and increment mail counter (e.g. if mail is new, increments the new mail counter
// msgq- mail, which increments the counters
// MN- counnters structure
void IncrementMailCounters(HYAMNMAIL msgq, struct CMailNumbers *MN);

enum
{
	UPDATE_FAIL = 0,		//function failed
	UPDATE_NONE,		//none update has been performed
	UPDATE_OK,		//some changes occured, update performed
};

// Just looks for mail changes in account and update the mail browser window
// hDlg- dialog handle
// ActualAccount- account handle
// nflags- flags what to do when new mail arrives
// nnflags- flags what to do when no new mail arrives
// returns one of UPDATE_XXX value(not implemented yet)
int UpdateMails(HWND hDlg, CAccount *ActualAccount, uint32_t nflags, uint32_t nnflags);

// When new mail occurs, shows window, plays sound, runs application...
// hDlg- dialog handle. Dialog of mailbrowser is already created and actions are performed over this window
// ActualAccount- handle of account, whose mails are to be notified
// MN- statistics of mails in account
// nflags- what to do or not to do (e.g. to show mailbrowser window or prohibit to show)
// nflags- flags what to do when new mail arrives
// nnflags- flags what to do when no new mail arrives
void DoMailActions(HWND hDlg, CAccount *ActualAccount, struct CMailNumbers *MN, uint32_t nflags, uint32_t nnflags);

// Looks for items in mailbrowser and if they were deleted, delete them from browser window
// hListView- handle of listview window
// ActualAccount- handle of account, whose mails are show
// MailNumbers- pointer to structure, in which function stores numbers of mails with some property
// returns one of UPDATE_XXX value (not implemented yet)
int ChangeExistingMailStatus(HWND hListView, CAccount *ActualAccount);

// Adds new mails to ListView and if any new, shows multi popup (every new message is new popup window created by popup plugin)
// hListView- handle of listview window
// ActualAccount- handle of account, whose mails are show
// NewMailPopup- pointer to prepared structure for popup plugin, can be NULL if no popup show
// MailNumbers- pointer to structure, in which function stores numbers of mails with some property
// nflags- flags what to do when new mail arrives
// returns one of UPDATE_XXX value (not implemented yet)
int AddNewMailsToListView(HWND hListView, CAccount *ActualAccount, uint32_t nflags);

// Window callback procedure for popup window (created by popup plugin)
LRESULT CALLBACK NewMailPopupProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Window callback procedure for popup window (created by popup plugin)
LRESULT CALLBACK NoNewMailPopupProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Dialog callback procedure for mail browser
INT_PTR CALLBACK DlgProcYAMNMailBrowser(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

// MailBrowser thread function creates window if needed, tray icon and plays sound
void __cdecl MailBrowser(void *Param);

LRESULT CALLBACK ListViewSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Runs mail browser in new thread
INT_PTR RunMailBrowserSvc(WPARAM, LPARAM);

#define	YAMN_BROWSER_SHOWPOPUP	0x01

//	list view items' order criteria
#define LVORDER_NOORDER		-1
#define LVORDER_STRING		 0
#define LVORDER_NUMERIC		 1
#define LVORDER_DATETIME	 2

//	list view order direction
#define LVORDER_ASCENDING	 1
#define LVORDER_NONE		 0
#define LVORDER_DESCENDING	-1

//	list view sort type
#define LVSORTPRIORITY_NONE -1

//	List view column info.
typedef struct _SAMPLELISTVIEWCOLUMN
{
	UINT	 uCXCol;		//	index
	int		 nSortType;		//	sorting type (STRING = 0, NUMERIC, DATE, DATETIME)
	int		 nSortOrder;	//	sorting order (ASCENDING = -1, NONE, DESCENDING)
	int		 nPriority;		//	sort priority (-1 for none, 0, 1, ..., nColumns - 1 maximum)
	wchar_t lpszName[128];	//	column name
} SAMPLELISTVIEWCOLUMN;

//	Compare priority
typedef struct _LVCOMPAREINFO
{
	int	iIdx;				//	Index
	int iPriority;			//	Priority
} LVCOMPAREINFO, *LPLVCOMPAREINFO;

//--------------------------------------------------------------------------------------------------

LPARAM readItemLParam(HWND hwnd, uint32_t iItem)
{
	LVITEM item;
	item.mask = LVIF_PARAM;
	item.iItem = iItem;
	item.iSubItem = 0;
	SendMessage(hwnd, LVM_GETITEM, 0, (LPARAM)&item);
	return item.lParam;
}

inline CAccount *GetWindowAccount(HWND hDlg)
{
	struct CMailWinUserInfo *mwui = (struct CMailWinUserInfo *)GetWindowLongPtr(hDlg, DWLP_USER);

	return (mwui == nullptr) ? nullptr : mwui->Account;
}

void IncrementMailCounters(HYAMNMAIL msgq, struct CMailNumbers *MN)
{
	if (msgq->Flags & YAMN_MSG_VIRTUAL)
		MN->Virtual.Total++;
	else
		MN->Real.Total++;

	if (msgq->Flags & YAMN_MSG_NEW)
		if (msgq->Flags & YAMN_MSG_VIRTUAL)
			MN->Virtual.New++;
		else
			MN->Real.New++;
	if (msgq->Flags & YAMN_MSG_UNSEEN)
		if (msgq->Flags & YAMN_MSG_VIRTUAL)
			MN->Virtual.UnSeen++;
		else
			MN->Real.UnSeen++;
	if ((msgq->Flags & (YAMN_MSG_UNSEEN | YAMN_MSG_BROWSER)) == (YAMN_MSG_UNSEEN | YAMN_MSG_BROWSER))
		if (msgq->Flags & YAMN_MSG_VIRTUAL)
			MN->Virtual.BrowserUC++;
		else
			MN->Real.BrowserUC++;
	if (msgq->Flags & YAMN_MSG_DISPLAY)
		if (msgq->Flags & YAMN_MSG_VIRTUAL)
			MN->Virtual.Display++;
		else
			MN->Real.Display++;
	if ((msgq->Flags & (YAMN_MSG_DISPLAYC | YAMN_MSG_DISPLAY)) == (YAMN_MSG_DISPLAYC | YAMN_MSG_DISPLAY))
		if (msgq->Flags & YAMN_MSG_VIRTUAL)
			MN->Virtual.DisplayTC++;
		else
			MN->Real.DisplayTC++;
	if ((msgq->Flags & (YAMN_MSG_UNSEEN | YAMN_MSG_DISPLAYC | YAMN_MSG_DISPLAY)) == (YAMN_MSG_UNSEEN | YAMN_MSG_DISPLAYC | YAMN_MSG_DISPLAY))
		if (msgq->Flags & YAMN_MSG_VIRTUAL)
			MN->Virtual.DisplayUC++;
		else
			MN->Real.DisplayUC++;
	if (msgq->Flags & YAMN_MSG_POPUP)
		if (msgq->Flags & YAMN_MSG_VIRTUAL)
			MN->Virtual.Popup++;
		else
			MN->Real.Popup++;
	if ((msgq->Flags & YAMN_MSG_POPUPC) == YAMN_MSG_POPUPC)
		if (msgq->Flags & YAMN_MSG_VIRTUAL)
			MN->Virtual.PopupTC++;
		else
			MN->Real.PopupTC++;
	if ((msgq->Flags & (YAMN_MSG_NEW | YAMN_MSG_POPUPC)) == (YAMN_MSG_NEW | YAMN_MSG_POPUPC))
		if (msgq->Flags & YAMN_MSG_VIRTUAL)
			MN->Virtual.PopupNC++;
		else
			MN->Real.PopupNC++;
	if ((msgq->Flags & (YAMN_MSG_NEW | YAMN_MSG_POPUP)) == (YAMN_MSG_NEW | YAMN_MSG_POPUP))
		if (msgq->Flags & YAMN_MSG_VIRTUAL)
			MN->Virtual.PopupRun++;
		else
			MN->Real.PopupRun++;
	if ((msgq->Flags & YAMN_MSG_NEW) && YAMN_MSG_SPAML(msgq->Flags, YAMN_MSG_SPAML2))
		if (msgq->Flags & YAMN_MSG_VIRTUAL)
			MN->Virtual.PopupSL2NC++;
		else
			MN->Real.PopupSL2NC++;
	if ((msgq->Flags & YAMN_MSG_NEW) && YAMN_MSG_SPAML(msgq->Flags, YAMN_MSG_SPAML3))
		if (msgq->Flags & YAMN_MSG_VIRTUAL)
			MN->Virtual.PopupSL3NC++;
		else
			MN->Real.PopupSL3NC++;
	/*	if (msgq->MailData->Flags & YAMN_MSG_SYSTRAY)
			if (msgq->Flags & YAMN_MSG_VIRTUAL)
			MN->Virtual.SysTray++;
			else
			MN->Real.SysTray++;
			*/	if ((msgq->Flags & (YAMN_MSG_UNSEEN | YAMN_MSG_SYSTRAY)) == (YAMN_MSG_UNSEEN | YAMN_MSG_SYSTRAY))
				if (msgq->Flags & YAMN_MSG_VIRTUAL)
					MN->Virtual.SysTrayUC++;
				else
					MN->Real.SysTrayUC++;
			/*	if (msgq->MailData->Flags & YAMN_MSG_SOUND)
					if (msgq->Flags & YAMN_MSG_VIRTUAL)
					MN->Virtual.Sound++;
					else
					MN->Real.Sound++;
					*/	if ((msgq->Flags & (YAMN_MSG_NEW | YAMN_MSG_SOUND)) == (YAMN_MSG_NEW | YAMN_MSG_SOUND))
						if (msgq->Flags & YAMN_MSG_VIRTUAL)
							MN->Virtual.SoundNC++;
						else
							MN->Real.SoundNC++;
					/*	if (msgq->MailData->Flags & YAMN_MSG_APP)
							if (msgq->Flags & YAMN_MSG_VIRTUAL)
							MN->Virtual.App++;
							else
							MN->Real.App++;
							*/	if ((msgq->Flags & (YAMN_MSG_NEW | YAMN_MSG_APP)) == (YAMN_MSG_NEW | YAMN_MSG_APP))
								if (msgq->Flags & YAMN_MSG_VIRTUAL)
									MN->Virtual.AppNC++;
								else
									MN->Real.AppNC++;
							if ((msgq->Flags & (YAMN_MSG_NEW | YAMN_MSG_NEVENT)) == (YAMN_MSG_NEW | YAMN_MSG_NEVENT))
								if (msgq->Flags & YAMN_MSG_VIRTUAL)
									MN->Virtual.EventNC++;
								else
									MN->Real.EventNC++;
}

int UpdateMails(HWND hDlg, CAccount *ActualAccount, uint32_t nflags, uint32_t nnflags)
{
	struct CMailNumbers MN;

	BOOL Loaded;
	BOOL RunMailBrowser, RunPopups;

	struct CMailWinUserInfo *mwui = (struct CMailWinUserInfo *)GetWindowLongPtr(hDlg, DWLP_USER);
	//now we ensure read access for account and write access for its mails
	if (WAIT_OBJECT_0 != WaitToReadFcn(ActualAccount->AccountAccessSO)) {
		PostMessage(hDlg, WM_DESTROY, 0, 0);
		return UPDATE_FAIL;
	}

	if (WAIT_OBJECT_0 != WaitToWriteFcn(ActualAccount->MessagesAccessSO)) {
		ReadDoneFcn(ActualAccount->AccountAccessSO);
		PostMessage(hDlg, WM_DESTROY, 0, 0);
		return UPDATE_FAIL;
	}

	memset(&MN, 0, sizeof(MN));

	for (HYAMNMAIL msgq = (HYAMNMAIL)ActualAccount->Mails; msgq != nullptr; msgq = msgq->Next) {
		if (!LoadedMailData(msgq))				//check if mail is already in memory
		{
			Loaded = false;
			if (nullptr == LoadMailData(msgq))			//if we could not load mail to memory, consider this mail deleted and do not display it
				continue;
		}
		else
			Loaded = true;

		IncrementMailCounters(msgq, &MN);

		if (!Loaded)
			UnloadMailData(msgq);			//do not keep data for mail in memory
	}

	if (mwui != nullptr)
		mwui->UpdateMailsMessagesAccess = TRUE;

	//Now we are going to check if extracting data from mail headers are needed.
	//If popups will be displayed or mailbrowser window
	if ((((mwui != nullptr) && !(mwui->RunFirstTime)) &&
		(
			((nnflags & YAMN_ACC_MSGP) && !(MN.Real.BrowserUC + MN.Virtual.BrowserUC)) ||
			((nflags & YAMN_ACC_MSGP) && (MN.Real.BrowserUC + MN.Virtual.BrowserUC))
			)
		) ||		//if mail window was displayed before and flag YAMN_ACC_MSGP is set
		((nnflags & YAMN_ACC_MSG) && !(MN.Real.BrowserUC + MN.Virtual.BrowserUC)) ||		//if needed to run mailbrowser when no unseen and no unseen mail found
		((nflags & YAMN_ACC_MSG) && (MN.Real.BrowserUC + MN.Virtual.BrowserUC)) ||		//if unseen mails found, we sure run mailbrowser
		((nflags & YAMN_ACC_ICO) && (MN.Real.SysTrayUC + MN.Virtual.SysTrayUC))
		)			//if needed to run systray
		RunMailBrowser = TRUE;
	else
		RunMailBrowser = FALSE;

	// if some popups with mails are needed to show
	if ((nflags & YAMN_ACC_POP) && (ActualAccount->Flags & YAMN_ACC_POPN) && (MN.Real.PopupNC + MN.Virtual.PopupNC))
		RunPopups = TRUE;
	else	RunPopups = FALSE;

	if (RunMailBrowser)
		ChangeExistingMailStatus(GetDlgItem(hDlg, IDC_LISTMAILS), ActualAccount);
	if (RunMailBrowser || RunPopups)
		AddNewMailsToListView(hDlg == nullptr ? nullptr : GetDlgItem(hDlg, IDC_LISTMAILS), ActualAccount, nflags);

	if (RunMailBrowser) {
		size_t len = mir_strlen(ActualAccount->Name) + mir_strlen(Translate(MAILBROWSERTITLE)) + 10;	//+10 chars for numbers
		char *TitleStrA = new char[len];
		wchar_t *TitleStrW = new wchar_t[len];

		mir_snprintf(TitleStrA, len, Translate(MAILBROWSERTITLE), ActualAccount->Name, MN.Real.DisplayUC + MN.Virtual.DisplayUC, MN.Real.Display + MN.Virtual.Display);
		MultiByteToWideChar(CP_ACP, MB_USEGLYPHCHARS, TitleStrA, -1, TitleStrW, (int)mir_strlen(TitleStrA) + 1);
		SetWindowTextW(hDlg, TitleStrW);
		delete[] TitleStrA;
		delete[] TitleStrW;
	}

	DoMailActions(hDlg, ActualAccount, &MN, nflags, nnflags);

	SetRemoveFlagsInQueueFcn((HYAMNMAIL)ActualAccount->Mails, YAMN_MSG_NEW, 0, YAMN_MSG_NEW, YAMN_FLAG_REMOVE);				//rempve the new flag
	if (!RunMailBrowser)
		SetRemoveFlagsInQueueFcn((HYAMNMAIL)ActualAccount->Mails, YAMN_MSG_UNSEEN, YAMN_MSG_STAYUNSEEN, YAMN_MSG_UNSEEN, YAMN_FLAG_REMOVE);	//remove the unseen flag when it was not displayed and it has not "stay unseen" flag set

	if (mwui != nullptr) {
		mwui->UpdateMailsMessagesAccess = FALSE;
		mwui->RunFirstTime = FALSE;
	}

	WriteDoneFcn(ActualAccount->MessagesAccessSO);
	ReadDoneFcn(ActualAccount->AccountAccessSO);

	if (RunMailBrowser)
		UpdateWindow(GetDlgItem(hDlg, IDC_LISTMAILS));
	else if (hDlg != nullptr)
		DestroyWindow(hDlg);

	return 1;
}

int ChangeExistingMailStatus(HWND hListView, CAccount *ActualAccount)
{
	LVITEM item;
	HYAMNMAIL mail, msgq;

	int in = ListView_GetItemCount(hListView);
	item.mask = LVIF_PARAM;

	for (int i = 0; i < in; i++) {
		item.iItem = i;
		item.iSubItem = 0;
		if (TRUE == ListView_GetItem(hListView, &item))
			mail = (HYAMNMAIL)item.lParam;
		else
			continue;
		for (msgq = (HYAMNMAIL)ActualAccount->Mails; (msgq != nullptr) && (msgq != mail); msgq = msgq->Next);	//found the same mail in account queue
		if (msgq == nullptr)		//if mail was not found
			if (TRUE == ListView_DeleteItem(hListView, i)) {
				in--; i--;
				continue;
			}
	}

	return TRUE;
}

void MimeDateToLocalizedDateTime(char *datein, wchar_t *dateout, int lendateout);
int AddNewMailsToListView(HWND hListView, CAccount *ActualAccount, uint32_t nflags)
{
	wchar_t *FromStr;
	wchar_t SizeStr[20];
	wchar_t LocalDateStr[128];

	LVITEMW item;
	LVFINDINFO fi;

	int foundi = 0, lfoundi = 0;
	struct CHeader UnicodeHeader;
	BOOL Loaded, Extracted, FromStrNew = FALSE;

	memset(&item, 0, sizeof(item));
	memset(&UnicodeHeader, 0, sizeof(UnicodeHeader));

	if (hListView != nullptr) {
		item.mask = LVIF_TEXT | LVIF_PARAM;
		item.iItem = 0;
		memset(&fi, 0, sizeof(fi));
		fi.flags = LVFI_PARAM;						//let's go search item by lParam number
		lfoundi = 0;
	}

	POPUPDATAW NewMailPopup = {};
	NewMailPopup.lchContact = (ActualAccount->hContact != NULL) ? ActualAccount->hContact : (UINT_PTR)ActualAccount;
	NewMailPopup.lchIcon = g_plugin.getIcon(IDI_NEWMAIL);
	if (nflags & YAMN_ACC_POPC) {
		NewMailPopup.colorBack = ActualAccount->NewMailN.PopupB;
		NewMailPopup.colorText = ActualAccount->NewMailN.PopupT;
	}
	else {
		NewMailPopup.colorBack = GetSysColor(COLOR_BTNFACE);
		NewMailPopup.colorText = GetSysColor(COLOR_WINDOWTEXT);
	}
	NewMailPopup.iSeconds = ActualAccount->NewMailN.PopupTime;

	NewMailPopup.PluginWindowProc = NewMailPopupProc;
	NewMailPopup.PluginData = nullptr;					//it's new mail popup

	for (HYAMNMAIL msgq = (HYAMNMAIL)ActualAccount->Mails; msgq != nullptr; msgq = msgq->Next, lfoundi++) {
		//		now we hide mail pointer to item's lParam member. We can later use it to retrieve mail datas

		Extracted = FALSE; FromStr = nullptr; FromStrNew = FALSE;

		if (hListView != nullptr) {
			fi.lParam = (LPARAM)msgq;
			if (-1 != (foundi = ListView_FindItem(hListView, -1, &fi))) { // if mail is already in window
				lfoundi = foundi;
				continue; // do not insert any item
			}

			item.iItem = lfoundi; // insert after last found item
			item.lParam = (LPARAM)msgq;
		}

		if (!LoadedMailData(msgq)) { // check if mail is already in memory
			Loaded = false;
			if (nullptr == LoadMailData(msgq))			//if we could not load mail to memory, consider this mail deleted and do not display it
				continue;
		}
		else Loaded = true;

		if (((hListView != nullptr) && (msgq->Flags & YAMN_MSG_DISPLAY)) ||
			((nflags & YAMN_ACC_POP) && (ActualAccount->Flags & YAMN_ACC_POPN) && (msgq->Flags & YAMN_MSG_POPUP) && (msgq->Flags & YAMN_MSG_NEW))) {

			if (!Extracted) ExtractHeader(msgq->MailData->TranslatedHeader, msgq->MailData->CP, &UnicodeHeader);
			Extracted = TRUE;

			if ((UnicodeHeader.From != nullptr) && (UnicodeHeader.FromNick != nullptr)) {
				size_t size = mir_wstrlen(UnicodeHeader.From) + mir_wstrlen(UnicodeHeader.FromNick) + 4;
				FromStr = new wchar_t[size];
				mir_snwprintf(FromStr, size, L"%s <%s>", UnicodeHeader.FromNick, UnicodeHeader.From);
				FromStrNew = TRUE;
			}
			else if (UnicodeHeader.From != nullptr)
				FromStr = UnicodeHeader.From;
			else if (UnicodeHeader.FromNick != nullptr)
				FromStr = UnicodeHeader.FromNick;
			else if (UnicodeHeader.ReturnPath != nullptr)
				FromStr = UnicodeHeader.ReturnPath;

			if (nullptr == FromStr) {
				FromStr = L"";
				FromStrNew = FALSE;
			}
		}

		if ((hListView != nullptr) && (msgq->Flags & YAMN_MSG_DISPLAY)) {
			item.iSubItem = 0;
			item.pszText = FromStr;
			item.iItem = SendMessage(hListView, LVM_INSERTITEM, 0, (LPARAM)&item);

			item.iSubItem = 1;
			item.pszText = (nullptr != UnicodeHeader.Subject ? UnicodeHeader.Subject : (wchar_t *)L"");
			SendMessage(hListView, LVM_SETITEMTEXT, (WPARAM)item.iItem, (LPARAM)&item);

			item.iSubItem = 2;
			mir_snwprintf(SizeStr, L"%d kB", msgq->MailData->Size / 1024);
			item.pszText = SizeStr;
			SendMessage(hListView, LVM_SETITEMTEXT, (WPARAM)item.iItem, (LPARAM)&item);

			item.iSubItem = 3;
			item.pszText = L"";

			for (CMimeItem *heads = msgq->MailData->TranslatedHeader; heads != nullptr; heads = heads->Next) {
				if (!_stricmp(heads->name, "Date")) {
					MimeDateToLocalizedDateTime(heads->value, LocalDateStr, 128);
					item.pszText = LocalDateStr;
					break;
				}
			}
			SendMessage(hListView, LVM_SETITEMTEXT, (WPARAM)item.iItem, (LPARAM)&item);
		}

		if ((nflags & YAMN_ACC_POP) && (ActualAccount->Flags & YAMN_ACC_POPN) && (msgq->Flags & YAMN_MSG_POPUP) && (msgq->Flags & YAMN_MSG_NEW)) {
			mir_wstrncpy(NewMailPopup.lpwzContactName, FromStr, _countof(NewMailPopup.lpwzContactName));
			mir_wstrncpy(NewMailPopup.lpwzText, UnicodeHeader.Subject, _countof(NewMailPopup.lpwzText));

			PYAMN_MAILSHOWPARAM MailParam = (PYAMN_MAILSHOWPARAM)malloc(sizeof(YAMN_MAILSHOWPARAM));
			if (MailParam) {
				MailParam->account = ActualAccount;
				MailParam->mail = msgq;
				MailParam->ThreadRunningEV = nullptr;
				NewMailPopup.PluginData = MailParam;
				PUAddPopupW(&NewMailPopup);
			}
		}

		if ((msgq->Flags & YAMN_MSG_UNSEEN) && (ActualAccount->NewMailN.Flags & YAMN_ACC_KBN))
			CallService(MS_KBDNOTIFY_EVENTSOPENED, 1, NULL);

		if (FromStrNew)
			delete[] FromStr;

		if (Extracted) {
			DeleteHeaderContent(&UnicodeHeader);
			memset(&UnicodeHeader, 0, sizeof(UnicodeHeader));
		}

		if (!Loaded) {
			SaveMailData(msgq);
			UnloadMailData(msgq);			//do not keep data for mail in memory
		}
	}

	return TRUE;
}

void DoMailActions(HWND hDlg, CAccount *ActualAccount, struct CMailNumbers *MN, uint32_t nflags, uint32_t nnflags)
{
	NOTIFYICONDATA nid = {};
	nid.cbSize = sizeof(nid);
	nid.hWnd = hDlg;

	if (MN->Real.EventNC + MN->Virtual.EventNC)
		NotifyEventHooks(hNewMailHook, 0, 0);

	if ((nflags & YAMN_ACC_KBN) && (MN->Real.PopupRun + MN->Virtual.PopupRun))
		CallService(MS_KBDNOTIFY_STARTBLINK, (WPARAM)MN->Real.PopupNC + MN->Virtual.PopupNC, NULL);

	if ((nflags & YAMN_ACC_CONT) && (MN->Real.PopupRun + MN->Virtual.PopupRun)) {
		wchar_t tszMsg[250];
		mir_snwprintf(tszMsg, TranslateT("%s : %d new mail message(s), %d total"), _A2T(ActualAccount->Name).get(), MN->Real.PopupNC + MN->Virtual.PopupNC, MN->Real.PopupTC + MN->Virtual.PopupTC);

		if (!(nflags & YAMN_ACC_CONTNOEVENT)) {
			CLISTEVENT evt = {};
			evt.flags = CLEF_UNICODE;
			evt.hContact = ActualAccount->hContact;
			evt.hIcon = g_plugin.getIcon(IDI_NEWMAIL);
			evt.hDbEvent = ActualAccount->hContact;
			evt.lParam = ActualAccount->hContact;
			evt.pszService = MS_YAMN_CLISTDBLCLICK;
			evt.szTooltip.w = tszMsg;
			g_clistApi.pfnAddEvent(&evt);
		}
		db_set_ws(ActualAccount->hContact, "CList", "StatusMsg", tszMsg);

		if (nflags & YAMN_ACC_CONTNICK)
			g_plugin.setWString(ActualAccount->hContact, "Nick", tszMsg);
	}

	if ((nflags & YAMN_ACC_POP) &&
		!(ActualAccount->Flags & YAMN_ACC_POPN) &&
		(MN->Real.PopupRun + MN->Virtual.PopupRun)) {
		POPUPDATAW NewMailPopup;

		NewMailPopup.lchContact = (ActualAccount->hContact != NULL) ? ActualAccount->hContact : (UINT_PTR)ActualAccount;
		NewMailPopup.lchIcon = g_plugin.getIcon(IDI_NEWMAIL);
		if (nflags & YAMN_ACC_POPC) {
			NewMailPopup.colorBack = ActualAccount->NewMailN.PopupB;
			NewMailPopup.colorText = ActualAccount->NewMailN.PopupT;
		}
		else {
			NewMailPopup.colorBack = GetSysColor(COLOR_BTNFACE);
			NewMailPopup.colorText = GetSysColor(COLOR_WINDOWTEXT);
		}
		NewMailPopup.iSeconds = ActualAccount->NewMailN.PopupTime;

		NewMailPopup.PluginWindowProc = NewMailPopupProc;
		NewMailPopup.PluginData = (void *)nullptr;	//multiple popups

		mir_wstrncpy(NewMailPopup.lpwzContactName, _A2T(ActualAccount->Name), _countof(NewMailPopup.lpwzContactName));
		mir_snwprintf(NewMailPopup.lpwzText, TranslateT("%d new mail message(s), %d total"), MN->Real.PopupNC + MN->Virtual.PopupNC, MN->Real.PopupTC + MN->Virtual.PopupTC);
		PUAddPopupW(&NewMailPopup);
	}

	// destroy tray icon if no new mail
	if ((MN->Real.SysTrayUC + MN->Virtual.SysTrayUC == 0) && (hDlg != nullptr))
		Shell_NotifyIcon(NIM_DELETE, &nid);

	// and remove the event
	if ((nflags & YAMN_ACC_CONT) && (!(nflags & YAMN_ACC_CONTNOEVENT)) && (MN->Real.UnSeen + MN->Virtual.UnSeen == 0))
		g_clistApi.pfnRemoveEvent(ActualAccount->hContact, ActualAccount->hContact);

	if ((MN->Real.BrowserUC + MN->Virtual.BrowserUC == 0) && (hDlg != nullptr)) {
		if (!IsWindowVisible(hDlg) && !(nflags & YAMN_ACC_MSG))
			PostMessage(hDlg, WM_DESTROY, 0, 0);				//destroy window if no new mail and window is not visible
		if (nnflags & YAMN_ACC_MSG)											//if no new mail and msg should be executed
		{
			SetForegroundWindow(hDlg);
			ShowWindow(hDlg, SW_SHOWNORMAL);
		}
	}
	else
		if (hDlg != nullptr)								//else insert icon and set window if new mails
		{
			SendDlgItemMessageW(hDlg, IDC_LISTMAILS, LVM_SCROLL, 0, (LPARAM)0x7ffffff);

			if ((nflags & YAMN_ACC_ICO) && (MN->Real.SysTrayUC + MN->Virtual.SysTrayUC)) {
				nid.hIcon = g_plugin.getIcon(IDI_NEWMAIL);
				nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
				nid.uCallbackMessage = WM_YAMN_NOTIFYICON;
				mir_snwprintf(nid.szTip, L"%S %s", ActualAccount->Name, TranslateT("- new mail message(s)"));
				Shell_NotifyIcon(NIM_ADD, &nid);
				SetTimer(hDlg, TIMER_FLASHING, 500, nullptr);
			}
			if (nflags & YAMN_ACC_MSG)											//if no new mail and msg should be executed
				ShowWindow(hDlg, SW_SHOWNORMAL);
		}

	if (MN->Real.AppNC + MN->Virtual.AppNC != 0) {
		if (nflags & YAMN_ACC_APP) {
			PROCESS_INFORMATION pi;
			STARTUPINFOW si;
			memset(&si, 0, sizeof(si));
			si.cb = sizeof(si);

			if (ActualAccount->NewMailN.App != nullptr) {
				wchar_t *Command;
				if (ActualAccount->NewMailN.AppParam != nullptr)
					Command = new wchar_t[mir_wstrlen(ActualAccount->NewMailN.App) + mir_wstrlen(ActualAccount->NewMailN.AppParam) + 6];
				else
					Command = new wchar_t[mir_wstrlen(ActualAccount->NewMailN.App) + 6];

				if (Command != nullptr) {
					mir_wstrcpy(Command, L"\"");
					mir_wstrcat(Command, ActualAccount->NewMailN.App);
					mir_wstrcat(Command, L"\" ");
					if (ActualAccount->NewMailN.AppParam != nullptr)
						mir_wstrcat(Command, ActualAccount->NewMailN.AppParam);
					CreateProcessW(nullptr, Command, nullptr, nullptr, FALSE, NORMAL_PRIORITY_CLASS, nullptr, nullptr, &si, &pi);
					delete[] Command;
				}
			}
		}
	}

	if (MN->Real.SoundNC + MN->Virtual.SoundNC != 0)
		if (nflags & YAMN_ACC_SND)
			Skin_PlaySound(YAMN_NEWMAILSOUND);

	if ((nnflags & YAMN_ACC_POP) && (MN->Real.PopupRun + MN->Virtual.PopupRun == 0)) {
		POPUPDATAW NoNewMailPopup = {};

		NoNewMailPopup.lchContact = (ActualAccount->hContact != NULL) ? ActualAccount->hContact : (UINT_PTR)ActualAccount;
		NoNewMailPopup.lchIcon = g_plugin.getIcon(IDI_LAUNCHAPP);
		if (nflags & YAMN_ACC_POPC) {
			NoNewMailPopup.colorBack = ActualAccount->NoNewMailN.PopupB;
			NoNewMailPopup.colorText = ActualAccount->NoNewMailN.PopupT;
		}
		else {
			NoNewMailPopup.colorBack = GetSysColor(COLOR_BTNFACE);
			NoNewMailPopup.colorText = GetSysColor(COLOR_WINDOWTEXT);
		}
		NoNewMailPopup.iSeconds = ActualAccount->NoNewMailN.PopupTime;

		NoNewMailPopup.PluginWindowProc = NoNewMailPopupProc;
		NoNewMailPopup.PluginData = nullptr;					//it's not new mail popup

		mir_wstrncpy(NoNewMailPopup.lpwzContactName, _A2T(ActualAccount->Name), _countof(NoNewMailPopup.lpwzContactName));
		if (MN->Real.PopupSL2NC + MN->Virtual.PopupSL2NC)
			mir_snwprintf(NoNewMailPopup.lpwzText, TranslateT("No new mail message, %d spam(s)"), MN->Real.PopupSL2NC + MN->Virtual.PopupSL2NC);
		else
			mir_wstrncpy(NoNewMailPopup.lpwzText, TranslateT("No new mail message"), _countof(NoNewMailPopup.lpwzText));
		PUAddPopupW(&NoNewMailPopup);
	}

	if ((nflags & YAMN_ACC_CONT) && (MN->Real.PopupRun + MN->Virtual.PopupRun == 0)) {
		if (ActualAccount->hContact != NULL) {
			if (MN->Real.PopupTC + MN->Virtual.PopupTC) {
				char tmp[255];
				mir_snprintf(tmp, Translate("%d new mail message(s), %d total"), MN->Real.PopupNC + MN->Virtual.PopupNC, MN->Real.PopupTC + MN->Virtual.PopupTC);
				db_set_s(ActualAccount->hContact, "CList", "StatusMsg", tmp);
			}
			else db_set_s(ActualAccount->hContact, "CList", "StatusMsg", Translate("No new mail message"));

			if (nflags & YAMN_ACC_CONTNICK)
				g_plugin.setString(ActualAccount->hContact, "Nick", ActualAccount->Name);
		}
	}
	return;
}

LRESULT CALLBACK NewMailPopupProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR PluginParam = 0;
	switch (msg) {
	case WM_COMMAND:
		// if clicked and it's new mail popup window
		if ((HIWORD(wParam) == STN_CLICKED) && (-1 != (PluginParam = (INT_PTR)PUGetPluginData(hWnd)))) {
			MCONTACT hContact = 0;
			CAccount *Account;
			if (PluginParam) {
				PYAMN_MAILSHOWPARAM MailParam = new YAMN_MAILSHOWPARAM;
				memcpy(MailParam, (PINT_PTR)PluginParam, sizeof(YAMN_MAILSHOWPARAM));
				hContact = MailParam->account->hContact;
				Account = MailParam->account;
				mir_forkthread(ShowEmailThread, MailParam);
			}
			else {
				DBVARIANT dbv;

				hContact = PUGetContact(hWnd);

				if (!g_plugin.getString(hContact, "Id", &dbv)) {
					Account = (CAccount *)CallService(MS_YAMN_FINDACCOUNTBYNAME, (WPARAM)POP3Plugin, (LPARAM)dbv.pszVal);
					db_free(&dbv);
				}
				else Account = (CAccount *)hContact; //????

				if (WAIT_OBJECT_0 == WaitToReadFcn(Account->AccountAccessSO)) {
					switch (msg) {
					case WM_COMMAND:
						{
							YAMN_MAILBROWSERPARAM Param = {(HANDLE)nullptr, Account,
								(Account->NewMailN.Flags & ~YAMN_ACC_POP) | YAMN_ACC_MSGP | YAMN_ACC_MSG,
								(Account->NoNewMailN.Flags & ~YAMN_ACC_POP) | YAMN_ACC_MSGP | YAMN_ACC_MSG};

							RunMailBrowserSvc((WPARAM)&Param, (LPARAM)YAMN_MAILBROWSERVERSION);
						}
						break;
					}
					ReadDoneFcn(Account->AccountAccessSO);
				}
			}
			if ((Account->NewMailN.Flags & YAMN_ACC_CONT) && !(Account->NewMailN.Flags & YAMN_ACC_CONTNOEVENT))
				g_clistApi.pfnRemoveEvent(hContact, hContact);
		}
		__fallthrough;

	case WM_CONTEXTMENU:
		PUDeletePopup(hWnd);
		break;
	case UM_FREEPLUGINDATA:
		{
			PYAMN_MAILSHOWPARAM mpd = (PYAMN_MAILSHOWPARAM)PUGetPluginData(hWnd);
			if ((mpd) && (INT_PTR)mpd != -1)free(mpd);
			return FALSE;
		}
	case UM_INITPOPUP:
		//This is the equivalent to WM_INITDIALOG you'd get if you were the maker of dialog popups.
		WindowList_Add(YAMNVar.MessageWnds, hWnd);
		break;
	case UM_DESTROYPOPUP:
		WindowList_Remove(YAMNVar.MessageWnds, hWnd);
		break;
	case WM_YAMN_STOPACCOUNT:
		{
			CAccount *ActualAccount;
			DBVARIANT dbv;

			MCONTACT hContact = PUGetContact(hWnd);

			if (!g_plugin.getString(hContact, "Id", &dbv)) {
				ActualAccount = (CAccount *)CallService(MS_YAMN_FINDACCOUNTBYNAME, (WPARAM)POP3Plugin, (LPARAM)dbv.pszVal);
				db_free(&dbv);
			}
			else
				ActualAccount = (CAccount *)hContact;

			if ((CAccount *)wParam != ActualAccount)
				break;
			DestroyWindow(hWnd);
			return 0;
		}
	case WM_NOTIFY:
	default:
		break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK NoNewMailPopupProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_COMMAND:
		if ((HIWORD(wParam) == STN_CLICKED) && (msg == WM_COMMAND)) {
			CAccount *ActualAccount;
			DBVARIANT dbv;

			MCONTACT hContact = PUGetContact(hWnd);

			if (!g_plugin.getString(hContact, "Id", &dbv)) {
				ActualAccount = (CAccount *)CallService(MS_YAMN_FINDACCOUNTBYNAME, (WPARAM)POP3Plugin, (LPARAM)dbv.pszVal);
				db_free(&dbv);
			}
			else
				ActualAccount = (CAccount *)hContact;

			if (WAIT_OBJECT_0 == WaitToReadFcn(ActualAccount->AccountAccessSO)) {
				switch (msg) {
				case WM_COMMAND:
					{
						YAMN_MAILBROWSERPARAM Param = {(HANDLE)nullptr, ActualAccount, ActualAccount->NewMailN.Flags, ActualAccount->NoNewMailN.Flags, nullptr};

						Param.nnflags = Param.nnflags | YAMN_ACC_MSG;			//show mails in account even no new mail in account
						Param.nnflags = Param.nnflags & ~YAMN_ACC_POP;

						Param.nflags = Param.nflags | YAMN_ACC_MSG;			//show mails in account even no new mail in account
						Param.nflags = Param.nflags & ~YAMN_ACC_POP;

						RunMailBrowserSvc((WPARAM)&Param, (LPARAM)YAMN_MAILBROWSERVERSION);
					}
					break;
				}
				ReadDoneFcn(ActualAccount->AccountAccessSO);
			}
			PUDeletePopup(hWnd);
		}
		break;

	case WM_CONTEXTMENU:
		PUDeletePopup(hWnd);
		break;

	case UM_FREEPLUGINDATA:
		//Here we'd free our own data, if we had it.
		return FALSE;
	case UM_INITPOPUP:
		//This is the equivalent to WM_INITDIALOG you'd get if you were the maker of dialog popups.
		WindowList_Add(YAMNVar.MessageWnds, hWnd);
		break;
	case UM_DESTROYPOPUP:
		WindowList_Remove(YAMNVar.MessageWnds, hWnd);
		break;
	case WM_YAMN_STOPACCOUNT:
		{
			CAccount *ActualAccount;
			DBVARIANT dbv;

			MCONTACT hContact = PUGetContact(hWnd);

			if (!g_plugin.getString(hContact, "Id", &dbv)) {
				ActualAccount = (CAccount *)CallService(MS_YAMN_FINDACCOUNTBYNAME, (WPARAM)POP3Plugin, (LPARAM)dbv.pszVal);
				db_free(&dbv);
			}
			else
				ActualAccount = (CAccount *)hContact;

			if ((CAccount *)wParam != ActualAccount)
				break;

			DestroyWindow(hWnd);
			return 0;
		}
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

#ifdef __GNUC__
//number of 100 ns periods between FILETIME 0 (1601/01/01 00:00:00.0000000) and TIMESTAMP 0 (1970/01/01 00:00:00)
#define NUM100NANOSEC  116444736000000000ULL
//The biggest time Get[Date|Time]Format can handle (Fri, 31 Dec 30827 23:59:59.9999999)
#define MAXFILETIME 0x7FFF35F4F06C7FFFULL
#else
#define NUM100NANOSEC  116444736000000000
#define MAXFILETIME 0x7FFF35F4F06C7FFF
#endif

ULONGLONG MimeDateToFileTime(char *datein)
{
	char *day = nullptr, *month = nullptr, *year = nullptr, *time = nullptr, *shift = nullptr;
	SYSTEMTIME st;
	ULONGLONG res = 0;
	int wShiftSeconds = TimeZone_ToLocal(0);
	GetLocalTime(&st);
	//datein = "Xxx, 1 Jan 2060 5:29:1 +0530 XXX";
	//datein = "Xxx,  1 Jan 2060 05:29:10 ";
	//datein = "      ManySpaces  1.5   Jan 2060 05::";
	//datein = "Xxx,  35 February 20 :29:10 ";
	//datein = "01.12.2007 (22:38:17)"; //
	if (datein) {
		char tmp[64];
		while (datein[0] == ' ')  datein++; // eat leading spaces
		strncpy(tmp, datein, 63); tmp[63] = 0;
		if (atoi(tmp)) { // Parseable integer on DayOfWeek field? Buggy mime date.
			day = tmp;
		}
		else {
			int i = 0;
			while (tmp[i] == ' ')i++; if (day = strchr(&tmp[i], ' ')) { day[0] = 0; day++; }
		}
		if (day) { while (day[0] == ' ')  day++; if (month = strchr(day, ' ')) { month[0] = 0; month++; } }
		if (month) { while (month[0] == ' ')month++; if (year = strchr(month, ' ')) { year[0] = 0; year++; } }
		if (year) { while (year[0] == ' ') year++; if (time = strchr(year, ' ')) { time[0] = 0; time++; } }
		if (time) { while (time[0] == ' ') time++; if (shift = strchr(time, ' ')) { shift[0] = 0; shift++; shift[5] = 0; } }

		if (year) {
			st.wYear = atoi(year);
			if (mir_strlen(year) < 4)	if (st.wYear < 70)st.wYear += 2000; else st.wYear += 1900;
		};
		if (month) for (int i = 0; i < 12; i++) if (strncmp(month, s_MonthNames[i], 3) == 0) { st.wMonth = i + 1; break; }
		if (day) st.wDay = atoi(day);
		if (time) {
			char *h, *m, *s;
			h = time;
			if (m = strchr(h, ':')) {
				m[0] = 0; m++;
				if (s = strchr(m, ':')) { s[0] = 0; s++; }
			}
			else s = nullptr;
			st.wHour = atoi(h);
			st.wMinute = m ? atoi(m) : 0;
			st.wSecond = s ? atoi(s) : 0;
		}
		else { st.wHour = st.wMinute = st.wSecond = 0; }

		if (shift) {
			if (mir_strlen(shift) < 4) {
				//has only hour
				wShiftSeconds = (atoi(shift)) * 3600;
			}
			else {
				char *smin = shift + mir_strlen(shift) - 2;
				int ismin = atoi(smin);
				smin[0] = 0;
				int ishour = atoi(shift);
				wShiftSeconds = (ishour * 60 + (ishour < 0 ? -1 : 1) * ismin) * 60;
			}
		}
	} // if (datein)
	FILETIME ft;
	if (SystemTimeToFileTime(&st, &ft)) {
		res = ((ULONGLONG)ft.dwHighDateTime << 32) | ((ULONGLONG)ft.dwLowDateTime);
		LONGLONG w100nano = Int32x32To64((uint32_t)wShiftSeconds, 10000000);
		res -= w100nano;
	}
	else {
		res = 0;
	}
	return res;
}

void FileTimeToLocalizedDateTime(LONGLONG filetime, wchar_t *dateout, int lendateout)
{
	int localeID = Langpack_GetDefaultLocale();
	//int localeID = MAKELCID(LANG_URDU, SORT_DEFAULT);
	if (localeID == CALLSERVICE_NOTFOUND) localeID = LOCALE_USER_DEFAULT;
	if (filetime > MAXFILETIME) filetime = MAXFILETIME;
	else if (filetime <= 0) {
		wcsncpy(dateout, TranslateT("Invalid"), lendateout);
		return;
	}
	SYSTEMTIME st;
	uint16_t wTodayYear = 0, wTodayMonth = 0, wTodayDay = 0;
	FILETIME ft;
	BOOL willShowDate = !(optDateTime & SHOWDATENOTODAY);
	if (!willShowDate) {
		GetLocalTime(&st);
		wTodayYear = st.wYear;
		wTodayMonth = st.wMonth;
		wTodayDay = st.wDay;
	}
	ft.dwLowDateTime = (uint32_t)filetime;
	ft.dwHighDateTime = (uint32_t)(filetime >> 32);
	FILETIME localft;
	if (!FileTimeToLocalFileTime(&ft, &localft)) {
		// this should never happen
		wcsncpy(dateout, L"Incorrect FileTime", lendateout);
	}
	else {
		if (!FileTimeToSystemTime(&localft, &st)) {
			// this should never happen
			wcsncpy(dateout, L"Incorrect LocalFileTime", lendateout);
		}
		else {
			dateout[lendateout - 1] = 0;
			int templen = 0;
			if (!willShowDate) willShowDate = (wTodayYear != st.wYear) || (wTodayMonth != st.wMonth) || (wTodayDay != st.wDay);
			if (willShowDate) {
				templen = GetDateFormatW(localeID, (optDateTime & SHOWDATELONG) ? DATE_LONGDATE : DATE_SHORTDATE, &st, nullptr, dateout, lendateout - 2);
				dateout[templen - 1] = ' ';
			}
			if (templen < (lendateout - 1)) {
				GetTimeFormatW(localeID, (optDateTime & SHOWDATENOSECONDS) ? TIME_NOSECONDS : 0, &st, nullptr, &dateout[templen], lendateout - templen - 1);
			}
		}
	}
}

void MimeDateToLocalizedDateTime(char *datein, wchar_t *dateout, int lendateout)
{
	ULONGLONG ft = MimeDateToFileTime(datein);
	FileTimeToLocalizedDateTime(ft, dateout, lendateout);
}

int CALLBACK ListViewCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	if (lParam1 == NULL || lParam2 == NULL)
		return 0;

	int					nResult = 0;
	char *str1;
	char *str2;
	HYAMNMAIL			email1 = (HYAMNMAIL)lParam1;
	HYAMNMAIL			email2 = (HYAMNMAIL)lParam2;
	struct CShortHeader	Header1;
	struct CShortHeader	Header2;
	memset(&Header1, 0, sizeof(Header1));
	memset(&Header2, 0, sizeof(Header2));

	try {
		ExtractShortHeader(email1->MailData->TranslatedHeader, &Header1);
		ExtractShortHeader(email2->MailData->TranslatedHeader, &Header2);

		switch ((int)lParamSort) {
		case 0:	//From
			if (Header1.FromNick == nullptr)
				str1 = Header1.From;
			else str1 = Header1.FromNick;

			if (Header2.FromNick == nullptr)
				str2 = Header2.From;
			else str2 = Header2.FromNick;

			nResult = mir_strcmp(str1, str2);

			if (bFrom) nResult = -nResult;
			break;
		case 1:	//Subject
			if (Header1.Subject == nullptr)
				str1 = " ";
			else str1 = Header1.Subject;

			if (Header2.Subject == nullptr)
				str2 = " ";
			else str2 = Header2.Subject;

			nResult = mir_strcmp(str1, str2);

			if (bSub) nResult = -nResult;
			break;
		case 2:	//Size
			if (email1->MailData->Size == email2->MailData->Size)	nResult = 0;
			if (email1->MailData->Size > email2->MailData->Size)	nResult = 1;
			if (email1->MailData->Size < email2->MailData->Size)	nResult = -1;

			if (bSize) nResult = -nResult;
			break;

		case 3:	//Date
			{
				ULONGLONG ts1 = 0, ts2 = 0;
				ts1 = MimeDateToFileTime(Header1.Date);
				ts2 = MimeDateToFileTime(Header2.Date);
				if (ts1 > ts2) nResult = 1;
				else if (ts1 < ts2) nResult = -1;
				else nResult = 0;
			}
			if (bDate) nResult = -nResult;
			break;

		default:
			if (Header1.Subject == nullptr) str1 = " ";
			else str1 = Header1.Subject;

			if (Header2.Subject == nullptr) str2 = " ";
			else str2 = Header2.Subject;

			nResult = mir_strcmp(str1, str2);
			break;
		}
		//MessageBox(NULL,str1,str2,0);
	}
	catch (...) {
	}

	//free mem
	DeleteShortHeaderContent(&Header1);
	DeleteShortHeaderContent(&Header2);
	return nResult;

}

HCURSOR hCurSplitNS, hCurSplitWE;
#define DM_SPLITTERMOVED     (WM_USER+15)

static LRESULT CALLBACK SplitterSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_NCHITTEST:
		return HTCLIENT;

	case WM_SETCURSOR:
		SetCursor(hCurSplitNS);
		return TRUE;

	case WM_LBUTTONDOWN:
		SetCapture(hwnd);
		return 0;

	case WM_MOUSEMOVE:
		if (GetCapture() == hwnd) {
			RECT rc;
			GetClientRect(hwnd, &rc);
			SendMessage(GetParent(hwnd), DM_SPLITTERMOVED, (short)HIWORD(GetMessagePos()) + rc.bottom / 2, (LPARAM)hwnd);
		}
		return 0;

	case WM_LBUTTONUP:
		ReleaseCapture();
		return 0;
	}
	return mir_callNextSubclass(hwnd, SplitterSubclassProc, msg, wParam, lParam);
}

void ConvertCodedStringToUnicode(char *stream, wchar_t **storeto, uint32_t cp, int mode);
int ConvertStringToUnicode(char *stream, unsigned int cp, wchar_t **out);

INT_PTR CALLBACK DlgProcYAMNShowMessage(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		{
			PYAMN_MAILSHOWPARAM MailParam = (PYAMN_MAILSHOWPARAM)lParam;
			wchar_t *iHeaderW = nullptr;
			wchar_t *iValueW = nullptr;
			int StrLen;
			HWND hListView = GetDlgItem(hDlg, IDC_LISTHEADERS);
			mir_subclassWindow(GetDlgItem(hDlg, IDC_SPLITTER), SplitterSubclassProc);
			SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)MailParam);
			Window_SetIcon_IcoLib(hDlg, g_plugin.getIconHandle(IDI_NEWMAIL));

			ListView_SetUnicodeFormat(hListView, TRUE);
			ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT);

			StrLen = MultiByteToWideChar(CP_ACP, MB_USEGLYPHCHARS, Translate("Header"), -1, nullptr, 0);
			iHeaderW = new wchar_t[StrLen + 1];
			MultiByteToWideChar(CP_ACP, MB_USEGLYPHCHARS, Translate("Header"), -1, iHeaderW, StrLen);

			StrLen = MultiByteToWideChar(CP_ACP, MB_USEGLYPHCHARS, Translate("Value"), -1, nullptr, 0);
			iValueW = new wchar_t[StrLen + 1];
			MultiByteToWideChar(CP_ACP, MB_USEGLYPHCHARS, Translate("Value"), -1, iValueW, StrLen);

			LVCOLUMN lvc0 = {LVCF_FMT | LVCF_TEXT | LVCF_WIDTH, LVCFMT_LEFT, 130, iHeaderW, 0, 0};
			LVCOLUMN lvc1 = {LVCF_FMT | LVCF_TEXT | LVCF_WIDTH, LVCFMT_LEFT, 400, iValueW, 0, 0};
			SendMessage(hListView, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc0);
			SendMessage(hListView, LVM_INSERTCOLUMN, 1, (LPARAM)&lvc1);
			if (nullptr != iHeaderW)
				delete[] iHeaderW;
			if (nullptr != iValueW)
				delete[] iValueW;

			SendMessage(hDlg, WM_YAMN_CHANGECONTENT, 0, (LPARAM)MailParam);
			MoveWindow(hDlg, HeadPosX, HeadPosY, HeadSizeX, HeadSizeY, 0);
			ShowWindow(hDlg, SW_SHOWNORMAL);
		}
		break;

	case WM_YAMN_CHANGECONTENT:
		{
			PYAMN_MAILSHOWPARAM MailParam = (PYAMN_MAILSHOWPARAM)
				(lParam ? lParam : GetWindowLongPtr(hDlg, DWLP_USER));
			HWND hListView = GetDlgItem(hDlg, IDC_LISTHEADERS);
			HWND hEdit = GetDlgItem(hDlg, IDC_EDITBODY);
			//do not redraw
			SendMessage(hListView, WM_SETREDRAW, 0, 0);
			ListView_DeleteAllItems(hListView);
			struct CMimeItem *Header;
			LVITEMW item;
			item.mask = LVIF_TEXT | LVIF_PARAM;
			wchar_t *From = nullptr, *Subj = nullptr;
			char *contentType = nullptr, *transEncoding = nullptr, *body = nullptr; //should not be delete[]-ed
			for (Header = MailParam->mail->MailData->TranslatedHeader; Header != nullptr; Header = Header->Next) {
				wchar_t *str1 = nullptr;
				wchar_t *str2 = nullptr;
				wchar_t str_nul[2] = {0};
				if (!body) if (!_stricmp(Header->name, "Body")) { body = Header->value; continue; }
				if (!contentType) if (!_stricmp(Header->name, "Content-Type")) contentType = Header->value;
				if (!transEncoding) if (!_stricmp(Header->name, "Content-Transfer-Encoding")) transEncoding = Header->value;
				//ConvertCodedStringToUnicode(Header->name,&str1,MailParam->mail->MailData->CP,1); 
				{
					int streamsize = MultiByteToWideChar(20127, 0, Header->name, -1, nullptr, 0);
					str1 = (wchar_t *)malloc(sizeof(wchar_t) * (streamsize + 1));
					MultiByteToWideChar(20127, 0, Header->name, -1, str1, streamsize);//US-ASCII
				}
				ConvertCodedStringToUnicode(Header->value, &str2, MailParam->mail->MailData->CP, 1);
				if (!str2) { str2 = (wchar_t *)str_nul; }// the header value may be NULL
				if (!From) if (!_stricmp(Header->name, "From")) {
					From = new wchar_t[mir_wstrlen(str2) + 1];
					mir_wstrcpy(From, str2);
				}
				if (!Subj) if (!_stricmp(Header->name, "Subject")) {
					Subj = new wchar_t[mir_wstrlen(str2) + 1];
					mir_wstrcpy(Subj, str2);
				}
				//if (!hasBody) if (!mir_strcmp(Header->name,"Body")) hasBody = true;
				int count = 0; wchar_t **split = nullptr;
				int ofs = 0;
				while (str2[ofs]) {
					if ((str2[ofs] == 0x266A) || (str2[ofs] == 0x25D9) || (str2[ofs] == 0x25CB) ||
						(str2[ofs] == 0x09) || (str2[ofs] == 0x0A) || (str2[ofs] == 0x0D))count++;
					ofs++;
				}
				split = new wchar_t *[count + 1];
				count = 0; ofs = 0;
				split[0] = str2;
				while (str2[ofs]) {
					if ((str2[ofs] == 0x266A) || (str2[ofs] == 0x25D9) || (str2[ofs] == 0x25CB) ||
						(str2[ofs] == 0x09) || (str2[ofs] == 0x0A) || (str2[ofs] == 0x0D)) {
						if (str2[ofs - 1]) {
							count++;
						}
						split[count] = (wchar_t *)(str2 + ofs + 1);
						str2[ofs] = 0;
					}
					ofs++;
				};

				if (!_stricmp(Header->name, "From") || !_stricmp(Header->name, "To") || !_stricmp(Header->name, "Date") || !_stricmp(Header->name, "Subject"))
					item.iItem = 0;
				else
					item.iItem = 999;
				for (int i = 0; i <= count; i++) {
					item.iSubItem = 0;
					if (i == 0)
						item.pszText = str1;
					else {
						item.iItem++;
						item.pszText = nullptr;
					}
					item.iItem = SendMessage(hListView, LVM_INSERTITEM, 0, (LPARAM)&item);
					item.iSubItem = 1;
					item.pszText = str2 ? split[i] : nullptr;
					SendMessage(hListView, LVM_SETITEMTEXT, (WPARAM)item.iItem, (LPARAM)&item);
				}
				delete[] split;

				if (str1)
					free(str1);
				if (str2 != (wchar_t *)str_nul)
					free(str2);
			}
			if (body) {
				wchar_t *bodyDecoded = nullptr;
				char *localBody = nullptr;
				if (contentType) {
					if (!_strnicmp(contentType, "text", 4)) {
						if (transEncoding) {
							if (!_stricmp(transEncoding, "base64")) {
								int size = (int)mir_strlen(body) * 3 / 4 + 5;
								localBody = new char[size + 1];
								DecodeBase64(body, localBody, size);
							}
							else if (!_stricmp(transEncoding, "quoted-printable")) {
								int size = (int)mir_strlen(body) + 2;
								localBody = new char[size + 1];
								DecodeQuotedPrintable(body, localBody, size, FALSE);
							}
						}
					}
					else if (!_strnicmp(contentType, "multipart/", 10)) {
						char *bondary = nullptr;
						if (nullptr != (bondary = ExtractFromContentType(contentType, "boundary="))) {
							bodyDecoded = ParseMultipartBody(body, bondary);
							delete[] bondary;
						}
					}
				}
				if (!bodyDecoded)ConvertStringToUnicode(localBody ? localBody : body, MailParam->mail->MailData->CP, &bodyDecoded);
				SetWindowTextW(hEdit, bodyDecoded);
				delete[] bodyDecoded;
				if (localBody) delete[] localBody;
				SetFocus(hEdit);
			}
			if (!(MailParam->mail->Flags & YAMN_MSG_BODYRECEIVED)) {
				MailParam->mail->Flags |= YAMN_MSG_BODYREQUESTED;
				CallService(MS_YAMN_ACCOUNTCHECK, (WPARAM)MailParam->account, 0);
			}
			else {
				if (MailParam->mail->Flags & YAMN_MSG_UNSEEN) {
					MailParam->mail->Flags &= ~YAMN_MSG_UNSEEN; //mark the message as seen
					HWND hMailBrowser = WindowList_Find(YAMNVar.NewMailAccountWnd, (UINT_PTR)MailParam->account);
					if (hMailBrowser) {
						struct CChangeContent Params = {MailParam->account->NewMailN.Flags | YAMN_ACC_MSGP, MailParam->account->NoNewMailN.Flags | YAMN_ACC_MSGP};
						SendMessage(hMailBrowser, WM_YAMN_CHANGECONTENT, (WPARAM)MailParam->account, (LPARAM)&Params);
					}
					else UpdateMails(nullptr, MailParam->account, MailParam->account->NewMailN.Flags, MailParam->account->NoNewMailN.Flags);
				}
			}
			ShowWindow(GetDlgItem(hDlg, IDC_SPLITTER), (MailParam->mail->Flags & YAMN_MSG_BODYRECEIVED) ? SW_SHOW : SW_HIDE);
			ShowWindow(hEdit, (MailParam->mail->Flags & YAMN_MSG_BODYRECEIVED) ? SW_SHOW : SW_HIDE);
			wchar_t *title = nullptr;
			size_t size = (From ? mir_wstrlen(From) : 0) + (Subj ? mir_wstrlen(Subj) : 0) + 4;
			title = new wchar_t[size];
			if (From && Subj)
				mir_snwprintf(title, size, L"%s (%s)", Subj, From);
			else if (From)
				wcsncpy_s(title, size, From, _TRUNCATE);
			else if (Subj)
				wcsncpy_s(title, size, Subj, _TRUNCATE);
			else
				wcsncpy_s(title, size, L"none", _TRUNCATE);
			if (Subj) delete[] Subj;
			if (From) delete[] From;
			SetWindowTextW(hDlg, title);
			delete[] title;
			// turn on redrawing
			SendMessage(hListView, WM_SETREDRAW, 1, 0);
			SendMessage(hDlg, WM_SIZE, 0, HeadSizeY << 16 | HeadSizeX);
		} break;

	case WM_YAMN_STOPACCOUNT:
		{
			PYAMN_MAILSHOWPARAM MailParam = (PYAMN_MAILSHOWPARAM)
				(lParam ? lParam : GetWindowLongPtr(hDlg, DWLP_USER));

			if (nullptr == MailParam)
				break;
			if ((CAccount *)wParam != MailParam->account)
				break;

			DestroyWindow(hDlg);
		}
		return 1;

	case WM_CTLCOLORSTATIC:
		// here should be check if this is our edittext control. 
		// but we have only one static control (for now);
		SetBkColor((HDC)wParam, GetSysColor(COLOR_WINDOW));
		SetTextColor((HDC)wParam, GetSysColor(COLOR_WINDOWTEXT));
		return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);

	case WM_DESTROY:
		Window_FreeIcon_IcoLib(hDlg);
		{
			RECT coord;
			if (GetWindowRect(hDlg, &coord)) {
				HeadPosX = coord.left;
				HeadSizeX = coord.right - coord.left;
				HeadPosY = coord.top;
				HeadSizeY = coord.bottom - coord.top;
			}

			PostQuitMessage(1);
		}
		break;

	case WM_SYSCOMMAND:
		switch (wParam) {
		case SC_CLOSE:
			DestroyWindow(hDlg);
			break;
		}
		break;

	case WM_MOVE:
		HeadPosX = LOWORD(lParam);	//((LPRECT)lParam)->right-((LPRECT)lParam)->left;
		HeadPosY = HIWORD(lParam);	//((LPRECT)lParam)->bottom-((LPRECT)lParam)->top;
		return 0;

	case DM_SPLITTERMOVED:
		if ((HWND)lParam == GetDlgItem(hDlg, IDC_SPLITTER)) {
			POINT pt;
			pt.x = 0;
			pt.y = wParam;
			ScreenToClient(hDlg, &pt);
			HeadSplitPos = (pt.y * 1000) / HeadSizeY;//+rc.bottom-rc.top;
			if (HeadSplitPos >= 1000) HeadSplitPos = 999;
			else if (HeadSplitPos <= 0) HeadSplitPos = 1;
			else SendMessage(hDlg, WM_SIZE, 0, HeadSizeY << 16 | HeadSizeX);
		}
		return 0;

	case WM_SIZE:
		if (wParam == SIZE_RESTORED) {
			HWND hList = GetDlgItem(hDlg, IDC_LISTHEADERS);
			HWND hEdit = GetDlgItem(hDlg, IDC_EDITBODY);
			BOOL isBodyShown = ((PYAMN_MAILSHOWPARAM)(GetWindowLongPtr(hDlg, DWLP_USER)))->mail->Flags & YAMN_MSG_BODYRECEIVED;
			HeadSizeX = LOWORD(lParam);	//((LPRECT)lParam)->right-((LPRECT)lParam)->left;
			HeadSizeY = HIWORD(lParam);	//((LPRECT)lParam)->bottom-((LPRECT)lParam)->top;
			int localSplitPos = (HeadSplitPos * HeadSizeY) / 1000;
			int localSizeX;
			RECT coord;
			MoveWindow(GetDlgItem(hDlg, IDC_SPLITTER), 5, localSplitPos, HeadSizeX - 10, 2, TRUE);
			MoveWindow(hEdit, 5, localSplitPos + 6, HeadSizeX - 10, HeadSizeY - localSplitPos - 11, TRUE);	//where to put text window while resizing
			MoveWindow(hList, 5, 5, HeadSizeX - 10, (isBodyShown ? localSplitPos : HeadSizeY) - 10, TRUE);	//where to put headers list window while resizing
			//if (changeX) {
			if (GetClientRect(hList, &coord)) {
				localSizeX = coord.right - coord.left;
			}
			else localSizeX = HeadSizeX;
			LONG iNameWidth = ListView_GetColumnWidth(hList, 0);
			ListView_SetColumnWidth(hList, 1, (localSizeX <= iNameWidth) ? 0 : (localSizeX - iNameWidth));
			//}
		}
		return 0;

	case WM_CONTEXTMENU:
		if (GetWindowLongPtr((HWND)wParam, GWLP_ID) == IDC_LISTHEADERS) {
			//MessageBox(0,"LISTHEADERS","Debug",0);
			HWND hList = GetDlgItem(hDlg, IDC_LISTHEADERS);
			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			if (pt.x == -1) pt.x = 0;
			if (pt.y == -1) pt.y = 0;
			if (int numRows = ListView_GetItemCount(hList)) {
				HMENU hMenu = CreatePopupMenu();
				AppendMenu(hMenu, MF_STRING, (UINT_PTR)1, TranslateT("Copy Selected"));
				AppendMenu(hMenu, MF_STRING, (UINT_PTR)2, TranslateT("Copy All"));
				AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
				AppendMenu(hMenu, MF_STRING, (UINT_PTR)0, TranslateT("Cancel"));
				int nReturnCmd = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hDlg, nullptr);
				DestroyMenu(hMenu);
				if (nReturnCmd > 0) {
					int courRow = 0;
					size_t sizeNeeded = 0;
					wchar_t headname[64] = {0}, headvalue[256] = {0};
					for (courRow = 0; courRow < numRows; courRow++) {
						if ((nReturnCmd == 1) && (ListView_GetItemState(hList, courRow, LVIS_SELECTED) == 0)) continue;
						ListView_GetItemText(hList, courRow, 0, headname, _countof(headname));
						ListView_GetItemText(hList, courRow, 1, headvalue, _countof(headvalue));
						size_t headnamelen = mir_wstrlen(headname);
						if (headnamelen) sizeNeeded += 1 + headnamelen;
						sizeNeeded += 3 + mir_wstrlen(headvalue);
					}
					if (sizeNeeded && OpenClipboard(hDlg)) {
						EmptyClipboard();
						HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE, (sizeNeeded + 1) * sizeof(wchar_t));
						wchar_t *buff = (wchar_t *)GlobalLock(hData);
						int courPos = 0;
						for (courRow = 0; courRow < numRows; courRow++) {
							if ((nReturnCmd == 1) && (ListView_GetItemState(hList, courRow, LVIS_SELECTED) == 0)) continue;
							ListView_GetItemText(hList, courRow, 0, headname, _countof(headname));
							ListView_GetItemText(hList, courRow, 1, headvalue, _countof(headvalue));
							if (mir_wstrlen(headname)) courPos += mir_snwprintf(&buff[courPos], sizeNeeded + 1, L"%s:\t%s\r\n", headname, headvalue);
							else courPos += mir_snwprintf(&buff[courPos], sizeNeeded + 1, L"\t%s\r\n", headvalue);
						}
						GlobalUnlock(hData);

						SetClipboardData(CF_UNICODETEXT, hData);

						CloseClipboard();
					}
				}
			}
		}
		break; // just in case
	}
	return 0;
}

void __cdecl ShowEmailThread(void *Param)
{
	struct MailShowMsgWinParam MyParam = *(struct MailShowMsgWinParam *)Param;

	SCIncFcn(MyParam.account->UsingThreads);

	if (MyParam.mail->MsgWindow) {
		//if (!BringWindowToTop(MyParam.mail->MsgWindow)) {
		if (!SetForegroundWindow(MyParam.mail->MsgWindow)) {
			SendMessage(MyParam.mail->MsgWindow, WM_DESTROY, 0, 0);
			MyParam.mail->MsgWindow = nullptr;
			goto CREADTEVIEWMESSAGEWINDOW;
		}

		if (IsIconic(MyParam.mail->MsgWindow))
			OpenIcon(MyParam.mail->MsgWindow);
	}
	else {
CREADTEVIEWMESSAGEWINDOW:
		MyParam.mail->MsgWindow = CreateDialogParamW(g_plugin.getInst(), MAKEINTRESOURCEW(IDD_DLGSHOWMESSAGE), nullptr, DlgProcYAMNShowMessage, (LPARAM)&MyParam);
		WindowList_Add(YAMNVar.MessageWnds, MyParam.mail->MsgWindow);
		MSG msg;
		while (GetMessage(&msg, nullptr, 0, 0)) {
			if (MyParam.mail->MsgWindow == nullptr || !IsDialogMessage(MyParam.mail->MsgWindow, &msg)) { /* Wine fix. */
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		WindowList_Remove(YAMNVar.MessageWnds, MyParam.mail->MsgWindow);
		MyParam.mail->MsgWindow = nullptr;
	}

	SCDecFcn(MyParam.account->UsingThreads);
	delete (struct MailShowMsgWinParam *)Param;
}

INT_PTR CALLBACK DlgProcYAMNMailBrowser(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CAccount *ActualAccount;
	int Items;

	switch (msg) {
	case WM_INITDIALOG:
		{
			struct MailBrowserWinParam *MyParam = (struct MailBrowserWinParam *)lParam;

			ListView_SetUnicodeFormat(GetDlgItem(hDlg, IDC_LISTMAILS), TRUE);
			ListView_SetExtendedListViewStyle(GetDlgItem(hDlg, IDC_LISTMAILS), LVS_EX_FULLROWSELECT);

			ActualAccount = MyParam->account;
			struct CMailWinUserInfo *mwui = new struct CMailWinUserInfo;
			mwui->Account = ActualAccount;
			mwui->TrayIconState = 0;
			mwui->UpdateMailsMessagesAccess = FALSE;
			mwui->Seen = FALSE;
			mwui->RunFirstTime = TRUE;

			SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)mwui);
			if (WAIT_OBJECT_0 != WaitToReadFcn(ActualAccount->AccountAccessSO)) {
				DestroyWindow(hDlg);
				return FALSE;
			}

			SetDlgItemText(hDlg, IDC_BTNAPP, TranslateT("Run application"));
			SetDlgItemText(hDlg, IDC_BTNDEL, TranslateT("Delete selected"));
			SetDlgItemText(hDlg, IDC_BTNCHECKALL, TranslateT("Select All"));
			SetDlgItemText(hDlg, IDC_BTNOK, TranslateT("OK"));

			LVCOLUMN lvc0 = {LVCF_FMT | LVCF_TEXT | LVCF_WIDTH, LVCFMT_LEFT, FromWidth, TranslateT("From"), 0, 0};
			LVCOLUMN lvc1 = {LVCF_FMT | LVCF_TEXT | LVCF_WIDTH, LVCFMT_LEFT, SubjectWidth, TranslateT("Subject"), 0, 0};
			LVCOLUMN lvc2 = {LVCF_FMT | LVCF_TEXT | LVCF_WIDTH, LVCFMT_LEFT, SizeWidth, TranslateT("Size"), 0, 0};
			LVCOLUMN lvc3 = {LVCF_FMT | LVCF_TEXT | LVCF_WIDTH, LVCFMT_LEFT, SizeDate, TranslateT("Date"), 0, 0};
			SendDlgItemMessage(hDlg, IDC_LISTMAILS, LVM_INSERTCOLUMN, 0, (LPARAM)&lvc0);
			SendDlgItemMessage(hDlg, IDC_LISTMAILS, LVM_INSERTCOLUMN, 1, (LPARAM)&lvc1);
			SendDlgItemMessage(hDlg, IDC_LISTMAILS, LVM_INSERTCOLUMN, (WPARAM)2, (LPARAM)&lvc2);
			SendDlgItemMessage(hDlg, IDC_LISTMAILS, LVM_INSERTCOLUMN, (WPARAM)3, (LPARAM)&lvc3);

			if ((ActualAccount->NewMailN.App != nullptr) && (mir_wstrlen(ActualAccount->NewMailN.App)))
				EnableWindow(GetDlgItem(hDlg, IDC_BTNAPP), TRUE);
			else
				EnableWindow(GetDlgItem(hDlg, IDC_BTNAPP), FALSE);

			ReadDoneFcn(ActualAccount->AccountAccessSO);

			WindowList_Add(YAMNVar.MessageWnds, hDlg);
			WindowList_Add(YAMNVar.NewMailAccountWnd, hDlg, (UINT_PTR)ActualAccount);

			{
				wchar_t accstatus[512];
				GetStatusFcn(ActualAccount, accstatus);
				SetDlgItemText(hDlg, IDC_STSTATUS, accstatus);
			}
			SetTimer(hDlg, TIMER_FLASHING, 500, nullptr);

			if (ActualAccount->hContact != NULL)
				g_clistApi.pfnRemoveEvent(ActualAccount->hContact, (LPARAM)"yamn new mail message");

			mir_subclassWindow(GetDlgItem(hDlg, IDC_LISTMAILS), ListViewSubclassProc);
		}
		break;

	case WM_DESTROY:
		{
			RECT coord;
			LVCOLUMN ColInfo;
			HYAMNMAIL Parser;

			Window_FreeIcon_IcoLib(hDlg);

			struct CMailWinUserInfo *mwui = (struct CMailWinUserInfo *)GetWindowLongPtr(hDlg, DWLP_USER);
			if (nullptr == (ActualAccount = GetWindowAccount(hDlg)))
				break;
			ColInfo.mask = LVCF_WIDTH;
			if (ListView_GetColumn(GetDlgItem(hDlg, IDC_LISTMAILS), 0, &ColInfo))
				FromWidth = ColInfo.cx;
			if (ListView_GetColumn(GetDlgItem(hDlg, IDC_LISTMAILS), 1, &ColInfo))
				SubjectWidth = ColInfo.cx;
			if (ListView_GetColumn(GetDlgItem(hDlg, IDC_LISTMAILS), 2, &ColInfo))
				SizeWidth = ColInfo.cx;
			if (ListView_GetColumn(GetDlgItem(hDlg, IDC_LISTMAILS), 3, &ColInfo))
				SizeDate = ColInfo.cx;

			if (!YAMNVar.Shutdown && GetWindowRect(hDlg, &coord))	//the YAMNVar.Shutdown testing is because M<iranda strange functionality at shutdown phase, when call to DBWriteContactSetting freezes calling thread
			{
				PosX = coord.left;
				SizeX = coord.right - coord.left;
				PosY = coord.top;
				SizeY = coord.bottom - coord.top;
				g_plugin.setDword(YAMN_DBPOSX, PosX);
				g_plugin.setDword(YAMN_DBPOSY, PosY);
				g_plugin.setDword(YAMN_DBSIZEX, SizeX);
				g_plugin.setDword(YAMN_DBSIZEY, SizeY);
			}
			KillTimer(hDlg, TIMER_FLASHING);

			WindowList_Remove(YAMNVar.NewMailAccountWnd, hDlg);
			WindowList_Remove(YAMNVar.MessageWnds, hDlg);

			if (WAIT_OBJECT_0 != WaitToWriteFcn(ActualAccount->MessagesAccessSO))
				break;

			//delete mails from queue, which are deleted from server (spam level 3 mails e.g.)
			for (Parser = (HYAMNMAIL)ActualAccount->Mails; Parser != nullptr; Parser = Parser->Next) {
				if ((Parser->Flags & YAMN_MSG_DELETED) && YAMN_MSG_SPAML(Parser->Flags, YAMN_MSG_SPAML3) && mwui->Seen)		//if spaml3 was already deleted and user knows about it
				{
					DeleteMessageFromQueueFcn((HYAMNMAIL *)&ActualAccount->Mails, Parser, 1);
					CallService(MS_YAMN_DELETEACCOUNTMAIL, (WPARAM)ActualAccount->Plugin, (LPARAM)Parser);
				}
			}

			//mark mails as read (remove "new" and "unseen" flags)
			if (mwui->Seen)
				SetRemoveFlagsInQueueFcn((HYAMNMAIL)ActualAccount->Mails, YAMN_MSG_DISPLAY, 0, YAMN_MSG_NEW | YAMN_MSG_UNSEEN, 0);

			WriteDoneFcn(ActualAccount->MessagesAccessSO);

			NOTIFYICONDATA nid;
			memset(&nid, 0, sizeof(NOTIFYICONDATA));

			delete mwui;
			SetWindowLongPtr(hDlg, DWLP_USER, NULL);

			nid.cbSize = sizeof(NOTIFYICONDATA);
			nid.hWnd = hDlg;
			nid.uID = 0;
			Shell_NotifyIcon(NIM_DELETE, &nid);
			PostQuitMessage(0);
		}
		break;

	case WM_SHOWWINDOW:
		{
			struct CMailWinUserInfo *mwui = (struct CMailWinUserInfo *)GetWindowLongPtr(hDlg, DWLP_USER);

			if (mwui == nullptr)
				return 0;
			mwui->Seen = TRUE;
		}

	case WM_YAMN_CHANGESTATUS:
		if (nullptr == (ActualAccount = GetWindowAccount(hDlg)))
			break;

		if ((CAccount *)wParam != ActualAccount)
			break;

		wchar_t accstatus[512];
		GetStatusFcn(ActualAccount, accstatus);
		SetDlgItemText(hDlg, IDC_STSTATUS, accstatus);
		return 1;

	case WM_YAMN_CHANGECONTENT:
		{
			struct CUpdateMails UpdateParams;
			BOOL ThisThreadWindow = (GetCurrentThreadId() == GetWindowThreadProcessId(hDlg, nullptr));

			if (nullptr == (UpdateParams.Copied = CreateEvent(nullptr, FALSE, FALSE, nullptr))) {
				DestroyWindow(hDlg);
				return 0;
			}
			UpdateParams.Flags = (struct CChangeContent *)lParam;
			UpdateParams.Waiting = !ThisThreadWindow;

			if (ThisThreadWindow) {
				if (!UpdateMails(hDlg, (CAccount *)wParam, UpdateParams.Flags->nflags, UpdateParams.Flags->nnflags))
					DestroyWindow(hDlg);
			}
			else if (PostMessage(hDlg, WM_YAMN_UPDATEMAILS, wParam, (LPARAM)&UpdateParams))	//this ensures UpdateMails will execute the thread who created the browser window
			{
				if (!ThisThreadWindow)
					WaitForSingleObject(UpdateParams.Copied, INFINITE);
			}

			CloseHandle(UpdateParams.Copied);
		}
		return 1;
	case WM_YAMN_UPDATEMAILS:
		{
			struct CUpdateMails *um = (struct CUpdateMails *)lParam;
			uint32_t nflags, nnflags;

			if (nullptr == (ActualAccount = GetWindowAccount(hDlg)))
				return 0;
			if ((CAccount *)wParam != ActualAccount)
				return 0;

			nflags = um->Flags->nflags;
			nnflags = um->Flags->nnflags;

			if (um->Waiting)
				SetEvent(um->Copied);

			if (!UpdateMails(hDlg, ActualAccount, nflags, nnflags))
				DestroyWindow(hDlg);
		}
		return 1;
	case WM_YAMN_STOPACCOUNT:
		if (nullptr == (ActualAccount = GetWindowAccount(hDlg)))
			break;
		if ((CAccount *)wParam != ActualAccount)
			break;
		PostQuitMessage(0);
		return 1;

	case WM_YAMN_NOTIFYICON:
		if (nullptr == (ActualAccount = GetWindowAccount(hDlg)))
			break;

		switch (lParam) {
		case WM_LBUTTONDBLCLK:
			if (WAIT_OBJECT_0 != WaitToReadFcn(ActualAccount->AccountAccessSO)) {
				return 0;
			}

			if (ActualAccount->AbilityFlags & YAMN_ACC_BROWSE) {
				ShowWindow(hDlg, SW_SHOWNORMAL);
				SetForegroundWindow(hDlg);
			}
			else DestroyWindow(hDlg);

			ReadDoneFcn(ActualAccount->AccountAccessSO);
			break;
		}
		break;

	case WM_YAMN_SHOWSELECTED:
		{
			int iSelect = SendDlgItemMessage(hDlg, IDC_LISTMAILS, LVM_GETNEXTITEM, -1, MAKELPARAM((UINT)LVNI_FOCUSED, 0)); // return item selected
			if (iSelect != -1) {
				LV_ITEMW item;

				item.iItem = iSelect;
				item.iSubItem = 0;
				item.mask = LVIF_PARAM | LVIF_STATE;
				item.stateMask = 0xFFFFFFFF;
				ListView_GetItem(GetDlgItem(hDlg, IDC_LISTMAILS), &item);
				HYAMNMAIL ActualMail = (HYAMNMAIL)item.lParam;
				if (nullptr != ActualMail) {
					PYAMN_MAILSHOWPARAM MailParam = new YAMN_MAILSHOWPARAM;
					MailParam->account = GetWindowAccount(hDlg);
					MailParam->mail = ActualMail;
					mir_forkthread(ShowEmailThread, MailParam);
				}
			}
		}
		break;

	case WM_SYSCOMMAND:
		if (nullptr == (ActualAccount = GetWindowAccount(hDlg)))
			break;
		switch (wParam) {
		case SC_CLOSE:
			DestroyWindow(hDlg);
			break;
		}
		break;

	case WM_COMMAND:
		if (nullptr == (ActualAccount = GetWindowAccount(hDlg)))
			break;

		switch (LOWORD(wParam)) {
		case IDC_BTNCHECKALL:
			ListView_SetItemState(GetDlgItem(hDlg, IDC_LISTMAILS), -1, 0, LVIS_SELECTED); // deselect all items
			ListView_SetItemState(GetDlgItem(hDlg, IDC_LISTMAILS), -1, LVIS_SELECTED, LVIS_SELECTED);
			Items = ListView_GetItemCount(GetDlgItem(hDlg, IDC_LISTMAILS));
			ListView_RedrawItems(GetDlgItem(hDlg, IDC_LISTMAILS), 0, Items);
			UpdateWindow(GetDlgItem(hDlg, IDC_LISTMAILS));
			SetFocus(GetDlgItem(hDlg, IDC_LISTMAILS));
			break;

		case IDC_BTNOK:
			DestroyWindow(hDlg);
			break;

		case IDC_BTNAPP:
			{
				PROCESS_INFORMATION pi;
				STARTUPINFOW si;

				memset(&si, 0, sizeof(si));
				si.cb = sizeof(si);

				if (WAIT_OBJECT_0 == WaitToReadFcn(ActualAccount->AccountAccessSO)) {
					if (ActualAccount->NewMailN.App != nullptr) {
						wchar_t *Command;
						if (ActualAccount->NewMailN.AppParam != nullptr)
							Command = new wchar_t[mir_wstrlen(ActualAccount->NewMailN.App) + mir_wstrlen(ActualAccount->NewMailN.AppParam) + 6];
						else
							Command = new wchar_t[mir_wstrlen(ActualAccount->NewMailN.App) + 6];

						if (Command != nullptr) {
							mir_wstrcpy(Command, L"\"");
							mir_wstrcat(Command, ActualAccount->NewMailN.App);
							mir_wstrcat(Command, L"\" ");
							if (ActualAccount->NewMailN.AppParam != nullptr)
								mir_wstrcat(Command, ActualAccount->NewMailN.AppParam);
							CreateProcessW(nullptr, Command, nullptr, nullptr, FALSE, NORMAL_PRIORITY_CLASS, nullptr, nullptr, &si, &pi);
							delete[] Command;
						}
					}

					ReadDoneFcn(ActualAccount->AccountAccessSO);
				}

				if (!(GetKeyState(VK_SHIFT) & 0x8000) && !(GetKeyState(VK_CONTROL) & 0x8000))
					DestroyWindow(hDlg);
			}
			break;

		case IDC_BTNDEL:
			{
				HYAMNMAIL ActualMail;
				uint32_t Total = 0;

				//	we use event to signal, that running thread has all needed stack parameters copied
				HANDLE ThreadRunningEV = CreateEvent(nullptr, FALSE, FALSE, nullptr);
				if (ThreadRunningEV == nullptr)
					break;

				Items = ListView_GetItemCount(GetDlgItem(hDlg, IDC_LISTMAILS));

				LVITEM item;
				item.stateMask = 0xFFFFFFFF;

				if (WAIT_OBJECT_0 == WaitToWriteFcn(ActualAccount->MessagesAccessSO)) {
					for (int i = 0; i < Items; i++) {
						item.iItem = i;
						item.iSubItem = 0;
						item.mask = LVIF_PARAM | LVIF_STATE;
						item.stateMask = 0xFFFFFFFF;
						ListView_GetItem(GetDlgItem(hDlg, IDC_LISTMAILS), &item);
						ActualMail = (HYAMNMAIL)item.lParam;
						if (nullptr == ActualMail)
							break;
						if (item.state & LVIS_SELECTED) {
							ActualMail->Flags |= YAMN_MSG_USERDELETE;	//set to mail we are going to delete it
							Total++;
						}
					}

					// Enable write-access to mails
					WriteDoneFcn(ActualAccount->MessagesAccessSO);

					if (Total) {
						wchar_t DeleteMsg[1024];

						mir_snwprintf(DeleteMsg, TranslateT("Do you really want to delete %d selected mails?"), Total);
						if (IDOK == MessageBox(hDlg, DeleteMsg, TranslateT("Delete confirmation"), MB_OKCANCEL | MB_ICONWARNING)) {
							struct DeleteParam ParamToDeleteMails = {YAMN_DELETEVERSION, ThreadRunningEV, ActualAccount, nullptr};

							// Find if there's mail marked to delete, which was deleted before
							if (WAIT_OBJECT_0 == WaitToWriteFcn(ActualAccount->MessagesAccessSO)) {
								for (ActualMail = (HYAMNMAIL)ActualAccount->Mails; ActualMail != nullptr; ActualMail = ActualMail->Next) {
									if ((ActualMail->Flags & YAMN_MSG_DELETED) && ((ActualMail->Flags & YAMN_MSG_USERDELETE)))	//if selected mail was already deleted
									{
										DeleteMessageFromQueueFcn((HYAMNMAIL *)&ActualAccount->Mails, ActualMail, 1);
										CallService(MS_YAMN_DELETEACCOUNTMAIL, (WPARAM)ActualAccount->Plugin, (LPARAM)ActualMail);	//delete it from memory
										continue;
									}
								}
								// Set flag to marked mails that they can be deleted
								SetRemoveFlagsInQueueFcn((HYAMNMAIL)ActualAccount->Mails, YAMN_MSG_DISPLAY | YAMN_MSG_USERDELETE, 0, YAMN_MSG_DELETEOK, 1);
								// Create new thread which deletes marked mails.
								HANDLE NewThread = mir_forkthread(ActualAccount->Plugin->Fcn->DeleteMailsFcnPtr, &ParamToDeleteMails);
								if (NewThread != nullptr)
									WaitForSingleObject(ThreadRunningEV, INFINITE);

								// Enable write-access to mails
								WriteDoneFcn(ActualAccount->MessagesAccessSO);
							}
						}
						else //else mark messages that they are not to be deleted
							SetRemoveFlagsInQueueFcn((HYAMNMAIL)ActualAccount->Mails, YAMN_MSG_DISPLAY | YAMN_MSG_USERDELETE, 0, YAMN_MSG_USERDELETE, 0);
					}
				}
				CloseHandle(ThreadRunningEV);
				if (g_plugin.getByte(YAMN_CLOSEDELETE, 0))
					DestroyWindow(hDlg);
			}
			break;
		}
		break;

	case WM_SIZE:
		if (wParam == SIZE_RESTORED) {
			LONG x = LOWORD(lParam);	//((LPRECT)lParam)->right-((LPRECT)lParam)->left;
			LONG y = HIWORD(lParam);	//((LPRECT)lParam)->bottom-((LPRECT)lParam)->top;
			MoveWindow(GetDlgItem(hDlg, IDC_BTNDEL), 5, y - 5 - 25, (x - 20) / 3, 25, TRUE);	//where to put DELETE button while resizing
			MoveWindow(GetDlgItem(hDlg, IDC_BTNCHECKALL), 10 + (x - 20) / 3, y - 5 - 25, (x - 20) / 6, 25, TRUE);	//where to put CHECK ALL button while resizing				
			MoveWindow(GetDlgItem(hDlg, IDC_BTNAPP), 15 + (x - 20) / 3 + (x - 20) / 6, y - 5 - 25, (x - 20) / 3, 25, TRUE);	//where to put RUN APP button while resizing
			MoveWindow(GetDlgItem(hDlg, IDC_BTNOK), 20 + 2 * (x - 20) / 3 + (x - 20) / 6, y - 5 - 25, (x - 20) / 6, 25, TRUE);	//where to put OK button while resizing
			MoveWindow(GetDlgItem(hDlg, IDC_LISTMAILS), 5, 5, x - 10, y - 55, TRUE);	//where to put list mail window while resizing
			MoveWindow(GetDlgItem(hDlg, IDC_STSTATUS), 5, y - 5 - 45, x - 10, 15, TRUE);	//where to put account status text while resizing
		}
		return 0;

	case WM_GETMINMAXINFO:
		((LPMINMAXINFO)lParam)->ptMinTrackSize.x = MAILBROWSER_MINXSIZE;
		((LPMINMAXINFO)lParam)->ptMinTrackSize.y = MAILBROWSER_MINYSIZE;
		return 0;

	case WM_TIMER:
		{
			NOTIFYICONDATA nid;
			struct CMailWinUserInfo *mwui = (struct CMailWinUserInfo *)GetWindowLongPtr(hDlg, DWLP_USER);

			memset(&nid, 0, sizeof(nid));
			nid.cbSize = sizeof(NOTIFYICONDATA);
			nid.hWnd = hDlg;
			nid.uID = 0;
			nid.uFlags = NIF_ICON;
			if (mwui->TrayIconState == 0)
				nid.hIcon = g_plugin.getIcon(IDI_CHECKMAIL);
			else
				nid.hIcon = g_plugin.getIcon(IDI_NEWMAIL);
			Shell_NotifyIcon(NIM_MODIFY, &nid);
			mwui->TrayIconState = !mwui->TrayIconState;
			//			UpdateWindow(hDlg);
		}
		break;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->idFrom) {
		case IDC_LISTMAILS:
			switch (((LPNMHDR)lParam)->code) {
			case NM_DBLCLK:
				SendMessage(hDlg, WM_YAMN_SHOWSELECTED, 0, 0);
				break;

			case LVN_COLUMNCLICK:
				if (nullptr != (ActualAccount = GetWindowAccount(hDlg))) {
					NM_LISTVIEW *pNMListView = (NM_LISTVIEW *)lParam;
					if (WAIT_OBJECT_0 == WaitToReadFcn(ActualAccount->AccountAccessSO)) {
						switch ((int)pNMListView->iSubItem) {
						case 0:
							bFrom = !bFrom;
							break;
						case 1:
							bSub = !bSub;
							break;
						case 2:
							bSize = !bSize;
							break;
						case 3:
							bDate = !bDate;
							break;
						default:
							break;
						}
						ListView_SortItems(pNMListView->hdr.hwndFrom, ListViewCompareProc, pNMListView->iSubItem);
						ReadDoneFcn(ActualAccount->AccountAccessSO);
					}
				}
				break;

			case NM_CUSTOMDRAW:
				{
					LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)lParam;
					LONG_PTR PaintCode;

					if (nullptr == (ActualAccount = GetWindowAccount(hDlg)))
						break;

					switch (cd->nmcd.dwDrawStage) {
					case CDDS_PREPAINT:
						PaintCode = CDRF_NOTIFYITEMDRAW;
						break;
					case CDDS_ITEMPREPAINT:
						PaintCode = CDRF_NOTIFYSUBITEMDRAW;
						break;
					case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
						{
							BOOL umma;
							{
								struct CMailWinUserInfo *mwui = (struct CMailWinUserInfo *)GetWindowLongPtr(hDlg, DWLP_USER);
								umma = mwui->UpdateMailsMessagesAccess;
							}
							HYAMNMAIL ActualMail = (HYAMNMAIL)cd->nmcd.lItemlParam;
							if (!ActualMail)
								ActualMail = (HYAMNMAIL)readItemLParam(cd->nmcd.hdr.hwndFrom, cd->nmcd.dwItemSpec);

							if (!umma)
								if (WAIT_OBJECT_0 != WaitToReadFcn(ActualAccount->MessagesAccessSO))
									return 0;

							switch (ActualMail->Flags & YAMN_MSG_SPAMMASK) {
							case YAMN_MSG_SPAML1:
							case YAMN_MSG_SPAML2:
								cd->clrText = RGB(150, 150, 150);
								break;
							case YAMN_MSG_SPAML3:
								cd->clrText = RGB(200, 200, 200);
								cd->clrTextBk = RGB(160, 160, 160);
								break;
							case 0:
								if (cd->nmcd.dwItemSpec & 1)
									cd->clrTextBk = RGB(230, 230, 230);
								break;
							default:
								break;
							}
							if (ActualMail->Flags & YAMN_MSG_UNSEEN)
								cd->clrTextBk = RGB(220, 235, 250);
							PaintCode = CDRF_DODEFAULT;

							if (!umma)
								ReadDoneFcn(ActualAccount->MessagesAccessSO);
							break;
						}
					default:
						PaintCode = 0;
					}
					SetWindowLongPtr(hDlg, DWLP_MSGRESULT, PaintCode);
					return 1;
				}
			}
		}
		break;

	case WM_CONTEXTMENU:
		if (GetWindowLongPtr((HWND)wParam, GWLP_ID) == IDC_LISTMAILS) {
			//MessageBox(0,"LISTHEADERS","Debug",0);
			HWND hList = GetDlgItem(hDlg, IDC_LISTMAILS);
			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			if (pt.x == -1) pt.x = 0;
			if (pt.y == -1) pt.y = 0;
			if (int numRows = ListView_GetItemCount(hList)) {
				HMENU hMenu = CreatePopupMenu();
				AppendMenu(hMenu, MF_STRING, (UINT_PTR)1, TranslateT("Copy Selected"));
				AppendMenu(hMenu, MF_STRING, (UINT_PTR)2, TranslateT("Copy All"));
				AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
				AppendMenu(hMenu, MF_STRING, (UINT_PTR)0, TranslateT("Cancel"));
				int nReturnCmd = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, hDlg, nullptr);
				DestroyMenu(hMenu);
				if (nReturnCmd > 0) {
					int courRow = 0;
					size_t sizeNeeded = 0;
					wchar_t from[128] = {0}, subject[256] = {0}, size[16] = {0}, date[64] = {0};
					for (courRow = 0; courRow < numRows; courRow++) {
						if ((nReturnCmd == 1) && (ListView_GetItemState(hList, courRow, LVIS_SELECTED) == 0)) continue;
						ListView_GetItemText(hList, courRow, 0, from, _countof(from));
						ListView_GetItemText(hList, courRow, 1, subject, _countof(subject));
						ListView_GetItemText(hList, courRow, 2, size, _countof(size));
						ListView_GetItemText(hList, courRow, 3, date, _countof(date));
						sizeNeeded += 5 + mir_wstrlen(from) + mir_wstrlen(subject) + mir_wstrlen(size) + mir_wstrlen(date);
					}
					if (sizeNeeded && OpenClipboard(hDlg)) {
						EmptyClipboard();
						HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE, (sizeNeeded + 1) * sizeof(wchar_t));
						wchar_t *buff = (wchar_t *)GlobalLock(hData);
						int courPos = 0;
						for (courRow = 0; courRow < numRows; courRow++) {
							if ((nReturnCmd == 1) && (ListView_GetItemState(hList, courRow, LVIS_SELECTED) == 0)) continue;
							ListView_GetItemText(hList, courRow, 0, from, _countof(from));
							ListView_GetItemText(hList, courRow, 1, subject, _countof(subject));
							ListView_GetItemText(hList, courRow, 2, size, _countof(size));
							ListView_GetItemText(hList, courRow, 3, date, _countof(date));
							courPos += mir_snwprintf(&buff[courPos], sizeNeeded + 1, L"%s\t%s\t%s\t%s\r\n", from, subject, size, date);
						}
						GlobalUnlock(hData);

						SetClipboardData(CF_UNICODETEXT, hData);

						CloseClipboard();
					}
				}
			}
		}
		break; // just in case
	}
	return 0;
}

LRESULT CALLBACK ListViewSubclassProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndParent = GetParent(hDlg);

	switch (msg) {
	case WM_GETDLGCODE:
		{
			LPMSG lpmsg = (LPMSG)lParam;
			if (lpmsg != nullptr) {
				if (lpmsg->message == WM_KEYDOWN
					&& lpmsg->wParam == VK_RETURN)
					return DLGC_WANTALLKEYS;
			}
		}
		break;

	case WM_KEYDOWN:
		{
			BOOL isCtrl = GetKeyState(VK_CONTROL) & 0x8000;
			BOOL isShift = GetKeyState(VK_SHIFT) & 0x8000;
			BOOL isAlt = GetKeyState(VK_MENU) & 0x8000;

			switch (wParam) {
			case 'A':  // ctrl-a
				if (!isAlt && !isShift && isCtrl) SendMessage(hwndParent, WM_COMMAND, IDC_BTNCHECKALL, 0);
				break;
			case VK_RETURN:
			case VK_SPACE:
				if (!isAlt && !isShift && !isCtrl) SendMessage(hwndParent, WM_YAMN_SHOWSELECTED, 0, 0);
				break;
			case VK_DELETE:
				SendMessage(hwndParent, WM_COMMAND, IDC_BTNDEL, 0);
				break;
			}
		}
		break;
	}
	return mir_callNextSubclass(hDlg, ListViewSubclassProc, msg, wParam, lParam);
}

void __cdecl MailBrowser(void *Param)
{
	MSG msg;

	HWND hMailBrowser;
	BOOL WndFound = FALSE;

	struct MailBrowserWinParam MyParam = *(struct MailBrowserWinParam *)Param;
	CAccount *ActualAccount = MyParam.account;
	SCIncFcn(ActualAccount->UsingThreads);

	//	we will not use params in stack anymore
	SetEvent(MyParam.ThreadRunningEV);

	__try {
		if (WAIT_OBJECT_0 != WaitToReadFcn(ActualAccount->AccountAccessSO))
			return;

		if (!(ActualAccount->AbilityFlags & YAMN_ACC_BROWSE)) {
			MyParam.nflags = MyParam.nflags & ~YAMN_ACC_MSG;
			MyParam.nnflags = MyParam.nnflags & ~YAMN_ACC_MSG;
		}

		if (!(ActualAccount->AbilityFlags & YAMN_ACC_POPUP))
			MyParam.nflags = MyParam.nflags & ~YAMN_ACC_POP;

		ReadDoneFcn(ActualAccount->AccountAccessSO);

		if (nullptr != (hMailBrowser = WindowList_Find(YAMNVar.NewMailAccountWnd, (UINT_PTR)ActualAccount)))
			WndFound = TRUE;

		if ((hMailBrowser == nullptr) && ((MyParam.nflags & YAMN_ACC_MSG) || (MyParam.nflags & YAMN_ACC_ICO) || (MyParam.nnflags & YAMN_ACC_MSG))) {
			hMailBrowser = CreateDialogParamW(g_plugin.getInst(), MAKEINTRESOURCEW(IDD_DLGVIEWMESSAGES), nullptr, DlgProcYAMNMailBrowser, (LPARAM)&MyParam);
			Window_SetIcon_IcoLib(hMailBrowser, g_plugin.getIconHandle(IDI_NEWMAIL));
			MoveWindow(hMailBrowser, PosX, PosY, SizeX, SizeY, TRUE);
		}

		if (hMailBrowser != nullptr) {
			struct CChangeContent Params = {MyParam.nflags, MyParam.nnflags};	//if this thread created window, just post message to update mails

			SendMessage(hMailBrowser, WM_YAMN_CHANGECONTENT, (WPARAM)ActualAccount, (LPARAM)&Params);	//we ensure this will do the thread who created the browser window
		}
		else
			UpdateMails(nullptr, ActualAccount, MyParam.nflags, MyParam.nnflags);	//update mails without displaying or refreshing any window

		if ((hMailBrowser != nullptr) && !WndFound) { //we process message loop only for thread that created window
			while (GetMessage(&msg, nullptr, 0, 0)) {
				if (hMailBrowser == nullptr || !IsDialogMessage(hMailBrowser, &msg)) { /* Wine fix. */
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}

		if ((!WndFound) && (ActualAccount->Plugin->Fcn != nullptr) && (ActualAccount->Plugin->Fcn->WriteAccountsFcnPtr != nullptr) && ActualAccount->AbleToWork)
			ActualAccount->Plugin->Fcn->WriteAccountsFcnPtr();
	}
	__finally {
		SCDecFcn(ActualAccount->UsingThreads);
	}
}

INT_PTR RunMailBrowserSvc(WPARAM wParam, LPARAM lParam)
{
	PYAMN_MAILBROWSERPARAM Param = (PYAMN_MAILBROWSERPARAM)wParam;

	if ((uint32_t)lParam != YAMN_MAILBROWSERVERSION)
		return 0;

	//an event for successfull copy parameters to which point a pointer in stack for new thread
	HANDLE ThreadRunningEV = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	Param->ThreadRunningEV = ThreadRunningEV;

	HANDLE NewThread = mir_forkthread(MailBrowser, Param);
	if (NewThread != nullptr)
		WaitForSingleObject(ThreadRunningEV, INFINITE);

	CloseHandle(ThreadRunningEV);
	return 1;
}
