// SbCommands.cc for Shynebox Window Manager

#include "SbCommands.hh"
#include "shynebox.hh"
#include "ClientMenu.hh"
#include "Screen.hh"
#include "ScreenPlacement.hh"
#include "FocusControl.hh"
#include "Workspace.hh"
#include "Window.hh"
#include "Keys.hh"
#include "MenuCreator.hh"
#include "WorkspaceMenu.hh"

#include "tk/KeyUtil.hh"
#include "tk/Theme.hh"
#include "tk/Menu.hh"
#include "tk/CommandParser.hh"
#include "tk/StringUtil.hh"

#include <unistd.h> // fork, setsid, execl

#include <fstream>
#include <iostream>

#if defined(__EMX__) && defined(HAVE_PROCESS_H)
#include <process.h> // for P_NOWAIT
#endif // __EMX__

using std::string;
using std::ofstream;
using std::ios;

namespace {

void showMenu(BScreen &screen, tk::Menu &menu) {
  // check if menu has changed
  if (typeid(menu) == typeid(SbMenu) ) {
    SbMenu *sbmenu = static_cast<SbMenu *>(&menu);
    if (sbmenu->reloadHelper() )
      sbmenu->reloadHelper()->checkReload();
  }

  SbMenu::setWindow(FocusControl::focusedSbWindow() );

  int x = 0, y = 0;

  int mk = Shynebox::instance()->lastEvent().type;
  if (mk == KeyPress || mk == KeyRelease) {
    int head = screen.getCurHead();
    int t = static_cast<signed>(screen.getHeadY(head) ),
        l = static_cast<signed>(screen.getHeadX(head) ),
        w = static_cast<signed>(screen.getHeadWidth(head) ) / 2,
        h = static_cast<signed>(screen.getHeadHeight(head) ) / 2;

    x = l + w;
    y = t + h - (menu.height() / 2);
  } else {
    tk::KeyUtil::get_pointer_coords(menu.sbwindow().display(),
                          screen.rootWindow().window(), x, y);
  }

  screen.placementStrategy().placeAndShowMenu(menu, x, y);
}

} // anonymous namespace

namespace SbCommands {

using tk::Command;

REGISTER_UNTRUSTED_COMMAND_WITH_ARGS(exec, SbCommands::ExecuteCmd, void);
REGISTER_UNTRUSTED_COMMAND_WITH_ARGS(execute, SbCommands::ExecuteCmd, void);
REGISTER_UNTRUSTED_COMMAND_WITH_ARGS(execcommand, SbCommands::ExecuteCmd, void);

ExecuteCmd::ExecuteCmd(const string &cmd, int screen_num) :m_cmd(cmd), m_screen_num(screen_num) { }

void ExecuteCmd::execute() {
  run();
}

int ExecuteCmd::run() {
#if defined(__EMX__)
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif
  char comspec[PATH_MAX] = {0};
  char * env_var = getenv("COMSPEC");
  if (env_var != NULL) {
    strncpy(comspec, env_var, PATH_MAX - 1);
    comspec[PATH_MAX - 1] = '\0';
  } else {
    strncpy(comspec, "cmd.exe", 7);
    comspec[7] = '\0';
  }

  return spawnlp(P_NOWAIT, comspec, comspec, "/c", m_cmd.c_str(), static_cast<void*>(NULL) );
#else
  pid_t pid = fork();
  if (pid)
    return pid;

  // 'display' is given as 'host:number.screen'. we want to give the
  // new app a good home, so we remove '.screen' from what is given
  // us from the xserver and replace it with the screen_num of the Screen
  // the user currently points at with the mouse
  string display = DisplayString(tk::App::instance()->display() );
  int screen_num = m_screen_num;
  if (screen_num < 0)
    screen_num = Shynebox::instance()->mouseScreen()->screenNumber();

  // strip away the '.screen'
  size_t dot = display.rfind(':');
  dot = display.find('.', dot);
  if (dot != string::npos) // 'display' has actually a '.screen' part
    display.erase(dot);
  display += '.';
  display += tk::StringUtil::number2String(screen_num);

  tk::App::setenv("DISPLAY", display.c_str() );

  const char *shell = getenv("SHELL");
  if (!shell)
    shell = "/bin/sh";

  setsid();
  execl(shell, shell, "-c", m_cmd.c_str(), static_cast<void*>(NULL) );
  exit(EXIT_SUCCESS);

  return pid; // compiler happy -> we are happy ;)
#endif // EMX
} // run

REGISTER_COMMAND(exit, SbCommands::ExitShyneboxCmd, void);
REGISTER_COMMAND(quit, SbCommands::ExitShyneboxCmd, void);

void ExitShyneboxCmd::execute() {
  Shynebox::instance()->shutdown();
}

REGISTER_COMMAND(saverc, SbCommands::SaveResources, void);

void SaveResources::execute() {
  Shynebox::instance()->save_rc();
}

REGISTER_COMMAND_PARSER(restart, RestartShyneboxCmd::parse, void);

tk::Command<void> *RestartShyneboxCmd::parse(const string &command,
                                 const string &args, bool trusted) {
  (void)command;
  (void)trusted;
  return new RestartShyneboxCmd(args);
}

RestartShyneboxCmd::RestartShyneboxCmd(const string &cmd):m_cmd(cmd){ }

void RestartShyneboxCmd::execute() {
  Shynebox::instance()->restart(m_cmd.c_str() );
}

REGISTER_COMMAND(reconfigure, SbCommands::ReconfigureShyneboxCmd, void);
REGISTER_COMMAND(reconfig, SbCommands::ReconfigureShyneboxCmd, void);

void ReconfigureShyneboxCmd::execute() {
  Shynebox::instance()->reconfigure();
}

REGISTER_COMMAND(reloadstyle, SbCommands::ReloadStyleCmd, void);

void ReloadStyleCmd::execute() {
  SetStyleCmd cmd(Shynebox::instance()->getStyleFilename() );
  cmd.execute();
}

REGISTER_COMMAND_WITH_ARGS(setstyle, SbCommands::SetStyleCmd, void);

SetStyleCmd::SetStyleCmd(const string &filename):m_filename(filename) { }

void SetStyleCmd::execute() {
  if (tk::ThemeManager::instance().load(m_filename,
      Shynebox::instance()->getStyleOverlayFilename() ) ) {
    Shynebox::instance()->saveStyleFilename(m_filename.c_str() );
    Shynebox::instance()->reconfigThemes();
    Shynebox::instance()->save_rc();
  }
}

REGISTER_COMMAND_WITH_ARGS(keymode, SbCommands::KeyModeCmd, void);

KeyModeCmd::KeyModeCmd(const string &arguments):m_keymode(arguments),m_end_args("None Escape") {
  string::size_type second_pos = m_keymode.find_first_of(" \t", 0);
  if (second_pos != string::npos) {
    // ok we have arguments, parsing them here
    m_end_args = m_keymode.substr(second_pos);
    m_keymode.erase(second_pos); // remove argument from command
  }
  if (m_keymode != "default")
    Shynebox::instance()->keys()->addBinding(m_keymode + ": " + m_end_args + " :keymode default");
}

void KeyModeCmd::execute() {
  Shynebox::instance()->keys()->keyMode(m_keymode);
}

REGISTER_COMMAND(hidemenus, SbCommands::HideMenuCmd, void);

void HideMenuCmd::execute() {
  tk::Menu::hideShownMenu();
}

tk::Command<void> *ShowClientMenuCmd::parse(const string &command,
                                        const string &args, bool trusted) {
  (void)command;
  (void)trusted;
  int opts;
  string pat;
  FocusableList::parseArgs(args, opts, pat);
  return new ShowClientMenuCmd(opts, pat);
}

REGISTER_COMMAND_PARSER(clientmenu, ShowClientMenuCmd::parse, void);

void ShowClientMenuCmd::execute() {
  BScreen *screen = Shynebox::instance()->mouseScreen();

  const FocusableList *list =
      FocusableList::getListFromOptions(*screen, m_option);
  m_list.clear();

  for (auto it : list->clientList() ) {
    Focusable *f = it;
    if (typeid(*f) == typeid(ShyneboxWindow) && m_pat.match(*f) )
      m_list.push_back(static_cast<ShyneboxWindow *>(f) );
  }

  if (m_menu)
    m_menu->updateMenu();
  else
    m_menu = new ClientMenu(*screen, m_list);

  screen->addTemporalMenu(m_menu);
  ::showMenu(*screen, *m_menu);
}

REGISTER_COMMAND_WITH_ARGS(custommenu, SbCommands::ShowCustomMenuCmd, void);

ShowCustomMenuCmd::ShowCustomMenuCmd(const string &arguments) : custom_menu_file(arguments) { }

void ShowCustomMenuCmd::execute() {
  BScreen *screen = Shynebox::instance()->mouseScreen();

  if (!m_menu) {
    m_menu = MenuCreator::createMenu("", *screen);
    m_menu->setReloadHelper(new tk::AutoReloadHelper() );
    tk::SimpleCommand<ShowCustomMenuCmd> *menu_rh
        = new tk::SimpleCommand<ShowCustomMenuCmd>( *this, &ShowCustomMenuCmd::reload);
    m_menu->reloadHelper()->setReloadCmd(*menu_rh);
    m_menu->reloadHelper()->setMainFile(custom_menu_file);
  } else
    m_menu->reloadHelper()->checkReload();

  screen->addTemporalMenu(m_menu);
  ::showMenu(*screen, *m_menu);
}

void ShowCustomMenuCmd::reload() {
  m_menu->removeAll();
  m_menu->setLabel(tk::BiDiString("") );
  MenuCreator::createFromFile(custom_menu_file, *m_menu, m_menu->reloadHelper() );
}

REGISTER_COMMAND(rootmenu, SbCommands::ShowRootMenuCmd, void);

void ShowRootMenuCmd::execute() {
  BScreen *screen = Shynebox::instance()->mouseScreen();

  // moved here for single update point of ws items and client menus
  screen->updateWSItems(true);
  ::showMenu(*screen, screen->rootMenu() );
}

REGISTER_COMMAND(workspacemenu, SbCommands::ShowWorkspaceMenuCmd, void);

void ShowWorkspaceMenuCmd::execute() {
  BScreen *screen = Shynebox::instance()->mouseScreen();

  ::showMenu(*screen, screen->workspaceMenu() );
}

} // end namespace SbCommands

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2003 - 2008 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
