//
// Download.cpp
//
// Copyright (c) Shareaza Development Team, 2002-2010.
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

#include "StdAfx.h"
#include "Shareaza.h"
#include "BTTrackerRequest.h"
#include "Download.h"
#include "DownloadGroups.h"
#include "DownloadSource.h"
#include "DownloadTask.h"
#include "DownloadTransfer.h"
#include "Downloads.h"
#include "FileExecutor.h"
#include "FragmentedFile.h"
#include "Library.h"
#include "LibraryBuilder.h"
#include "LibraryHistory.h"
#include "Network.h"
#include "Settings.h"
#include "SharedFile.h"
#include "Transfers.h"
#include "Uploads.h"
#include "XML.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// CDownload construction

CDownload::CDownload() :
	m_nSerID		( Downloads.GetFreeSID() )
,	m_bExpanded		( Settings.Downloads.AutoExpand )
,	m_bSelected		( FALSE )
,	m_tCompleted	( 0 )
,	m_nRunCookie	( 0 )
,	m_nGroupCookie	( 0 )

,	m_bTempPaused	( FALSE )
,	m_bPaused		( FALSE )
,	m_bBoosted		( FALSE )
,	m_bShared		( Settings.Uploads.SharePartials )
,	m_bComplete		( false )
,	m_tSaved		( 0 )
,	m_tBegan		( 0 )
,	m_bDownloading	( false )
{
}

CDownload::~CDownload()
{
	AbortTask();
	DownloadGroups.Unlink( this );
}

//////////////////////////////////////////////////////////////////////
// CDownload control : pause

void CDownload::Pause(BOOL bRealPause)
{
	if ( m_bComplete || m_bPaused )
		return;

	theApp.Message( MSG_NOTICE, IDS_DOWNLOAD_PAUSED, GetDisplayName() );

	StopTrying();
	m_bTempPaused = TRUE;

	if ( bRealPause )
		m_bPaused = TRUE;
}

//////////////////////////////////////////////////////////////////////
// CDownload control : resume

void CDownload::Resume()
{
	if ( IsCompleted() )
		return;
	
	if ( !IsPaused() )
	{
		StartTrying();
		return;
	}

	theApp.Message( MSG_NOTICE, IDS_DOWNLOAD_RESUMED, (LPCTSTR)GetDisplayName() );

	if ( IsFileOpen() )
	{
		for ( POSITION posSource = GetIterator(); posSource ; )
		{
			CDownloadSource* pSource = GetNext( posSource );

			pSource->OnResume();
		}
	}

	m_bPaused				= FALSE;
	m_bTempPaused			= FALSE;
	m_tReceived				= GetTickCount();
	m_bTorrentTrackerError	= FALSE;

	// Try again
	ClearFileError();

	SetModified();
}

//////////////////////////////////////////////////////////////////////
// CDownload control : remove

void CDownload::Remove()
{
	StopTrying();
	CloseTorrent();
	CloseTransfers();
	AbortTask();

	theApp.Message( MSG_NOTICE, IDS_DOWNLOAD_REMOVE, (LPCTSTR)GetDisplayName() );

	if ( IsCompleted() )
		CloseFile();
	else
		DeleteFile();

	DeletePreviews();

	if ( ! m_sPath.IsEmpty() )
	{
		DeleteFileEx( m_sPath + _T(".png"), FALSE, FALSE, TRUE );
		DeleteFileEx( m_sPath + _T(".sav"), FALSE, FALSE, TRUE );
		DeleteFileEx( m_sPath, FALSE, FALSE, TRUE );
		m_sPath.Empty();
	}

	Downloads.Remove( this );
}

//////////////////////////////////////////////////////////////////////
// CDownload control : boost

void CDownload::Boost()
{
	if ( ! IsFileOpen() || m_bBoosted ) return;

	theApp.Message( MSG_NOTICE, IDS_DOWNLOAD_BOOST, (LPCTSTR)GetDisplayName() );

	for ( CDownloadTransfer* pTransfer = GetFirstTransfer() ; pTransfer ; pTransfer = pTransfer->m_pDlNext )
	{
		pTransfer->Boost();
	}

	m_bBoosted = TRUE;
	SetModified();
}

//////////////////////////////////////////////////////////////////////
// CDownload control : sharing

void CDownload::Share(BOOL bShared)
{
	m_bShared = bShared;
	SetModified();
}

//////////////////////////////////////////////////////////////////////
// CDownload control : Stop trying

void CDownload::StopTrying()
{
	if ( !IsTrying() || ( IsCompleted() && !IsSeeding() ) )
		return;

	Downloads.StopTrying( IsTorrent() );

	m_tBegan = 0;
	m_bDownloading = false;

	// if m_bTorrentRequested = TRUE, raza sends Stop
	// CloseTorrent() additionally closes uploads
	if ( IsTorrent() )
		CloseTorrent();

	CloseTransfers();
	CloseFile();
	StopSearch();
	SetModified();
}

//////////////////////////////////////////////////////////////////////
// CDownload control : StartTrying

void CDownload::StartTrying()
{
	ASSERT( !IsCompleted() || IsSeeding() );
	ASSERT( !IsPaused() );
	if ( IsTrying() || IsPaused() || ( IsCompleted() && !IsSeeding() ) )
		return;

	if ( !Network.IsConnected() && !Network.Connect( TRUE ) )
		return;

	Downloads.StartTrying( IsTorrent() );
	m_tBegan = GetTickCount();
}

//////////////////////////////////////////////////////////////////////
// CDownload control : GetStartTimer

DWORD CDownload::GetStartTimer() const
{
	return m_tBegan ;
}

//////////////////////////////////////////////////////////////////////
// CDownload state checks

bool CDownload::IsStarted() const
{
	return GetVolumeComplete() > 0;
}

bool CDownload::IsPaused(bool bRealState /*= false*/) const
{
	if ( bRealState )
		return m_bPaused != 0;
	else
		return m_bTempPaused != 0;
}

// Is the download receiving data?
bool CDownload::IsDownloading() const
{
	return m_bDownloading;
}

bool CDownload::IsCompleted() const
{
	return m_bComplete;
}

bool CDownload::IsBoosted() const
{
	return ( m_bBoosted != 0 );
}

// Is the download currently trying to download?
bool CDownload::IsTrying() const
{
	return ( m_tBegan != 0 );
}

bool CDownload::IsShared() const
{
	if ( m_bShared )
		return true;

	if ( Settings.eDonkey.EnableToday && m_oED2K )
		return true;

	if ( Settings.BitTorrent.EnableToday && IsTorrent()
		&& ( IsSeeding() || IsStarted() ) )
	{
		return true;
	}

	return false;
}

CString CDownload::GetDownloadStatus() const
{
	CString strText;
	int nSources = GetEffectiveSourceCount();

	if ( IsCompleted() )
	{
		if ( IsSeeding() )
		{
			if ( m_bTorrentTrackerError )
				LoadString( strText, IDS_STATUS_TRACKERDOWN );
			else
				LoadString( strText, IDS_STATUS_SEEDING );
		}
		else
			LoadString( strText, IDS_STATUS_COMPLETED );
	}
	else if ( IsPaused() )
	{
		if ( GetFileError() != ERROR_SUCCESS )
			if ( IsMoving() )
				LoadString( strText, IDS_STATUS_CANTMOVE );
			else
				LoadString( strText, IDS_STATUS_FILEERROR );
		else
			LoadString( strText, IDS_STATUS_PAUSED );
	}
	else if ( IsMoving() )
		LoadString( strText, IDS_STATUS_MOVING );
	else if ( IsStarted() && GetProgress() == 100.0f )
		LoadString( strText, IDS_STATUS_VERIFYING );
	else if ( IsDownloading() )
	{
		DWORD nTime = GetTimeRemaining();
	
		if ( nTime == 0xFFFFFFFF )
			LoadString( strText, IDS_STATUS_ACTIVE );
		else
		{
			if ( nTime > 86400 )
				strText.Format( _T("%i:%.2i:%.2i:%.2i"), nTime / 86400, ( nTime / 3600 ) % 24, ( nTime / 60 ) % 60, nTime % 60 );
			else
				strText.Format( _T("%i:%.2i:%.2i"), nTime / 3600, ( nTime / 60 ) % 60, nTime % 60 );
		}
	}
	else if ( ! IsTrying() )
		LoadString( strText, IDS_STATUS_QUEUED );
	else if ( IsDownloading() )
		LoadString( strText, IDS_STATUS_DOWNLOADING );
	else if ( nSources > 0 )
		LoadString( strText, IDS_STATUS_PENDING );
	else if ( IsTorrent() )
	{
		if ( GetTaskType() == dtaskAllocate )
			LoadString( strText, IDS_STATUS_CREATING );
		else if ( m_bTorrentTrackerError )
			LoadString( strText, IDS_STATUS_TRACKERDOWN );
		else
			LoadString( strText, IDS_STATUS_TORRENT );
	}
	else
		LoadString( strText, IDS_STATUS_QUEUED );

	return strText;
}

int CDownload::GetClientStatus() const
{
	return IsCompleted() ? -1 : (int)GetEffectiveSourceCount();
}

CString CDownload::GetDownloadSources() const
{
	int nTotalSources = GetSourceCount();
	int nSources = GetEffectiveSourceCount();

	CString strText;
	if ( IsCompleted() )
	{
		if ( m_bVerify == TRI_TRUE )
			LoadString( strText, IDS_STATUS_VERIFIED );
		else if ( m_bVerify == TRI_FALSE )
			LoadString( strText, IDS_STATUS_UNVERIFIED );
	}
	else if ( nTotalSources == 0 )
		LoadString( strText, IDS_STATUS_NOSOURCES );
	else if ( nSources == nTotalSources )
	{
		CString strSource;
		LoadSourcesString( strSource,  nSources );
		strText.Format( _T("(%i %s)"), nSources, (LPCTSTR)strSource );
	}
	else
	{
		CString strSource;
		LoadSourcesString( strSource,  nTotalSources, true );
		strText.Format( _T("(%i/%i %s)"), nSources, nTotalSources, (LPCTSTR)strSource );
	}
	return strText;
}

//////////////////////////////////////////////////////////////////////
// CDownload run handler

void CDownload::OnRun()
{
	// Set the currently downloading state
	// (Used to optimize display in Ctrl/Wnd functions)
	m_bDownloading = false;

	DWORD tNow = GetTickCount();

	if ( !IsPaused() )
	{
		if ( GetFileError() != ERROR_SUCCESS  )
		{
			// File or disk errors
			Pause( FALSE );
		}
		else if ( IsMoving() )
		{
			if ( ! IsCompleted() && ! IsTasking() )
				OnDownloaded();
		}
		else if ( IsTrying() || IsSeeding() )
		{	//This download is trying to download

			//'Dead download' check- if download appears dead, give up and allow another to start.
			if ( ! IsCompleted() && ( tNow - GetStartTimer() ) > ( 3 * 60 * 60 * 1000 )  )
			{	//If it's not complete, and we've been trying for at least 3 hours

				DWORD tHoursToTry = min( ( GetEffectiveSourceCount() + 49u ) / 50u, 9lu ) + Settings.Downloads.StarveGiveUp;

				if (  ( tNow - m_tReceived ) > ( tHoursToTry * 60 * 60 * 1000 ) )
				{	//And have had no new data for 5-14 hours

					if ( IsTorrent() )	//If it's a torrent
					{
						if ( Downloads.GetTryingCount( true ) >= Settings.BitTorrent.DownloadTorrents )
						{	//If there are other torrents that could start
							StopTrying();		//Give up for now, try again later
							return;
						}
					}
					else			//It's a regular download
					{
						if ( Downloads.GetTryingCount() >= ( Settings.Downloads.MaxFiles + Settings.Downloads.MaxFileSearches ) )
						{	//If there are other downloads that could try
							StopTrying();		//Give up for now, try again later
							return;
						}
					}
				}
			}	//End of 'dead download' check

			// Run the download
			if ( ! IsTorrent() || RunTorrent( tNow ) )
			{
				RunSearch( tNow );
				RunValidation();

				if ( IsSeeding() )
				{
					// Mark as collapsed to get correct heights when dragging files
					if ( ! Settings.General.DebugBTSources && m_bExpanded )
						m_bExpanded = FALSE;
				}
				else
				{
					if ( IsComplete() && IsFileOpen() )
					{
						if ( IsFullyVerified() )
							OnDownloaded();
					}
					else if ( CheckTorrentRatio() )
					{
						if ( Network.IsConnected() )
							StartTransfersIfNeeded( tNow );
						else
						{
							StopTrying();
							return;
						}
					}
				}
			} // if ( RunTorrent( tNow ) )

			// Calculate the current downloading state
			if( HasActiveTransfers() )
				m_bDownloading = true;
		}
		else if ( ! IsCompleted() && m_bVerify != TRI_TRUE )
		{
			if ( Network.IsConnected() )
			{
				// We have extra regular downloads 'trying' so when a new slot
				// is ready, a download has sources and is ready to go.
				if ( Downloads.GetTryingCount() < ( Settings.Downloads.MaxFiles + Settings.Downloads.MaxFileSearches ) )
				{
					if ( IsTorrent() )
					{
						// Torrents only try when 'ready to go'. (Reduce tracker load)
						if ( Downloads.GetTryingCount( true ) < Settings.BitTorrent.DownloadTorrents )
							Resume();
					}
					else
						Resume();
				}
			}
			else
				ASSERT( !IsTrying() );
		}
	}

	// Don't save Downloads with many sources too often, since it's slow
	if ( tNow - m_tSaved >=
		( GetCount() > 20 ? 5 * Settings.Downloads.SaveInterval : Settings.Downloads.SaveInterval ) )
	{
		if ( IsModified() )
		{
			FlushFile();
			if ( Save() )
				m_tSaved = tNow;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// CDownload download complete handler

void CDownload::OnDownloaded()
{
	ASSERT( m_bComplete == false );

	theApp.Message( MSG_NOTICE, IDS_DOWNLOAD_COMPLETED, GetDisplayName() );
	m_tCompleted = GetTickCount();
	m_bDownloading = false;

	CloseTransfers();

	// AppendMetadata();

	if ( GetTaskType() == dtaskMergeFile || GetTaskType() == dtaskPreviewRequest )
	{
		AbortTask();
	}

	CDownloadTask::Copy( this );

	SetModified();
}

//////////////////////////////////////////////////////////////////////
// CDownload task completion

void CDownload::OnTaskComplete(const CDownloadTask* pTask)
{
	SetTask( NULL );

	// Check if task was aborted
	if ( pTask->WasAborted() )
		return;

	if ( pTask->GetTaskType() == dtaskPreviewRequest )
	{
		OnPreviewRequestComplete( pTask );
	}
	else if ( pTask->GetTaskType() == dtaskCopy )
	{
		if ( ! pTask->HasSucceeded() )
			SetFileError( pTask->GetFileError() );
		else
			OnMoved();
	}
}

//////////////////////////////////////////////////////////////////////
// CDownload moved handler

void CDownload::OnMoved()
{
	// We just completed torrent
	if ( IsTorrent() && IsFullyVerified() )
	{
		// Set to FALSE to prevent sending 'stop' announce to tracker
		m_bTorrentRequested = FALSE;
		StopTrying();

		// Send 'completed' announce to tracker
		SendCompleted();

		// This torrent is now seeding
		m_bSeeding = TRUE;
		m_bVerify = TRI_TRUE;
		m_bTorrentStarted = TRUE;
		m_bTorrentRequested = TRUE;
	}
	else if ( IsTorrent() ) // Something wrong (?), since we moved the torrent
	{
		// Explicitly set to TRUE to send stop announce to tracker
		m_bTorrentRequested = TRUE;
		StopTrying();
	}
	else
		StopTrying();

	ClearSources();

	ASSERT( ! m_sPath.IsEmpty() );
	DeleteFileEx( m_sPath + _T(".png"), FALSE, FALSE, TRUE );
	DeleteFileEx( m_sPath + _T(".sav"), FALSE, FALSE, TRUE );
	DeleteFileEx( m_sPath, FALSE, FALSE, TRUE );
	m_sPath.Empty();

	// Download finalized, tracker notified, set flags that we completed
	m_bComplete		= true;
	m_tCompleted	= GetTickCount();
}

//////////////////////////////////////////////////////////////////////
// CDownload load and save

BOOL CDownload::Load(LPCTSTR pszName)
{
	ASSERT( m_sPath.IsEmpty() );
	m_sPath = pszName;

	BOOL bSuccess = FALSE;
	CFile pFile;
	if ( pFile.Open( m_sPath, CFile::modeRead ) )
	{
		TRY
		{
			CArchive ar( &pFile, CArchive::load, 32768 );	// 32 KB buffer
			Serialize( ar, 0 );
			bSuccess = TRUE;
		}
		CATCH( CFileException, pException )
		{
			if ( pException->m_cause == CFileException::fileNotFound )
			{
				// Subfile missing
				return FALSE;
			}
		}
		AND_CATCH_ALL( pException )
		{
			theApp.Message( MSG_ERROR, IDS_DOWNLOAD_FILE_OPEN_ERROR, m_sPath );
		}
		END_CATCH_ALL

		pFile.Close();
	}

	if ( ! bSuccess && pFile.Open( m_sPath + _T(".sav"), CFile::modeRead ) )
	{
		TRY
		{
			CArchive ar( &pFile, CArchive::load, 32768 );	// 32 KB buffer
			Serialize( ar, 0 );
			bSuccess = TRUE;
		}
		CATCH( CFileException, pException )
		{
			if ( pException->m_cause == CFileException::fileNotFound )
			{
				// Subfile missing
				return FALSE;
			}
		}
		AND_CATCH_ALL( pException )
		{
			theApp.Message( MSG_ERROR, IDS_DOWNLOAD_FILE_OPEN_ERROR, m_sPath + _T(".sav") );
		}
		END_CATCH_ALL

		pFile.Close();

		if ( bSuccess )
			Save();
	}

	m_bGotPreview = GetFileAttributes( m_sPath + _T(".png") ) != INVALID_FILE_ATTRIBUTES;
	m_nSaveCookie = m_nCookie;

	return bSuccess;
}

BOOL CDownload::Save(BOOL bFlush)
{
	CSingleLock pTransfersLock( &Transfers.m_pSection, TRUE );

	if ( m_sPath.IsEmpty() )
	{
		// From incomplete folder
		m_sPath = Settings.Downloads.IncompletePath + _T("\\") + GetFilename() + _T(".sd");
	}

	m_nSaveCookie = m_nCookie;
	m_tSaved = GetTickCount();

	if ( m_bComplete && !m_bSeeding )
		return TRUE;

	if ( m_bSeeding && !Settings.BitTorrent.AutoSeed )
		return TRUE;

	DeleteFileEx( m_sPath + _T(".sav"), FALSE, FALSE, FALSE );

	CFile pFile;
	if ( ! pFile.Open( m_sPath + _T(".sav"),
		CFile::modeReadWrite|CFile::modeCreate|CFile::osWriteThrough ) )
		return FALSE;

	CHAR szID[3] = { 0, 0, 0 };
	try
	{
		CArchive ar( &pFile, CArchive::store, 32768 );	// 32 KB buffer
		try
		{
			Serialize( ar, 0 );
			ar.Close();
		}
		catch ( CException* pException )
		{
			ar.Abort();
			pFile.Abort();
			pException->Delete();
			return FALSE;
		}

		if ( Settings.Downloads.FlushSD || bFlush )
			pFile.Flush();

		pFile.SeekToBegin();
		pFile.Read( szID, 3 );
		pFile.Close();
	}
	catch ( CException* pException )
	{
		pFile.Abort();
		pException->Delete();
		return FALSE;
	}

	BOOL bSuccess = FALSE;
	if ( szID[0] == 'S' && szID[1] == 'D' && szID[2] == 'L' )
	{
		bSuccess = ::MoveFileEx( CString( _T("\\\\?\\") ) + m_sPath + _T(".sav"),
			CString( _T("\\\\?\\") ) + m_sPath,
			MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH );
	}
	else
		DeleteFileEx( m_sPath + _T(".sav"), FALSE, FALSE, FALSE );

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////
// CDownload serialize

void CDownload::Serialize(CArchive& ar, int nVersion /* DOWNLOAD_SER_VERSION */)
{
	ASSERT( ! m_bComplete || m_bSeeding );

	if ( !Settings.BitTorrent.AutoSeed && m_bSeeding )
		return;

	if ( nVersion == 0 )
	{
		nVersion = DOWNLOAD_SER_VERSION;

		if ( ar.IsStoring() )
		{
			ar.Write( "SDL", 3 );
			ar << nVersion;
		}
		else
		{
			CHAR szID[3];
			ReadArchive( ar, szID, 3 );
			if ( strncmp( szID, "SDL", 3 ) ) AfxThrowUserException();
			ar >> nVersion;
			if ( nVersion <= 0 || nVersion > DOWNLOAD_SER_VERSION ) AfxThrowUserException();
		}
	}
	else if ( nVersion < 11 && ar.IsLoading() )
	{
		SerializeOld( ar, nVersion );
		return;
	}

	CDownloadWithExtras::Serialize( ar, nVersion );

	if ( ar.IsStoring() )
	{
		ar << m_bExpanded;
		ar << m_bPaused;
		ar << m_bBoosted;
		ar << m_bShared;

		ar << m_nSerID;
	}
	else
	{
		ar >> m_bExpanded;
		ar >> m_bPaused;
		m_bTempPaused = m_bPaused;
		ar >> m_bBoosted;
		if ( nVersion >= 14 ) ar >> m_bShared;
		if ( nVersion >= 26 ) ar >> m_nSerID;

		DownloadGroups.Link( this );

		if ( nVersion == 32 )
		{
			// Compatibility for CB Branch.
			if ( ! ar.IsBufferEmpty() )
			{
				CString sSearchKeyword;
				ar >> sSearchKeyword;
			}
		}
	}
}

void CDownload::SerializeOld(CArchive& ar, int nVersion /* DOWNLOAD_SER_VERSION */)
{
	ASSERT( ar.IsLoading() );

	ar >> m_sPath;
	m_sPath += _T(".sd");
	ar >> m_sName;

	DWORD nSize;
	ar >> nSize;
	m_nSize = nSize;

	Hashes::Sha1Hash oSHA1;
	SerializeIn( ar, oSHA1, nVersion );
	m_oSHA1 = oSHA1;
	m_bSHA1Trusted = true;

	ar >> m_bPaused;
	ar >> m_bExpanded;
	if ( nVersion >= 6 ) ar >> m_bBoosted;

	CDownloadWithFile::SerializeFile( ar, nVersion );

	for ( DWORD_PTR nSources = ar.ReadCount() ; nSources ; nSources-- )
	{
		CDownloadSource* pSource = new CDownloadSource( this );
		pSource->Serialize( ar, nVersion );
		AddSourceInternal( pSource );
	}

	if ( nVersion >= 3 && ar.ReadCount() )
	{
		auto_ptr< CXMLElement > pXML( new CXMLElement() );
		pXML->Serialize( ar );
		MergeMetadata( pXML.get() );
	}
}

void CDownload::ForceComplete()
{
	m_bPaused = FALSE;
	m_bTempPaused = FALSE;
	m_bVerify = TRI_FALSE;
	MakeComplete();
	StopTrying();
	Share( FALSE );
	OnDownloaded();
}

BOOL CDownload::Launch(int nIndex, CSingleLock* pLock, BOOL bForceOriginal)
{
	if ( nIndex < 0 )
		nIndex = SelectFile( pLock );
	if ( nIndex < 0 || ! Downloads.Check( this ) )
		return FALSE;

	BOOL bResult = TRUE;
	CString strPath = GetPath( nIndex );
	CString strName = GetName( nIndex );
	CString strExt = strName.Mid( strName.ReverseFind( '.' ) );
	if ( IsCompleted() )
	{
		// Run complete file
		if ( m_bVerify == TRI_FALSE )
		{
			CString strFormat, strMessage;
			LoadString( strFormat, IDS_LIBRARY_VERIFY_FAIL );
			strMessage.Format( strFormat, (LPCTSTR)strName );

			if ( pLock ) pLock->Unlock();

			INT_PTR nResponse = AfxMessageBox( strMessage,
				MB_ICONEXCLAMATION | MB_YESNOCANCEL | MB_DEFBUTTON2 );

			if ( pLock ) pLock->Lock();

			if ( nResponse == IDCANCEL )
				return FALSE;
			if ( nResponse == IDNO )
				return TRUE;
		}

		if ( pLock ) pLock->Unlock();

		bResult = CFileExecutor::Execute( strPath, FALSE, strExt );

		if ( pLock ) pLock->Lock();
	}
	else if ( CanPreview( nIndex ) )
	{
		if ( ! bForceOriginal  )
		{
			// Previewing...
			if ( pLock ) pLock->Unlock();

			TRISTATE bSafe = CFileExecutor::IsSafeExecute( strExt, strName );

			if ( pLock ) pLock->Lock();

			if ( bSafe == TRI_UNKNOWN )
				return FALSE;
			else if ( bSafe == TRI_FALSE )
				return TRUE;

			if ( ! Downloads.Check( this ) )
				return TRUE;

			if ( PreviewFile( nIndex, pLock ) )
				return TRUE;
		}

		// Run file as is
		if ( pLock ) pLock->Unlock();

		bResult = CFileExecutor::Execute( strPath, FALSE, strExt );

		if ( pLock ) pLock->Lock();
	}

	return bResult;
}

BOOL CDownload::Enqueue(int nIndex, CSingleLock* pLock)
{
	if ( nIndex < 0 )
		nIndex = SelectFile( pLock );
	if ( nIndex < 0 || ! Downloads.Check( this ) )
		return TRUE;

	BOOL bResult = TRUE;
	CString strPath = GetPath( nIndex );
	CString strName = GetName( nIndex );
	CString strExt = strName.Mid( strName.ReverseFind( '.' ) );
	if ( IsStarted() )
	{
		if ( pLock ) pLock->Unlock();

		bResult = CFileExecutor::Enqueue( strPath, FALSE, strExt );

		if ( pLock ) pLock->Lock();
	}

	return bResult;
}
