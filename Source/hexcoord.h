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

#ifndef HexCoord_h
#define HexCoord_h

const int HexWidth = 28;
const int HexHeight = 24;

const int HexXSpacing = 3 * HexWidth / 4;
const int HexYSpacing = HexHeight;

const int HexXOrigin = -HexWidth + HexXSpacing;
const int HexYOrigin = -HexYSpacing/2;

struct HexCoord
{
    int m, n;
    HexCoord(): m(0), n(0) {}
    HexCoord( int m_, int n_ ): m(m_), n(n_) {}
    ~HexCoord() {}

    int x() const { return m * HexXSpacing + HexXOrigin; }
    int y() const { return n * HexYSpacing + HexYOrigin + HexHeight/2 * (m%2); }
    int left() const { return x() - HexWidth/2; }
    int right() const { return x() + HexWidth/2; }
    int bottom() const { return y() - HexHeight/2; }
    int top() const { return y() + HexHeight/2; }
    Rect rect() const { return Rect(left(),bottom(),right(),top()); }
};

inline int operator == ( const HexCoord& a, const HexCoord& b ) { return a.m == b.m && a.n == b.n; }
inline int operator != ( const HexCoord& a, const HexCoord& b ) { return a.m != b.m || a.n != b.n; }

enum HexDirection { DirN, DirNE, DirSE, DirS, DirSW, DirNW, DirNone };

void VectorDir( HexDirection d, int& dx, int& dy );
HexCoord PointToHex( int xp, int yp );

#define NEIGHBOR_DECL const POINTL neighbors[2][6] = \
{ \
    { {0,1},{1,0},{1,-1},{0,-1},{-1,-1},{-1,0} }, \
    { {0,1},{1,1},{1,0},{0,-1},{-1,0},{-1,1} } \
}

extern const POINTL neighbors[2][6];
inline HexCoord Neighbor( const HexCoord& h, HexDirection d )
{
    return HexCoord( h.m + neighbors[h.m&1][int(d)].x,
                     h.n + neighbors[h.m&1][int(d)].y );
}

#endif
