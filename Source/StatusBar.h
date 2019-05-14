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

#ifndef StatusBar_h
#define StatusBar_h

#include "BufferWin.h"
#include "Layer.h"
#include "TextGlyph.h"

class StatusBar : public BufferWindow
{
  public:
    StatusBar();
    virtual ~StatusBar();

    virtual void handle_mouse( int msg, int x, int y, unsigned short kbdflag );
    virtual void update();

    BackgroundLayer layer0;
    TextLayer layer1;
};

#endif
