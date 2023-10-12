// SbWinFrameTheme.hh for Shynebox Window Manager

/*
  Theme for frames. Titlebar, grips, border, etc
  Used with FocusableTheme to create focused vs unfocused themes.
*/

#ifndef SBWINFRAMETHEME_HH
#define SBWINFRAMETHEME_HH

#include "tk/Font.hh" // string
#include "tk/Texture.hh"
#include "tk/Color.hh"
#include "tk/Theme.hh"
#include "tk/BorderTheme.hh"
#include "tk/GContext.hh"
#include "tk/Shape.hh"

#include "IconbarTheme.hh"

class SbWinFrameTheme: public tk::Theme,
                       public tk::ThemeProxy<SbWinFrameTheme> {
public:
    explicit SbWinFrameTheme(int screen_num, const std::string &extra);
    ~SbWinFrameTheme();

    const tk::Texture &titleTexture() const { return *m_title; }
    const tk::Texture &handleTexture() const { return *m_handle; }
    const tk::Texture &buttonTexture() const { return *m_button; }
    const tk::Texture &buttonPressedTexture() const { return *m_button_pressed; }
    const tk::Texture &gripTexture() const { return *m_grip; }

    const tk::Color &buttonColor() const { return *m_button_color; }
    tk::Font &font() { return *m_font; }
    GC buttonPicGC() const { return m_button_pic_gc.gc(); }

    bool fallback(tk::ThemeItem_base &item);
    void reconfigTheme();

    Cursor moveCursor() const { return m_cursor_move; }
    Cursor lowerLeftAngleCursor() const { return m_cursor_lower_left_angle; }
    Cursor lowerRightAngleCursor() const { return m_cursor_lower_right_angle; }
    Cursor upperLeftAngleCursor() const { return m_cursor_upper_left_angle; }
    Cursor upperRightAngleCursor() const { return m_cursor_upper_right_angle; }
    Cursor leftSideCursor() const { return m_cursor_left_side; }
    Cursor rightSideCursor() const { return m_cursor_right_side; }
    Cursor topSideCursor() const { return m_cursor_top_side; }
    Cursor bottomSideCursor() const { return m_cursor_bottom_side; }

    tk::Shape::ShapePlace shapePlace() const { return *m_shape_place; }
    const tk::BorderTheme &border() const { return m_border; }

    unsigned int titleHeight() const { return *m_title_height; }
    unsigned int bevelWidth() const { return *m_bevel_width; }
    unsigned int handleWidth() const { return *m_handle_width; }

    IconbarTheme &iconbarTheme() { return m_iconbar_theme; }

    virtual SbWinFrameTheme &operator *() { return *this; }
    virtual const SbWinFrameTheme &operator *() const { return *this; }

private:
    tk::ThemeItem<tk::Texture> m_title, m_handle, m_button,
                                   m_button_pressed, m_grip;
    tk::ThemeItem<tk::Color> m_button_color;
    tk::ThemeItem<tk::Font> m_font;
    tk::ThemeItem<tk::Shape::ShapePlace> m_shape_place;

    tk::ThemeItem<int> m_title_height, m_bevel_width, m_handle_width;
    tk::BorderTheme m_border;

    tk::GContext m_button_pic_gc;

    Cursor m_cursor_move;
    Cursor m_cursor_lower_left_angle;
    Cursor m_cursor_lower_right_angle;
    Cursor m_cursor_upper_left_angle;
    Cursor m_cursor_upper_right_angle;
    Cursor m_cursor_left_side;
    Cursor m_cursor_right_side;
    Cursor m_cursor_top_side;
    Cursor m_cursor_bottom_side;

    IconbarTheme m_iconbar_theme;
};

#endif // SBWINFRAMETHEME_HH

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
