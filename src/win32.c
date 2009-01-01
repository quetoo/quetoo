/*
 * Copyright(c) 1997-2001 Id Software, Inc.
 * Copyright(c) 2002 The Quakeforge Project.
 * Copyright(c) 2006 Quake2World.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or(at your option) any later version.
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

#ifdef _WIN32

#include "win32.h"

// wrap dlfcn calls
void *dlopen(const char *filename, int flag){
	return LoadLibrary(filename);
}

char *dlerror(void){
	return "";  // FIXME
}

void *dlsym(void *handle, const char *symbol){
	return GetProcAddress(handle, symbol);
}

void dlclose(void *handle){
	FreeLibrary(handle);
}

// wrap ioctl for sockets
int ioctl(int sockfd, int flags, void *null){
	return ioctlsocket(sockfd, flags, null);
}

#endif /* _WIN32 */
