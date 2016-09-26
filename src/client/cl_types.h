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

#ifndef __CL_TYPES_H__
#define __CL_TYPES_H__

#include "net/net_types.h"
#include "renderer/r_types.h"
#include "sound/s_types.h"
#include "ui/ui_types.h"

typedef struct {
	pm_cmd_t cmd; // the movement command
	uint32_t time; // simulation time when the command was sent
	uint32_t timestamp; // system time when the command was sent
} cl_cmd_t;

typedef struct {
	uint32_t time; // simulation time for which the frame is valid
	int32_t frame_num; // sequential identifier, used for delta
	int32_t delta_frame_num; // negatives indicate no delta
	byte area_bits[MAX_BSP_AREAS >> 3]; // portal area visibility bits
	player_state_t ps;
	uint16_t num_entities;
	uint32_t entity_state; // non-masked index into cl.entity_states array
	_Bool valid; // false if delta parsing failed
} cl_frame_t;

typedef struct {
	entity_animation_t animation;
	uint32_t time;
	int32_t frame;
	int32_t old_frame;
	vec_t lerp;
	vec_t fraction;
} cl_entity_animation_t;

typedef struct {
	entity_state_t baseline; // delta from this if not from a previous frame
	entity_state_t current;
	entity_state_t prev; // will always be valid, but might just be a copy of current

	int32_t frame_num; // if not current, this entity isn't in the frame

	uint32_t time; // for intermittent effects

	uint32_t breath_puff_time; // time breath puffs should be drawn next

	cl_entity_animation_t animation1; // torso animation
	cl_entity_animation_t animation2; // legs animation

	vec3_t origin; // interpolated origin
	vec3_t termination; // and termination
	vec3_t angles; // and angles

	matrix4x4_t matrix; // interpolated translation and rotation
	matrix4x4_t inverse_matrix; // for box hull collision

	r_lighting_t lighting; // cached static lighting info
} cl_entity_t;

typedef struct {
	char info[MAX_QPATH]; // the full info string, e.g. newbie\qforcer/blue
	char name[MAX_QPATH]; // the player name, e.g. newbie
	char model[MAX_QPATH]; // the model name, e.g. qforcer
	char skin[MAX_QPATH]; // the skin name, e.g. blue

	r_model_t *head;
	r_material_t *head_skins[MD3_MAX_MESHES];

	r_model_t *torso;
	r_material_t *torso_skins[MD3_MAX_MESHES];

	r_model_t *legs;
	r_material_t *legs_skins[MD3_MAX_MESHES];

	r_image_t *icon; // for the scoreboard
} cl_client_info_t;

/**
 * @brief A circular buffer of recently sent user_cmd_t is maintained so that
 * we can always re-send the last 2 commands to counter packet loss, and so
 * that client-side prediction can verify its accuracy.
 */
#define CMD_BACKUP 64
#define CMD_MASK (CMD_BACKUP - 1)

/**
 * @brief Client side prediction output, produced by running sent but
 * unacknowledged user_cmd_t's through the player movement code locally.
 */
typedef struct {
	vec3_t origin; // this essentially becomes the view orientation

	vec3_t view_offset; // by adding in this offset
	vec3_t view_angles; // and applying these angles

	vec3_t error; // delta from prediction result to server result

	struct g_entity_s *ground_entity;

	uint32_t step_time; // simulation time when step was traversed
	uint32_t step_interval; // interpolation interval for step
	vec_t step; // step height (up or down)

	vec3_t origins[CMD_BACKUP]; // for debugging against the server
} cl_predicted_state_t;

/**
 * @brief We accumulate a large circular buffer of entity states for each
 * entity in order to calculate delta compression.
 */
#define ENTITY_STATE_BACKUP (PACKET_BACKUP * MAX_PACKET_ENTITIES)
#define ENTITY_STATE_MASK (ENTITY_STATE_BACKUP - 1)

/**
 * @brief The client structure is cleared at each level load, and is exposed to
 * the client game module to provide access to media and other client state.
 */
typedef struct {
	uint32_t time_demo_frames;
	uint32_t time_demo_start;

	uint32_t frame_counter;
	uint32_t packet_counter;

	cl_cmd_t cmds[CMD_BACKUP]; // each message will send several old cmds
	cl_predicted_state_t predicted_state; // client side prediction output

	cl_frame_t frame; // received from server
	cl_frame_t frames[PACKET_BACKUP]; // for calculating delta compression

	cl_frame_t *delta_frame; // the delta frame for the current frame

	cl_entity_t entities[MAX_ENTITIES]; // client entities

	entity_state_t entity_states[ENTITY_STATE_BACKUP]; // accumulated each frame
	uint32_t entity_state; // index (not wrapped) into entity states

	uint16_t client_num; // our client number, which is our entity number - 1

	uint32_t surpress_count; // number of messages rate suppressed

	uint32_t time; // server time that the client is rendering at
	uint32_t systime; // system time, not clamped to the server

	vec_t lerp; // linear interpolation between frames

	// the client maintains its own idea of view angles, which are
	// sent to the server each frame. It is cleared to 0 upon entering each level.
	// the server sends a delta when necessary which is added to the locally
	// tracked view angles to account for spawn and teleport direction changes
	vec3_t angles;

	uint32_t server_count; // server identification for precache
	uint16_t server_hz; // server frame rate (packets per second)

	_Bool demo_server; // we're viewing a demo
	_Bool third_person; // we're using a 3rd person camera

	char config_strings[MAX_CONFIG_STRINGS][MAX_STRING_CHARS];
	uint16_t precache_check;

	// for client side prediction clipping
	cm_bsp_model_t *cm_models[MAX_MODELS];

	// caches of indexed assets to be referenced by the server
	r_model_t *model_precache[MAX_MODELS];
	s_sample_t *sound_precache[MAX_SOUNDS];
	s_music_t *music_precache[MAX_MUSICS];
	r_image_t *image_precache[MAX_IMAGES];

	cl_client_info_t client_info[MAX_CLIENTS];
} cl_client_t;

typedef enum {
	CL_UNINITIALIZED, // not initialized
	CL_DISCONNECTED, // not talking to a server
	CL_CONNECTING, // sending request packets to the server
	CL_CONNECTED, // netchan_t established, waiting for svc_server_data
	CL_LOADING, // loading media
	CL_ACTIVE // game views should be displayed
} cl_state_t;

typedef enum {
	KEY_UI,
	KEY_CONSOLE,
	KEY_GAME,
	KEY_CHAT
} cl_key_dest_t;

typedef enum {
	SDL_SCANCODE_MOUSE1 = (SDL_SCANCODE_APP2 + 1),
	SDL_SCANCODE_MOUSE3,
	SDL_SCANCODE_MOUSE2,
	SDL_SCANCODE_MOUSE4,
	SDL_SCANCODE_MOUSE5,
	SDL_SCANCODE_MOUSE6,
	SDL_SCANCODE_MOUSE7,
	SDL_SCANCODE_MOUSE8
} SDL_Buttoncode;

enum {
	SDLK_MOUSE1 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MOUSE1),
	SDLK_MOUSE2 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MOUSE2),
	SDLK_MOUSE3 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MOUSE3),
	SDLK_MOUSE4 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MOUSE4),
	SDLK_MOUSE5 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MOUSE5),
	SDLK_MOUSE6 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MOUSE6),
	SDLK_MOUSE7 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MOUSE7),
	SDLK_MOUSE8 = SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MOUSE8)
};

#if defined(__APPLE__)
#define KMOD_CLIPBOARD KMOD_GUI
#else
#define KMOD_CLIPBOARD KMOD_CTRL
#endif

typedef struct {
	cl_key_dest_t dest;

	char *binds[SDL_NUM_SCANCODES];
	_Bool down[SDL_NUM_SCANCODES];
} cl_key_state_t;

typedef struct {
	_Bool team_chat;
} cl_chat_state_t;

typedef struct {
	int32_t x, y;
	int32_t old_x, old_y;
	_Bool grabbed;
} cl_mouse_state_t;

typedef struct {
	_Bool http;
	file_t *file;
	char tempname[MAX_OS_PATH];
	char name[MAX_OS_PATH];
} cl_download_t;

// server information, for finding network games
typedef enum {
	SERVER_SOURCE_INTERNET,
	SERVER_SOURCE_USER,
	SERVER_SOURCE_BCAST
} cl_server_source_t;

typedef struct {
	net_addr_t addr;
	cl_server_source_t source;
	char hostname[64];
	char name[32];
	char gameplay[32];
	uint16_t clients;
	uint16_t max_clients;
	uint32_t ping_time; // when we pinged the server
	uint16_t ping; // server latency
} cl_server_info_t;

typedef struct {
	uint16_t percent;
	const char *status;
	r_image_t *background;
} cl_loading_t;

/**
 * @brief The cl_static_t structure is persistent for the execution of the
 * game. It is only cleared when Cl_Init is called. It is not exposed to the
 * client game module, but is (rarely) visible to other subsystems (renderer).
 */
typedef struct {
	cl_state_t state;

	cl_key_state_t key_state;

	cl_mouse_state_t mouse_state;

	cl_chat_state_t chat_state;

	uint32_t packet_delta; // milliseconds since last outgoing packet
	uint32_t render_delta; // milliseconds since last renderer frame

	// connection information
	char server_name[MAX_OS_PATH]; // name of server to connect to
	uint32_t connect_time; // for connection retransmits

	net_chan_t net_chan; // network channel

	uint32_t challenge; // from the server to use for connecting
	uint32_t spawn_count;

	cl_loading_t loading; // loading status

	char download_url[MAX_OS_PATH]; // for http downloads
	cl_download_t download; // current download (udp or http)

	char demo_filename[MAX_OS_PATH];
	file_t *demo_file;

	GList *servers; // list of cl_server_info_t from all sources

	uint32_t broadcast_time; // time when last broadcast ping was sent

	struct cg_export_s *cgame;
} cl_static_t;

#endif /* __CL_TYPES_H__ */
