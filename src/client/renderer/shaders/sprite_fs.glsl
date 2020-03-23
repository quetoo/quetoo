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

uniform sampler2D texture_diffusemap;

in vertex_data {
	vec3 position;
	vec2 diffusemap;
	vec4 color;
} vertex;

out vec4 out_color;

/**
 * @brief
 */
void main(void) {

	out_color = vertex.color * texture(texture_diffusemap, vertex.diffusemap);

	// maybe move this to the vertex shader?
	float fogginess = fog_factor(vertex.position);
	
	out_color.rgb = mix(out_color.rgb, fog_color, fogginess);

	out_color.a = mix(out_color.a, 0.0, fogginess);

	out_color.rgb = color_filter(out_color.rgb);

	out_color.a *= soft_edges();
}