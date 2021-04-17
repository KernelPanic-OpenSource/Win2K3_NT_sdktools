/****************************** Module Header ******************************\
* Module Name: sofile.c
*
* Copyright (c) 1985-96, Microsoft Corporation
*
* 04/09/96 GerardoB Created
\***************************************************************************/
#include "structo.h"

/*********************************************************************
* soWriteOutputFileHeader
*
\***************************************************************************/
BOOL soWriteOutputFileHeader (PWORKINGFILES pwf)
{
    char ** ppszHeader;

    /*
     * If building list only, done
     */
    if (pwf->dwOptions & SOWF_LISTONLY) {
        return TRUE;
    }

    if (   !soWriteFile(pwf->hfileOutput, "/*********************************************************************\\\r\n")
        || !soWriteFile(pwf->hfileOutput, "* File: %s\r\n", pwf->pszOutputFile)
        || !soWriteFile(pwf->hfileOutput, "* Generated by StructO on %s at %s\r\n", __DATE__, __TIME__)
        || !soWriteFile(pwf->hfileOutput, "\\*********************************************************************/\r\n\r\n")) {

        return FALSE;
   }

   if (pwf->dwOptions & SOWF_INLCLUDEPRECOMPH) {
       if (!soWriteFile(pwf->hfileOutput, gszPrecomph)) {
           return FALSE;
       }
   }

   /*
    * structure definitions for generated tables
    */
   ppszHeader = gpszHeader;
   while (*ppszHeader != NULL) {
       if (!soWriteFile(pwf->hfileOutput, *ppszHeader)
            || !soWriteFile(pwf->hfileOutput, "\r\n")) {

           return FALSE;
       }
       ppszHeader++;
   }


   return TRUE;
}
/*********************************************************************
* soUnmapFile
*
\***************************************************************************/
void soUnmapFile (PFILEMAP pfm)
{
    if (pfm->pmapStart != NULL) {
        UnmapViewOfFile(pfm->pmap);
        pfm->pmapStart = NULL;
        pfm->pmap = NULL;
        pfm->pmapEnd = NULL;
    }

    if (pfm->hmap != NULL) {
        CloseHandle(pfm->hmap);
        pfm->hmap = NULL;
    }

    if (pfm->hfile != INVALID_HANDLE_VALUE) {
        CloseHandle(pfm->hfile);
        pfm->hfile = INVALID_HANDLE_VALUE;
    }
}
/*********************************************************************
* soMapFile
*
\***************************************************************************/
BOOL soMapFile (char * pszFile, PFILEMAP pfm)
{
    DWORD dwFileSize;

    pfm->hfile = CreateFile(pszFile, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
    if (pfm->hfile == INVALID_HANDLE_VALUE) {
        soLogMsg(SOLM_APIERROR, "CreateFile");
        goto CleanupAndFail;
    }

    dwFileSize = GetFileSize(pfm->hfile, NULL);
    if (dwFileSize == 0xFFFFFFFF) {
        soLogMsg(SOLM_APIERROR, "GetFileSize");
        goto CleanupAndFail;
    }

    pfm->hmap = CreateFileMapping(pfm->hfile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (pfm->hmap == NULL) {
        soLogMsg(SOLM_APIERROR, "CreateFileMapping");
        goto CleanupAndFail;
    }

    pfm->pmapStart = MapViewOfFile(pfm->hmap, FILE_MAP_READ, 0, 0, 0);
    if (pfm->pmapStart == NULL) {
        soLogMsg(SOLM_APIERROR, "MapViewOfFile");
        goto CleanupAndFail;
    }
    pfm->pmap = pfm->pmapStart;
    pfm->pmapEnd = pfm->pmapStart + dwFileSize;

    return TRUE;

CleanupAndFail:
    soLogMsg(SOLM_ERROR, "soMapFile failed. File: '%s'", pszFile);
    soUnmapFile (pfm);
    return FALSE;
}
/*********************************************************************
* soBuildStructsList
*
\***************************************************************************/
BOOL soBuildStructsList (PWORKINGFILES pwf)
{
    static char gszEOL [] = "\r\n";

    char * pmap, * pStruct;
    FILEMAP fm;
    PSTRUCTLIST psl;
    PVOID pTemp;
    UINT uAlloc, uCount, uSize;


    soLogMsg (SOLM_NOEOL, "Building structs list from %s ...", pwf->pszStructsFile);

    if (!soMapFile (pwf->pszStructsFile, &fm)) {
        goto CleanupAndFail;
    }

    /*
     * Let's guess a number of structures
     */
#define SO_LISTSIZE 20
     uAlloc = SO_LISTSIZE;

    /*
     * Allocate list
     */
    pwf->psl = (PSTRUCTLIST) LocalAlloc(LPTR, sizeof(STRUCTLIST) * (uAlloc + 1));
    if (pwf->psl == NULL) {
        soLogMsg(SOLM_APIERROR, "LocalAlloc");
        goto CleanupAndFail;
    }

    /*
     * Load structure names
     */
    pmap = fm.pmapStart;
    psl = pwf->psl;
    uCount = 0;
    while (pmap < fm.pmapEnd) {
        /*
         * Name should start at first column
         */
        if (!soIsIdentifierChar(*pmap)) {
            /*
             * Skip this line
             */
             pmap = soFindTag(pmap, fm.pmapEnd, gszEOL);
             if (pmap == NULL) {
                 break;
             }
             pmap += sizeof(gszEOL) - 1;
             continue;
        }

        /*
         * Find the name
         */
        pStruct = soGetIdentifier (pmap, fm.pmapEnd, &uSize);
        if (pStruct == NULL) {
            soLogMsg(SOLM_ERROR, "soGetIdentifier failed.");
            goto CleanupAndFail;
        }

        /*
         * Grow the list if needed
         */
         if (uCount >= uAlloc) {
             soLogMsg (SOLM_APPEND, ".");
             uAlloc += SO_LISTSIZE;
             pTemp = LocalReAlloc(pwf->psl, sizeof(STRUCTLIST) * (uAlloc + 1), LMEM_MOVEABLE | LMEM_ZEROINIT);
             if (pTemp == NULL) {
                 soLogMsg(SOLM_APIERROR, "LocalReAlloc");
                 goto CleanupAndFail;
             }
             pwf->psl = (PSTRUCTLIST)pTemp;
             psl = pwf->psl + uCount;
         }

        /*
         * Copy it
         */
        psl->uSize = uSize;
        psl->pszName = soCopyTagName (pStruct, uSize);
        if (psl->pszName == NULL) {
            goto CleanupAndFail;
        }

        psl++;
        uCount++;
        pmap = pStruct + uSize;
    }

    /*
     * Make sure it found at least one struct
     */
    if (uCount == 0) {
        soLogMsg(SOLM_ERROR, "Failed to get structure name");
        goto CleanupAndFail;
    }

    /*
     * Let's save some memory
     */
    if (uCount < uAlloc) {
        pTemp = LocalReAlloc(pwf->psl, sizeof(STRUCTLIST) * (uCount + 1), LMEM_MOVEABLE | LMEM_ZEROINIT);
        if (pTemp == NULL) {
            soLogMsg(SOLM_APIERROR, "LocalReAlloc");
            goto CleanupAndFail;
        }
        pwf->psl = (PSTRUCTLIST)pTemp;
    }

    soUnmapFile (&fm);
    soLogMsg (SOLM_NOLABEL, ".");
    return TRUE;

CleanupAndFail:
    soLogMsg(SOLM_ERROR, "soBuildStructsList failed. File: '%s'", pwf->pszStructsFile);
    soUnmapFile (&fm);
    /*
     * The process is going away so never mind the heap
     */
    return FALSE;
}
/*********************************************************************
* soIncludeInputFile
*
* Add #include <pszInputFile Name>.<pszIncInputFileExt> to output file
\***************************************************************************/
BOOL soIncludeInputFile (PWORKINGFILES pwf)
{
    BOOL fRet;
    char * pszIncFile, * pDot;
    UINT uInputFileNameSize;

    /*
     * If building list only, done
     */
    if (pwf->dwOptions & SOWF_LISTONLY) {
        return TRUE;
    }

    /*
     * Allocate a buffer to build the name
     */
    uInputFileNameSize = lstrlen(pwf->pszInputFile);
    pszIncFile = (char *) LocalAlloc(LPTR,
                          uInputFileNameSize + lstrlen(pwf->pszIncInputFileExt) + 2);
    if (pszIncFile == NULL) {
        soLogMsg(SOLM_APIERROR, "LocalAlloc");
        return FALSE;
    }

    /*
     * Copy file name
     */
    pDot = soFindChar (pwf->pszInputFile, pwf->pszInputFile + uInputFileNameSize, '.');
    if (pDot == NULL) {
        strcpy(pszIncFile, pwf->pszInputFile);
        strcat(pszIncFile, ".");
    } else {
        strncpy(pszIncFile, pwf->pszInputFile, (UINT)(pDot - pwf->pszInputFile + 1));
    }

    /*
     * Copy extension and write it to output file
     */
     strcat(pszIncFile, pwf->pszIncInputFileExt);
     fRet = soWriteFile(pwf->hfileOutput, gszIncInput, pszIncFile);

     LocalFree(pszIncFile);
     return fRet;
}
/*********************************************************************
* soOpenWorkingFiles
*
\***************************************************************************/
BOOL soOpenWorkingFiles (PWORKINGFILES pwf)
{
    char szTempPath [MAX_PATH];
    char szTempFile [MAX_PATH];
    DWORD dwFileSize;

    /*
     * Load the structures list if provided and not built already
     */
    if ((pwf->pszStructsFile != NULL) && (pwf->psl == NULL)) {
        if (!soBuildStructsList(pwf)) {
            goto CleanupAndFail;
        }
    }

    /*
     * Map input file
     */
    if (!soMapFile (pwf->pszInputFile, (PFILEMAP) &(pwf->hfileInput))) {
        goto CleanupAndFail;
    }


    /*
     * Open output file if not open already
     */
    if (pwf->hfileOutput == INVALID_HANDLE_VALUE) {
        pwf->hfileOutput = CreateFile(pwf->pszOutputFile, GENERIC_WRITE, 0, NULL,
                            (pwf->dwOptions & SOWF_APPENDOUTPUT ? OPEN_EXISTING : CREATE_ALWAYS),
                            FILE_ATTRIBUTE_NORMAL,  NULL);
        if (pwf->hfileOutput == INVALID_HANDLE_VALUE) {
            soLogMsg(SOLM_APIERROR, "CreateFile");
            soLogMsg(SOLM_ERROR, "Failed to open output file: %s", pwf->pszOutputFile);
            goto CleanupAndFail;
        }

        if (pwf->dwOptions & SOWF_APPENDOUTPUT) {
            if (0xFFFFFFFF == SetFilePointer (pwf->hfileOutput, 0, 0, FILE_END)) {
                soLogMsg(SOLM_APIERROR, "SetFilePointer");
                goto CleanupAndFail;
            }
        } else {
            if (!soWriteOutputFileHeader(pwf)) {
                goto CleanupAndFail;
            }
        }
    }


    /*
     * #include input file if requested
     */
    if (pwf->dwOptions & SOWF_INCLUDEINPUTFILE) {
        if (!soIncludeInputFile(pwf)) {
            goto CleanupAndFail;
        }
    }

    /*
     * Create temp file if not created already
     */
    if (pwf->hfileTemp == INVALID_HANDLE_VALUE) {
        if (!GetTempPath(sizeof(szTempPath) - 1, szTempPath)) {
            soLogMsg(SOLM_APIERROR, "GetTempPath");
            goto CleanupAndFail;
        }

        if (!GetTempFileName(szTempPath, "sot", 0, szTempFile)) {
            soLogMsg(SOLM_APIERROR, "GetTempFileName");
            soLogMsg(SOLM_ERROR, "Failed to get temp file name. szTempPath: %s.", szTempPath);
            goto CleanupAndFail;
        }

        pwf->hfileTemp = CreateFile(szTempFile, GENERIC_WRITE | GENERIC_READ, 0, NULL,
                            CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE,  NULL);
        if (pwf->hfileTemp == INVALID_HANDLE_VALUE) {
            soLogMsg(SOLM_APIERROR, "CreateFile");
            soLogMsg(SOLM_ERROR, "Failed to create temp file: '%s'", szTempFile);
            goto CleanupAndFail;
        }

        if (!soWriteFile(pwf->hfileTemp, "%s", gszTableDef)) {
            goto CleanupAndFail;
        }
    }


    return TRUE;

CleanupAndFail:
    soLogMsg(SOLM_ERROR, "soOpenWorkingFiles Failed");
    soCloseWorkingFiles (pwf, SOCWF_CLEANUP);
    return FALSE;
}

/*********************************************************************
* soCloseWorkingFiles
*
\***************************************************************************/
BOOL soCloseWorkingFiles (PWORKINGFILES pwf, DWORD dwFlags)
{

   if (dwFlags & SOCWF_CLEANUP) {
       if (pwf->hfileTemp != INVALID_HANDLE_VALUE) {
            CloseHandle(pwf->hfileTemp);
            pwf->hfileTemp = INVALID_HANDLE_VALUE;
       }
       if (pwf->hfileOutput != INVALID_HANDLE_VALUE) {
            CloseHandle(pwf->hfileOutput);
            pwf->hfileOutput = INVALID_HANDLE_VALUE;
       }
       if (pwf->psl != NULL) {
           // Never mind cleanning the heap. The process is going away.
       }
   }

   soUnmapFile((PFILEMAP) &(pwf->hfileInput));

   return TRUE;
}
/*********************************************************************
* soWriteFile
*
\***************************************************************************/
BOOL __cdecl soWriteFile(HANDLE hfile, char *pszfmt, ...)
{
    static char gszbuff [1024+1];

    BOOL fRet = TRUE;
    va_list va;
    DWORD dwWritten;

    va_start(va, pszfmt);
    vsprintf(gszbuff, pszfmt, va);

    if (!WriteFile(hfile, gszbuff, strlen(gszbuff), &dwWritten, NULL)) {
        soLogMsg(SOLM_APIERROR, "WriteFile");
        soLogMsg(SOLM_ERROR, "buffer not written: %s.", gszbuff);
        fRet = FALSE;
    }

    va_end(va);
    return fRet;
}
/*********************************************************************
* soCopyStructuresTable
*
\***************************************************************************/
BOOL soCopyStructuresTable (PWORKINGFILES pwf)
{
    static char szTemp[1024];

    char ** ppszTail;
    DWORD dwFileSize, dwRead, dwWritten;
    PSTRUCTLIST psl;
    UINT uLoops;

    soLogMsg (SOLM_NOEOL, "Writting structs table ...");

    /*
     * If there are no structures, bail.
     * If there was a struct list, fail
     */
    if (pwf->uTablesCount == 0) {
        if (pwf->psl != NULL) {
            soLogMsg(SOLM_ERROR, "None of the structures in '%s' was found", pwf->pszStructsFile);
            return FALSE;
        } else {
            return TRUE;
        }
    }

    /*
     * If building list only, done
     */
    if (pwf->dwOptions & SOWF_LISTONLY) {
        soLogMsg (SOLM_DEFAULT, "%d Structures found.", pwf->uTablesCount);
        return TRUE;
    }

   /*
     * Let them know if we didn't find any of the structures in the list
     */
    if (pwf->psl != NULL) {
        psl = pwf->psl;
        while (psl->uSize != 0) {
            if (psl->uCount == 0) {
                soLogMsg(SOLM_WARNING, "Structure not found: %s", psl->pszName);
            }
            psl++;
        }
    }

    if (!soWriteFile(pwf->hfileTemp, "%s", gszTableEnd)) {
        goto MsgAndFail;
    }

   /*
    * Move to beginning of temp file
    */
   dwFileSize = GetFileSize(pwf->hfileTemp, NULL);
   if (dwFileSize == 0xFFFFFFFF) {
       soLogMsg(SOLM_APIERROR, "GetFileSize");
       goto MsgAndFail;
   }

   if (0xFFFFFFFF == SetFilePointer (pwf->hfileTemp, 0, 0, FILE_BEGIN)) {
       soLogMsg(SOLM_APIERROR, "SetFilePointer");
       goto MsgAndFail;
   }

   /*
    * Append temp file to output file
    */
   uLoops = 0;
   while (dwFileSize != 0) {
       if (!ReadFile(pwf->hfileTemp, szTemp, sizeof(szTemp), &dwRead, NULL)) {
           soLogMsg(SOLM_APIERROR, "ReadFile");
           goto MsgAndFail;
       }

       if (!WriteFile(pwf->hfileOutput, szTemp, dwRead, &dwWritten, NULL)) {
           soLogMsg(SOLM_APIERROR, "WriteFile");
           goto MsgAndFail;
       }

       dwFileSize -= dwRead;
       if (++uLoops == 50) {
           uLoops = 0;
           soLogMsg (SOLM_APPEND, ".");
       }
   }
   soLogMsg (SOLM_NOLABEL, ".");

   soLogMsg (SOLM_DEFAULT, "%d Tables generated.", pwf->uTablesCount);

   /*
    * Write file tail (code)
    */
   ppszTail = gpszTail;
    while (*ppszTail != NULL) {
    if (!soWriteFile(pwf->hfileOutput, *ppszTail)
         || !soWriteFile(pwf->hfileOutput, "\r\n")) {

        return FALSE;
    }
    ppszTail++;
}


   return TRUE;

MsgAndFail:
   soLogMsg(SOLM_ERROR, "soCopyStructuresTable failed.");
   return FALSE;
}