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

layout (points) in;
layout (points, max_vertices = 1) out;

uniform mat4 view;

uniform vec4 plane;

in vertex_data {
	vec3 position;
	vec4 color;
} in_vertex[];

out vertex_data {
	vec3 position;
	vec4 color;
} out_vertex;

/**
 * @brief
 */
void main() {

	if (dot(plane.xyz, in_vertex[0].position) - plane.w < 0.0) {

		gl_Position = gl_in[0].gl_Position;

		out_vertex.position = in_vertex[0].position;

		gl_PointSize = gl_in[0].gl_PointSize;

		out_vertex.color = in_vertex[0].color;

		EmitVertex();
	}

    EndPrimitive();
}
