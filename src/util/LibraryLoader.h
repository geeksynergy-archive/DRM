/******************************************************************************\
 *
 * Copyright (c) 2012
 *
 * Author(s):
 *	David Flamand
 *
 * Description:
 *	Dynamic Link Library Loader
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#ifndef _LIBRARYLOADER_H_
#define _LIBRARYLOADER_H_

#ifdef _WIN32
# include <windows.h>
# define LOADLIB(a) (void*)LoadLibraryA(a)
# define GETPROC(a, b) (void*)GetProcAddress((HMODULE)a, b)
# define FREELIB(a) FreeLibrary((HMODULE)a)
#else
# include <dlfcn.h>
# define LOADLIB(a) dlopen(a, RTLD_LOCAL | RTLD_NOW)
# define GETPROC(a, b) dlsym(a, b)
# define FREELIB(a) dlclose(a)
#endif

typedef struct LIBFUNC
{
	const char *pcFunctionName;
	void **ppvFunctionAddress;
	void *pvDummyFunctionAddress;
} LIBFUNC;

class CLibraryLoader
{
public:
	static void* Load(const char** LibraryNames, const LIBFUNC* LibFuncs, bool (*LibCheckCallback)()=NULL)
	{
		void* hLib = NULL;
		for (int l = 0; LibraryNames[l]; l++)
		{
			hLib = LOADLIB(LibraryNames[l]);
			if (hLib != NULL)
			{
				int f;
				for (f = 0; LibFuncs[f].pcFunctionName; f++)
				{
					void *pvFunc = GETPROC(hLib, LibFuncs[f].pcFunctionName);
					if (!pvFunc)
						break;
					*LibFuncs[f].ppvFunctionAddress = pvFunc;
				}
				if (!LibFuncs[f].pcFunctionName)
				{
					if (LibCheckCallback==NULL || LibCheckCallback())
						break;
				}
				FREELIB(hLib);
				hLib = NULL;
			}
		}
		if (hLib == NULL)
		{
			for (int f = 0; LibFuncs[f].pcFunctionName; f++)
				*LibFuncs[f].ppvFunctionAddress = LibFuncs[f].pvDummyFunctionAddress;
		}
		return hLib;
	}
	static void Free(void** hLib, const LIBFUNC* LibFuncs)
	{
		for (int f = 0; LibFuncs[f].pcFunctionName; f++)
			*LibFuncs[f].ppvFunctionAddress = LibFuncs[f].pvDummyFunctionAddress;
		if (*hLib != NULL)
		{
			FREELIB(*hLib);
			*hLib = NULL;
		}
	}
};

#endif
