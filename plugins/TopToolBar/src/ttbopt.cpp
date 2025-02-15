#include "stdafx.h"

static HWND OptionshWnd;

struct OrderData
{
	int dragging;
	HTREEITEM hDragItem;
	HIMAGELIST himlButtonIcons;
};

/////////////////////////////////////////////////////////////////////////////////////////

static HTREEITEM AddLine(HWND hTree,TopButtonInt *b, HTREEITEM hItem, HIMAGELIST il)
{
	TVINSERTSTRUCT tvis = {};
	tvis.hInsertAfter = hItem;
	tvis.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;

	int index;
	wchar_t* tmp;

	if (b->dwFlags & TTBBF_ISSEPARATOR) {
		tvis.item.pszText = L"------------------";
		index = -1;
		tmp = nullptr;
	}
	else {
		if (b->hIconHandleUp) {
			HICON hIcon = IcoLib_GetIconByHandle(b->hIconHandleUp);
			index = ImageList_AddIcon(il, hIcon);
			IcoLib_ReleaseIcon(hIcon);
		}
		else index = ImageList_AddIcon(il, b->hIconUp);

		tmp = mir_a2u( b->pszName );
		tvis.item.pszText = TranslateW(tmp);
	}
	tvis.item.iImage = tvis.item.iSelectedImage = index;

	tvis.item.lParam = (LPARAM)b;
	HTREEITEM hti = TreeView_InsertItem(hTree, &tvis);

	if (!(b->dwFlags & TTBBF_ISSEPARATOR))
		mir_free(tmp);

	TreeView_SetCheckState(hTree, hti, b->isVisible());
	return hti;
}

static int BuildTree(HWND hwndDlg)
{
	HWND hTree = GetDlgItem(hwndDlg, IDC_BUTTONORDERTREE);
	OrderData *dat = (OrderData*)GetWindowLongPtr(hTree, GWLP_USERDATA);

	dat->himlButtonIcons = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR32 | ILC_MASK, 2, 2);
	TreeView_SetImageList(hTree, dat->himlButtonIcons, TVSIL_NORMAL);
	SetWindowLongPtr(hTree, GWL_STYLE, GetWindowLongPtr(hTree,GWL_STYLE)|TVS_NOHSCROLL);
	TreeView_DeleteAllItems(hTree);

	if (Buttons.getCount() == 0)
		return FALSE;

	for (auto &it : Buttons)
		AddLine(hTree, it, TVI_LAST, dat->himlButtonIcons);
	return TRUE;
}

static void SaveTree(HWND hwndDlg)
{
	HWND hTree = GetDlgItem(hwndDlg, IDC_BUTTONORDERTREE);

	TVITEM tvi = { 0 };
	tvi.hItem = TreeView_GetRoot(hTree);
	tvi.stateMask = TVIS_STATEIMAGEMASK;
	tvi.mask = TVIF_PARAM | TVIF_HANDLE | TVIF_STATE;

	LIST<TopButtonInt> tmpList(8);

	while(tvi.hItem != nullptr) {
		TreeView_GetItem(hTree, &tvi);

		TopButtonInt* btn = (TopButtonInt*)tvi.lParam;
		Buttons.remove(btn);

		if (TreeView_GetCheckState(hTree,tvi.hItem))
			btn->dwFlags |= TTBBF_VISIBLE;
		else
			btn->dwFlags &= ~TTBBF_VISIBLE;
		btn->dwFlags &= ~TTBBF_OPTIONAL;
		btn->arrangedpos = tmpList.getCount();

		tmpList.insert(btn);
		tvi.hItem = TreeView_GetNextSibling(hTree, tvi.hItem);
	}
	{
		mir_cslock lck(csButtonsHook);
		for (auto &it : Buttons)
			delete it;
		Buttons = tmpList;
	}
	SaveAllButtonsOptions();
}

void CancelProcess(HWND hwndDlg)
{
	HWND hTree = GetDlgItem(hwndDlg, IDC_BUTTONORDERTREE);

	TVITEM tvi = { 0 };
	tvi.hItem = TreeView_GetRoot(hTree);

	while(tvi.hItem != nullptr) {
		tvi.stateMask = TVIS_STATEIMAGEMASK;
		tvi.mask = TVIF_PARAM | TVIF_HANDLE | TVIF_STATE;
		TreeView_GetItem(hTree, &tvi);

		TopButtonInt* btn = (TopButtonInt*)tvi.lParam;

		if (btn ->dwFlags & TTBBF_OPTIONAL)
			delete btn;

		tvi.hItem = TreeView_GetNextSibling(hTree, tvi.hItem);
	}
}

static void RecreateWindows()
{
	{
		mir_cslock lck(csButtonsHook);
		for (auto &b : Buttons) {
			if (b->hwnd) {
				if (g_ctrl->bHardUpdate) {
					DestroyWindow(b->hwnd);
					b->CreateWnd();
				}
				else b->SetBitmap();
			}
		}
	}

	if (g_ctrl->hWnd)
		PostMessage(g_ctrl->hWnd, TTB_UPDATEFRAMEVISIBILITY, 0, 0);
}

/////////////////////////////////////////////////////////////////////////////////////////
// external functions

void AddToOptions(TopButtonInt* b)
{
	if (OptionshWnd) {
		HWND hTree = GetDlgItem(OptionshWnd, IDC_BUTTONORDERTREE);
		OrderData *dat = (OrderData*)GetWindowLongPtr(hTree, GWLP_USERDATA);
		AddLine(hTree, b, TVI_LAST, dat->himlButtonIcons);
	}
}

void RemoveFromOptions(int id)
{
	if (OptionshWnd) {
		HWND hTree = GetDlgItem(OptionshWnd, IDC_BUTTONORDERTREE);
		TVITEM tvi = { 0 };
		tvi.hItem = TreeView_GetRoot(hTree);
		tvi.mask = TVIF_PARAM | TVIF_HANDLE;

		TopButtonInt* btn;
		while(tvi.hItem != nullptr) {
			TreeView_GetItem(hTree, &tvi);
			btn = (TopButtonInt*)tvi.lParam;
			if (btn->id == id) {
			  // delete if was changed
				if (btn->dwFlags & TTBBF_OPTIONAL)
					delete btn;
				TreeView_DeleteItem(hTree,tvi.hItem);
				break;
			}

			tvi.hItem = TreeView_GetNextSibling(hTree, tvi.hItem);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// Options window: main

static INT_PTR CALLBACK ButOrderOpts(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND hTree = GetDlgItem(hwndDlg, IDC_BUTTONORDERTREE);
	OrderData *dat = (OrderData*)GetWindowLongPtr(hTree, GWLP_USERDATA);

	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		dat = (OrderData*)malloc( sizeof(OrderData));
		SetWindowLongPtr(hTree, GWLP_USERDATA, (LONG_PTR)dat);
		dat->dragging = 0;

		SetWindowLongPtr(hTree, GWL_STYLE, GetWindowLongPtr(hTree, GWL_STYLE)|TVS_NOHSCROLL);

		SetDlgItemInt(hwndDlg, IDC_BUTTHEIGHT, g_ctrl->nButtonHeight, FALSE);
		SendDlgItemMessage(hwndDlg, IDC_SPIN_HEIGHT, UDM_SETRANGE, 0, MAKELONG(50,10));
		SendDlgItemMessage(hwndDlg, IDC_SPIN_HEIGHT, UDM_SETPOS, 0, MAKELONG(g_ctrl->nButtonHeight,0));

		SetDlgItemInt(hwndDlg, IDC_BUTTWIDTH, g_ctrl->nButtonWidth, FALSE);
		SendDlgItemMessage(hwndDlg, IDC_SPIN_WIDTH, UDM_SETRANGE, 0, MAKELONG(50,10));
		SendDlgItemMessage(hwndDlg, IDC_SPIN_WIDTH, UDM_SETPOS, 0, MAKELONG(g_ctrl->nButtonWidth,0));

		SetDlgItemInt(hwndDlg, IDC_BUTTGAP, g_ctrl->nButtonSpace, FALSE);
		SendDlgItemMessage(hwndDlg, IDC_SPIN_GAP, UDM_SETRANGE, 0, MAKELONG(20,0));
		SendDlgItemMessage(hwndDlg, IDC_SPIN_GAP, UDM_SETPOS, 0, MAKELONG(g_ctrl->nButtonSpace,0));

		CheckDlgButton(hwndDlg, IDC_USEFLAT, g_ctrl->bFlatButtons ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_AUTORESIZE, g_ctrl->bAutoSize ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwndDlg, IDC_SINGLELINE, g_ctrl->bSingleLine ? BST_CHECKED : BST_UNCHECKED);

		BuildTree(hwndDlg);
		EnableWindow(GetDlgItem(hwndDlg, IDC_ENAME), FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_EPATH), FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_DELLBUTTON), FALSE);

		OptionshWnd = hwndDlg;
		return TRUE;

	case WM_COMMAND:
		if (HIWORD(wParam) == EN_CHANGE && OptionshWnd) {
			switch(LOWORD(wParam)) {
			case IDC_ENAME: case IDC_EPATH:
				break;
			default:
				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			}
			break;
		}

		if ((HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == BN_DBLCLK)) {
			int ctrlid = LOWORD(wParam);

			//----- Launch buttons -----

			if (ctrlid == IDC_BROWSE) {
				wchar_t str[MAX_PATH];
				OPENFILENAME ofn = {0};

				GetDlgItemText(hwndDlg, IDC_EPATH, str, _countof(str));
				ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
				ofn.hwndOwner = hwndDlg;
				ofn.hInstance = nullptr;
				ofn.lpstrFilter = nullptr;
				ofn.lpstrFile = str;
				ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
				ofn.nMaxFile = _countof(str);
				ofn.nMaxFileTitle = MAX_PATH;
				ofn.lpstrDefExt = L"exe";
				if (!GetOpenFileName(&ofn))
					break;

				SetDlgItemText(hwndDlg, IDC_EPATH, str);

				break;
			}

			SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);

			if (ctrlid == IDC_LBUTTONSET) {
				TVITEM tvi ={0};
				tvi.hItem = TreeView_GetSelection(hTree);
				if (tvi.hItem == nullptr)
					break;

				tvi.mask = TVIF_PARAM;
				TreeView_GetItem(hTree, &tvi);

				TopButtonInt* btn = (TopButtonInt*)tvi.lParam;
				wchar_t buf [256];
				// probably, condition not needs
				if (btn->dwFlags & TTBBF_ISLBUTTON) {
					if (!(btn->dwFlags & TTBBF_OPTIONAL)) {
						// create button
						TTBButton ttb = {};
						ttb.hIconDn = (HICON)LoadImage(g_plugin.getInst(), MAKEINTRESOURCE(IDI_RUN), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
						ttb.dwFlags = TTBBF_VISIBLE | TTBBF_ISLBUTTON | TTBBF_INTERNAL | TTBBF_OPTIONAL;
						ttb.name = nullptr;
						ttb.program = nullptr;
						int id = btn->id;
						btn = CreateButton(&ttb);
						btn->id = id;

						tvi.lParam = (LPARAM)btn;
						TreeView_SetItem(hTree, &tvi);
					}

					GetDlgItemText(hwndDlg, IDC_ENAME, buf, _countof(buf));
					replaceStr(btn->pszName, _T2A(buf));

					tvi.mask = TVIF_TEXT;
					tvi.pszText = buf;
					TreeView_SetItem(hTree, &tvi);

					GetDlgItemText(hwndDlg, IDC_EPATH, buf, _countof(buf));
					replaceStrW(btn->ptszProgram, buf);
				}
				break;
			}

			if (ctrlid == IDC_ADDLBUTTON) {
				// create button
				TTBButton ttb = {};
				ttb.hIconDn = (HICON)LoadImage(g_plugin.getInst(), MAKEINTRESOURCE(IDI_RUN), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
				ttb.dwFlags = TTBBF_VISIBLE | TTBBF_ISLBUTTON | TTBBF_INTERNAL | TTBBF_OPTIONAL;
				ttb.name = LPGEN("Default");
				ttb.program = L"Execute Path";
				TopButtonInt* b = CreateButton(&ttb);

				// get selection for insert
				TVITEM tvi = {0};
				tvi.hItem = TreeView_GetSelection(hTree);

				// insert item
				AddLine(hTree, b, tvi.hItem, dat->himlButtonIcons);

				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				break;
			}

			//----- Separators -----

			if (ctrlid == IDC_ADDSEP) {
				// create button
				TTBButton ttb = {};
				ttb.dwFlags = TTBBF_VISIBLE | TTBBF_ISSEPARATOR | TTBBF_INTERNAL | TTBBF_OPTIONAL;
				TopButtonInt* b = CreateButton(&ttb);

				// get selection for insert
				TVITEM tvi = {0};
				tvi.hItem = TreeView_GetSelection(hTree);

				// insert item
				AddLine(hTree, b, tvi.hItem, dat->himlButtonIcons);

				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				break;
			}

			if (ctrlid == IDC_REMOVEBUTTON) {
				TVITEM tvi = {0};
				tvi.hItem = TreeView_GetSelection(hTree);
				if (tvi.hItem == nullptr)
					break;

				tvi.mask = TVIF_PARAM;
				TreeView_GetItem(hTree, &tvi);

				TopButtonInt* btn = (TopButtonInt*)tvi.lParam;
				// if button enabled for separator and launch only, no need condition
				// except possible service button introducing
				if (btn->dwFlags & (TTBBF_ISSEPARATOR | TTBBF_ISLBUTTON)) {
					// delete if was added in options
					if (btn->dwFlags & TTBBF_OPTIONAL)
						delete btn;

					TreeView_DeleteItem(hTree,tvi.hItem);

					SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
				}
				break;
			}
		}
		break;

	case WM_NOTIFY:
		switch(((LPNMHDR)lParam)->idFrom) {
		case 0:
			switch (((LPNMHDR)lParam)->code) {
			case PSN_APPLY:
				g_ctrl->nButtonHeight = GetDlgItemInt(hwndDlg, IDC_BUTTHEIGHT, nullptr, FALSE);
				g_ctrl->nButtonWidth = GetDlgItemInt(hwndDlg, IDC_BUTTWIDTH, nullptr, FALSE);
				g_ctrl->nButtonSpace = GetDlgItemInt(hwndDlg, IDC_BUTTGAP, nullptr, FALSE);
				
				g_ctrl->bFlatButtons = (uint8_t)IsDlgButtonChecked(hwndDlg, IDC_USEFLAT);
				g_ctrl->bAutoSize = (uint8_t)IsDlgButtonChecked(hwndDlg, IDC_AUTORESIZE);
				g_ctrl->bSingleLine = (uint8_t)IsDlgButtonChecked(hwndDlg, IDC_SINGLELINE);

				g_plugin.setDword("BUTTHEIGHT", g_ctrl->nButtonHeight);
				g_plugin.setDword("BUTTWIDTH", g_ctrl->nButtonWidth);
				g_plugin.setDword("BUTTGAP", g_ctrl->nButtonSpace);

				g_plugin.setByte("UseFlatButton", g_ctrl->bFlatButtons);
				g_plugin.setByte("SingleLine", g_ctrl->bSingleLine);
				g_plugin.setByte("AutoSize", g_ctrl->bAutoSize);

				SaveTree(hwndDlg);
				RecreateWindows();
				ArrangeButtons();
			}
			break;

		case IDC_BUTTONORDERTREE:
			switch (((LPNMHDR)lParam)->code) {
			case TVN_BEGINDRAG:
				SetCapture(hwndDlg);
				dat->dragging = 1;
				dat->hDragItem = ((LPNMTREEVIEW)lParam)->itemNew.hItem;
				TreeView_SelectItem(hTree, dat->hDragItem);
				break;

			case NM_CLICK:
				{
					TVHITTESTINFO hti;
					hti.pt.x = (short)LOWORD(GetMessagePos());
					hti.pt.y = (short)HIWORD(GetMessagePos());
					ScreenToClient(((LPNMHDR)lParam)->hwndFrom, &hti.pt);
					if (TreeView_HitTest(((LPNMHDR)lParam)->hwndFrom, &hti))
						if (hti.flags & TVHT_ONITEMSTATEICON) {
							SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
							TreeView_SelectItem(hTree, hti.hItem);
						}
				}
				break;

			case TVN_SELCHANGED:
				{
					HTREEITEM hti = TreeView_GetSelection(hTree);
					if (hti == nullptr)
						break;

					TopButtonInt *btn = (TopButtonInt*)((LPNMTREEVIEW)lParam)->itemNew.lParam;

					mir_cslock lck(csButtonsHook);

					if (btn->dwFlags & TTBBF_ISLBUTTON) {
						bool enable = (btn->dwFlags & TTBBF_INTERNAL) !=0;
						EnableWindow(GetDlgItem(hwndDlg, IDC_ENAME), enable);
						EnableWindow(GetDlgItem(hwndDlg, IDC_EPATH), enable);
						EnableWindow(GetDlgItem(hwndDlg, IDC_REMOVEBUTTON), enable);
						EnableWindow(GetDlgItem(hwndDlg, IDC_LBUTTONSET), enable);
						if (btn->pszName != nullptr)
							SetDlgItemTextA(hwndDlg, IDC_ENAME, btn->pszName);
						else
							SetDlgItemTextA(hwndDlg, IDC_ENAME, "");

						if (btn->ptszProgram != nullptr)
							SetDlgItemText(hwndDlg, IDC_EPATH, btn->ptszProgram);
						else
							SetDlgItemTextA(hwndDlg, IDC_EPATH, "");
					}
					else {
						EnableWindow(GetDlgItem(hwndDlg,IDC_REMOVEBUTTON),
								(btn->dwFlags & TTBBF_ISSEPARATOR)?TRUE:FALSE);

						EnableWindow(GetDlgItem(hwndDlg, IDC_ENAME), FALSE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_EPATH), FALSE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_LBUTTONSET), FALSE);
						SetDlgItemTextA(hwndDlg, IDC_ENAME, "");
						SetDlgItemTextA(hwndDlg, IDC_EPATH, "");
					}
				}
			}
			break;
		}
		break;

	case WM_MOUSEMOVE:
		if (dat->dragging) {
			TVHITTESTINFO hti;
			hti.pt.x = (short)LOWORD(lParam);
			hti.pt.y = (short)HIWORD(lParam);
			ClientToScreen(hwndDlg, &hti.pt);
			ScreenToClient(hTree, &hti.pt);
			TreeView_HitTest(hTree, &hti);
			if (hti.flags & (TVHT_ONITEM | TVHT_ONITEMRIGHT)) {
				HTREEITEM it=hti.hItem;
				hti.pt.y -= TreeView_GetItemHeight(hTree)/2;
				TreeView_HitTest(hTree, &hti);
				if (!(hti.flags&TVHT_ABOVE))
					TreeView_SetInsertMark(hTree,hti.hItem,1);
				else 
					TreeView_SetInsertMark(hTree,it,0);
			}
			else {
				if (hti.flags & TVHT_ABOVE) SendMessage(hTree, WM_VSCROLL, MAKEWPARAM(SB_LINEUP, 0), 0);
				if (hti.flags & TVHT_BELOW) SendMessage(hTree, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), 0);
				TreeView_SetInsertMark(hTree, NULL, 0);
			}
		}
		break;

	case WM_LBUTTONUP:
		if (dat->dragging) {
			TreeView_SetInsertMark(hTree, NULL, 0);
			dat->dragging = 0;
			ReleaseCapture();

			TVHITTESTINFO hti;
			hti.pt.x = (short)LOWORD(lParam);
			hti.pt.y = (short)HIWORD(lParam);
			ClientToScreen(hwndDlg, &hti.pt);
			ScreenToClient(hTree, &hti.pt);
			hti.pt.y -= TreeView_GetItemHeight(hTree)/2;
			TreeView_HitTest(hTree, &hti);
			if (dat->hDragItem == hti.hItem)
				break;
			if (hti.flags&TVHT_ABOVE)
				hti.hItem=TVI_FIRST;

			TVITEM tvi;
			tvi.mask = TVIF_HANDLE|TVIF_PARAM;
			tvi.hItem = (HTREEITEM) dat->hDragItem;
			TreeView_GetItem(hTree, &tvi);
			if ( (hti.flags & (TVHT_ONITEM | TVHT_ONITEMRIGHT)) || (hti.hItem==TVI_FIRST)) {
				TVINSERTSTRUCT tvis;
				wchar_t name[128];
				tvis.item.mask = TVIF_HANDLE | TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
				tvis.item.stateMask = 0xFFFFFFFF;
				tvis.item.pszText = name;
				tvis.item.cchTextMax = _countof(name);
				tvis.item.hItem = dat->hDragItem;
				TreeView_GetItem(hTree, &tvis.item);

				TreeView_DeleteItem(hTree, dat->hDragItem);
				tvis.hParent = nullptr;
				tvis.hInsertAfter = hti.hItem;
				TreeView_SelectItem(hTree, TreeView_InsertItem(hTree, &tvis));

				SendMessage(GetParent(hwndDlg), PSM_CHANGED, 0, 0);
			}
		}
		break;

	case WM_DESTROY:
		if (dat) {
			ImageList_Destroy(dat->himlButtonIcons);
			free(dat);
		}
		OptionshWnd = nullptr;
		break;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////

int TTBOptInit(WPARAM wParam, LPARAM)
{
	OPTIONSDIALOGPAGE odp = {};
	odp.szGroup.a = LPGEN("Contact list");
	odp.position = -1000000000;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_BUTORDER);
	odp.szTitle.a = LPGEN("Toolbar");
	odp.pfnDlgProc = ButOrderOpts;
	odp.flags = ODPF_BOLDGROUPS;
	g_plugin.addOptions(wParam, &odp);
	return 0;
}
