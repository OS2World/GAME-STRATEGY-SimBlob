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

#include <complex.h>

#include "Notion.h"
#include "Figment.h"

int solid( PS& ps, Color rgb )
{
    return GpiQueryNearestColor( ps, 0, rgb );
}

Mutex::Mutex()
{
    DosCreateMutexSem( NULL, &handle_, 0, FALSE );
}

Mutex::Mutex( const char* name )
{
    char tempname[256];
    strcpy( tempname, "\\sem32\\" );
    strcat( tempname, name );
    DosCreateMutexSem( tempname, &handle_, 0, FALSE );
}

Mutex::~Mutex()
{
    DosRequestMutexSem( handle_, 600 );
    DosCloseMutexSem( handle_ );
    handle_ = 0;
}

void Mutex::lock()
{
    WinRequestMutexSem( handle_, SEM_INDEFINITE_WAIT );
}

bool Mutex::lock( int timeout )
{
    return ( WinRequestMutexSem( handle_, timeout ) ) == 0;
}

void Mutex::unlock()
{
    DosReleaseMutexSem( handle_ );
}

EventSem::EventSem()
{
    DosCreateEventSem( PSZ(NULL), &handle_, 0L, FALSE );
}

EventSem::~EventSem()
{
    DosCloseEventSem( handle_ );
}

void EventSem::post()
{
    DosPostEventSem( handle_ );
}

void EventSem::reset()
{
    unsigned long count;
    DosResetEventSem( handle_, &count );
}

bool EventSem::wait( int timeout )
{
    return DosWaitEventSem( handle_, timeout ) == 0;
}

//////////////////////////////////////////////////////////////////////
// Regions
//    HRGN:  WinQueryVisibleRegion, GpiQueryRegionRects
//    I never implemented this completely; it's using bounding rect right now

Region::Region( PS& ps_ )
    :ps(ps_)
{
    hrgn = GpiCreateRegion( ps, 0, NULL );
}

Region::~Region()
{
    GpiDestroyRegion( ps, hrgn );
}

int Region::query_rects( int count, Rect rects[] ) const
{
    static RGNRECT rgn_ctl;
    rgn_ctl.ircStart = 0;
    rgn_ctl.crc = count;
    rgn_ctl.ulDirection = 1;
    if( GpiQueryRegionRects( ps, hrgn, NULL, &rgn_ctl, rects ) )
        return rgn_ctl.crcReturned;
    else
        return 0;
}

//////////////////////////////////////////////////////////////////////

DamageArea::DamageArea( const Rect& clipping_area )
    :clipping(clipping_area)
{
}

DamageArea::~DamageArea()
{
}

void DamageArea::operator += ( const Rect& r )
{
    Rect t = (clipping.width()>0)? intersect( r, clipping ) : r;
    if( t.width() > 0 )
        area = merge( area, t );
}

int DamageArea::num_rects() const
{
    if( area.width() > 0 || area.height() > 0 )
        return 1;
    else
        return 0;
}

Rect DamageArea::rect( int ) const
{
    return area;
}

//////////////////////////////////////////////////////////////////////
// Helper functions for mixing colors together
inline unsigned blend_one( unsigned a, unsigned b, int percentage )
{
    return ( a*(100-percentage) + b*percentage ) / 100;
}

unsigned blend( unsigned a, unsigned b, int percentage )
{
    unsigned ra = ( a >> 16 ) & 0xff , rb = ( b >> 16 ) & 0xff ;
    unsigned ga = ( a >> 8 ) & 0xff  , gb = ( b >> 8 ) & 0xff  ;
    unsigned ba = a & 0xff           , bb = b & 0xff           ;
    return iRGB( blend_one( ra, rb, percentage ),
                 blend_one( ga, gb, percentage ),
                 blend_one( ba, bb, percentage ) );
}
