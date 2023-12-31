// ButtonTheme.hh for Shynebox Window Manager

/*
  Theme for Buttons
*/

#ifndef BUTTONTHEME_HH
#define BUTTONTHEME_HH

#include "ToolTheme.hh"

#include "tk/GContext.hh"

class ButtonTheme: public ToolTheme, public tk::ThemeProxy<ButtonTheme> {
public:
    ButtonTheme(int screen_num,
                const std::string &name,
                const std::string &extra_fallback);
    virtual ~ButtonTheme() { }

    bool fallback(tk::ThemeItem_base &item);
    void reconfigTheme();

    const tk::Texture &pressed() const { return *m_pressed_texture; }
    GC gc() const { return m_gc.gc(); }
    int scale() const { return *m_scale; } // scale factor for inside objects
    const std::string &name() const { return m_name; }

    virtual ButtonTheme &operator *() { return *this; }
    virtual const ButtonTheme &operator *() const { return *this; }

private:
    tk::ThemeItem<tk::Color> m_pic_color;
    tk::ThemeItem<tk::Texture> m_pressed_texture;
    tk::GContext m_gc;
    tk::ThemeItem<int> m_scale;
    const std::string m_name;
    const std::string m_fallbackname;
};

#endif // BUTTONTHEME_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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
