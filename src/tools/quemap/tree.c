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

#include "tree.h"
#include "portal.h"
#include "qbsp.h"

/**
 * @brief
 */
node_t *AllocNode(void) {

	if (debug) {
		SDL_SemPost(semaphores.active_nodes);
	}

	return Mem_TagMalloc(sizeof(node_t), MEM_TAG_NODE);
}

/**
 * @brief
 */
void FreeNode(node_t *node) {

	if (debug) {
		SDL_SemWait(semaphores.active_nodes);
	}

	Mem_Free(node);
}

/**
 * @brief
 */
tree_t *AllocTree(void) {

	tree_t *tree = Mem_TagMalloc(sizeof(*tree), MEM_TAG_TREE);
	ClearBounds(tree->mins, tree->maxs);

	return tree;
}

/**
 * @brief
 */
void FreeTreePortals_r(node_t *node) {
	portal_t *p, *nextp;
	int32_t s;

	// free children
	if (node->plane_num != PLANENUM_LEAF) {
		FreeTreePortals_r(node->children[0]);
		FreeTreePortals_r(node->children[1]);
	}
	// free portals
	for (p = node->portals; p; p = nextp) {
		s = (p->nodes[1] == node);
		nextp = p->next[s];

		RemovePortalFromNode(p, p->nodes[!s]);
		FreePortal(p);
	}
	node->portals = NULL;
}

/**
 * @brief
 */
void FreeTree_r(node_t *node) {
	face_t *f, *nextf;

	// free children
	if (node->plane_num != PLANENUM_LEAF) {
		FreeTree_r(node->children[0]);
		FreeTree_r(node->children[1]);
	}
	// free bspbrushes
	FreeBrushes(node->brushes);

	// free faces
	for (f = node->faces; f; f = nextf) {
		nextf = f->next;
		FreeFace(f);
	}

	// free the node
	if (node->volume) {
		FreeBrush(node->volume);
	}

	FreeNode(node);
}

/**
 * @brief
 */
void FreeTree(tree_t *tree) {

	Com_Verbose("--- FreeTree ---\n");
	FreeTreePortals_r(tree->head_node);
	FreeTree_r(tree->head_node);
	Mem_Free(tree);
	Com_Verbose("--- FreeTree complete ---\n");
}

/**
 * @brief
 */
static void LeafNode(node_t *node, csg_brush_t *brushes) {

	node->plane_num = PLANENUM_LEAF;
	node->contents = 0;

	for (csg_brush_t *b = brushes; b; b = b->next) {
		// if the brush is solid and all of its sides are on nodes, it eats everything
		if (b->original->contents & CONTENTS_SOLID) {
			int32_t i;
			for (i = 0; i < b->num_sides; i++)
				if (b->sides[i].texinfo != TEXINFO_NODE) {
					break;
				}
			if (i == b->num_sides) {
				node->contents = CONTENTS_SOLID;
				break;
			}
		}
		node->contents |= b->original->contents;
	}

	node->brushes = brushes;
}

/**
 * @brief
 */
static void CheckPlaneAgainstParents(int32_t pnum, node_t *node) {

	for (node_t *p = node->parent; p; p = p->parent) {
		if (p->plane_num == pnum) {
			Com_Error(ERROR_FATAL, "Tried parent\n");
		}
	}
}

/**
 * @brief
 */
static _Bool CheckPlaneAgainstVolume(int32_t pnum, node_t *node) {
	csg_brush_t *front, *back;
	SplitBrush(node->volume, pnum, &front, &back);

	const _Bool good = (front && back);
	if (front) {
		FreeBrush(front);
	}
	if (back) {
		FreeBrush(back);
	}

	return good;
}

/**
 * @brief Using a heuristic, chooses one of the sides out of the brush list
 * to partition the brushes with.
 * Returns NULL if there are no valid planes to split with..
 */
static brush_side_t *SelectSplitSide(csg_brush_t *brushes, node_t *node) {
	int32_t value, best_value;
	csg_brush_t *brush, *test;
	brush_side_t *side, *best_side;
	int32_t i, j, pass, num_passes;
	int32_t pnum;
	int32_t s;
	int32_t front, back, both, facing, splits;
	int32_t bsplits;
	int32_t epsilon_brush;
	_Bool hint_split;

	best_side = NULL;
	best_value = -99999;

	// the search order goes: visible-structural, visible-detail,
	// nonvisible-structural, nonvisible-detail.
	// If any valid plane is available in a pass, no further
	// passes will be tried.
	num_passes = 4;
	for (pass = 0; pass < num_passes; pass++) {
		for (brush = brushes; brush; brush = brush->next) {
			if ((pass & 1) && !(brush->original->contents & CONTENTS_DETAIL)) {
				continue;
			}
			if (!(pass & 1) && (brush->original->contents & CONTENTS_DETAIL)) {
				continue;
			}
			for (i = 0; i < brush->num_sides; i++) {
				side = brush->sides + i;
				if (side->bevel) {
					continue;    // never use a bevel as a splitter
				}
				if (!side->winding) {
					continue;    // nothing visible, so it can't split
				}
				if (side->texinfo == TEXINFO_NODE) {
					continue;    // already a node splitter
				}
				if (side->tested) {
					continue;    // we already have metrics for this plane
				}
				if (side->surf & SURF_SKIP) {
					continue;    // skip surfaces are never chosen
				}
				if (side->visible ^ (pass < 2)) {
					continue;    // only check visible faces on first pass
				}

				pnum = side->plane_num;
				pnum &= ~1; // always use positive facing plane

				CheckPlaneAgainstParents(pnum, node);

				if (!CheckPlaneAgainstVolume(pnum, node)) {
					continue; // would produce a tiny volume
				}

				front = 0;
				back = 0;
				both = 0;
				facing = 0;
				splits = 0;
				epsilon_brush = 0;
				hint_split = false;

				for (test = brushes; test; test = test->next) {
					s = TestBrushToPlane(test, pnum, &bsplits, &hint_split, &epsilon_brush);

					splits += bsplits;
					if (bsplits && (s & SIDE_FACING)) {
						Com_Error(ERROR_FATAL, "SIDE_FACING with splits\n");
					}

					test->test_side = s;
					// if the brush shares this face, don't bother
					// testing that facenum as a splitter again
					if (s & SIDE_FACING) {
						facing++;
						for (j = 0; j < test->num_sides; j++) {
							if ((test->sides[j].plane_num & ~1) == pnum) {
								test->sides[j].tested = true;
							}
						}
					}
					if (s & SIDE_FRONT) {
						front++;
					}
					if (s & SIDE_BACK) {
						back++;
					}
					if (s == SIDE_BOTH) {
						both++;
					}
				}

				// give a value estimate for using this plane

				value = 5 * facing - 5 * splits - abs(front - back);
				if (AXIAL(&planes[pnum])) {
					value += 5;    // axial is better
				}
				value -= epsilon_brush * 1000; // avoid!

				// never split a hint side except with another hint
				if (hint_split && !(side->surf & SURF_HINT)) {
					value = -9999999;
				}

				// save off the side test so we don't need
				// to recalculate it when we actually seperate
				// the brushes
				if (value > best_value) {
					best_value = value;
					best_side = side;
					for (test = brushes; test; test = test->next) {
						test->side = test->test_side;
					}
				}
			}
		}

		// if we found a good plane, don't bother trying any other passes
		if (best_side) {
			if (pass > 1) {
				if (debug) {
					SDL_SemPost(semaphores.nonvis_nodes);
				}
			}
			if (pass > 0) {
				node->detail_seperator = true;    // not needed for vis
			}
			break;
		}
	}

	// clear all the tested flags we set
	for (brush = brushes; brush; brush = brush->next) {
		for (i = 0; i < brush->num_sides; i++) {
			brush->sides[i].tested = false;
		}
	}

	return best_side;
}

/**
 * @brief
 */
static void SplitBrushes(csg_brush_t *brushes, node_t *node, csg_brush_t **front, csg_brush_t **back) {
	csg_brush_t *brush, *front_brush, *back_brush;
	brush_side_t *side;

	*front = *back = NULL;

	for (brush = brushes; brush; brush = brush->next) {

		const int32_t sides = brush->side;
		if (sides == SIDE_BOTH) { // split into two brushes
			SplitBrush(brush, node->plane_num, &front_brush, &back_brush);
			if (front_brush) {
				front_brush->next = *front;
				*front = front_brush;
			}
			if (back_brush) {
				back_brush->next = *back;
				*back = back_brush;
			}
			continue;
		}

		csg_brush_t *new_brush = CopyBrush(brush);

		// if the plane_num is actualy a part of the brush
		// find the plane and flag it as used so it won't be tried as a splitter again
		if (sides & SIDE_FACING) {
			for (int32_t i = 0; i < new_brush->num_sides; i++) {
				side = new_brush->sides + i;
				if ((side->plane_num & ~1) == node->plane_num) {
					side->texinfo = TEXINFO_NODE;
				}
			}
		}

		if (sides & SIDE_FRONT) {
			new_brush->next = *front;
			*front = new_brush;
			continue;
		}
		if (sides & SIDE_BACK) {
			new_brush->next = *back;
			*back = new_brush;
			continue;
		}
	}
}


/**
 * @brief
 */
static node_t *BuildTree_r(node_t *node, csg_brush_t *brushes) {
	csg_brush_t *children[2];

	if (debug) {
		SDL_SemPost(semaphores.vis_nodes);
	}

	// find the best plane to use as a splitter
	brush_side_t *split_side = SelectSplitSide(brushes, node);
	if (!split_side) {
		// leaf node
		node->side = NULL;
		node->plane_num = -1;
		LeafNode(node, brushes);
		return node;
	}

	// this is a splitplane node
	node->side = split_side;
	node->plane_num = split_side->plane_num & ~1; // always use front facing

	SplitBrushes(brushes, node, &children[0], &children[1]);
	FreeBrushes(brushes);

	// allocate children before recursing
	for (int32_t i = 0; i < 2; i++) {
		node_t *child = AllocNode();
		child->parent = node;
		node->children[i] = child;
	}

	SplitBrush(node->volume, node->plane_num, &node->children[0]->volume, &node->children[1]->volume);

	// recursively process children
	for (int32_t i = 0; i < 2; i++) {
		node->children[i] = BuildTree_r(node->children[i], children[i]);
	}

	return node;
}

/**
 * @brief
 * @remark The incoming list will be freed before exiting
 */
tree_t *BuildTree(csg_brush_t *brushes, const vec3_t mins, const vec3_t maxs) {

	Com_Debug(DEBUG_ALL, "--- BuildTree ---\n");

	tree_t *tree = AllocTree();

	int32_t c_brushes = 0;
	int32_t c_faces = 0;
	int32_t c_nonvisfaces = 0;

	for (csg_brush_t *b = brushes; b; b = b->next) {
		c_brushes++;

		const vec_t volume = BrushVolume(b);
		if (volume < micro_volume) {
			Mon_SendSelect(MON_WARN, b->original->entity_num, b->original->brush_num, "Microbrush");
		}

		for (int32_t i = 0; i < b->num_sides; i++) {
			if (b->sides[i].bevel) {
				continue;
			}
			if (!b->sides[i].winding) {
				continue;
			}
			if (b->sides[i].texinfo == TEXINFO_NODE) {
				continue;
			}
			if (b->sides[i].visible) {
				c_faces++;
			} else {
				c_nonvisfaces++;
			}
		}

		AddPointToBounds(b->mins, tree->mins, tree->maxs);
		AddPointToBounds(b->maxs, tree->mins, tree->maxs);
	}

	Com_Debug(DEBUG_ALL, "%5i brushes\n", c_brushes);
	Com_Debug(DEBUG_ALL, "%5i visible faces\n", c_faces);
	Com_Debug(DEBUG_ALL, "%5i nonvisible faces\n", c_nonvisfaces);

	SDL_DestroySemaphore(semaphores.vis_nodes);
	semaphores.vis_nodes = SDL_CreateSemaphore(0);

	SDL_DestroySemaphore(semaphores.nonvis_nodes);
	semaphores.nonvis_nodes = SDL_CreateSemaphore(0);

	node_t *node = AllocNode();

	node->volume = BrushFromBounds(mins, maxs);

	tree->head_node = node;

	node = BuildTree_r(node, brushes);

	const uint32_t vis_nodes = SDL_SemValue(semaphores.vis_nodes);
	const uint32_t nonvis_nodes = SDL_SemValue(semaphores.nonvis_nodes);

	Com_Debug(DEBUG_ALL, "%5i visible nodes\n", vis_nodes / 2 - nonvis_nodes);
	Com_Debug(DEBUG_ALL, "%5i nonvis nodes\n", nonvis_nodes);
	Com_Debug(DEBUG_ALL, "%5i leafs\n", (vis_nodes + 1) / 2);

	return tree;
}

/*
 *
 * NODES THAT DON'T SEPERATE DIFFERENT CONTENTS CAN BE PRUNED
 *
 */

static int32_t c_pruned;

/**
 * @brief
 */
void PruneNodes_r(node_t *node) {
	csg_brush_t *b, *next;

	if (node->plane_num == PLANENUM_LEAF) {
		return;
	}
	PruneNodes_r(node->children[0]);
	PruneNodes_r(node->children[1]);

	if ((node->children[0]->contents & CONTENTS_SOLID) && (node->children[1]->contents
	        & CONTENTS_SOLID)) {
		if (node->faces) {
			Com_Error(ERROR_FATAL, "Node faces separating CONTENTS_SOLID\n");
		}
		if (node->children[0]->faces || node->children[1]->faces) {
			Com_Error(ERROR_FATAL, "Node has no faces but children do\n");
		}

		// FIXME: free stuff
		node->plane_num = PLANENUM_LEAF;
		node->contents = CONTENTS_SOLID;
		node->detail_seperator = false;

		if (node->brushes) {
			Com_Error(ERROR_FATAL, "Node still references brushes\n");
		}

		// combine brush lists
		node->brushes = node->children[1]->brushes;

		for (b = node->children[0]->brushes; b; b = next) {
			next = b->next;
			b->next = node->brushes;
			node->brushes = b;
		}

		c_pruned++;
	}
}

void PruneNodes(node_t *node) {
	Com_Verbose("--- PruneNodes ---\n");
	c_pruned = 0;
	PruneNodes_r(node);
	Com_Verbose("%5i pruned nodes\n", c_pruned);
}

/**
 * @brief
 */
void MergeNodeFaces(node_t *node) {

	const plane_t *plane = &planes[node->plane_num];

	for (face_t *f1 = node->faces; f1; f1 = f1->next) {
		if (f1->merged) {
			continue;
		}
		for (face_t *f2 = node->faces; f2 != f1; f2 = f2->next) {
			if (f2->merged) {
				continue;
			}
			face_t *merged = MergeFaces(f1, f2, plane->normal);
			if (!merged) {
				continue;
			}

			// add merged to the end of the node face list
			// so it will be checked against all the faces again
			face_t *last = node->faces;
			while (last->next) {
				last = last->next;
			}

			merged->next = NULL;
			last->next = merged;
			break;
		}
	}
}
