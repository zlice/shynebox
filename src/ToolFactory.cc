// ToolFactory.cc for Shynebox Window Manager

#include "ToolFactory.hh"

#include "ButtonTool.hh"
#include "ClockTool.hh"
#include "SpacerTool.hh"
#include "WorkspaceCmd.hh"
#include "WorkspaceNameTool.hh"

#include "WorkspaceNameTheme.hh"
#include "ButtonTheme.hh"

#include "tk/CommandParser.hh"
#include "tk/Config.hh"
#include "Screen.hh"
#include "ScreenPlacement.hh"
#include "IconbarTool.hh"
#include "Toolbar.hh"
#include "shynebox.hh"

ToolFactory::ToolFactory(BScreen &screen):m_screen(screen),
    m_clock_theme(screen.screenNumber(), "toolbar.clock"),
    m_button_theme(new ButtonTheme(screen.screenNumber(), "toolbar.button", "toolbar.clock") ),
    m_workspace_theme(new WorkspaceNameTheme(screen.screenNumber(), "toolbar.workspace") ),
    m_systray_theme(new ButtonTheme(screen.screenNumber(), "toolbar.systray", "toolbar.clock") ),
    m_iconbar_theme(screen.screenNumber(), "toolbar.iconbar"),
    m_focused_iconbar_theme(screen.screenNumber(), "toolbar.iconbar.focused"),
    m_unfocused_iconbar_theme(screen.screenNumber(), "toolbar.iconbar.unfocused")
{ }
// ToolFactory class init

ToolbarItem *ToolFactory::create(const std::string &name, const tk::SbWindow &parent, Toolbar &tbar) {
  ToolbarItem * item = 0;

  tk::CommandParser<void>& cp = tk::CommandParser<void>::instance();

  if (name == "workspacename") {
    m_ws_tool = new WorkspaceNameTool(parent, *m_workspace_theme, screen() );
    using namespace tk;
    Command<void> *leftCommand = 0, *rightCommand = 0;
    leftCommand = new PrevWorkspaceCmd(1);
    rightCommand = new NextWorkspaceCmd(1);
    m_ws_tool->button().setOnClick(*leftCommand);
    m_ws_tool->button().setOnClick(*rightCommand, 3);
    item = m_ws_tool;
  } else if (name == "iconbar") {
    m_icon_bar = new IconbarTool(parent, m_iconbar_theme, m_focused_iconbar_theme,
                                m_unfocused_iconbar_theme, screen(), tbar.menu() );
    item = m_icon_bar;
  } else if (name == "systemtray") {
    m_sys_tray = new SystemTray(parent, dynamic_cast<ButtonTheme &>(*m_systray_theme), screen() );
    item = m_sys_tray;
  } else if (name == "clock") {
    item = new ClockTool(parent, m_clock_theme, screen(), tbar.menu() );
  } else if (name.find("spacer") == 0) {
    int size = -1;
    if (name.size() > 6) { // spacer_20 creates a 20px spacer
      if (name.at(6) == '_')
        size = atoi(name.substr(7, std::string::npos).c_str() );
      if (size < 1)
        size = 10; // hardcoded default size
    }
    item = new SpacerTool(size);
  } else if (name.find("button.") == 0) {
    // A generic button. Needs a label and a command (chain) configured
    // example: toolbar.button.xxx.label: texthere
    //          toolbar.button.xxx.commands: Exec /path/to/something

    // NOTE: map '[]' operator creates empty values for items not in map
    //       values MUST be checked here!
    const string tb_but_label = "toolbar." + name + ".label";
    const string tb_but_cmds  = "toolbar." + name + ".commands";

    if (m_screen.m_cfgmap.count(tb_but_label) == 0
        || m_screen.m_cfgmap.count(tb_but_cmds) == 0)
      return 0;

    std::string label = *m_screen.m_cfgmap[tb_but_label]; // str should copy config ref

    if (label.empty() ) {
      m_screen.m_cfgmap.erase(tb_but_label);
      return 0;
    }

    std::string cmds = *m_screen.m_cfgmap[tb_but_cmds];

    tk::TextButton *btn = new tk::TextButton(parent, m_button_theme->font(), label);
    screen().mapToolButton(name, btn);

    std::list<std::string> commands;
    tk::StringUtil::stringtok(commands, cmds, ":");
    int i = 1;
    for (auto &it : commands) {
      std::string cmd_str = it;
      tk::StringUtil::removeTrailingWhitespace(cmds);
      tk::StringUtil::removeFirstWhitespace(cmds);
      tk::Command<void> *cmd = 0;
      cmd = cp.parse(cmd_str);
      if (cmd != 0)
        btn->setOnClick(*cmd, i);
      i++;
    }
    item = new ButtonTool(btn, ToolbarItem::FIXED,
                          dynamic_cast<ButtonTheme &>(*m_button_theme),
                          screen().imageControl() );
  } // if-chain tool name

  if (item)
    item->renderTheme();

  return item;
} // create

void ToolFactory::updateWSNameTag() {
 if (m_ws_tool)
   m_ws_tool->update();
}

void ToolFactory::resetIconbar(Focusable *win) {
  if (! m_icon_bar)
    return;
  if (win == 0)
    m_icon_bar->update(IconbarTool::LIST_RESET, 0);
  else
    m_icon_bar->update(IconbarTool::LIST_REMOVE, win);
}

void ToolFactory::updateIconbar(Focusable *win) {
  if (! m_icon_bar)
    return;
  if (win == 0)
    m_icon_bar->update(IconbarTool::ALIGN, win);
  else {
    m_icon_bar->update(IconbarTool::LIST_ADD, win);
  }
}

void ToolFactory::updateThemes() {
  m_clock_theme.reconfigTheme();
  m_focused_iconbar_theme.reconfigTheme();
  m_unfocused_iconbar_theme.reconfigTheme();
  m_button_theme->reconfigTheme();
  m_workspace_theme->reconfigTheme();
}

int ToolFactory::maxFontHeight() {
  unsigned int max_height = 0;

  max_height = std::max(max_height, m_clock_theme.font().height() );
  max_height = std::max(max_height, m_focused_iconbar_theme.text().font().height() );
  max_height = std::max(max_height, m_unfocused_iconbar_theme.text().font().height() );
  max_height = std::max(max_height, m_workspace_theme->font().height() );

  return max_height;
}

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
