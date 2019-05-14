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
#include "SimBlob.h"                    // Resource file symbolics

#include "Bitmaps.h"
#include "Sprites.h"
#include "Map.h"

#include "View.h"
#include "MsgWin.h"
#include "Menu.h"

#include "ViewWin.h"
#include "GameWin.h"
#include "MainWin.h"
#include "StatusBar.h"

#include "GlyphLib.h"
#include "Layout.h"

#include "MapCmd.h"
#include "Path.h"

#include "Ui.h"
#include "Images.h"
#include "Map_Const.h"

#pragma option -Od

bool GameWindow::bitmaps_initialized = false;

GameWindow::GameWindow()
    :view_thread_id(0), init_thread_id(0), simulate_thread_id(0),
     ps_window( NULL ), map( new Map ), viewwin_( NULL ), 
     statusbar_( NULL ), infoarea_( NULL ), buttonbar_( NULL ), 
     frame_( NULL ), palette( NULL ), horiz( NULL ), vert( NULL ),
     toolmsg_obs( toolmsg, closure(this,&GameWindow::notify_update) ),
     world_map_( NULL ), menu( NULL )
{
}

void GameWindow::setup_window( Window* parent )
{
    create_window( parent->handle(), PSZ(ClassName), WS_CLIPCHILDREN, 
                   FID_CLIENT, NULLHANDLE, this );

    ps_window.retarget( handle() );
    palette = new ColorPalette( ps_window );
    palette->create();
    palette->realize( handle() );

    horiz = new HorizScrollBar( FID_HORZSCROLL );
    horiz -> setup_window( this );
    vert = new VertScrollBar( FID_VERTSCROLL );
    vert -> setup_window( this );

    viewwin_ = new ViewWindow( this );
    viewwin_ -> setup_window( this );
    viewwin_ -> select_palette( palette );

    statusbar_ = new StatusBar;
    statusbar_ -> setup_window( this );
    statusbar_ -> select_palette( palette );
    
    world_map_ = new WorldMapWindow( map, viewwin_ );
    world_map_ -> setup_window( this );
    world_map_ -> select_palette( palette );

    infoarea_ = new StatusBar;
    infoarea_ -> setup_window( this );
    infoarea_ -> select_palette( palette );

    buttonbar_ = new StatusBar;
    buttonbar_ -> setup_window( this );
    buttonbar_ -> select_palette( palette );
    
    update_view_menu();

    menu.set_window( frame() );
    menu.toggle( ID_OPTIONS_DROUGHT, map->drought );

    menu.value( ID_SPEED_SLOWER, map->game_speed, 1 );
    menu.value( ID_SPEED_SLOW, map->game_speed, 5 );
    menu.value( ID_SPEED_MEDIUM, map->game_speed, 20 );
    menu.value( ID_SPEED_FAST, map->game_speed, 40 );
    menu.value( ID_SPEED_FASTER, map->game_speed, 120 );
    menu.value( ID_SPEED_FASTEST, map->game_speed, 500 );
    menu.value( ID_SPEED_ZOOM, map->game_speed, 1000 );

    menu.toggle( ID_SPEED_DETAILS, Sprite::ShowTransparent );
                
    const int debug_menu_ids[] = 
    { ID_DEBUG_NONE, ID_DEBUG_PREFS, ID_DEBUG_HEAT,
      ID_DEBUG_LABOR, ID_DEBUG_FOOD, ID_DEBUG_MOISTURE
    };
    const int NUM_DEBUGS = sizeof(debug_menu_ids)/sizeof(debug_menu_ids[0]);
    for( int i = 0; i < NUM_DEBUGS; i++ )
        menu.value( debug_menu_ids[i], debug_view, debug_menu_ids[i] );

    extern Subject<int> path_div;
    menu.value( ID_PATH_BEST, path_div, 8 );
    menu.value( ID_PATH_APPROX1, path_div, 6 );
    menu.value( ID_PATH_APPROX2, path_div, 4 );
    menu.value( ID_PATH_APPROX3, path_div, 3 );
    if(path_div != 6) DosBeep(100,100);

    menu.command( ID_SPECIAL_VOLCANO,
                  closure(this,&GameWindow::create_volcano) );
    menu.command( ID_SPECIAL_FLOOD, closure(this,&GameWindow::create_flood) );
    menu.command( ID_SPECIAL_FLASH_FLOOD,
                  closure(this,&GameWindow::create_flash_flood) );
    menu.command( ID_SPECIAL_RAIN, closure(this,&GameWindow::create_rain) );
    menu.command( ID_SPECIAL_FIRE, closure(this,&GameWindow::create_fire) );
    menu.command( ID_SPECIAL_ROADS,
                  closure(this,&GameWindow::create_road) );
    
    Closure<bool,int> erase_cmd = closure(this,&GameWindow::erase_map);
    menu.command( ID_SPECIAL_ERASE_ALL, erase_cmd );
    menu.command( ID_SPECIAL_ERASE_CIV, erase_cmd );
    menu.command( ID_SPECIAL_ERASE_ROADS, erase_cmd );
    menu.command( ID_SPECIAL_ERASE_WALLS, erase_cmd );
    menu.command( ID_SPECIAL_ERASE_CANALS, erase_cmd );
    menu.command( ID_SPECIAL_ERASE_UNIT, erase_cmd );
    
    //////////////////////////////////////////////////////////////////////
    build_glyphs( infoarea_, buttonbar_, statusbar_ );
    infoarea_->layer1.add(infoarea);
    buttonbar_->layer1.add(btarea);
    statusbar_->layer1.add(sbarea);
}

bool GameWindow::notify_update( const string& ) 
{
    if( infoarea_ && statusbar_ && buttonbar_ )
    {
        infoarea_->update();
        statusbar_->update();
        buttonbar_->update();
    }
    return true;
}

template <class T>
void KILL( T*& ptr ) { delete ptr; ptr = NULL; }

GameWindow::~GameWindow()
{
    Figment::kill_thread( view_thread_id );
    Figment::kill_thread( simulate_thread_id );
    Figment::kill_thread( init_thread_id );
    
    if( palette != NULL ) viewwin_->unselect_palette( palette );
    KILL(viewwin_);
    KILL(horiz);
    KILL(vert);
    KILL(statusbar_);
    KILL(infoarea_);
    KILL(buttonbar_);
    KILL(world_map_);
    KILL(map);
    KILL(palette);
}

Window* GameWindow::frame()
{
    if( frame_ != NULLHANDLE )
        return frame_;
    else
        return parent();
}

struct { bool enabled; int id; } view_options[] = 
                                 {
                                 { 1, ID_VIEW_ROADS },
                                 { 1, ID_VIEW_FIRE },
                                 { 1, ID_VIEW_BUILDINGS },
                                 { 1, ID_VIEW_STRUCTURES },
                                 { 1, ID_VIEW_WATER }
                                 };
int NUM_VIEW_OPTIONS = sizeof(view_options)/sizeof(view_options[0]);

void GameWindow::update_view_menu()
{
    HWND menu = WinWindowFromID( frame()->handle(), FID_MENU );
    bool all_on = true;
    bool all_off = true;
    for( int i = 0; i < NUM_VIEW_OPTIONS; i++ )
    {
        bool val = view_options[i].enabled;
        WinCheckMenuItem( menu, view_options[i].id, val );
        if( val ) all_off = false;
        if( !val ) all_on = false;
    }
    WinCheckMenuItem( menu, ID_VIEW_ALL_ON, all_on );
    WinCheckMenuItem( menu, ID_VIEW_ALL_OFF, all_off );
    viewwin_->request_full_paint();
    world_map_->update_now();
}

void GameWindow::evSize()
{
    WindowLayout& WL = *(static_cast<WindowLayout*>(NULL));

    // Everything to the right
    WindowLayout* right =
        WL.vert
        (
         WL.hstretchable( statusbar_, 1, 16 ),
         WL.horiz
         (
          WL.hstretchable( horiz, 1, 15 ),
          WL.space( 15, 15 )
          ),
         WL.horiz
         (
          WL.stretchable( viewwin_, 10, 10 ),
          WL.stretchable( vert, 15, 1, 0 )
          ),
         WL.hstretchable( buttonbar_, 1, 56 )
         );
    
    // Bottom to top:
    WindowLayout* left = 
        WL.vert
        ( 
         WL.vspace( 1 ),
         WL.fixed( world_map_, MAP_SIZE_X, MAP_SIZE_Y ),
         WL.vspace( 2 ),
         WL.stretchable( infoarea_, MAP_SIZE_X, 10, 0, 500 ),
         WL.vspace( 2 )
         );

    WindowLayout* layout =
        WL.horiz( left, 
                  WL.hspace( 2 ),
                  right );

    Size c, s;
    layout->request( c, s );
    layout->allocate( area() );

    // Now that we know where the info area is, lay its elements out
    infoarea->request( c, s );
    infoarea->allocate( infoarea_->area() );
    infoarea_->update();

    sbarea->request( c, s );
    sbarea->allocate( statusbar_->area() );
    statusbar_->update();
    
    btarea->request( c, s );
    btarea->allocate( buttonbar_->area() );
    buttonbar_->update();
    
    delete layout;
    world_map_->update_now();
}

void GameWindow::evPaint( PS& ps, const Rect& area )
{
    ps.fill_rect( area, CLR_PALEGRAY );
    // smallblob.bitblt( ps, MAP_SIZE_X-42, MAP_SIZE_Y+2, ROP_SRCCOPY, 0, 0, 40, 40 );
}

MRESULT GameWindow::WndProc( HWND hWnd, ULONG iMessage, MPARAM mParam1, MPARAM mParam2 )
{
    switch( iMessage )
    {
      case WM_REALIZEPALETTE:
      {
          if( palette->realize( handle() ) )
          {
              invalidate();
              statusbar_->invalidate();
              infoarea_->invalidate();
              buttonbar_->invalidate();
              world_map_->invalidate();
              viewwin_->invalidate();
          }
          return MRESULT(0);
      }

      case WM_HSCROLL:
      case WM_VSCROLL:
      case WM_CHAR:
          return WinSendMsg( viewwin_->handle(), iMessage, mParam1, mParam2 );

      case WM_ACTIVATE:
      {
          if( !SimBlobWindow::program_running || !bitmaps_initialized )
              break;

          bool active = bool(mParam1);        
          if( active )  viewwin_->activate();
          else          viewwin_->deactivate();

          break;
      }

      case WM_ERASEBACKGROUND:
      {
          return MRESULT(TRUE);
      }

      case WM_CLOSE:
      {
          DosSetPriority( PRTYS_THREAD, PRTYC_REGULAR, +15, 0 );
          WinStoreWindowPos( PSZ(AppName), PSZ(AppWinPos),
                             WinQueryWindow(handle(), QW_PARENT) );
          SimBlobWindow::program_running = false;
          (static_cast<FrameWindow*>(frame()))->quit_application();
          return 0;
      }
    }
    return CustomWindow::WndProc( hWnd, iMessage, mParam1, mParam2 );
}

Terrain GameWindow::current_terrain_draw()
{
    Terrain clicks[] = 
    { Clear, Road, Bridge, Canal, Wall, Gate, WatchFire, Fire, Trees };
    int index = tb_index;

    if( index < 0 || index > 7 ) index = 0;
    return clicks[index];
}

// Manual volcano creation is for debugging.
bool GameWindow::create_volcano( int )
{
    int m = 4+ShortRandom(Map::MSize-8);
    int n = 4+ShortRandom(Map::NSize-8);
    map->create_volcano( HexCoord(m,n) );
    world_map_->update_now();
    return true;
}

// Manual flood creation is for debugging.
bool GameWindow::create_flood(int)
{
    extern int flood_timing;
    extern bool flooding;
    if( !flooding )
    {
        Mutex::Lock lock( map->mutex );
        flood_timing = 0;
        world_map_->update_now();
    }
    return true;
}

// Manual flash flood creation is for debugging.
bool GameWindow::create_flash_flood(int)
{
    Mutex::Lock lock( map->mutex );
    for( int m = 1; m <= Map::MSize; ++m )
        for( int n = 1; n <= Map::NSize; ++n )
        {
            HexCoord h(m,n);
            map->set_water( h, map->water(h) * 3 );
        }
    world_map_->update_now();
    return true;
}

// Manual rain is for debugging.
bool GameWindow::create_rain(int)
{
    Mutex::Lock lock( map->mutex );
    for( int m = 1; m <= Map::MSize; ++m )
        for( int n = 1; n <= Map::NSize; ++n )
        {
            HexCoord h(m,n);
            map->set_water( h, map->water(h) + 3 );
        }
    world_map_->update_now();
    return true;
}

// Manual fire creation is for debugging.
bool GameWindow::create_fire(int)
{
    Mutex::Lock lock( map->mutex );
    for( int m = 1; m <= Map::MSize; ++m )
        for( int n = 1; n <= Map::NSize; ++n )
            if( ByteRandom(100) == 0 )
                map->set_terrain( HexCoord(m,n), Fire );
    world_map_->update_now();
    return true;
}

// Guess what?  Automatic road creation is just for debugging.
bool GameWindow::create_road(int)
{
    map_commands.push( Command::make_roads() );
    return true;
}

// Erase commands.. should these really be in the game?
bool GameWindow::erase_map( int id )
{
    Mutex::Lock lock( map->mutex );

    if( id == ID_SPECIAL_ERASE_ALL )
    {
        for( int m = 1; m <= Map::MSize; ++m )
            for( int n = 1; n <= Map::NSize; ++n )
            {
                HexCoord h(m,n);
                map->set_terrain( h, Clear );
                map->set_terrain( h, Clear );
            }
    }

    if( id == ID_SPECIAL_ERASE_CANALS )
    {
        for( int m = 1; m <= Map::MSize; ++m )
            for( int n = 1; n <= Map::NSize; ++n )
            {
                HexCoord h(m,n);
                if( !( map->flags_[h] & FLAG_EROSION ) )
                {
                    map->set_terrain( h, Clear );
                    map->set_terrain( h, Clear );
                }
            }
    }
    
    if( id == ID_SPECIAL_ERASE_WALLS )
    {
        for( int m = 1; m <= Map::MSize; ++m )
            for( int n = 1; n <= Map::NSize; ++n )
            {
                HexCoord h(m,n);
                Terrain t = map->terrain( h );
                if( t == Wall || t == Gate )
                    map->set_terrain( h, Clear );
            }
    }

    if( id == ID_SPECIAL_ERASE_CIV )
    {
        for( int m = 1; m <= Map::MSize; ++m )
            for( int n = 1; n <= Map::NSize; ++n )
            {
                HexCoord h(m,n);
                Terrain t = map->terrain( h );
                if( t == Houses || t == Farm || t == Market )
                    map->set_terrain( h, Clear );
            }
    }
            
    if( id == ID_SPECIAL_ERASE_ROADS )
    {
        for( int m = 1; m <= Map::MSize; ++m )
            for( int n = 1; n <= Map::NSize; ++n )
            {
                HexCoord h(m,n);
                Terrain t = map->terrain( h );
                if( t == Road || t == Bridge )
                    map->set_terrain( h, Clear );
            }
    }
            
    if( id == ID_SPECIAL_ERASE_ALL )
        map_commands.push( Command::erase_all() );
    
    if( id == ID_SPECIAL_ERASE_UNIT || id == ID_SPECIAL_ERASE_ALL )
    {
        Mutex::Lock lock( map->unit_mutex );
        for( vector<Unit*>::iterator u = map->units.begin();
             u != map->units.end(); ++u )
        {
            if( !(*u)->dead() ) (*u)->die( map );
        }
    }

    viewwin_->request_full_paint();
    world_map_->update_now();
    return true;
}

bool GameWindow::evCommand( USHORT command, HWND hWnd )
{
    if( menu.handle( command ) )
        return true;

#if 0
    // Ew, here's some code to make the train start moving around...
    // but the train is disabled
    if( command == ID_POPUP_MENU )
        train.begin_moving( map, HexCoord(1,1), HexCoord(Map::MSize-2,Map::NSize-2) );
#endif
    
    if( command == ID_HELP_ABOUT )
    {
        int r = WinDlgBox( HWND_DESKTOP,     // Place anywhere on desktop
                           handle(),         // Owned by frame
                           NULL,             // Address of dialog procedure
                           NULL,             // Module handle
                           ID_HELP_ABOUT,    // Dialog identifier in resource
                           NULL );           // Initialization data
        char s[255];
        sprintf(s,"Dialog Box code %d / %lx",r, WinGetLastError(Figment::hab));
        Log( "**", s );
    }

    for( int i = 0; i < NUM_VIEW_OPTIONS; i++ )
    {
        int flag = -1;

        if( command == ID_VIEW_ALL_ON ) flag = 1;
        else if( command == ID_VIEW_ALL_OFF ) flag = 0;
        else if( command == view_options[i].id ) 
            flag = !view_options[i].enabled;
        if( flag != -1 )
        {
            view_options[i].enabled = flag;
            update_view_menu();
        }
    }

    if( command == ID_EXITPROG )
    {
        (static_cast<FrameWindow*>(frame()))->close();
    }

    return CustomWindow::evCommand( command, hWnd );
}

