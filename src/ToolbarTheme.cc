// ToolbarTheme.cc for Shynebox Window Manager

#include "ToolbarTheme.hh"

using std::string;

ToolbarTheme::ToolbarTheme(int screen_num):
      tk::Theme(screen_num),
      m_toolbar(*this, "toolbar"),
      m_border(*this, "toolbar"),
      m_bevel_width(*this, "toolbar.bevelWidth"),
      m_shape(*this, "toolbar.shaped"),
      m_height(*this, "toolbar.height"),
      m_button_size(*this, "toolbar.button.size") {
  *m_bevel_width = 0;
  *m_shape = false;
  *m_height = 0;
  *m_button_size = -1;
  tk::ThemeManager::instance().loadTheme(*this);
} // ToolbarTheme class init

ToolbarTheme::~ToolbarTheme() { } // Toolbartheme class destroy

bool ToolbarTheme::fallback(tk::ThemeItem_base &item) {
  if (item.name().find(".borderWidth") != string::npos)
    return tk::ThemeManager::instance().loadItem(item, "borderWidth");
  else if (item.name().find(".borderColor") != string::npos)
    return tk::ThemeManager::instance().loadItem(item, "borderColor");
  else if (item.name() == "toolbar.bevelWidth")
    return tk::ThemeManager::instance().loadItem(item, "bevelWidth");
  return false;
}

void ToolbarTheme::reconfigTheme() {
  if (*m_bevel_width > 20)
    *m_bevel_width = 20;

  if (*m_height > 100)
    *m_height = 100;
  else if (*m_height < 0)
    *m_height = 0;

  if (*m_button_size > 100)
    *m_button_size = 100;
}

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
