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

// This is just a convenient place to put all my common #includes

#define INCL_DOS
#define INCL_PM                 
#define INCL_GPI
#include <os2.h>                // PM includes
#include "stl.h"                // STL
#include <string.h>             // String functions
#include <math.h>               // Math functions
#include <time.h>               // Time (clock())
#include <stdio.h>              // For debugging, mostly
#include <stdlib.h>             // Miscellaneous
#include <cstring.h>            // C++ String library

#include "Closure.h"            // Command pattern
#include "Subject.h"            // Subject/Observer pattern

// The following line is used for Borland C++'s precompiled headers
#pragma hdrstop
