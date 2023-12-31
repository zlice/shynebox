// Menu.hh for Shynebox Window Manager

/*
  Base class for Menus.
  Holds a list of direct commands, MenuItems, which are sometimes
  other Menus themselves called a submenus.

  The list can be added to with insertXXX() functions and cleared
  with remove() or removeAll().

  Note: Some menus may be shared or managed by something that
  didn't create it. In this case they are flagged so destroying
  the parent menu does not get deleted in the MenuItem destroy.
*/

#ifndef TK_MENU_HH
#define TK_MENU_HH

#include <vector>

#include "SbString.hh"
#include "SbWindow.hh"
#include "EventHandler.hh"
#include "MenuTheme.hh"
#include "Timer.hh"

namespace tk {

template <typename T> class Command;
class MenuItem;
class MenuSearch;
class ImageControl;

class Menu: public tk::EventHandler, tk::SbWindowRenderer {
public:
  static Menu* shownMenu();
  static Menu* focused();
  static void hideShownMenu();

  enum Alignment{ ALIGNDONTCARE = 1, ALIGNTOP, ALIGNBOTTOM };
  enum { RIGHT = 1, LEFT };
  enum { UP = 1, DOWN = 0 };

  // Bullet type
  enum { EMPTY = 0, SQUARE, TRIANGLE, DIAMOND };

  Menu(tk::ThemeProxy<MenuTheme> &tm, ImageControl &imgctrl);
  virtual ~Menu();

  int insertCommand(const SbString &label, Command<void> &cmd, int pos=-1);
  int insert(const SbString &label, int pos=-1);
  int insertSubmenu(const SbString &label, Menu *submenu, int pos= -1);
  int insertItem(MenuItem *item, int pos=-1);
  int remove(unsigned int item);
  int removeItem(MenuItem* item);
  void removeAll();
  void setInternalMenu(bool val = true) { m_internal_menu = val; }
  bool getInternalMenu() { return m_internal_menu; }
  void setAlignment(Alignment a) { m_alignment = a; }

  virtual void raise();
  virtual void lower();
  void cycleItems(bool reverse);
  void setActiveIndex(int new_index);
  void enterSubmenu();

  void disableTitle();
  void enableTitle();
  bool isTitleVisible() const { return m_title.visible; }

  void setScreen(int x, int y, unsigned int w, unsigned int h);

  // event handlers
  void handleEvent(XEvent &event);
  void buttonPressEvent(XButtonEvent &bp);
  virtual void buttonReleaseEvent(XButtonEvent &br);
  void motionNotifyEvent(XMotionEvent &mn);
  void exposeEvent(XExposeEvent &ee);
  void keyPressEvent(XKeyEvent &ke);
  void leaveNotifyEvent(XCrossingEvent &ce);

  void grabInputFocus();
  virtual void reconfigure();
  void setLabel(const tk::BiDiString &labelstr);
  virtual void move(int x, int y);
  virtual void updateMenu();
  void setItemSelected(unsigned int index, bool val);
  void setItemEnabled(unsigned int index, bool val);
  void setMinimumColumns(int columns) { m_min_columns = columns; }
  virtual void drawSubmenu(unsigned int index);
  virtual void show();
  virtual void hide(bool force = false);
  virtual void clearWindow();

  bool isTorn() const              { return m_state.torn; }
  bool isVisible() const           { return m_state.visible; }
  bool isMoving() const            { return m_state.moving; }
  int screenNumber() const         { return m_window.screenNumber(); }
  Window window() const            { return m_window.window(); }
  SbWindow &sbwindow()             { return m_window; }
  const SbWindow &sbwindow() const { return m_window; }
  SbWindow &titleWindow()          { return m_title.win; }
  SbWindow &frameWindow()          { return m_frame.win; }

  const tk::BiDiString &label() const { return m_title.label; }
  int x() const                    { return m_window.x(); }
  int y() const                    { return m_window.y(); }
  unsigned int width() const       { return m_window.width(); }
  unsigned int height() const      { return m_window.height(); }
  size_t numberOfItems() const     { return m_items.size(); }
  int currentSubmenu() const       { return m_which_sub; }

  bool isItemSelected(unsigned int index) const;
  bool isItemEnabled(unsigned int index) const;
  bool isItemSelectable(unsigned int index) const;
  tk::ThemeProxy<MenuTheme> &theme() { return m_theme; }
  const tk::ThemeProxy<MenuTheme> &theme() const { return m_theme; }
  const MenuItem *find(size_t i) const { return m_items[i]; }
  MenuItem *find(size_t i) { return m_items[i]; }

  // returns index of 'submenu', it it is in the top most list of
  // menu items. -1 if no match is found
  int findSubmenuIndex(const Menu* submenu) const;

  bool validIndex(int index) const {
    return (index < static_cast<int>(numberOfItems() ) && index >= 0);
  }

  Menu *parent() { return m_parent; }
  const Menu *parent() const { return m_parent; }

  void renderForeground(SbWindow &win, SbDrawable &drawable);

protected:
  void themeReconfigured();
  void setTitleVisibility(bool b);

  // renders item onto pm
  int drawItem(SbDrawable &pm, unsigned int index,
               bool highlight = false,
               bool exclusive_drawable = false);
  void clearItem(int index, bool clear = true);
  void highlightItem(int index);
  virtual void redrawTitle(SbDrawable &pm);
  virtual void redrawFrame(SbDrawable &pm);

  virtual void internal_hide(bool first = true);

private:
  void openSubmenu();
  void closeMenu();
  void startHide();
  void stopHide();

  void resetTypeAhead();
  void drawTypeAheadItems();

  Menu *m_parent;

  std::vector<MenuItem*> m_items;
  MenuSearch *m_search = 0;

  struct State {
      bool moving;
      bool closing; // right click title
      bool visible;
      bool torn; // torn from parent
  } m_state;

  bool m_need_update;
  bool m_internal_menu; // whether we should destroy this menu or if it's managed somewhere else
  int m_active_index;   // current highlighted index
  int m_which_sub;
  int m_x_move;
  int m_y_move;

  struct Rect {
      int x, y;
      unsigned int width, height;
  } m_screen;

  // the menu window
  tk::SbWindow m_window;

  // the title
  struct Title {
      tk::SbWindow   win;
      Pixmap         pixmap;
      tk::BiDiString label;
      bool           visible;
  } m_title;

  // area for the menuitems
  struct Frame {
      tk::SbWindow win;
      Pixmap pixmap;
      unsigned int height;
  } m_frame;


  // the menuitems are rendered in a grid with
  // 'm_columns' (a minimum of 'm_min_columns') and
  // a max of 'm_rows_per_column'
  int m_columns;
  int m_rows_per_column;
  int m_min_columns;
  unsigned int m_item_w;

  tk::ThemeProxy<MenuTheme>& m_theme;
  ImageControl& m_image_ctrl;
  tk::Shape   *m_shape = 0; // the corners
  Pixmap      m_hilite_pixmap;
  Alignment   m_alignment;

  Timer m_submenu_timer;
  Timer m_hide_timer;
};

} // end namespace tk

#endif // TK_MENU_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2001 - 2004 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Basemenu.hh for Blackbox - an X11 Window manager
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
