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
#include "Map.h"
#include "MapCmd.h"
#include "GameWin.h"

#include "Tools.h"

unsigned Tool::kbdflags = 0;

Tool::Tool()
{
}

Tool::~Tool()
{
}

void Tool::activate()
{
}

void Tool::deactivate()
{
}

//////////////////////////////////////////////////////////////////////

BuildTool::BuildTool( GameWindow* gw, Terrain t )
    :gamewin(gw), map(gw->map), terrain(t)
{
    if( t == Road || t == Bridge || t == Wall || t == Canal )
        default_linedraw = true;
    else
        default_linedraw = false;

    linedraw = default_linedraw;
}

inline bool BuildTool::valid( HexCoord h )
{
    return map->valid( h );
}

inline bool BuildTool::valid()
{
    return map->valid( current );
}

bool BuildTool::validity( Point p )
{
    static const char* firemsg = "Can't erase fires";
    static const char* emptymsg = "";
    extern Subject<string> toolmsg;
    HexCoord h = PointToHex( p.x, p.y );

    if( !valid(h) ) return false;
        
    if( terrain != Fire )
    {
        if( map->terrain(h) == Fire )
        {
            set( toolmsg, firemsg );
            return false;
        }

        if( linedraw )
        {
            for( vector<HexCoord>::iterator j = map->selected.begin();
                 j != map->selected.end(); ++j )
                if( map->terrain( *j ) == Fire )
                {
                    set( toolmsg, firemsg );
                    return false;
                }
        }
    }

    set( toolmsg, emptymsg );
    return true;
}

void BuildTool::select( Point p )
{
    current = PointToHex( p.x, p.y );
    if( valid() )
    {
        map_commands.push( Command::build(terrain, current) );
    }
}

void BuildTool::swipe( Point p )
{
    HexCoord h = PointToHex( p.x, p.y );
    if( valid(origin) && h != current )
    {
        current = h;
        if( !linedraw ) select(p);
        else
        {
            if( valid() )
                map->set_end( current );
            else
                map->cancel_selection();
        }
    }
}

void BuildTool::begin_swipe( Point p )
{
    current = HexCoord(0,0);
    origin = PointToHex( p.x, p.y );
    linedraw = default_linedraw;
    if( kbdflags & KC_SHIFT ) linedraw = !linedraw;
    if( valid(origin) )
    {
        if( linedraw ) map->set_begin( origin );
        swipe(p);
    }
}

void BuildTool::end_swipe( Point p )
{
    swipe(p);
    if( linedraw && valid() && valid(origin) )
    {
        map->set_end( current );

        for( int i = 0; i < 10; i++ )
        {
            Mutex::Lock lock( map->selection_mutex, 200 );
            if( lock.locked() )
            {
                for( vector<HexCoord>::iterator j = map->selected.begin(); 
                     j != map->selected.end(); ++j )
                {
                    map_commands.push( Command::build( terrain, *j ) );
                }
                break;
            }
        }
    }
    cancel_swipe();
}

void BuildTool::cancel_swipe()
{
    origin = HexCoord(0,0);
    map->cancel_selection();
}

//////////////////////////////////////////////////////////////////////

void BlobTool::select( Point p )
{
    // What should we do when LMB clicking on a blob?
    #if 0
    HexCoord h = PointToHex( p.x, p.y );
    if( map->valid(h) )
        ;
    #endif
}

void BlobTool::begin_drag( Point p )
{
    // Dragging a blob will tell him to move somewhere
    HexCoord h = PointToHex( p.x, p.y );
    if( map->valid(h) )
    {
        int i = map->occupied_[h];
        blob = ( i != -1 )? (map->units[i]) : NULL;
    }   
}

void BlobTool::end_drag( Point p )
{
    HexCoord h = PointToHex( p.x, p.y );
    if( map->valid(h) && blob != NULL )
    {
        blob->set_dest( map, h );
    }
    cancel_drag();
}

void BlobTool::cancel_drag()
{
    blob = NULL;
}

bool BlobTool::validity( Point p )
{
    HexCoord h = PointToHex( p.x, p.y );
    return map->valid(h) && map->occupied_[h] != -1;
}

//////////////////////////////////////////////////////////////////////

Point last_identify;
void SelectTool::activate()
{
    last_identify = Point(-1,-1);
}

void SelectTool::deactivate()
{
    last_identify = Point(-1,-1);
}

void SelectTool::leave()
{
    last_identify = Point(-1,-1);
}

bool SelectTool::validity( Point p )
{
    last_identify = p;
    return false;
}
