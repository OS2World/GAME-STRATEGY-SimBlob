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

#ifndef Notion_h
#define Notion_h

#include "Types.h"

#define pi (3.14159265358979323846)

class PS;
inline Color iRGB( unsigned r, unsigned g, unsigned b ) { return (r<<16) | (g<<8) | b; }
inline int ColorToRed( Color c ) { return ( c >> 16 ) & 0xff; }
inline int ColorToGreen( Color c ) { return ( c >> 8 ) & 0xff; }
inline int ColorToBlue( Color c ) { return c & 0xff; }

int solid( PS& ps, Color rgb );

struct Point: public POINTL
{
    Point( int x_, int y_ ) { x=x_; y=y_; }
    Point() { x=0; y=0; }
};

inline bool operator == ( const Point& a, const Point& b )
{
    return (a.x==b.x) && (a.y==b.y);
}

struct Size: public SIZEL
{
    Size( int cx_, int cy_ ) { cx = cx_; cy = cy_; }
    Size() { cx = 0; cy = 0; }
};

struct Rect: public RECTL
{
    Rect( int x1, int y1, int x2, int y2 ) { xLeft = x1; yBottom = y1; xRight = x2; yTop = y2; }
    Rect( Point p1, Point p2 ) { xLeft = p1.x; yBottom = p1.y; xRight = p2.x; yTop = p2.y; }
    Rect( const RECTL& r ) { xLeft = r.xLeft; yBottom = r.yBottom; xRight = r.xRight; yTop = r.yTop; }
    Rect( Point p, Size s ) { xLeft = p.x; yBottom = p.y; xRight = p.x+s.cx; yTop = p.y+s.cy; }
    Rect() { xLeft = 0; yBottom = 0; xRight = 0; yTop = 0; }

    Point top_left() const { return Point(xLeft,yTop); }
    Point bottom_right() const { return Point(xRight,yBottom); }
    Point top_right() const { return Point(xRight,yTop); }
    Point bottom_left() const { return Point(xLeft,yBottom); }

    bool contains( const Point& p ) const
    { return p.x >= xLeft && p.x < xRight && p.y >= yBottom && p.y < yTop; }

    bool contains( const Rect& r ) const
    { return contains(r.top_left()) && contains(r.bottom_right()); }
        
    int width() const { return xRight - xLeft; }
    int height() const { return yTop - yBottom; }

    bool operator ! () const { return width()==0 || height()==0; }
};

inline Rect merge( const Rect& a, const Rect& b )
{
    if( !a ) return b;
    if( !b ) return a;
    return Rect( min(a.xLeft,b.xLeft), min(a.yBottom,b.yBottom),
                 max(a.xRight,b.xRight), max(a.yTop,b.yTop) );
}

inline Rect intersect( const Rect& a, const Rect& b )
{
    if( !a || !b ) return Rect();
    Rect rc( max(a.xLeft,b.xLeft), max(a.yBottom,b.yBottom),
             min(a.xRight,b.xRight), min(a.yTop,b.yTop) );
    if( rc.width() <= 0 || rc.height() <= 0 ) return Rect();
    return rc;
}

inline Rect operator - ( const Rect& a, const Point& p )
{
    return Rect( a.xLeft-p.x, a.yBottom-p.y, 
                 a.xRight-p.x, a.yTop-p.y );
}

inline Rect operator + ( const Rect& a, const Point& p )
{
    return Rect( a.xLeft+p.x, a.yBottom+p.y, 
                 a.xRight+p.x, a.yTop+p.y );
}

class Region
{
    HRGN hrgn;
    PS& ps;

  public:
    Region( PS& ps );
    ~Region();

    int query_rects( int count, Rect rects[] ) const;

    operator HRGN() { return hrgn; }
};

struct DamageArea
{
    DamageArea( const Rect& clipping_area );
    ~DamageArea();

    // Merge:
    void operator += ( const Rect& r );

    // Iteration:
    int num_rects() const;
    Rect rect( int r ) const;
    
    void clip_to( const Rect& c ) { clipping = c; }
    void no_clip() { clipping = Rect(); }

  private:
    Rect area;
    Rect clipping;
};

class Mutex
{
  public:
    Mutex();
    Mutex( const char* name );
    ~Mutex();

    void lock();
    bool lock( int timeout );           // true for success
    void unlock();

    struct LockFailed {};
    struct Lock
    {
        Mutex& mutex;
        int gotten;
        Lock( Mutex& m ): mutex(m), gotten(false)
        { mutex.lock(); gotten = true; }
        Lock( Mutex& m, int timeout ): mutex(m), gotten(false)
        { if( mutex.lock( timeout ) ) gotten = true; }
        ~Lock() { if( gotten) mutex.unlock(); }
        bool locked() { return gotten; }
    };

  private:
    HMTX handle_;
};

class EventSem
{
  public:
    EventSem();
    ~EventSem();

    void post();
    void reset();
    bool wait( int timeout = SEM_INDEFINITE_WAIT );

  private:
    HEV handle_;
};

template<int shift>
struct Random
{
    static inline int generate( int range )
    {
        return range >= 0 ? int(rand()%range) : 0;
    }
};

#define M_PI 3.1415926
inline void randomize()
{
    // Transition -- Borland provided randomize() but GCC & CSet did not.
    srand(unsigned(time(NULL)));
}

inline int ShortRandom( int range ) { return Random<16>::generate(range); }
inline int ByteRandom( int range ) { return Random<23>::generate(range); }

// Exception disabling code
#if 0
#define Throw(x) throw x
#define Catch(x) catch(x)
#define ThrowAgain() throw
#define Try try
#else
extern void throw_error( const char* msg, const char* where );
#define Throw_(x,y,z) throw_error(x,Merge(y,z))
#define STR1(x) _STR1(x)
#define _STR1(x) #x
#define Throw(x) Throw_(x,__FILE__,STR1(__LINE__))
#define Merge(x,y) x ## y
#define Catch(x) if(0)
#define ThrowAgain() 0
#define Try /* */
#endif

#define Assert(exp) ((exp) ?  (void)0 : Throw(#exp))
void Log( const char* text1, const char* text2 );

#endif
