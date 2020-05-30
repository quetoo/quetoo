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
layout (location = 1) in vec3 in_next_position;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform float lerp;

uniform float dist;
uniform float z;
uniform float min_z, max_z;

out vertex_data {
	vec3 position;
	float dist_to_ground;
} vertex;

/**
 * @brief
 */
void main(void) {

	vec4 position = vec4(mix(in_position, in_next_position, lerp), 1.0);

	vertex.dist_to_ground = (16.f + ((position.z - min_z) + dist)) / 512.f;
	
	position.x *= 1.0 + (vertex.dist_to_ground * 2);
	position.y *= 1.0 + (vertex.dist_to_ground * 2);

	position = model * position;
	position.z *= 0;
	position.z += z;

	vertex.position = vec3(view * position);

	gl_Position = projection * vec4(vertex.position, 1.0);
}
