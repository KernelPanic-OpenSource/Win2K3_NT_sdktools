/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    listmung.c

Abstract:

    This is the main module for a stubfile generation utility

Author:

    Sanford Staab (sanfords) 22-Apr-1992

Revision History:

--*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windef.h>

#define STRING_BUFFER_SIZE 120
char StringBuffer[STRING_BUFFER_SIZE];
char ItemBuffer[STRING_BUFFER_SIZE];
char ItemBuffer2[STRING_BUFFER_SIZE];
char *ListName, *TemplateName;
FILE *ListFile, *TemplateFile;

char szBEGINTRANSLATE[] = "BeginTranslate";
char szENDTRANSLATE[] = "EndTranslate";
char szENDTRANSLATEQLPC[] = "EndTranslateQLPC";

BOOL IsTranslateTag(char * pBuffer)
{
    return (_strnicmp(pBuffer, szBEGINTRANSLATE, sizeof(szBEGINTRANSLATE)-1) == 0)
            || (_strnicmp(pBuffer, szENDTRANSLATE, sizeof(szENDTRANSLATE)-1) == 0);
}

BOOL IsCommentOrTag(char * pBuffer)
{
    return ((*pBuffer == ';') || IsTranslateTag(pBuffer));
}

int
ProcessParameters(
    int argc,
    char *argv[]
    )
{
    char c, *p;

    while (*++argv != NULL) {

        p = *argv;

        //
        // if we have a delimiter for a parameter, case throught the valid
        // parameter. Otherwise, the rest of the parameters are the list of
        // input files.
        //

        if (*p == '/' || *p == '-') {

            //
            // Switch on all the valid delimiters. If we don't get a valid
            // one, return with an error.
            //

            c = *++p;

            switch (toupper( c )) {
            default:
                return 0;
            }

        } else {

            ListName = *argv++;
            TemplateName = *argv++;

            return (ListName && TemplateName);
        }
    }

    return 0;
}


BOOL mysubstr(
char *s,
char *find,
char *put)
{
    char *p;
    if (p = strstr(s, find)) {
        strcpy(p, put);
        strcpy(p + strlen(put), p + strlen(find));  // find > put!
        return(TRUE);
    }
    return(FALSE);
}


VOID myprint(
char *s,
char *item,
int index)
{
    if (strstr(s, "%d") || mysubstr(s, "%%INDEX%%", "%d")) {
        printf(s, item, index);
    } else {
        printf(s, item);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   myfgets
//
//  Synopsis:  Calls fgets to read a string from a file.
//              It ignores empty lines or lines starting with a blank character
//
//              This is needed when the input file was generated by the
//               compiler preprocessor. Using the preprocessor allows us
//               to use #idfdef, #inlcude, etc in the original list file.
//
//  Arguments:  [pszBuff] -- Buffer to store the string
//              [iBuffSize] -- Buffer size
//              [pFile] -- Pointer to the file to read
//
//  Return      [pszRet] -- Pointer to pszBuff if succesful read. NULL otherwise.
//----------------------------------------------------------------------------
char * myfgets(
char * pszBuff,
int iBuffSize,
FILE * pFile)
{
    char *pszRet;
    while (pszRet = fgets(pszBuff, iBuffSize, pFile)) {
        if ((*pszRet == '\n') || (*pszRet == ' ')) {
            continue;
        } else {
            break;
        }
    }

    return pszRet;
}
//+---------------------------------------------------------------------------
//
//  Function:   SkipCommentsAndTags
//
//  Synopsis:  Calls myfgets to read a string from a file.
//              It ignores lines starting with ; (ie, comments) and lines
//               containing the Begin/EndTranslate tags.
//
//  Arguments:  [pszBuff] -- Buffer to store the string
//              [iBuffSize] -- Buffer size
//              [pFile] -- Pointer to the file to read
//
//  Return      [pszRet] -- Pointer to pszBuff if succesful read. NULL otherwise.
//----------------------------------------------------------------------------
char * SkipCommentsAndTags(
char * pszBuff,
int iBuffSize,
FILE * pFile)
{
    char *pszRet;
    while (pszRet = myfgets(pszBuff, iBuffSize, pFile)) {
        if (IsCommentOrTag(pszRet)) {
            continue;
        } else {
            break;
        }
    }

    return pszRet;
}

void
ProcessTemplate( void )
{
    char *s;
    char *pchItem;
    char *pchLastItem;
    int index;

    s = fgets(StringBuffer,STRING_BUFFER_SIZE,TemplateFile);

    while ( s ) {
        if (mysubstr(s, "%%FOR_ALL_UPPER%%", "%-45s")) {
            rewind(ListFile);
            index = 0;
            while (pchItem = myfgets(ItemBuffer, STRING_BUFFER_SIZE, ListFile)) {
                if (ItemBuffer[0] != ';') {
                    pchItem[strlen(pchItem) - 1] = '\0';  // strip off \n
                    pchItem = _strupr(pchItem);
                    if (IsTranslateTag(ItemBuffer)) {
                        myprint(s, pchItem, index);
                    } else {
                        myprint(s, pchItem, index++);
                    }
                } else {
                    printf("// %s", ItemBuffer);
                }
            }

        } else if (mysubstr(s, "%%FOR_ALL%%", "%s")) {
            rewind(ListFile);
            index = 0;
            while (pchItem = myfgets(ItemBuffer, STRING_BUFFER_SIZE, ListFile)) {
                if (ItemBuffer[0] != ';') {
                    pchItem[strlen(pchItem) - 1] = '\0';  // strip off \n
                    myprint(s, pchItem, index++);
                }
            }

        } else if (mysubstr(s, "%%FOR_ALL_QLPC%%", "%s")) {
            rewind(ListFile);
            index = 0;
            while (pchItem = myfgets(ItemBuffer, STRING_BUFFER_SIZE, ListFile)) {
                if (ItemBuffer[0] != ';') {
                    pchItem[strlen(pchItem) - 1] = '\0';  // strip off \n
                    if (_strnicmp(ItemBuffer, szENDTRANSLATEQLPC, sizeof(szENDTRANSLATEQLPC)-1) == 0)
                        break;
                    myprint(s, pchItem, index++);
                }
            }

        } else if (mysubstr(s, "%%FOR_ALL_LPC%%", "%s")) {
            rewind(ListFile);
            index = 0;
            while (pchItem = myfgets(ItemBuffer, STRING_BUFFER_SIZE, ListFile)) {
                if (ItemBuffer[0] != ';') {
                    pchItem[strlen(pchItem) - 1] = '\0';  // strip off \n
                    if (_strnicmp(ItemBuffer, szENDTRANSLATEQLPC, sizeof(szENDTRANSLATEQLPC)-1) == 0)
                        break;
                }
            }
            while (pchItem = myfgets(ItemBuffer, STRING_BUFFER_SIZE, ListFile)) {
                if (ItemBuffer[0] != ';') {
                    pchItem[strlen(pchItem) - 1] = '\0';  // strip off \n
                    myprint(s, pchItem, index++);
                }
            }

        } else if (mysubstr(s, "%%FOR_ALL_BUT_LAST%%", "%s")) {
            rewind(ListFile);
            index = 0;
            pchLastItem = SkipCommentsAndTags(ItemBuffer, STRING_BUFFER_SIZE, ListFile);
            if (pchLastItem != NULL) {
                pchLastItem[strlen(pchLastItem) - 1] = '\0';  // strip off \n
                while (pchItem = myfgets(ItemBuffer2, STRING_BUFFER_SIZE, ListFile)) {
                    if (!IsCommentOrTag(pchItem)) {
                        pchItem[strlen(pchItem) - 1] = '\0';  // strip off \n
                        myprint(s, pchLastItem, index++);     // Write previous line
                        strcpy(pchLastItem, pchItem);         // Save current line
                    }
                }
            } else {
                fprintf(stderr,"LISTMUNG: FOR_ALL_BUT_LAST: no lines found\n");
            }

        } else if (mysubstr(s, "%%FOR_LAST%%", "%s")) {
            rewind(ListFile);
            index = 0;
            pchLastItem = SkipCommentsAndTags(ItemBuffer, STRING_BUFFER_SIZE, ListFile);
            if (pchLastItem != NULL) {
                pchLastItem[strlen(pchLastItem) - 1] = '\0';  // strip off \n
                while (pchItem = myfgets(ItemBuffer2, STRING_BUFFER_SIZE, ListFile)) {
                    if (!IsCommentOrTag(pchItem)) {
                        pchItem[strlen(pchItem) - 1] = '\0';  // strip off \n
                        strcpy(pchLastItem, pchItem);         // Save current line
                        index++;
                    }
                }
                myprint(s, pchLastItem, index);         // Write Last line.
            } else {
                fprintf(stderr,"LISTMUNG: FOR_LAST: no lines found\n");
            }

        } else {
            printf("%s", s);
        }
        s = fgets(StringBuffer,STRING_BUFFER_SIZE,TemplateFile);
    }
}



int
__cdecl main( argc, argv )
int argc;
char *argv[];
{

    if (!ProcessParameters( argc, argv )) {

        fprintf( stderr, "Stub File Generation Utility. Version: 1.1\n" );
        fprintf( stderr, "usage: listmung <symbol_list_file> <template>\n" );
        fprintf( stderr, " Converts the elements in the list file into an output file\n" );
        fprintf( stderr, " where the template dictates the format.  The following strings\n");
        fprintf( stderr, " are substituted apropriately:\n");
        fprintf( stderr, " %%FOR_ALL%%\n");
        fprintf( stderr, " %%FOR_ALL_UPPER%%\n");
        fprintf( stderr, " %%FOR_ALL_BUT_LAST%%\n");
        fprintf( stderr, " %%FOR_LAST%%\n");
        fprintf( stderr, " %%INDEX%%\n");
        fprintf( stderr, " output is to stdout.\n");

        return 1;

    }

    if ( (ListFile = fopen(ListName,"r")) == 0) {
        fprintf(stderr,"LISTMUNG: Unable to open list file.\n");
        return 1;
    }

    if ( (TemplateFile = fopen(TemplateName,"r")) == 0) {
        fprintf(stderr,"LISTMUNG: Unable to open template file.\n");
        return 1;
    }

    ProcessTemplate();

    fclose(ListFile);
    fclose(TemplateFile);

    return( 0 );
}
