// TextureRender.hh for Shynebox Window Manager

/*
  Renders texture to pixmap.
  This is used with ImageControl to render textures
*/

#ifndef TK_TEXTURRENDER_HH
#define TK_TEXTURRENDER_HH

#include "Orientation.hh"

#include <X11/Xlib.h>

namespace tk {

class ImageControl;
class Texture;

struct RGBA;
class TextureRender {
public:
  TextureRender(ImageControl &ic, unsigned int width, unsigned int height,
                Orientation orient = ROT0);
  ~TextureRender();
  // render to pixmap
  Pixmap render(const tk::Texture &src_texture);
  // render solid texture to pixmap
  Pixmap renderSolid(const tk::Texture &src_texture);
  // render gradient texture to pixmap
  Pixmap renderGradient(const tk::Texture &src_texture);
  // scales and renders a pixmap
  Pixmap renderPixmap(const tk::Texture &src_texture);
private:
  // allocates red, green and blue for gradient rendering
  void allocateColorTables();

  Pixmap renderPixmap();
  XImage *renderXImage();

  ImageControl &control;

  int cpc, cpccpc;

  RGBA* rgba;
  Orientation orientation;
  unsigned int width, height;
};

} // end namespace tk
#endif // TK_TEXTURERENDER_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2002 - 2003 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Image.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes at tcac.net)
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
