// WorkspaceCmd.cc for Shynebox Window Manager

#include "WorkspaceCmd.hh"

#include "Workspace.hh"
#include "Window.hh"
#include "Screen.hh"
#include "TextDialog.hh"
#include "Toolbar.hh"
#include "shynebox.hh"
#include "WinClient.hh"
#include "FocusControl.hh"
#include "WindowCmd.hh"

#include "tk/KeyUtil.hh"
#include "tk/CommandParser.hh"
#include "tk/StringUtil.hh"
#include "tk/I18n.hh"

#include "Debug.hh"

#ifdef HAVE_CMATH
  #include <cmath>
#else
  #include <math.h>
#endif
#include <vector>

using std::string;

REGISTER_COMMAND_PARSER(map, WindowListCmd::parse, void);
REGISTER_COMMAND_PARSER(foreach, WindowListCmd::parse, void);

tk::Command<void> *WindowListCmd::parse(const string &command,
                             const string &args, bool trusted) {
  tk::Command<void> *cmd = 0;
  tk::Command<bool> *filter = 0;
  std::vector<string> tokens;
  int opts = 0;
  string pat;

  tk::StringUtil::stringTokensBetween(tokens, args, pat, '{', '}');
  if (tokens.empty() )
    return 0;

  cmd = tk::CommandParser<void>::instance().parse(tokens[0], trusted);
  if (!cmd)
    return 0;

  if (tokens.size() > 1) {
    FocusableList::parseArgs(tokens[1], opts, pat);

    filter = tk::CommandParser<bool>::instance().parse(pat, trusted);
  }

  return new WindowListCmd(opts, *cmd, *filter);
}

void WindowListCmd::execute() {
  BScreen *screen = Shynebox::instance()->keyScreen();

  FocusableList::Focusables win_list(
    FocusableList::getListFromOptions(*screen, m_opts)->clientList() );

  FocusableList::Focusables::iterator it = win_list.begin(),
                                      it_end = win_list.end();
  // save old value, so we can restore it later
  WinClient *old = WindowCmd<void>::client();
  for (; it != it_end; ++it) {
    Focusable* wptr = *it;
    if (typeid(*wptr) == typeid(ShyneboxWindow) )
      WindowCmd<void>::setWindow((wptr)->sbwindow() );
    else if (typeid(*wptr) == typeid(WinClient) )
      WindowCmd<void>::setClient(dynamic_cast<WinClient *>(wptr) );
    if (!m_filter || m_filter->execute() )
      m_cmd->execute();
  }
  WindowCmd<void>::setClient(old);
}

tk::Command<bool> *SomeCmd::parse(const string &command, const string &args,
                                                                 bool trusted) {
  tk::Command<bool> *boolcmd =
          tk::CommandParser<bool>::instance().parse(args, trusted);
  if (!boolcmd)
    return 0;
  if (command == "some")
    return new SomeCmd(*boolcmd);
  return new EveryCmd(*boolcmd);
}

REGISTER_COMMAND_PARSER(some, SomeCmd::parse, bool);
REGISTER_COMMAND_PARSER(every, SomeCmd::parse, bool);

bool SomeCmd::execute() {
  BScreen *screen = Shynebox::instance()->keyScreen();
  FocusControl::Focusables win_list(screen->focusControl().creationOrderList().clientList() );

  WinClient *old = WindowCmd<void>::client();
  for (auto it : win_list) {
    WinClient *client = dynamic_cast<WinClient *>(it);
    if (!client) continue;
    WindowCmd<void>::setClient(client);
    if (m_cmd->execute() ) {
      WindowCmd<void>::setClient(old);
      return true;
    }
  }

  WindowCmd<void>::setClient(old);
  return false;
}

bool EveryCmd::execute() {
  BScreen *screen = Shynebox::instance()->keyScreen();
  FocusControl::Focusables win_list(screen->focusControl().creationOrderList().clientList() );

  WinClient *old = WindowCmd<void>::client();
  for (auto it : win_list) {
    WinClient *client = dynamic_cast<WinClient *>(it);
    if (!client) continue;
    WindowCmd<void>::setClient(client);
    if (!m_cmd->execute() ) {
      WindowCmd<void>::setClient(old);
      return false;
    }
  }

  WindowCmd<void>::setClient(old);
  return true;
}

namespace {

tk::Command<void> *parseWindowList(const string &command,
                               const string &args, bool trusted) {
  (void) trusted;
  int opts;
  string pat;
  FocusableList::parseArgs(args, opts, pat);
  if (command == "attach")
    return new AttachCmd(pat);
  else if (command == "nextwindow")
    return new NextWindowCmd(opts, pat);
  else if (command == "nextgroup") {
    opts |= FocusableList::LIST_GROUPS;
    return new NextWindowCmd(opts, pat);
  } else if (command == "prevwindow")
    return new PrevWindowCmd(opts, pat);
  else if (command == "prevgroup") {
    opts |= FocusableList::LIST_GROUPS;
    return new PrevWindowCmd(opts, pat);
  } else if (command == "arrangewindows") {
    int method = ArrangeWindowsCmd::UNSPECIFIED;
    return new ArrangeWindowsCmd(method,pat);
  } else if (command == "arrangewindowsvertical") {
    int method = ArrangeWindowsCmd::VERTICAL;
    return new ArrangeWindowsCmd(method,pat);
  } else if (command == "arrangewindowshorizontal") {
    int method = ArrangeWindowsCmd::HORIZONTAL;
    return new ArrangeWindowsCmd(method,pat);
   } else if (command == "arrangewindowsstackleft") {
    int method = ArrangeWindowsCmd::STACKLEFT;
    return new ArrangeWindowsCmd(method,pat);
  } else if (command == "arrangewindowsstackright") {
    int method = ArrangeWindowsCmd::STACKRIGHT;
    return new ArrangeWindowsCmd(method,pat);
  } else if (command == "arrangewindowsstacktop") {
    int method = ArrangeWindowsCmd::STACKTOP;
    return new ArrangeWindowsCmd(method,pat);
  } else if (command == "arrangewindowsstackbottom") {
    int method = ArrangeWindowsCmd::STACKBOTTOM;
    return new ArrangeWindowsCmd(method,pat);
  } else if (command == "unclutter") {
    return new UnclutterCmd(pat);
  }

  return 0;
} // parseWindowList

REGISTER_COMMAND_PARSER(attach, parseWindowList, void);
REGISTER_COMMAND_PARSER(nextwindow, parseWindowList, void);
REGISTER_COMMAND_PARSER(nextgroup, parseWindowList, void);
REGISTER_COMMAND_PARSER(prevwindow, parseWindowList, void);
REGISTER_COMMAND_PARSER(prevgroup, parseWindowList, void);
REGISTER_COMMAND_PARSER(arrangewindows, parseWindowList, void);
REGISTER_COMMAND_PARSER(arrangewindowsvertical, parseWindowList, void);
REGISTER_COMMAND_PARSER(arrangewindowshorizontal, parseWindowList, void);
REGISTER_COMMAND_PARSER(arrangewindowsstackleft, parseWindowList, void);
REGISTER_COMMAND_PARSER(arrangewindowsstackright, parseWindowList, void);
REGISTER_COMMAND_PARSER(arrangewindowsstacktop, parseWindowList, void);
REGISTER_COMMAND_PARSER(arrangewindowsstackbottom, parseWindowList, void);
REGISTER_COMMAND_PARSER(unclutter, parseWindowList, void);

} // end anonymous namespace

void AttachCmd::execute() {
  BScreen *screen = Shynebox::instance()->keyScreen();
  FocusControl::Focusables win_list(screen->focusControl().focusedOrderWinList().clientList() );

  ShyneboxWindow *first = 0;
  for (auto &it : win_list) {
    if (m_pat.match(*it) && it->sbwindow() ) {
      if (first == 0)
        first = it->sbwindow();
      else
        first->attachClient(it->sbwindow()->winClient() );
    }
  }
}

void NextWindowCmd::execute() {
  Shynebox::instance()->keyScreen()->cycleFocus(m_option, &m_pat, false);
}

void PrevWindowCmd::execute() {
  Shynebox::instance()->keyScreen()->cycleFocus(m_option, &m_pat, true);
}

tk::Command<void> *GoToWindowCmd::parse(const string &command,
                      const string &arguments, bool trusted) {
  (void) command;
  (void) trusted;
  int num = 1, opts;
  string args, pat;
  tk::StringUtil::extractNumber(arguments, num);
  string::size_type pos = arguments.find_first_of("({");

  if (pos != string::npos && pos != arguments.size() )
    args = arguments.c_str() + pos;
  FocusableList::parseArgs(args, opts, pat);
  return new GoToWindowCmd(num, opts, pat);
}

REGISTER_COMMAND_PARSER(gotowindow, GoToWindowCmd::parse, void);

void GoToWindowCmd::execute() {
  BScreen *screen = Shynebox::instance()->keyScreen();
  const FocusableList *win_list =
      FocusableList::getListFromOptions(*screen, m_option);
  screen->focusControl().goToWindowNumber(*win_list, m_num, &m_pat);
}

REGISTER_COMMAND(addworkspace, AddWorkspaceCmd, void);

void AddWorkspaceCmd::execute() {
  Shynebox::instance()->mouseScreen()->addWorkspace();
}

REGISTER_COMMAND(removelastworkspace, RemoveLastWorkspaceCmd, void);

void RemoveLastWorkspaceCmd::execute() {
  Shynebox::instance()->mouseScreen()->removeLastWorkspace();
}

namespace {

tk::Command<void> *parseIntCmd(const string &command, const string &args,
                           bool trusted) {
  (void) trusted;
  int num = 1;
  tk::StringUtil::extractNumber(args, num);

  if (command == "nextworkspace")
    return new NextWorkspaceCmd(num);
  else if (command == "prevworkspace")
    return new PrevWorkspaceCmd(num);
  else if (command == "rightworkspace")
    return new RightWorkspaceCmd(num);
  else if (command == "leftworkspace")
    return new LeftWorkspaceCmd(num);
  else if (command == "workspace")
    // workspaces appear 1-indexed to the user, hence the minus 1
    return new JumpToWorkspaceCmd(num - 1);
  return 0;
}

REGISTER_COMMAND_PARSER(nextworkspace, parseIntCmd, void);
REGISTER_COMMAND_PARSER(prevworkspace, parseIntCmd, void);
REGISTER_COMMAND_PARSER(rightworkspace, parseIntCmd, void);
REGISTER_COMMAND_PARSER(leftworkspace, parseIntCmd, void);
REGISTER_COMMAND_PARSER(workspace, parseIntCmd, void);

} // end anonymous namespace

void NextWorkspaceCmd::execute() {
  Shynebox::instance()->mouseScreen()->nextWorkspace(m_option);
}

void PrevWorkspaceCmd::execute() {
  Shynebox::instance()->mouseScreen()->prevWorkspace(m_option);
}

void LeftWorkspaceCmd::execute() {
  Shynebox::instance()->mouseScreen()->leftWorkspace(m_param);
}

void RightWorkspaceCmd::execute() {
  Shynebox::instance()->mouseScreen()->rightWorkspace(m_param);
}

JumpToWorkspaceCmd::JumpToWorkspaceCmd(int workspace_num):m_workspace_num(workspace_num) { }

void JumpToWorkspaceCmd::execute() {
  BScreen *screen = Shynebox::instance()->mouseScreen();

  screen->focusControl().stopCyclingFocus();
  int num = screen->numberOfWorkspaces();
  int actual = m_workspace_num;
  // workspace IDs actually start at 1 for user
  if (actual < 0)    actual += num;
  if (actual < 0)    actual = 0;
  if (actual >= num) actual = num - 1;
  screen->changeWorkspaceID(actual);
}

// TODO: window raises are left to user. but kind of expect raise personally
// TODO: better head logic/options to tile per head, all, only active
//       currently only does mouse head

/* NOTE: this is 'tiling' as most people know it
         arrangewindowsstackleft will make a main on right
         and tile rest on left
*/

/*
  try to arrange the windows on the current workspace in a 'clever' way.
  shaded windows are done separate from unshaded(normal) and will likely
  cover things up or be covered up
 */
void ArrangeWindowsCmd::execute() {
  BScreen *screen = Shynebox::instance()->mouseScreen();

  Workspace *space = screen->currentWorkspace();

  if (space->windowList().empty() )
    return;

  const int head = screen->getCurHead(); // mouse head
  Workspace::Windows normal_windows; // std::list
  Workspace::Windows shaded_windows;
  ShyneboxWindow *main_window = 0; // Main (big) window for stacked modes

  for (auto win : space->windowList() ) {
    int winhead = screen->getHead(win->sbWindow() );
    if ((winhead == head || winhead == 0) && m_pat.match(*win) ) {
      if (win->isShaded() ) // don't allow shaded to be the 'main' window
        shaded_windows.push_back(win);
      else if ((m_tile_method >= STACKLEFT) && win->isFocused() )
        main_window = win;
      else
        normal_windows.push_back(win);
    }
  } // for windowList

  // if stacking and no focused window
  // (e.g. mouse on diff monitor than focused window)
  // use last window as 'main' window
  if (main_window == 0 && (m_tile_method >= STACKLEFT) ) {
    if (!normal_windows.empty() ) {
      main_window = normal_windows.back();
      normal_windows.pop_back();
    }
  }

  const size_t norm_cnt  = normal_windows.size(),
               shade_cnt = shaded_windows.size();
  const unsigned int bw = norm_cnt != 0 ? normal_windows.front()->frame().window().borderWidth() * 2 : 0;
  unsigned int x_start = screen->maxLeft(head); // window position offset in x
  unsigned int y_start = screen->maxTop(head); // window position offset in y
  // remaining space to allow placements
  unsigned int max_right = screen->maxRight(head),
               max_bot = screen->maxBottom(head),
               max_width = max_right - x_start,
               max_height = max_bot - y_start;

  // If using a stacked mechanism we now need to place the main window.
  // Stacked mode only uses half the screen for tiled windows, so adjust
  // offset to half the screen (horizontal or vertical depending on mode)
  // Sets main_ vars for placement and resets above vars for tiling
  if (main_window != 0) {
    unsigned int main_x = x_start,   main_y = y_start,
                 main_w = max_width, main_h = max_height;
    switch (m_tile_method) {
      case STACKRIGHT:
        x_start = main_w = max_width = (max_width / 2);
        x_start += main_x; // strut corrections
        x_start += max_width & 1; // odd/rounding fixes
        main_w += max_width & 1;
        break;
      case STACKTOP:
        main_h = max_bot;
        main_y = max_height = (max_height / 2);
        main_y += y_start;
        main_h -= main_y;
        max_bot = main_y;
        break;
      case STACKBOTTOM:
        y_start = main_h = max_height = (max_height / 2);
        y_start += main_y;
        y_start += max_height & 1;
        main_h += max_height & 1;
        break;
      case STACKLEFT:
      default: // shouldn't default
        main_w = max_right;
        main_x = max_width = (max_width / 2);
        main_x += x_start;
        main_w -= main_x;
        max_right = main_x;
        break;
    } // switch title_method
    main_window->moveResize(main_x, main_y, main_w - bw, main_h - bw);
  } // if main_window

  unsigned int cols = 0, rows = 0, plc_col = x_start, plc_row = y_start;
  if (norm_cnt) {
    // try to get the same number of rows as columns.
    cols = int(sqrt((float)norm_cnt) ); // truncate to lower
    rows = int(0.99 + float(norm_cnt) / float(cols) );
    if ((m_tile_method == VERTICAL) // rotate if the user has asked for it or automagically
         || ((m_tile_method == UNSPECIFIED) && (max_width < max_height) ) )
      std::swap(cols, rows);
  } else if (shade_cnt) { // just shaded windows
    cols = max_width; // stops div by 0. ends up 1 below here
    rows = max_height;
  } else
    return; // no windows, nothing to do

  unsigned int each_w = max_width/cols,  // min width of every window
               each_h = max_height/rows; // min height of every window
  // rounding for col/rows can be pixels off
  // just assign extra windows in the last-col / first-row
  // it's a hassle trying to spread pixels evenly
  // and it's only a few pixels so there's no point
  unsigned int col_add = max_width % cols, col_rnd = 0,
               row_rnd = max_height % rows;

  // place shaded windows - THE PROBLEM
  /*
    basically, no matter how you dice it shaded windows become a pain here
    if you have too many, they will hide something or be hidden regardless

    you COULD try to give shaded their own section-space like another single
    window but it will either leave a big empty space with very few windows,
    or make an already high count of tiled windows harder to see. only happens
    to make maybe 5-10 total normal-windows look better but adds extra logic for
    little benefit (unless you have a decent but not overwhelming number of shades)

    so... just place them row->col for now. overlaping non-main (stacking) window
  */
  // TODO: shaded can be hidden by the toolbar, which is annoying.
  //       should always take tb placement into acct like updateStrut()
  if (shade_cnt) {
    for (auto shdwin : shaded_windows) {
      if (plc_row >= max_bot) { // who has this many shaded windows ?!?!?!?
        plc_row = y_start;
        plc_col += max_right/6; // random number choice
        if (plc_col >= max_right) { // wrap (lol did you shade EVERYTHING?)
          plc_col = x_start;
          plc_row = y_start;
        }
      }
      shdwin->move(plc_col, plc_row);
      plc_row += shdwin->frame().height();
    } // for shaded
    plc_col = x_start; // reset for normal windows
    plc_row = y_start;
  } // if shaded

  // place unshaded windows
  for (auto nrmwin : normal_windows) {
    if (plc_col >= max_right) {
      plc_col = x_start;
      plc_row += each_h + row_rnd;
      row_rnd = 0; // only use on first row
    }

    if (nrmwin == normal_windows.back() ) // last window gets more space
      each_w = max_right - plc_col;
    // every ending col win gets a bit extra space for rounding issues
    else if (plc_col + each_w + col_add >= max_right)
      col_rnd = col_add;

    nrmwin->moveResize(plc_col, plc_row,
                  each_w + col_rnd - bw,
                  each_h + row_rnd - bw);

    plc_col += each_w;
    if (col_rnd) {
      plc_col += col_rnd;
      col_rnd = 0;
    }
  }
} // ArrangeWindowsCmd execute

// re-place windows without a resize
void UnclutterCmd::execute() {
  BScreen *screen = Shynebox::instance()->mouseScreen();

  Workspace *space = screen->currentWorkspace();

  if (space->windowList().empty() )
    return;

  Workspace::Windows placed_windows;

  // placements only do current ws at most
  for (auto &win : space->windowList() ) {
    if (m_pat.match(*win) ) {
      placed_windows.push_back(win);
      win->move(-win->width(), -win->height() );
    }
  }

  if (placed_windows.empty() )
    return;

  // place
  for (auto win : placed_windows)
    win->placeWindow(win->getOnHead() );
} // UnclutterCmd

REGISTER_COMMAND(showdesktop, ShowDesktopCmd, void);

void ShowDesktopCmd::execute() {
  BScreen *screen = Shynebox::instance()->mouseScreen();

  // iconify windows in focus order, so it gets restored properly
  const std::list<Focusable *> wins =
          screen->focusControl().focusedOrderWinList().clientList();
  unsigned int space = screen->currentWorkspaceID();
  unsigned int count = 0;
  XGrabServer(Shynebox::instance()->display() );
  for (auto it : wins) {
    if (!it->sbwindow()->isIconic()
        && (it->sbwindow()->isStuck()
          || it->sbwindow()->workspaceNumber() == space)
        && it->sbwindow()->layerNum() < (int)tk::ResLayers_e::DESKTOP) {
      it->sbwindow()->iconify();
      count++;
    }
  }

  if (count == 0) {
    BScreen::Icons icon_list = screen->iconList();
    BScreen::Icons::reverse_iterator iconit = icon_list.rbegin();
    BScreen::Icons::reverse_iterator itend= icon_list.rend();
    for (; iconit != itend; ++iconit) {
      if ((*iconit)->workspaceNumber() == space || (*iconit)->isStuck() )
        (*iconit)->deiconify(false);
    }
  } else
    FocusControl::revertFocus(*screen);

  XUngrabServer(Shynebox::instance()->display() );
} // ShowDesktopCmd

REGISTER_COMMAND(toggletoolbarabove, ToggleToolbarAboveCmd, void);
void ToggleToolbarAboveCmd::execute() {
#if USE_TOOLBAR
  BScreen *screen = Shynebox::instance()->mouseScreen();
  screen->toolbar()->toggleAboveDock();
  const_cast<tk::SbWindow&>(screen->toolbar()->window() ).raise();
#endif
}

REGISTER_COMMAND(toggletoolbarvisible, ToggleToolbarHiddenCmd, void);
void ToggleToolbarHiddenCmd::execute() {
#if USE_TOOLBAR
  BScreen *screen = Shynebox::instance()->mouseScreen();
  screen->toolbar()->toggleHidden();
  const_cast<tk::SbWindow&>(screen->toolbar()->window() ).raise();
#endif
}

REGISTER_COMMAND(closeallwindows, CloseAllWindowsCmd, void);

void CloseAllWindowsCmd::execute() {
  BScreen *screen = Shynebox::instance()->mouseScreen();

  Workspace::Windows windows;

  for (auto workspace_it : screen->getWorkspacesList() ) {
    windows = workspace_it->windowList();
    for (auto it : windows)
      it->close();
  }

  windows = screen->iconList();
  for (auto it : windows)
    it->close();
}

REGISTER_COMMAND(setworkspacenamedialog, WorkspaceNameDialogCmd, void);

namespace {
class SetWSNameDialog: public TextDialog {
public:
  SetWSNameDialog():
      TextDialog(*Shynebox::instance()->mouseScreen(),
                 _SB_XTEXT(ConfigMenu, WorkspaceSet, "Set Current Workspace Name",
                 "set current workspace name ") ) {
    // NLS - don't know if this works, being in constructor
    setText(m_screen.currentWorkspace()->name() );
  }

private:
  void exec(const std::string &text) {
    m_screen.currentWorkspace()->setName(text);
#if USE_TOOLBAR
    m_screen.updateWorkspaceName(m_screen.currentWorkspaceID() );
    m_screen.updateToolbar(); // name may have increased tag size
#endif
  }
};
} // end anonymous namespace

void WorkspaceNameDialogCmd::execute() {
  _SB_USES_NLS;
  SetWSNameDialog *win = new SetWSNameDialog();
  win->show();
}

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                and Simon Bowden (rathnor at users.sourceforge.net)
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
