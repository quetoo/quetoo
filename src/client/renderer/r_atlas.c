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

#include "r_local.h"

/**
 * @brief Free event listener for atlases.
 */
static void R_FreeAtlas(r_media_t *media) {
	r_atlas_t *atlas = (r_atlas_t *) media;

	for (size_t i = 0; i < atlas->atlas->nodes->len; i++) {
		atlas_node_t *node = g_ptr_array_index(atlas->atlas->nodes, i);

		for (int32_t layer = 0; layer < atlas->atlas->layers; layer++) {
			SDL_FreeSurface(node->surfaces[layer]);
		}
	}

	Atlas_Destroy(atlas->atlas);
}

/**
 * @brief Creates a blank state for an atlas and returns it.
 */
r_atlas_t *R_CreateAtlas(const char *name) {

	r_atlas_t *atlas = (r_atlas_t *) R_AllocMedia(name, sizeof(r_atlas_t), MEDIA_ATLAS);

	atlas->media.Free = R_FreeAtlas;
	atlas->image = (r_image_t *) R_AllocMedia(va("%s image", atlas->media.name), sizeof(r_image_t), MEDIA_IMAGE);
	atlas->image->type = IT_ATLAS;

	R_RegisterDependency((r_media_t *) atlas, (r_media_t *) atlas->image);

	atlas->atlas = Atlas_Create(1);
	return atlas;
}

/**
 * @brief Add an image to the list of images for this atlas.
 */
r_atlas_image_t *R_LoadAtlasImage(r_atlas_t *atlas, const char *name, r_image_type_t type) {
	static int32_t pixels = 0xff0000ff;

	r_atlas_image_t *atlas_image = (r_atlas_image_t *) R_FindMedia(name);
	if (!atlas_image) {

		atlas_image = (r_atlas_image_t *) R_AllocMedia(name, sizeof(*atlas_image), MEDIA_ATLAS_IMAGE);
		assert(atlas_image);

		SDL_Surface *surf = Img_LoadSurface(name);
		if (!surf) {
			surf = SDL_CreateRGBSurfaceWithFormatFrom(&pixels, 1, 1, 32, 4, SDL_PIXELFORMAT_RGBA32);
		}

		atlas_node_t *node = Atlas_Insert(atlas->atlas, surf);
		assert(node);

		atlas_image->image.type = type;
		atlas_image->image.width = surf->w;
		atlas_image->image.height = surf->h;

		R_RegisterDependency((r_media_t *) atlas_image, (r_media_t *) atlas);

		node->data = atlas_image;
	}

	return atlas_image;
}

/**
 * @brief
 */
static void R_CompileAtlas_Node(gpointer data, gpointer user_data) {

	const atlas_node_t *node = data;
	const r_atlas_t *atlas = user_data;

	r_atlas_image_t *atlas_image = node->data;

	atlas_image->image.texnum = atlas->image->texnum;

	const float w = atlas->image->width, h = atlas->image->height;

	atlas_image->texcoords.x = node->x / w;
	atlas_image->texcoords.y = node->y / h;
	atlas_image->texcoords.z = (node->x + atlas_image->image.width) / w;
	atlas_image->texcoords.w = (node->y + atlas_image->image.height) / h;
}

/**
 * @brief Compiles the specified atlas.
 */
void R_CompileAtlas(r_atlas_t *atlas) {

	atlas->image->width = 0;

	for (int32_t w = 1024; atlas->image->width == 0; w <<= 1) {

		if (w > r_config.max_texture_size) {
			Com_Error(ERROR_DROP, "Atlas exceeds GL_MAX_TEXTURE_SIZE\n");
		}

		SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormat(0, w, w, 32, SDL_PIXELFORMAT_RGBA32);
		if (Atlas_Compile(atlas->atlas, 0, surf) == 0) {

			atlas->image->width = w;
			atlas->image->height = w;

			R_UploadImage(atlas->image, GL_RGBA, surf->pixels);

			g_ptr_array_foreach(atlas->atlas->nodes, R_CompileAtlas_Node, atlas);
		}

		SDL_FreeSurface(surf);
	}

	R_RegisterMedia((r_media_t *) atlas);
}
