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

class Glyph;
class StatusBar;

extern Glyph* infoarea;
extern Glyph* btarea;
extern Glyph* sbarea;

extern Subject<int> tb_index;
extern Subject<int> debug_view;
extern Subject<string> toolmsg;

// This module will build the three user interface glyphs (infoarea
// at the top, buttonbar at the left, statusbar at the bottom)
void build_glyphs( StatusBar* infoarea_,
                   StatusBar* buttonbar_,
                   StatusBar* statusbar_ );
