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

#include "color.h"
#include "swap.h"
#include "shared.h"

/**
 * @brief
 */
color_t Color3b(byte r, byte g, byte b) {
	return Color4b(r, g, b, 255);
}

/**
 * @brief
 */
color_t Color3bv(uint32_t rgb) {
	return Color4bv((rgb << 8) | 0x000000ff);
}

/**
 * @brief
 */
color_t Color3f(float r, float g, float b) {
	return Color4f(r, g, b, 1.f);
}

/**
 * @brief
 */
color_t Color3fv(const vec3_t rgb) {
	return Color3f(rgb.x, rgb.y, rgb.z);
}

/**
 * @brief
 */
color_t Color4b(byte r, byte g, byte b, byte a) {
	return (color_t) {
		.r = r / 255.f,
		.g = g / 255.f,
		.b = b / 255.f,
		.a = a / 255.f
	};
}

/**
 * @brief
 */
color_t Color4bv(uint32_t rgba) {
	union {
		uint32_t rgba;
		byte bytes[4];
	} c = {
		BigLong(rgba)
	};
	return Color4b(c.bytes[0], c.bytes[1], c.bytes[2], c.bytes[3]);
}

/**
 * @brief
 */
color_t Color4f(float r, float g, float b, float a) {

	const float max = Maxf(r, Maxf(g, b));
	if (max > 1.f) {
		const float inverse = 1.f / max;
		r *= inverse;
		g *= inverse;
		b *= inverse;
	}

	return (color_t) {
		.r = Clampf(r, 0.f, 1.f),
		.g = Clampf(g, 0.f, 1.f),
		.b = Clampf(b, 0.f, 1.f),
		.a = Clampf(a, 0.f, 1.f)
	};
}

/**
 * @brief
 */
color_t Color4fv(const vec4_t rgba) {
	return Color4f(rgba.x, rgba.y, rgba.z, rgba.w);
}

/**
 * @brief
 */
color_t ColorHSL(float hue, float saturation, float lightness) {
	
	hue = ClampEuler(hue);

	const float chroma = (1.f - fabs((2.f * lightness) - 1.f)) * saturation;
	const uint8_t huePrime = (uint8_t)(hue / 60.0f);
	const float secondComponent = chroma * (1.f - fabs((huePrime % 2) - 1.f));

	float red = 0, green = 0, blue = 0;

	switch (huePrime) {
		case 0:
			red = chroma;
			green = secondComponent;
			break;
		case 1:
			red = secondComponent;
			green = chroma;
			break;
		case 2:
			green = chroma;
			blue = secondComponent;
			break;
		case 3:
			green = secondComponent;
			blue = chroma;
			break;
		case 4:
			red = secondComponent;
			blue = chroma;
			break;
		case 5:
			red = chroma;
			blue = secondComponent;
			break;
	}

	const float lightnessAdjustment = lightness - (chroma / 2.f);

	return Color3f(
		red + lightnessAdjustment,
		green + lightnessAdjustment,
		blue + lightnessAdjustment
	);
}

/**
 * @brief
 */
color_t Color_Add(const color_t a, const color_t b) {
	return Color4fv(Vec4_Add(Color_Vec4(a), Color_Vec4(b)));
}

/**
 * @brief
 */
color_t Color_Mix(const color_t a, const color_t b, float mix) {
	return Color4fv(Vec4_Mix(Color_Vec4(a), Color_Vec4(b), mix));
}

/**
 * @brief
 */
color_t Color_Subtract(const color_t a, const color_t b) {
	return Color4fv(Vec4_Subtract(Color_Vec4(a), Color_Vec4(b)));
}

/**
 * @brief
 */
color_t Color_Scale(const color_t a, const float b) {
	return Color4fv(Vec4_Scale(Color_Vec4(a), b));
}

/**
 * @brief
 */
vec3_t Color_Vec3(const color_t color) {
	return Vec3(color.r, color.g, color.b);
}

/**
 * @brief
 */
vec4_t Color_Vec4(const color_t color) {
	return Vec4(color.r, color.g, color.b, color.a);
}

/**
 * @brief
 */
uint32_t Color_Rgba(const color_t color) {
	union {
		byte bytes[4];
		uint32_t rgba;
	} c = {
		{ color.r * 255.f, color.g * 255.f, color.b * 255.f, color.a * 255.f }
	};
	return c.rgba;
}

/**
 * @brief Attempt to convert a hexadecimal value to its string representation.
 */
_Bool ColorHex(const char *s, color_t *color) {

	const size_t length = strlen(s);
	if (length != 6 && length != 8) {
		return false;
	}

	char buffer[9];
	g_strlcpy(buffer, s, sizeof(buffer));

	if (length == 6) {
		g_strlcat(buffer, "ff", sizeof(buffer));
	}

	uint32_t rgba;
	if (sscanf(buffer, "%x", &rgba) != 1) {
		return false;
	}

	*color = Color4bv(rgba);
	return true;
}

/**
 * @brief
 */
const char *Color_ToHex(const color_t color) {
	union {
		uint32_t rgba;
		byte bytes[4];
	} c = {
		Color_Rgba(color)
	};
	return va("%02x%02x%02x%02x", c.bytes[0], c.bytes[1], c.bytes[2], c.bytes[3]);
}