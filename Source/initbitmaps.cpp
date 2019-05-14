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
#include "Images.h"
#include "Map.h"

#include "InitCode.h"

#define TEXT_ORIGIN_X 1
#define TEXT_ORIGIN_Y 3

#define COLOR_OVERLAY iRGB(255,128,192)
#define COLOR_TRANS iRGB(255,0,255)

/////////////////////////////////////////////////////////////////////////
#if INIT_BITMAPS != 0

class HexTile
{
  public:
	HexTile();
	~HexTile();

	void assign_tile( int terrain_id );
	void assign_tile( Bitmap* tiles, Bitmap* masks, int index );

	void draw( PS& ps, int x, int y );
	void separate_mask();

	Bitmap* image() { return image_; }
	Bitmap* mask() { return mask_; }

  private:
	void free();

	Bitmap* image_;
	Bitmap* mask_;

	bool subtile_;	// If this is set, then the image points to a large
	int index_;		// bitmap and (0,index_*HexWidth) is our tile
};

HexTile::HexTile()
    :image_(NULL), mask_(NULL), subtile_(false), index_(0)
{
}

#if 0
void HexTile::draw( PS& ps, int x, int y )
{
    // Offset so that (x,y) passed in are the center of the hexagon
    // and not the lower left corner
    x -= HexWidth/2;
    y -= HexHeight/2;

    // Draw the mask and/or the image
    if( mask_ == &tile )
        mask_->bitblt( ps, x, y, ROP_SRCAND, 0, 0 );
    else if( mask_ )
        mask_->bitblt( ps, x, y, ROP_SRCAND, 0, HexHeight*index_ );
    if( image_ )
        image_->bitblt( ps, x, y, ROP_SRCPAINT, 0, HexHeight*index_ );
}
#endif

void HexTile::assign_tile( int ID )
{
    free();
    image_ = new Bitmap(ID);
    mask_ = &tile;
    subtile_ = false;
    index_ = 0;
}

void HexTile::assign_tile( Bitmap* tiles, Bitmap* masks, int index )
{
    free();
    image_ = tiles;
    mask_ = masks;
    subtile_ = true;
    index_ = index;
}

HexTile::~HexTile()
{
    free();
}

void HexTile::free()
{
    if( !subtile_ )
        delete image_;
    if( !subtile_ && mask_ != &tile )
        delete mask_;
    image_ = NULL;
    mask_ = NULL;
    subtile_ = false;
    index_ = 0;
}

Bitmap pink, bigblob, smallblob, background_bmp;
HexTile clear, tree, farm, tower, fire1, fire2, canal, lava;
HexTile marktile, market;

#endif
//////////////////////////////////////////////////////////////////////

static void LoadRleArray( const char* filename, Sprite rle[], int num,
                          int hotspot_x = HexWidth/2, 
                          int hotspot_y = HexHeight/2 )
{
    FILE* f = fopen( filename, "rb" );
    if( !f ) Throw("Could not open RLE file");
    for( int i = 0; i < num; i++ )
    {
        rle[i].image = LoadRLE( f );
        rle[i].trans1 = LoadRLE( f );
        rle[i].trans2 = LoadRLE( f );
        rle[i].hotspot.x = hotspot_x;
        rle[i].hotspot.y = hotspot_y;
    }
    fclose( f );
}

#if INIT_BITMAPS != 0

Bitmap newwatertiles;
Bitmap newwatermask;
Bitmap font_bmp, fonti_bmp;

Bitmap watertiles;
Bitmap watermasks;
Bitmap terraintiles;

static void SaveRleArray( const char* filename, Sprite rle[], int num )
{
    FILE* f = fopen( filename, "wb" );
    for( int i = 0; i < num; i++ )
    {
        SaveRLE( rle[i].image, f );
        SaveRLE( rle[i].trans1, f );
        SaveRLE( rle[i].trans2, f );
    }
    fclose( f );
}

void Convert3x3ToSprite( PS& ps, Sprite& sprite, int width, int height,
                         int spacing = 3 )
{
    Bitmap im1( width, height );
    Bitmap im2( width, height );
    Bitmap im3( width, height );
    PS ps1( im1.ps() );
    PS ps2( im2.ps() );
    PS ps3( im3.ps() );
    ps1.rgb_mode();
    ps2.rgb_mode();
    ps3.rgb_mode();
    ps1.fill_rect(Point(0,0),Point(width,height),COLOR_TRANS);
    ps2.fill_rect(Point(0,0),Point(width,height),COLOR_TRANS);
    ps3.fill_rect(Point(0,0),Point(width,height),COLOR_TRANS);
    Color trans = ps.pixel( Point(0,0) );

    int pixelweight[3][3] 
        = {
        { 1, 1, 1 },
        { 1, 2, 1 },
        { 1, 1, 1 }
        };
    const int totalweight = 1+1+1 + 1+2+1 + 1+1+1;

    for( int x = 0; x < width; x++ )
        for( int y = 0; y < height; y++ )
        {
            int count = 0;
            int r = 0;
            int g = 0;
            int b = 0;
            for( int dx = 0; dx < 3; dx++ )
                for( int dy = 0; dy < 3; dy++ )
                {
                    Color c = ps.pixel( Point(x*spacing+dx,y*spacing+dy) );
                    if( c != trans )
                    {
                        int weight = pixelweight[dx][dy];

                        count += weight;
                        r += weight*ColorToRed(c);
                        g += weight*ColorToGreen(c);
                        b += weight*ColorToBlue(c);
                    }
                }

            // Turn pixel array into transparency
            if( count >= (2+totalweight)/4 )
            {
                r = (r+count/2)/count;
                g = (g+count/2)/count;
                b = (b+count/2)/count;
                Color c = iRGB(r,g,b);
                if( count >= (2+2*totalweight)/3 )
                    ps3.pixel( Point(x,y), c );
                else if( count >= (1+totalweight)/3 )
                    ps2.pixel( Point(x,y), c );
                else
                    ps1.pixel( Point(x,y), c );
            }
        }
    
    if( width == HexWidth && height == HexHeight )
    {
        // Mask off the hexagons
        int ROP_INVPAINT = 0x22;
        tile.bitblt( ps1, 0, 0, ROP_INVPAINT );
        pink.bitblt( ps1, 0, 0, ROP_SRCPAINT );
        tile.bitblt( ps2, 0, 0, ROP_INVPAINT );
        pink.bitblt( ps2, 0, 0, ROP_SRCPAINT );
        tile.bitblt( ps3, 0, 0, ROP_INVPAINT );
        pink.bitblt( ps3, 0, 0, ROP_SRCPAINT );
    }

    PixelBuffer buf( width, height );
    
    PSToBuffer( ps1, trans, RLE_MASK, buf );
    sprite.trans1 = MakeRLE( buf, RLE_MASK );
    
    PSToBuffer( ps2, trans, RLE_MASK, buf );
    sprite.trans2 = MakeRLE( buf, RLE_MASK );
    
    PSToBuffer( ps3, trans, RLE_MASK, buf );
    sprite.image = MakeRLE( buf, RLE_MASK );
}

void ShadeEdges( PS& ps, int brighten, int darken )
{
    Color pink_c = pink.ps().pixel( Point(HexWidth/2,HexHeight/2) );
    for( int x = 0; x < HexWidth*3; x++ )
        for( int y = 0; y < HexHeight*3; y++ )
        {
            // From statusbar:
            extern unsigned blend( unsigned a, unsigned b, int percent );

            Point p(x,y);
            Color c = ps.pixel(p);

            if( ps.pixel(p) != COLOR_TRANS )
            {
                int dx = ByteRandom(4);
                int dy = ByteRandom(4);
                if( x >= dx && y < HexHeight*3-dy && 
                    ps.pixel( Point(x-dx,y+dy) ) == COLOR_TRANS &&
                    pink.ps().pixel( Point((x-dx)/3,(y+dy)/3) ) == pink_c )
                {
                    ps.pixel( p, blend(c,iRGB(255,255,255),brighten) );
                }
                else if( x < HexWidth*3-dx && y >= dy &&
                         ps.pixel( Point(x+dx,y-dy) ) == COLOR_TRANS &&
                         pink.ps().pixel(Point((x+dx)/3,(y-dy)/3)) == pink_c )
                {
                    ps.pixel( p, blend(c,iRGB(0,0,0),darken) );
                }
            }
        }
}

static int GeneratePoints_Arc( double dir1, double dir2, 
                               double x0, double y0, double r, 
                               double x[], double y[] )
{
    const int N = 25;
    double a0 = dir1 * M_PI / 3;
    double da = M_PI / 3.0 / N;
    double ddir = dir2-dir1+6.0;
    while( ddir > 6.0 )
        ddir -= 6.0;
    int n = N * int(ddir);
    for( int i = 0; i <= n; i++ )
    {
        double a = a0 + da*i;
        x[i] = x0 + r * sin(a);
        y[i] = y0 + r * cos(a);
    }

    return n+1;
}

static int GeneratePoints_Fillet( int dir1, int dir2,
                                  double x0, double y0, double r, double w,
                                  double x[], double y[] )
{
    double a1 = dir1 * pi / 3;
    double a2 = dir2 * pi / 3;
    double x1 = x0 + sin(a1) * r;
    double y1 = y0 + cos(a1) * r;
    double x2 = x0 + sin(a2) * r;
    double y2 = y0 + cos(a2) * r;

    double cx = x1;
    double cy = y1;
    double bx = 2*(x0-x1);
    double by = 2*(y0-y1);
    double ax = x2+x1-2*x0;
    double ay = y2+y1-2*y0;

    if( dir1 == -1 )
    {
        ax = 0;
        ay = 0;
        bx = x2-x0;
        by = y2-y0;
        cx = x0;
        cy = y0;
    }

    if( dir2 == -1 )
    {
        ax = 0;
        ay = 0;
        bx *= 0.5;
        by *= 0.5;
    }

    int n = 50;
    for( int i = 0; i < n; i++ )
    {
        double t = double(i) / n;
        double tx = ax*t*t + bx*t + cx;
        double ty = ay*t*t + by*t + cy;
        double dx = 2*ax*t + bx;
        double dy = 2*ay*t + by;
        double d = 0.01+sqrt(dx*dx+dy*dy);
        dx /= d;
        dy /= d;
        x[i] = tx - dy*w;
        y[i] = ty + dx*w;
    }

    return n;
}

static void GenerateOwnerOverlay()
{
    Bitmap bmp( HexWidth*3, HexHeight*3 );
    PS ps( bmp.ps() );
    ps.rgb_mode();

    double x0 = HexWidth*1.5-0.5;
    double y0 = HexHeight*1.5-0.5;

#if INIT_BITMAPS == 12
    // Road
    int Width1 = 11, Width2 = 14;
    double r0 = HexHeight*1.6;
#elif INIT_BITMAPS == 11
    // Wall
    int Width1 = 12, Width2 = 16;
    double r0 = HexHeight*1.6;
#else
    // Border
    int Width1 = 18, Width2 = 24;
    double r0 = HexHeight*1.5;
#endif

    for( int index = 0; index < 64; index++ )
    {
        ps.fill_rect( Point(0,0), Point(HexWidth*3,HexHeight*3), COLOR_TRANS );
        static double xL[4000];
        static double yL[4000];
        static double xR[4000];
        static double yR[4000];
        static double xC[4000];
        static double yC[4000];
        int nO = 0, nC = 0;
        int count = 0;
        if( index == 0 )
        {
            GeneratePoints_Arc( 0, 3, x0, y0, Width2, xL, yL );
            nO = GeneratePoints_Arc( 0, 3, x0, y0, Width1, xR, yR );
            GeneratePoints_Arc( 3, 6, x0, y0, Width2, xL+nO, yL+nO );
            nO += GeneratePoints_Arc( 3, 6, x0, y0, Width1, xR+nO, yR+nO );
        }
        else
        {
            int last_d = 0;
            for( int d0 = 0; d0 < 6; d0++ )
            {
                // Scan the directions, looking for places that need a fillet
                if( index & (1<<d0) )
                {           
                    last_d = d0;
                    count++;
                    for( int dd = 1; dd < 6; dd++ )
                    {
                        int d1 = (d0+dd)%6;
                        if( index & (1<<d1) )
                        {
                            if( dd > 1 )
                            {
                                GeneratePoints_Fillet
                                ( d0, d1, x0, y0, r0, Width2, xL+nO, yL+nO );
                                nO += GeneratePoints_Fillet
                                ( d0, d1, x0, y0, r0, Width1, xR+nO, yR+nO );
                                nC += GeneratePoints_Fillet
                                ( d0, d1, x0, y0, r0, 0, xC+nC, yC+nC );
                            }
                            else
                            {
                                double a = (d0+0.5) * M_PI / 3;
                                xL[nO] = x0 + HexWidth*1.6 * sin(a);
                                yL[nO] = y0 + HexWidth*1.6 * cos(a);
                                xR[nO] = xL[nO];
                                yR[nO] = yL[nO];
                                nO++;
                            }
                            break;
                        }
                    }
                }
            }

            if( count == 1 )
            {
                int end_d = (last_d+3)%6;

                GeneratePoints_Fillet
                    ( last_d, -1, x0, y0, r0, Width2, xL+nO, yL+nO );
                nO += GeneratePoints_Fillet
                    ( last_d, -1, x0, y0, r0, Width1, xR+nO, yR+nO );
                GeneratePoints_Arc( end_d-1.5, end_d+1.5, x0, y0, Width2, xL+nO, yL+nO );
                nO += GeneratePoints_Arc( end_d-1.5, end_d+1.5, x0, y0, Width1, xR+nO, yR+nO );
                GeneratePoints_Fillet
                    ( -1, last_d, x0, y0, r0, Width2, xL+nO, yL+nO );
                nO += GeneratePoints_Fillet
                    ( -1, last_d, x0, y0, r0, Width1, xR+nO, yR+nO );
                nC += GeneratePoints_Fillet
                    ( -1, last_d, x0, y0, r0, 0, xC+nC, yC+nC );
            }           
        }

#if INIT_BITMAPS == 10
        double spacing = 15;
        double len = 0.0;
        for( int i = 1; i < nO; i++ )
        {
            double dx = xR[i]-xR[i-1];
            double dy = yR[i]-yR[i-1];
            len += sqrt( dx*dx + dy*dy );
        }
        if( int(len/spacing) > 0 )
        {
            double dlen = len / int(len/spacing);

            int last_i = -1;
            len = 0.0;
            for( i = 0; i < nO; i++ )
            {
                if( i > 0 )
                {
                    double dx = xR[i]-xR[i-1];
                    double dy = yR[i]-yR[i-1];
                    double d = sqrt( dx*dx + dy*dy );
                    len += d;
                    if( d > spacing )
                    {
                        last_i = -1;
                        len = 0.0;
                    }
                }
                double w = len/dlen - int(len/dlen);
                if( w > 0.25 && w <= 0.75 )
                {
                    if( last_i != -1 && last_i != i-1 )
                    {
                        GpiBeginPath( ps, 1 );
                        Point apt[4] = {
                            Point(int(xL[last_i]),int(yL[last_i])),
                            Point(int(xR[last_i]),int(yR[last_i])),
                            Point(int(xL[i]),int(yL[i])),
                            Point(int(xR[i]),int(yR[i]))
                        };
                        GpiMove( ps, &(apt[0]) );
                        GpiPolyLine( ps, 3, &(apt[1]) );
                        GpiCloseFigure( ps );
                        GpiEndPath( ps );
                        GpiSetColor( ps, COLOR_OVERLAY );
                        GpiFillPath( ps, 1, FPATH_ALTERNATE );
                    }
                }
                else
                    last_i = i;
            }
        }

        Convert3x3ToSprite( ps, overlay_rle[index], HexWidth, HexHeight );
#endif

#if INIT_BITMAPS == 11 || INIT_BITMAPS == 12 
        for( int part = 0; part < 2; part++ )
        {
            double* x = (part==0)?xL:xR;
            double* y = (part==0)?yL:yR;
#if INIT_BITMAPS == 11
            Color c = (part==0)?iRGB(180,180,180):iRGB(204,204,204);
#else
            Color c = (part==0)?iRGB(0,0,0):iRGB(128,109,85);
#endif
            Point p = Point( int(x[0]+0.5), int(y[0]+0.5) );
            GpiBeginPath( ps, 1 );
            GpiMove( ps, &p );
            for( int i = 1; i < nO; i++ )
            {
                p.x = int(x[i]);
                p.y = int(y[i]);
                GpiLine( ps, &p );
            }
            GpiCloseFigure( ps );
            GpiEndPath( ps );
            GpiSetColor( ps, c );
            GpiFillPath( ps, 1, FPATH_INCL );
        }

#if INIT_BITMAPS == 12
        double dist = 0.0;
        for( int i = 1; i < nC; i++ )
        {
            const Color stripe = iRGB(170,146,85);
            Point p = Point( int(xC[i]+0.5), int(yC[i]+0.5) );

            if( int(dist) % 25 <= 6 || int(dist) % 25 >= 19 )
                ps.fill_rect( Point(p.x-1,p.y-1), Point(p.x+2,p.y+2), stripe );
            
            dist += sqrt( (xC[i]-xC[i-1])*(xC[i]-xC[i-1])
                          + (yC[i]-yC[i-1])*(yC[i]-yC[i-1]) );
        }
#endif

#if INIT_BITMAPS == 11 || INIT_BITMAPS == 12
        PS ps2( tower.image()->ps() );
        ps2.rgb_mode();
        Color pink_rgb = ps2.pixel( Point(0,0) );
        for( int x = 0; x < 3*HexWidth; x++ )
            for( int y = 0; y < 3*HexHeight; y++ )
            {
                Point p(x,y);
                int pink_c = pink.ps().pixel( Point(HexWidth/2,HexHeight/2) );
                if( ps.pixel(p) != pink_rgb )
                {
                    int x2 = ( x/3 ) % (HexWidth/3) + HexWidth/3;
                    int y2 = ( y/3 ) % (HexHeight/2) + HexHeight/4;
#if INIT_BITMAPS == 11
                    int c = ps2.pixel( Point(x2,y2) );
#else
                    int c1 = ps.pixel( p );
                    int c2 = ps2.pixel( Point(x2,y2) );
                    int r_ = (ColorToRed(c1)*2+ColorToRed(c2))/3;
                    int g_ = (ColorToGreen(c1)*2+ColorToGreen(c2))/3;
                    int b_ = (ColorToBlue(c1)*2+ColorToBlue(c2))/3;
                    int c = ((x+y)%2==0)? c1 : iRGB(r_,g_,b_);
#endif
                    ps.pixel( p, c );

#if INIT_BITMAPS == 11
                    int dx = ByteRandom(4);
                    int dy = ByteRandom(4);
                    if( x >= dx && y < HexHeight*3-dy && 
                        ps.pixel( Point(x-dx,y+dy) ) == COLOR_TRANS &&
                        pink.ps().pixel( Point((x-dx)/3,(y+dy)/3) ) == pink_c )
                    {
                        ps.pixel( p, iRGB(255,255,255) );
                    }
                    else if( x < HexWidth*3-dx && y >= dy &&
                             ps.pixel( Point(x+dx,y-dy) ) == COLOR_TRANS &&
                             pink.ps().pixel(Point((x+dx)/3,(y-dy)/3)) == pink_c )
                    {
                        ps.pixel( p, iRGB(128,128,128) );
                    }
#endif
                }
            }
#if INIT_BITMAPS == 12
        Convert3x3ToSprite( ps, road_rle[index], HexWidth, HexHeight );
#elif INIT_BITMAPS == 11
        Convert3x3ToSprite( ps, wall_rle[index], HexWidth, HexHeight );
#endif

#endif

#endif
    }       
}

static void GenerateTreeBitmaps()
{
    // This used to generate house bitmaps (7 spheres) but it was
    // turned into a tree bitmap generator, where trees are some number
    // of spheres, which become larger as they get older.
    Bitmap bmp( HexWidth*3, HexHeight*3 );
    PS ps( bmp.ps() );
    ps.rgb_mode();

    // First figure out the shape of the trees
    struct Shape { double x, y, z; double r; int rm, gm, bm; };
    const int NUM_CLUSTERS = 9;
    const int NUM_SPHERES = 16;
    Shape shapes[NUM_CLUSTERS*NUM_SPHERES];
    for( int i = 0; i < NUM_CLUSTERS; ++i )
    {
        double da = 2*M_PI / 6;
        double rad = HexWidth*1.05;
        double angles[NUM_CLUSTERS] = { 3.5, 5.5, 1.5, 0, 2, 4, 3, 5, 1 };
        double dist[NUM_CLUSTERS] = { 0.45, 0.45, 0.45, 1, 1, 1, 1, 1, 1 };
        double a = da * (0.167 + angles[i]);
        int dx = int( rad * dist[i] * cos(a) );
        int dy = int( rad * dist[i] * sin(a) );
        int x = HexWidth*3/2 + dx;
        int y = HexHeight*3/2 + dy;

        for( int j = 0; j < NUM_SPHERES; ++j )
        {
            Shape& S = shapes[i*NUM_SPHERES+j];
            if( j == 0 )
            {
                // A brown trunk in the center
                S.x = x;
                S.y = y;
                S.z = 0.0;
                S.r = 90.0;
                S.rm = 100;
                S.gm = 100;
                S.bm = 40;
            }
            else
            {
                S.x = x + 0.1*ByteRandom(90) - 0.1*ByteRandom(90);
                S.y = y + 0.1*ByteRandom(90) - 0.1*ByteRandom(90);
                S.z = 10.0 + ByteRandom(5) - ByteRandom(5);
                S.r = (30.0 + ByteRandom(20) - ByteRandom(20))*dist[i];
                S.rm =  80 + ByteRandom(70) - ByteRandom(30);
                S.gm = 150 + ByteRandom(90) - ByteRandom(40);
                S.bm =  80 + ByteRandom(30) - ByteRandom(30);
            }
        }
    }
    
    for( int m = 0; m < NUM_TREE_SEASONS; m++ )
        for( int n = 0; n < NUM_TREES; n++ )
        {
            ps.fill_rect( Point(0,0), Point(HexWidth*3,HexHeight*3),
                          COLOR_TRANS );

            double z[3*HexWidth][3*HexHeight]; // Z-Buffer
            for( int xx = 0; xx < 3*HexWidth; ++xx )
                for( int yy = 0; yy < 3*HexHeight; ++yy )
                    z[xx][yy] = 0.0;
        
            for( int i = 0; i < NUM_CLUSTERS*NUM_SPHERES; i++ )
            {
                double Radius = shapes[i].r*(3+n)/(3+NUM_TREES);
                double radius = sqrt(Radius);

                // Here, (xx,yy,zz) are vector values, not absolutes,
                // and we want them to line up on an integer grid
                for( double xx = -15-shapes[i].x+int(shapes[i].x);
                     xx <= 15; xx++ )
                {
                    for( double yy = -15-shapes[i].y+int(shapes[i].y);
                         yy <= 15; yy++ )
                    {
                        int x = int(xx+shapes[i].x), y = int(yy+shapes[i].y);
                        if( x < 0 || x >= 3*HexWidth ) continue;
                        if( y < 0 || y >= 3*HexHeight ) continue;
                    
                        double rr = radius*radius - xx*xx - yy*yy;
                        if( rr <= 0.01 ) continue;
                        double zz = sqrt( rr );

                        // Check & store to z-buffer
                        if( zz + shapes[i].z < z[x][y] )
                            continue;
                        z[x][y] = zz+shapes[i].z;

                        // Now figure out lighting, and draw the pixel
                        // where the light vector is (1,1,0.8)
                        double dot = (-0.8*xx+yy+zz)/(sqrt(2+0.8*0.8)*radius);
                        dot = ( dot + 1 ) / 2;
                        int r = int(shapes[i].rm*dot)
                            + ByteRandom(30) - ByteRandom(30);
                        int g = int(shapes[i].gm*dot)
                            + ByteRandom(60) - ByteRandom(40);
                        int b = int(shapes[i].bm*dot)
                            + ByteRandom(30) - ByteRandom(30);

                        // Seasonal effects on color
                        if( m < NUM_TREE_SEASONS*3/4 )
                            r += 4/3*m*60/NUM_TREE_SEASONS;
                        else
                            r += 4*(NUM_TREE_SEASONS-m)*60/NUM_TREE_SEASONS;
                        if( m > NUM_TREE_SEASONS/2 )
                            g -= (NUM_TREE_SEASONS/4 - abs(m-NUM_TREE_SEASONS*3/4))*20/NUM_TREE_SEASONS;

                        // Range checking
                        r = min(255,r);
                        r = max(0,r);
                        g = min(255,g);
                        g = max(0,g);
                        b = min(255,b);
                        b = max(0,b);
                        ps.pixel( Point(x,y), iRGB(r,g,b) );
                    }
                }
            }

            Convert3x3ToSprite( ps, trees_rle[n+m*NUM_TREES],
                                HexWidth, HexHeight );
        }
}

static void GenerateHouseBitmaps()
{
    Bitmap bmp( HexWidth*3, HexHeight*3 );
    PS ps( bmp.ps() );
    ps.rgb_mode();
    const double ROOF_HEIGHT = 5.0;
    for( int n = 0; n < 8; n++ )
    {
        ps.fill_rect( Point(0,0), Point(HexWidth*3,HexHeight*3), COLOR_TRANS );

        double da = 2*M_PI / 6;
        for( int i = 1; i < 8; i++ )
        {
            double angles[] = { 0, 1, 3, 5, 2, 4, 0, 1.5 };
            double a0 = da * angles[i];
            double aL = da * (angles[i]+0.34);
            double aR = da * (angles[i]-0.34);

            double
                rad0B = HexWidth*0.75, // inner radius
                rad0M = HexWidth*1.1,
                rad0E = HexWidth*1.3; // outer radius

            int rm = 190, gm = 190, bm = 190;
            if( i == 7 )
            {
                rad0B = HexWidth*0.8;
                aL += da*0.1;
                aR -= da*0.1;
            }
            
            if( i <= n )
            {
                rm = 265;
                gm = 140;
                bm = 168;
                aL += da * 0.05;
                aR -= da * 0.05;
                rad0B = HexWidth*0.85;
                rad0E = HexWidth*1.35;
            }

            double
                rad1B=rad0B*sin(da), // radii for the left and right sides
                rad1M=rad0M*sin(da),
                rad1E=rad0E*sin(da);
            
            // Six control points for our polygon, which is an L shape
            // d: delta
            // 0/L/R: center/left/right of polygon
            // B/M/E: beginning (closest to center), middle, end radius
            Point d0B( int(rad0B*cos(a0)), int(rad0B*sin(a0)) );
            Point d0M( int(rad0M*cos(a0)), int(rad0M*sin(a0)) );
            Point d0E( int(rad0E*cos(a0)), int(rad0E*sin(a0)) );
            Point dLB( int(rad1B*cos(aL)), int(rad1B*sin(aL)) );
            Point dLM( int(rad1M*cos(aL)), int(rad1M*sin(aL)) );
            Point dLE( int(rad1E*cos(aL)), int(rad1E*sin(aL)) );
            Point dRB( int(rad1B*cos(aR)), int(rad1B*sin(aR)) );
            Point dRM( int(rad1M*cos(aR)), int(rad1M*sin(aR)) );
            Point dRE( int(rad1E*cos(aR)), int(rad1E*sin(aR)) );
            
            // Offset to add
            int dx = 0, dy = 0;
            if( i == 7 ) { dx = -d0B.x; dy = -d0B.y; }
            dx += HexWidth*3/2;
            dy += HexHeight*3/2;

            // In these surfaces, the first three points should form
            // a right angle, for lighting calculations.
            // The points should be counterclockwise.
            const int NUM_SURFACES = 4;
            const int NUM_POINTS = 4;
            Point surfaces[NUM_SURFACES][NUM_POINTS] = {
            { d0B, dRB, dRM, d0M },
            { dRM, dRE, d0E, d0M },
            { d0E, dLE, dLM, d0M },
            { dLM, dLB, d0B, d0M }
            };
            int zcorners[NUM_SURFACES][2] = {
            { 0, +1 },
            { -1, 0 },
            { 0, +1 },
            { -1, 0 }
            };

            for( int surf = 0; surf < NUM_SURFACES; ++surf )
            {
                for( int j = 0; j < NUM_POINTS; ++j )
                {
                    surfaces[surf][j].x += dx;
                    surfaces[surf][j].y += dy;
                }

                GpiBeginPath( ps, 1 );
                GpiMove( ps, &(surfaces[surf][0]) );
                GpiPolyLine( ps, NUM_POINTS-1, &(surfaces[surf][1]) );
                GpiCloseFigure( ps );
                GpiEndPath( ps );

                // Now we have to do some icky lighting calculations
                // It would be better if we preserved the original
                // coordinates as doubles instead of converting to ints.
                double x1 = surfaces[surf][1].x - surfaces[surf][0].x;
                double x2 = surfaces[surf][2].x - surfaces[surf][1].x;
                double y1 = surfaces[surf][1].y - surfaces[surf][0].y;
                double y2 = surfaces[surf][2].y - surfaces[surf][1].y;
                double z1 = -ROOF_HEIGHT*zcorners[surf][1];
                double z2 = -ROOF_HEIGHT*zcorners[surf][0];

                double Nx = y1*z2-z1*y2, Ny = z1*x2-x1*z2, Nz = x1*y2-y1*x2;
                double Nsize = sqrt(Nx*Nx+Ny*Ny+Nz*Nz);
                double Lx = 0.3, Ly = 0.5, Lz = 1.0;
                double Lsize = sqrt(Lx*Lx+Ly*Ly+Lz*Lz);

                double dot = (Nx*Lx+Ny*Ly+Nz*Lz)/(Nsize*Lsize);
                double lighting = (1.0+dot)*0.55;

                rm = min( int(lighting*rm), 255 );
                gm = min( int(lighting*gm), 255 );
                bm = min( int(lighting*bm), 255 );
                
                GpiSetColor( ps, iRGB(rm,gm,bm) );
                GpiFillPath( ps, 1, FPATH_ALTERNATE );
            }
        }

        ShadeEdges( ps, 40, 60 );
        Convert3x3ToSprite( ps, house_rle[n], HexWidth, HexHeight );
    }
}

#if INIT_BITMAPS == 13
static void GenerateAltFont( Sprite rle[], Bitmap& font_bmp )
{
    // Bitmap is an array of bytes, 0 for white to 255 for black
    const int BMP_WIDTH=456, BMP_HEIGHT=311;
    static byte bitmap[BMP_WIDTH][BMP_HEIGHT];

    FILE* f = fopen("/tmp/font.out","wt");
    
    // Convert the OS/2 bitmap to our array of bytes
    {
        PS ps( font_bmp.ps() );
        ps.rgb_mode();
        int top = 0xff;
        for( int x = 0; x < BMP_WIDTH; x++ )
            for( int y = 0; y < BMP_HEIGHT; y++ )
                bitmap[x][y] = top - (ps.pixel( Point(x,y) ) & 0xff);
    }

    // Now extract characters from the array
    
    // First, find the rows:
    int start_row[6], numrows = 0;
    const int CHARS_PER_ROW = 16;
    for( int y = 0; y < BMP_HEIGHT-1; y++ )
    {
        if( bitmap[2][y] < 128 && bitmap[2][y+1] > 128 )
            start_row[numrows++] = y;
    }

    if( numrows != 6 ) Throw("Didn't find all the rows.");

    // FUDGE for GhostView misalignment(?)
    start_row[0]++;
    start_row[3]++;
    
    // Next, find the columns:
    int start_column[128], end_column[128];
    int x1 = 0, y1 = start_row[0];
    for( int c = 32; c < 127; c++ )
    {
        // ASSUMPTIONS about the bitmap size, the font spacing, etc.,
        // are being made here!
        int col = (c-32) % CHARS_PER_ROW;
        int row = (c-32) / CHARS_PER_ROW;
        if( row >= numrows ) Throw("Too few rows");
        if( start_row[row] != y1 )
        {
            // Changed row, so reset x
            y1 = start_row[row];
            x1 = 0;
        }

        int y2 = y1+20;         // where are the colbars relative to rowbars?
        if( y2 > BMP_HEIGHT-1 )
            Throw("Row too high");
        
        // Search for first marker: beginning of column is WHITE, BLACK
        x1 += 2;
        while( x1 < BMP_WIDTH-1 &&
               !( bitmap[x1][y2] < 128 && bitmap[x1+1][y2] > 128 ) )
            x1++;
        start_column[c] = x1;
        
        // Search for second marker: beginning of column is BLACK, WHITE
        x1 += 2;
        while( x1 < BMP_WIDTH-1 &&
               !( bitmap[x1][y2] > 128 && bitmap[x1+1][y2] < 128 ) )
            x1++;
        end_column[c] = x1+1;   // 1 because this should be one past the end
        end_column[c] += 4;     // extra space for italic font
        
        fprintf(f,"Char %d(`%c'): width = %d\n",c,c,end_column[c]-start_column[c]);
    }

    // Now transfer the array data to character bitmaps
    Bitmap bmp( 100, 100 );
    PS ps( bmp.ps() );
    ps.rgb_mode();

    for( c = 32; c < 127; c++ )
    {
        int row = (c-32) / CHARS_PER_ROW;
        int y1 = start_row[row];

        for( int brightness = 0; brightness < 6; brightness++ )
        {
            ps.fill_rect( Rect(0,0,100,100), COLOR_TRANS );
            double threshold1 = 0.1 + brightness*0.3;
            double threshold2 = threshold1 + 0.3;
            if( brightness >= 3 )
            {
                threshold1 = 0.01 + (brightness-3)*0.33;
                threshold2 = threshold1 + 0.33;
            }
            int a = int(threshold1*256);
            int b = int(threshold2*256);

            // ASSUME that this is the proper range for descenders & ascenders
            for( int y = y1-3; y <= y1+12; y++ )
            {
                for( int x = start_column[c]; x < end_column[c]; x++ )
                    if( a <= bitmap[x][y] && bitmap[x][y] < b )
                        ps.pixel( Point(x-start_column[c],y-y1+3),
                                  iRGB(0,0,0) );
            }
            // ASSUME this is the right height
            PixelBuffer buf( end_column[c]-start_column[c], 16 );
            PSToBuffer( ps, COLOR_TRANS, RLE_MASK, buf );
            RLE* r = MakeRLE( buf, RLE_MASK );

            int ci = c + 128*(brightness>=3);
            if( brightness%3 == 0 ) rle[ci].trans1 = r;
            if( brightness%3 == 1 ) rle[ci].trans2 = r;
            if( brightness%3 == 2 ) rle[ci].image = r;
        }
    }
    fprintf(f,"\n");
    fclose(f);
}
#endif

static void GenerateFont( const char* face, int height, Sprite rle[], int sz )
{
    Bitmap bmp( 100, 100 );
    PS ps( bmp.ps() );
    ps.rgb_mode();
    ps.font( face, height, false );
    ps.text_mode( PS::TransparentFont );
    const int EXTRA = 0;

    for( int c = 32; c <= 127; c++ )
    {
        char str[3] = {'\0','\0','\0'};
        str[0] = char(c);

        // First draw the text, ignoring sz because we want bold and
        // nonbold text to use the same offsets
        ps.fill_rect( Rect(0,0,100,100), COLOR_TRANS );
        int width = ps.text_width( str );
        for( int x = -EXTRA; x <= 0; x++ )
            for( int y = -EXTRA; y <= 0; y++ )
                ps.text( Point(TEXT_ORIGIN_X*3+x+3,TEXT_ORIGIN_Y*3+1+y), str );

        // Determine the best offset
        int count[3] = {0,0,0};
        for( int x = 0; x < width+5; x++ )
            for( int y = 0; y < height+2; y++ )
                if( ps.pixel( Point(x,y) ) != COLOR_TRANS && 
                    ps.pixel( Point(x+1,y) ) != COLOR_TRANS )
                {
                    ++count[x%3];
                    if( ps.pixel( Point(x+2,y) ) != COLOR_TRANS )
                        ++count[x%3];
                }

        // We want to use the best offset this time; the offset added
        // to the index of the best count should be 0, mod 3.
        int xoffset = (count[0]>count[1])?
            ( (count[0]>count[2])?  0 : 1 ) :
            ( (count[1]>count[2])? -1 : 1 );

        // Now draw again, with the best offset
        ps.fill_rect( Rect(0,0,100,100), COLOR_TRANS );
        for( int x = -sz-EXTRA; x <= sz; x++ )
            for( int y = -sz-EXTRA; y <= sz; y++ )
                ps.text( Point(TEXT_ORIGIN_X*3+x+xoffset,TEXT_ORIGIN_Y*3+1+y), str );
        Convert3x3ToSprite( ps, rle[c], 2+sz+width/3, (sz+5)/9+height/3 );
    }
}

static void GenerateFarms()
{
    Bitmap bmp( HexWidth*3, HexHeight*3 );
    PS ps( bmp.ps() );
    ps.rgb_mode();
    for( int n = 0; n < NUM_FARMS; n++ )
    {
        ps.fill_rect( Point(0,0), Point(HexWidth*3,HexHeight*3), 
                      COLOR_TRANS );

        // First draw the background
        int L = 8*3;
        int R = 0;
        int mid_row = 13*3;
        for( int row = 2*3+2; row <= 21*3; row++ )
        {
            if( row >= 13*3 ) R = 24*3 + (18-24)*(row-13*3)/8;
            if( row <= 10*3 ) R = 18*3 + (24-18)*(row-2*3)/8;
            for( int col = L; col <= R; col++ )
            {
                int b = ByteRandom(64);
                if( (row+col)%6<=3 ) b += 64;
                int r = ByteRandom(128) + b;
                int g = ByteRandom(128) + r;
                // Local changes
                if( row > mid_row )
                {
                    if( col >= (L+R)/2 )
                        g = (g+255)/2;
                }
                else
                {
                    if( col <= (L*2+R)/3 )
                        g = ((row+col)/3)%2 ? 255 : 192;
                    else if( col <= (L+R*2)/3 )
                        r = g;
                }
                if( r > 255 ) r = 255;
                if( g > 255 ) g = 255;
                ps.pixel( Point( col, row ), iRGB(r,g,b) );
            }

            if( row >= 7*3 && row <= 17*3 )
                for( int col = 3*3; col <= 8*3+1; col++ )
                {
                    int g = ByteRandom(96);
                    if( (row+col)%6<=3 ) g += 64;
                    int r = ByteRandom(128) + g;
                    int b = r * 3 / 5;
                    if( r > 255 ) r = 255;
                    if( b > 255 ) b = 255;
                    ps.pixel( Point( col, row ), iRGB(r,g,b) );
                }
        }

        for( int x = 0; x < HexWidth*3; x++ )
            for( int y = 0; y < HexHeight*3; y++ )
            {
                Point p(x,y);
                Color c = ps.pixel(p);

                // Draw an overlay
                if( x >= 22 && c != COLOR_TRANS && (x*2+y) % 5 <= 2 )
                {
                    // Black, brown, green, red
                    int r = n+ByteRandom(NUM_FARMS)-ByteRandom(NUM_FARMS);
                    if( r < NUM_FARMS/8 )
                    {
                        // Blacken
                        if( (x*2+y) % 7 <= 1 )
                            ps.pixel( p, iRGB(0,0,0) );
                    }
                    else if( r >= NUM_FARMS*3/4 )
                    {
                        // Redden
                        if( (x+y*2) % 5 <= 2 )
                            ps.pixel( p, iRGB(255,64,64) );
                        else if( (x+y*2) % 5 == 3 || (x*2+y) % 5 == 2 )
                            ps.pixel( p, iRGB(0,0,0) );
                    }
                }
            }
        
        // Shade the edges
        ShadeEdges( ps, 60, 60 );

        Convert3x3ToSprite( ps, farm_rle[n], HexWidth, HexHeight );
        farm_rle[n].hotspot.x = HexWidth/2;
        farm_rle[n].hotspot.y = HexHeight/2;
    }
}

static void GenerateCursors()
{
    int multiplier = 6;
    double da = 2 * M_PI / multiplier;

    int color0[] = { 128,   0,   0 };
    int color1[] = { 192, 128,  64 };
    int color2[] = { 255, 255, 128 };

    for( int i = 0; i < NUM_CURSOR_FRAMES; i++ )
    {
        double angle0 = da * i / (1.0*NUM_CURSOR_FRAMES);

        HexTile& C = marktile;
        Bitmap bmp1( HexWidth, HexHeight );
        Bitmap bmp2( HexWidth, HexHeight );
        Bitmap bmp3( HexWidth, HexHeight );
        PS ps0( C.image()->ps() );
        PS ps1( bmp1.ps() );
        PS ps2( bmp2.ps() );
        PS ps3( bmp3.ps() );
        ps0.rgb_mode();
        ps1.rgb_mode();
        ps2.rgb_mode();
        ps3.rgb_mode();
        Color pink = ps0.pixel( Point(0,0) );
        Color interior = ps0.pixel( Point(HexWidth/2,HexHeight/2) );

        for( int x = 0; x < HexWidth; x++ )
            for( int y = 0; y < HexHeight; y++ )
            {
                Color c = ps0.pixel( Point(x,y) );
                ps1.pixel( Point(x,y), pink );
                ps2.pixel( Point(x,y), pink );
                ps3.pixel( Point(x,y), pink );

                if( c != pink && ( x != HexWidth/2 || y != HexHeight/2 ) )
                {
                    // Figure out what color to put here
                    double dx = x - 0.5*HexWidth;
                    double dy = y - 0.5*HexHeight;

                    double a = atan2(dy,dx);
                    double diffa = (i*1.0)/NUM_CURSOR_FRAMES;
                    double r = sqrt(dx*dx+dy*dy)/3.0 + (a/6.28)+diffa;
                    
                    if( c == interior )
                    {
                        int g = abs( int( 512*(r-int(r)-0.5) ) ) % 256;
                        
                        if( g < 40 || g > 210 )
                            ps3.pixel( Point(x,y), iRGB(g,g,g ) );
                        else if( g < 80 || g > 170 && ( x % 2 == y % 3 ) )
                            ps3.pixel( Point(x,y), iRGB(g,g,g ) );
                    }
                    else
                    {
                        double d = (a-angle0) / da;
                        int gradient = int(25600.0 + 256.0*d) % 256;
                        int *cL = color0, *cR = color1;
                        if( gradient > 128 )
                        {
                            gradient = gradient - 128;
                            cL = color1;
                            cR = color2;
                        }
                        gradient = 2 * gradient;

                        // Interpolate
                        int r = int( cL[0] * (256-gradient) + cR[0] * gradient ) / 256;
                        int g = int( cL[1] * (256-gradient) + cR[1] * gradient ) / 256;
                        int b = int( cL[2] * (256-gradient) + cR[2] * gradient ) / 256;
                        r = min(r,255);
                        g = min(g,255);
                        b = min(b,255);
                        r = max(r,0);
                        g = max(g,0);
                        b = max(b,0);
                        
                        ps1.pixel( Point(x,y), iRGB(r,g,b) );
                    }
                }
            }

        PixelBuffer buf( HexWidth, HexHeight );
        PSToBuffer( ps1, COLOR_TRANS, RLE_MASK, buf );
        cursor_rle[i].image = MakeRLE( buf, RLE_MASK );
        PSToBuffer( ps2, COLOR_TRANS, RLE_MASK, buf );
        cursor_rle[i].trans2 = MakeRLE( buf, RLE_MASK );
        PSToBuffer( ps3, COLOR_TRANS, RLE_MASK, buf );
        cursor_rle[i].trans1 = MakeRLE( buf, RLE_MASK );
    }
}

static int closest_neighbor( Point p )
{
    const int x0 = HexWidth/2;
    const int y0 = HexHeight/2;
    int x = p.x, y = p.y;
    static int neighbors[6][2] = {{0,2},{1,1},{1,-1},{0,-2},{-1,-1},{-1,1}};

    double bdist = 1.2*( (x0-x+0.5)*(x0-x+0.5) + (y0-y+0.5)*(y0-y+0.5) );
    int bd = 6;
    for( int d = 0; d < 6; d++ )
    {
        int x2 = x0 + (HexXSpacing+1)/2*neighbors[d][0];
        int y2 = y0 + HexYSpacing/4*neighbors[d][1];
        double dist2 = (x2-x+0.5)*(x2-x+0.5) + (y2-y+0.5)*(y2-y+0.5);
        if( dist2 < bdist ) { bd = d; bdist = dist2; }
    }
    return bd;
}

static bool inside_smallhex( Point p )
{
    const int x0 = 6;
    const int y0 = 6;
    int x = p.x, y = p.y;
    static int neighbors[6][2] = {{0,2},{1,1},{1,-1},{0,-2},{-1,-1},{-1,1}};

    double dist1 = (x0-x+0.5)*(x0-x+0.5) + (y0-y+0.5)*(y0-y+0.5);
    dist1 *= (1-0.01*ByteRandom(30));
    for( int d = 0; d < 6; d++ )
    {
        int x2 = x0 + (HexXSpacing+1)/2*neighbors[d][0];
        int y2 = y0 + HexYSpacing/4*neighbors[d][1];
        double dist2 = (x2-x+0.5)*(x2-x+0.5) + (y2-y+0.5)*(y2-y+0.5);
        if( dist1 > dist2 )
            return false;
    }
    return true;
}

static void GenerateWaterBitmaps()
{
    PS ps( newwatertiles.ps() );
    ps.rgb_mode();
    ps.fill_rect( Rect( 0, 0, WaterWidth, WaterHeight*64 ), COLOR_TRANS );

    for( int index = 0; index < 64; index++ )
    {
        for( int x = 0; x < WaterWidth; x++ )
            for( int y = 0; y < WaterHeight; y++ )
            {
                Point p(x,y);
                if( inside_smallhex(p) )
                {
                    int r =  64  - index/2 - ByteRandom(25);
                    int g = 192  - index*2 - ByteRandom(50);
                    int b = 255  - index*2 - ByteRandom(30);
                    if( b > 255 ) b = 255;
                    if( b < 0 ) b = 0;

                    // Some dithering
                    if( y%2==0 && x%2 == (y/2)%2 )
                    {
                        r = 2*r/3;
                        g = 2*g/3;
                        b = min(255,3*b/2);
                    }
                    
                    p.y += index*WaterHeight;
                    ps.pixel( p, iRGB(r,g,b) );
                }
            }

        PixelBuffer buf( WaterWidth, WaterHeight );
        PSToBuffer( ps, COLOR_TRANS, RLE_MASK, buf, 
                    0, index*WaterHeight );
        water_rle[index].image = MakeRLE( buf, RLE_MASK );
    }
}

static void GenerateTerrain()
{
    double weight[HexWidth][HexHeight];
    {
        // Initialize the weight array with `random' fluctuations
        for( int x = 0; x < HexWidth; x++ )
            for( int y = 0; y < HexHeight; y++ )
                weight[x][y] = sin(0.1*x+0.2*y) / 1000.0;
        weight[HexWidth/2][HexHeight/2] = -0.01;
    }

    Point ordering[HexWidth*HexHeight];
    for( int i = 0; i < HexWidth*HexHeight; i++ )
    {
        // Find the point that should be filled in next
        int bx = 0, by = 0;
        {
            // Find the spot with the lowest weight
            for( int x = 0; x < HexWidth; x++ )
                for( int y = 0; y < HexHeight; y++ )
                    if( weight[x][y] < weight[bx][by] )
                    {
                        bx = x;
                        by = y;
                    }
        }

        // Record this point in the array
        ordering[i] = Point(bx,by);

        // We don't want to draw over this point again
        weight[bx][by] += 10000.0;
                
        if( pink.ps().pixel( Point(bx,by) ) == pink.ps().pixel( Point(HexWidth/2,HexHeight/2) ) )
        {
            // Adjust the weights if the pixel just placed will not be masked
            for( int x = 0; x < HexWidth; x++ )
                for( int y = 0; y < HexHeight; y++ )
                {
#define DIST(dx,dy) (double(dx+bx-x)*double(dx+bx-x)+double(dy+by-y)*double(dy+by-y))
                    // Find the distances to the point and six translations
                    double d[] = {
                    DIST(0,0),
                    DIST(0,-HexHeight),
                    DIST(0,HexHeight),
                    DIST(HexXSpacing,-HexHeight/2),
                    DIST(HexXSpacing,HexHeight/2),
                    DIST(-HexXSpacing,-HexHeight/2),
                    DIST(-HexXSpacing,HexHeight/2)
                    };
#undef DIST
                    // Find the distance to the closest translation
                    double bd = d[0];
                    for( int dir = 1; dir <= 6; dir++ )
                        if( d[dir] < bd ) bd = d[dir];
                    double dw = 10.0/(1.0+bd);
                    weight[x][y] += dw;
                }
        }
    }

    // Generate sprinkles
    {
        HexTile terrains;
        terrains.assign_tile( IDB_TILE );
        PS ps( terrains.image()->ps() );
        ps.rgb_mode();
        ps.fill_rect( Point(0,0), Point(HexWidth,HexHeight), COLOR_TRANS );

        int i = 0;
        for( int n = 0; n < 16; n++ )
        {
            for( ; i < (n+1)*HexHeight*HexWidth/16; i++ )
                ps.pixel( ordering[i], iRGB(255,255,255) );

            int ROP_INVPAINT = 0x22;
            tile.bitblt( ps, 0, 0, ROP_INVPAINT );
            pink.bitblt( ps, 0, 0, ROP_SRCPAINT );
            
            PixelBuffer buf( HexWidth, HexHeight );
            PSToBuffer( ps, COLOR_TRANS, RLE_MASK, buf );
            sprinkles[n].image = MakeRLE( buf, RLE_MASK );
        }
    }

    // Notes:  DOMINUS uses these colors
    // Blues
    // 45 66 105
    // 51 72 111
    // 54 75 114
    // 59 80 120

    // Browns
    // 192 163 87
    // 181 156 82
    // 170 149 77
    // 160 142 72
    // 149 135 66
    // 138 128 61
    // 128 120 56
    // 117 113 50
    // 96 99 40
    // 85 92 34
    // 74 84 29
    // 64 77 24
    // 53 70 18
    // 42 63 13
    // 32 65 8

    // Generate terrain tiles

    struct { int point; Color c; double f; }
    table[] =
    {
    // 0, 73, 0
    // 0, 109, 85
    // 0, 109, 0
    // 43, 146, 0
    // 85, 109, 85 --
    // 85, 146, 85
    // 128, 182, 85
    // 128, 182, 0
    // 128, 146, 85 -- 70
    // 170, 182, 85 -- 85
    // 213, 182, 85
    // 213, 219, 170
    {  -5, iRGB(  0,109, 85) },
    {   0, iRGB( 43,146,  0) },
    {   3, iRGB( 85,109, 85) },
    {   7, iRGB( 43,146,  0) },
    {  15, iRGB( 43,146,  0) },
    {  25, iRGB( 85,146, 85) },
    {  40, iRGB(128,146, 85) },
    {  55, iRGB(128,128,  0) },
    {  70, iRGB(170,146, 85) },
    {  85, iRGB(170,182, 85) },
    {  90, iRGB(213,182, 85) },
    {  92, iRGB(213,219,170) },
    {  94, iRGB(213,255,170) },
    {  96, iRGB(230,255,167) },
    {  98, iRGB(240,240,240) },
    { 100, iRGB(255,255,255) },
    { 102, iRGB(255,255,255) },
    { 105, iRGB(255,255,255) }
    };
    int tablesize = sizeof(table)/sizeof(table[0]) - 1;
        
    for( int n = 0; n < NUM_TERRAIN_TILES; n++ )
    {
        // Do interpolation on the table to find two colors and a percentage
        // between them.  Rescale n to [0..100] as point:
        int point = n * 100 / NUM_TERRAIN_TILES;
        int best_i = 0;
        double total_f = 0.0;
        for( int i = 0; i < tablesize; i++ )
        {
            const double k = -0.003;
            table[i].f = exp(k*(point-table[i].point)*(point-table[i].point));
            if( table[i].f < 0.01 ) table[i].f = 0.0;
            if( table[i].f > table[best_i].f ) best_i = i;
            total_f += table[i].f;
        }

        // Fill in the bitmap
        HexTile terrains;
        terrains.assign_tile( IDB_TILE );
        PS ps( terrains.image()->ps() );
        ps.rgb_mode();

        for( int phase = 0; phase < 7; phase++ )
        {
            ps.fill_rect( Point(0,0), Point(HexWidth,HexHeight), table[best_i].c );

            if( phase != 6 )
                ps.fill_rect( Point(0,0), Point(HexWidth,HexHeight), COLOR_TRANS );
            
            // Fill in the hexagon using the ordering we selected
            int which_pixel = 0;
            for( int i = 0; i < tablesize; i++ )
            {
                int num_colored = int(HexWidth*HexHeight*table[i].f/total_f);
                for( int j = 0; j < num_colored; j++ )
                {
                    // Draw the pixel
                    if( closest_neighbor( ordering[which_pixel] ) == phase )
                        ps.pixel( ordering[which_pixel], table[i].c );
                    else
                        ps.pixel( ordering[which_pixel], COLOR_TRANS );
                    if( which_pixel < HexWidth*HexHeight )
                        which_pixel++;
                }
            }

            // Mask off the hexagon
            int ROP_INVPAINT = 0x22;
            tile.bitblt( ps, 0, 0, ROP_INVPAINT );
            pink.bitblt( ps, 0, 0, ROP_SRCPAINT );

            PixelBuffer buf( HexWidth, HexHeight );
            PSToBuffer( ps, COLOR_TRANS, RLE_MASK, buf );
            terrain_rle[n+phase*NUM_TERRAIN_TILES].image = MakeRLE( buf, RLE_MASK );
        }
    }
}

static void GenerateTrains()
{
    Bitmap bmp( HexWidth*3, HexHeight*3 );
    PS ps( bmp.ps() );
    ps.rgb_mode();
    for( int angle = 0; angle < NUM_ANGLES; ++angle )
    {
        ps.fill_rect( Point(0,0), Point(HexWidth*3,HexHeight*3), 
                      COLOR_TRANS );

        double vx = cos(pi*angle/NUM_ANGLES), vy = sin(pi*angle/NUM_ANGLES);
        double ux = vy, uy = -vx;

        for( int i = -22; i <= 22; ++i )
            for( int j = -12; j <= 12; ++j )
            {
                // Saw off corners
                if( abs(i) + abs(j) > 22+12-3 ) continue;

                // Find the correct point by using the two basis vectors
                double x = vx*i + ux*j;
                double y = vy*i + uy*j;

                Point p(int(x)+3*HexWidth/2, int(y)+3*HexHeight/2);

                int col = iRGB(240,100,90);
                if( (i+j/3+100)%7==0 )
                    col = iRGB(100,30,30);
                if( (i+100)%7<2 && abs(abs(j)-9)<2)
                    col = iRGB(0,0,0);
                ps.pixel( p, col );
            }
        
        // Shade the edges
        ShadeEdges( ps, 60, 60 );

        Convert3x3ToSprite( ps, trains[angle], HexWidth, HexHeight );
        trains[angle].hotspot.x = HexWidth/2;
        trains[angle].hotspot.y = HexHeight/2;
    }
}

void InitializeBitmaps()
{
#if INIT_BITMAPS == 13
    GenerateAltFont( text12.data, font_bmp );
    SaveRleArray( "Data/Font12.RLE", text12.data, 256 );
    GenerateAltFont( text12i.data, fonti_bmp );
    SaveRleArray( "Data/Font12I.RLE", text12i.data, 256 );
#define BIG_FONT 1

#if 0
#define ITALIC_FONT 1
    //  const char* face1 = "ITC Franklin Gothic Heavy";
    const char* face1 = "ITC Avant Garde Gothic Medium";
    GenerateFont( face1, 36, text12.data, 0 );
    GenerateFont( face1, 36, text12.data+128, 1 );
    SaveRleArray( "Data/Font12.RLE", text12.data, 256 );
#if ITALIC_FONT
    // const char* face2 = "ITC Franklin Gothic Heavy Itali";
    const char* face2 = "ITC Avant Garde Gothic Medium O";
    GenerateFont( face2, 36, text12i.data, 0 );
    GenerateFont( face2, 36, text12i.data+128, 1 );
    SaveRleArray( "Data/Font12I.RLE", text12i.data, 256 );
#endif

#endif

#if BIG_FONT
    const char* face3 = "Bitstream Cooper Black";
    GenerateFont( face3, 72, text24.data, 0 );
    GenerateFont( face3, 72, text24.data+128, 2 );
    SaveRleArray( "Data/Font24.RLE", text24.data, 256 );
#endif

#endif

    switch( INIT_BITMAPS )
    {
      case 3: GenerateTrains(); break;
      case 4: GenerateTerrain(); break;
      case 6: GenerateCursors(); break;
      case 8: GenerateWaterBitmaps(); break;
      case 9: GenerateHouseBitmaps(); break;
      case 10:
      case 11:
      case 12: GenerateOwnerOverlay(); break;
      case 14: GenerateFarms(); break;
      case 15: GenerateTreeBitmaps(); break;
    }
}

class Hexagon
{
    int x, y, s, c;

  public:
    static double height;

    Hexagon( int x_, int y_, int side_, int color_ )
        :x(x_), y(y_), s(side_), c(color_) {}

    void draw( PS& ps );
};

double Hexagon::height = sqrt(3)/2.0;

void Hexagon::draw( PS& ps )
{
    POINTL pt[6];
    int hn = int( - height * s );
    int hp = int( height * 2 * s ) + hn;
    int sn = -(s/2);
    int sp = s + sn;
    pt[0].x = -s;   pt[0].y =  -1;
    pt[1].x =  sn;  pt[1].y =  hp-1;
    pt[2].x =  sp-1;pt[2].y =  hp-1;
    pt[3].x =  s-1; pt[3].y =  -1;
    pt[4].x =  sp-1;pt[4].y =  hn;
    pt[5].x =  sn;  pt[5].y =  hn;
    for( int i = 0; i < 6; i++ )
    {
        pt[i].x += x;
        pt[i].y += y;
    }

    ps.color( c );
    GpiMove( ps, &pt[5] );
    GpiPolyLine( ps, 6, pt );
}

Color BestColorMatch( long palette[], int r1, int g1, int b1 );

#if INIT_BITMAPS == 5
static void GenerateRGBTable()
{
    CachedPS dps( HWND_DESKTOP );
    long rgbtable[256];
    static byte transp[256][256];
    
    GpiQueryRealColors( dps, 0, 0, 256, rgbtable );

    DosBeep( 500, 100 );
    
    for( int i = 0; i < 256; i++ )
        for( int j = 0; j < 256; j++ )
        {
            int r1 = ColorToRed( rgbtable[i] );
            int g1 = ColorToGreen( rgbtable[i] );
            int b1 = ColorToBlue( rgbtable[i] );
            int r2 = ColorToRed( rgbtable[j] );
            int g2 = ColorToGreen( rgbtable[j] );
            int b2 = ColorToBlue( rgbtable[j] );
            
            // Weight color components from 0 to 3 (0% to 100%)
            // Transparency table is 33% + 67%

            int r = (r1+r2*2+1)/3;
            int g = (g1+g2*2+1)/3;
            int b = (b1+b2*2+1)/3;
            
            transp[i][j] = BestColorMatch( rgbtable, r, g, b );
        }

    // Write the first (ascii) file
    FILE* f = fopen( "RGBTable.inc", "wt" );
    fprintf( f, "unsigned long DesktopRGB[256] = \n{" );
    for( int i = 0; i < 256; i++ )
    {
        if( i % 8 == 0 )
            fprintf( f, "\n  " );
        fprintf( f, "0x%06X", rgbtable[i] );
        if( i != 255 )
            fprintf( f, "," );
    }
    fprintf( f, "\n};\n\n" );
    DosBeep( 700, 100 );

    fprintf( f, "byte Transparency1[256][256] = \n{\n" );
    for( int i = 0; i < 256; i++ )
    {
        fprintf( f, " {" );
        for( int j = 0; j < 256; j++ )
        {
            if( j % 16 == 0 )
                fprintf( f, "\n  " );
            fprintf( f, "%3d", transp[i][j] );
            if( j != 256 )
                fprintf( f, "," );
        }
        fprintf( f, "\n }" );
        if( i != 256 )
            fprintf( f, "," );
        fprintf( f, "\n" );
    }
    fprintf( f, "\n};\n\n" );

    fclose( f );

    // Write a second (binary) file
    f = fopen( "Data/RGBTable.dat", "wb" );
    fwrite( rgbtable, sizeof(rgbtable[0]), 256, f );
    fwrite( transp, sizeof(transp[0][0]), 65536, f );
    fclose( f );
}
#endif

void alt_main()
{
    randomize();

#if INIT_BITMAPS != 5
    // Load the color tables
    FILE* f = fopen( "Data/RGBTable.dat", "rb" );
    fread( DesktopRGB, sizeof(DesktopRGB[0]), 256, f );
    fread( Transparency1, sizeof(Transparency1[0]), 65536, f );
    fclose(f);
    
    HexTile scorched;

    tile.load( IDB_MASK );
    pink.load( IDB_PINK );
#if INIT_BITMAPS == 13
    font_bmp.load( ID_FONTS, 460, 312 );
    fonti_bmp.load( ID_FONTS_I, 460, 312 );
#elif INIT_BITMAPS == 7
    background_bmp.load( ID_BACKGROUND, 40, 40 );
#endif

    marktile.assign_tile( IDB_MARKED );

    clear.assign_tile( IDB_TILE );
    tower.assign_tile( IDB_TOWER );
    fire1.assign_tile( IDB_FIRE1 );
    fire2.assign_tile( IDB_FIRE2 );
    scorched.assign_tile( IDB_SCORCHED );
    canal.assign_tile( IDB_CANAL );
    lava.assign_tile( IDB_LAVA );
    market.assign_tile( IDB_MARKET );

    HexTile gate; gate.assign_tile( IDB_GATE );
    HexTile clear; clear.assign_tile( IDB_CLEAR );
    HexTile wall; wall.assign_tile( IDB_WALL );
    HexTile bridge; bridge.assign_tile( IDB_BRIDGE );

    newwatermask.load( IDB_NEWWATERMASK, WaterWidth, WaterHeight );
    newwatertiles.load( WaterWidth, WaterHeight*64 );

    InitializeBitmaps();
#endif

#if INIT_BITMAPS == 9
    SaveRleArray( "Data/Houses.RLE", house_rle, 8 );
#elif INIT_BITMAPS == 6
    SaveRleArray( "Data/Cursor.RLE", cursor_rle, NUM_CURSOR_FRAMES );
#elif INIT_BITMAPS == 10
    SaveRleArray( "Data/Overlay.RLE", overlay_rle, 64 );
#elif INIT_BITMAPS == 11
    SaveRleArray( "Data/Walls.RLE", wall_rle, 64 );
#elif INIT_BITMAPS == 12
    SaveRleArray( "Data/Roads.RLE", road_rle, 64 );
#elif INIT_BITMAPS == 14 
    SaveRleArray( "Data/Farms.RLE", farm_rle, NUM_FARMS );
#elif INIT_BITMAPS == 15
    SaveRleArray( "Data/Trees.RLE", trees_rle, NUM_TREES*NUM_TREE_SEASONS );
#elif INIT_BITMAPS == 4
    SaveRleArray( "Data/Terrain.RLE", terrain_rle, NUM_TERRAIN_TILES*7 );
    SaveRleArray( "Data/Sprinkle.RLE", sprinkles, 16 );
#elif INIT_BITMAPS == 5
    GenerateRGBTable();
#elif INIT_BITMAPS == 3
    SaveRleArray( "Data/Trains.RLE", trains, NUM_ANGLES );
#elif INIT_BITMAPS == 7
#define CONV(B,R) { PSToBuffer( B->ps(), transparent, RLE_MASK, buf ); R.image = MakeRLE( buf, RLE_MASK ); SaveRLE( R.image, f ); }
    f = fopen( "Data/Other.RLE", "wb" );
    PixelBuffer buf( HexWidth, HexHeight );

    // Figure out what the transparency color is
    marktile.image()->ps().rgb_mode();
    Color transparent = marktile.image()->ps().pixel( Point(0,0) );

    CONV( clear.image(), tile_rle );
    CONV( marktile.image(), marktile_rle );
    CONV( tree.image(), lone_tree_rle );
    CONV( tower.image(), tower_rle );
    CONV( fire1.image(), fire1_rle );
    CONV( fire2.image(), fire2_rle );
    CONV( canal.image(), canal_rle );
    CONV( lava.image(), lava_rle );
    CONV( scorched.image(), scorched_rle );
    CONV( clear.image(), clear_rle );
    CONV( gate.image(), gate_rle );
    CONV( wall.image(), wallbmp_rle );
    CONV( bridge.image(), bridge_rle );
    CONV( market.image(), market_rle );
    
    {
        PixelBuffer buf( 40, 40 );  
        CONV( (&background_bmp), background_rle );
    }

    PixelBuffer buf2( 16, 16 );

    pink.load( IDB_SOLDIER, 16, 16 );
    PSToBuffer( pink.ps(), transparent, RLE_MASK, buf2 );
    soldier_rle.image = MakeRLE( buf2, RLE_MASK );
    SaveRLE( soldier_rle.image, f );

    pink.load( IDB_WORKER, 16, 16 );
    PSToBuffer( pink.ps(), transparent, RLE_MASK, buf2 );
    worker_rle.image = MakeRLE( buf2, RLE_MASK );
    SaveRLE( worker_rle.image, f );

    pink.load( IDB_FIREFIGHTER, 16, 16 );
    PSToBuffer( pink.ps(), transparent, RLE_MASK, buf2 );
    firefighter_rle.image = MakeRLE( buf2, RLE_MASK );
    SaveRLE( firefighter_rle.image, f );

    PixelBuffer buf3( 32, 32 );
    pink.load( IDB_HAND, 32, 32 );
    PSToBuffer( pink.ps(), transparent, RLE_MASK, buf3 );
    hand_rle.image = MakeRLE( buf3, RLE_MASK );
    SaveRLE( hand_rle.image, f );

    fclose( f );
#undef CONV
#elif INIT_BITMAPS == 8
    f = fopen( "Data/Water.RLE", "wb" );
    for( int i = 0; i < 64; i++ )
        SaveRLE( water_rle[i].image, f );
    fclose( f );
#endif
}

#endif

void LoadBitmaps()
{
    // tile.load( IDB_MASK );
    // pink.load( IDB_PINK );

    // bigblob.load( ID_BIGBLOB, 240, 280 );
    // smallblob.load( ID_SMALLBLOB, 40, 40 );
    // background_bmp.load( ID_BACKGROUND, 40, 40 );

    FILE* f;
    int i;

    f = fopen( "Data/RGBTable.dat", "rb" );
    fread( DesktopRGB, sizeof(DesktopRGB[0]), 256, f );
    fread( Transparency1, sizeof(Transparency1[0]), 65536, f );
    fclose(f);
    
    f = fopen( "Data/Water.RLE", "rb" );
    if( !f ) Throw("Could not open Data/Water.RLE");
    for( i = 0; i < 64; i++ )
    {
        water_rle[i].image = LoadRLE( f );
        water_rle[i].hotspot.x = WaterWidth/2;
        water_rle[i].hotspot.y = WaterHeight/2;
    }
    fclose( f );

    LoadRleArray( "Data/Terrain.RLE", terrain_rle, NUM_TERRAIN_TILES*7 );
    LoadRleArray( "Data/Sprinkle.RLE", sprinkles, 16 );
    LoadRleArray( "Data/Houses.RLE", house_rle, 8 );
    LoadRleArray( "Data/Roads.RLE", road_rle, 64 );
    LoadRleArray( "Data/Walls.RLE", wall_rle, 64 );
    LoadRleArray( "Data/Overlay.RLE", overlay_rle, 64 );
    LoadRleArray( "Data/Font12.RLE", text12.data, 256, 
                  TEXT_ORIGIN_X, TEXT_ORIGIN_Y );
    LoadRleArray( "Data/Font12I.RLE", text12i.data, 256, 
                  TEXT_ORIGIN_X, TEXT_ORIGIN_Y );
    LoadRleArray( "Data/Font24.RLE", text24.data, 256, 
                  TEXT_ORIGIN_X, TEXT_ORIGIN_Y );
    LoadRleArray( "Data/Farms.RLE", farm_rle, NUM_FARMS );
    LoadRleArray( "Data/Cursor.RLE", cursor_rle, NUM_CURSOR_FRAMES );
    LoadRleArray( "Data/Trains.RLE", trains, NUM_ANGLES );
    LoadRleArray( "Data/Trees.RLE", trees_rle, NUM_TREES*NUM_TREE_SEASONS );
    f = fopen( "Data/Other.RLE", "rb" );
    if( !f ) Throw("Could not open Data/Other.RLE");
#define LOAD(I) { I##_rle.image = LoadRLE( f ); I##_rle.hotspot.x = HexWidth/2, I##_rle.hotspot.y = HexHeight/2; }
    LOAD(tile);
    LOAD(marktile);
    marktile_rle.trans1 = marktile_rle.image;
    marktile_rle.image = NULL;
    LOAD(lone_tree);
    LOAD(tower);
    LOAD(fire1);
    LOAD(fire2);
    LOAD(canal);
    LOAD(lava);
    LOAD(scorched);
    LOAD(clear);
    LOAD(gate);
    LOAD(wallbmp);
    LOAD(bridge);
    LOAD(market);
    background_rle.image = LoadRLE( f );
    background_rle.hotspot.x = 0;
    background_rle.hotspot.y = 0;
    soldier_rle.image = LoadRLE( f );
    soldier_rle.hotspot.x = 8;
    soldier_rle.hotspot.y = 8;
    worker_rle.image = LoadRLE( f );
    worker_rle.hotspot.x = 8;
    worker_rle.hotspot.y = 8;
    firefighter_rle.image = LoadRLE( f );
    firefighter_rle.hotspot.x = 8;
    firefighter_rle.hotspot.y = 8;
    hand_rle.image = LoadRLE( f );
    hand_rle.hotspot.x = 16;
    hand_rle.hotspot.y = 16;
#undef LOAD
    fclose( f );
}
