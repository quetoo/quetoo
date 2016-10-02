/*
 * Copyright(c) 1997-2001 id Software, Inc.
 * Copyright(c) 2002 The Quakeforge Project.
 * Copyright(c) 2006 Quetoo.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "sys.h"
#include "filesystem.h"

#include <signal.h>

#if !defined(_MSC_VER)
#include <sys/time.h>
#else
#include <DbgHelp.h>
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#include <shlobj.h>
#define RTLD_NOW 0
#define dlopen(file_name, mode) LoadLibrary(file_name)

const char *dlerror()
{
	static char num_buffer[32];
	const DWORD err = GetLastError();

	itoa(err, num_buffer, 10);
	return va("Error loading library: %lu", err);
}

#define dlsym(handle, symbol) GetProcAddress(handle, symbol)
#define dlclose(handle) FreeLibrary(handle)

#else
#include <dlfcn.h>
#endif

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

#if HAVE_EXECINFO
#include <execinfo.h>
#define MAX_BACKTRACE_SYMBOLS 50
#endif

/**
 * @return The current executable path (argv[0]).
 */
const char *Sys_ExecutablePath(void) {
	static char path[MAX_OS_PATH];

#if defined(__APPLE__)
	uint32_t i = sizeof(path);

	if (_NSGetExecutablePath(path, &i) > -1) {
		return path;
	}

#elif defined(__linux__)

	if (readlink(va("/proc/%d/exe", getpid()), path, sizeof(path)) > -1) {
		return path;
	}

#elif defined(_WIN32) || defined(__CYGWIN__)

	if (GetModuleFileName(0, path, sizeof(path))) {
		return path;
	}

#endif

	Com_Warn("Failed to resolve executable path\n");
	return NULL;
}

/**
 * @return The current user's name.
 */
const char *Sys_Username(void) {
	return g_get_user_name();
}

/**
 * @brief Returns the current user's Quetoo directory.
 *
 * @remarks On Windows, this is `\My Documents\My Games\Quetoo`. On POSIX
 * platforms, it's `~/.quetoo`.
 */
const char *Sys_UserDir(void) {
	static char user_dir[MAX_OS_PATH];

#if defined(_WIN32)
	const char *my_documents = g_get_user_special_dir(G_USER_DIRECTORY_DOCUMENTS);
	g_snprintf(user_dir, sizeof(user_dir), "%s\\My Games\\Quetoo", my_documents);
#else
	const char *home = g_get_home_dir();
	g_snprintf(user_dir, sizeof(user_dir), "%s/.quetoo", home);
#endif

	return user_dir;
}

/**
 * @brief
 */
void Sys_OpenLibrary(const char *name, void **handle) {
	*handle = NULL;

#if defined(_WIN32) || defined(__CYGWIN__)
	char *so_name = va("%s.dll", name);
#else
	char *so_name = va("%s.so", name);
#endif

	if (Fs_Exists(so_name)) {
		char path[MAX_OS_PATH];

		g_snprintf(path, sizeof(path), "%s%c%s", Fs_RealDir(so_name), G_DIR_SEPARATOR, so_name);
		Com_Print("Trying %s...\n", path);

		if ((*handle = dlopen(path, RTLD_NOW)))
			return;

		Com_Error(ERR_DROP, "%s\n", dlerror());
	}

	Com_Error(ERR_DROP, "Couldn't find %s\n", so_name);
}

/**
 * @brief Closes an open game module.
 */
void Sys_CloseLibrary(void **handle) {
	if (*handle)
		dlclose(*handle);
	*handle = NULL;
}

/**
 * @brief Opens and loads the specified shared library. The function identified by
 * entry_point is resolved and invoked with the specified parameters, its
 * return value returned by this function.
 */
void *Sys_LoadLibrary(const char *name, void **handle, const char *entry_point, void *params) {
	typedef void *EntryPointFunc(void *);
	EntryPointFunc *EntryPoint;

	if (*handle) {
		Com_Warn("%s: handle already open\n", name);
		Sys_CloseLibrary(handle);
	}

	Sys_OpenLibrary(name, handle);

	EntryPoint = (EntryPointFunc *) dlsym(*handle, entry_point);

	if (!EntryPoint) {
		Sys_CloseLibrary(handle);
		Com_Error(ERR_DROP, "%s: Failed to resolve %s\n", name, entry_point);
	}

	return EntryPoint(params);
}

/**
 * @brief On platforms supporting it, print a backtrace.
 */
void Sys_Backtrace(void) {
#if HAVE_EXECINFO
	void *symbols[MAX_BACKTRACE_SYMBOLS];
	int32_t i;

	i = backtrace(symbols, MAX_BACKTRACE_SYMBOLS);
	backtrace_symbols_fd(symbols, i, STDERR_FILENO);

	fflush(stderr);
#elif defined(_MSC_VER)
	void *symbols[MAXSHORT];
    int32_t i;
    WORD frames;
    SYMBOL_INFO *symbol;
    HANDLE process = GetCurrentProcess();
	
	SymSetOptions(SYMOPT_UNDNAME);
	
	if (SymInitialize(process, NULL, TRUE))
	{
		frames = CaptureStackBackTrace(0, MAXSHORT, symbols, NULL);
    
		symbol = Mem_Malloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char));
		symbol->MaxNameLen = 255;
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

		for (i = 0; i < frames; i++)
		{
			const char *symbol_name = "unknown symbol";
        
			SymFromAddr(process, (DWORD64)(symbols[i]), 0, symbol);

			if (symbol->NameLen)
				symbol_name = symbol->Name;
			if (symbol->ModBase)
			{
				IMAGEHLP_MODULE module;
				module.SizeOfStruct = sizeof(module);
				SymGetModuleInfo(process, (DWORD)symbol->ModBase, &module);
				fprintf(stderr, "%s ", module.ImageName);
			}
			else
				fprintf(stderr, "unknown module ");
		
			fprintf(stderr, "(%s+%lux) [0x%" PRIx64 "]\n", symbol_name, symbol->Register, symbol->Address);
		}

		fflush(stderr);
		Mem_Free(symbol);
	}
	else
	{
		fprintf(stderr, "Couldn't get stack trace.\n");
		fflush(stderr);
	}
#endif
}

/**
 * @brief Catch kernel interrupts and dispatch the appropriate exit routine.
 */
void Sys_Signal(int32_t s) {

	switch (s) {
		case SIGINT:
		case SIGTERM:
#ifndef _WIN32
		case SIGHUP:
		case SIGQUIT:
#endif
			Com_Shutdown("Received signal %d, quitting...\n", s);
			break;
		default:
			Com_Error(ERR_FATAL, "Received signal %d\n", s);
			break;
	}
}
