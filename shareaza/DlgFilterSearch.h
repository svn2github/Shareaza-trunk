//
// DlgFilterSearch.h
//
// Copyright (c) Shareaza Development Team, 2002-2004.
// This file is part of SHAREAZA (www.shareaza.com)
//
// Shareaza is free software; you can redistribute it
// and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2 of
// the License, or (at your option) any later version.
//
// Shareaza is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Shareaza; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#if !defined(AFX_DLGFILTERSEARCH_H__779A07CC_0E13_4720_8337_ED05A9790295__INCLUDED_)
#define AFX_DLGFILTERSEARCH_H__779A07CC_0E13_4720_8337_ED05A9790295__INCLUDED_

#pragma once

#include "DlgSkinDialog.h"

class CMatchList;


class CFilterSearchDlg : public CSkinDialog
{
// Construction
public:
	CFilterSearchDlg(CWnd* pParent = NULL, CMatchList* pList = NULL);

// Dialog Data
public:
	//{{AFX_DATA(CFilterSearchDlg)
	enum { IDD = IDD_FILTER_SEARCH };
	CSpinButtonCtrl	m_wndSources;
	CString	m_sFilter;
	BOOL	m_bHideBusy;
	BOOL	m_bHideLocal;
	BOOL	m_bHidePush;
	BOOL	m_bHideReject;
	BOOL	m_bHideUnstable;
	BOOL	m_bHideBogus;
	int		m_nSources;
	CString	m_sMaxSize;
	CString	m_sMinSize;
	//}}AFX_DATA

	CMatchList*	m_pMatches;

// Overrides
public:
	//{{AFX_VIRTUAL(CFilterSearchDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CFilterSearchDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}

#endif // !defined(AFX_DLGFILTERSEARCH_H__779A07CC_0E13_4720_8337_ED05A9790295__INCLUDED_)
