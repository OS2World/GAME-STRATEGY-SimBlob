//
// Copyright (C) 1999 Amit J. Patel
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  Amit J. Patel makes no
// representations about the suitability of this software for any
// purpose.  It is provided "as is" without express or implied warranty.
//

#ifndef Images_h
#define Images_h

// These are sprites used by SimBlob.

// Num terrain tiles is used for two things, and really shouldn't be:
//   1.  It is used to determine how many images are used to draw the
//       terrain.  (There are actually 7 times as many because each
//       hex is divided into 7 hexes, for smoother maps)
//   2.  It is used as the internal representation of altitude.  The
//       altitudes can be from 0 to NUM_TERRAIN_TILES-1.
#define NUM_TERRAIN_TILES 128

// Num cursor frames is used for the animated cursor
#define NUM_CURSOR_FRAMES 12

// Num farms is used for the different farm tiles used in the various seasons
// In one game year, we go through all NUM_FARM tiles once
#define NUM_FARMS 16

// Num trees is used for the different tree tiles used for various ages
// The youngest tree uses 0 and the oldest tree uses NUM_TREES-1
#define NUM_TREES 14

// Num tree seasons is used for the seasons; in one game year we go
// through each tree season.
#define NUM_TREE_SEASONS 18

// Num angles is used for the train images, which aren't really being
// used in the game right now.  However, it may be a start for someone
// wanting to make a railroad-tycoonish game.
// *** Angles are defined with EAST being angle 0 and WEST being NUM_ANGLES
#define NUM_ANGLES 32

// These are the sizes and positions of the water bitmaps
const int WaterWidth = 14;
const int WaterHeight = 14;
const int WaterXOffset = 7;
const int WaterYOffset = 5;

void LoadBitmaps();

//////////////////////////////////////////////////////////////////////

#ifdef Sprites_h
// SimBlob sprites
extern Sprite* terrain_rle; // [NUM_TERRAIN_TILES*7];
extern Sprite* water_rle; // [64];
extern Sprite* house_rle; // [8];
extern Sprite* road_rle; // [64];
extern Sprite* wall_rle; // [64];
extern Sprite* overlay_rle; // [64];
extern Sprite* farm_rle; // [NUM_FARMS];
extern Sprite* sprinkles; // [16];
extern Sprite* cursor_rle; // [NUM_CURSOR_FRAMES];
extern Sprite* trains; // [NUM_ANGLES];
extern Sprite* trees_rle; // [NUM_TREES*NUM_TREE_SEASONS]

extern Sprite lone_tree_rle, tower_rle, canal_rle, gate_rle, clear_rle;
extern Sprite wallbmp_rle, bridge_rle, market_rle;
extern Sprite fire1_rle, fire2_rle, lava_rle, scorched_rle;
extern Sprite marktile_rle, tile_rle, soldier_rle, worker_rle, firefighter_rle;
extern Sprite background_rle, hand_rle;

extern Font text12;
extern Font text12i;
extern Font text24;
#endif

#endif
