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

#include "Figment.h"

//////////////////////////////////////////////////////////////////////
// Palette and Transparency Lookup Tables
// I used to include this in the source but now I read it from Data/RGBTable
// #include "RGBTable.inc"  

unsigned long* DesktopRGB = new unsigned long[256];
byte* Transparency1 = new byte[65536];
