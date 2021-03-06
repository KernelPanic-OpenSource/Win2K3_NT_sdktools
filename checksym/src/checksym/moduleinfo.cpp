//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       moduleinfo.cpp
//
//--------------------------------------------------------------------------

// ModuleInfo.cpp: implementation of the CModuleInfo class.
//
//////////////////////////////////////////////////////////////////////
#include "pch.h"

#include <dia2.h>
#include <dbghelp.h>

#include "ModuleInfo.h"
#include "SymbolVerification.h"
#include "FileData.h"
#include "DmpFile.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CModuleInfo::CModuleInfo()
{
	m_dwRefCount = 1;
	m_dwCurrentReadPosition = 0;

	m_lpInputFile = NULL;
	m_lpOutputFile = NULL;
	m_lpDmpFile = NULL;

	// File version information
	m_fPEImageFileVersionInfo = false;
	m_tszPEImageFileVersionDescription = NULL;
	m_tszPEImageFileVersionCompanyName = NULL;
	
	m_tszPEImageFileVersionString = NULL;
	m_dwPEImageFileVersionMS = 0;
	m_dwPEImageFileVersionLS = 0;
	
	m_tszPEImageProductVersionString = NULL;
	m_dwPEImageProductVersionMS = 0;
	m_dwPEImageProductVersionLS = 0;

	// PE Image Properties
	m_tszPEImageModuleName = NULL;			// Module name (eg. notepad.exe)
	m_tszPEImageModuleFileSystemPath = NULL;			// Full path to module (eg. C:\winnt\system32\notepad.exe)
	m_dwPEImageFileSize = 0;
	m_ftPEImageFileTimeDateStamp.dwLowDateTime  = 0;
	m_ftPEImageFileTimeDateStamp.dwHighDateTime = 0;
	m_dwPEImageCheckSum = 0;
	m_dwPEImageTimeDateStamp = 0;
	m_dwPEImageSizeOfImage = 0;
	m_enumPEImageType = PEImageTypeUnknown;	// We need to track what ImageType we have (PE32/PE64/???)
	m_dw64PreferredLoadAddress = 0;
	m_dw64BaseAddressOfLoadedImage = 0;

	m_wPEImageMachineArchitecture = 0;
	m_wCharacteristics = 0;						// These are the PE image characteristics

	// PE Image has a reference to DBG file
	m_enumPEImageSymbolStatus = SYMBOLS_NO;	// Assume there are no symbols for this module
	m_tszPEImageDebugDirectoryDBGPath = NULL;			// Path to DBG (found in PE Image)
	
	// PE Image has internal symbols
	m_dwPEImageDebugDirectoryCoffSize = 0;
	m_dwPEImageDebugDirectoryFPOSize = 0;
	m_dwPEImageDebugDirectoryCVSize = 0;
	m_dwPEImageDebugDirectoryOMAPtoSRCSize = 0;
	m_dwPEImageDebugDirectoryOMAPfromSRCSize = 0;
	
	// PE Image has a reference to PDB file...
	m_tszPEImageDebugDirectoryPDBPath = NULL;
	m_dwPEImageDebugDirectoryPDBFormatSpecifier = 0;
	m_dwPEImageDebugDirectoryPDBSignature = 0;				// PDB Signature (unique (across PDB instances) signature)
	m_dwPEImageDebugDirectoryPDBAge = 0;					// PDB Age (Number of times this instance has been updated)
	::ZeroMemory(&m_guidPEImageDebugDirectoryPDBGuid, sizeof(m_guidPEImageDebugDirectoryPDBGuid));

	// DBG Symbol information
	m_enumDBGModuleStatus = SYMBOL_NOT_FOUND;	// Status of the DBG file
	m_tszDBGModuleFileSystemPath = NULL;					// Path to the DBG file (after searching)
	m_dwDBGTimeDateStamp = 0;
	m_dwDBGCheckSum = 0;
	m_dwDBGSizeOfImage = 0;
	m_dwDBGImageDebugDirectoryCoffSize = 0;
	m_dwDBGImageDebugDirectoryFPOSize = 0;
	m_dwDBGImageDebugDirectoryCVSize = 0;
	m_dwDBGImageDebugDirectoryOMAPtoSRCSize = 0;
	m_dwDBGImageDebugDirectoryOMAPfromSRCSize = 0;
	
	// DBG File has a reference to a PDB file...
	m_tszDBGDebugDirectoryPDBPath = NULL;
	m_dwDBGDebugDirectoryPDBFormatSpecifier = 0;		// NB10, RSDS, etc...
	m_dwDBGDebugDirectoryPDBAge = 0;
	m_dwDBGDebugDirectoryPDBSignature = 0;
	ZeroMemory(&m_guidDBGDebugDirectoryPDBGuid, sizeof(m_guidDBGDebugDirectoryPDBGuid));

	// PDB File Information
	m_enumPDBModuleStatus = SYMBOL_NOT_FOUND; // Status of the PDB file
	m_tszPDBModuleFileSystemPath = NULL;		// Path to the PDB file (after searching)
	m_dwPDBFormatSpecifier = sigUNKNOWN;
	m_dwPDBSignature = 0;
	m_dwPDBAge = 0;
	memset(&m_guidPDBGuid, 0, sizeof(GUID));

	// Source Information
	m_fPDBSourceEnabled = false;
	m_fPDBSourceEnabledMismatch = false;

	// Source Information - MSDBI.LIB
	m_dwPDBTotalBytesOfLineInformation = 0;
	m_dwPDBTotalBytesOfSymbolInformation = 0;
	m_dwPDBTotalSymbolTypesRange = 0;

	// Source Information - MSDIA20.DLL
	m_dwPDBTotalSourceFiles = 0;
}

CModuleInfo::~CModuleInfo()
{
	// Let's cleanup a bit...
	if (m_tszPEImageFileVersionDescription)
		delete [] m_tszPEImageFileVersionDescription;

	if (m_tszPEImageFileVersionCompanyName)
		delete [] m_tszPEImageFileVersionCompanyName;
	
	if (m_tszPEImageFileVersionString)
		delete [] m_tszPEImageFileVersionString;
		
	if (m_tszPEImageProductVersionString)
		delete [] m_tszPEImageProductVersionString;

	if (m_tszPEImageModuleName)
		delete [] m_tszPEImageModuleName;

	if (m_tszPEImageModuleFileSystemPath)
		delete [] m_tszPEImageModuleFileSystemPath;

	if (m_tszPEImageDebugDirectoryDBGPath)
		delete [] m_tszPEImageDebugDirectoryDBGPath;

	if (m_tszPEImageDebugDirectoryPDBPath)
		delete [] m_tszPEImageDebugDirectoryPDBPath;

	if (m_tszDBGModuleFileSystemPath)
		delete [] m_tszDBGModuleFileSystemPath;

	if (m_tszDBGDebugDirectoryPDBPath)
		delete [] m_tszDBGDebugDirectoryPDBPath;

	if (m_tszPDBModuleFileSystemPath)
		delete [] m_tszPDBModuleFileSystemPath;

}

bool CModuleInfo::Initialize(CFileData * lpInputFile, CFileData * lpOutputFile, CDmpFile * lpDmpFile)
{
	// Let's save off the Program Options so we don't have to pass it to every method...
	m_lpInputFile =  lpInputFile;
	m_lpOutputFile = lpOutputFile;
	m_lpDmpFile = lpDmpFile;

	return true;
}

bool CModuleInfo::GetModuleInfo(LPTSTR tszModulePath, bool fDmpFile, DWORD64 dw64ModAddress, bool fGetDataFromCSVFile)
{
	bool fReturn = false;
	HANDLE hModuleHandle = INVALID_HANDLE_VALUE;

	if (fGetDataFromCSVFile)
	{
		fReturn = GetModuleInfoFromCSVFile(tszModulePath);
	} else
	{
		if (!fDmpFile)
		{
			TCHAR tszFileName[_MAX_FNAME];
			TCHAR tszFileExtension[_MAX_EXT];
		
			// Copy the Module Name to the ModuleInfo Object...
			_tsplitpath(m_tszPEImageModuleFileSystemPath, NULL, NULL, tszFileName, tszFileExtension);

			if (tszFileName && tszFileExtension)
			{
				// Compose the module name...
				m_tszPEImageModuleName = new TCHAR[_tcsclen(tszFileName)+_tcsclen(tszFileExtension)+1];
				
				if (!m_tszPEImageModuleName)
					goto cleanup;

				_tcscpy(m_tszPEImageModuleName, tszFileName);
				_tcscat(m_tszPEImageModuleName, tszFileExtension);
			}

			// Avoid opening a file at all if we're not using -R -S or -V
			if (g_lpProgramOptions->GetMode(CProgramOptions::OutputSymbolInformationMode) ||
				g_lpProgramOptions->GetMode(CProgramOptions::VerifySymbolsMode) ||
				g_lpProgramOptions->GetMode(CProgramOptions::CollectVersionInfoMode))
			{
				// Let's open the file... we use this for both Version Info and PE Image collection
				hModuleHandle = CreateFile(	tszModulePath,
											GENERIC_READ ,
											FILE_SHARE_READ,
											NULL,
											OPEN_EXISTING,
											0,
											0);

				if (hModuleHandle == INVALID_HANDLE_VALUE)
				{
					goto cleanup;
				}
			}
		}

		// Save the base address so that all DmpFile reads become relative to this...
		m_dw64BaseAddressOfLoadedImage = dw64ModAddress;
	
		// Even if we don't use -S, -V, or -R consider this successful (we have a filename)
		fReturn = true;

		// Are we to collected PE Information from this module?
		if (	g_lpProgramOptions->GetMode(CProgramOptions::OutputSymbolInformationMode) ||
			g_lpProgramOptions->GetMode(CProgramOptions::VerifySymbolsMode))
		{
			fReturn = GetPEImageInfoFromModule(hModuleHandle, fDmpFile);
		}
		
		// Do we need to collect Version Information?
		if (g_lpProgramOptions->GetMode(CProgramOptions::CollectVersionInfoMode))
		{
			fReturn = GetVersionInfoFromModule(hModuleHandle, fDmpFile);
		}
	}

cleanup:

		if (hModuleHandle != INVALID_HANDLE_VALUE)
			CloseHandle(hModuleHandle);

	return fReturn;
}

LPTSTR CModuleInfo::GetModulePath()
{
	return m_tszPEImageModuleFileSystemPath;
}

bool CModuleInfo::OutputData(LPTSTR tszProcessName, DWORD iProcessID, unsigned int dwModuleNumber)
{
	// Output to STDOUT?
	if (!g_lpProgramOptions->GetMode(CProgramOptions::QuietMode))
	{
		if (!OutputDataToStdout(dwModuleNumber))
			return false;
	}	

	// Output to file?
	if (g_lpProgramOptions->GetMode(CProgramOptions::OutputCSVFileMode))
	{
		// Try and output to file...
		if (!OutputDataToFile(tszProcessName, iProcessID))
			return false;
	}	

	return true;
}

bool CModuleInfo::fCheckPDBSignature(bool fDmpFile, HANDLE hModuleHandle, OMFSignature *pSig, PDB_INFO *ppdb)
{
	if (!DoRead(fDmpFile, hModuleHandle, pSig, sizeof(*pSig)))
		return false;

	if ( (pSig->Signature[0] != 'N') ||
         (pSig->Signature[1] != 'B') ||
         (!isdigit(pSig->Signature[2])) ||
         (!isdigit(pSig->Signature[3]))) 

	{
         //
         // If this is a DMP file (fDmpFile), odds are good that this was not compiled with
         // a linker 6.20 or higher (which marks the PDB path in the PE image such
         // that it gets mapped into the virtual address space (and will be in the user.dmp
         // file).
         //
         
        return false;
    }

	// This switch statement is reminiscent of some windbg code...don't shoot me
	// (I modified it slightly since the NB signature isn't super important to me)...
    switch (*(LONG UNALIGNED *)(pSig->Signature))
	{
        case sigNB10:	// OMF Signature, and hopefully some PDB INFO
			{
				if (!DoRead(fDmpFile, hModuleHandle, ppdb, sizeof(PDB_INFO)))
					break;
			}

		default:
            break;
    }

	// Before returning true (since we have some form of NB## symbols), we'll save this...
/*
#ifdef _UNICODE
			// Source is in ANSI, dest is in _UNICODE... need to convert...
			MultiByteToWideChar(CP_ACP,
								MB_PRECOMPOSED,
								pSig->Signature,
								4,
								m_tszPEImageDebugDirectoryNBInfo,
								4);
#else
			// Copy the ANSI string over...
			strncpy(m_tszPEImageDebugDirectoryNBInfo, pSig->Signature, 4);
#endif

	m_tszPEImageDebugDirectoryNBInfo[4] = '\0';
*/
	return true;
}

bool CModuleInfo::VerifySymbols()
{
	bool fReturn = false;

	if (!m_tszPEImageModuleName)
		goto cleanup;

	// Find/Verify a DBG file...
	if (m_enumPEImageSymbolStatus == SYMBOLS_DBG)
	{
		if ( g_lpProgramOptions->GetMode(CProgramOptions::VerifySymbolsModeWithSymbolPath) )
		{
			fReturn = GetDBGModuleFileUsingSymbolPath(g_lpProgramOptions->GetSymbolPath());
		}

		// Do we want to try an alternate method to find symbols?
		if ( m_enumDBGModuleStatus != SYMBOL_MATCH )
		{
			// Try SQL server next...
			if ( g_lpProgramOptions->GetMode(CProgramOptions::VerifySymbolsModeWithSQLServer) )
			{
				fReturn = GetDBGModuleFileUsingSQLServer();
			}
			// Try SQL2 server next ... mjl 12/14/99			
			if ( g_lpProgramOptions->GetMode(CProgramOptions::VerifySymbolsModeWithSQLServer2) )
			{
				fReturn = GetDBGModuleFileUsingSQLServer2();
			}
		}
	}
	
	// Note, it is possible that the m_enumPEImageSymbolStatus will have changed from SYMBOLS_DBG
	// to SYMBOLS_DBG_AND_PDB after the DBG file is found above...
	if ( (m_enumPEImageSymbolStatus == SYMBOLS_DBG_AND_PDB) ||
		 (m_enumPEImageSymbolStatus == SYMBOLS_PDB) )
	{
		// This gets populated with the PDB filename (obtained from the DBG or PE Image)
		if (GetDebugDirectoryPDBPath())
		{
			if (!g_lpSymbolVerification)
			{
				m_enumPDBModuleStatus = SYMBOL_NO_HELPER_DLL;
				goto cleanup;
			}

			fReturn = GetPDBModuleFileUsingSymbolPath(g_lpProgramOptions->GetSymbolPath());

			// Do we want to try an alternate method to find symbols?
			if ( m_enumPDBModuleStatus != SYMBOL_MATCH )
			{
				// Search for PDB in SQL2 if enabled - mjl 12/14/99
				if ( g_lpProgramOptions->GetMode(CProgramOptions::VerifySymbolsModeWithSQLServer2) )
				{
					fReturn = GetPDBModuleFileUsingSQLServer2();
				}
			}

		}
	}

cleanup:

	return fReturn;
}

bool CModuleInfo::GetDBGModuleFileUsingSymbolPath(LPTSTR tszSymbolPath)
{
	HANDLE hModuleHandle = INVALID_HANDLE_VALUE;
	TCHAR tszDebugModulePath[_MAX_PATH+1];				// Returned by FindFile* functions with path to DBG file

	TCHAR tszDrive[_MAX_DRIVE];							// Contains the Drove path (temporary)
	TCHAR tszDir[_MAX_DIR];								// Contains the Directory path (temporary)
	TCHAR tszFname[_MAX_FNAME];						// Contains the Filename (temporary)
	TCHAR tszPEImageExt[_MAX_EXT];						// Contains the extension of the PE image

	TCHAR tszDBGModuleName[_MAX_DIR+_MAX_FNAME+_MAX_EXT]; // Contains the DBG filename to search for (EXE\BADAPP.DBG)

	TCHAR tszDBGModuleNamePath[_MAX_PATH+1];			// Symbol path to search for DBG

	bool fDebugSearchPaths = g_lpProgramOptions->fDebugSearchPaths();
	bool fRecursiveSearch = g_lpProgramOptions->GetMode(CProgramOptions::VerifySymbolsModeWithSymbolPathRecursion);

	// Let's allocate enough storage for the complete symbol path and the module path
	LPTSTR tszSymbolPathWithModulePathPrepended = new TCHAR[_tcslen(tszSymbolPath) + _MAX_PATH + 1];

	if (tszSymbolPathWithModulePathPrepended == NULL)
		goto cleanup;

	// FIRST, BREAK THE PE IMAGE PATH TO ITS COMPONENT PARTS, BUILD PATH TO MODULE
	::_tsplitpath(m_tszPEImageModuleFileSystemPath, tszDrive, tszDir, tszFname, tszPEImageExt);

	// We may not have a path... (like from a user.dmp file)
	::_tcscpy(tszDBGModuleNamePath, tszDrive);
	::_tcscat(tszDBGModuleNamePath, tszDir);

	// SECOND, COMPUTE THE DBG FILENAME WE WILL BE USING
	//
	// The FindDebugInfoFileEx() really prefers the form of <EXT>\<FILE>.DBG
	//
	// We have two options on the name of the DBG file...
	//
	// We compose the name of the DBG file we're searching for and pass that as the
	// first parameter
	//
	// or
	//
	// We actually grab the MISC section and pull the name from there... 
	// (The debuggers have apparently changed and use the MISC section now... so we will)
	//
	if (g_lpProgramOptions->GetMode(CProgramOptions::VerifySymbolsModeNotUsingDBGInMISCSection))
	{
		// -Y4 Option

		// Compose the DBG filename from the PE Image Name
		if (fRecursiveSearch)
		{
			// Just initialize it to NULL
			tszDBGModuleName[0] = _T('\0');	
		} else
		{
			// Prepend the <EXT>\ to the front...
			::_tcscpy(tszDBGModuleName, tszPEImageExt+1);
			::_tcscat(tszDBGModuleName, _T("\\"));
		}
		::_tcscat(tszDBGModuleName, tszFname);
		::_tcscat(tszDBGModuleName, TEXT(".DBG"));
	} else
	{
		TCHAR tszExtModuleName[_MAX_EXT];					// Contains the module extension

		// If we're told to grab the DBG name from the MISC section... and there isn't one...
		// bail!
		if (!m_tszPEImageDebugDirectoryDBGPath)
			goto cleanup;

		// Okay, the user wants us to look in the MISC section of the Debug Directories
		// to figure out the DBG file location... though the FindDebugInfoFileEx() takes
		// as an argument the name of the DBG file... if you provide the PE Image name
		// instead, it performs a search that is more "broad"...

		// Split the DBG file info found in the MISC section into components...
		::_tsplitpath(m_tszPEImageDebugDirectoryDBGPath, NULL, tszDir, tszFname, tszExtModuleName);

		if (fRecursiveSearch)
		{
			// Just initialize it to NULL
			tszDBGModuleName[0] = _T('\0');	
		} else
		{
			// Prepend the <EXT>\ to the front...

			// Re-compose the MISC path...
			if (*tszDir == _T('\0'))
			{
				// Files do not tend to preserve the original extension in the MISC section like PE images...
				::_tcscpy(tszDBGModuleName, tszPEImageExt+1);
			} else
			{
				// Hopefully we have a good directory from the PE image
				::_tcscpy(tszDBGModuleName, tszDir);
			}

			CUtilityFunctions::EnsureTrailingBackslash(tszDBGModuleName);
		}

		// Tack on the rest...
		::_tcscat(tszDBGModuleName, tszFname);
		::_tcscat(tszDBGModuleName, tszExtModuleName);
	}

	if (fDebugSearchPaths)
	{
		::_tprintf(TEXT("DBG Search - Looking for [%s] Using Symbol Path...\n"), tszDBGModuleName);
	};

	//
	// If we're copying symbols to images, search adject to image first...
	// The DBG name should be present in the tszDBGModuleName...
	//
	if (g_lpProgramOptions->GetMode(CProgramOptions::CopySymbolsToImage) )
	{
		if (fDebugSearchPaths)
		{
			::_tprintf(TEXT("DBG Search - SEARCH Adjacent to Image for Symbol!\n"));
		};

		// Compose a DBG path that is adjacent to our image...
		hModuleHandle = CUtilityFunctions::FindDebugInfoFileEx(tszDBGModuleName, tszDBGModuleNamePath, tszDebugModulePath, VerifyDBGFile, this);

		// Close handle if one is returned...
		if (hModuleHandle != INVALID_HANDLE_VALUE)
		{
			::CloseHandle(hModuleHandle);
			hModuleHandle = INVALID_HANDLE_VALUE;
		}

		// Hey, if we found it, we're done...
		if (GetDBGSymbolModuleStatus() == SYMBOL_MATCH)
		{
			goto cleanup;
		}
	}

	//
	// If we're building a symbol tree... look there first for our symbol
	//
	if (g_lpProgramOptions->GetMode(CProgramOptions::BuildSymbolTreeMode) )
	{
		if (fDebugSearchPaths)
		{
			::_tprintf(TEXT("DBG Search - SEARCH in Symbol Tree We're Building!\n"));
		};

		// When we are constructing a build symbol tree... we should first
		// search there to see if our symbol is already present...
		hModuleHandle = CUtilityFunctions::FindDebugInfoFileEx(tszDBGModuleName, g_lpProgramOptions->GetSymbolTreeToBuild(), tszDebugModulePath, VerifyDBGFile, this);

		// Close handle if one is returned...
		if (hModuleHandle != INVALID_HANDLE_VALUE)
		{
			::CloseHandle(hModuleHandle);
			hModuleHandle = INVALID_HANDLE_VALUE;
		}

		// Hey, if we found it, we're done...
		if (GetDBGSymbolModuleStatus() == SYMBOL_MATCH)
		{
			goto cleanup;
		}
	}

	// Okay, we're not building a symbol tree... or we didn't find our symbol match in
	// the symbol tree... keep looking...
	
	// Well... let's search the SymbolPath provided for the DBG file...
	if (fRecursiveSearch)
	{
		// Do we use recursion???
		if (fDebugSearchPaths)
		{
			::_tprintf(TEXT("DBG Search - SEARCH Symbol path with recursion!\n"));
			// ISSUE-2000/07/24-GREGWI: Does FindDebugInfoFileEx2 support SYMSRV?
		};
	
		hModuleHandle = CUtilityFunctions::FindDebugInfoFileEx2(tszDBGModuleName, tszSymbolPath, VerifyDBGFile, this);

		// Close handle if one is returned...
		if (hModuleHandle)
		{
			::CloseHandle(hModuleHandle);
			hModuleHandle = INVALID_HANDLE_VALUE;
		}

	} else
	{
		// Only do this if we're doing the standard file search mechanism...
		if (!g_lpProgramOptions->GetMode(CProgramOptions::VerifySymbolsModeWithSymbolPathOnly))
		{
			// Don't do this block here if VerifySymbolsModeWithSymbolPathOnly option has been set...

			// Hmm... Windbg changed behavior and now prepends the module path to the
			// front of the symbolpath before called FindDebugInfoFileEx()...
			::_tcscpy(tszSymbolPathWithModulePathPrepended, tszDBGModuleNamePath);
			::_tcscat(tszSymbolPathWithModulePathPrepended, TEXT(";"));
		} else
		{
			tszSymbolPathWithModulePathPrepended[0] = '\0'; // Fast way to empty this string ;)
		}
		
		::_tcscat(tszSymbolPathWithModulePathPrepended, tszSymbolPath);

		if (fDebugSearchPaths)
		{
			::_tprintf(TEXT("DBG Search - SEARCH Symbol path!\n"));
		};

		// Do we do the standard thing?
		hModuleHandle = CUtilityFunctions::FindDebugInfoFileEx(tszDBGModuleName, tszSymbolPathWithModulePathPrepended, tszDebugModulePath, VerifyDBGFile, this);

		// Close handle if one is returned...
		if (hModuleHandle != INVALID_HANDLE_VALUE)
		{
			::CloseHandle(hModuleHandle);
			hModuleHandle = INVALID_HANDLE_VALUE;
		}
	}

cleanup:
	// If we have a hModuleHandle... free it now..
	if (hModuleHandle != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(hModuleHandle);
		hModuleHandle = INVALID_HANDLE_VALUE;
	}

	// We found the following path
	if (m_tszDBGModuleFileSystemPath)
	{
		// Okay, let's clean up any "strangeness" added by FindDebugInfoFileEx()
		// If a symbol is found in the same directory as the module, it will be
		// returned with an extra \.\ combination (which is superfluous normally)...
		LPTSTR tszLocationOfExtraJunk = _tcsstr(m_tszDBGModuleFileSystemPath, TEXT("\\.\\"));

		if ( tszLocationOfExtraJunk )
		{
			// Remember where we were...
			LPTSTR tszPreviousLocation = tszLocationOfExtraJunk;

			// Skip the junk...
			tszLocationOfExtraJunk = CharNext(tszLocationOfExtraJunk);  // '\\'
			tszLocationOfExtraJunk = CharNext(tszLocationOfExtraJunk);  // '.'

			// While we have data... copy to the old location...
			while (*tszLocationOfExtraJunk)
			{
				*tszPreviousLocation = *tszLocationOfExtraJunk;
				tszLocationOfExtraJunk = CharNext(tszLocationOfExtraJunk);
				tszPreviousLocation    = CharNext(tszPreviousLocation);
			}

			// Null terminate the module path...
			*tszPreviousLocation = '\0';
		}

	}

	if (tszSymbolPathWithModulePathPrepended)
	{
		delete [] tszSymbolPathWithModulePathPrepended;
		tszSymbolPathWithModulePathPrepended = NULL;
	}
	
	if (fDebugSearchPaths)
	{
		if (GetDBGSymbolModuleStatus() == SYMBOL_MATCH)
		{
			::_tprintf(TEXT("DBG Search - Debug Module Found at [%s]\n\n"), m_tszDBGModuleFileSystemPath);
		} else
		{
			::_tprintf(TEXT("DBG Search - Debug Module Not Found.\n\n"));
		}
	}

	return true;
}

bool CModuleInfo::GetDBGModuleFileUsingSQLServer()
{
	// Do we need to initialize the SQL Server Connection?
	if (!g_lpSymbolVerification->SQLServerConnectionInitialized() &&
		!g_lpSymbolVerification->SQLServerConnectionAttempted())
	{
		if (!g_lpSymbolVerification->InitializeSQLServerConnection( g_lpProgramOptions->GetSQLServerName() ) )
			return false;
	}

	// Let's only use the SQL Server if it was initialized properly...
	if ( g_lpSymbolVerification->SQLServerConnectionInitialized() )
	{
		if (!g_lpSymbolVerification->SearchForDBGFileUsingSQLServer(m_tszPEImageModuleName, m_dwPEImageTimeDateStamp, this))
			return false;
	}
	return true;
}

// begin SQL2 - mjl 12/14/99
bool CModuleInfo::GetDBGModuleFileUsingSQLServer2()
{
	// Do we need to initialize the SQL Server Connection?
	if (!g_lpSymbolVerification->SQLServerConnectionInitialized2() &&
		!g_lpSymbolVerification->SQLServerConnectionAttempted2())
	{
		if (!g_lpSymbolVerification->InitializeSQLServerConnection2( g_lpProgramOptions->GetSQLServerName2() ) )
			return false;
	}

	// Let's only use the SQL Server if it was initialized properly...
	if ( g_lpSymbolVerification->SQLServerConnectionInitialized2() )
	{
		if (!g_lpSymbolVerification->SearchForDBGFileUsingSQLServer2(m_tszPEImageModuleName, m_dwPEImageTimeDateStamp, this))
			return false;
	}
	return true;
}

bool CModuleInfo::GetPDBModuleFileUsingSQLServer2()
{
	// Do we need to initialize the SQL Server Connection?
	if (!g_lpSymbolVerification->SQLServerConnectionInitialized2() &&
		!g_lpSymbolVerification->SQLServerConnectionAttempted2())
	{
		if (!g_lpSymbolVerification->InitializeSQLServerConnection2( g_lpProgramOptions->GetSQLServerName2() ) )
			return false;
	}

	// Let's only use the SQL Server if it was initialized properly...
	if ( g_lpSymbolVerification->SQLServerConnectionInitialized2() )
	{
		if (!g_lpSymbolVerification->SearchForPDBFileUsingSQLServer2(m_tszPEImageModuleName, m_dwPEImageDebugDirectoryPDBSignature, this))
			return false;
	}
	return true;
}
// end SQL2 - mjl 12/14/99

bool CModuleInfo::fValidDBGCheckSum()
{
	if (m_enumDBGModuleStatus == SYMBOL_MATCH)
		return true;

	if ((g_lpProgramOptions->GetVerificationLevel() == 1) && fValidDBGTimeDateStamp())
		return true;

	if (m_enumDBGModuleStatus == SYMBOL_POSSIBLE_MISMATCH)
		return ( (m_dwPEImageCheckSum == m_dwDBGCheckSum) && 
				(m_dwPEImageSizeOfImage == m_dwDBGSizeOfImage));

	return false;
}

bool CModuleInfo::fValidDBGTimeDateStamp()
{
	return ( (m_enumDBGModuleStatus == SYMBOL_POSSIBLE_MISMATCH) ||
			 (m_enumDBGModuleStatus == SYMBOL_MATCH) )
			? (m_dwPEImageTimeDateStamp == m_dwDBGTimeDateStamp ) : false;
}

bool CModuleInfo::GetPDBModuleFileUsingSymbolPath(LPTSTR tszSymbolPath)
{
	enum {
		niNil        = 0,
		PDB_MAX_PATH = 260,
		cbErrMax     = 1024,
	};

	HANDLE hModuleHandle = NULL;
	bool fSuccess = false;
	TCHAR tszRefPath[_MAX_PATH];
	TCHAR tszImageExt[_MAX_EXT] = {0}; 	// In case there is no extension we need to null terminate now...
	char szPDBOut[cbErrMax];
	TCHAR tszPDBModuleName[_MAX_FNAME];
	LPTSTR pcEndOfPath = NULL;
	tszPDBModuleName[0] = '\0';
	LPTSTR lptszPointerToPDBName = NULL;
	bool fDebugSearchPaths = g_lpProgramOptions->fDebugSearchPaths();
	bool fDiaDataSourcePresent = g_lpSymbolVerification->DiaDataSourcePresent();
	HRESULT hr = S_OK;

	// Let's allocate enough storage for the complete symbol path and the module path
	unsigned int iSymbolPathWithModulePathPrependedBufferLength = _tcslen(tszSymbolPath) + _MAX_PATH + 1;
	LPTSTR tszSymbolPathWithModulePathPrepended = new TCHAR[iSymbolPathWithModulePathPrependedBufferLength];

	if (tszSymbolPathWithModulePathPrepended == NULL)
		goto cleanup;

	tszSymbolPathWithModulePathPrepended[0] = '\0';

	_tsplitpath(m_tszPEImageModuleFileSystemPath, NULL, NULL, NULL, tszImageExt);

	// Copy the symbol name we're searching for from the Debug Directories
	lptszPointerToPDBName = _tcsrchr(GetDebugDirectoryPDBPath(), '\\');

	// If we don't find a \ char, then go ahead and copy the PDBPath directly...
	if (lptszPointerToPDBName == NULL)
	{
		//_tcscpy(tszPDBModuleName, m_tszDebugDirectoryPDBPath);
		_tcscpy(tszPDBModuleName, GetDebugDirectoryPDBPath());
	} else
	{
		// Advance past the \ char...
		lptszPointerToPDBName = CharNext(lptszPointerToPDBName);
		_tcscpy(tszPDBModuleName, lptszPointerToPDBName);
	}

	if (fDebugSearchPaths)
	{
		_tprintf(TEXT("PDB Search - Looking for [%s] Using Symbol Path...\n"), g_lpProgramOptions->GetMode(CProgramOptions::VerifySymbolsModeWithSymbolPathOnly) ? tszPDBModuleName : GetDebugDirectoryPDBPath());
	};

	if (g_lpProgramOptions->GetMode(CProgramOptions::BuildSymbolTreeMode) )
	{
		// When we are constructing a build symbol tree... we should first
		// search there to see if our symbol is already present...
		
		// Do we do the standard thing?
		if (fDebugSearchPaths)
		{
			_tprintf(TEXT("PDB Search - SEARCH in Symbol Tree We're Building!\n"));
		};

		// Use diaLocatePdb() here when there is support for DIA...
		if (fDiaDataSourcePresent)
		{
			hr = diaLocatePdb( g_lpProgramOptions->GetMode(CProgramOptions::VerifySymbolsModeWithSymbolPathOnly) ? tszPDBModuleName : GetDebugDirectoryPDBPath(), 
							 GetDebugDirectoryPDBGuid(), 
							 GetDebugDirectoryPDBSignature(), 
							 GetDebugDirectoryPDBAge(), 
							 g_lpProgramOptions->GetSymbolTreeToBuild(),
							 &tszImageExt[1], 
							 ipNone);

			fSuccess = SUCCEEDED(hr);

		} else
		{
			_tprintf(TEXT("MSDIA20.DLL is required!\n"));
			fSuccess = false;
		}

		// Hey, if we found it, we're done...
		if (fSuccess)
		{
			goto cleanup;
		}
	}

	if (!g_lpProgramOptions->GetMode(CProgramOptions::VerifySymbolsModeWithSymbolPathOnly))
	{
		// Hey, we better have one or the other...
		if (!m_tszDBGModuleFileSystemPath && !m_tszPEImageModuleFileSystemPath)
			goto cleanup;

		// figure out the home directory of the EXE/DLL or DBG file - pass that along to
		_tfullpath(tszRefPath, m_tszDBGModuleFileSystemPath ? m_tszDBGModuleFileSystemPath : m_tszPEImageModuleFileSystemPath, sizeof(tszRefPath)/sizeof(TCHAR));
		pcEndOfPath = _tcsrchr(tszRefPath, '\\');
		*pcEndOfPath = '\0';        // null terminate it
		*szPDBOut = '\0';

		if (iSymbolPathWithModulePathPrependedBufferLength < (_tcsclen(tszRefPath) + _tcsclen(g_lpProgramOptions->GetSymbolPath())+2))
			goto cleanup;	// Buffer isn't big enough... sigh...

		_tcscat(tszSymbolPathWithModulePathPrepended, tszRefPath);
		_tcscat(tszSymbolPathWithModulePathPrepended, TEXT(";"));
	}

    _tcscat(tszSymbolPathWithModulePathPrepended, tszSymbolPath);

	// Well... let's search the SymbolPath provided for the PDB file...
	if (g_lpProgramOptions->GetMode(CProgramOptions::VerifySymbolsModeWithSymbolPathRecursion))
	{
		if (fDebugSearchPaths)
		{
			_tprintf(TEXT("PDB Search - SEARCH Symbol path with recursion!\n"));
			// ISSUE-2000/07/24-GREGWI: Does FindDebugInfoFileEx2 support SYMSRV?
		};

		// Do we use recursion??? 
		// ISSUE-2000/07/24-GREGWI - Are we passing the right first arg?  Is this supported?
		hModuleHandle = CUtilityFunctions::FindDebugInfoFileEx2(tszPDBModuleName, tszSymbolPathWithModulePathPrepended, VerifyPDBFile, this);

		if (hModuleHandle)
		{
			// We'll assume that a filehandle indicates success...
			CloseHandle(hModuleHandle);
			fSuccess = true;
		}

		// Hey, if we found it, we're done...
		if (fSuccess)
		{
			goto cleanup;
		}

	} else
	{
		if (fDebugSearchPaths)
		{
			_tprintf(TEXT("PDB Search - SEARCH Symbol path!\n"));
		};

		// Use diaLocatePdb() here when there is support for DIA...
		if (fDiaDataSourcePresent)
		{
			hr = diaLocatePdb( g_lpProgramOptions->GetMode(CProgramOptions::VerifySymbolsModeWithSymbolPathOnly) ? tszPDBModuleName : GetDebugDirectoryPDBPath(), 
								GetDebugDirectoryPDBGuid(), 
								GetDebugDirectoryPDBSignature(), 
								GetDebugDirectoryPDBAge(), 
								tszSymbolPathWithModulePathPrepended, 
								&tszImageExt[1], 
								ipNone);

			fSuccess = SUCCEEDED(hr);

		} else
		{
			_tprintf(TEXT("MSDIA20.DLL is required!\n"));
			fSuccess = false;
		}
	}

cleanup:

	if (tszSymbolPathWithModulePathPrepended)
	{
		delete [] tszSymbolPathWithModulePathPrepended;
		tszSymbolPathWithModulePathPrepended = NULL;
	}

	if (fDebugSearchPaths)
	{
		if (GetPDBSymbolModuleStatus() == SYMBOL_MATCH)
		{
			_tprintf(TEXT("PDB Search - Debug Module Found at [%s]\n\n"), m_tszPDBModuleFileSystemPath);
		} else
		{
			_tprintf(TEXT("PDB Search - Debug Module Not Found.\n\n"));
		}
	}

//    if (pdia) {
//			diaRelease(pdia);
//        pdia = NULL;
//}

	return fSuccess;
}

BOOL CModuleInfo::VerifyPDBFile(HANDLE hFileHandle, LPTSTR tszFileName, PVOID CallerData)
{
	bool fSuccess = false;
	TCHAR tszPathToSearch[_MAX_PATH];
	TCHAR tszModulePath[_MAX_DIR];
	TCHAR tszImageExt[_MAX_EXT] = {0}; 	// In case there is no extension we need to null terminate now...
	bool fDiaDataSourcePresent = g_lpSymbolVerification->DiaDataSourcePresent();
	HRESULT hr = E_FAIL;
	UNREFERENCED_PARM(hFileHandle);

	// Let's grab the data passed to us...
	CModuleInfo * lpModuleInfo = (CModuleInfo *) CallerData;		// mjl
	_tsplitpath(tszFileName, tszPathToSearch, tszModulePath, NULL, tszImageExt);
	_tcscat(tszPathToSearch, tszModulePath);
		
	// Use diaLocatePdb() here when there is support for DIA...
	if (fDiaDataSourcePresent)
	{
		hr = lpModuleInfo->diaLocatePdb(	tszFileName, 
											lpModuleInfo->GetDebugDirectoryPDBGuid(), 
											lpModuleInfo->GetDebugDirectoryPDBSignature(), 
											lpModuleInfo->GetDebugDirectoryPDBAge(), 
											tszPathToSearch,
											&tszImageExt[1], 
											ipFirst);

		fSuccess = SUCCEEDED(hr);
		
	} else
	{
		_tprintf(TEXT("MSDIA20.DLL is required!\n"));
		fSuccess = false;
 	}
	
	return fSuccess;
}

// Okay, at this point we need to decide if we're going to save the PDB file we're pointing
// at based on a number of factors...
//
// 1. Is it a match, and does it match any SourceSymbol criteria (-SOURCEONLY, -NOSOURCE etc...)
// 2. Is it better than what we currently have?  (Source enabled better than no source enabled?)

bool CModuleInfo::fCopyNewPDBInformation(bool fSymbolMatch, bool fSourceEnabled)
{
	bool fRetValue = false;
	
	// If this doesn't match, and we already have a symbol... bail
	if ( (m_enumPDBModuleStatus != SYMBOL_NOT_FOUND) && (!fSymbolMatch))
		goto cleanup;
		
	// If we already have a match, is this a better match?  (If not, bail...)
	if (m_enumPDBModuleStatus == SYMBOL_MATCH)
	{
		switch (g_lpProgramOptions->GetSymbolSourceModes())
		{
			// -SOURCE
			case CProgramOptions::enumVerifySymbolsModeSourceSymbolsPreferred:

				// They want Source Enabled but we're already Source Enabled
				// Or the new file is not Source Enabled (so our existing one is just as good)
				if (m_fPDBSourceEnabled || !fSourceEnabled)
					goto cleanup;
				break;

			// -SOURCEONLY
			case CProgramOptions::enumVerifySymbolsModeSourceSymbolsOnly:
				if (m_fPDBSourceEnabled || !fSourceEnabled)
					goto cleanup;
				break;

			// -NOSOURCE
			case CProgramOptions::enumVerifySymbolsModeSourceSymbolsNotAllowed:
				if (!m_fPDBSourceEnabled || fSourceEnabled)
					goto cleanup;
				break;

			// No source preference provided... bail... (we already have a good symbol then)
			case CProgramOptions::enumVerifySymbolsModeSourceSymbolsNoPreference:
				goto cleanup;
				break;
		}
	}

	fRetValue = true;

cleanup:
	
	return fRetValue;

}

//
// Borrowed from dia.c (from sdktools\imagehlp\dia.c) transformed to be TCHAR compatible
//
HRESULT
CModuleInfo::diaLocatePdb(
					LPTSTR  tszPDB,
					GUID *PdbGUID,
					DWORD PdbSignature,
					DWORD PdbAge,
					LPTSTR tszSymbolPath,
					LPTSTR tszImageExt,
					int   ip
					)
{
	HRESULT hr = E_FAIL;

	TCHAR	tszPDBSansPath[_MAX_FNAME];
	TCHAR	tszPDBExt[_MAX_EXT];
	TCHAR	tszPDBLocal[_MAX_PATH];
	LPTSTR	lptszSemiColon;
	DWORD	pass;
	//    EC		hrcode = E_PDB_NOT_FOUND;
	bool	symsrv = true;
	TCHAR	tszPDBName[_MAX_PATH];
	LPTSTR	lptszSavedSymbolPath = tszSymbolPath;
	GUID	guid = {0};
	bool fDebugSearchPaths = g_lpProgramOptions->fDebugSearchPaths();

	// We're going to add our Dia Interfaces here as appropriate...
	CComPtr<IDiaDataSource> pSource = NULL;

	bool	ssfile;
	bool	refpath;
	bool	first;

	if (!PdbSignature && !g_lpSymbolVerification->ValidGUID(PdbGUID))
	{
		return E_PDB_INVALID_SIG;
	}

	// SymbolPath is a semicolon delimited path (reference path first)
	_tcscpy (tszPDBLocal, tszPDB);
	_tsplitpath(tszPDBLocal, NULL, NULL, tszPDBSansPath, tszPDBExt);

	first = true;

	do 
	{
		lptszSemiColon = _tcsstr(tszSymbolPath, TEXT(";"));

		if (lptszSemiColon) 
		{
			*lptszSemiColon = '\0';
		}

		if (first) 
		{
			refpath = (ip == ipFirst);
			first = false;
		} else if (!lptszSemiColon) 
		{
			refpath = (ip == ipLast);
		} else 
		{
			refpath = false;
		}

		if (refpath) 
		{
			pass = 2;
			ip = ipNone;
		} else 
		{
			pass = 0;
		}

		if (tszSymbolPath) 
		{
do_again:
			ssfile = false;

			// Let's create a DIA Session interface pointer if necessary...
			hr = diaGetDiaDataSource(pSource);

			if (FAILED(hr))
			{
				goto cleanup;
			}

			if (!_tcsncicmp(tszSymbolPath, TEXT("SYMSRV*"), 7)) 
			{
				*tszPDBLocal = 0;
				_stprintf(tszPDBName, TEXT("%s%s"), tszPDBSansPath, TEXT(".pdb"));

				if (symsrv)
				{
					ssfile = true;
					if (PdbSignature)
						guid.Data1 = PdbSignature;
					else if (PdbGUID)
						memcpy(&guid, PdbGUID, sizeof(GUID));

					// Let's invoke DBGHELP.DLL's SymFindFileInPath to query the symbol server
					{
						char szSymbolPath[_MAX_PATH];
						char szPDBLocal[_MAX_PATH];
						char szPDBName[_MAX_FNAME];

						if (fDebugSearchPaths)
						{
							wchar_t wszPDBGuid[39];
							TCHAR	tszPDBGuid[39];

							// We're going to convert the binary GUID format to a string for easier printing
							StringFromGUID2(guid, wszPDBGuid, sizeof(wszPDBGuid)/sizeof(wchar_t));
							CUtilityFunctions::CopyUnicodeStringToTSTR(wszPDBGuid, tszPDBGuid, 39);

							_tprintf(TEXT("PDB Search - SYMSRV [%s,%s,%s,0x%x]\n"),
							tszSymbolPath,
							tszPDBName,
							tszPDBGuid,
							PdbAge);
						}

						CUtilityFunctions::CopyTSTRStringToAnsi(tszSymbolPath, szSymbolPath, _MAX_PATH);
						CUtilityFunctions::CopyTSTRStringToAnsi(tszPDBName, szPDBName, _MAX_FNAME);

						if (SymFindFileInPath(
									NULL,
									szSymbolPath,
									szPDBName,
									&guid,
									PdbAge,
									0,
									SSRVOPT_GUIDPTR,	//Flags
									szPDBLocal,
									NULL,
									NULL))
						{
							// Copy the string back...
							CUtilityFunctions::CopyAnsiStringToTSTR(szPDBLocal, tszPDBLocal, _MAX_PATH);
						}
					}
					symsrv = false;
				}
			} else {

			_tcscpy(tszPDBLocal, tszSymbolPath);
			CUtilityFunctions::EnsureTrailingBackslash(tszPDBLocal);

			// search order is ...
			//
			//   %dir%\symbols\%ext%\%file%
			//   %dir%\%ext%\%file%
			//   %dir%\%file%

			switch (pass)
			{
				case 0:
					_tcscat(tszPDBLocal, TEXT("symbols"));
					CUtilityFunctions::EnsureTrailingBackslash(tszPDBLocal);
					// pass through
				case 1:
					_tcscat(tszPDBLocal, tszImageExt);
					// pass through
				default:
					CUtilityFunctions::EnsureTrailingBackslash(tszPDBLocal);
				break;
			}

			_tcscat(tszPDBLocal, tszPDBSansPath);
			_tcscat(tszPDBLocal, tszPDBExt);

			}

			if (*tszPDBLocal) 
			{
				if (fDebugSearchPaths)
				{
					_tprintf(TEXT("PDB Search - Search here [%s]\n"), tszPDBLocal);
				}

				// Probe for file (it's lame but we want to see if you have connectivity
				hr = CUtilityFunctions::VerifyFileExists(TEXT("PDB Search - Failed to open [%s]!  "), tszPDBLocal);

				// Search only if we know we can see the file... save an expensive call
				if (hr == S_OK)
				{
					hr = diaGetPdbInfo(pSource, tszPDBLocal, g_lpSymbolVerification->ValidGUID(PdbGUID) ? PdbGUID : NULL, PdbSignature, PdbAge);
				}

				if (hr == S_OK) 
				{
					break;
				} else 
				{
					if (pass < 2) 
					{
						pass++;
						goto do_again;
					}
				}
				refpath = false;
				}
			}

			if (lptszSemiColon) 
			{
				*lptszSemiColon = ';';
				lptszSemiColon++;
				symsrv = true;
			}

			tszSymbolPath = lptszSemiColon;
	} while (lptszSemiColon);

	if (hr != S_OK) 
	{
		_tcscpy(tszPDBLocal, tszPDB);

		if (fDebugSearchPaths)
		{
			_tprintf(TEXT("PDB Search - Search here [%s]\n"), tszPDBLocal);
		}

		// Let's create a DIA Session interface pointer if necessary...
		hr = diaGetDiaDataSource(pSource);

		if (FAILED(hr))
		{
			goto cleanup;
		}

		// Probe for file (it's lame but we want to see if you have connectivity
		hr = CUtilityFunctions::VerifyFileExists(TEXT("PDB Search - Failed to open [%s]!  "), tszPDBLocal);

		// Search only if we know we can see the file... save an expensive call
		if (hr == S_OK)
		{
			hr = diaGetPdbInfo(pSource, tszPDBLocal, g_lpSymbolVerification->ValidGUID(PdbGUID) ? PdbGUID : NULL, PdbSignature, PdbAge);
		}
	}

	// TODO: Why is this here???
	/*
	if (hr == S_OK) {
	// Store the name of the PDB we actually opened for later reference.
	//		_tcscpy(tszPDB, tszPDBLocal);
	_tcscpy(tszPDBLocal, tszPDB);
	}
	*/

	// Only try this last check if we're not bound to our Symbol Path Only
	if (!g_lpProgramOptions->GetMode(CProgramOptions::VerifySymbolsModeWithSymbolPathOnly))
	{
		if (hr != S_OK && (PdbSignature || g_lpSymbolVerification->ValidGUID(PdbGUID)))
		{
			// Release as necessary...
			if (pSource)
			pSource = NULL;

			return diaLocatePdb(/*pIDD, */ tszPDB, NULL, 0, 0, lptszSavedSymbolPath, tszImageExt, ipNone);
		}
	}

cleanup:

	// Release as necessary...
	if (pSource)
		pSource = NULL;

	return hr;
}

HRESULT CModuleInfo::diaGetPdbInfo(CComPtr<IDiaDataSource> & pSource, LPTSTR tszPdbPath, GUID *PdbGUIDToMatch, DWORD PdbSignatureToMatch, DWORD PdbAgeToMatch)
{
	CComPtr<IDiaSession> pSession = NULL;
	CComPtr<IDiaSymbol> pSymbols = NULL;

	// Used when examining PDB files...
	DWORD dwSourceFiles = 0;
	bool fSourceEnabled = false;
	HRESULT hr = S_OK;
	HRESULT hr_retval = S_OK;
	bool fSymbolFileOpened = false;
	bool fSymbolMatch = false;

	WCHAR wszPDBPath[_MAX_PATH + 1];
	CUtilityFunctions::CopyTSTRStringToUnicode(tszPdbPath, wszPDBPath, _MAX_PATH + 1);

	//
	// First, let's just try and validate that we've got the right PDB file!
	//
	hr_retval = pSource->loadAndValidateDataFromPdb(wszPDBPath, g_lpSymbolVerification->ValidGUID(PdbGUIDToMatch) ? PdbGUIDToMatch : NULL, PdbSignatureToMatch, PdbAgeToMatch);

	fSymbolMatch = (hr_retval == S_OK);

	switch (hr_retval)
	{
		// We can work with these returns...
		case S_OK:
			fSymbolFileOpened = true;
			break;

		case E_PDB_INVALID_SIG:
		case E_PDB_INVALID_AGE:
			
			if (FAILED(hr = pSource->loadDataFromPdb(wszPDBPath)))
			{
				_tprintf(TEXT("Unable to open [%s] pdb file!  hr=0x%x\r\n"), tszPdbPath, hr);
				hr_retval = hr;
				goto cleanup;
			} else
			{
				fSymbolFileOpened = true;
			}
				

			break;

		// All else are fatal for us...			

#ifdef _DEBUG
//		case E_PDB_NOT_FOUND:
//			_tprintf(TEXT("PDB Not Found!  hr=0x%x\r\n"), hr);
//			goto cleanup;
//
//		case E_PDB_FORMAT:
//			_tprintf(TEXT("PDB Format Not Recognized!  hr=0x%x\r\n"), hr);
//			goto cleanup;
//
//		case E_UNEXPECTED:
//			_tprintf(TEXT("PDB Data Source already prepared!  hr=0x%x\r\n"), hr);
//			goto cleanup;
#endif

		default:
			goto cleanup;
	}

	// Get a Session Interface
	if (FAILED(hr = pSource->openSession(&pSession)))
	{
		_tprintf(TEXT("Unable to get PDB Session [%s] hr = 0x%x\r\n"), tszPdbPath, hr);
		hr_retval = hr;
		goto cleanup;
	}

	// Get a Symbols Interface
	if (FAILED(hr = pSession->get_globalScope(&pSymbols)))
	{
		_tprintf(TEXT("Unable to get PDB globalScope [%s] hr = 0x%x\r\n"), tszPdbPath, hr);
		hr_retval = hr;
		goto cleanup;
	}

	// Collect Source Info from PDB (if any) -- errors are tolerated for this call
	hr = diaPdbSourceEnabled(pSession, fSourceEnabled, dwSourceFiles);

	// With the New PDB found, do we want to record it into our ModuleInfo Object?
	if (!fCopyNewPDBInformation(fSymbolMatch, fSourceEnabled))
	{
		hr_retval = E_FAIL;	// This ensures we continue searching for a better hit...
		goto cleanup;
	}
	
	// On a match, let's populate what we know to be true
	if (fSymbolMatch)
	{
		// Did we get what we really wanted...		
		if ( (fSourceEnabled && (g_lpProgramOptions->GetSymbolSourceModes() == CProgramOptions::enumVerifySymbolsModeSourceSymbolsNotAllowed)) ||
			 (!fSourceEnabled && (g_lpProgramOptions->GetSymbolSourceModes() == CProgramOptions::enumVerifySymbolsModeSourceSymbolsOnly)) )
		{
			// Not perfect... we don't want this... even though it matches...
			m_enumPDBModuleStatus = SYMBOL_POSSIBLE_MISMATCH;
			m_fPDBSourceEnabledMismatch = true;
			hr_retval = E_FAIL;	// This ensures we continue searching for a better hit...
		} else
		{
			// Perfect...
			m_enumPDBModuleStatus = SYMBOL_MATCH;
			m_fPDBSourceEnabledMismatch = false;
			
			// If this is not source enabled... and -SOURCE was specified... continue searching
			// for a better hit!!!
			if (!fSourceEnabled && (g_lpProgramOptions->GetSymbolSourceModes() == CProgramOptions::enumVerifySymbolsModeSourceSymbolsPreferred))
			{
				hr_retval = E_FAIL;	// This ensures we continue searching for a better hit...
			}
		}

		// For now, save the results...
		m_fPDBSourceEnabled = fSourceEnabled;
		m_dwPDBTotalSourceFiles = dwSourceFiles;

		// On a perfect match, these must be equal...
		m_dwPDBSignature 		= PdbSignatureToMatch;
		m_dwPDBAge 				= PdbAgeToMatch;
		m_dwPDBFormatSpecifier 	= GetDebugDirectoryPDBFormatSpecifier(); // Assumed
		
		if (g_lpSymbolVerification->ValidGUID(PdbGUIDToMatch))
		{
			memcpy(&m_guidPDBGuid, &PdbGUIDToMatch, sizeof(m_guidPDBGuid));
		}

		m_tszPDBModuleFileSystemPath = CUtilityFunctions::CopyString(tszPdbPath, m_tszPDBModuleFileSystemPath);

		if (!m_tszPDBModuleFileSystemPath)
			return false;

		// Now that we're done verifying the module... do we save the symbol in
		// our tree?
		if ( g_lpProgramOptions->GetMode(CProgramOptions::CopySymbolsToImage) )
		{
			// Though we matched the PDB... we may not have correct PDB (source enabled, etc...)
			if (m_enumPDBModuleStatus == SYMBOL_MATCH)
			{
				// Yup...
				CUtilityFunctions::CopySymbolFileToImagePath(m_tszPEImageModuleFileSystemPath, &m_tszPDBModuleFileSystemPath);
			}
		}
		if ( g_lpProgramOptions->GetMode(CProgramOptions::BuildSymbolTreeMode) )
		{
			// Though we matched the PDB... we may not have correct PDB (source enabled, etc...)
			if (m_enumPDBModuleStatus == SYMBOL_MATCH)
			{
				// Yup...
				CUtilityFunctions::CopySymbolFileToSymbolTree(m_tszPEImageModuleName, &m_tszPDBModuleFileSystemPath, g_lpProgramOptions->GetSymbolTreeToBuild());
			}
		}

	}else
	{
		// We only record the first mismatch...
		if (m_enumPDBModuleStatus == SYMBOL_NOT_FOUND)
		{
			// Let's record this PDB file as a "hit" though not perfect...
			m_enumPDBModuleStatus = SYMBOL_POSSIBLE_MISMATCH;

			m_tszPDBModuleFileSystemPath = CUtilityFunctions::CopyString(tszPdbPath, m_tszPDBModuleFileSystemPath);

			if (!m_tszPDBModuleFileSystemPath)
				return false;

			// First, let's try the PDB Signature...
			if (FAILED(hr = pSymbols->get_signature(&m_dwPDBSignature)))
			{
				_tprintf(TEXT("Unable to get PDB signature from PDB file [%s] hr = 0x%x\r\n"), tszPdbPath, hr);
				hr_retval = hr;
				goto cleanup;
			}

			// Second, the PDB Guid...
			if (FAILED(hr = pSymbols->get_guid(&m_guidPDBGuid)))
			{
				_tprintf(TEXT("Unable to get PDB Guid from PDB file [%s] hr = 0x%x\r\n"), tszPdbPath, hr);
				hr_retval = hr;
				goto cleanup;
			}

			// Third, the PDB Age...
			if (FAILED(hr = pSymbols->get_age(&m_dwPDBAge)))
			{
				_tprintf(TEXT("Unable to get PDB Age from PDB file [%s] hr = 0x%x\r\n"), tszPdbPath, hr);
				hr_retval = hr;
				goto cleanup;
			}

			// Fourth... we need to identify if this is a sigNB10 or sigRSDS format...

			// One simple test is to look at the GUID itself... DIA returns the signature in
			// the first DWORD of the GUID, and the rest of the members are 0.

			if (g_lpSymbolVerification->diaOldPdbFormatFound(&m_guidPDBGuid, m_dwPDBSignature))
			{
				m_dwPDBFormatSpecifier = sigNB10;
			} else
			{
				m_dwPDBFormatSpecifier = sigRSDS;
			}

 			// For now, save the results...
			m_fPDBSourceEnabled = fSourceEnabled;
			m_dwPDBTotalSourceFiles = dwSourceFiles;

		} else
		{
			// We have to bail since this PDB doesn't match, and we found another already...
			goto cleanup;
		}
	}

cleanup:

	if (fSymbolFileOpened)
	{
		// Free this resource to allow for a new one on the next PDB file
		if (pSource)
			pSource = NULL;
	}
	if (pSession)
		pSession = NULL;

	if (pSymbols)
		pSymbols = NULL;

	return hr_retval;
}

HRESULT CModuleInfo::diaPdbSourceEnabled(CComPtr<IDiaSession> & pSession, bool & fSourceEnabled, DWORD & dwSourceFiles)
{
	CComPtr<IDiaEnumTables> pTables = NULL;
	CComPtr<IDiaTable > pTable = NULL;
	CComPtr<IDiaSourceFile> pSourceFile = NULL;
	CComPtr<IDiaEnumSourceFiles> pSourceFiles = NULL;

	LONG TotalCountOfSourceFiles = 0;
	ULONG celt = 0;
	HRESULT hr = S_OK;
	fSourceEnabled = false;
	dwSourceFiles = 0;

	// First, query for tables in the PDB file...
	if ( FAILED( pSession->getEnumTables( &pTables ) ) )
	{
        goto cleanup;
	}

	// Second, loop through each table and get a pointer to the table itself
    while ( SUCCEEDED( hr = pTables->Next( 1, &pTable, &celt ) ) && celt == 1 ) 
	{

//		Tablenum++;

/*		// Print the name of the table...
		if (SUCCEEDED(hr = pTable->get_name( &name )))
		{
			printf("Table[%d] = %ws\n", Tablenum, name);
			SysFreeString( name );
		}
*/
		// Check to see if this table has an interface to SourceFiles
		if ( SUCCEEDED( pTable->QueryInterface( IID_IDiaEnumSourceFiles, (void**)&pSourceFiles ) ) ) 
		{
			LONG CountOfSourceFiles;

			if (SUCCEEDED( hr = pSourceFiles->get_Count(&CountOfSourceFiles)))
			{
				TotalCountOfSourceFiles =+ CountOfSourceFiles;
/*
#ifdef _DEBUG
				_tprintf(TEXT("Number of source files = %d\n"), CountOfSourceFiles);
#endif
*/
			}
/*
            while ( SUCCEEDED( hr = pSourceFiles->Next( 1, &pSourceFile, &celt ) ) && celt == 1 ) 
			{
				BSTR name;
				fSourceEnabled = true;

				// Get the filename itself...
                pSourceFile->get_fileName( &name );

                if ( name != NULL ) 
				{
                    printf( "  File Name: %ws\n", name );
                    SysFreeString( name );
                }

                pSourceFile = NULL;
            }
*/
			pSourceFiles = NULL;
		}

        pTable = NULL;
	}

	// If we actually found source files in the PDB, then we're source enabled...
	if (TotalCountOfSourceFiles)
	{
		fSourceEnabled = true;
		dwSourceFiles = TotalCountOfSourceFiles;
	}


cleanup:

	if (pTables)
		pTables = NULL;

	if (pTable)
		pTable = NULL;

	if (pSourceFile)
		pSourceFile = NULL;

	if (pSourceFiles)
		pSourceFiles = NULL;

	return hr;
}

HRESULT CModuleInfo::diaGetDiaDataSource(CComPtr<IDiaDataSource> & pSource)
{
	HRESULT hr = S_OK;

	// Let's create a DIA Session interface pointer if necessary...
	if (pSource == NULL)
	{
		hr = g_lpSymbolVerification->diaGetDataSource(pSource);

		if (FAILED(hr))
		{
			_tprintf(TEXT("Unable to acquire DiaSource (hr=0x%x)\n"), hr);
			goto cleanup;
		}
	}

cleanup:

	return hr;
}
BOOL CModuleInfo::VerifyDBGFile(HANDLE hFileHandle, LPTSTR tszFileName, PVOID CallerData)
{
	CModuleInfo * lpModuleInfo = (CModuleInfo * )CallerData;
	WORD wMagic;				// Read to identify a DBG file...
	bool fPerfectMatch = false;	// Assume we don't have a good DBG match...

	// DBG Image Locals
	IMAGE_SEPARATE_DEBUG_HEADER ImageSeparateDebugHeader;

	// Start at the top of the image...
	lpModuleInfo->SetReadPointer(false, hFileHandle, 0, FILE_BEGIN);

	// Read in a signature word... is this a DBG file?
	if ( !lpModuleInfo->DoRead(false, hFileHandle, &wMagic, sizeof(wMagic) ) )
		goto cleanup;

	// No sense in going further since we're expecting a DBG image file...
	if (wMagic != IMAGE_SEPARATE_DEBUG_SIGNATURE)
		goto cleanup;

	// Start at the top of the image...
	lpModuleInfo->SetReadPointer(false, hFileHandle, 0, FILE_BEGIN);

	// Read in the full Separate Debug Header
	if ( !lpModuleInfo->DoRead(false, hFileHandle, &ImageSeparateDebugHeader, sizeof(ImageSeparateDebugHeader) ) )
		goto cleanup;

	//
	// We have a more stringent requirement for matching the checksum if the verification level is set...
	//
	if ( lpModuleInfo->m_dwPEImageTimeDateStamp == ImageSeparateDebugHeader.TimeDateStamp)
	{
		// For localized builds... these get altered... enforce this check here?
		if (g_lpProgramOptions->GetVerificationLevel() == 2)
		{
			if (	lpModuleInfo->m_dwPEImageCheckSum == ImageSeparateDebugHeader.CheckSum &&
				lpModuleInfo->m_dwPEImageSizeOfImage == ImageSeparateDebugHeader.SizeOfImage)
			{
				fPerfectMatch = true;
			}
		} else
		{
			fPerfectMatch = true;
		}
	}

	//
	// We're going to perform some action below unless this is not a perfect match
	// and we've already picked up a "bad" DBG file reference...
	//
	if (!fPerfectMatch && lpModuleInfo->m_tszDBGModuleFileSystemPath)
		goto cleanup;

	//
	// Take action based on our results...
	// 1. If we have a perfect match... save our stuff!
	// 2. If we don't already have a DBG, go ahead and save (even if wrong)
	//

	// Save off the checksum/linker information...
	lpModuleInfo->m_dwDBGTimeDateStamp = ImageSeparateDebugHeader.TimeDateStamp;
	lpModuleInfo->m_dwDBGCheckSum = ImageSeparateDebugHeader.CheckSum;
	lpModuleInfo->m_dwDBGSizeOfImage = ImageSeparateDebugHeader.SizeOfImage;

	lpModuleInfo->m_tszDBGModuleFileSystemPath = CUtilityFunctions::CopyString(tszFileName, lpModuleInfo->m_tszDBGModuleFileSystemPath);

	// Delete any PDB reference we may have found in our last DBG file (if any)...
	if (lpModuleInfo->m_tszDBGDebugDirectoryPDBPath)
	{
		delete [] lpModuleInfo->m_tszDBGDebugDirectoryPDBPath;
		lpModuleInfo->m_tszDBGDebugDirectoryPDBPath = NULL;
	}

	//
	// At this point, we only continue on if we've made a perfect "hit"
	//
	if (!fPerfectMatch)
	{
		// Not a perfect symbol.. record the status and exit...
		lpModuleInfo->m_enumDBGModuleStatus = SYMBOL_POSSIBLE_MISMATCH;

		goto cleanup;
	}

	// Good symbol.. record this...
	lpModuleInfo->m_enumDBGModuleStatus = SYMBOL_MATCH;

	// Now that we're done verifying the module... do we save the symbol in...
	if ( g_lpProgramOptions->GetMode(CProgramOptions::CopySymbolsToImage) )
	{
		// Our module path?
		CUtilityFunctions::CopySymbolFileToImagePath(lpModuleInfo->m_tszPEImageModuleFileSystemPath, &lpModuleInfo->m_tszDBGModuleFileSystemPath);
	} else
	if ( g_lpProgramOptions->GetMode(CProgramOptions::BuildSymbolTreeMode) )
	{
		// Out Symbol Tree?
		CUtilityFunctions::CopySymbolFileToSymbolTree(lpModuleInfo->m_tszPEImageModuleName, &lpModuleInfo->m_tszDBGModuleFileSystemPath, g_lpProgramOptions->GetSymbolTreeToBuild());
	}

	//
	// Okay, now with a good symbol let's extract the goods...
	//

	// If there's no debug info, we can't continue further.
	if (ImageSeparateDebugHeader.DebugDirectorySize == 0)
	{
		goto cleanup;
	}

	// Okay, we need to advance by the IMAGE_SECTION_HEADER...
	lpModuleInfo->SetReadPointer(false, hFileHandle, (ImageSeparateDebugHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER)), FILE_CURRENT);

	// Skip over the exported names.
	if (ImageSeparateDebugHeader.ExportedNamesSize)
	{
		lpModuleInfo->SetReadPointer(false, hFileHandle, ImageSeparateDebugHeader.ExportedNamesSize, FILE_CURRENT);
	}
		
	if (!lpModuleInfo->ProcessDebugDirectory(false, false, hFileHandle, ImageSeparateDebugHeader.DebugDirectorySize, lpModuleInfo->GetReadPointer()))
		goto cleanup;

cleanup:

	return (fPerfectMatch ? TRUE : FALSE);
}

bool CModuleInfo::OutputDataToFile(LPTSTR tszProcessName, DWORD iProcessID)
{

	LPTSTR tszString = NULL;

	bool fReturn = false;

	// We remove the first three columns if -E was specified...
	if (!g_lpProgramOptions->GetMode(CProgramOptions::ExceptionMonitorMode))
	{
		// Let's skip the first column to make room for the tag in the first column...
		if (!m_lpOutputFile->WriteString(TEXT(",")))
			goto cleanup;

		//
		// Process Name
		//
		if (tszProcessName)
		{
			if (!m_lpOutputFile->WriteString(tszProcessName, true))
				goto cleanup;
		}

		if (!m_lpOutputFile->WriteString(TEXT(",")))
			goto cleanup;

		if (tszProcessName) 
		{
			// Let's only emit the PID if there is a process name...
			if (!m_lpOutputFile->WriteDWORD(iProcessID))
				goto cleanup;
		}

		if (!m_lpOutputFile->WriteString(TEXT(",")))
			goto cleanup;

	}

	// if -E is specified, we only spit out if the module has a problem
	if ( g_lpProgramOptions->GetMode(CProgramOptions::ExceptionMonitorMode) )
	{

		switch (m_enumPEImageSymbolStatus)
		{
			case SYMBOLS_DBG:
				if ( m_enumDBGModuleStatus == SYMBOL_MATCH)
				{
					// Don't print this out.. it matches...
					fReturn = true;
					goto cleanup;
				}
				break;

			case SYMBOLS_DBG_AND_PDB:
				if ( m_enumDBGModuleStatus == SYMBOL_MATCH &&
					 m_enumPDBModuleStatus == SYMBOL_MATCH )
				{
					// Don't print this out.. it matches...
					fReturn = true;
					goto cleanup;
				}
				
				break;

			case SYMBOLS_PDB:
				if ( m_enumPDBModuleStatus == SYMBOL_MATCH)
				{
					// Don't print this out.. it matches...
					fReturn = true;
					goto cleanup;
				}
				break;
		}
	}

	//
	//  Module Path
	//
	if (m_tszPEImageModuleFileSystemPath)
	{
		if (!m_lpOutputFile->WriteString(m_tszPEImageModuleFileSystemPath, true))
			goto cleanup;
	}

	if (!m_lpOutputFile->WriteString(TEXT(",")))
		goto cleanup;

	//
	// Symbol Status
	//
	if (m_enumPEImageSymbolStatus != SYMBOL_INFORMATION_UNKNOWN)
	{
		tszString = SymbolInformationString(m_enumPEImageSymbolStatus);

		if (tszString)
		{
			if (!m_lpOutputFile->WriteString(tszString))
				goto cleanup;
		}
	}

	if (!m_lpOutputFile->WriteString(TEXT(",")))
		goto cleanup;

	// We remove this column if -E is specified
	if (!g_lpProgramOptions->GetMode(CProgramOptions::ExceptionMonitorMode))
	{
		//
		//  Checksum
		//
		if ( m_enumPEImageSymbolStatus != SYMBOL_INFORMATION_UNKNOWN )
		{
			if (!m_lpOutputFile->WriteDWORD(m_dwPEImageCheckSum))
				goto cleanup;
		}

		if (!m_lpOutputFile->WriteString(TEXT(",")))
			goto cleanup;
		
		//
		//  Time/Date Stamp
		//
		if ( m_enumPEImageSymbolStatus != SYMBOL_INFORMATION_UNKNOWN )
		{
			if (!m_lpOutputFile->WriteDWORD(m_dwPEImageTimeDateStamp))
				goto cleanup;
		}

		if (!m_lpOutputFile->WriteString(TEXT(",")))
			goto cleanup;

	}

	//
	//  Time/Date String
	//
	// If -E is specified we'll use version2 of the output format...
	if (!g_lpProgramOptions->GetMode(CProgramOptions::ExceptionMonitorMode))
	{
		if ( m_enumPEImageSymbolStatus != SYMBOL_INFORMATION_UNKNOWN )
		{
			if (!m_lpOutputFile->WriteTimeDateString(m_dwPEImageTimeDateStamp))
				goto cleanup;
		}
	} else
	{
			if (!m_lpOutputFile->WriteTimeDateString2(m_dwPEImageTimeDateStamp))
				goto cleanup;
	}

	if (!m_lpOutputFile->WriteString(TEXT(",")))
		goto cleanup;

	// We remove these columns if -E is specified
	if (!g_lpProgramOptions->GetMode(CProgramOptions::ExceptionMonitorMode))
	{
		//
		// Size Of Image (internal PE value) - used for SYMSRV support
		//
		if (!m_lpOutputFile->WriteDWORD(m_dwPEImageSizeOfImage))
			goto cleanup;

		if (!m_lpOutputFile->WriteString(TEXT(",")))
			goto cleanup;

		//
		//  DBG Pointer
		//
		if ( m_enumPEImageSymbolStatus == SYMBOLS_DBG ||
			 m_enumPEImageSymbolStatus == SYMBOLS_DBG_AND_PDB )
		{
			// Output the Path	
			if (m_tszPEImageDebugDirectoryDBGPath)
			{
				if (!m_lpOutputFile->WriteString(m_tszPEImageDebugDirectoryDBGPath, true))
					goto cleanup;
			}
		}

		if (!m_lpOutputFile->WriteString(TEXT(",")))
			goto cleanup;

		//
		//  PDB Pointer
		//
		if ( m_enumPEImageSymbolStatus == SYMBOLS_PDB )
		{
			// Output the Path	
			if (GetDebugDirectoryPDBPath())
			{
				if (!m_lpOutputFile->WriteString(GetDebugDirectoryPDBPath(), true))
					goto cleanup;
			}
		}

		if (!m_lpOutputFile->WriteString(TEXT(",")))
			goto cleanup;

		//
		//  PDB Signature
		//
		if ( m_enumPEImageSymbolStatus == SYMBOLS_PDB )
		{
			switch (m_dwPEImageDebugDirectoryPDBFormatSpecifier)
			{
				case sigNB10:
					// Output the Signature		
					if (!m_lpOutputFile->WriteDWORD(m_dwPEImageDebugDirectoryPDBSignature))
						goto cleanup;
					break;

				case sigRSDS:
					// Output the GUID
					if (!m_lpOutputFile->WriteGUID(m_guidPEImageDebugDirectoryPDBGuid))
						goto cleanup;
					
					break;
			}
		}

		if (!m_lpOutputFile->WriteString(TEXT(",")))
			goto cleanup;

		//
		//  PDB Age
		//
		if ( m_enumPEImageSymbolStatus == SYMBOLS_PDB )
		{
			if (!m_lpOutputFile->WriteDWORD(m_dwPEImageDebugDirectoryPDBAge))
				goto cleanup;
		}

		if (!m_lpOutputFile->WriteString(TEXT(",")))
			goto cleanup;

	}

	//
	//  Product Version
	//
	// We remove these columns if -E is specified
	if (!g_lpProgramOptions->GetMode(CProgramOptions::ExceptionMonitorMode))
	{
		if (m_fPEImageFileVersionInfo && m_tszPEImageProductVersionString)
		{
			if (!m_lpOutputFile->WriteString(m_tszPEImageProductVersionString))
				goto cleanup;
		}

		if (!m_lpOutputFile->WriteString(TEXT(",")))
			goto cleanup;
	}

	//
	//  File Version
	//
	if (m_fPEImageFileVersionInfo && m_tszPEImageFileVersionString)
	{
		if (!m_lpOutputFile->WriteString(m_tszPEImageFileVersionString))
			goto cleanup;
	}

	if (!m_lpOutputFile->WriteString(TEXT(",")))
		goto cleanup;
	
	//
	//  Company Name
	//
	if (m_fPEImageFileVersionInfo && m_tszPEImageFileVersionCompanyName)
	{
		if (!m_lpOutputFile->WriteString(m_tszPEImageFileVersionCompanyName, true))
				goto cleanup;
	}

	if (!m_lpOutputFile->WriteString(TEXT(",")))
		goto cleanup;

	//
	//   File Description
	//
	if (m_fPEImageFileVersionInfo && m_tszPEImageFileVersionDescription)
	{
		if (!m_lpOutputFile->WriteString(m_tszPEImageFileVersionDescription, true))
				goto cleanup;
	}

	if (!m_lpOutputFile->WriteString(TEXT(",")))
		goto cleanup;

	// We remove these columns if -E is specified
	if (!g_lpProgramOptions->GetMode(CProgramOptions::ExceptionMonitorMode))
	{
		//
		// File Size (in bytes)
		//
		if ( m_dwPEImageFileSize )
		{
			if (!m_lpOutputFile->WriteDWORD(m_dwPEImageFileSize))
					goto cleanup;
		}

		if (!m_lpOutputFile->WriteString(TEXT(",")))
			goto cleanup;

		// File Date High Word
		if ( m_ftPEImageFileTimeDateStamp.dwLowDateTime ||
			 m_ftPEImageFileTimeDateStamp.dwHighDateTime )
		{
			if (!m_lpOutputFile->WriteDWORD(m_ftPEImageFileTimeDateStamp.dwHighDateTime))
					goto cleanup;
		}

		if (!m_lpOutputFile->WriteString(TEXT(",")))
			goto cleanup;

		// File Date Low Word
		if ( m_ftPEImageFileTimeDateStamp.dwLowDateTime ||
			 m_ftPEImageFileTimeDateStamp.dwHighDateTime )
		{
			if (!m_lpOutputFile->WriteDWORD(m_ftPEImageFileTimeDateStamp.dwLowDateTime))
					goto cleanup;
		}

		if (!m_lpOutputFile->WriteString(TEXT(",")))
			goto cleanup;

	}

	// File Date String
	if ( m_ftPEImageFileTimeDateStamp.dwLowDateTime ||
		 m_ftPEImageFileTimeDateStamp.dwHighDateTime )
	{
		// If -E is specified we'll use version2 of the output format...
		if (!g_lpProgramOptions->GetMode(CProgramOptions::ExceptionMonitorMode))
		{
			if (!m_lpOutputFile->WriteFileTimeString(m_ftPEImageFileTimeDateStamp))
				goto cleanup;
		} else
		{
			if (!m_lpOutputFile->WriteFileTimeString2(m_ftPEImageFileTimeDateStamp))
				goto cleanup;
		}
	}

	if (!m_lpOutputFile->WriteString(TEXT(",")))
		goto cleanup;

	if (g_lpProgramOptions->GetMode(CProgramOptions::VerifySymbolsMode))
	{
		//
		//  Local DBG Status
		//
		if ( (m_enumPEImageSymbolStatus == SYMBOLS_DBG) || (m_enumPEImageSymbolStatus == SYMBOLS_DBG_AND_PDB) )
		{

			tszString = SymbolModuleStatusString(m_enumDBGModuleStatus);

			if (tszString)
			{
				if (!m_lpOutputFile->WriteString(tszString))
					goto cleanup;
			}

		}

		if (!m_lpOutputFile->WriteString(TEXT(",")))
			goto cleanup;

		//
		//  Local DBG
		//
		if ( (m_enumPEImageSymbolStatus == SYMBOLS_DBG) || (m_enumPEImageSymbolStatus == SYMBOLS_DBG_AND_PDB) )
		{
			if (m_tszDBGModuleFileSystemPath)
			{
				if (!m_lpOutputFile->WriteString(m_tszDBGModuleFileSystemPath, true))
					goto cleanup;
			}
		}

		if (!m_lpOutputFile->WriteString(TEXT(",")))
			goto cleanup;

		//
		//  Local PDB Status
		//
		if ( (m_enumPEImageSymbolStatus == SYMBOLS_DBG_AND_PDB) || (m_enumPEImageSymbolStatus == SYMBOLS_PDB) )
		{
			tszString = SymbolModuleStatusString(m_enumPDBModuleStatus);
		
			if (tszString)
			{
				if (!m_lpOutputFile->WriteString(tszString))
					goto cleanup;
			}
		}

		if (!m_lpOutputFile->WriteString(TEXT(",")))
			goto cleanup;

		//
		//  Local PDB
		//
		if ( (m_enumPEImageSymbolStatus == SYMBOLS_DBG_AND_PDB) || (m_enumPEImageSymbolStatus == SYMBOLS_PDB) )
		{
			if (m_tszPDBModuleFileSystemPath)
			{
				if (!m_lpOutputFile->WriteString(m_tszPDBModuleFileSystemPath, true))
					goto cleanup;
			}
		}
	}
	// Write the carriage-return line-feed at the end of the line...
	if (!m_lpOutputFile->WriteString(TEXT("\r\n")))
		goto cleanup;

	fReturn = true; // Success

cleanup:

	if (!fReturn)
	{
		_tprintf(TEXT("Error: Failure writing module data!\n"));
		m_lpOutputFile->PrintLastError();
	}
		
	return fReturn;
}

bool CModuleInfo::OutputDataToStdout(DWORD dwModuleNumber)
{
	//
	// Do we output this module?
	//
	if (!OutputDataToStdoutThisModule())
		return false;
	
	//
	// First, Output Module Info
	//
	OutputDataToStdoutModuleInfo(dwModuleNumber);

	bool fPrintCarriageReturn = false;

	// Provide -PERF output if requested...
	if (g_lpProgramOptions->GetMode(CProgramOptions::OutputModulePerf))
	{
		if (m_dw64BaseAddressOfLoadedImage != m_dw64PreferredLoadAddress)
		{
			_tprintf(TEXT("  PERFORMANCE ISSUE DUE TO RELOCATION\n"));
			_tprintf(TEXT("  Module Load Address    = 0x%08I64x\n"), m_dw64BaseAddressOfLoadedImage);
			_tprintf(TEXT("  Preferred Load Address = 0x%08I64x\n"), m_dw64PreferredLoadAddress);
		}
	}

	//
	// Second, if we were to collect symbol info, but NOT verify... dump out what we
	// discovered about the symbol info...
	//
	if (g_lpProgramOptions->GetMode(CProgramOptions::OutputSymbolInformationMode) &&
	   !g_lpProgramOptions->GetMode(CProgramOptions::VerifySymbolsMode))
	{
		fPrintCarriageReturn = true;

		switch (m_enumPEImageSymbolStatus)
		{
			case SYMBOL_INFORMATION_UNKNOWN:
				_tprintf(TEXT("  Module symbol information was not collected!\n"));
				break;

			case SYMBOLS_NO:
				_tprintf(TEXT("  Module has NO symbols!\n"));
				break;

			case SYMBOLS_LOCAL:
				//
				// This module has ONLY local symbols...
				//
				_tprintf(TEXT("  Module has internal symbols only! %s\n"), SourceEnabledPEImage());
				OutputDataToStdoutInternalSymbolInfo(m_dwPEImageDebugDirectoryCoffSize, m_dwPEImageDebugDirectoryFPOSize, m_dwPEImageDebugDirectoryCVSize, m_dwPEImageDebugDirectoryOMAPtoSRCSize, m_dwPEImageDebugDirectoryOMAPfromSRCSize);
				break;

			case SYMBOLS_DBG:

				//
				// This module may have Internal Symbols but has a DBG file...
				//
				OutputDataToStdoutInternalSymbolInfo(m_dwPEImageDebugDirectoryCoffSize, m_dwPEImageDebugDirectoryFPOSize, m_dwPEImageDebugDirectoryCVSize, m_dwPEImageDebugDirectoryOMAPtoSRCSize, m_dwPEImageDebugDirectoryOMAPfromSRCSize);

				//
				// Output the DBG Symbol Information
				//
				OutputDataToStdoutDbgSymbolInfo(m_tszPEImageDebugDirectoryDBGPath, m_dwPEImageTimeDateStamp, m_dwPEImageCheckSum, m_dwPEImageSizeOfImage);

				//
				// Output the DBG Internal Symbol Information
				//
				OutputDataToStdoutInternalSymbolInfo(m_dwDBGImageDebugDirectoryCoffSize, m_dwDBGImageDebugDirectoryFPOSize, m_dwDBGImageDebugDirectoryCVSize, m_dwDBGImageDebugDirectoryOMAPtoSRCSize, m_dwDBGImageDebugDirectoryOMAPfromSRCSize);
				break;

			case SYMBOLS_PDB:
				//
				// Output any internal symbols (that should be "splitsym'ed out")
				//
				OutputDataToStdoutInternalSymbolInfo(m_dwPEImageDebugDirectoryCoffSize, m_dwPEImageDebugDirectoryFPOSize, m_dwPEImageDebugDirectoryCVSize, m_dwPEImageDebugDirectoryOMAPtoSRCSize, m_dwPEImageDebugDirectoryOMAPfromSRCSize);

				//
				// In this case, we have a PE Image with a PDB file...
				//
				OutputDataToStdoutPdbSymbolInfo(m_dwPEImageDebugDirectoryPDBFormatSpecifier, m_tszPEImageDebugDirectoryPDBPath, m_dwPEImageDebugDirectoryPDBSignature, &m_guidPEImageDebugDirectoryPDBGuid, m_dwPEImageDebugDirectoryPDBAge);
				break;
		}
	}

	//
	// Third, if we were to verify symbol info, display the results...
	//
	if (g_lpProgramOptions->GetMode(CProgramOptions::VerifySymbolsMode))
	{
		fPrintCarriageReturn = true;

		switch (m_enumPEImageSymbolStatus)
		{
			case SYMBOL_INFORMATION_UNKNOWN:
				_tprintf(TEXT("  Module symbol information was not collected!\n"));
				break;

			case SYMBOLS_NO:
				_tprintf(TEXT("  Module has NO symbols\n"));
				break;

			case SYMBOLS_LOCAL:
				_tprintf(TEXT("  Module has internal symbols only! %s\n"), SourceEnabledPEImage());
				OutputDataToStdoutInternalSymbolInfo(m_dwPEImageDebugDirectoryCoffSize, m_dwPEImageDebugDirectoryFPOSize, m_dwPEImageDebugDirectoryCVSize, m_dwPEImageDebugDirectoryOMAPtoSRCSize, m_dwPEImageDebugDirectoryOMAPfromSRCSize);
				break;

			case SYMBOLS_DBG:
			case SYMBOLS_DBG_AND_PDB:
				switch (m_enumDBGModuleStatus)
				{
					case SYMBOL_MATCH:
						
						// Did they want the debug/symbol info for the PE image itself?
						if(g_lpProgramOptions->GetMode(CProgramOptions::OutputSymbolInformationMode))
						{
							OutputDataToStdoutInternalSymbolInfo(m_dwPEImageDebugDirectoryCoffSize, m_dwPEImageDebugDirectoryFPOSize, m_dwPEImageDebugDirectoryCVSize, m_dwPEImageDebugDirectoryOMAPtoSRCSize, m_dwPEImageDebugDirectoryOMAPfromSRCSize);
						}

						if(m_tszDBGModuleFileSystemPath )
						{
							_tprintf(TEXT("  DBG File = %s [VERIFIED] %s\n"), m_tszDBGModuleFileSystemPath, SourceEnabledDBGImage());
						}
						
						if(g_lpProgramOptions->GetMode(CProgramOptions::OutputSymbolInformationMode))
						{
							OutputDataToStdoutInternalSymbolInfo(m_dwDBGImageDebugDirectoryCoffSize, m_dwDBGImageDebugDirectoryFPOSize, m_dwDBGImageDebugDirectoryCVSize, m_dwDBGImageDebugDirectoryOMAPtoSRCSize, m_dwDBGImageDebugDirectoryOMAPfromSRCSize);
						}
						break;

					case SYMBOL_NOT_FOUND:
						OutputDataToStdoutInternalSymbolInfo(m_dwPEImageDebugDirectoryCoffSize, m_dwPEImageDebugDirectoryFPOSize, m_dwPEImageDebugDirectoryCVSize, m_dwPEImageDebugDirectoryOMAPtoSRCSize, m_dwPEImageDebugDirectoryOMAPfromSRCSize);
						OutputDataToStdoutDbgSymbolInfo(m_tszPEImageDebugDirectoryDBGPath, m_dwPEImageTimeDateStamp, m_dwPEImageCheckSum, m_dwPEImageSizeOfImage);
						_tprintf(TEXT("  DBG File NOT FOUND!\n"));
						break; // If we didn't find the DBG file... we don't bother with the PDB...

					case SYMBOL_POSSIBLE_MISMATCH:
						OutputDataToStdoutInternalSymbolInfo(m_dwPEImageDebugDirectoryCoffSize, m_dwPEImageDebugDirectoryFPOSize, m_dwPEImageDebugDirectoryCVSize, m_dwPEImageDebugDirectoryOMAPtoSRCSize, m_dwPEImageDebugDirectoryOMAPfromSRCSize);

						OutputDataToStdoutDbgSymbolInfo(m_tszPEImageDebugDirectoryDBGPath, m_dwPEImageTimeDateStamp, m_dwPEImageCheckSum, m_dwPEImageSizeOfImage);
						OutputDataToStdoutDbgSymbolInfo(m_tszDBGModuleFileSystemPath, m_dwDBGTimeDateStamp, m_dwDBGCheckSum, m_dwDBGSizeOfImage, TEXT("DISCREPANCY"), m_dwPEImageTimeDateStamp, m_dwPEImageCheckSum, m_dwPEImageSizeOfImage);
						OutputDataToStdoutInternalSymbolInfo(m_dwDBGImageDebugDirectoryCoffSize, m_dwDBGImageDebugDirectoryFPOSize, m_dwDBGImageDebugDirectoryCVSize, m_dwDBGImageDebugDirectoryOMAPtoSRCSize, m_dwDBGImageDebugDirectoryOMAPfromSRCSize);
						break;
				};

				//
				// Intentional fall through to SYMBOLS_PDB (we might have one)
				//

			case SYMBOLS_PDB:

				// These two cases should have a PDB file... if we can find it...
				//
				if ( (m_enumPEImageSymbolStatus == SYMBOLS_DBG_AND_PDB) ||
					 (m_enumPEImageSymbolStatus == SYMBOLS_PDB) )
				{
					//
					// If we have a DebugDirectoryPDBPath... then display the goods...
					//
					if (GetDebugDirectoryPDBPath())
					{
						switch(m_enumPDBModuleStatus)
						{
							case SYMBOL_NOT_FOUND:
								OutputDataToStdoutInternalSymbolInfo(m_dwPEImageDebugDirectoryCoffSize, m_dwPEImageDebugDirectoryFPOSize, m_dwPEImageDebugDirectoryCVSize, m_dwPEImageDebugDirectoryOMAPtoSRCSize, m_dwPEImageDebugDirectoryOMAPfromSRCSize);

								// Output PDB info as appropriate
								if (m_enumPEImageSymbolStatus == SYMBOLS_PDB)
								{
									OutputDataToStdoutPdbSymbolInfo(m_dwPEImageDebugDirectoryPDBFormatSpecifier, m_tszPEImageDebugDirectoryPDBPath, m_dwPEImageDebugDirectoryPDBSignature, &m_guidPEImageDebugDirectoryPDBGuid, m_dwPEImageDebugDirectoryPDBAge);
								}
								else
								{
									OutputDataToStdoutPdbSymbolInfo(m_dwDBGDebugDirectoryPDBFormatSpecifier, m_tszDBGDebugDirectoryPDBPath, m_dwDBGDebugDirectoryPDBSignature, &m_guidDBGDebugDirectoryPDBGuid, m_dwDBGDebugDirectoryPDBAge);
								}

								_tprintf(TEXT("  NO PDB FILE FOUND!!\n"));
								break;

							case SYMBOL_MATCH:
								// Did they want the debug/symbol info for the PE image itself?
								if(m_tszPDBModuleFileSystemPath )
									_tprintf(TEXT("  PDB File = %s [VERIFIED] %s\n"), m_tszPDBModuleFileSystemPath, SourceEnabledPDB());

								// NEWCODE: Testing...
								if (g_lpProgramOptions->GetMode(CProgramOptions::OutputSymbolInformationMode))
								{
									if (m_dwPDBTotalSourceFiles)
									{
										// DIA Symbol information first...
										_tprintf(TEXT("    Module PDB Source Files = 0x%x\n"), m_dwPDBTotalSourceFiles);
									} else
									{
										// MSDBI Symbol information second...
										if (m_dwPDBTotalBytesOfLineInformation)
											_tprintf(TEXT("    Module PDB Bytes of Lines     = 0x%x\n"), m_dwPDBTotalBytesOfLineInformation);

										if (m_dwPDBTotalBytesOfSymbolInformation)
											_tprintf(TEXT("    Module PDB Bytes of Symbols   = 0x%x\n"), m_dwPDBTotalBytesOfSymbolInformation);

										if (m_dwPDBTotalSymbolTypesRange)
											_tprintf(TEXT("    Module PDB Symbol Types Range = 0x%x\n"), m_dwPDBTotalSymbolTypesRange);
									}
								
								}
									
								break;

							case SYMBOL_POSSIBLE_MISMATCH:
								if(m_tszPDBModuleFileSystemPath )
								{
									// Output PDB info as appropriate
									if (m_enumPEImageSymbolStatus == SYMBOLS_PDB)
									{
										OutputDataToStdoutPdbSymbolInfo(m_dwPEImageDebugDirectoryPDBFormatSpecifier, m_tszPEImageDebugDirectoryPDBPath, m_dwPEImageDebugDirectoryPDBSignature, &m_guidPEImageDebugDirectoryPDBGuid, m_dwPEImageDebugDirectoryPDBAge);
									}
									else
									{
										OutputDataToStdoutPdbSymbolInfo(m_dwDBGDebugDirectoryPDBFormatSpecifier, m_tszDBGDebugDirectoryPDBPath, m_dwDBGDebugDirectoryPDBSignature, &m_guidDBGDebugDirectoryPDBGuid, m_dwDBGDebugDirectoryPDBAge);
									}

									//
									// Output the PDB data itself...
									//
									OutputDataToStdoutPdbSymbolInfo(m_dwPDBFormatSpecifier, m_tszPDBModuleFileSystemPath, m_dwPDBSignature, &m_guidPDBGuid, m_dwPDBAge, TEXT("DISCREPANCY"));
								}
								break;

							case SYMBOL_INVALID_FORMAT:
								if(m_tszPDBModuleFileSystemPath )
								{
									// Output PDB info as appropriate
									if (m_enumPEImageSymbolStatus == SYMBOLS_PDB)
									{
										OutputDataToStdoutPdbSymbolInfo(m_dwPEImageDebugDirectoryPDBFormatSpecifier, m_tszPEImageDebugDirectoryPDBPath, m_dwPEImageDebugDirectoryPDBSignature, &m_guidPEImageDebugDirectoryPDBGuid, m_dwPEImageDebugDirectoryPDBAge);
									}
									else
									{
										OutputDataToStdoutPdbSymbolInfo(m_dwDBGDebugDirectoryPDBFormatSpecifier, m_tszDBGDebugDirectoryPDBPath, m_dwDBGDebugDirectoryPDBSignature, &m_guidDBGDebugDirectoryPDBGuid, m_dwDBGDebugDirectoryPDBAge);
									}
								
									_tprintf(TEXT("  PDB File = %s [INVALID_FORMAT]\n"), m_tszPDBModuleFileSystemPath );
								}
								break;

							case SYMBOL_NO_HELPER_DLL:
								if(m_tszPDBModuleFileSystemPath )
								{
									// Output PDB info as appropriate
									if (m_enumPEImageSymbolStatus == SYMBOLS_PDB)
									{
										OutputDataToStdoutPdbSymbolInfo(m_dwPEImageDebugDirectoryPDBFormatSpecifier, m_tszPEImageDebugDirectoryPDBPath, m_dwPEImageDebugDirectoryPDBSignature, &m_guidPEImageDebugDirectoryPDBGuid, m_dwPEImageDebugDirectoryPDBAge);
									}
									else
									{
										OutputDataToStdoutPdbSymbolInfo(m_dwDBGDebugDirectoryPDBFormatSpecifier, m_tszDBGDebugDirectoryPDBPath, m_dwDBGDebugDirectoryPDBSignature, &m_guidDBGDebugDirectoryPDBGuid, m_dwDBGDebugDirectoryPDBAge);
									}
								
									_tprintf(TEXT("  PDB File = %s [Unable to Validate]\n"), m_tszPDBModuleFileSystemPath );
								}
								break;
						}
					} else
					{
						OutputDataToStdoutInternalSymbolInfo(m_dwPEImageDebugDirectoryCoffSize, m_dwPEImageDebugDirectoryFPOSize, m_dwPEImageDebugDirectoryCVSize, m_dwPEImageDebugDirectoryOMAPtoSRCSize, m_dwPEImageDebugDirectoryOMAPfromSRCSize);
						OutputDataToStdoutDbgSymbolInfo(m_tszPEImageDebugDirectoryDBGPath, m_dwPEImageTimeDateStamp, m_dwPEImageCheckSum, m_dwPEImageSizeOfImage);
						_tprintf(TEXT("  Module has PDB File\n"));
						_tprintf(TEXT("  Module Pointer to PDB = [UNKNOWN] (Could not find in PE Image)\n"));
					};
				};
			}
	}

	// Should we tack an extra carriage-return?
	if ( fPrintCarriageReturn )
		_tprintf(TEXT("\n"));

	return true;
}

LPTSTR CModuleInfo::SymbolModuleStatusString(enum SymbolModuleStatus enumModuleStatus)
{
	LPTSTR tszStringPointer = NULL;

	// Output the Symbol Information for the PE module
	switch (enumModuleStatus)
	{
		case SYMBOL_NOT_FOUND:
			tszStringPointer = TEXT("SYMBOL_NOT_FOUND");
			break;

		case SYMBOL_MATCH:
			tszStringPointer = TEXT("SYMBOL_MATCH");
			break;

		case SYMBOL_POSSIBLE_MISMATCH:
			tszStringPointer = TEXT("SYMBOL_POSSIBLE_MISMATCH");
			break;

		case SYMBOL_INVALID_FORMAT:
			tszStringPointer = TEXT("SYMBOL_INVALID_FORMAT");
			break;

		case SYMBOL_NO_HELPER_DLL:
			tszStringPointer = TEXT("SYMBOL_NO_HELPER_DLL");
			break;

		default:
			tszStringPointer = NULL;
	}

	return tszStringPointer;
}

LPTSTR CModuleInfo::SymbolInformationString(enum SymbolInformationForPEImage enumSymbolInformationForPEImage)
{
	LPTSTR tszStringPointer = NULL;

	// Ouput the Symbol Information for the PE module
	switch (enumSymbolInformationForPEImage)
	{
		case SYMBOL_INFORMATION_UNKNOWN:
			tszStringPointer = TEXT("SYMBOL_INFORMATION_UNKNOWN");
			break;

		case SYMBOLS_NO:
			tszStringPointer = TEXT("SYMBOLS_NO");
			break;

		case SYMBOLS_LOCAL:
			tszStringPointer = TEXT("SYMBOLS_LOCAL");
			break;
		
		case SYMBOLS_DBG:
			tszStringPointer = TEXT("SYMBOLS_DBG");
			break;
		
		case SYMBOLS_DBG_AND_PDB:
			tszStringPointer = TEXT("SYMBOLS_DBG_AND_PDB");
			break;

		case SYMBOLS_PDB:
			tszStringPointer = TEXT("SYMBOLS_PDB");
			break;

		default:
			tszStringPointer = NULL;
	}

	return tszStringPointer;
}

bool CModuleInfo::GetPEImageInfoFromModule(HANDLE hModuleHandle, const bool fDmpFile)
{
	bool fReturn = false;

	// PE Image Locals
	IMAGE_DOS_HEADER    		ImageDosHeader;
	DWORD 				  		dwMagic;
	IMAGE_FILE_HEADER    		ImageFileHeader;
	IMAGE_DATA_DIRECTORY 		DebugImageDataDirectory;
	IMAGE_OPTIONAL_HEADER64 	ImageOptionalHeader64;
	PIMAGE_OPTIONAL_HEADER32 	lpImageOptionalHeader32 = NULL;
	PIMAGE_SECTION_HEADER 		lpImageSectionHeader = NULL;
	ULONG				 		OffsetImageDebugDirectory;
	unsigned long				ul;
	
	// We don't know anything about symbols yet... (we may not when we exit if the user
	// didn't ask us to look...)
	m_enumPEImageSymbolStatus = SYMBOL_INFORMATION_UNKNOWN;
	
	// Start at the top of the image...
	SetReadPointer(fDmpFile, hModuleHandle, 0, FILE_BEGIN);

	// Read in a dos exe header
	if ( !DoRead(fDmpFile, hModuleHandle, &ImageDosHeader, sizeof(ImageDosHeader) ) )
		goto cleanup;
	
	if (ImageDosHeader.e_magic != IMAGE_DOS_SIGNATURE)
	{	// No sense in going further since we're expecting a PE image file...
		goto cleanup;
	}

	if (ImageDosHeader.e_lfanew == 0)
	{ // This is a DOS program... very odd...
		goto cleanup;
	}

	// Great, we have a valid DOS_SIGNATURE... now read in the NT_SIGNATURE?!
	SetReadPointer(fDmpFile, hModuleHandle, ImageDosHeader.e_lfanew, FILE_BEGIN);

	// Read in a DWORD to see if this is an image worth looking at...
	if ( !DoRead(fDmpFile, hModuleHandle, &dwMagic, sizeof(DWORD)) )
		goto cleanup;

	// Probe to see if this is a valid image... we only handle NT images (PE/PE64)
	if (dwMagic != IMAGE_NT_SIGNATURE)
		goto cleanup;

	// Now read the ImageFileHeader...
	if ( !DoRead(fDmpFile, hModuleHandle, &ImageFileHeader, sizeof(IMAGE_FILE_HEADER)) )
		goto cleanup;	

	// Okay, we have a PE Image!!!!

	// Save the Time Date Stamp
	m_dwPEImageTimeDateStamp = ImageFileHeader.TimeDateStamp;

	// Save the Machine Architecture
	m_wPEImageMachineArchitecture = ImageFileHeader.Machine;

	// Save the PE Image Characteristics
	m_wCharacteristics = ImageFileHeader.Characteristics;

	// The OptionalHeader is necessary to get the SizeOfImage and to find the DebugDirectoryInfo.
	if (ImageFileHeader.SizeOfOptionalHeader == 0)
		goto cleanup;

	// Now... the size of the Optional Header is DIFFERENT between PE32 and PE64...
	// The only items we need from the option header are:
	//
	// ULONG CheckSum
	// ULONG SizeOfImage
	// IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG]
	//
	// We'll read as a PE64 (since it's larger) and cast to PE32 if required...
	if ( !DoRead(fDmpFile, hModuleHandle, &ImageOptionalHeader64, sizeof(IMAGE_OPTIONAL_HEADER64)) )
		goto cleanup;

	switch (ImageOptionalHeader64.Magic)
	{
		case IMAGE_NT_OPTIONAL_HDR32_MAGIC:

			m_enumPEImageType = PE32;

			lpImageOptionalHeader32 = (PIMAGE_OPTIONAL_HEADER32)&ImageOptionalHeader64;

			// Save the Checksum info (though it's not very relavent to identifying symbols)
			m_dwPEImageCheckSum = lpImageOptionalHeader32->CheckSum;

			// Save the SizeOfImage info...
			m_dwPEImageSizeOfImage = lpImageOptionalHeader32->SizeOfImage;

			// Get the preferred load address
			m_dw64PreferredLoadAddress = lpImageOptionalHeader32->ImageBase;
			
			// In the absence of a an actual Load Address, use the preferred load address
			if (!m_dw64BaseAddressOfLoadedImage)
				m_dw64BaseAddressOfLoadedImage = m_dw64PreferredLoadAddress;
			
			DebugImageDataDirectory.Size = lpImageOptionalHeader32->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
			DebugImageDataDirectory.VirtualAddress = lpImageOptionalHeader32->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
			
			break;

		case IMAGE_NT_OPTIONAL_HDR64_MAGIC:

			m_enumPEImageType = PE64;

			// Save the Checksum info (though it's not very relavent to identifying symbols)
			m_dwPEImageCheckSum = ImageOptionalHeader64.CheckSum;

			// Save the SizeOfImage info...
			m_dwPEImageSizeOfImage = ImageOptionalHeader64.SizeOfImage;

			// Get the preferred load address
			m_dw64PreferredLoadAddress = ImageOptionalHeader64.ImageBase;

			// In the absence of a an actual Load Address, use the preferred load address
			if (!m_dw64BaseAddressOfLoadedImage)
				m_dw64BaseAddressOfLoadedImage = m_dw64PreferredLoadAddress;
			
			DebugImageDataDirectory.Size = ImageOptionalHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
			DebugImageDataDirectory.VirtualAddress = ImageOptionalHeader64.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
				
			break;

		default:
			goto cleanup;

	}

	// Let's quickly look to see if there is even a Debug Directory in the PE image!
	if (DebugImageDataDirectory.Size == 0)
	{
		m_enumPEImageSymbolStatus = SYMBOLS_NO;
		fReturn = true;
		goto cleanup; // No Debug Directory found...
	}

	// Now, go ahead and allocate the storage...
	lpImageSectionHeader = (PIMAGE_SECTION_HEADER) new IMAGE_SECTION_HEADER[ImageFileHeader.NumberOfSections];

	if (lpImageSectionHeader == NULL)
		goto cleanup;

 	// Set the pointer to the start of the Section Headers... (we may need to back up if we  read
	// PE64 Optional Headers and this is a PE32 image...
	if (m_enumPEImageType == PE32)
	{
		SetReadPointer(fDmpFile, hModuleHandle, (LONG)(sizeof(IMAGE_OPTIONAL_HEADER32)-sizeof(IMAGE_OPTIONAL_HEADER64)), FILE_CURRENT);
	}

	// Read in the Section Headers...
	if (!DoRead(fDmpFile, hModuleHandle, lpImageSectionHeader, (ImageFileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER))))
		goto cleanup;

	// Let's walk through these Section Headers...

	// For PE images, walk the section headers looking for the
	// one that's got the debug directory.
	for (ul=0; ul < ImageFileHeader.NumberOfSections; ul++) {

		// If the virtual address for the Debug Entry falls into this section header, then we've found it!
		if ( DebugImageDataDirectory.VirtualAddress >= lpImageSectionHeader[ul].VirtualAddress &&
			 DebugImageDataDirectory.VirtualAddress < lpImageSectionHeader[ul].VirtualAddress + lpImageSectionHeader[ul].SizeOfRawData )
		{
			break;
		}
	}

	// Assuming we haven't exhausted the list of section headers, we should have the debug directory now.
	if (ul >= ImageFileHeader.NumberOfSections)
	{
		m_enumPEImageSymbolStatus = SYMBOLS_NO;
		fReturn = true;
		goto cleanup; // No Debug Directory found...
	}

	// For a DmpFile, the address is based on the Section Header's Virtual Address, not PointerToRawData
	if (fDmpFile)
	{
		OffsetImageDebugDirectory = ((DebugImageDataDirectory.VirtualAddress - lpImageSectionHeader[ul].VirtualAddress) + lpImageSectionHeader[ul].VirtualAddress);

	} else
	{
		OffsetImageDebugDirectory = ((DebugImageDataDirectory.VirtualAddress - lpImageSectionHeader[ul].VirtualAddress) + lpImageSectionHeader[ul].PointerToRawData);
	}
	
	if (!ProcessDebugDirectory(true, fDmpFile, hModuleHandle, DebugImageDataDirectory.Size, OffsetImageDebugDirectory))
		goto cleanup;

	fReturn = true;

	/**
	**
	**	What type of symbols were found to be present...
	**
	**	NO SYMBOLS
	**	=============
	**	No Debug Directory
	**	NO Debug information stripped
	**	Symbols stripped
	**
	**	LOCAL SYMBOLS
	**	=============
	**	Debug Directory
	**	NO Debug information stripped
	**	NO Symbols stripped
	**
	**	PDB SYMBOL
	**	=============
	**	Debug Directory
	**	NO Debug information stripped
	**	Symbols stripped
	**
	**	DBG SYMBOL
	**	=============
	**	Debug Directory (assumed)
	**	BOTH - YES/NO Debug information stripped
	**	NO Symbols stripped
	**
	 **/

	if ((ImageFileHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED) == IMAGE_FILE_DEBUG_STRIPPED)
	{ // Debug Information Stripped!  (A DBG file is assumed)
		m_enumPEImageSymbolStatus = SYMBOLS_DBG;
	} else
	{ 
		// Debug Information NOT stripped! (Either a PDB exists, or symbols are local, or both...)
		if ( m_tszPEImageDebugDirectoryPDBPath || 
			(fDmpFile && (m_dwPEImageDebugDirectoryPDBFormatSpecifier == sigNOTMAPPED))
		  )
		{	// If we find PDB data, a PDB file is assumed...
			// Starting with LINK.EXE 6.2 and higher, we'll find PDB data in USER.DMP files....
			m_enumPEImageSymbolStatus = SYMBOLS_PDB;
		} else
		{ // Symbols NOT stripped (Symbols appear to be local to the PE Image)
			m_enumPEImageSymbolStatus = SYMBOLS_LOCAL;
		}
	}

cleanup:

	if (lpImageSectionHeader)
		delete [] lpImageSectionHeader;

	return fReturn;

}

bool CModuleInfo::GetVersionInfoFromModule(HANDLE hModuleHandle, const bool fDmpFile)
{
	bool fReturn = true;
	HRESULT Hr = S_OK;

	DWORD dwHandle = 0;

	struct LANGANDCODEPAGE 
	{
		WORD wLanguage;
		WORD wCodePage;
	} * pTranslate;

	// - No \VarFileInfo\Translation, assuming 00000409
	DWORD dwDefaultLanguageAndCodePage = 0x00000409;

	// Okay, we're ready to get the goods...
	VS_FIXEDFILEINFO * lpFixedFileInfo = NULL;
	VS_FIXEDFILEINFO  FixedFileInfo = {0};

	DWORD * lpdwLang = NULL;
	DWORD	dwLang = 0;
	
	TCHAR	tszSubBlock[256];
	LPTSTR	lptszBuffer = NULL;
	TCHAR	tszBuffer[_MAX_PATH];


	// Well, we can't get the filesystem information from a dmp file... no big surprise here...
	if (!fDmpFile)
	{
		// Now, get CheckSum, TimeDateStamp, and other Image properties...
		BY_HANDLE_FILE_INFORMATION lpFileInformation;

		if ( GetFileInformationByHandle(hModuleHandle, &lpFileInformation) )
		{
			// Get the file size... (hmm... we only get the low DWORD... but what EXE/DLL exceeds 2 Gig in size?)
			m_dwPEImageFileSize = lpFileInformation.nFileSizeLow;
			m_ftPEImageFileTimeDateStamp = lpFileInformation.ftLastWriteTime;
		}
	}

	// Now let's go after the version information...

	//
	// If we are calling the file version API directly, then we create a buffer that's used to return all
	// of the data...
	//
	// If we are calling the dbgeng.dll to get th file version, we provide the base address of the
	// module and a structure to return the data into (we need to allocate storage for this ourselves)
	//
	LPBYTE lpBuffer = NULL;
	DWORD dwVersionInfoSize = 0;

	if (fDmpFile)
	{
		// The DBGENG.DLL expects the buffers to be allocated already... let's hook them up...
		lpFixedFileInfo = &FixedFileInfo;
		lpdwLang = &dwLang;
		lptszBuffer = tszBuffer;

	} else
	{
		// First, is there any FileVersionInfo at all?
		dwVersionInfoSize = GetFileVersionInfoSize(m_tszPEImageModuleFileSystemPath, &dwHandle);

		if (!dwVersionInfoSize)
		{
			goto cleanup;
		}

		// Allocate a buffer to read into...
		lpBuffer = new BYTE[dwVersionInfoSize];
		
		if (!lpBuffer)
		{
			goto cleanup;
		}

		// Okay... query to get this version info...
		if (!GetFileVersionInfo(m_tszPEImageModuleFileSystemPath, dwHandle, dwVersionInfoSize, (void *)lpBuffer))
		{
			goto cleanup;
		}
	}

	//
	// Get the VS_FIXEDFILEINFO structure which carries version info...
	//
	if (SUCCEEDED(Hr = VersionQueryValue((ULONG_PTR)lpBuffer, fDmpFile, TEXT("\\"), (LPVOID *)&lpFixedFileInfo,  sizeof(FixedFileInfo))))
	{
		m_fPEImageFileVersionInfo = true;
		
		m_dwPEImageFileVersionMS = lpFixedFileInfo->dwFileVersionMS;
		m_dwPEImageFileVersionLS = lpFixedFileInfo->dwFileVersionLS;

		m_dwPEImageProductVersionMS = lpFixedFileInfo->dwProductVersionMS;
		m_dwPEImageProductVersionLS = lpFixedFileInfo->dwProductVersionLS;

		// Okay, before we go allocating a version string... let's ensure
		// we actually have a version number worth reporting...
		if ( m_dwPEImageFileVersionMS || m_dwPEImageFileVersionLS )
		{
			m_tszPEImageFileVersionString = new TCHAR[1+5+1+5+1+5+1+5+1+1]; // Format will be (#.#:#.#) where each # is a word

			if (m_tszPEImageFileVersionString) // Okay, blitz the data into place...
				_stprintf( m_tszPEImageFileVersionString, TEXT("(%d.%d:%d.%d)"), HIWORD(m_dwPEImageFileVersionMS), LOWORD(m_dwPEImageFileVersionMS), HIWORD(m_dwPEImageFileVersionLS), LOWORD(m_dwPEImageFileVersionLS) );
		}

		// Okay, before we go allocating a version string... let's ensure
		// we actually have a version number worth reporting...
		if ( m_dwPEImageProductVersionMS || m_dwPEImageProductVersionLS )
		{
			m_tszPEImageProductVersionString = new TCHAR[1+5+1+5+1+5+1+5+1+1]; // Format will be (#.#:#.#) where each # is a word

			if (m_tszPEImageProductVersionString) // Okay, blitz the data into place...
				_stprintf( m_tszPEImageProductVersionString, TEXT("(%d.%d:%d.%d)"), HIWORD(m_dwPEImageProductVersionMS), LOWORD(m_dwPEImageProductVersionMS), HIWORD(m_dwPEImageProductVersionLS), LOWORD(m_dwPEImageProductVersionLS) );
		}


	}

	// Get the language and codepage information for the CompanyName and
	// FileDescription string table resources...

	// Get the CompanyName!
	if (SUCCEEDED(Hr = VersionQueryValue((ULONG_PTR)lpBuffer, fDmpFile, TEXT("\\VarFileInfo\\Translation"), (LPVOID *)&lpdwLang,  sizeof(dwLang))))
	{
		pTranslate = (struct LANGANDCODEPAGE *)lpdwLang;
	} else
	{
		pTranslate = (struct LANGANDCODEPAGE *)&dwDefaultLanguageAndCodePage;
	}

// We may not get a value back... check for it...
//
//	if (!pTranslate) // Firewall code... just to be certain...
//	{
//		pTranslate = (struct LANGANDCODEPAGE *)&dwDefaultLanguageAndCodePage;
//	}

	_stprintf(tszSubBlock,TEXT("\\StringFileInfo\\%04x%04x\\CompanyName"), 
			pTranslate->wLanguage, 
			pTranslate->wCodePage);

	if (SUCCEEDED(Hr = VersionQueryValue((ULONG_PTR)lpBuffer, fDmpFile, tszSubBlock, (LPVOID *)&lptszBuffer ,  sizeof(tszBuffer))))
	{
		if (!fDmpFile)
		{
			// Cool, we have a Company Name...
			if (lptszBuffer && *lptszBuffer)
			{
				m_tszPEImageFileVersionCompanyName = CUtilityFunctions::CopyUnicodeStringToTSTR((LPWSTR)lptszBuffer, m_tszPEImageFileVersionCompanyName);
			}
		} else
		{
			// Cool, we have a Company Name...
			// Unfortunately, DBGENG.DLL returns all strings as ANSI
			if ((LPSTR)lptszBuffer && (char)*lptszBuffer)
			{
				m_tszPEImageFileVersionCompanyName = CUtilityFunctions::CopyAnsiStringToTSTR((LPSTR)lptszBuffer, m_tszPEImageFileVersionCompanyName);
			}
		}
	}

	_stprintf(tszSubBlock,TEXT("\\StringFileInfo\\%04x%04x\\FileDescription"),
			pTranslate->wLanguage, 
			pTranslate->wCodePage);

	if (SUCCEEDED(Hr = VersionQueryValue((ULONG_PTR)lpBuffer, fDmpFile, tszSubBlock, (LPVOID *)&lptszBuffer ,  sizeof(tszBuffer))))
	{
		if (!fDmpFile)
		{
			// Cool, we have a Description
			if (lptszBuffer && *lptszBuffer)
			{
				m_tszPEImageFileVersionDescription = CUtilityFunctions::CopyUnicodeStringToTSTR((LPWSTR)lptszBuffer, m_tszPEImageFileVersionDescription);
			}
		} else
		{
			// Cool, we have a Description
			// Unfortunately, DBGENG.DLL returns all strings as ANSI
			if ((LPSTR)lptszBuffer && (char)*lptszBuffer)
			{
				m_tszPEImageFileVersionDescription = CUtilityFunctions::CopyAnsiStringToTSTR((LPSTR)lptszBuffer, m_tszPEImageFileVersionDescription);
			}
		}
	}

	// If we still don't have a proper file version... just try
	// and grab the FileVersion string and hope it's good...
	if ( !m_dwPEImageFileVersionMS && !m_dwPEImageFileVersionLS )
	{
		_stprintf(tszSubBlock,TEXT("\\StringFileInfo\\%04x%04x\\FileVersion"),
			pTranslate->wLanguage, 
			pTranslate->wCodePage);

		if (SUCCEEDED(Hr = VersionQueryValue((ULONG_PTR)lpBuffer, fDmpFile, tszSubBlock, (LPVOID *)&lptszBuffer ,  sizeof(tszBuffer))))
		{
			if (!fDmpFile)
			{
				// Cool, we have a FileVersion String...
				if (lptszBuffer && *lptszBuffer)
				{
					m_tszPEImageFileVersionString = CUtilityFunctions::CopyUnicodeStringToTSTR((LPWSTR)lptszBuffer, m_tszPEImageFileVersionString);
				}
			} else
			{
				// Cool, we have a FileVersion String...
				// Unfortunately, DBGENG.DLL returns all strings as ANSI
				if ((LPSTR)lptszBuffer && (char)*lptszBuffer)
				{
					m_tszPEImageFileVersionString = CUtilityFunctions::CopyAnsiStringToTSTR((LPSTR)lptszBuffer, m_tszPEImageFileVersionString);
				}
			}
		}
	}
	
	// If we still don't have a proper file version... just try
	// and grab the ProductVersion string and hope it's good...
	if ( !m_dwPEImageProductVersionMS && !m_dwPEImageProductVersionLS )
	{
		_stprintf(tszSubBlock,TEXT("\\StringFileInfo\\%04x%04x\\ProductVersion"),
			pTranslate->wLanguage, 
			pTranslate->wCodePage);

		if (SUCCEEDED(Hr = VersionQueryValue((ULONG_PTR)lpBuffer, fDmpFile, tszSubBlock, (LPVOID *)&lptszBuffer ,  sizeof(tszBuffer))))
		{
			if (!fDmpFile)
			{
				// Cool, we have a ProductVersion String...
				if (lptszBuffer && *lptszBuffer)
				{
					m_tszPEImageProductVersionString = CUtilityFunctions::CopyUnicodeStringToTSTR((LPWSTR)lptszBuffer, m_tszPEImageProductVersionString);
				}
			} else
			{
				// Cool, we have a ProductVersion  String...
				// Unfortunately, DBGENG.DLL returns all strings as ANSI
				if ((LPSTR)lptszBuffer && (char)*lptszBuffer)
				{
					m_tszPEImageProductVersionString = CUtilityFunctions::CopyAnsiStringToTSTR((LPSTR)lptszBuffer, m_tszPEImageProductVersionString);
				}
			}
		}
	}

	fReturn = true;

cleanup:

	if (lpBuffer) 
	{
		delete [] lpBuffer;
		lpBuffer = NULL;
	};
	
	return fReturn;
}

HRESULT CModuleInfo::VersionQueryValue(ULONG_PTR lpAddress, bool fDmpFile, LPTSTR tszSubBlock, LPVOID * lplpBuffer, unsigned int cbSizeOfBuffer)
{
	HRESULT Hr = E_FAIL;
	unsigned int SizeRead;
	
	if (fDmpFile)
	{
		char	szSubBlock[256];

		CUtilityFunctions::CopyTSTRStringToAnsi(tszSubBlock, szSubBlock, 256);

		Hr = m_lpDmpFile->m_pIDebugSymbols2->GetModuleVersionInformation(DEBUG_ANY_ID,
				  m_dw64BaseAddressOfLoadedImage, 
				  szSubBlock,
				  (PVOID)(* lplpBuffer),
				  cbSizeOfBuffer,
				  (ULONG *)&SizeRead) ;
		
	
	} else
	{
		if (VerQueryValue((LPVOID)lpAddress, tszSubBlock, lplpBuffer, &SizeRead))
		{
			Hr = S_OK;
		}
	}


	return Hr;
}

bool CModuleInfo::GetModuleInfoFromCSVFile(LPTSTR tszModulePath)
{
	TCHAR tszFileName[_MAX_FNAME];
	TCHAR tszFileExtension[_MAX_EXT];
	char * lpstrOpenParenthesis = NULL;

	// Copy the Module Name to the ModuleInfo Object...
	_tsplitpath(tszModulePath, NULL, NULL, tszFileName, tszFileExtension);

	if (tszFileName && tszFileExtension)
	{
		// Compose the module name...
		m_tszPEImageModuleName = new TCHAR[_tcsclen(tszFileName)+_tcsclen(tszFileExtension)+1];
		
		if (!m_tszPEImageModuleName)
			return false;

		_tcscpy(m_tszPEImageModuleName, tszFileName);
		_tcscat(m_tszPEImageModuleName, tszFileExtension);
	}

	// Get the symbol status
	enum {BUFFER_SIZE = 32};
	char szSymbolStatus[BUFFER_SIZE];

	m_lpInputFile->ReadString(szSymbolStatus, BUFFER_SIZE);

	// Get the enum value for this string...
	m_enumPEImageSymbolStatus = SymbolInformation(szSymbolStatus);
	
	// Reset the symbol status if it is DBG/PDB (that may have
	// applied on the other machine where the data was captured,
	// but on this machine we'll have to find the DBG file
	// first, then see if a PDB file exists...
	if (m_enumPEImageSymbolStatus == SYMBOLS_DBG_AND_PDB)
		m_enumPEImageSymbolStatus = SYMBOLS_DBG;

	m_lpInputFile->ReadDWORD(&m_dwPEImageCheckSum);

	m_lpInputFile->ReadDWORD((LPDWORD)&m_dwPEImageTimeDateStamp);

	// Skip the time/date string...
	m_lpInputFile->ReadString();

	m_lpInputFile->ReadDWORD(&m_dwPEImageSizeOfImage);

	char szBuffer[_MAX_PATH+1];

	DWORD dwStringLength;

	// Read in the DBG Module Path
	dwStringLength = m_lpInputFile->ReadString(szBuffer, _MAX_PATH+1);

	if (dwStringLength)
	{
		// Okay, if we found a good path... allocate space for it...

		m_tszPEImageDebugDirectoryDBGPath = CUtilityFunctions::CopyAnsiStringToTSTR(szBuffer);

		if (!m_tszPEImageDebugDirectoryDBGPath)
			return false;
	}

	// Read in the PDB Module Path
	dwStringLength = m_lpInputFile->ReadString(szBuffer, _MAX_PATH+1);

	if (dwStringLength)
	{
		// Okay, if we found a good path... allocate space for it...
		m_tszPEImageDebugDirectoryPDBPath = CUtilityFunctions::CopyAnsiStringToTSTR(szBuffer);

		if (!m_tszPEImageDebugDirectoryPDBPath)
			return false; // Failure allocating...
	}

	// We need to probe the PDB Signature to see if it is a GUID or not...
	m_lpInputFile->ReadString(szBuffer, _MAX_PATH+1);

	// Look for open parenthesis...
	lpstrOpenParenthesis = strstr(szBuffer, "{");

	if (lpstrOpenParenthesis)
	{
		// It's a GUID!
#ifdef _UNICODE

		WCHAR tszBuffer[_MAX_PATH+1];
#else
		unsigned char tszBuffer[_MAX_PATH+1];
#endif

		m_dwPEImageDebugDirectoryPDBFormatSpecifier = sigRSDS;
		
		// Advance beyond the "{" brace...
		lpstrOpenParenthesis = CharNextA(lpstrOpenParenthesis);

		int len = strlen(lpstrOpenParenthesis);

		if (len)
		{
			lpstrOpenParenthesis[len-1] = '\0';

			CUtilityFunctions::CopyAnsiStringToTSTR(lpstrOpenParenthesis, (LPTSTR)tszBuffer, _MAX_PATH+1);
	
			if(RPC_S_OK != ::UuidFromString(tszBuffer, &m_guidPEImageDebugDirectoryPDBGuid))
			{
				// Couldn't convert, ensure the guid is Zero then...
				::ZeroMemory(&m_guidPEImageDebugDirectoryPDBGuid, sizeof(m_guidPEImageDebugDirectoryPDBGuid));
			}
		}
	} else
	{
		// It's not a GUID, it's a DWORD
		m_dwPEImageDebugDirectoryPDBSignature = atoi(szBuffer);
		m_dwPEImageDebugDirectoryPDBFormatSpecifier = sigNB10;
	}
	
	m_lpInputFile->ReadDWORD(&m_dwPEImageDebugDirectoryPDBAge);

	// Read in the Product Version String
	dwStringLength = m_lpInputFile->ReadString(szBuffer, _MAX_PATH+1);

	if (dwStringLength)
	{
		// Okay, if we found a good version... allocate space for it...
		m_tszPEImageProductVersionString = CUtilityFunctions::CopyAnsiStringToTSTR(szBuffer);

		if (!m_tszPEImageProductVersionString )
			return false; // Failure allocating...
	}

	// Read in the File Version String
	dwStringLength = m_lpInputFile->ReadString(szBuffer, _MAX_PATH+1);

	if (dwStringLength)
	{
		// Okay, if we found a good version... allocate space for it...
		m_tszPEImageFileVersionString = CUtilityFunctions::CopyAnsiStringToTSTR(szBuffer);

		if (!m_tszPEImageFileVersionString )
			return false; // Failure allocating...
	}
	
	// Read in the File Version Company String
	dwStringLength = m_lpInputFile->ReadString(szBuffer, _MAX_PATH+1);

	if (dwStringLength)
	{
		// Okay, if we found a good version... allocate space for it...
		m_tszPEImageFileVersionCompanyName = CUtilityFunctions::CopyAnsiStringToTSTR(szBuffer);

		if ( !m_tszPEImageFileVersionCompanyName )
			return false; // Failure allocating...
	}
	
	// Read in the File Version Description String
	dwStringLength = m_lpInputFile->ReadString(szBuffer, _MAX_PATH+1);

	if (dwStringLength)
	{
		// Okay, if we found a good version... allocate space for it...
		m_tszPEImageFileVersionDescription = CUtilityFunctions::CopyAnsiStringToTSTR(szBuffer);

		if ( !m_tszPEImageFileVersionDescription )
			return false; // Failure allocating...
	}
	
	m_lpInputFile->ReadDWORD(&m_dwPEImageFileSize);
	
	m_lpInputFile->ReadDWORD(&m_ftPEImageFileTimeDateStamp.dwHighDateTime);

	m_lpInputFile->ReadDWORD(&m_ftPEImageFileTimeDateStamp.dwLowDateTime);

	// Okay... read to the start of the next line...
	m_lpInputFile->ReadFileLine();

	return true;
}

// This function is for ANSI strings explicitly because we only need to map these from
// ANSI strings read from a file, to an enum...
CModuleInfo::SymbolInformationForPEImage CModuleInfo::SymbolInformation(LPSTR szSymbolInformationString)
{
	if (0 == _stricmp(szSymbolInformationString, "SYMBOLS_DBG"))
		return SYMBOLS_DBG;

	if (0 == _stricmp(szSymbolInformationString, "SYMBOLS_PDB"))
		return SYMBOLS_PDB;

	if (0 == _stricmp(szSymbolInformationString, "SYMBOLS_DBG_AND_PDB"))
		return SYMBOLS_DBG_AND_PDB;

	if (0 == _stricmp(szSymbolInformationString, "SYMBOLS_NO"))
		return SYMBOLS_NO;

	if (0 == _stricmp(szSymbolInformationString, "SYMBOLS_LOCAL"))
		return SYMBOLS_LOCAL;

	if (0 == _stricmp(szSymbolInformationString, "SYMBOL_INFORMATION_UNKNOWN"))
		return SYMBOL_INFORMATION_UNKNOWN;

	return SYMBOL_INFORMATION_UNKNOWN;
}

bool CModuleInfo::OutputFileTime(FILETIME ftFileTime, LPTSTR tszFileTime, int iFileTimeBufferSize)
{

	// Thu Oct 08 15:37:22 1998

	FILETIME ftLocalFileTime;
	SYSTEMTIME lpSystemTime;
	int cch = 0, cch2 = 0;

	// Let's convert this to a local file time first...
	if (!FileTimeToLocalFileTime(&ftFileTime, &ftLocalFileTime))
		return false;

	FileTimeToSystemTime( &ftLocalFileTime, &lpSystemTime );
	
	cch = GetDateFormat( LOCALE_USER_DEFAULT,
						 0,
						 &lpSystemTime,
						 TEXT("ddd MMM dd"),
						 tszFileTime,
						 iFileTimeBufferSize );

	if (!cch)
		return false;

	// Let's keep going...
	tszFileTime[cch-1] = TEXT(' ');

	//
    // Get time and format to characters
    //
     cch2 = GetTimeFormat( LOCALE_USER_DEFAULT,
						   NULL,
						   &lpSystemTime,
						   TEXT("HH':'mm':'ss"),
						   tszFileTime + cch,
						   iFileTimeBufferSize - cch );

	// Let's keep going... we have to tack on the year...
	tszFileTime[cch + cch2 - 1] = TEXT(' ');

	GetDateFormat( LOCALE_USER_DEFAULT,
					 0,
					 &lpSystemTime,
					 TEXT("yyyy"),
					 tszFileTime + cch + cch2,
					 iFileTimeBufferSize - cch - cch2);
	return true;
}

bool CModuleInfo::SetModulePath(LPTSTR tszModulePath)
{
	// Copy the Module Path to the ModuleInfo Object...
	if (!tszModulePath) {
		return false;
	}

	if (m_tszPEImageModuleFileSystemPath)
		delete [] m_tszPEImageModuleFileSystemPath;

	m_tszPEImageModuleFileSystemPath = new TCHAR[(_tcsclen(tszModulePath)+1)];

	if (!m_tszPEImageModuleFileSystemPath)
		return false;

	_tcscpy(m_tszPEImageModuleFileSystemPath, tszModulePath);
	return true;
}

ULONG CModuleInfo::SetReadPointer(bool fDmpFile, HANDLE hModuleHandle, LONG cbOffset, int iFrom)
{
    if (fDmpFile)
	{
        switch( iFrom )
		{
			case FILE_BEGIN:
				m_dwCurrentReadPosition = cbOffset;
				break;

			case FILE_CURRENT:
				m_dwCurrentReadPosition += cbOffset;
				break;

			default:
				break;
        }
	} else
	{
        m_dwCurrentReadPosition = SetFilePointer(hModuleHandle, cbOffset, NULL, iFrom);
    }

    return m_dwCurrentReadPosition;
}

bool CModuleInfo::DoRead(bool fDmpFile, HANDLE hModuleHandle, LPVOID lpBuffer, DWORD cbNumberOfBytesToRead)
{
    DWORD       cbActuallyRead;
	bool fReturn = false;

    if (fDmpFile)
	{
		if (m_lpDmpFile)
		{
			HRESULT Hr;

			if (FAILED(Hr = m_lpDmpFile->m_pIDebugDataSpaces->ReadVirtual(m_dw64BaseAddressOfLoadedImage+(DWORD64)m_dwCurrentReadPosition,
				lpBuffer,
				cbNumberOfBytesToRead,
				&cbActuallyRead)))
			{
				goto exit;
			}

			if (cbActuallyRead != cbNumberOfBytesToRead)
			{
				goto exit;
			}

		} else
		{
			goto exit;
		}

		m_dwCurrentReadPosition += cbActuallyRead;

    } else if ( (ReadFile(hModuleHandle, lpBuffer, cbNumberOfBytesToRead, &cbActuallyRead, NULL) == 0) ||
                (cbNumberOfBytesToRead != cbActuallyRead) )
	{
        goto exit;
    }

	fReturn = true;

exit:
    return fReturn;
}

bool CModuleInfo::SetDebugDirectoryDBGPath(LPTSTR tszNewDebugDirectoryDBGPath)
{
	m_tszPEImageDebugDirectoryDBGPath = CUtilityFunctions::CopyString(tszNewDebugDirectoryDBGPath, m_tszPEImageDebugDirectoryDBGPath);

	return true;
}

bool CModuleInfo::SetPEDebugDirectoryPDBPath(LPTSTR tszNewDebugDirectoryPDBPath)
{
	m_tszPEImageDebugDirectoryPDBPath = CUtilityFunctions::CopyString(tszNewDebugDirectoryPDBPath, m_tszPEImageDebugDirectoryPDBPath );

	return true;
}

bool CModuleInfo::SetPEImageModulePath(LPTSTR tszNewPEImageModulePath)
{
	m_tszPEImageModuleFileSystemPath = CUtilityFunctions::CopyString(tszNewPEImageModulePath, m_tszPEImageModuleFileSystemPath);

	_tcsupr(m_tszPEImageModuleFileSystemPath);

	return true;
}

bool CModuleInfo::SetPEImageModuleName(LPTSTR tszNewModuleName)
{
	m_tszPEImageModuleName = CUtilityFunctions::CopyString(tszNewModuleName, m_tszPEImageModuleName);

	_tcsupr(m_tszPEImageModuleName);

	return true;
}


// Evaluate whether we've found the symbolic information for this module
// that the user is looking for.
bool CModuleInfo::GoodSymbolNotFound()
{
	bool fBadSymbol = true;

	// Well, we evaluate success based on the type of symbol we're looking for 
	// and whether it was successfully found.
	switch (GetPESymbolInformation())
	{
		// This is bad... consider this fatal...
		case SYMBOL_INFORMATION_UNKNOWN:
			break;

		// Is this bad?  I think so... but if you inherit a module as an import should you
		// be punished for the ills of another?  Hmm....  For now we'll say it's okay...
		case SYMBOLS_NO:
			fBadSymbol = false;
			break;

		// While this is wasteful, we have symbolic info... so that's cool
		case SYMBOLS_LOCAL:
			fBadSymbol = false;
			break;

		case SYMBOLS_DBG:
			fBadSymbol = SYMBOL_MATCH == GetDBGSymbolModuleStatus();
			break;

		case SYMBOLS_DBG_AND_PDB:
			fBadSymbol = (SYMBOL_MATCH == GetDBGSymbolModuleStatus()) &&
						 (SYMBOL_MATCH == GetPDBSymbolModuleStatus());
			break;
		
		case SYMBOLS_PDB:
			fBadSymbol = SYMBOL_MATCH == GetPDBSymbolModuleStatus();
			break;

		default:
			break;
	}
	return fBadSymbol;
}

//
// Process the DebugDirectory data for a PE image (or a DBG file)
//
bool CModuleInfo::ProcessDebugDirectory(const bool fPEImage, const bool fDmpFile, const HANDLE hModuleHandle, unsigned int iDebugDirectorySize, ULONG OffsetImageDebugDirectory)
{
	unsigned int iNumberOfDebugDirectoryEntries = iDebugDirectorySize / sizeof(IMAGE_DEBUG_DIRECTORY);
	
	//
	// Let's loop through the debug directories and collect the relavent info...
	//
    while (iNumberOfDebugDirectoryEntries--) 
    {
        IMAGE_DEBUG_DIRECTORY ImageDebugDirectory;

 		// Set the pointer to the DebugDirectories entry
		SetReadPointer(fDmpFile, hModuleHandle, OffsetImageDebugDirectory, FILE_BEGIN);

		// Read the DebugDirectoryImage
		if (!DoRead(fDmpFile, hModuleHandle, &ImageDebugDirectory, sizeof(IMAGE_DEBUG_DIRECTORY)))
			goto cleanup;

		//
		// Processing of the Debug Directory is dependent on the type
		//
		switch (ImageDebugDirectory.Type)
		{
			//
			// This is our preferred debug format as it offers full source level debugging (typically)
			//
			case IMAGE_DEBUG_TYPE_CODEVIEW:
				ProcessDebugTypeCVDirectoryEntry(fPEImage, fDmpFile, hModuleHandle, &ImageDebugDirectory);
				break;

			//
			// COFF symbols are okay... CV is better :)
			//
			case IMAGE_DEBUG_TYPE_COFF:
				ProcessDebugTypeCoffDirectoryEntry(fPEImage, &ImageDebugDirectory);
				break;
				
			//
			// MISC implies that a DBG file is created...
			//
			case IMAGE_DEBUG_TYPE_MISC:
				ProcessDebugTypeMiscDirectoryEntry(fPEImage, fDmpFile, hModuleHandle, &ImageDebugDirectory);
				break;
				
			//
			// FPO info is important for working with functions with FPO
			//
			case IMAGE_DEBUG_TYPE_FPO:
				ProcessDebugTypeFPODirectoryEntry(fPEImage, &ImageDebugDirectory);
				break;
				
			case IMAGE_DEBUG_TYPE_OMAP_TO_SRC:
			case IMAGE_DEBUG_TYPE_OMAP_FROM_SRC:
				ProcessDebugTypeOMAPDirectoryEntry(fPEImage, &ImageDebugDirectory);
				break;
				
			case IMAGE_DEBUG_TYPE_UNKNOWN:
			case IMAGE_DEBUG_TYPE_EXCEPTION:
			case IMAGE_DEBUG_TYPE_FIXUP:
			case IMAGE_DEBUG_TYPE_BORLAND:
			case IMAGE_DEBUG_TYPE_RESERVED10:
			case IMAGE_DEBUG_TYPE_CLSID:
				break;

			default:
				break;
		}

        OffsetImageDebugDirectory += sizeof(IMAGE_DEBUG_DIRECTORY);
    }
	
cleanup:

	return true;
}


bool CModuleInfo::ProcessDebugTypeMiscDirectoryEntry(const bool fPEImage, const bool fDmpFile, const HANDLE hModuleHandle, const PIMAGE_DEBUG_DIRECTORY lpImageDebugDirectory)
{
	bool				fReturnValue = false;
	PIMAGE_DEBUG_MISC 	lpImageDebugMisc = NULL, lpCurrentImageDebugMiscPointer = NULL;
	ULONG	 			OffsetImageDebugDirectory = NULL;
	unsigned long 		ulSizeOfMiscDirectoryEntry = lpImageDebugDirectory->SizeOfData;

	//
	// DBG files tend to store the EXE name here... not too useful for now...
	//
	if (!fPEImage)
	{
		fReturnValue = true;
		goto cleanup;
	}

	//
	// Allocate storage for the MISC data...
	//
	lpImageDebugMisc = (PIMAGE_DEBUG_MISC) new BYTE[ulSizeOfMiscDirectoryEntry];

	if (lpImageDebugMisc == NULL)
		goto cleanup;
	
	// Calculate the location/size so we can load it.
	if (fDmpFile)
	{
		OffsetImageDebugDirectory = lpImageDebugDirectory->AddressOfRawData;
	} else
	{
		OffsetImageDebugDirectory = lpImageDebugDirectory->PointerToRawData;
	}

	// Advance to the location of the Debug Info
	SetReadPointer(fDmpFile, hModuleHandle, OffsetImageDebugDirectory, FILE_BEGIN);

	// Read the data...
	if (!DoRead(fDmpFile, hModuleHandle, lpImageDebugMisc, ulSizeOfMiscDirectoryEntry))
		goto cleanup;

	// Set our pointer to the start of our data...
	lpCurrentImageDebugMiscPointer = lpImageDebugMisc;
	
	//
	// The logic of this routine will skip past bad sections of the MISC datastream...
	//
	while(ulSizeOfMiscDirectoryEntry > 0)
	{
		//
		// Hopefully we have a string here...
		//
		if (lpCurrentImageDebugMiscPointer->DataType == IMAGE_DEBUG_MISC_EXENAME)
		{
			LPSTR lpszExeName;

            lpszExeName = (LPSTR)&lpCurrentImageDebugMiscPointer->Data[ 0 ];
					
			// Save off the DBG Path...
			if (m_tszPEImageDebugDirectoryDBGPath)
				delete [] m_tszPEImageDebugDirectoryDBGPath;

			if (lpCurrentImageDebugMiscPointer->Unicode)
			{
				// Is this a Unicode string?
				m_tszPEImageDebugDirectoryDBGPath = CUtilityFunctions::CopyUnicodeStringToTSTR((LPWSTR)lpszExeName);
			} else
			{
				// Is this an ANSI string?
				m_tszPEImageDebugDirectoryDBGPath = CUtilityFunctions::CopyAnsiStringToTSTR(lpszExeName);
			}

			if (!m_tszPEImageDebugDirectoryDBGPath)
				goto cleanup;

				break;
		
	
		} else
		{
			// Beware of corrupt images
			if (lpCurrentImageDebugMiscPointer->Length == 0)
			{
				break;
			}

			// Decrement the ulSizeOfMiscDirectoryEntry by the length of this "stuff"
    		ulSizeOfMiscDirectoryEntry -= lpCurrentImageDebugMiscPointer->Length;

			// If our new value exceeds the SizeOfData we need to bail...
            if (ulSizeOfMiscDirectoryEntry > lpImageDebugDirectory->SizeOfData)
			{
				ulSizeOfMiscDirectoryEntry = 0; // Avoid AV on bad exe
            	break;
            }
            
            lpCurrentImageDebugMiscPointer = (PIMAGE_DEBUG_MISC) (lpCurrentImageDebugMiscPointer + lpCurrentImageDebugMiscPointer->Length);
			
		}
	
	}

	fReturnValue = true;

cleanup:
	
	if (lpImageDebugMisc)
	{
		delete [] lpImageDebugMisc;
		lpImageDebugMisc = NULL;
	}
	
	return fReturnValue;
}

bool CModuleInfo::ProcessDebugTypeCoffDirectoryEntry(const bool fPEImage, const PIMAGE_DEBUG_DIRECTORY lpImageDebugDirectory)
{
	//
	// The only thing we really care about is the size right now...
	//
	if (fPEImage)
	{
		m_dwPEImageDebugDirectoryCoffSize = lpImageDebugDirectory->SizeOfData;
	} else
	{
		m_dwDBGImageDebugDirectoryCoffSize = lpImageDebugDirectory->SizeOfData;
	}

	return true;
}

bool CModuleInfo::ProcessDebugTypeFPODirectoryEntry(const bool fPEImage, const PIMAGE_DEBUG_DIRECTORY lpImageDebugDirectory)
{
	//
	// The only thing we really care about is the size right now...
	//
	if (fPEImage)
	{
		m_dwPEImageDebugDirectoryFPOSize = lpImageDebugDirectory->SizeOfData;
	} else
	{
		m_dwDBGImageDebugDirectoryFPOSize = lpImageDebugDirectory->SizeOfData;
	}

	return true;

}

bool CModuleInfo::ProcessDebugTypeCVDirectoryEntry(const bool fPEImage, const bool fDmpFile, const HANDLE hModuleHandle, const PIMAGE_DEBUG_DIRECTORY lpImageDebugDirectory)
{
	bool		fReturnValue = false;
	ULONG		OffsetImageDebugDirectory;
	DWORD 		dwCVFormatSpecifier;
	char		szPdb[_MAX_PATH * 3];	// Must this be so large?
	bool 		fDebugSearchPaths = g_lpProgramOptions->fDebugSearchPaths();
	LPSTR		pszPEImageModuleName = NULL;
	LPSTR		pszSymbolPath = NULL;
	HRESULT		hr = S_OK;
	HANDLE		hProcess = ::GetCurrentProcess();
	CModuleInfo * lpModuleInfo = NULL;

	// Calculate the location/size so we can load it.
	if (fDmpFile)
	{
		OffsetImageDebugDirectory = lpImageDebugDirectory->AddressOfRawData;

		// Check the offset returned... if it is zero then it is not mapped
		// record this with our own "CVFormatSpecifier" of sigNOTMAPPED
		if (0 == OffsetImageDebugDirectory)
		{
			// Before we give up, let's search the EXEPATH to see if we can find a matching
			// binary for this PE image... and from that we can extract the data?
			if (fDebugSearchPaths)
			{
				_tprintf(TEXT("PE Image Search - SEARCH for matching PE image to extract CV Data!\n"));
			};

			pszPEImageModuleName = CUtilityFunctions::CopyTSTRStringToAnsi(m_tszPEImageModuleFileSystemPath);
			pszSymbolPath = CUtilityFunctions::CopyTSTRStringToAnsi(g_lpProgramOptions->GetExePath());

			if (!pszPEImageModuleName || !pszSymbolPath)
			{
				fReturnValue = false;
				goto cleanup;
			}

			// Use the DBGHELP APIs to find our matching PE image
			if (::SymInitialize(hProcess, pszSymbolPath, FALSE))
			{

				// Here we go!
				CHAR szImagePath[_MAX_PATH+1];
				TCHAR tszImagePath[_MAX_PATH+1];

				// Attempt the search path!!!
				if (::SymFindFileInPath(		hProcess,
											pszSymbolPath,
											pszPEImageModuleName,
											ULongToPtr(GetPEImageTimeDateStamp()), // Cast to make compiler happy
											GetPEImageSizeOfImage(),
											0,
											SSRVOPT_DWORD,	//Flags
											szImagePath,
											NULL,
											NULL))
				{
					CHAR szImageFullPath[_MAX_DRIVE+_MAX_DIR+1];
					CHAR szImageDir[_MAX_DIR+1];
					::_splitpath(szImagePath, szImageFullPath, szImageDir, NULL, NULL);
					::strcat(szImageFullPath, szImageDir);

					// On Success copy the string back...
					CUtilityFunctions::CopyAnsiStringToTSTR(szImagePath, tszImagePath, _MAX_PATH+1);

					if (fDebugSearchPaths)
					{
						_tprintf(TEXT("PE Image Search - Looking for [%s]\r\n"), tszImagePath);
					}

					// Probe for file (it's lame but we want to see if you have connectivity
					hr = CUtilityFunctions::VerifyFileExists(TEXT("PE Image Search - Failed to open [%s]!  "), tszImagePath);

					// Search only if we know we can see the file... save an expensive call
					if (hr != S_OK)
						goto NotMapped;

					// Let's allocate a new module info to query this module!
					lpModuleInfo = new CModuleInfo();

					if (NULL == lpModuleInfo)
						goto NotMapped;

					// Must initialize the module (even one this short-lived)...
					if (!lpModuleInfo->Initialize(NULL, NULL, NULL))
						goto NotMapped;

					// Set our Module Path
					if (!lpModuleInfo->SetModulePath(tszImagePath))
						goto NotMapped;

					//
					// Okay, get the module info from the file...
					//
					if (!lpModuleInfo->GetModuleInfo(tszImagePath) )
						goto NotMapped;

					// Yahoo... now, copy the data over from our new PE image...

					// PDB Path
					if (NULL != lpModuleInfo->GetDebugDirectoryPDBPath())
					{
						m_tszPEImageDebugDirectoryPDBPath = CUtilityFunctions::CopyString(lpModuleInfo->GetDebugDirectoryPDBPath());
					}

					// PDB Format Specifier
					m_dwPEImageDebugDirectoryPDBFormatSpecifier	= lpModuleInfo->GetDebugDirectoryPDBFormatSpecifier();

					// RSDS images should not require this work-around... only sigNB10 with older linkers...
					Assert(m_dwPEImageDebugDirectoryPDBFormatSpecifier == sigNB10);

					m_dwPEImageDebugDirectoryPDBAge	= lpModuleInfo->GetDebugDirectoryPDBAge();
					m_dwPEImageDebugDirectoryPDBSignature = lpModuleInfo->GetDebugDirectoryPDBSignature();
				}

				::SymCleanup(hProcess);
			}
			fReturnValue = true;
			goto cleanup;
			// Couldn't find a matching image... we have no choice but to mark it as not-mapped

NotMapped:
			m_dwPEImageDebugDirectoryPDBFormatSpecifier = sigNOTMAPPED;
			fReturnValue = true;
			goto cleanup;
		}
	} else
	{
		OffsetImageDebugDirectory = lpImageDebugDirectory->PointerToRawData;
	}

	// Advance to the location of the Debug Info
	SetReadPointer(fDmpFile, hModuleHandle, OffsetImageDebugDirectory, FILE_BEGIN);

	// Read the data...
	if (!DoRead(fDmpFile, hModuleHandle, &dwCVFormatSpecifier, sizeof(DWORD)))
		goto cleanup;

	if (fPEImage)
	{
		m_dwPEImageDebugDirectoryPDBFormatSpecifier = dwCVFormatSpecifier;
	} else
	{
		m_dwDBGDebugDirectoryPDBFormatSpecifier = dwCVFormatSpecifier;
	}

	switch (dwCVFormatSpecifier)
	{
		case sigNB09:
		case sigNB11:
			//
			// The only thing we really care about is the size right now...
			//
			m_dwPDBFormatSpecifier = dwCVFormatSpecifier;

			if (fPEImage)
			{
				m_dwPEImageDebugDirectoryCVSize = lpImageDebugDirectory->SizeOfData;
			} else
			{
				m_dwDBGImageDebugDirectoryCVSize = lpImageDebugDirectory->SizeOfData;
			}

			break;
			
		case sigNB10:

            NB10I nb10i;
			m_dwPDBFormatSpecifier = sigNB10;

			// Read the data...
			if (!DoRead(fDmpFile, hModuleHandle, &nb10i.off, sizeof(NB10I) - sizeof(DWORD)))
				goto cleanup;

			if (fPEImage)
			{
				// Save away the PDB Signature...
				m_dwPEImageDebugDirectoryPDBSignature = nb10i.sig;

				// Save away the PDB Age...
				m_dwPEImageDebugDirectoryPDBAge = nb10i.age;
			} else
			{
				// Save away the PDB Signature...
				m_dwDBGDebugDirectoryPDBSignature = nb10i.sig;

				// Save away the PDB Age...
				m_dwDBGDebugDirectoryPDBAge = nb10i.age;
			}

 			// Read the data...
			if (!DoRead(fDmpFile, hModuleHandle, szPdb, (lpImageDebugDirectory->SizeOfData) - sizeof(NB10I)))
				goto cleanup;

			if (szPdb[0] != '\0')
			{
				// Save the data (as appropriate)
				if (fPEImage)
				{
					// Copy the PDB path away...
					m_tszPEImageDebugDirectoryPDBPath = CUtilityFunctions::CopyAnsiStringToTSTR(szPdb);

					if (!m_tszPEImageDebugDirectoryPDBPath)
						goto cleanup;
				} else 
				{
					// Copy the PDB path away...
					m_tszDBGDebugDirectoryPDBPath = CUtilityFunctions::CopyAnsiStringToTSTR(szPdb);

					if (!m_tszDBGDebugDirectoryPDBPath)
						goto cleanup;

					// We now know that we have a DBG/PDB combination...
					m_enumPEImageSymbolStatus = SYMBOLS_DBG_AND_PDB;
				}
			}
			break;

		case sigRSDS:
			
            RSDSI rsdsi;
			m_dwPDBFormatSpecifier = sigRSDS;

            // Read the RSDSI structure (except for the rsds DWORD at the beginning).
			if (!DoRead(fDmpFile, hModuleHandle, &rsdsi.guidSig, sizeof(RSDSI) - sizeof(DWORD)))
				goto cleanup;

			if (fPEImage)
			{
				// Save away the PDB Age...
				m_dwPEImageDebugDirectoryPDBAge = rsdsi.age;

				// Copy the GUID....
				memcpy(&m_guidPEImageDebugDirectoryPDBGuid, &rsdsi.guidSig, sizeof(m_guidPEImageDebugDirectoryPDBGuid));
			} else
			{
				// Save away the PDB Age...
				m_dwDBGDebugDirectoryPDBAge = rsdsi.age;

				// Copy the GUID....
				memcpy(&m_guidDBGDebugDirectoryPDBGuid, &rsdsi.guidSig, sizeof(m_guidDBGDebugDirectoryPDBGuid));
			}

			// Now, read in the PDB path... apparently it's in UTF8 format...
			if (!DoRead(fDmpFile, hModuleHandle, szPdb, (lpImageDebugDirectory->SizeOfData) - sizeof(RSDSI)))
				goto cleanup;
			
			if (szPdb[0] != '\0')
			{
				// Save the data (as appropriate)
				if (fPEImage)
				{
					wchar_t wszPdb[_MAX_PATH];
					CUtilityFunctions::UTF8ToUnicode(szPdb, wszPdb, sizeof(wszPdb)/sizeof(wszPdb[0]));

					// Copy the PDB path away...
					m_tszPEImageDebugDirectoryPDBPath = CUtilityFunctions::CopyUnicodeStringToTSTR(wszPdb);
					
					if (!m_tszPEImageDebugDirectoryPDBPath)
						goto cleanup;
				} else
				{
					wchar_t wszPdb[_MAX_PATH];
					CUtilityFunctions::UTF8ToUnicode(szPdb, wszPdb, sizeof(wszPdb)/sizeof(wszPdb[0]));

					// Copy the PDB path away...
					m_tszDBGDebugDirectoryPDBPath = CUtilityFunctions::CopyUnicodeStringToTSTR(wszPdb);
					
					if (!m_tszDBGDebugDirectoryPDBPath)
						goto cleanup;
				}
			}
            break;

		// Unknown CV format...
		default:
			m_dwPDBFormatSpecifier = sigUNKNOWN;
			break;
	}

	fReturnValue = true;

cleanup:
	if (pszPEImageModuleName)
	{
		delete [] pszPEImageModuleName;
		pszPEImageModuleName  = NULL;
	}
	
	if (pszSymbolPath)
	{
		delete [] pszSymbolPath;
		pszSymbolPath = NULL;
	}

	if (NULL != lpModuleInfo)
	{
		delete lpModuleInfo;
		lpModuleInfo = NULL;
	}
	return fReturnValue;
}

bool CModuleInfo::ProcessDebugTypeOMAPDirectoryEntry(const bool fPEImage, const PIMAGE_DEBUG_DIRECTORY lpImageDebugDirectory)
{
	DWORD dwSize = lpImageDebugDirectory->SizeOfData;
	
	//
	// The only thing we really care about is the size right now...
	//
	switch (lpImageDebugDirectory->Type)
	{
		case IMAGE_DEBUG_TYPE_OMAP_TO_SRC:
			if (fPEImage)
			{
				m_dwPEImageDebugDirectoryOMAPtoSRCSize = dwSize;
			} else
			{
				m_dwDBGImageDebugDirectoryOMAPtoSRCSize = dwSize;
			}
			
			break;
			
		case IMAGE_DEBUG_TYPE_OMAP_FROM_SRC:
			if (fPEImage)
			{
				m_dwPEImageDebugDirectoryOMAPfromSRCSize = dwSize;
			} else
			{
				m_dwDBGImageDebugDirectoryOMAPfromSRCSize = dwSize;
			}
			break;
	}

	return true;
}

bool CModuleInfo::OutputDataToStdoutInternalSymbolInfo(DWORD dwCoffSize, DWORD dwFPOSize, DWORD dwCVSize, DWORD dwOMAPtoSRC, DWORD dwOMAPfromSRC)
{
	if (dwCoffSize)
	{
		_tprintf(TEXT("    Internal COFF   Symbols - Size 0x%08x bytes\n"), dwCoffSize);
	}

	if (dwFPOSize)
	{
		_tprintf(TEXT("    Internal FPO    Symbols - Size 0x%08x bytes\n"), dwFPOSize);
	}

	if (dwCVSize)
	{
		_tprintf(TEXT("    Internal CV     Symbols - Size 0x%08x bytes\n"), dwCVSize);
	}

	if (dwOMAPtoSRC)
	{
		_tprintf(TEXT("    Internal -> SRC Symbols - Size 0x%08x bytes\n"), dwOMAPtoSRC);
	}

	if (dwOMAPfromSRC)
	{
		_tprintf(TEXT("    Internal SRC -> Symbols - Size 0x%08x bytes\n"), dwOMAPfromSRC);
	}

	
	return true;
}

//
// Dump DBG information
//
bool CModuleInfo::OutputDataToStdoutDbgSymbolInfo(LPCTSTR tszModulePointerToDbg, DWORD dwTimeDateStamp, DWORD dwChecksum, DWORD dwSizeOfImage, LPCTSTR tszDbgComment, DWORD dwExpectedTimeDateStamp, DWORD dwExpectedChecksum, DWORD dwExpectedSizeOfImage)
{
	if (!tszDbgComment)
	{
		// Dump out the pointer to the DBG file from the PE Image
		if (tszModulePointerToDbg)
		{
			_tprintf(TEXT("  Module Pointer to DBG = [%s]\n"), tszModulePointerToDbg);
		} else
		{
			_tprintf(TEXT("  Module had DBG File stripped from it.\n"));
		}

		time_t time = dwTimeDateStamp;
		_tprintf(TEXT("    Module TimeDateStamp = 0x%08x - %s"), dwTimeDateStamp, _tctime(&time));
		_tprintf(TEXT("    Module Checksum      = 0x%08x\n"), dwChecksum);
		_tprintf(TEXT("    Module SizeOfImage   = 0x%08x\n"), dwSizeOfImage);

	} else
	{
		TCHAR tszBuffer[2*_MAX_PATH]; // This should be large enough ;)
		size_t tszStringLength;

		// Is this discrepancy stuff...
		if (tszModulePointerToDbg)
		{
			_tprintf(TEXT("  DBG File = [%s] [%s]\n"), tszModulePointerToDbg, tszDbgComment);
		}

		time_t time = dwTimeDateStamp;
		_stprintf(tszBuffer, TEXT("    DBG TimeDateStamp    = 0x%08x - %s"), dwTimeDateStamp, _tctime(&time));

		// If our TimeDateStamps don't match... we have some fixup to do...
		if (dwTimeDateStamp != dwExpectedTimeDateStamp)
		{
			tszStringLength = _tcslen(tszBuffer);
			if (tszBuffer[tszStringLength-1] == '\n')
				tszBuffer[tszStringLength-1] = '\0';
		}
		
		_tprintf(tszBuffer);
		
		// If our TimeDateStamps don't match... we have some fixup to do...
		if (dwTimeDateStamp != dwExpectedTimeDateStamp)
		{
			_tprintf(TEXT(" [%s]!\n"), (dwTimeDateStamp > dwExpectedTimeDateStamp) ? TEXT("NEWER") : TEXT("OLDER"));
		}

		_tprintf(TEXT("    DBG Checksum         = 0x%08x [%s]\n"), dwChecksum, ( (dwChecksum == dwExpectedChecksum) ? TEXT("MATCHED"):TEXT("UNMATCHED")) );
		_tprintf(TEXT("    DBG SizeOfImage      = 0x%08x [%s]\n"), dwSizeOfImage, ( ( dwSizeOfImage == dwExpectedSizeOfImage) ? TEXT("MATCHED"):TEXT("UNMATCHED")) );
	}


	return true;
}

bool CModuleInfo::OutputDataToStdoutPdbSymbolInfo(DWORD dwPDBFormatSpecifier, LPTSTR tszModulePointerToPDB, DWORD dwPDBSignature, GUID * guidPDBGuid, DWORD dwPDBAge, LPCTSTR tszPdbComment)
{
	if (tszModulePointerToPDB)
	{
		if (!tszPdbComment)
		{
			_tprintf(TEXT("  Module Pointer to PDB = [%s]\n"), tszModulePointerToPDB);
		} else
		{
			_tprintf(TEXT("  PDB File = [%s] [%s]\n"), tszModulePointerToPDB, tszPdbComment);
		}
		switch (dwPDBFormatSpecifier)
		{
			case sigNB10:
				_tprintf(TEXT("    Module PDB Signature = 0x%x\n"), dwPDBSignature);
				break;
				
			case sigRSDS:
				wchar_t wszPDBGuid[39];
				TCHAR	tszPDBGuid[39];

				// We're going to convert the binary GUID format to a string for easier printing
				StringFromGUID2(*guidPDBGuid, wszPDBGuid, sizeof(wszPDBGuid)/sizeof(wchar_t));
				CUtilityFunctions::CopyUnicodeStringToTSTR(wszPDBGuid, tszPDBGuid, 39);

				_tprintf(TEXT("    Module PDB Guid = %s\n"), tszPDBGuid);
				break;
			
			case sigNOTMAPPED:
				_tprintf(TEXT("    Module PDB Signature = [Data Not Mapped into DMP file]\n"));
				break;

			default:
				_tprintf(TEXT("    UNKNOWN PDB Format!\n"));
				break;
		}
		
		if(dwPDBFormatSpecifier != sigNOTMAPPED)
		{
			_tprintf(TEXT("    Module PDB Age = 0x%x\n"), dwPDBAge);
		}

		// Do we have a problem because		
		if ( g_lpProgramOptions->GetMode(CProgramOptions::VerifySymbolsMode) && 
			m_enumPDBModuleStatus == SYMBOL_POSSIBLE_MISMATCH &&
			tszPdbComment)
		{
			if (m_fPDBSourceEnabled && (g_lpProgramOptions->GetSymbolSourceModes() == CProgramOptions::enumVerifySymbolsModeSourceSymbolsNotAllowed))
			{
				_tprintf(TEXT("    PDB Source Enabled (-NOSOURCE requested)\n"));
			} else
			if (!m_fPDBSourceEnabled && (g_lpProgramOptions->GetSymbolSourceModes() == CProgramOptions::enumVerifySymbolsModeSourceSymbolsOnly))
			{
				_tprintf(TEXT("    PDB Not Source Enabled (-SOURCEONLY requested)\n"));
			}
		}

	} else
	{
		_tprintf(TEXT("  Module has PDB File\n"));
		_tprintf(TEXT("  Module Pointer to PDB = [UNKNOWN] (Could not find in PE Image)\n"));
	}

	return true;
}

bool CModuleInfo::OutputDataToStdoutModuleInfo(DWORD dwModuleNumber)
{
	_tprintf(TEXT("Module[%3d] [%s] %s\n"), dwModuleNumber, m_tszPEImageModuleFileSystemPath, (m_dwPEImageDebugDirectoryCVSize ? TEXT("(Source Enabled)") : TEXT("")));

//	LPTSTR lpMachineArchitecture;
//
//	switch(m_wPEImageMachineArchitecture)
//	{
//		case IMAGE_FILE_MACHINE_I386:
//			lpMachineArchitecture = TEXT("Binary Image for Intel Machines");
//			break;
//
//		case IMAGE_FILE_MACHINE_ALPHA64:
//			lpMachineArchitecture = TEXT("Binary Image for Alpha Machines");
//			break;
//
//		default:
//			lpMachineArchitecture = TEXT("Binary Image for Unknown Machine Architecture");
//	}
//
//	if (m_wPEImageMachineArchitecture) _tprintf(TEXT("  %s\n"), lpMachineArchitecture);

	//
	// First, let's output version information if requested
	//
	if (g_lpProgramOptions->GetMode(CProgramOptions::CollectVersionInfoMode) )
	{
		// Version Information
		if (m_tszPEImageFileVersionCompanyName)	_tprintf(TEXT("  Company Name:      %s\n"), m_tszPEImageFileVersionCompanyName);
		if (m_tszPEImageFileVersionDescription)	_tprintf(TEXT("  File Description:  %s\n"), m_tszPEImageFileVersionDescription);
		if (m_tszPEImageProductVersionString)	_tprintf(TEXT("  Product Version:   %s\n"), m_tszPEImageProductVersionString);
		if (m_tszPEImageFileVersionString)	    _tprintf(TEXT("  File Version:      %s\n"), m_tszPEImageFileVersionString);
		if (m_dwPEImageFileSize)				_tprintf(TEXT("  File Size (bytes): %d\n"), m_dwPEImageFileSize);
		
		if ( m_ftPEImageFileTimeDateStamp.dwHighDateTime || m_ftPEImageFileTimeDateStamp.dwLowDateTime)
		{
			enum { FILETIME_BUFFERSIZE = 128 };
			TCHAR tszFileTime[FILETIME_BUFFERSIZE];
			
			if (OutputFileTime(m_ftPEImageFileTimeDateStamp, tszFileTime, FILETIME_BUFFERSIZE))
				_tprintf(TEXT("  File Date:         %s\n"), tszFileTime);
		}
	}

	return true;
}

bool CModuleInfo::OutputDataToStdoutThisModule()
{
	//
	// If we're not doing "Discrepancies Only" then we output this module unconditionally...
	//
	if (!g_lpProgramOptions->GetMode(CProgramOptions::OutputDiscrepanciesOnly))
		return true;

	//
	// If we're not in verification mode, then we output everything...
	//
	if (!g_lpProgramOptions->GetMode(CProgramOptions::VerifySymbolsMode))
		return true;
	
	//
	// This is "Discrepancy Only" mode, so check for discrepancies...
	//
	bool fAnyDiscrepancies = false;

	// Hey, if they only want to dump out modules with discrepancies... check to see
	// if this qualifies...
	switch (m_enumPEImageSymbolStatus)
	{
		// Consider these normal status codes...
		case SYMBOLS_DBG:
		case SYMBOLS_DBG_AND_PDB:
		case SYMBOLS_PDB:
		case SYMBOLS_LOCAL:
			break;

		// Anything else is worth reporting...
		default:
			fAnyDiscrepancies = true;
	}

	// If we don't have a discrepancy yet... let's look further...
	if (!fAnyDiscrepancies)
	{
		// Is there a DBG file?
		if ( (m_enumPEImageSymbolStatus == SYMBOLS_DBG) ||
			 (m_enumPEImageSymbolStatus == SYMBOLS_DBG_AND_PDB) )
		{
			// Does it match?
			if ( m_enumDBGModuleStatus != SYMBOL_MATCH )
				fAnyDiscrepancies = true;
		}

		// Is there a PDB file?
		if ( GetDebugDirectoryPDBPath() )
		{
			if (m_enumPDBModuleStatus != SYMBOL_MATCH )
				fAnyDiscrepancies = true;
		}
	}

	return fAnyDiscrepancies;
}
