// PixmapWithMask.hh for Shynebox Window Manager

/*
  Adds a mask (see/pass through area) to pixmaps.
*/

#ifndef TK_PIXMAPWITHMASK_HH
#define TK_PIXMAPWITHMASK_HH

#include "SbPixmap.hh"

namespace tk {

class PixmapWithMask {
public:
  PixmapWithMask() { }
  PixmapWithMask(Pixmap pm, Pixmap mask):m_pixmap(pm), m_mask(mask) { }

  void scale(unsigned int width, unsigned int height) {
    pixmap().scale(width, height);
    mask().scale(width, height);
  }
  unsigned int width() const { return m_pixmap.width(); }
  unsigned int height() const { return m_pixmap.height(); }
  SbPixmap &pixmap() { return m_pixmap; }
  SbPixmap &mask() { return m_mask; }

  const SbPixmap &pixmap() const { return m_pixmap; }
  const SbPixmap &mask() const { return m_mask; }

private:
  SbPixmap m_pixmap;
  SbPixmap m_mask;
};

} // end namespace tk

#endif // TK_PIXMAPWITHMASK_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
