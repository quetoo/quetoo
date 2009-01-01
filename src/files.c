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
#include <windows.h>

#define CSIDL_APPDATA  0x001a
#define CSIDL_PERSONAL 0x0005

#endif /* ifdef _WIN32 */

#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>

#include "common.h"
#include "pak.h"

/*
 * Fs_Write
 */
size_t Fs_Write(void *ptr, size_t size, size_t nmemb, FILE *stream){
	size_t len;

	if(size * nmemb == 0)
		return 0;

	len = fwrite(ptr, size, nmemb, stream);

	if(len <= 0)
		Com_Warn("Fs_Write: Failed to write\n");

	return len;
}

/*
 * Fs_Read
 */
size_t Fs_Read(void *ptr, size_t size, size_t nmemb, FILE *stream){
	size_t len;

	if(size * nmemb == 0)
		return 0;

	len = fread(ptr, size, nmemb, stream);

	if(len <= 0)  // read failed, exit
		Com_Warn("Fs_Read: Failed to read file.");

	return len;
}

/*
 * Fs_CloseFile
 */
void Fs_CloseFile(FILE *f){
	fclose(f);
}

#ifndef __LIBPAK_LA__

static char fs_gamedir[MAX_OSPATH];

static cvar_t *fs_basedir;
cvar_t *fs_gamedirvar;

typedef struct searchpath_s {
	char path[MAX_OSPATH];
	struct searchpath_s *next;
} searchpath_t;

static searchpath_t *fs_searchpaths;
static searchpath_t *fs_base_searchpaths;  // without gamedirs

static hashtable_t fs_hashtable;  // pakfiles are pushed into a hash

/*
 * Fs_FileLength
 */
static int Fs_FileLength(FILE *f){
	int pos;
	int end;

	pos = ftell(f);
	fseek(f, 0, SEEK_END);
	end = ftell(f);
	fseek(f, pos, SEEK_SET);

	return end;
}


/*
 * Fs_CreatePath
 * 
 * Creates any directories needed to store the given path.
 */
void Fs_CreatePath(const char *path){
	char pathCopy[MAX_OSPATH];
	char *ofs;

	strncpy(pathCopy, path, sizeof(pathCopy) - 1);

	for(ofs = pathCopy + 1; *ofs; ofs++){
		if(*ofs == '/'){  // create the directory
			*ofs = 0;
			Sys_Mkdir(pathCopy);
			*ofs = '/';
		}
	}
}

/*
 * Fs_OpenFile
 * 
 * Attempts to open the specified file on the search path.  Returns filesize
 * and an open FILE pointer.  This generalizes opening files from paks vs
 * opening filesystem resources directly.
 */
char *last_pak;  // the server uses this to determine CS_PAK
int Fs_OpenFile(const char *filename, FILE **file, filemode_t mode){
	searchpath_t *search;
	char path[MAX_OSPATH];
	struct stat sbuf;
	pak_t *pak;
	pakentry_t *e;

	// open for write or append in gamedir and return
	if(mode == FILE_WRITE || mode == FILE_APPEND){

		snprintf(path, sizeof(path), "%s/%s", Fs_Gamedir(), filename);
		Fs_CreatePath(path);

		*file = fopen(path, (mode == FILE_WRITE ? "wb" : "ab"));
		return *file ? 0 : -1;
	}

	// try the searchpaths
	for(search = fs_searchpaths; search; search = search->next){

		snprintf(path, sizeof(path), "%s/%s", search->path, filename);

		if(stat(path, &sbuf) == -1 || !S_ISREG(sbuf.st_mode))
			continue;

		if((*file = fopen(path, "rb")) == NULL)
			continue;

		return Fs_FileLength(*file);
	}

	// fall back on the pakfiles
	if((pak = (pak_t *)Com_HashValue(&fs_hashtable, filename))){

		// find the entry within the pakfile
		if((e = (pakentry_t *)Com_HashValue(&pak->hashtable, filename))){

			*file = fopen(pak->filename, "rb");

			if(!*file){
				Com_Warn("Fs_OpenFile: couldn't reopen %s.\n", pak->filename);
				return -1;
			}

			fseek(*file, e->fileofs, SEEK_SET);

			last_pak = strrchr(pak->filename, '/') + 1;
			return e->filelen;
		}
	}

	last_pak = NULL;


	if(Com_Mixedcase(filename)) {  // try lowercase version
		char lower[MAX_QPATH];
		strncpy(lower, filename, sizeof(lower) - 1);
		return Fs_OpenFile(Com_Lowercase(lower), file, mode);
	}

	//Com_Dprintf("Fs_OpenFile: can't find %s.\n", filename);

	*file = NULL;
	return -1;
}


/*
 * Fs_ReadFile
 * 
 * Properly handles partial reads
 */
void Fs_ReadFile(void *buffer, int len, FILE *f){
	int read;

	read = Fs_Read(buffer, 1, len, f);
	if(read != len){  // read failed, exit
		Com_Error(ERR_DROP, "Fs_ReadFile: %d bytes read.", read);
	}
}


/*
 * Fs_LoadFile
 * 
 * Filename are reletive to the quake search path
 * A NULL buffer will just return the file length without loading.
 */
int Fs_LoadFile(const char *filename, void **buffer){
	FILE *h;
	byte *buf;
	int len;

	buf = NULL;  // quiet compiler warning

	// look for it in the filesystem or pak files
	len = Fs_OpenFile(filename, &h, FILE_READ);
	if(!h){
		if(buffer)
			*buffer = NULL;
		return -1;
	}

	if(!buffer){
		Fs_CloseFile(h);
		return len;
	}

	// allocate, and string-terminate it in case it's text
	buf = Z_Malloc(len + 1);
	buf[len] = 0;

	*buffer = buf;

	Fs_ReadFile(buf, len, h);

	Fs_CloseFile(h);

	return len;
}


/*
 * Fs_FreeFile
 */
void Fs_FreeFile(void *buffer){
	Z_Free(buffer);
}


/*
 * Fs_AddPakfile
 */
void Fs_AddPakfile(const char *pakfile){
	pak_t *pak;
	int i;

	if(!(pak = Pak_ReadPakfile(pakfile)))
		return;

	for(i = 0; i < pak->numentries; i++)  // hash the entries to the pak
		Com_HashInsert(&fs_hashtable, pak->entries[i].name, pak);

	Com_Printf("Added %s: %i files.\n", pakfile, pak->numentries);
}


/*
 * Fs_AddGameDirectory
 * 
 * Adds the directory to the head of the path, and loads all paks within it.
 */
static void Fs_AddGameDirectory(const char *dir){
	searchpath_t *search;
	const char *p;

	// don't add the same searchpath twice
	search = fs_searchpaths;
	while(search){
		if(!strcmp(search->path, dir))
			return;
		search = search->next;
	}

	// add the base directory to the search path
	search = Z_Malloc(sizeof(searchpath_t));
	strncpy(search->path, dir, sizeof(search->path) - 1);
	search->path[sizeof(search->path) - 1] = 0;

	search->next = fs_searchpaths;
	fs_searchpaths = search;

	p = Sys_FindFirst(va("%s/*.pak", dir));
	while(p){
		Fs_AddPakfile(p);
		p = Sys_FindNext();
	}

	Sys_FindClose();
}


static char homedir[MAX_OSPATH];

/*
 * Fs_Homedir
 */
static char *Fs_Homedir(void){
#ifdef _WIN32
	void *handle;
	FARPROC GetFolderPath;

	memset(homedir, 0, sizeof(homedir));

	if((handle = dlopen("shfolder.dll", 0))){
		if((GetFolderPath = dlsym(handle, "SHGetFolderPathA")))
			GetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, homedir);
		dlclose(handle);
	}

	if(strlen(homedir))  // append our directory name
		strcat(homedir, "\\My Games\\Quake2World");
	else  // or simply use ./
		strcat(homedir, PKGDATADIR);
#else
	memset(homedir, 0, sizeof(homedir));

	strncpy(homedir, getenv("HOME"), sizeof(homedir));
	strcat(homedir, "/.quake2world");
#endif
	return homedir;
}


/*
 * Fs_AddHomeAsGameDirectory
 */
static void Fs_AddHomeAsGameDirectory(const char *dir){
	char gdir[MAX_OSPATH];

	memset(homedir, 0, sizeof(homedir));
	snprintf(gdir, sizeof(gdir), "%s/%s", Fs_Homedir(), dir);

	Com_Printf("Using %s for writing.\n", gdir);

	Fs_CreatePath(va("%s/", gdir));

	strncpy(fs_gamedir, gdir, sizeof(fs_gamedir) - 1);
	fs_gamedir[sizeof(fs_gamedir) - 1] = 0;

	Fs_AddGameDirectory(gdir);
}


/*
 * Fs_Gamedir
 * 
 * Called to find where to write a file (demos, screenshots, etc)
 */
const char *Fs_Gamedir(void){
	return fs_gamedir;
}


/*
 * Fs_FindFirst
 */
const char *Fs_FindFirst(const char *path, qboolean fullpath){
	const char *n;
	char name[MAX_OSPATH];
	const searchpath_t *s;

	// search through all the paths for path
	for(s = fs_searchpaths; s != NULL; s = s->next){

		snprintf(name, sizeof(name), "%s/%s", s->path, path);
		if((n = Sys_FindFirst(name))){
			Sys_FindClose();
			return fullpath ? n : n + strlen(s->path) + 1;
		}

		Sys_FindClose();
	}

	return NULL;
}


/*
 * Fs_ExecAutoexec
 * 
 * Execs the local autoexec.cfg for the current gamedir.  This is
 * a function call rather than simply stuffing "exec autoexec.cfg"
 * because we do not wish to use default/autoexec.cfg for all mods.
 */
void Fs_ExecAutoexec(void){
	char name[MAX_QPATH];
	searchpath_t *s, *end;

	// don't look in default if gamedir is set
	if(fs_searchpaths == fs_base_searchpaths)
		end = NULL;
	else
		end = fs_base_searchpaths;

	// search through all the paths for an autoexec.cfg file
	for(s = fs_searchpaths; s != end; s = s->next){

		snprintf(name, sizeof(name), "%s/autoexec.cfg", s->path);

		if(Sys_FindFirst(name)){
			Cbuf_AddText("exec autoexec.cfg\n");
			Sys_FindClose();
			break;
		}

		Sys_FindClose();
	}

	Cbuf_Execute();  // execute it
}


/*
 * Fs_SetGamedir
 * 
 * Sets the gamedir and path to a different directory.
 */
void Fs_SetGamedir(const char *dir){
	searchpath_t *s;
	hashentry_t *e;
	pak_t *pak;
	int i;

	if(strstr(dir, "..") || strstr(dir, "/")
			|| strstr(dir, "\\") || strstr(dir, ":")){
		Com_Printf("Gamedir should be a single directory name, not a path\n");
		return;
	}

	// free up any current game dir info
	while(fs_searchpaths != fs_base_searchpaths){

		// free paks not living in base searchpaths
		for(i = 0; i < HASHBINS; i++){
			e = fs_hashtable.bins[i];
			while(e){
				pak = (pak_t*)e->value;

				if(!strstr(pak->filename, fs_searchpaths->path))
					continue;

				Com_HashRemoveEntry(&fs_hashtable, e);
				Pak_FreePakfile(pak);
			}
		}
		s = fs_searchpaths->next;
		Z_Free(fs_searchpaths);
		fs_searchpaths = s;
	}

	// now add new entries for
	if(!strcmp(dir, BASEDIRNAME) || (*dir == 0)){
		Cvar_FullSet("gamedir", "", CVAR_SERVERINFO | CVAR_NOSET);
		Cvar_FullSet("game", "", CVAR_LATCH | CVAR_SERVERINFO);
	} else {
		Cvar_FullSet("gamedir", dir, CVAR_SERVERINFO | CVAR_NOSET);
		Fs_AddGameDirectory(va(PKGLIBDIR"/%s", dir));
		Fs_AddGameDirectory(va(PKGDATADIR"/%s", dir));
		Fs_AddHomeAsGameDirectory(dir);
	}
}


/*
 * Fs_NextPath
 * 
 * Allows enumerating all of the directories in the search path
 */
const char *Fs_NextPath(const char *prevpath){
	searchpath_t *s;
	char *prev;

	prev = NULL;  // fs_gamedir is the first directory in the searchpath
	for(s = fs_searchpaths; s; s = s->next){
		if(prevpath == NULL)
			return s->path;
		if(prevpath == prev)
			return s->path;
		prev = s->path;
	}

	return NULL;
}


#define GZIP_BUFFER (64 * 1024)

/*
 * Fs_GunzipFile
 * 
 * Deflates the specified file in place, removing the .gz suffix from the path.
 * The original deflated file is removed upon successful decompression.
 */
void Fs_GunzipFile(const char *path){
	gzFile gz;
	FILE *f;
	char *c;
	byte *buffer;
	size_t r, w;
	char p[MAX_OSPATH];

	if(!path || !path[0])
		return;

	strncpy(p, path, sizeof(p));

	if(!(c = strstr(p, ".gz"))){
		Com_Warn("Fs_GunzipFile: %s lacks .gz suffix.\n", p);
		return;
	}

	if(!(gz = gzopen(p, "rb"))){
		Com_Warn("Fs_GunzipFile: Failed to open %s.\n", p);
		return;
	}

	*c = 0;  // mute .gz extension

	if(!(f = fopen(p, "wb"))){
		Com_Warn("Fs_GunzipFile: Failed to open %s.\n", p);
		gzclose(gz);
		return;
	}

	*c = 0;
	buffer = (byte *)Z_Malloc(GZIP_BUFFER);

	while(true){
		r = gzread(gz, buffer, GZIP_BUFFER);

		if(r == 0)  // end of file
			break;

		if(r == -1){  // error in gzip stream
			Com_Warn("Fs_GunzipFile: Error in gzip stream %s.\n", path);
			break;
		}

		w = Fs_Write(buffer, 1, r, f);

		if(r != w){  // bad write
			Com_Warn("Fs_GunzipFile: Incomplete write %s.\n", path);
			break;
		}
	}

	gzclose(gz);
	Fs_CloseFile(f);

	Z_Free(buffer);

	unlink(path);
}


/*
 * Fs_InitFilesystem
 */
void Fs_InitFilesystem(void){
	char bd[MAX_OSPATH];

	fs_searchpaths = NULL;

	Com_HashInit(&fs_hashtable);

	// allow the game to run from outside the data tree
	fs_basedir = Cvar_Get("fs_basedir", "", CVAR_NOSET, NULL);

	if(fs_basedir && strlen(fs_basedir->string)){  // something was specified
		snprintf(bd, sizeof(bd), "%s/%s", fs_basedir->string, BASEDIRNAME);
		Fs_AddGameDirectory(bd);
	}

	// add default to search path
	Fs_AddGameDirectory(PKGLIBDIR"/"BASEDIRNAME);
	Fs_AddGameDirectory(PKGDATADIR"/"BASEDIRNAME);

	// then add a '.quake2world/default' directory in home directory by default
	Fs_AddHomeAsGameDirectory(BASEDIRNAME);

	// any set gamedirs will be freed up to here
	fs_base_searchpaths = fs_searchpaths;

	// check for game override
	fs_gamedirvar = Cvar_Get("game", "", CVAR_LATCH | CVAR_SERVERINFO, NULL);

	if(fs_gamedirvar && strlen(fs_gamedirvar->string))
		Fs_SetGamedir(fs_gamedirvar->string);
}

#endif // __LIBPAK_LA__
