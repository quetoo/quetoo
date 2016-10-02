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

#include "quemap.h"

#include <SDL2/SDL.h>

quetoo_t quetoo;

char map_name[MAX_OS_PATH];
char bsp_name[MAX_OS_PATH];
char outbase[MAX_OS_PATH];

_Bool verbose = false;
_Bool debug = false;
_Bool legacy = false;
_Bool is_monitor = false;


/* BSP */
extern _Bool noprune;
extern _Bool nodetail;
extern _Bool fulldetail;
extern _Bool onlyents;
extern _Bool nomerge;
extern _Bool nowater;
extern _Bool nofill;
extern _Bool nocsg;
extern _Bool noweld;
extern _Bool noshare;
extern _Bool nosubdivide;
extern _Bool notjunc;
extern _Bool noopt;
extern _Bool leaktest;
extern _Bool verboseentities;

extern int32_t block_xl, block_xh, block_yl, block_yh;
extern vec_t microvolume;
extern int32_t subdivide_size;

/* VIS */
extern _Bool fastvis;
extern _Bool nosort;

/* LIGHT */
extern _Bool extra_samples;
extern vec_t brightness;
extern vec_t saturation;
extern vec_t contrast;
extern vec_t surface_scale;
extern vec_t entity_scale;

static void Print(const char *msg);

/**
 * @brief
 */
static void Debug(const char *msg) {

	if (!debug)
		return;

	Print(msg);
}

static void Shutdown(const char *msg);

/**
 * @brief
 */
static void Error(err_t err, const char *msg) __attribute__((noreturn));
static void Error(err_t err __attribute__((unused)), const char *msg) {

	fprintf(stderr, "************ ERROR ************\n");
	fprintf(stderr, "%s", msg);

	fflush(stderr);

	Shutdown(msg);

	exit(err);
}

/**
 * @brief Print to stdout and, if not escaped, to the monitor socket.
 */
static void Print(const char *msg) {

	if (msg) {
		if (*msg == '@') {
			fputs(msg + 1, stdout);
		} else {
			fputs(msg, stdout);
			Mon_SendMessage(ERR_PRINT, msg);
		}

		fflush(stdout);
	}
}

/**
 * @brief Print a verbose message to stdout and, unless escaped, to the monitor
 * socket.
 */
static void Verbose(const char *msg) {

	if (!verbose)
		return;

	Print(msg);
}

/**
 * @brief Print a warning message to stdout and, if not escaped, to the monitor
 * socket.
 */
static void Warn(const char *msg) {

	if (msg) {
		if (*msg == '@') {
			fprintf(stderr, "WARNING: %s", msg + 1);
		} else {
			fprintf(stderr, "WARNING: %s", msg);
			Mon_SendMessage(ERR_WARN, va("WARNING: %s", msg));
		}

		fflush(stderr);
	}
}

/**
 * @brief Initializes subsystems quemap relies on.
 */
static void Init(void) {

#if defined(_WIN32)
	if (AllocConsole()) {
		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stdout);
		freopen("CONERR$", "w", stderr);
	} else {
		Com_Error(ERR_FATAL, "Failed to allocate console: %u\n", (uint32_t) GetLastError());
	}
#endif

	SDL_Init(SDL_INIT_TIMER);

	Mem_Init();

	Fs_Init(true);

	Sem_Init();

	Com_Print("Quetoo Map %s %s %s initialized\n", VERSION, __DATE__, BUILD_HOST);
}

/**
 * @brief Shuts down subsystems.
 */
static void Shutdown(const char *msg) {

	Mon_Shutdown(msg);

	Sem_Shutdown();

	Thread_Shutdown();

	Fs_Shutdown();

	Mem_Shutdown();

	SDL_Quit();

#if defined(_WIN32)
	if (!is_monitor) {
		puts("\nPress any key to close..\n");
		getchar();
	}

	FreeConsole();
#endif
}

/**
 * @brief
 */
static void Check_BSP_Options(int32_t argc) {

	for (int32_t i = argc; i < Com_Argc(); i++) {
		if (!g_strcmp0(Com_Argv(i), "-noweld")) {
			Com_Verbose("noweld = true\n");
			noweld = true;
		} else if (!g_strcmp0(Com_Argv(i), "-nocsg")) {
			Com_Verbose("nocsg = true\n");
			nocsg = true;
		} else if (!g_strcmp0(Com_Argv(i), "-noshare")) {
			Com_Verbose("noshare = true\n");
			noshare = true;
		} else if (!g_strcmp0(Com_Argv(i), "-notjunc")) {
			Com_Verbose("notjunc = true\n");
			notjunc = true;
		} else if (!g_strcmp0(Com_Argv(i), "-nowater")) {
			Com_Verbose("nowater = true\n");
			nowater = true;
		} else if (!g_strcmp0(Com_Argv(i), "-noopt")) {
			Com_Verbose("noopt = true\n");
			noopt = true;
		} else if (!g_strcmp0(Com_Argv(i), "-noprune")) {
			Com_Verbose("noprune = true\n");
			noprune = true;
		} else if (!g_strcmp0(Com_Argv(i), "-nofill")) {
			Com_Verbose("nofill = true\n");
			nofill = true;
		} else if (!g_strcmp0(Com_Argv(i), "-nomerge")) {
			Com_Verbose("nomerge = true\n");
			nomerge = true;
		} else if (!g_strcmp0(Com_Argv(i), "-nosubdivide")) {
			Com_Verbose("nosubdivide = true\n");
			nosubdivide = true;
		} else if (!g_strcmp0(Com_Argv(i), "-nodetail")) {
			Com_Verbose("nodetail = true\n");
			nodetail = true;
		} else if (!g_strcmp0(Com_Argv(i), "-fulldetail")) {
			Com_Verbose("fulldetail = true\n");
			fulldetail = true;
		} else if (!g_strcmp0(Com_Argv(i), "-onlyents")) {
			Com_Verbose("onlyents = true\n");
			onlyents = true;
		} else if (!g_strcmp0(Com_Argv(i), "-micro")) {
			microvolume = atof(Com_Argv(i + 1));
			Com_Verbose("microvolume = %f\n", microvolume);
			i++;
		} else if (!g_strcmp0(Com_Argv(i), "-leaktest")) {
			Com_Verbose("leaktest = true\n");
			leaktest = true;
		} else if (!g_strcmp0(Com_Argv(i), "-verboseentities")) {
			Com_Verbose("verboseentities = true\n");
			verboseentities = true;
		} else if (!g_strcmp0(Com_Argv(i), "-subdivide")) {
			subdivide_size = atoi(Com_Argv(i + 1));
			Com_Verbose("subdivide_size = %d\n", subdivide_size);
			i++;
		} else if (!g_strcmp0(Com_Argv(i), "-block")) {
			block_xl = block_xh = atoi(Com_Argv(i + 1));
			block_yl = block_yh = atoi(Com_Argv(i + 2));
			Com_Verbose("block: %i,%i\n", block_xl, block_yl);
			i += 2;
		} else if (!g_strcmp0(Com_Argv(i), "-blocks")) {
			block_xl = atoi(Com_Argv(i + 1));
			block_yl = atoi(Com_Argv(i + 2));
			block_xh = atoi(Com_Argv(i + 3));
			block_yh = atoi(Com_Argv(i + 4));
			Com_Verbose("blocks: %i,%i to %i,%i\n", block_xl, block_yl, block_xh, block_yh);
			i += 4;
		} else if (!g_strcmp0(Com_Argv(i), "-tmpout")) {
			strcpy(outbase, "/tmp");
		} else
			break;
	}
}

/**
 * @brief
 */
static void Check_VIS_Options(int32_t argc) {

	for (int32_t i = argc; i < Com_Argc(); i++) {
		if (!g_strcmp0(Com_Argv(i), "-fast")) {
			Com_Verbose("fastvis = true\n");
			fastvis = true;
		} else if (!g_strcmp0(Com_Argv(i), "-nosort")) {
			Com_Verbose("nosort = true\n");
			nosort = true;
		} else
			break;
	}
}

/**
 * @brief
 */
static void Check_LIGHT_Options(int32_t argc) {

	for (int32_t i = argc; i < Com_Argc(); i++) {
		if (!g_strcmp0(Com_Argv(i), "-extra")) {
			extra_samples = true;
			Com_Verbose("extra samples = true\n");
		} else if (!g_strcmp0(Com_Argv(i), "-brightness")) {
			brightness = atof(Com_Argv(i + 1));
			Com_Verbose("brightness at %f\n", brightness);
			i++;
		} else if (!g_strcmp0(Com_Argv(i), "-saturation")) {
			saturation = atof(Com_Argv(i + 1));
			Com_Verbose("saturation at %f\n", saturation);
			i++;
		} else if (!g_strcmp0(Com_Argv(i), "-contrast")) {
			contrast = atof(Com_Argv(i + 1));
			Com_Verbose("contrast at %f\n", contrast);
			i++;
		} else if (!g_strcmp0(Com_Argv(i), "-surface")) {
			surface_scale *= atof(Com_Argv(i + 1));
			Com_Verbose("surface light scale at %f\n", surface_scale);
			i++;
		} else if (!g_strcmp0(Com_Argv(i), "-entity")) {
			entity_scale *= atof(Com_Argv(i + 1));
			Com_Verbose("entity light scale at %f\n", entity_scale);
			i++;
		} else
			break;
	}
}

/**
 * @brief
 */
static void Check_AAS_Options(int32_t argc __attribute__((unused))) {
}

/**
 * @brief
 */
static void Check_ZIP_Options(int32_t argc __attribute__((unused))) {
}

/**
 * @brief
 */
static void Check_MAT_Options(int32_t argc __attribute__((unused))) {
}

/**
 * @brief
 */
static void PrintHelpMessage(void) {
	Com_Print("General options\n");
	Com_Print("-v -verbose\n");
	Com_Print("-l -legacy - compile a legacy Quake II map\n");
	Com_Print("-d -debug\n");
	Com_Print("-t -threads <int>\n");
	Com_Print("-p -path <game directory> - add the path to the search directory\n");
	Com_Print("-w -wpath <game directory> - add the write path to the search directory\n");
	Com_Print("-c -connect <host> - use GtkRadiant's BSP monitoring server\n");

	Com_Print("\n");
	Com_Print("-bsp               BSP stage options:\n");
	Com_Print(" -block <int> <int>\n");
	Com_Print(" -blocks <int> <int> <int> <int>\n");
	Com_Print(" -fulldetail - don't treat details (and trans surfaces) as details\n");
	Com_Print(" -leaktest\n");
	Com_Print(" -micro <float>\n");
	Com_Print(" -nocsg\n");
	Com_Print(" -nodetail - skip detail brushes\n");
	Com_Print(" -nofill\n");
	Com_Print(" -nomerge - skip node face merging\n");
	Com_Print(" -noopt\n");
	Com_Print(" -noprune - don't prune (or cut) nodes\n");
	Com_Print(" -noshare\n");
	Com_Print(" -nosubdivide\n");
	Com_Print(" -notjunc\n");
	Com_Print(" -nowater - skip water brushes in compilation\n");
	Com_Print(" -noweld\n");
	Com_Print(" -onlyents - modify existing bsp file with entities from map file\n");
	Com_Print(" -subdivide <int> - subdivide brushes for better light effects\n");
	Com_Print(" -tmpout\n");
	Com_Print(" -verboseentities - also be verbose about submodels (entities)\n");
	Com_Print("\n");
	Com_Print("-vis               VIS stage options:\n");
	Com_Print(" -fast\n");
	Com_Print(" -nosort\n");
	Com_Print("\n");
	Com_Print("-light             LIGHT stage options:\n");
	Com_Print(" -extra - extra light samples\n");
	Com_Print(" -entity <float> - entity light scaling\n");
	Com_Print(" -surface <float> - surface light scaling\n");
	Com_Print(" -brightness <float> - brightness factor\n");
	Com_Print(" -contrast <float> - contrast factor\n");
	Com_Print(" -saturation <float> - saturation factor\n");
	Com_Print("\n");
	Com_Print("-aas               AAS stage options:\n");
	Com_Print("\n");
	Com_Print("-mat               MAT stage options:\n");
	Com_Print("\n");
	Com_Print("-zip               ZIP stage options:\n");
	Com_Print("\n");
	Com_Print("Examples:\n");
	Com_Print("Standard full compile:\n quemap -bsp -vis -light maps/my.map\n");
	Com_Print("Fast vis, extra light, two threads:\n"
		" quemap -t 2 -bsp -vis -fast -light -extra maps/my.map\n");
	Com_Print("Area awareness compile (for bots):\n quemap -aas maps/my.bsp\n");
	Com_Print("Materials file generation:\n quemap -mat maps/my.map\n");
	Com_Print("Zip file generation:\n quemap -zip maps/my.bsp\n");
	Com_Print("\n");
}

/**
 * @brief
 */
int32_t main(int32_t argc, char **argv) {
	int32_t i;
	_Bool do_bsp = false;
	_Bool do_vis = false;
	_Bool do_light = false;
	_Bool do_aas = false;
	_Bool do_mat = false;
	_Bool do_zip = false;

	printf("Quetoo Map %s %s %s\n", VERSION, __DATE__, BUILD_HOST);

	memset(&quetoo, 0, sizeof(quetoo));

	quetoo.Debug = Debug;
	quetoo.Error = Error;
	quetoo.Print = Print;
	quetoo.Verbose = Verbose;
	quetoo.Warn = Warn;

	quetoo.Init = Init;
	quetoo.Shutdown = Shutdown;

	signal(SIGINT, Sys_Signal);
	signal(SIGILL, Sys_Signal);
	signal(SIGABRT, Sys_Signal);
	signal(SIGFPE, Sys_Signal);
	signal(SIGSEGV, Sys_Signal);
	signal(SIGTERM, Sys_Signal);
#ifndef _WIN32
	signal(SIGHUP, Sys_Signal);
	signal(SIGQUIT, Sys_Signal);
#endif

	Com_Init(argc, argv);

	// general options
	for (i = 1; i < Com_Argc(); i++) {

		if (!g_strcmp0(Com_Argv(i), "-h") || !g_strcmp0(Com_Argv(i), "-help")) {
			PrintHelpMessage();
			Com_Shutdown(NULL);
		}

		if (!g_strcmp0(Com_Argv(i), "-v") || !g_strcmp0(Com_Argv(i), "-verbose")) {
			verbose = true;
			continue;
		}

		if (!g_strcmp0(Com_Argv(i), "-d") || !g_strcmp0(Com_Argv(i), "-debug")) {
			debug = true;
			continue;
		}

		if (!g_strcmp0(Com_Argv(i), "-l") || !g_strcmp0(Com_Argv(i), "-legacy")) {
			legacy = true;
			continue;
		}

		if (!g_strcmp0(Com_Argv(i), "-t") || !g_strcmp0(Com_Argv(i), "-threads")) {
			Thread_Init(atoi(Com_Argv(i + 1)));
			Com_Print("Using %u threads\n", Thread_Count());
			continue;
		}

		if (!g_strcmp0(Com_Argv(i), "-c") || !g_strcmp0(Com_Argv(i), "-connect")) {
			is_monitor = Mon_Init(Com_Argv(i + 1));
			continue;
		}
	}

	// read compiling options
	for (i = 1; i < Com_Argc(); i++) {

		if (!g_strcmp0(Com_Argv(i), "-bsp")) {
			do_bsp = true;
			Check_BSP_Options(i + 1);
		}

		if (!g_strcmp0(Com_Argv(i), "-vis")) {
			do_vis = true;
			Check_VIS_Options(i + 1);
		}

		if (!g_strcmp0(Com_Argv(i), "-light")) {
			do_light = true;
			Check_LIGHT_Options(i + 1);
		}

		if (!g_strcmp0(Com_Argv(i), "-aas")) {
			do_aas = true;
			Check_AAS_Options(i + 1);
		}

		if (!g_strcmp0(Com_Argv(i), "-mat")) {
			do_mat = true;
			Check_MAT_Options(i + 1);
		}

		if (!g_strcmp0(Com_Argv(i), "-zip")) {
			do_zip = true;
			Check_ZIP_Options(i + 1);
		}
	}

	if (!do_bsp && !do_vis && !do_light && !do_aas && !do_mat && !do_zip) {
		Com_Error(ERR_FATAL, "No action specified. Try %s -help\n", Com_Argv(0));
	}

	// ugly little hack to localize global paths to game paths
	// for e.g. GtkRadiant
	const char *c = strstr(Com_Argv(Com_Argc() - 1), "/maps/");
	c = c ? c + 1 : Com_Argv(Com_Argc() - 1);

	StripExtension(c, map_name);
	g_strlcpy(bsp_name, map_name, sizeof(bsp_name));
	g_strlcat(map_name, ".map", sizeof(map_name));
	g_strlcat(bsp_name, ".bsp", sizeof(bsp_name));

	// start timer
	const time_t start = time(NULL);

	if (do_bsp)
		BSP_Main();
	if (do_vis)
		VIS_Main();
	if (do_light)
		LIGHT_Main();
	if (do_aas)
		AAS_Main();
	if (do_mat)
		MAT_Main();
	if (do_zip)
		ZIP_Main();

	// emit time
	const time_t end = time(NULL);
	const time_t duration = end - start;
	Com_Print("\nTotal Time: ");
	if (duration > 59)
		Com_Print("%d Minutes ", (int32_t) (duration / 60));
	Com_Print("%d Seconds\n", (int32_t) (duration % 60));

	Com_Shutdown(NULL);

	return 0;
}
