// MenuItem.hh for Shynebox Window Manager

/*
  Items that are inserted into Menus. These will either have a command
  associated or another Menu called a submenu.
  Items can be enabled, disabled or toggled.
*/

#ifndef TK_MENUITEM_HH
#define TK_MENUITEM_HH

#include "Command.hh"
#include "PixmapWithMask.hh"
#include "ITypeAheadable.hh"
#include "SbString.hh"

namespace tk {

class Menu;
class MenuTheme;
class SbDrawable;
template <class T> class ThemeProxy;

// An interface for a menu item in Menu
class MenuItem : public tk::ITypeAheadable {
public:
  MenuItem()
      : m_label(BiDiString("") ),
        m_menu(0),
        m_submenu(0),
        m_enabled(true),
        m_selected(false),
        m_close_on_click(true),
        m_toggle_item(false)
  { }

  explicit MenuItem(const BiDiString &label)
      : m_label(label),
        m_menu(0),
        m_submenu(0),
        m_enabled(true),
        m_selected(false),
        m_close_on_click(true),
        m_toggle_item(false)
  { }

  MenuItem(const BiDiString &label, Menu &host_menu)
      : m_label(label),
        m_menu(&host_menu),
        m_submenu(0),
        m_enabled(true),
        m_selected(false),
        m_close_on_click(true),
        m_toggle_item(false)
  { }
  /// create a menu item with a specific command to be executed on click
  MenuItem(const BiDiString &label, Command<void> &cmd, Menu *menu = 0)
      : m_label(label),
        m_menu(menu),
        m_submenu(0),
        m_command(&cmd),
        m_enabled(true),
        m_selected(false),
        m_close_on_click(true),
        m_toggle_item(false)
  { }

  MenuItem(const BiDiString &label, Menu *submenu, Menu *host_menu = 0)
      : m_label(label),
        m_menu(host_menu),
        m_submenu(submenu),
        m_enabled(true),
        m_selected(false),
        m_close_on_click(true),
        m_toggle_item(false)
  { }

  virtual ~MenuItem();

  void setCommand(Command<void> &cmd) {
    if (m_command != 0)
      delete m_command;
    m_command = &cmd;
  }
  virtual void setSelected(bool selected) { m_selected = selected; }
  virtual void setEnabled(bool enabled) { m_enabled = enabled; }
  virtual void setLabel(const BiDiString &label) { m_label = label; }
  virtual void setToggleItem(bool val) { m_toggle_item = val; }
  void setCloseOnClick(bool val) { m_close_on_click = val; }
  void setIcon(const std::string &filename, int screen_num);
  virtual Menu *submenu() { return m_submenu; }

  virtual const tk::BiDiString& label() const { return m_label; }
  virtual const PixmapWithMask *icon() const {
    return m_icon ? m_icon->pixmap : 0;
  }
  virtual const Menu *submenu() const { return m_submenu; }
  virtual bool isEnabled() const { return m_enabled; }
  virtual bool isSelected() const { return m_selected; }
  virtual bool isToggleItem() const { return m_toggle_item; }

  // iType functions
  const SbString &iTypeString() const { return m_label.visual(); }
  virtual void drawLine(SbDrawable &draw,
                    const tk::ThemeProxy<MenuTheme> &theme,
                    size_t n_chars,
                    int text_x, int text_y,
                    unsigned int width, size_t skip_chars = 0) const;

  virtual unsigned int width(const tk::ThemeProxy<MenuTheme> &theme) const;
  virtual unsigned int height(const tk::ThemeProxy<MenuTheme> &theme) const;
  virtual void draw(SbDrawable &drawable,
                    const tk::ThemeProxy<MenuTheme> &theme,
                    bool highlight,
                    // "foreground" is the transient bits - more likely to change
                    bool draw_background,
                    //bool draw_foreground, bool draw_background,
                    int x, int y,
                    unsigned int width, unsigned int height) const;
  virtual void updateTheme(const tk::ThemeProxy<MenuTheme> &theme);
  /**
     Called when the item was clicked with a specific button
     @param button the button number
     @param time the time stamp
  */
  virtual void click(int button, int time, unsigned int mods);
  /// must use this to show submenu to ensure consistency for
  // object like window menu in ClientMenu (see Workspace.cc)
  virtual void showSubmenu();
  Command<void> &command() { return *m_command; }
  const Command<void> &command() const { return *m_command; }

  void setMenu(Menu &menu) { m_menu = &menu; }
  Menu *menu() { return m_menu; }

private:
  BiDiString m_label;           // label of this item
  Menu *m_menu = 0;             // the menu we live in
  Menu *m_submenu = 0;          // a submenu, 0 if we don't have one
  Command<void> *m_command = 0; // command to be executed
  bool m_enabled, m_selected;
  bool m_close_on_click, m_toggle_item;

  struct Icon {
    PixmapWithMask *pixmap = 0;
    std::string filename;
  };
  Icon *m_icon = 0;
};

} // end namespace tk

#endif // TK_MENUITEM_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2003-2004 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
