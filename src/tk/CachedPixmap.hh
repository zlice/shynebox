// CachedPixmap.hh for Shynebox Window Manager

/*
  Holds cached pixmap and releases it from cache when it dies.
*/

#ifndef TK_CACHED_PIXMAP_H
#define TK_CACHED_PIXMAP_H

#include <X11/Xlib.h>

namespace tk {

class ImageControl;

class CachedPixmap {
public:
  explicit CachedPixmap(tk::ImageControl& ctrl);
  CachedPixmap(tk::ImageControl& ctrl, Pixmap pm);
  ~CachedPixmap();

  operator Pixmap() const {
    return m_pixmap;
  }

  void reset(Pixmap pm);

  Pixmap operator *() const {
    return m_pixmap;
  }

public:
  void destroy(); // releases pixmap from cache

  Pixmap m_pixmap;          // cached pixmap
  tk::ImageControl &m_ctrl; // cache control
};

} // namespace CachedPixmap

#endif // TK_CACHED_PIXMAP

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2006 Fluxbox Team (fluxgen at fluxbox dot org)
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
