//
// DlgTorrentSeed.h
//
// Copyright (c) Shareaza Development Team, 2002-2008.
// This file is part of SHAREAZA (shareaza.sourceforge.net)
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

#pragma once

#include "DlgSkinDialog.h"
#include "BTInfo.h"


class CTorrentSeedDlg :
	public CSkinDialog,
	public CThreadImpl
{
// Construction
public:
	CTorrentSeedDlg(LPCTSTR pszTorrent, BOOL bForceSeed = FALSE, CWnd* pParent = NULL);
	virtual ~CTorrentSeedDlg();

	DECLARE_DYNAMIC(CTorrentSeedDlg)
	enum { IDD = IDD_TORRENT_SEED };

// Dialog Data
protected:
	CProgressCtrl	m_wndProgress;
	CButton			m_wndDownload;
	CButton			m_wndSeed;
protected:
	BOOL			m_bCancel;
	CString			m_sTorrent;
	BOOL			m_bForceSeed;
	CString			m_sTarget;
	CBTInfo			m_pInfo;
	DWORD			m_nBlockNumber;
	DWORD			m_nBlockLength;
protected:
	CString			m_sMessage;
	QWORD			m_nVolume;
	QWORD			m_nTotal;
	int				m_nScaled;
	int				m_nOldScaled;

// Implementation
protected:
	void		OnRun();
	void		RunSingleFile();
	void		RunMultiFile();
	BOOL		CheckFiles();
	HANDLE		CreateTarget();
	BOOL		CreateDownload();

// Message Map
protected:
	DECLARE_MESSAGE_MAP()
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	afx_msg void OnDownload();
	afx_msg void OnSeed();
	virtual void OnCancel();
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
