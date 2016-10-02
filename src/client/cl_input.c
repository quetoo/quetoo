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

#include "cl_local.h"

cvar_t *cl_run;
static cvar_t *cl_forward_speed;
static cvar_t *cl_pitch_speed;
static cvar_t *cl_right_speed;
static cvar_t *cl_up_speed;
static cvar_t *cl_yaw_speed;

cvar_t *m_interpolate;
cvar_t *m_invert;
cvar_t *m_sensitivity;
cvar_t *m_sensitivity_zoom;
cvar_t *m_pitch;
cvar_t *m_yaw;
static cvar_t *m_grab;

/*
 * KEY BUTTONS
 *
 * Continuous button event tracking is complicated by the fact that two different
 * input sources (say, mouse button 1 and the control key) can both press the
 * same button, but the button should only be released when both of the
 * pressing key have been released.
 *
 * When a key event issues a button command (+forward, +attack, etc), it appends
 * its key number as a parameter to the command so it can be matched up with
 * the release.
 *
 * state bit 0 is the current state of the key
 * state bit 1 is edge triggered on the up to down transition
 * state bit 2 is edge triggered on the down to up transition
 */

typedef struct {
	SDL_Scancode keys[2]; // keys holding it down
	uint32_t down_time; // msec timestamp
	uint32_t msec; // msec down this frame
	byte state;
} cl_button_t;

static cl_button_t cl_buttons[12];
#define in_left cl_buttons[0]
#define in_right cl_buttons[1]
#define in_forward cl_buttons[2]
#define in_back cl_buttons[3]
#define in_look_up cl_buttons[4]
#define in_look_down cl_buttons[5]
#define in_move_left cl_buttons[6]
#define in_move_right cl_buttons[7]
#define in_speed cl_buttons[8]
#define in_attack cl_buttons[9]
#define in_up cl_buttons[10]
#define in_down cl_buttons[11]

/**
 * @brief
 */
static void Cl_KeyDown(cl_button_t *b) {
	SDL_Scancode k;

	const char *c = Cmd_Argv(1);
	if (c[0])
		k = atoi(c);
	else
		k = SDL_NUM_SCANCODES; // typed manually at the console for continuous down

	if (k == b->keys[0] || k == b->keys[1])
		return; // repeating key

	if (b->keys[0] == SDL_SCANCODE_UNKNOWN)
		b->keys[0] = k;
	else if (b->keys[1] == SDL_SCANCODE_UNKNOWN)
		b->keys[1] = k;
	else {
		Com_Debug("3 keys down for button\n");
		return;
	}

	if (b->state & 1)
		return; // still down

	// save the down time so that we can calculate fractional time later
	const char *t = Cmd_Argv(2);
	b->down_time = atoi(t);
	if (!b->down_time)
		b->down_time = quetoo.time;

	b->state |= 1;
}

/**
 * @brief
 */
static void Cl_KeyUp(cl_button_t *b) {

	if (Cmd_Argc() < 2) { // typed manually at the console, assume for un-sticking, so clear all
		b->keys[0] = b->keys[1] = 0;
		return;
	}

	const SDL_Scancode k = atoi(Cmd_Argv(1));

	if (b->keys[0] == k)
		b->keys[0] = SDL_SCANCODE_UNKNOWN;
	else if (b->keys[1] == k)
		b->keys[1] = SDL_SCANCODE_UNKNOWN;
	else
		return; // key up without corresponding down

	if (b->keys[0] || b->keys[1])
		return; // some other key is still holding it down

	if (!(b->state & 1))
		return; // still up (this should not happen)

	// save timestamp
	const char *t = Cmd_Argv(2);
	const uint32_t up_time = atoi(t);
	if (up_time)
		b->msec += up_time - b->down_time;
	else
		b->msec += 10;

	b->state &= ~1; // now up
}

static void Cl_Up_down_f(void) {
	Cl_KeyDown(&in_up);
}
static void Cl_Up_up_f(void) {
	Cl_KeyUp(&in_up);
}
static void Cl_Down_down_f(void) {
	Cl_KeyDown(&in_down);
}
static void Cl_Down_up_f(void) {
	Cl_KeyUp(&in_down);
}
static void Cl_Left_down_f(void) {
	Cl_KeyDown(&in_left);
}
static void Cl_Left_up_f(void) {
	Cl_KeyUp(&in_left);
}
static void Cl_Right_down_f(void) {
	Cl_KeyDown(&in_right);
}
static void Cl_Right_up_f(void) {
	Cl_KeyUp(&in_right);
}
static void Cl_Forward_down_f(void) {
	Cl_KeyDown(&in_forward);
}
static void Cl_Forward_up_f(void) {
	Cl_KeyUp(&in_forward);
}
static void Cl_Back_down_f(void) {
	Cl_KeyDown(&in_back);
}
static void Cl_Back_up_f(void) {
	Cl_KeyUp(&in_back);
}
static void Cl_LookUp_down_f(void) {
	Cl_KeyDown(&in_look_up);
}
static void Cl_LookUp_up_f(void) {
	Cl_KeyUp(&in_look_up);
}
static void Cl_LookDown_down_f(void) {
	Cl_KeyDown(&in_look_down);
}
static void Cl_LookDown_up_f(void) {
	Cl_KeyUp(&in_look_down);
}
static void Cl_MoveLeft_down_f(void) {
	Cl_KeyDown(&in_move_left);
}
static void Cl_MoveLeft_up_f(void) {
	Cl_KeyUp(&in_move_left);
}
static void Cl_MoveRight_down_f(void) {
	Cl_KeyDown(&in_move_right);
}
static void Cl_MoveRight_up_f(void) {
	Cl_KeyUp(&in_move_right);
}
static void Cl_Speed_down_f(void) {
	Cl_KeyDown(&in_speed);
}
static void Cl_Speed_up_f(void) {
	Cl_KeyUp(&in_speed);
}
static void Cl_Attack_down_f(void) {
	Cl_KeyDown(&in_attack);
}
static void Cl_Attack_up_f(void) {
	Cl_KeyUp(&in_attack);
}
static void Cl_CenterView_f(void) {
	cl.angles[PITCH] = 0;
}

/**
 * @brief Returns the fraction of the command interval for which the key was down.
 */
static vec_t Cl_KeyState(cl_button_t *key, uint32_t cmd_msec) {

	uint32_t msec = key->msec;
	key->msec = 0;

	if (key->state) { // still down, reset downtime for next frame
		msec += quetoo.time - key->down_time;
		key->down_time = quetoo.time;
	}

	const vec_t frac = (msec * 1000.0) / (cmd_msec * 1000.0);

	return Clamp(frac, 0.0, 1.0);
}


/**
 * @brief Inserts source into destination at the specified offset, without
 * exceeding the specified length.
 *
 * @return The number of chars inserted.
 */
static size_t Cl_TextEvent_Insert(char *dest, const char *src, const size_t ofs, const size_t len) {
	char tmp[MAX_STRING_CHARS];

	const size_t l = strlen(dest);

	g_strlcpy(tmp, dest + ofs, sizeof(tmp));
	dest[ofs] = '\0';

	const size_t i = g_strlcat(dest, src, len);
	if (i < len) {
		g_strlcat(dest, tmp, len);
	}

	return strlen(dest) - l;
}

/**
 * @brief
 */
static void Cl_TextEvent(const SDL_Event *event) {

	console_input_t *in;

	if (cls.key_state.dest == KEY_CONSOLE) {
		in = &cl_console.input;
	} else if (cls.key_state.dest == KEY_CHAT) {
		in = &cl_chat_console.input;
	} else {
		return;
	}

	const char *src = event->text.text;

	in->pos += Cl_TextEvent_Insert(in->buffer, src, in->pos, sizeof(in->buffer));
}

/**
 * @brief Handles system events, spanning all key destinations.
 *
 * @return True if the event was handled, false otherwise.
 */
static _Bool Cl_HandleSystemEvent(const SDL_Event *event) {

	if (event->type != SDL_KEYDOWN) { // don't care
		return false;
	}

	if (event->key.keysym.sym == SDLK_ESCAPE) { // escape can cancel a few things

		// connecting to a server
		switch (cls.state) {
			case CL_CONNECTING:
			case CL_CONNECTED:
				Com_Error(ERR_DROP, "Connection aborted by user\n");
				break;
			case CL_LOADING:
				return false;
			default:
				break;
		}

		// message mode
		if (cls.key_state.dest == KEY_CHAT) {
			Cl_SetKeyDest(KEY_GAME);
			return true;
		}

		// console
		if (cls.key_state.dest == KEY_CONSOLE) {
			Cl_ToggleConsole_f();
			return true;
		}

		// and menus
		if (cls.key_state.dest == KEY_UI) {

			// if we're in the game, just hide the menus
			if (cls.state == CL_ACTIVE) {
				Cl_SetKeyDest(KEY_GAME);
				return true;
			}

			return false;
		}

		Cl_SetKeyDest(KEY_UI);
		return true;
	}

	// for everything other than ESC, check for system-level command binds

	SDL_Scancode key = event->key.keysym.scancode;
	if (cls.key_state.binds[key]) {
		cmd_t *cmd;

		if ((cmd = Cmd_Get(cls.key_state.binds[key]))) {
			if (cmd->flags & CMD_SYSTEM) {
				Cbuf_AddText(cls.key_state.binds[key]);
				Cbuf_Execute();
				return true;
			}
		}
	}
	
	return false;
}

/**
 * @brief
 */
static void Cl_HandleEvent(const SDL_Event *event) {

	switch (event->type) {

		case SDL_KEYDOWN:
		case SDL_KEYUP:
			Cl_KeyEvent(event);
			break;

		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
			Cl_MouseButtonEvent(event);
			break;

		case SDL_MOUSEWHEEL:
			Cl_MouseWheelEvent(event);
			break;

		case SDL_MOUSEMOTION:
			Cl_MouseMotionEvent(event);
			break;

		case SDL_TEXTINPUT:
			Cl_TextEvent(event);
			break;

		case SDL_QUIT:
			Cmd_ExecuteString("quit");
			break;

		case SDL_WINDOWEVENT:
			if (event->window.event == SDL_WINDOWEVENT_RESIZED) {
				if (!r_context.fullscreen) {
					Cvar_SetValue(r_windowed_width->name, event->window.data1);
					Cvar_SetValue(r_windowed_height->name, event->window.data2);
					Cbuf_AddText("r_restart\n");
				}
			}
			break;
	}
}

/**
 * @brief Updates mouse state, ensuring the window has mouse focus, and draws the cursor.
 */
static void Cl_UpdateMouseState(void) {

	// force a mouse grab when changing video modes
	if (r_view.update) {
		cls.mouse_state.grabbed = false;
	}

	if (cls.key_state.dest == KEY_UI || !m_grab->integer) {
		if (cls.mouse_state.grabbed) {
			SDL_ShowCursor(true);
			SDL_SetWindowGrab(r_context.window, false);
			cls.mouse_state.grabbed = false;
		}
	} else if (cls.key_state.dest == KEY_CONSOLE) {
		if (!r_context.fullscreen) { // allow cursor to move outside window
			if (cls.mouse_state.grabbed) {
				SDL_ShowCursor(true);
				SDL_SetWindowGrab(r_context.window, false);
				cls.mouse_state.grabbed = false;
			}
		}
	} else {
		if (!cls.mouse_state.grabbed) { // grab it for everything else
			SDL_ShowCursor(false);
			SDL_SetWindowGrab(r_context.window, true);
			cls.mouse_state.grabbed = true;
		}
	}
}

/**
 * @brief
 */
void Cl_HandleEvents(void) {

	if (!SDL_WasInit(SDL_INIT_VIDEO))
		return;

	Cl_UpdateMouseState();

	// handle new key events
	while (true) {
		SDL_Event event;

		memset(&event, 0, sizeof(event));

		if (SDL_PollEvent(&event)) {
			if (Cl_HandleSystemEvent(&event) == false) {
				Ui_HandleEvent(&event);
				Cl_HandleEvent(&event);
			}
		} else {
			break;
		}
	}
}

/**
 * @brief
 */
static void Cl_ClampPitch(void) {
	const pm_state_t *s = &cl.frame.ps.pm_state;

	// ensure our pitch is valid
	vec_t pitch = UnpackAngle(s->delta_angles[PITCH] + s->kick_angles[PITCH]);

	if (pitch > 180.0)
		pitch -= 360.0;

	if (cl.angles[PITCH] + pitch < -360.0)
		cl.angles[PITCH] += 360.0; // wrapped
	if (cl.angles[PITCH] + pitch > 360.0)
		cl.angles[PITCH] -= 360.0; // wrapped

	if (cl.angles[PITCH] + pitch > 89.0)
		cl.angles[PITCH] = 89.0 - pitch;
	if (cl.angles[PITCH] + pitch < -89.0)
		cl.angles[PITCH] = -89.0 - pitch;
}

/**
 * @brief Accumulate this frame's movement-related inputs and assemble a movement
 * command to send to the server. This may be called several times for each
 * command that is transmitted if the client is running asynchronously.
 */
void Cl_Move(pm_cmd_t *cmd) {

	if (cmd->msec < 1) // save key states for next move
		return;

	// keyboard move forward / back
	cmd->forward += cl_forward_speed->value * cmd->msec * Cl_KeyState(&in_forward, cmd->msec);
	cmd->forward -= cl_forward_speed->value * cmd->msec * Cl_KeyState(&in_back, cmd->msec);

	// keyboard strafe left / right
	cmd->right += cl_right_speed->value * cmd->msec * Cl_KeyState(&in_move_right, cmd->msec);
	cmd->right -= cl_right_speed->value * cmd->msec * Cl_KeyState(&in_move_left, cmd->msec);

	// keyboard jump / crouch
	cmd->up += cl_up_speed->value * cmd->msec * Cl_KeyState(&in_up, cmd->msec);
	cmd->up -= cl_up_speed->value * cmd->msec * Cl_KeyState(&in_down, cmd->msec);

	// keyboard turn left / right
	cl.angles[YAW] -= cl_yaw_speed->value * cmd->msec * Cl_KeyState(&in_right, cmd->msec);
	cl.angles[YAW] += cl_yaw_speed->value * cmd->msec * Cl_KeyState(&in_left, cmd->msec);

	// keyboard look up / down
	cl.angles[PITCH] -= cl_pitch_speed->value * cmd->msec * Cl_KeyState(&in_look_up, cmd->msec);
	cl.angles[PITCH] += cl_pitch_speed->value * cmd->msec * Cl_KeyState(&in_look_down, cmd->msec);

	Cl_ClampPitch(); // clamp, accounting for frame delta angles

	// pack the angles into the command
	PackAngles(cl.angles, cmd->angles);

	// set any button hits that occurred since last frame
	if (in_attack.state & 3)
	cmd->buttons |= BUTTON_ATTACK;

	in_attack.state &= ~2;

	if (cl_run->value) { // run by default, walk on speed toggle
		if (in_speed.state & 1)
		cmd->buttons |= BUTTON_WALK;
	} else { // walk by default, run on speed toggle
		if (!(in_speed.state & 1))
		cmd->buttons |= BUTTON_WALK;
	}
}

/**
 * @brief
 */
void Cl_ClearInput(void) {

	memset(cl_buttons, 0, sizeof(cl_buttons));
}

/**
 * @brief
 */
void Cl_InitInput(void) {

	Cmd_Add("center_view", Cl_CenterView_f, CMD_CLIENT, NULL);
	Cmd_Add("+move_up", Cl_Up_down_f, CMD_CLIENT, NULL);
	Cmd_Add("-move_up", Cl_Up_up_f, CMD_CLIENT, NULL);
	Cmd_Add("+move_down", Cl_Down_down_f, CMD_CLIENT, NULL);
	Cmd_Add("-move_down", Cl_Down_up_f, CMD_CLIENT, NULL);
	Cmd_Add("+left", Cl_Left_down_f, CMD_CLIENT, NULL);
	Cmd_Add("-left", Cl_Left_up_f, CMD_CLIENT, NULL);
	Cmd_Add("+right", Cl_Right_down_f, CMD_CLIENT, NULL);
	Cmd_Add("-right", Cl_Right_up_f, CMD_CLIENT, NULL);
	Cmd_Add("+forward", Cl_Forward_down_f, CMD_CLIENT, NULL);
	Cmd_Add("-forward", Cl_Forward_up_f, CMD_CLIENT, NULL);
	Cmd_Add("+back", Cl_Back_down_f, CMD_CLIENT, NULL);
	Cmd_Add("-back", Cl_Back_up_f, CMD_CLIENT, NULL);
	Cmd_Add("+look_up", Cl_LookUp_down_f, CMD_CLIENT, NULL);
	Cmd_Add("-look_up", Cl_LookUp_up_f, CMD_CLIENT, NULL);
	Cmd_Add("+look_down", Cl_LookDown_down_f, CMD_CLIENT, NULL);
	Cmd_Add("-look_down", Cl_LookDown_up_f, CMD_CLIENT, NULL);
	Cmd_Add("+move_left", Cl_MoveLeft_down_f, CMD_CLIENT, NULL);
	Cmd_Add("-move_left", Cl_MoveLeft_up_f, CMD_CLIENT, NULL);
	Cmd_Add("+move_right", Cl_MoveRight_down_f, CMD_CLIENT, NULL);
	Cmd_Add("-move_right", Cl_MoveRight_up_f, CMD_CLIENT, NULL);
	Cmd_Add("+speed", Cl_Speed_down_f, CMD_CLIENT, NULL);
	Cmd_Add("-speed", Cl_Speed_up_f, CMD_CLIENT, NULL);
	Cmd_Add("+attack", Cl_Attack_down_f, CMD_CLIENT, NULL);
	Cmd_Add("-attack", Cl_Attack_up_f, CMD_CLIENT, NULL);

	cl_run = Cvar_Add("cl_run", "1", CVAR_ARCHIVE, NULL);
	cl_forward_speed = Cvar_Add("cl_forward_speed", "300.0", 0, NULL);
	cl_pitch_speed = Cvar_Add("cl_pitch_speed", "0.15", 0, NULL);
	cl_right_speed = Cvar_Add("cl_right_speed", "300.0", 0, NULL);
	cl_up_speed = Cvar_Add("cl_up_speed", "300.0", 0, NULL);
	cl_yaw_speed = Cvar_Add("cl_yaw_speed", "0.15", 0, NULL);

	m_sensitivity = Cvar_Add("m_sensitivity", "3.0", CVAR_ARCHIVE, NULL);
	m_sensitivity_zoom = Cvar_Add("m_sensitivity_zoom", "1.0", CVAR_ARCHIVE, NULL);
	m_interpolate = Cvar_Add("m_interpolate", "0", CVAR_ARCHIVE, NULL);
	m_invert = Cvar_Add("m_invert", "0", CVAR_ARCHIVE, "Invert the mouse");
	m_pitch = Cvar_Add("m_pitch", "0.022", 0, NULL);
	m_yaw = Cvar_Add("m_yaw", "0.022", 0, NULL);
	m_grab = Cvar_Add("m_grab", "1", 0, NULL);

	Cl_ClearInput();

	cls.mouse_state.grabbed = true;
}

