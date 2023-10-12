// ToolFactory.hh for Shynebox Window Manager

/*
  Toolbar helper class.
  Creates Systray, Iconbar, Clock, Workspace Name tag, custom buttons, or spacers.
  Provides functions for Screen to call for updates to toolbar items directly.
*/

#ifndef TOOLFACTORY_HH
#define TOOLFACTORY_HH

#include "ToolTheme.hh"
#include "IconbarTheme.hh"
#include "IconbarTool.hh"
#include "SystemTray.hh"
#include "WorkspaceNameTool.hh"

#include "tk/NotCopyable.hh"

class BScreen;
class WorkspaceNameTool;
class ToolbarItem;
class Toolbar;
class IconbarTool;

namespace tk {
class SbWindow;
}

// creates toolbaritems
class ToolFactory:private tk::NotCopyable {
public:
  explicit ToolFactory(BScreen &screen);
  virtual ~ToolFactory() {
    if (m_button_theme)
      delete m_button_theme;
    if (m_workspace_theme)
      delete m_workspace_theme;
    if (m_systray_theme)
      delete m_systray_theme;
  }

  ToolbarItem *create(const std::string &name, const tk::SbWindow &parent, Toolbar &tbar);
  void updateThemes();
  int maxFontHeight();
  const BScreen &screen() const { return m_screen; }
  BScreen &screen() { return m_screen; }

  IconbarTool *m_icon_bar = 0;
  SystemTray *m_sys_tray = 0;
  WorkspaceNameTool *m_ws_tool = 0;

  void updateWSNameTag();
  void resetIconbar(Focusable *win);
  void updateIconbar(Focusable *win);
  bool hasSystray() { return m_sys_tray != 0; }

private:
  BScreen   &m_screen;
  ToolTheme m_clock_theme;
  ToolTheme *m_button_theme = 0,
            *m_workspace_theme = 0,
            *m_systray_theme = 0;

  IconbarTheme m_iconbar_theme, m_focused_iconbar_theme,
               m_unfocused_iconbar_theme;
};

#endif // TOOLFACTORY_HH

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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
