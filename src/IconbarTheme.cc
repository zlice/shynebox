// IconbarTheme.cc for Shynebox Window Manager

#include "IconbarTheme.hh"

IconbarTheme::IconbarTheme(int screen_num,
                           const std::string &name):
      tk::Theme(screen_num),
      m_texture(*this, name),
      m_empty_texture(*this, name + ".empty"),
      m_border(*this, name),
      m_text(*this, name),
      m_name(name) {
  tk::ThemeManager::instance().loadTheme(*this);
} // IconbarTheme class init

IconbarTheme::~IconbarTheme() { } // IconbarTheme class destroy

void IconbarTheme::reconfigTheme() {
  m_text.updateTextColor();
}

// fallback resources
bool IconbarTheme::fallback(tk::ThemeItem_base &item) {
  using namespace tk;
  ThemeManager &tm = ThemeManager::instance();
  std::string base = m_name;
  base.erase(base.find_last_of(".") );

  if (&m_texture == &item)
    return tm.loadItem(item, "toolbar.windowLabel");
  else if (&m_empty_texture == &item)
    return (tm.loadItem(item, "toolbar.iconbar.empty")
            || tm.loadItem(item, m_texture.name() )
            || tm.loadItem(item, "toolbar.windowLabel")
            || tm.loadItem(item, "toolbar") );
  else if (item.name() == m_name + ".borderWidth")
    return (tm.loadItem(item, base + ".borderWidth")
            || tm.loadItem(item, "window.borderWidth")
            || tm.loadItem(item, "borderWidth") );
  // don't fallback for base border, for theme backwards compatibility
  else if (item.name() == m_name + ".borderColor")
    return (tm.loadItem(item, base + ".borderColor")
            || tm.loadItem(item, "window.borderColor")
            || tm.loadItem(item, "borderColor") );
  else if (item.name() == m_name + ".font")
    return tm.loadItem(item, "window.font");
  else if (item.name() == m_name + ".justify")
    return (tm.loadItem(item, base + ".justify")
            || tm.loadItem(item, "window.justify") );
  return false;
} // fallback

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
