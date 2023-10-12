// IconbarTool.hh for Shynebox Window Manager

/*
  Toolbar item to display FocusableList (windows in a given ClientPattern).
  Default ClientPattern is all windows (not tabbed) in current workspace.
*/

#ifndef ICONBARTOOL_HH
#define ICONBARTOOL_HH

#include "ToolbarItem.hh"
#include "SbMenu.hh"

#include "tk/ButtonTrain.hh"
#include "tk/CachedPixmap.hh"
#include "tk/Config.hh"
#include "tk/Timer.hh"

#include <map>

class IconbarTheme;
class BScreen;
class IconButton;
class Focusable;
class FocusableList;

class IconbarTool: public ToolbarItem {
public:
  typedef std::map<Focusable *, IconButton *> IconMap;

  IconbarTool(const tk::SbWindow &parent, IconbarTheme &theme,
              tk::ThemeProxy<IconbarTheme> &focused_theme,
              tk::ThemeProxy<IconbarTheme> &unfocused_theme,
              BScreen &screen, tk::Menu &menu);
  ~IconbarTool();

  void move(int x, int y);
  void resize(unsigned int width, unsigned int height);
  void moveResize(int x, int y,
                  unsigned int width, unsigned int height);

  void show();
  void hide();
  void setAlignment(tk::ButtonTrainAlignment_e a);
  void setMode(std::string mode);
  void parentMoved() { m_icon_container.parentMoved(); }

  unsigned int width() const;
  unsigned int preferredWidth() const;
  unsigned int height() const;
  unsigned int borderWidth() const;

  std::string mode() const { return m_rc_mode; }

  void setOrientation(tk::Orientation orient);
  tk::ButtonTrainAlignment_e alignment() const { return m_icon_container.alignment(); }
  static std::string &iconifiedPrefix() { return s_iconifiedDecoration[0]; }
  static std::string &iconifiedSuffix() { return s_iconifiedDecoration[1]; }

  const BScreen &screen() const { return m_screen; }

  void reset(); // remove all windows and add again
  enum UpdateReason { LIST_ADD, LIST_REMOVE, LIST_RESET, ALIGN };
  void update(UpdateReason reason, Focusable *win);

private:
  void updateSizing();

  // render single button, and probably apply changes (clear)
  void renderButton(IconButton &button, bool clear = true);
  void renderTheme();
  void deleteIcons();
  void insertWindow(Focusable &win);
  void removeWindow(Focusable &win);
  IconButton *makeButton(Focusable &win);

  void updateMaxSizes(unsigned int width, unsigned int height);

  void updateIconifiedPattern();

  void themeReconfigured();

  void resetLock();
  void unlockSig();
  tk::Timer m_locker_timer;
  tk::Timer m_resizeSig_timer;

  BScreen &m_screen;
  tk::ButtonTrain m_icon_container;
  IconbarTheme &m_theme;
  tk::ThemeProxy<IconbarTheme> &m_focused_theme, &m_unfocused_theme;
  tk::CachedPixmap m_empty_pm; // pixmap for empty container

  // focusable list
  FocusableList *m_winlist = 0;
  IconMap m_icons;
  std::string m_mode;

  // config items
  std::string &m_rc_mode;
  tk::ButtonTrainAlignment_e &m_rc_alignment; // alignment of buttons
  int &m_rc_client_width;                     // size of client button in LEFT/CENTER/RIGHT mode
  unsigned int &m_rc_client_padding;          // padding of the text
  bool &m_rc_use_pixmap;                      // if iconbar should use win pixmap or not

  SbMenu m_menu;
  static std::string s_iconifiedDecoration[2];
  bool m_lock_gfx = false; // lock in case many windows are added at once
};

#endif // ICONBARTOOL_HH

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
