// MenuTheme.cc for Shynebox Window Manager

#include "MenuTheme.hh"

#include "Color.hh"
#include "Texture.hh"
#include "Font.hh"
#include "App.hh"
#include "StringUtil.hh"

#ifdef HAVE_CSTDIO
  #include <cstdio>
#else
  #include <stdio.h>
#endif
//#include <algorithm>

namespace tk {

MenuTheme::MenuTheme(int screen_num):
      tk::Theme(screen_num),
      t_text(*this, "menu.title.textColor"),
      f_text(*this, "menu.frame.textColor"),
      h_text(*this, "menu.hilite.textColor"),
      d_text(*this, "menu.frame.disableColor"),
      u_text(*this, "menu.frame.underlineColor"),
      title(*this, "menu.title"),
      frame(*this, "menu.frame"),
      hilite(*this, "menu.hilite"),
      titlefont(*this, "menu.title.font"),
      framefont(*this, "menu.frame.font"),
      hilitefont(*this, "menu.hilite.font"),
      framefont_justify(*this, "menu.frame.justify"),
      hilitefont_justify(*this, "menu.hilite.justify"),
      titlefont_justify(*this, "menu.title.justify"),
      bullet_pos(*this, "menu.bullet.position"),
      m_bullet(*this, "menu.bullet"),
      m_shapeplace(*this, "menu.roundCorners"),
      m_title_height(*this, "menu.titleHeight"),
      m_item_height(*this, "menu.itemHeight"),
      m_border_width(*this, "menu.borderWidth"),
      m_bevel_width(*this, "menu.bevelWidth"),
      m_border_color(*this, "menu.borderColor"),
      m_bullet_pixmap(*this, "menu.submenu.pixmap"),
      m_selected_pixmap(*this, "menu.selected.pixmap"),
      m_unselected_pixmap(*this, "menu.unselected.pixmap"),
      m_hl_bullet_pixmap(*this, "menu.hilite.submenu.pixmap"),
      m_hl_selected_pixmap(*this, "menu.hilite.selected.pixmap"),
      m_hl_unselected_pixmap(*this, "menu.hilite.unselected.pixmap"),
      m_display(tk::App::instance()->display() ),
      t_text_gc(RootWindow(m_display, screen_num) ),
      f_text_gc(RootWindow(m_display, screen_num) ),
      u_text_gc(RootWindow(m_display, screen_num) ),
      h_text_gc(RootWindow(m_display, screen_num) ),
      d_text_gc(RootWindow(m_display, screen_num) ),
      hilite_gc(RootWindow(m_display, screen_num) ),
      m_delay(0), // no delay as default
      m_real_title_height(*m_title_height),
      m_real_item_height(*m_item_height) {
  *m_border_width = 0;
  *m_bevel_width = 0;
  *m_border_width = 0;
  *m_shapeplace = tk::Shape::NONE;

  ThemeManager::instance().loadTheme(*this);

  if (*m_title_height < 1)
    *m_title_height = 1;
  const unsigned int pad = 2*bevelWidth();
  m_real_item_height = std::max(std::max(pad + 1, *m_item_height),
                                std::max(frameFont().height() + pad,
                                         hiliteFont().height() + pad) );
  m_real_title_height = std::max(std::max(pad + 1, *m_title_height),
                                 titleFont().height() + pad);

  t_text_gc.setForeground(*t_text);
  f_text_gc.setForeground(*f_text);
  u_text_gc.setForeground(*u_text);
  h_text_gc.setForeground(*h_text);
  d_text_gc.setForeground(*d_text);
  hilite_gc.setForeground(hilite->color() );
} // MenuTheme class init

MenuTheme::~MenuTheme() { } // MenuTheme class destroy

void MenuTheme::reconfigTheme() {
  // clamp to "normal" size
  if (*m_bevel_width > 20)
    *m_bevel_width = 20;
  if (*m_border_width > 20)
    *m_border_width = 20;


  const unsigned int pad = 2*bevelWidth();
  m_real_item_height = std::max(std::max(pad + 1, *m_item_height),
                                std::max(frameFont().height() + pad,
                                         hiliteFont().height() + pad) );
  m_real_title_height = std::max(std::max(pad + 1, *m_title_height),
                                 titleFont().height() + pad);

  unsigned int item_pm_height = itemHeight();

  m_bullet_pixmap->scale(item_pm_height, item_pm_height);
  m_selected_pixmap->scale(item_pm_height, item_pm_height);
  m_unselected_pixmap->scale(item_pm_height, item_pm_height);

  m_hl_bullet_pixmap->scale(item_pm_height, item_pm_height);
  m_hl_selected_pixmap->scale(item_pm_height, item_pm_height);
  m_hl_unselected_pixmap->scale(item_pm_height, item_pm_height);

  t_text_gc.setForeground(*t_text);
  f_text_gc.setForeground(*f_text);
  u_text_gc.setForeground(*u_text);
  h_text_gc.setForeground(*h_text);
  d_text_gc.setForeground(*d_text);
  hilite_gc.setForeground(hilite->color() );
} // reconfigTheme

bool MenuTheme::fallback(ThemeItem_base &item) {
  if (item.name() == "menu.borderWidth")
    return ThemeManager::instance().loadItem(item, "borderWidth");
  else if (item.name() == "menu.borderColor")
    return ThemeManager::instance().loadItem(item, "borderColor");
  else if (item.name() == "menu.bevelWidth")
    return ThemeManager::instance().loadItem(item, "bevelWidth");
  else if (item.name() == "menu.hilite.font")
    return ThemeManager::instance().loadItem(item, "menu.frame.font");
  else if (item.name() == "menu.hilite.justify")
    return ThemeManager::instance().loadItem(item, "menu.frame.justify");

  return false;
}


template <>
void ThemeItem<MenuTheme::BulletType>::setDefaultValue() {
  m_value = MenuTheme::EMPTY;
}

template <>
void ThemeItem<MenuTheme::BulletType>::setFromString(const char *str) {
  // do nothing
  if (StringUtil::strcasestr(str, "empty") != 0)
    m_value = MenuTheme::EMPTY;
  else if (StringUtil::strcasestr(str, "square") != 0)
    m_value = MenuTheme::SQUARE;
  else if (StringUtil::strcasestr(str, "triangle") != 0)
    m_value = MenuTheme::TRIANGLE;
  else if (StringUtil::strcasestr(str, "diamond") != 0)
    m_value = MenuTheme::DIAMOND;
  else
    setDefaultValue();
}

template <>
void ThemeItem<MenuTheme::BulletType>::load(const std::string *name) {
  // do nothing, we don't have anything extra to load
  (void) name;
}

} // end namespace  tk

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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
