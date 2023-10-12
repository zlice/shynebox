// CachedPixmap.cc for Shynebox Window Manager

#include "CachedPixmap.hh"
#include "ImageControl.hh"

namespace tk {

CachedPixmap::CachedPixmap(tk::ImageControl& ctrl):
  m_pixmap(0),
  m_ctrl(ctrl) {
}

CachedPixmap::CachedPixmap(tk::ImageControl& ctrl, Pixmap pm):
  m_pixmap(pm),
  m_ctrl(ctrl) { }
// CachePixmap class inits

CachedPixmap::~CachedPixmap() {
  destroy();
} // CachePixmap class destroy

void CachedPixmap::reset(Pixmap pm) {
  destroy();
  m_pixmap = pm;
}

void CachedPixmap::destroy() {
  if (m_pixmap != 0)
    m_ctrl.removeImage(m_pixmap);
}

} // namespace tk

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
