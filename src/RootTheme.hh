// RootTheme.hh for Shynebox Window Manager

/*
  Contains border color, border size, bevel width
  and opGC for objects like geometry window in BScreen.
  Also holds background pixmap generated.
*/

#ifndef ROOTTHEME_HH
#define ROOTTHEME_HH

#include "tk/Theme.hh"
#include "tk/GContext.hh"

class BackgroundItem;
class BScreen;

namespace tk {
class ResourceManager;
class ImageControl;
}

class RootTheme: public tk::Theme, public tk::ThemeProxy<RootTheme> {
public:
  RootTheme(tk::ImageControl &image_control);
  ~RootTheme();

  bool fallback(tk::ThemeItem_base &item);
  void reconfigTheme();
  void reset() { m_first = true; reconfigTheme(); }

  GC opGC() const { return m_opgc.gc(); }

  virtual RootTheme &operator *() { return *this; }
  virtual const RootTheme &operator *() const { return *this; }

private:
  BackgroundItem *m_background; // background image/texture
  tk::GContext m_opgc;
  bool m_first;
};

#endif // ROOTTHEME_HH

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
