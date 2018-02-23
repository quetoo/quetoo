/**
 * @brief Default fragment shader.
 */

#version 330

#define FRAGMENT_SHADER

#include "include/matrix.glsl"
#include "include/fog.glsl"
#include "include/noise3d.glsl"
#include "include/tint.glsl"
#include "include/color.glsl"

#define MAX_LIGHTS $r_max_lights
#define MODULATE_SCALE $r_modulate

#if MAX_LIGHTS
struct LightParameters {
	vec3 ORIGIN[MAX_LIGHTS];
	vec3 COLOR[MAX_LIGHTS];
	float RADIUS[MAX_LIGHTS];
};

uniform LightParameters LIGHTS;
#endif

struct CausticParameters {
	bool ENABLE;
	vec3 COLOR;
};

uniform CausticParameters CAUSTIC;

uniform bool DIFFUSE;
uniform bool LIGHTMAP;
uniform bool DELUXEMAP;
uniform bool NORMALMAP;
uniform bool GLOSSMAP;
uniform bool STAINMAP;

uniform float BUMP;
uniform float PARALLAX;
uniform float HARDNESS;
uniform float SPECULAR;

uniform sampler2D SAMPLER0;
uniform sampler2D SAMPLER1;
uniform sampler2D SAMPLER2;
uniform sampler2D SAMPLER3;
uniform sampler2D SAMPLER4;
uniform sampler2D SAMPLER8;

uniform float ALPHA_THRESHOLD;

uniform float TIME_FRACTION;
uniform float TIME;

in VertexData {
	vec3 modelpoint;
	vec2 texcoords[2];
	vec4 color;
	vec3 point;
	vec3 normal;
	vec3 tangent;
	vec3 bitangent;
	vec3 eye;
};

const vec3 two = vec3(2.0);
const vec3 negHalf = vec3(-0.5);

vec3 eyeDir;

out vec4 fragColor;

/**
 * @brief Used to dither the image before quantizing, combats banding artifacts.
 */
void DitherFragment(inout vec3 color) {

	// source: Alex Vlachos, Valve Software. Advanced VR Rendering, GDC2015.

	vec3 pattern = vec3(dot(vec2(171.0, 231.0), gl_FragCoord.xy));
	pattern.rgb = fract(pattern.rgb / vec3(103.0, 71.0, 97.0));
	color = clamp(color + (pattern.rgb / 255.0), 0.0, 1.0);

}

/**
 * @brief Yield the parallax offset for the texture coordinate.
 */
vec2 BumpTexcoord() {
	float height = texture(SAMPLER3, texcoords[0]).a;
	return texcoords[0] + eyeDir.xy * (height * 0.04 - 0.02) * PARALLAX;
}

/**
 * @brief Yield the diffuse modulation from bump-mapping.
 */
void BumpFragment(in vec3 deluxemap, in vec3 normalmap, in vec3 glossmap, out float lightmapBumpScale, out float lightmapSpecularScale) {
	float glossFactor = clamp(dot(glossmap, vec3(0.299, 0.587, 0.114)), 0.0078125, 1.0);

	lightmapBumpScale = clamp(dot(deluxemap, normalmap), 0.0, 1.0);
	lightmapSpecularScale = (HARDNESS * glossFactor) * pow(clamp(-dot(eyeDir, reflect(deluxemap, normalmap)), 0.0078125, 1.0), (16.0 * glossFactor) * SPECULAR);
}

/**
 * @brief Yield the final sample color after factoring in dynamic light sources.
 */
void LightFragment(in vec4 diffuse, in vec3 lightmap, in vec3 normalmap, in float lightmapBumpScale, in float lightmapSpecularScale) {

	vec3 light = vec3(0.0);

#if MAX_LIGHTS
	/*
	 * Iterate the hardware light sources, accumulating dynamic lighting for
	 * this fragment. A light radius of 0.0 means break.
	 */
	for (int i = 0; i < MAX_LIGHTS; i++) {

		if (LIGHTS.RADIUS[i] == 0.0)
			break;

		vec3 delta = LIGHTS.ORIGIN[i] - point;
		float len = length(delta);

		if (len < LIGHTS.RADIUS[i]) {

			float lambert = dot(normalmap, normalize(delta));
			if (lambert > 0.0) {

				// windowed inverse square falloff
				float dist = len/LIGHTS.RADIUS[i];
				float falloff = clamp(1.0 - dist * dist * dist * dist, 0.0, 1.0);
				falloff = falloff * falloff;
				falloff = falloff / (dist * dist + 1.0);

				light += LIGHTS.COLOR[i] * falloff * lambert;
			}
		}
	}

	light *= MODULATE_SCALE;
#endif

	// now modulate the diffuse sample with the modified lightmap
	float lightmapLuma = dot(lightmap.rgb, vec3(0.299, 0.587, 0.114));

	float blackPointLuma = 0.015625;
	float l = exp2(lightmapLuma) - blackPointLuma;
	float lightmapDiffuseBumpedLuma = l * lightmapBumpScale;
	float lightmapSpecularBumpedLuma = l * lightmapSpecularScale;

	vec3 diffuseLightmapColor = lightmap.rgb * lightmapDiffuseBumpedLuma;
	vec3 specularLightmapColor = (lightmapLuma + lightmap.rgb) * 0.5 * lightmapSpecularBumpedLuma;

	fragColor.rgb = diffuse.rgb * ((diffuseLightmapColor + specularLightmapColor) * MODULATE_SCALE + light);

	// lastly modulate the alpha channel by the color
	fragColor.a = diffuse.a * color.a;
}

/**
 * @brief Render caustics
 */
void CausticFragment(in vec3 lightmap) {
	if (CAUSTIC.ENABLE) {
		vec3 model_scale = vec3(0.024, 0.024, 0.016);
		float time_scale = 0.6;
		float caustic_thickness = 0.02;
		float caustic_glow = 8.0;
		float caustic_intensity = 0.3;

		// grab raw 3d noise
		float factor = noise3d((modelpoint * model_scale) + (TIME * time_scale));

		// scale to make very close to -1.0 to 1.0 based on observational data
		factor = factor * (0.3515 * 2.0);

		// make the inner edges stronger, clamp to 0-1
		factor = clamp(pow((1 - abs(factor)) + caustic_thickness, caustic_glow), 0, 1);

		// start off with simple color * 0-1
		vec3 caustic_out = CAUSTIC.COLOR * factor * caustic_intensity;

		// multiply caustic color by lightmap, clamping so it doesn't go pure black
		caustic_out *= clamp((lightmap * 1.6) - 0.5, 0.1, 1.0);

		// add it up
		fragColor.rgb = clamp(fragColor.rgb + caustic_out, 0.0, 1.0);
	}
}

/**
 * @brief Shader entry point.
 */
void main(void) {

	eyeDir = normalize(eye);

	// texture coordinates
	vec2 uvTextures = NORMALMAP ? BumpTexcoord() : texcoords[0];
	vec2 uvLightmap = texcoords[1];

	// flat shading
	vec3 lightmap = color.rgb;
	vec3 deluxemap = vec3(0.0, 0.0, 1.0);

	if (LIGHTMAP) {
		lightmap = texture(SAMPLER1, uvLightmap).rgb;

		if (STAINMAP) {
			vec4 stain = texture(SAMPLER8, uvLightmap);
			lightmap = mix(lightmap.rgb, stain.rgb, stain.a).rgb;
		}
	}

	// then resolve any bump mapping
	vec4 normalmap = vec4(normal, 1.0);

	float lightmapBumpScale = 1.0;
	float lightmapSpecularScale = 0.0;

	if (NORMALMAP) {

		if (DELUXEMAP) {
			deluxemap = texture(SAMPLER2, uvLightmap).rgb;
			deluxemap = normalize(two * (deluxemap + negHalf));
		}

		normalmap = texture(SAMPLER3, uvTextures);

		// scale by BUMP
		normalmap.xy = (normalmap.xy * 2.0 - 1.0) * BUMP;
		normalmap.xyz = normalize(normalmap.xyz);

		vec3 glossmap = vec3(0.5);

		if (GLOSSMAP) {
			glossmap = texture(SAMPLER4, uvTextures).rgb;
		} else if (DIFFUSE) {
			vec4 diffuse = texture(SAMPLER0, uvTextures);
			float processedGrayscaleDiffuse = dot(diffuse.rgb * diffuse.a, vec3(0.299, 0.587, 0.114)) * 0.875 + 0.125;
			float guessedGlossValue = clamp(pow(processedGrayscaleDiffuse * 3.0, 4.0), 0.0, 1.0) * 0.875 + 0.125;

			glossmap = vec3(guessedGlossValue);
		}

		// resolve the bumpmap modulation
		BumpFragment(deluxemap, normalmap.xyz, glossmap, lightmapBumpScale, lightmapSpecularScale);

		// and then transform the normalmap to model space for lighting
		normalmap.xyz = normalize(
			normalmap.x * normalize(tangent) +
			normalmap.y * normalize(bitangent) +
			normalmap.z * normalize(normal)
		);
	}

	vec4 diffuse = vec4(1.0);

	if (DIFFUSE) { // sample the diffuse texture, honoring the parallax offset
		diffuse = ColorFilter(texture(SAMPLER0, uvTextures));

		// see if diffuse can be discarded because of alpha test
		if (diffuse.a < ALPHA_THRESHOLD) {
			discard;
		}

		TintFragment(diffuse, uvTextures);
	}

	// add any dynamic lighting to yield the final fragment color
	LightFragment(diffuse, lightmap, normalmap.xyz, lightmapBumpScale, lightmapSpecularScale);

    // underliquid caustics
	CausticFragment(lightmap);

	// tonemap
	fragColor.rgb *= exp(fragColor.rgb);
	fragColor.rgb /= fragColor.rgb + 0.825;

	FogFragment(length(point), fragColor);
	
	DitherFragment(fragColor.rgb);
}
