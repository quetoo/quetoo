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

uniform mat4 view;

uniform sampler2DArray texture_material;
uniform sampler2D texture_stage;

uniform sampler3D texture_lightgrid_ambient;
uniform sampler3D texture_lightgrid_diffuse;
uniform sampler3D texture_lightgrid_direction;

uniform float alpha_threshold;

uniform float modulate;
uniform float entity_ambient;

uniform material_t material;
uniform stage_t stage;

in vertex_data {
	vec3 position;
	vec3 normal;
	vec3 tangent;
	vec3 bitangent;
	vec2 diffusemap;
	vec3 lightgrid;
	vec4 color;
} vertex;

out vec4 out_color;
vec3 out_color_debug;

uniform vec4 tint_colors[3];

vec3 do_tintmap(vec3 diffuse, vec4 tintmap)
{
	diffuse.rgb *= 1.0 - tintmap.a;
	
	for (int i = 0; i < 3; i++)
		diffuse.rgb += (tint_colors[i] * tintmap[i]).rgb * tintmap.a;

	return diffuse.rgb;
}

/**
 * @brief
 */
void main(void) {

	if ((stage.flags & STAGE_MATERIAL) == STAGE_MATERIAL) {

		vec4 diffusemap = texture(texture_material, vec3(vertex.diffusemap, 0));

		diffusemap *= vertex.color;

		if (diffusemap.a < alpha_threshold) {
			discard;
		}

		vec4 tintmap = texture(texture_material, vec3(vertex.diffusemap, 3));
		diffusemap.rgb = do_tintmap(diffusemap.rgb, tintmap);

		vec4 normalmap = texture(texture_material, vec3(vertex.diffusemap, 1));
		vec4 glossmap = texture(texture_material, vec3(vertex.diffusemap, 2));

		mat3 tbn = mat3(normalize(vertex.tangent), normalize(vertex.bitangent), normalize(vertex.normal));
		vec3 normal = normalize(tbn * ((normalmap.xyz * 2.0 - 1.0) * vec3(material.roughness, material.roughness, 1.0)));

		vec3 ambient = texture(texture_lightgrid_ambient, vertex.lightgrid).rgb + vec3(entity_ambient);
		vec3 diffuse = texture(texture_lightgrid_diffuse, vertex.lightgrid).rgb;
		vec3 direction = texture(texture_lightgrid_direction, vertex.lightgrid).xyz;

		direction = normalize((view * vec4(direction * 2.0 - 1.0, 1.0)).xyz);

		out_color = diffusemap;

		vec3 light_diffuse = ambient + diffuse * max(0.0, dot(normal, direction));
		vec3 light_specular = vec3(0.0);

		dynamic_light(vertex.position, normal, 64, light_diffuse, light_specular, out_color_debug);

		out_color.rgb = clamp(out_color.rgb * light_diffuse  * modulate, 0.0, 32.0);
		out_color.rgb = clamp(out_color.rgb + light_specular * modulate, 0.0, 32.0);

	} else {
		vec4 effect = texture(texture_stage, vertex.diffusemap);

		effect *= vertex.color;

		out_color = effect;
	}

	// postprocessing
	
	out_color.rgb = tonemap(out_color.rgb);

	out_color.rgb = fog(vertex.position, out_color.rgb);

	out_color.rgb = color_filter(out_color.rgb);

	out_color.rgb = dither(out_color.rgb);

	//out_color.rgb = out_color_debug; // debuggery
}

