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

#include "s_local.h"
#include "client.h"

LPALGENFILTERS alGenFilters;
LPALDELETEFILTERS alDeleteFilters;
LPALFILTERI alFilteri;
LPALFILTERF alFilterf;
LPALGENEFFECTS alGenEffects;
LPALDELETEEFFECTS alDeleteEffects;
LPALEFFECTI alEffecti;
LPALEFFECTF alEffectf;
LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots;
LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots;
LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti;
LPALAUXILIARYEFFECTSLOTF alAuxiliaryEffectSlotf;

// the sound environment
s_env_t s_env;

cvar_t *s_ambient;
cvar_t *s_doppler;
cvar_t *s_effects;
cvar_t *s_rate;
cvar_t *s_volume;

extern cl_client_t cl;
extern cl_static_t cls;

/**
 * @brief Check and report OpenAL errors.
 */
void S_CheckALError(void) {
	const ALenum v = alGetError();

	if (v == AL_NO_ERROR) {
		return;
	}

	Com_Debug(DEBUG_BREAKPOINT | DEBUG_SOUND, "AL error: %s\n", alGetString(v));
}

/**
 * @brief
 */
static sf_count_t S_RWops_get_filelen(void *user_data) {
	SDL_RWops *rwops = (SDL_RWops *) user_data;
	return rwops->size(rwops);
}

/**
 * @brief
 */
static sf_count_t S_RWops_seek(sf_count_t offset, int whence, void *user_data) {
	SDL_RWops *rwops = (SDL_RWops *) user_data;
	return rwops->seek(rwops, offset, whence);
}

/**
 * @brief
 */
static sf_count_t S_RWops_read(void *ptr, sf_count_t count, void *user_data) {
	SDL_RWops *rwops = (SDL_RWops *) user_data;
	return rwops->read(rwops, ptr, 1, count);
}

/**
 * @brief
 */
static sf_count_t S_RWops_write(const void *ptr, sf_count_t count, void *user_data) {
	SDL_RWops *rwops = (SDL_RWops *) user_data;
	return rwops->write(rwops, ptr, 1, count);
}

/**
 * @brief
 */
static sf_count_t S_RWops_tell(void *user_data) {
	SDL_RWops *rwops = (SDL_RWops *) user_data;
	return rwops->seek(rwops, 0, SEEK_CUR);
}

/**
 * @brief An interface to SDL_RWops for libsndfile
 */
SF_VIRTUAL_IO s_rwops_io = {
	.get_filelen = S_RWops_get_filelen,
	.seek = S_RWops_seek,
	.read = S_RWops_read,
	.write = S_RWops_write,
	.tell = S_RWops_tell
};

/**
 * @brief Stop sounds that are playing, if any.
 */
void S_Stop(void) {

	memset(s_env.channels, 0, sizeof(s_env.channels));

	alSourceStopv(MAX_CHANNELS, s_env.sources);

	for (size_t i = 0; i < MAX_CHANNELS; i++) {
		alSourcei(s_env.sources[i], AL_BUFFER, 0);
	}

	S_CheckALError();
}

/**
 * @brief Adds a single frame of audio. Also, if a loading cycle has completed,
 * media is freed here.
 */
void S_Frame(void) {

	if (!s_env.context) {
		return;
	}

	S_FrameMusic();

	if (cls.state != CL_ACTIVE) {
		S_Stop();
		return;
	}

	if (s_env.update) {
		s_env.update = false;
		S_FreeMedia();
	}

	S_MixChannels();
}

/**
 * @brief Loads all media for the sound subsystem.
 */
void S_LoadMedia(void) {
	extern cl_client_t cl;

	if (!s_env.context) {
		return;
	}

	if (!cl.config_strings[CS_MODELS][0]) {
		return; // no map specified
	}

	S_ClearPlaylist();

	S_BeginLoading();

	Cl_LoadingProgress(80, "sounds");

	for (uint32_t i = 0; i < MAX_SOUNDS; i++) {

		if (!cl.config_strings[CS_SOUNDS + i][0]) {
			break;
		}

		cl.sound_precache[i] = S_LoadSample(cl.config_strings[CS_SOUNDS + i]);
	}

	for (uint32_t i = 0; i < MAX_MUSICS; i++) {

		if (!cl.config_strings[CS_MUSICS + i][0]) {
			break;
		}

		cl.music_precache[i] = S_LoadMusic(cl.config_strings[CS_MUSICS + i]);
	}

	S_NextTrack_f();

	Cl_LoadingProgress(85, "music");

	s_env.update = true;
}

/**
 * @brief
 */
static void S_Play_f(void) {

	int32_t i = 1;
	while (i < Cmd_Argc()) {
		S_AddSample(&(const s_play_sample_t) {
			.sample = S_LoadSample(Cmd_Argv(i++))
		});
	}
}

/**
 * @brief
 */
static void S_Stop_f(void) {
	S_Stop();
}

/**
 * @brief
 */
void S_Restart_f(void) {

	if (cls.state == CL_LOADING) {
		return;
	}

	S_Shutdown();

	S_Init();

	const cl_state_t state = cls.state;

	if (cls.state == CL_ACTIVE) {
		cls.state = CL_LOADING;
	}

	S_LoadMedia();

	cls.state = state;
}

/**
 * @brief Initializes variables and commands for the sound subsystem.
 */
static void S_InitLocal(void) {

	s_ambient = Cvar_Add("s_ambient", "1", CVAR_ARCHIVE, "Controls playback of ambient sounds.");
	s_doppler = Cvar_Add("s_doppler", "0", CVAR_ARCHIVE, "The scale for the doppler effect. 0 is disabled, 1 is default, anything inbetween is scale.");
	s_effects = Cvar_Add("s_effects", "0", CVAR_ARCHIVE | CVAR_S_MEDIA, "Whether sound filtering is enabled for systems that support it.");
	s_rate = Cvar_Add("s_rate", "44100", CVAR_ARCHIVE | CVAR_S_DEVICE, "Sound sample rate in Hz.");
	s_volume = Cvar_Add("s_volume", "1.0", CVAR_ARCHIVE, "Global sound volume level.");

	Cvar_ClearAll(CVAR_S_MASK);

	Cmd_Add("s_list_media", S_ListMedia_f, CMD_SOUND, "List all currently loaded media");
	Cmd_Add("s_play", S_Play_f, CMD_SOUND, NULL);
	Cmd_Add("s_restart", S_Restart_f, CMD_SOUND, "Restart the sound subsystem");
	Cmd_Add("s_stop", S_Stop_f, CMD_SOUND, NULL);
}

/**
 * @brief Initializes the sound subsystem.
 */
void S_Init(void) {
	memset(&s_env, 0, sizeof(s_env));

	if (Cvar_GetValue("s_disable")) {
		Com_Warn("Sound disabled\n");
		return;
	}

	Com_Print("Sound initialization...\n");

	S_InitLocal();

	s_env.device = alcOpenDevice(NULL);

	if (!s_env.device) {
		Com_Warn("%s\n", alcGetString(NULL, alcGetError(NULL)));
		return;
	}

	s_env.context = alcCreateContext(s_env.device, NULL);

	if (!s_env.context || !alcMakeContextCurrent(s_env.context)) {
		Com_Warn("%s\n", alcGetString(NULL, alcGetError(NULL)));
		return;
	}

	if (s_effects->integer) {
		alGenFilters = (LPALGENFILTERS)alGetProcAddress("alGenFilters");
		alDeleteFilters = (LPALDELETEFILTERS)alGetProcAddress("alDeleteFilters");
		alFilteri = (LPALFILTERI)alGetProcAddress("alFilteri");
		alFilterf = (LPALFILTERF)alGetProcAddress("alFilterf");
		alGenEffects = (LPALGENEFFECTS)alGetProcAddress("alGenEffects");
		alDeleteEffects = (LPALDELETEEFFECTS)alGetProcAddress("alDeleteEffects");
		alEffecti = (LPALEFFECTI)alGetProcAddress("alEffecti");
		alEffectf = (LPALEFFECTF)alGetProcAddress("alEffectf");
		alGenAuxiliaryEffectSlots = (LPALGENAUXILIARYEFFECTSLOTS)alGetProcAddress("alGenAuxiliaryEffectSlots");
		alDeleteAuxiliaryEffectSlots = (LPALDELETEAUXILIARYEFFECTSLOTS)alGetProcAddress("alDeleteAuxiliaryEffectSlots");
		alAuxiliaryEffectSloti = (LPALAUXILIARYEFFECTSLOTI)alGetProcAddress("alAuxiliaryEffectSloti");
		alAuxiliaryEffectSlotf = (LPALAUXILIARYEFFECTSLOTF)alGetProcAddress("alAuxiliaryEffectSlotf");
		S_CheckALError();

		if (!alGenFilters || !alGenEffects || !alGenAuxiliaryEffectSlots) {
			Com_Warn("s_effects is enabled but OpenAL driver does not support them.");
			Cvar_ForceSet("s_effects", "0");
			s_effects->modified = false;
			s_env.effects.loaded = false;
		} else {
			alGenFilters(1, &s_env.effects.underwater);
			S_CheckALError();

			alFilteri(s_env.effects.underwater, AL_FILTER_TYPE, AL_FILTER_LOWPASS);
			S_CheckALError();

			alFilterf(s_env.effects.underwater, AL_LOWPASS_GAIN, 0.8);
			S_CheckALError();

			alFilterf(s_env.effects.underwater, AL_LOWPASS_GAINHF, 0.3);
			S_CheckALError();

			s_env.effects.loaded = true;
		}
	} else {
		s_env.effects.loaded = false;
	}

	alGenSources(MAX_CHANNELS, s_env.sources);

	ALenum error = alGetError();

	if (error) {
		Com_Warn("Couldn't allocate channels: %s\n", alGetString(error));
		return;
	}

	Com_Print("Sound initialized (OpenAL, resample @ %dhz)\n", s_rate->integer);

	S_InitMedia();

	S_InitMusic();

	s_env.resample_buffer = Mem_TagMalloc(sizeof(int16_t) * 2048, MEM_TAG_SOUND);
}

/**
 * @brief
 */
void S_Shutdown(void) {

	if (!s_env.context) {
		return;
	}

	S_Stop();

	alDeleteSources(MAX_CHANNELS, s_env.sources);
	S_CheckALError();

	if (s_env.effects.loaded) {
		alDeleteFilters(1, &s_env.effects.underwater);
		S_CheckALError();

		s_env.effects.loaded = false;
	}

	S_ShutdownMusic();

	S_ShutdownMedia();

	alcMakeContextCurrent(NULL);
	S_CheckALError();

	alcDestroyContext(s_env.context);
	S_CheckALError();

	alcCloseDevice(s_env.device);
	S_CheckALError();

	SDL_QuitSubSystem(SDL_INIT_AUDIO);

	Cmd_RemoveAll(CMD_SOUND);
	
	Mem_Free(s_env.raw_sample_buffer);
	Mem_Free(s_env.converted_sample_buffer);
	Mem_Free(s_env.resample_buffer);

	Mem_FreeTag(MEM_TAG_SOUND);

	memset(&s_env, 0, sizeof(s_env));
}
