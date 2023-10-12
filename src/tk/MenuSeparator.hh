// MenuSeparator.hh for Shynebox Window Manager

/*
  A simple MenuItem that draws a separator line.
*/

#ifndef MENUSEPARATOR_HH
#define MENUSEPARATOR_HH

#include "MenuItem.hh"

namespace tk {

class MenuSeparator: public MenuItem {
public:
  virtual void draw(SbDrawable &drawable,
                    const tk::ThemeProxy<MenuTheme> &theme,
                    bool highlight, bool draw_background, // NOTE: highlight unused
                    int x, int y,
                    unsigned int width, unsigned int height) const;
  virtual bool isEnabled() const { return false; }
};

} // end namespace tk

#endif // MENUSEPARATOR_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2004 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                and Simon Bowden (rathnor at users.sourceforge.net)
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
