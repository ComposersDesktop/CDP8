/*
 * Copyright (c) 1983-2013 Martin Atkins, Richard Dobson and Composers Desktop Project Ltd
 * http://people.bath.ac.uk/masrwd
 * http://www.composersdesktop.com
 *
 This file is part of the CDP System.

 The CDP System is free software; you can redistribute it
 and/or modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 The CDP System is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with the CDP System; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307 USA
 *
 */



/***********************************************************

 How to Obtain Filename and Path from a Shell Link or Shortcut

 Last reviewed: July 19, 1996
 Article ID: Q130698
 The information in this article applies to:

 * Microsoft Win32 Software Development Kit (SDK), versions 3.51
      and 4.0

 SUMMARY

 The shortcuts used in Microsoft Windows 95 provide applications and
 users a way to create shortcuts or links to objects in the shell's
 namespace. The IShellLink OLE Interface can be used to obtain the
 path and filename from the shortcut, among other things.

 MORE INFORMATION

 A shortcut allows the user or an application to access an object
 from anywhere in the namespace. Shortcuts to objects are stored as
 binary files. These files contain information such as the path to
 the object, working directory, the path of the icon used to display
 the object, the description string, and so on.

 Given a shortcut, applications can use the IShellLink interface and
 its functions to obtain all the pertinent information about that
 object. The IShellLink interface supports functions such as
 GetPath(), GetDescription(), Resolve(), GetWorkingDirectory(), and
 so on.

 Sample Code

 The following code shows how to obtain the filename or path and
 description of a given link file:

************************************************/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include <shlobj.h>
#include <io.h>
#include "alias.h"               //RWD: for wrapper funcs

/********* CONTENTS OF ALIAS.H :

           int COMinit(void);
           void COMclose(void);
           int getAliasInfo(const char *linkfilename, char *path, char *desc);
           int fileExists(const char *name);

           *********** END OF CONTENTS OF ALIAS.H ************/

static void ErrorMessage(LPCTSTR str, HRESULT hr);

/* RWD: for encapsulation, this should be declared static

 * GetLinkInfo() fills the filename and path buffer
 * with relevant information.
 * hWnd         - calling application's window handle.
 *
 * lpszLinkName - name of the link file passed into the function.
 *
 * lpszPath     - the buffer that receives the file's path name.
 *
 * lpszDescription - the buffer that receives the file's
 * description.
 */

HRESULT
GetLinkInfo( HWND    hWnd,
             LPCTSTR lpszLinkName,
             LPSTR   lpszPath,
             LPSTR   lpszDescription)
{

    HRESULT hres;
    IShellLink *pShLink;
    WIN32_FIND_DATA wfd;

    /* Initialize the return parameters to null strings.*/
    *lpszPath = '\0';
    *lpszDescription = '\0';

    /* Call CoCreateInstance to obtain the IShellLink
     * Interface pointer. This call fails if
     * CoInitialize is not called, so it is assumed that
     * CoInitialize has been called.
     */
    hres = CoCreateInstance( &CLSID_ShellLink,
                             NULL,
                             CLSCTX_INPROC_SERVER,
                             &IID_IShellLink,
                             (LPVOID *)&pShLink );

    if (SUCCEEDED(hres))
        {
            IPersistFile *ppf;

            /* The IShellLink Interface supports the IPersistFile
             * interface. Get an interface pointer to it.
             */
            hres = pShLink->lpVtbl->QueryInterface(pShLink,
                                                   &IID_IPersistFile,
                                                   (LPVOID *)&ppf );
            if (SUCCEEDED(hres))
                {
                    WORD wsz[MAX_PATH];

                    /* Convert the given link name string to a wide character string. */
                    MultiByteToWideChar( CP_ACP, 0,
                                         lpszLinkName,
                                         -1, wsz, MAX_PATH );
                    /* Load the file. */
                    hres = ppf->lpVtbl->Load(ppf, wsz, STGM_READ );
                    if (SUCCEEDED(hres))
                        {
                            /* Resolve the link by calling the Resolve() interface function.
                             * This enables us to find the file the link points to even if
                             * it has been moved or renamed.
                             */
                            hres = pShLink->lpVtbl->Resolve(pShLink,  hWnd,
                                                            SLR_ANY_MATCH | SLR_NO_UI);
                            if (SUCCEEDED(hres))
                                {
                                    /* Get the path of the file the link points to. */
                                    hres = pShLink->lpVtbl->GetPath( pShLink, lpszPath,
                                                                     MAX_PATH,
                                                                     &wfd,
                                                                     SLGP_SHORTPATH );

                                    /* Only get the description if we successfully got the path
                                     * (We can't return immediately because we need to release ppf &
                                     *  pShLink.)
                                     */
                                    if(SUCCEEDED(hres))
                                        {
                                            /* Get the description of the link.  ** RWD: probably empty most times! */
                                            hres = pShLink->lpVtbl->GetDescription(pShLink,
                                                                                   lpszDescription,
                                                                                   MAX_PATH );
                                        }
                                }
                        }
                    ppf->lpVtbl->Release(ppf);
                }
            pShLink->lpVtbl->Release(pShLink);
        }
    else
        ErrorMessage("Failure: CoCreateInstance",hres);      /*RWD: eg of error message usage*/
    return hres;
}

/********* RWD utility functions *********/


int COMinit(void)
{
    HRESULT res = CoInitialize(NULL);
    if(res==S_OK)
        return 1;
    else
        return 0;
}

void COMclose(void)
{
    CoUninitialize();
}

int fileExists(const char *name)
{
    struct _finddata_t file;
    long hFile;

    hFile = _findfirst((char *)name,&file);
    return (int)(hFile != -1L);
}
/*the wrapper function itself, for a console application (no window handle) */
/*requires COM to be initialized*/
int getAliasInfo(const char *linkfilename, char *path, char *desc)
{
    HRESULT res;
    res = GetLinkInfo(NULL, (LPCTSTR) linkfilename, (LPSTR) path, (LPSTR) desc);
    return (int) SUCCEEDED(res);
}

/*from Inside COM: DaleRogerson, changed from C++ to plain C*/
void ErrorMessage(LPCTSTR str, HRESULT hr)
{
    void *pMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL,hr,MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
                  (LPTSTR) &pMsgBuf, 0, NULL);

    printf("\n%s\n%s\n",str,(const char *)pMsgBuf);
    LocalFree(pMsgBuf);
}
