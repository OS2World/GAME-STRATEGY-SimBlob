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

#ifndef Tools_h
#define Tools_h

class GameWindow;

class Tool
{
  public:
    Tool();
    virtual ~Tool();

    // Tool operations
    virtual void activate();
    virtual void deactivate();

    // Mouse operations
    virtual void select( Point ) {}
    virtual void contextmenu( Point ) {}
    virtual void open( Point ) {}

    virtual void enter() {}
	virtual void leave() {}
	
    virtual void begin_swipe( Point ) {}
    virtual void swipe( Point ) {}
    virtual void end_swipe( Point ) {}
    virtual void cancel_swipe() {}
    
    virtual void begin_drag( Point ) {}
    virtual void drag( Point ) {}
    virtual void end_drag( Point ) {}
    virtual void cancel_drag() {}
    
    // Return true if the cursor should be shown at point p
    virtual bool validity( Point ) { return true; } 
    
  public:
    static unsigned kbdflags;
    static Tool* tools[];
    static int num_tools;
};

class BuildTool: public Tool
{
  public:
    BuildTool( GameWindow* gw, Terrain t );

    virtual void select( Point p );
    virtual void begin_swipe( Point p );
    virtual void swipe( Point p );
    virtual void end_swipe( Point p );
    virtual void cancel_swipe();

    virtual bool validity( Point p );
    
  protected:
    bool valid();
    bool valid( HexCoord h );
    
    GameWindow* gamewin;
    Map* map;
    HexCoord origin, current;
    bool linedraw, default_linedraw;

  public:
    Terrain terrain;
};

class BlobTool: public Tool
{
  public:
    BlobTool( Map* m ): map(m), blob(NULL) {}

    virtual void select( Point p );
    virtual void begin_drag( Point p );
    virtual void end_drag( Point p );
    virtual void cancel_drag();
    
    virtual bool validity( Point p );
    
  protected:
    Map* map;
    Unit* blob;
};

class SelectTool: public Tool
{
  public:
    SelectTool( Map* m, View* v ): map(m), view(v) {}

    virtual void activate();
    virtual void deactivate();
	virtual void leave();
    virtual bool validity( Point p );

  protected:
    Map* map;
    View* view;
};

#endif
