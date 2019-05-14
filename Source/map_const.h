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

const int MAX_MOISTURE = 10;

// This should be a power of two
const int NUM_WATER_SOURCES = 8;

#define MONTHS_PER_YEAR 13
#define TICKS_PER_DAY 5
#define DAYS_PER_MONTH 28

const int TICKS_PER_MONTH = TICKS_PER_DAY * DAYS_PER_MONTH;
const int TICKS_PER_YEAR = TICKS_PER_MONTH * MONTHS_PER_YEAR;
const int DAYS_PER_YEAR = DAYS_PER_MONTH * MONTHS_PER_YEAR;

const int MIN_DESERT = (NUM_TERRAIN_TILES/5);
const int MAX_DESERT = (MIN_DESERT*3);

const int WATER_MULT = 6;    // How much water per unit of altitude
const int WALL_HEIGHT = 40;
const int CANAL_DEPTH = -22;
const int TREE_MATURITY = 5;
