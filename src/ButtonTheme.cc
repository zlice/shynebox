// ButtonTheme.cc for Shynebox Window Manager

#include "ButtonTheme.hh"
#include "tk/App.hh"

//!! TODO: still missing *.pressed.picColor
ButtonTheme::ButtonTheme(int screen_num,
                         const std::string &name,
                         const std::string &extra_fallback):
      ToolTheme(screen_num, name),
      m_pic_color(*this, name + ".picColor"),
      m_pressed_texture(*this, name + ".pressed"),
      m_gc(RootWindow(tk::App::instance()->display(), screen_num) ),
      m_scale(*this, name + ".scale"),
      m_name(name),
      m_fallbackname(extra_fallback) {
  tk::ThemeManager::instance().loadTheme(*this);
}

bool ButtonTheme::fallback(tk::ThemeItem_base &item) {
  if (item.name() == name() )
    return tk::ThemeManager::instance().loadItem(item, m_fallbackname);
  // default to the toolbar label style
  else if (item.name().find(".picColor") != std::string::npos)
    return tk::ThemeManager::instance().loadItem(item, m_fallbackname + ".picColor")
           || tk::ThemeManager::instance().loadItem(item, m_fallbackname + ".textColor");
  else if (item.name().find(".pressed") != std::string::npos) {
    // copy texture
    *m_pressed_texture = texture();
    // invert the bevel if it has one!
    unsigned long type = m_pressed_texture->type();
    unsigned long bevels = (tk::Texture::SUNKEN | tk::Texture::RAISED);
    if ((type & bevels) != 0) {
      type ^= bevels;
      m_pressed_texture->setType(type);
    }
    return true;
  }

  return ToolTheme::fallback(item);
}

void ButtonTheme::reconfigTheme() {
  m_gc.setForeground(*m_pic_color);
}

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
