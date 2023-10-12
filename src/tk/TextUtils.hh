// TextUtils.hh for Shynebox Window Manager

/*
  Not to be confused with StringUtils.
  The C portion helps calculate and align text.
  The header providess rotation helpers for text and other objects.
*/

#ifndef TK_TEXTUTILS_HH
#define TK_TEXTUTILS_HH

#include "Orientation.hh"

namespace tk {

class Font;

/**
   Aligns the text after max_pixels and bevel
 */
int doAlignment(int max_pixels, int bevel, tk::Justify justify,
                const tk::Font &font, const char * const text,
                unsigned int textlen, unsigned int &newlen);

// translate coordinates from ROT0 into different orientations
// coords are relative to rot0 0,0 position
// Need width and height of the area being rotated (in ROT0 coords)
inline void translateCoords(Orientation orient, int &x, int &y, unsigned int w, unsigned int h) {
  int orig_x = x;
  int orig_y = y;

  switch(orient) {
//  case ROT180: // unreachable
//    x = w - orig_x;
//    y = h - orig_y;
//    break;
  case ROT90:
    x = h - orig_y;
    y = orig_x;
    break;
  case ROT270:
    x = orig_y;
    y = w - orig_x;
  default:
  case ROT0:
  case ROT180:
    break;
  }
}

// XmbFont only user
inline void untranslateCoords(Orientation orient, int &orig_x, int &orig_y,
                                          unsigned int w, unsigned int h) {
  int x = orig_x;
  int y = orig_y;

  switch(orient) {
//  case ROT180: // unreachable
//    orig_x = w - x;
//    orig_y = h - y;
//    break;
  case ROT90:
    orig_y = h - x;
    orig_x = y;
    break;
  case ROT270:
    orig_y = x;
    orig_x = w - y;
  default:
  case ROT0:
  case ROT180:
    break;
  }
}

// When positioning an X11 box inside another area, we need to
// relocate the x,y coordinates
inline void translatePosition(Orientation orient, int &x, int &y,
               unsigned int w, unsigned int h, unsigned int bw) {
  switch(orient) {
//  case ROT180: // unreachable
//    x -= w + 2*bw;
//    y -= h + 2*bw;
//    break;
  case ROT90:
    x -= h + 2*bw;
    break;
  case ROT270:
    y -= w + 2*bw;
  default:
  case ROT0:
  case ROT180:
    break;
  }
}

// translate horizontal to vertical
inline void translateSize(Orientation orient, unsigned int &w, unsigned int &h) {
  if (orient <= ROT180) // horz
    return;

  unsigned int tmp;
  tmp = w;
  w = h;
  h = tmp;
}

} // end namespace tk

#endif // TK_TEXTUTILS_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2002 - 2003 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
