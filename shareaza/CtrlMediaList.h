//
// CtrlMediaList.h
//
// Copyright (c) Shareaza Development Team, 2002-2011.
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

#include "CtrlLibraryTip.h"


class CMediaListCtrl : public CListCtrl
{
	DECLARE_DYNAMIC(CMediaListCtrl)

public:
	CMediaListCtrl();
	virtual ~CMediaListCtrl();


	virtual BOOL Create(CWnd* pParentWnd, UINT nID);

	BOOL	Open(LPCTSTR pszFile);
	BOOL	Enqueue(LPCTSTR pszFile, BOOL bStart = TRUE);
	int		RecursiveEnqueue(LPCTSTR pszPath);
	void	Remove(LPCTSTR pszFile);
	BOOL	LoadTextList(LPCTSTR pszFile);
	BOOL	SaveTextList(LPCTSTR pszFile);
	int		GetCount();
	void	Clear();
	UINT	GetSelectedCount();
	int		GetCurrent();
	void	SetCurrent(int nCurrent);
	int		GetNext(BOOL bSet = TRUE);
	void	Reset(BOOL bNext = TRUE);
	CString	GetPath(int nItem);
	void	OnSkinChange();

protected:
	CImageList*		m_pDragImage;
	int				m_nDragDrop;
	BOOL			m_bCreateDragImage;
	CLibraryTipCtrl	m_wndTip;
	DWORD			m_tLastUpdate;
	UINT			m_nSelectedCount;

	int		Add(LPCTSTR pszPath, int nItem = -1);
	void	Remove(int nItem);
	BOOL	AreSelectedFilesInLibrary();
	void	ShowFilePropertiesDlg(int nPage = 0 );

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnDoubleClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeyDown(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCustomDraw(NMHDR* pNotify, LRESULT* pResult);
	afx_msg void OnMediaAdd();
	afx_msg void OnUpdateMediaRate(CCmdUI* pCmdUI);
	afx_msg void OnMediaRate();
	afx_msg void OnUpdateMediaProperties(CCmdUI* pCmdUI);
	afx_msg void OnMediaProperties();
	afx_msg void OnUpdateMediaRemove(CCmdUI* pCmdUI);
	afx_msg void OnMediaRemove();
	afx_msg void OnUpdateMediaClear(CCmdUI* pCmdUI);
	afx_msg void OnMediaClear();
	afx_msg void OnUpdateMediaSelect(CCmdUI* pCmdUI);
	afx_msg void OnMediaSelect();
	afx_msg void OnUpdateMediaSave(CCmdUI* pCmdUI);
	afx_msg void OnMediaSave();
	afx_msg void OnMediaOpen();
	afx_msg void OnUpdateMediaPrevious(CCmdUI* pCmdUI);
	afx_msg void OnMediaPrevious();
	afx_msg void OnUpdateMediaNext(CCmdUI* pCmdUI);
	afx_msg void OnMediaNext();
	afx_msg void OnUpdateMediaRepeat(CCmdUI* pCmdUI);
	afx_msg void OnMediaRepeat();
	afx_msg void OnUpdateMediaRandom(CCmdUI* pCmdUI);
	afx_msg void OnMediaRandom();
	afx_msg void OnUpdateMediaCollection(CCmdUI* pCmdUI);
	afx_msg void OnMediaCollection();
	afx_msg void OnMediaAddFolder();

	DECLARE_MESSAGE_MAP()
};

#define MLN_NEWCURRENT	101
