// GContext.cc for Shynebox Window Manager

#include "GContext.hh"

#include "App.hh"
#include "SbDrawable.hh"
#include "SbPixmap.hh"
#include "Color.hh"
#include "Font.hh"

namespace tk {

Display *GContext::m_display = 0;

GContext::GContext(const tk::SbDrawable &drawable):
    m_gc(XCreateGC(drawable.display(), drawable.drawable(), 0, 0) ) {
  if (m_display == 0)
    m_display = drawable.display();

  setGraphicsExposure(false);
}

GContext::GContext(Drawable drawable):
      m_gc(XCreateGC(m_display != 0 ? m_display : tk::App::instance()->display(),
                     drawable, 0, 0) ) {
  if (m_display == 0)
    m_display = tk::App::instance()->display();
  setGraphicsExposure(false);
}

GContext::GContext(Drawable d, const GContext &gc):
    m_gc(XCreateGC(m_display != 0 ? m_display : tk::App::instance()->display(),
                   d, 0, 0) ) {
  if (m_display == 0)
    m_display = tk::App::instance()->display();
  setGraphicsExposure(false);
  copy(gc);
}
// GContext class inits

GContext::~GContext() {
  if (m_gc)
    XFreeGC(m_display, m_gc);
} // GContext class destroy

/// not implemented!
//void GContext::setFont(const tk::Font &font) {
    //!! TODO
//}
void GContext::copy(GC gc) {
  // copy gc with mask: all
  XCopyGC(m_display, gc, ~0, m_gc);
}

void GContext::copy(const GContext &gc) {
  // copy X gc
  copy(gc.gc() );

  //!! TODO: copy our extended gcontext
}

} // end namespace tk

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
