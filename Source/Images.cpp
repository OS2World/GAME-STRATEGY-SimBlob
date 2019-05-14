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

#include "std.h"

#include "Figment.h"                    // Figment class library
#include "Bitmaps.h"
#include "Sprites.h"
#include "Images.h"
#include "Initcode.h"

Sprite* terrain_rle = new Sprite[NUM_TERRAIN_TILES*7];
Sprite* water_rle = new Sprite [64];
Sprite* cursor_rle = new Sprite[NUM_CURSOR_FRAMES];
Sprite* house_rle = new Sprite[8];
Sprite* road_rle = new Sprite[64];
Sprite* wall_rle = new Sprite[64];
Sprite* overlay_rle = new Sprite[64];
Sprite* sprinkles = new Sprite[16];
Sprite* farm_rle = new Sprite[NUM_FARMS];
Sprite* trains = new Sprite[NUM_ANGLES];
Sprite* trees_rle = new Sprite[NUM_TREES*NUM_TREE_SEASONS];
Sprite lone_tree_rle, tower_rle, canal_rle, gate_rle, clear_rle, wallbmp_rle;
Sprite fire1_rle, fire2_rle, lava_rle, scorched_rle, bridge_rle;
Sprite marktile_rle, tile_rle, soldier_rle, worker_rle, firefighter_rle;
Sprite background_rle, hand_rle, market_rle;

Font text12("Hershey 12");
Font text12i("Hershey 12 Italic");
Font text24("Text 24");
