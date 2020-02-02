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

#define MAX_ACTIVE_LIGHTS               10

#define TEXTURE_DIFFUSE                  0
#define TEXTURE_NORMALMAP                1
#define TEXTURE_GLOSSMAP                 2
#define TEXTURE_LIGHTMAP                 3
#define TEXTURE_STAINMAP                 4

#define TEXTURE_MASK_DIFFUSE            (1 << TEXTURE_DIFFUSE)
#define TEXTURE_MASK_NORMALMAP          (1 << TEXTURE_NORMALMAP)
#define TEXTURE_MASK_GLOSSMAP           (1 << TEXTURE_GLOSSMAP)
#define TEXTURE_MASK_LIGHTMAP           (1 << TEXTURE_LIGHTMAP)
#define TEXTURE_MASK_STAINMAP           (1 << TEXTURE_STAINMAP)
#define TEXTURE_MASK_ALL                0xff

uniform int textures;

uniform sampler2D texture_diffuse;
uniform sampler2D texture_normalmap;
uniform sampler2D texture_glossmap;
uniform sampler2DArray texture_lightmap;

uniform float alpha_threshold;

uniform float modulate;

uniform float bump;
uniform float parallax;
uniform float hardness;
uniform float specular;

uniform vec4 light_positions[MAX_ACTIVE_LIGHTS];
uniform vec3 light_colors[MAX_ACTIVE_LIGHTS];

uniform vec3 fog_parameters;
uniform vec3 fog_color;

uniform vec4 caustics;

in vertex_data {
	vec3 position;
	vec3 normal;
	vec3 tangent;
	vec3 bitangent;
	vec2 diffuse;
	vec2 lightmap;
	vec4 color;
	vec3 eye;
} vertex;

out vec4 out_color;

/**
 * @brief
 */
void main(void) {

	vec4 diffuse;
	if ((textures & TEXTURE_MASK_DIFFUSE) == TEXTURE_MASK_DIFFUSE) {
		diffuse = texture(texture_diffuse, vertex.diffuse) * vertex.color;

		if (diffuse.a < alpha_threshold) {
			discard;
		}
	} else {
		diffuse = vertex.color;
	}

	vec4 normalmap;
	if ((textures & TEXTURE_MASK_NORMALMAP) == TEXTURE_MASK_NORMALMAP) {
		normalmap = texture(texture_normalmap, vertex.diffuse);
		normalmap.xyz = normalize(normalmap.xyz);
		normalmap.xy = (normalmap.xy * 2.0 - 1.0) * bump;
		normalmap.xyz = normalize(normalmap.xyz);
	} else {
		normalmap = vec4(0.0, 0.0, 1.0, 0.5);
	}

	vec4 glossmap;
	if ((textures & TEXTURE_MASK_GLOSSMAP) == TEXTURE_MASK_GLOSSMAP) {
		glossmap = texture(texture_glossmap, vertex.diffuse);
	} else {
		glossmap = vec4(1.0);
	}

	vec3 lightmap;
	if ((textures & TEXTURE_MASK_LIGHTMAP) == TEXTURE_MASK_LIGHTMAP) {
		vec3 ambient = texture(texture_lightmap, vec3(vertex.lightmap, 0)).rgb * modulate;
		vec3 diffuse = texture(texture_lightmap, vec3(vertex.lightmap, 1)).rgb * modulate;
		vec3 radiosity = texture(texture_lightmap, vec3(vertex.lightmap, 2)).rgb * modulate;
		vec3 diffuse_dir = texture(texture_lightmap, vec3(vertex.lightmap, 3)).xyz;
		vec3 radiosity_dir = texture(texture_lightmap, vec3(vertex.lightmap, 4)).xyz;

		diffuse_dir = normalize(diffuse_dir * 2.0 - 1.0);
		radiosity_dir = normalize(radiosity_dir * 2.0 - 1.0);

		lightmap = ambient + diffuse + radiosity;

		lightmap = normalize(diffuse_dir + radiosity_dir);
	} else {
		lightmap = vec3(1.0);
	}

	vec4 stainmap;
	if ((textures & TEXTURE_MASK_STAINMAP) == TEXTURE_MASK_STAINMAP) {
		stainmap = texture(texture_lightmap, vec3(vertex.lightmap, 2));
	} else {
		stainmap = vec4(0.0);
	}

	out_color = ColorFilter(/*diffuse * */vec4(lightmap, 1.0));
}
