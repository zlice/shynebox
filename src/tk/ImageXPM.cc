// ImageXPM.cc for Shynebox Window Manager

#include "ImageXPM.hh"

#include "App.hh"
#include "PixmapWithMask.hh"

#include <X11/xpm.h>

namespace tk {

ImageXPM::ImageXPM() {
  Image::registerType("XPM", *this);
}

PixmapWithMask *ImageXPM::load(const std::string &filename, int screen_num) const {
  XpmAttributes xpm_attr;
  xpm_attr.valuemask = 0;
  Display *dpy = tk::App::instance()->display();
  Pixmap pm = 0, mask = 0;
  int retvalue = XpmReadFileToPixmap(dpy,
                                     RootWindow(dpy, screen_num),
                                     const_cast<char *>(filename.c_str() ),
                                     &pm,
                                     &mask, &xpm_attr);
  if (retvalue == 0) // success
    return new PixmapWithMask(pm, mask);
  else
    return 0;
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
