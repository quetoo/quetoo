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

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_diffusemap;

uniform mat4 model;
uniform mat4 cube;

out vertex_data {
	vec3 position;
	vec3 cubemap;
	vec3 lightgrid;
} vertex;

invariant gl_Position;

/**
 * @brief
 */
void main(void) {

	mat4 view_model = view * model;

	vec4 position = vec4(in_position, 1.0);

	vertex.position = vec3(view_model * position);
	vertex.cubemap = vec3(cube * position);
	vertex.lightgrid = lightgrid_uvw(vec3(512.0, 512.0, 512.0));

	gl_Position = projection3D * view_model * vec4(in_position, 1.0);
}
