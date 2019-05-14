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

#ifndef GameWin_h
#define GameWin_h

#include "Sprites.h"
#include "Palette.h"
#include "BufferWin.h"
#include "Menu.h"

class ViewWindow;
class Blitter;

class WorldMapWindow: public BufferWindow
{
    Map* map;
    ViewWindow* view;
    
    PixelBuffer *pb_terrain;
    bool create_terrain_map();  // true if map was (re)created

    bool fire_on_map;
    bool mousedown;
  public:
    enum { VIEW_NORMAL, VIEW_CIV, VIEW_TERRAIN } view_type;
    
    WorldMapWindow( Map* map, ViewWindow* view );
    ~WorldMapWindow();
    
    bool evCommand( USHORT command, HWND hWnd );
    MRESULT WndProc( HWND, ULONG, MPARAM, MPARAM );
    virtual void update();
    virtual void draw_normal( int left, int bottom, int right, int top );
    virtual void draw_shaded( int left, int bottom, int right, int top );
    virtual void draw_civilization( int left, int bottom, int right, int top );
    void update_now();  // Force a big update
};

class StatusBar;
class GameWindow: public CustomWindow
{
  public:
    static bool bitmaps_initialized;

    GameWindow();
    virtual ~GameWindow();

    virtual void setup_window( Window* parent );

    Map* map;
    CachedPS ps_window;
    ColorPalette* palette;

    TID view_thread_id;
    TID init_thread_id;
    TID simulate_thread_id;
    int update_thread( int );

    void update_build_menu();
    void update_debug_menu();
    void update_path_menu();
    void update_view_menu();
    Terrain current_terrain_draw();
    
    bool initialization_message( const char* );
    int initialization_thread(int);

    bool create_volcano(int);
    bool create_flood(int);
    bool create_flash_flood(int);
    bool create_rain(int);
    bool create_fire(int);
    bool create_road(int);
    
    bool erase_map(int);
    
    virtual bool evCommand( USHORT command, HWND hWnd );
    virtual void evSize();
    virtual void evPaint( PS&, const Rect& );

    virtual MRESULT WndProc( HWND hWnd, ULONG iMessage, MPARAM, MPARAM );

    Window* frame();

    WorldMapWindow* world_map_;
    StatusBar* statusbar_;
    StatusBar* infoarea_;
    StatusBar* buttonbar_;
    ScrollBar* horiz;
    ScrollBar* vert;
  protected:
    Observer<string> toolmsg_obs;
    bool notify_update( const string& );
    Window* frame_;
    ViewWindow* viewwin_;
    Menu menu;
};

#endif
