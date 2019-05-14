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

// I don't remember why these are in a header file.

inline double FN(double x) { return (x); }

// This routine moves land upwards and then uses water to
// erode the landscape back down.  By repeating this process
// we can hopefully get valleys and hills.
static void uplift_terrain( Map& map, int count,
                            Closure<bool,const char *> pause )
{
    int m, n, M=Map::MSize, N=Map::NSize;
    double alt[Map::MSize][Map::NSize];
    double threshold = 3.0;
    int drainage[Map::MSize][Map::NSize];
    
    for( m = 0; m < M; m++ )
        for( n = 0; n < N; n++ )
        {
            alt[m][n] = map.altitude( HexCoord(m+1,n+1) );
            drainage[m][n] = 1;
        }

    for( int i = 0; i < count; i++ )
    {
        // Figure out what the largest slope is
        if( pause("Analyze") ) return;
        double max_slope = 0.0;
        for( m = 0; m < M-1; m++ )
            for( n = 0; n < N-1; n++ )
            {
                double s1 = fabs(alt[m][n]-alt[m+1][n]);
                double s2 = fabs(alt[m][n]-alt[m][n+1]);
                s1 = s1 / FN(1.0*drainage[m][n]);
                s2 = s2 / FN(1.0*drainage[m][n]);
                if( s1 > max_slope ) max_slope = s1;
                if( s2 > max_slope ) max_slope = s2;
            }
        
        // Then increase the slope everywhere
        if( max_slope < 1.5*threshold && max_slope > 0.0 )
        {
            double mult = 1.1*threshold / max_slope;

            char s[256];
            sprintf(s,"Uplift:   %6.3f, %6.3f",mult,max_slope);
            if( pause(s) ) return;
            
            for( m = 0; m < M; m++ )
                for( n = 0; n < N; n++ )
                    alt[m][n] *= mult;
        }

        // Calculate the drainage area per square
        if( i % 8 == 0 || i < 3 )
        {
            if( pause("Drainage") ) return;         
            for( m = 0; m < M; m++ )
                for( n = 0; n < N; n++ )
                    drainage[m][n] = 1;
            for( m = 0; m < M; m++ )
            {
                for( n = 0; n < N; n++ )
                {
                    int m1 = m, n1 = n;

                    while( m1 != -1 )
                    {
                        static int dir[4][2] = {{-1,0},{0,-1},{1,0},{0,1}};
                        int bd = -1;
                        double bs = 0.001*threshold;
                        int bd2 = -1;
                        double bs2 = -1000.0;
                        for( int d = 0; d < 4; d++ )
                        {
                            int m2 = m1+dir[d][0];
                            int n2 = n1+dir[d][1];
                            bool valid = (m2>=0&&m2<M) && (n2>=0&&n2<N);
                            double s = valid?(alt[m1][n1]-alt[m2][n2]):0.0;
                            if( s >= bs ) { bd = d; bs = s; }
                            if( s >= bs2 ) { bd2 = d; bs2 = s; }
                        }
                    
                        if( bd != -1 )
                        {
                            int m2 = m1+dir[bd][0];
                            int n2 = n1+dir[bd][1];
                            bool valid = (m2>=0&&m2<M) && (n2>=0&&n2<N);
                            if( valid )
                            {
                                drainage[m2][n2]++;
                                m1 = m2;
                                n1 = n2;
                            }
                            else
                                m1 = -1;
                        }
                        else
                        {
                            // Change one of the points to be lower
                            if( bd2 != -1 )
                            {
                                int m2 = m1+dir[bd2][0];
                                int n2 = n1+dir[bd2][1];
                                bool valid = (m2>=0&&m2<M) && (n2>=0&&n2<N);
                                if( valid && alt[m1][n1] < alt[m2][n2] )
                                {
                                    alt[m2][n2] -= 1.1*(alt[m2][n2]-alt[m1][n1]);
                                    if( alt[m2][n2] > alt[m1][n1] )
                                        DosBeep(100,10);
                                }
                            }
                            m1 = -1;
                        }
                    }
                }
            }
        }
        
        // Now for each large slope, perform some erosion
        if( pause("Erosion") ) return;
        for( m = 0; m < M; m++ )
            for( n = 0; n < N; n++ )
            {
                int m1 = m, n1 = n;

                while( m1 != -1 )
                {
                    static int dir[4][2] = {{-1,0},{0,-1},{1,0},{0,1}};
                    int bd = -1;
                    double bs = threshold * FN(1.0*drainage[m1][n1]);
                    for( int d = 0; d < 4; d++ )
                    {
                        int m2 = m1+dir[d][0];
                        int n2 = n1+dir[d][1];
                        bool valid = (m2>=0&&m2<M) && (n2>=0&&n2<N);
                        double s = valid?(alt[m1][n1]-alt[m2][n2]):0.0;
                        if( s >= bs ) { bd = d; bs = s; }
                    }
                    
                    if( bd != -1 )
                    {
                        int m2 = m1+dir[bd][0];
                        int n2 = n1+dir[bd][1];
                        bool valid = (m2>=0&&m2<M) && (n2>=0&&n2<N);
                        alt[m1][n1] -= bs*0.5;
                        if( valid )
                        {
                            alt[m2][n2] += bs*0.5;
                            m1 = m2;
                            n1 = n2;
                        }
                        else
                            m1 = -1;
                    }
                    else
                        m1 = -1;
                }
            }

        // Transfer altitudes to the map array
        double min_alt = alt[0][0], max_alt = alt[0][0];
        for( m = 0; m < M; m++ )
            for( n = 0; n < N; n++ )
            {
                double a = alt[m][n];
                if( a < min_alt ) min_alt = a;
                if( a > max_alt ) max_alt = a;
            }
        for( m = 0; m < M; m++ )
            for( n = 0; n < N; n++ )
            {
                double a = (alt[m][n]-min_alt)/(max_alt-min_alt);
                HexCoord h(m+1,n+1);
                map.set_altitude( h, int(a*NUM_TERRAIN_TILES) );
                map.set_water( h, (drainage[m][n]-1)/5 );
            }
    }
}

