// CurrentWindowCmd.cc for Shynebox Window Manager

#include "CurrentWindowCmd.hh"

#include "shynebox.hh"
#include "Window.hh"
#include "WindowCmd.hh"
#include "Screen.hh"
#include "TextDialog.hh"
#include "WinClient.hh"

#include "FocusControl.hh"
#include "tk/CommandParser.hh"
#include "tk/I18n.hh"
#include "tk/StringUtil.hh"
#include "tk/RelCalcHelper.hh"
#include "tk/Config.hh"

#ifdef HAVE_CSTDLIB
#include <cstdlib>
#else
#include <stdlib.h>
#endif

#include <cstring> // strlen

using namespace std;
using namespace tk::StringUtil;
using tk::Command;

namespace {

void disableMaximizationIfNeeded(ShyneboxWindow& win) {
  if (win.isMaximized()
      || win.isMaximizedVert()
      || win.isMaximizedHorz()
      || win.isFullscreen() )
    win.disableMaximization();
}

tk::Command<void> *createCurrentWindowCmd(const string &command,
                             const string &args, bool trusted) {
  (void) args;
  (void) trusted;
  if (command == "minimizewindow" || command == "minimize" || command == "iconify")
    return new CurrentWindowCmd(&ShyneboxWindow::iconify);
  else if (command == "maximizewindow" || command == "maximize")
    return new CurrentWindowCmd(&ShyneboxWindow::maximizeFull);
  else if (command == "maximizevertical")
    return new CurrentWindowCmd(&ShyneboxWindow::maximizeVertical);
  else if (command == "maximizehorizontal")
    return new CurrentWindowCmd(&ShyneboxWindow::maximizeHorizontal);
  else if (command == "raise")
    return new CurrentWindowCmd(&ShyneboxWindow::raise);
  else if (command == "lower")
    return new CurrentWindowCmd(&ShyneboxWindow::lower);
  else if (command == "close")
    return new CurrentWindowCmd(&ShyneboxWindow::close);
  else if (command == "killwindow" || command == "kill")
    return new CurrentWindowCmd(&ShyneboxWindow::kill);
  else if (command == "shade" || command == "shadewindow")
    return new CurrentWindowCmd(&ShyneboxWindow::shade);
  else if (command == "shadeon" )
    return new CurrentWindowCmd(&ShyneboxWindow::shadeOn);
  else if (command == "shadeoff" )
    return new CurrentWindowCmd(&ShyneboxWindow::shadeOff);
  else if (command == "stick" || command == "stickwindow")
    return new CurrentWindowCmd(&ShyneboxWindow::stick);
  else if (command == "toggledecor")
    return new CurrentWindowCmd(&ShyneboxWindow::toggleDecoration);
  else if (command == "nexttab")
    return new CurrentWindowCmd(&ShyneboxWindow::nextClient);
  else if (command == "prevtab")
    return new CurrentWindowCmd(&ShyneboxWindow::prevClient);
  else if (command == "movetableft")
    return new CurrentWindowCmd(&ShyneboxWindow::moveClientLeft);
  else if (command == "movetabright")
    return new CurrentWindowCmd(&ShyneboxWindow::moveClientRight);
  else if (command == "detachclient")
    return new CurrentWindowCmd(&ShyneboxWindow::detachCurrentClient);
  else if (command == "windowmenu")
    return new CurrentWindowCmd(&ShyneboxWindow::popupMenu);
  return 0;
}

REGISTER_COMMAND_PARSER(minimizewindow, createCurrentWindowCmd, void);
REGISTER_COMMAND_PARSER(minimize, createCurrentWindowCmd, void);
REGISTER_COMMAND_PARSER(iconify, createCurrentWindowCmd, void);
REGISTER_COMMAND_PARSER(maximizewindow, createCurrentWindowCmd, void);
REGISTER_COMMAND_PARSER(maximize, createCurrentWindowCmd, void);
REGISTER_COMMAND_PARSER(maximizevertical, createCurrentWindowCmd, void);
REGISTER_COMMAND_PARSER(maximizehorizontal, createCurrentWindowCmd, void);
REGISTER_COMMAND_PARSER(raise, createCurrentWindowCmd, void);
REGISTER_COMMAND_PARSER(lower, createCurrentWindowCmd, void);
REGISTER_COMMAND_PARSER(close, createCurrentWindowCmd, void);
REGISTER_COMMAND_PARSER(killwindow, createCurrentWindowCmd, void);
REGISTER_COMMAND_PARSER(kill, createCurrentWindowCmd, void);
REGISTER_COMMAND_PARSER(shade, createCurrentWindowCmd, void);
REGISTER_COMMAND_PARSER(shadewindow, createCurrentWindowCmd, void);
REGISTER_COMMAND_PARSER(shadeon, createCurrentWindowCmd, void);
REGISTER_COMMAND_PARSER(shadeoff, createCurrentWindowCmd, void);
REGISTER_COMMAND_PARSER(stick, createCurrentWindowCmd, void);
REGISTER_COMMAND_PARSER(stickwindow, createCurrentWindowCmd, void);
REGISTER_COMMAND_PARSER(toggledecor, createCurrentWindowCmd, void);
REGISTER_COMMAND_PARSER(nexttab, createCurrentWindowCmd, void);
REGISTER_COMMAND_PARSER(prevtab, createCurrentWindowCmd, void);
REGISTER_COMMAND_PARSER(movetableft, createCurrentWindowCmd, void);
REGISTER_COMMAND_PARSER(movetabright, createCurrentWindowCmd, void);
REGISTER_COMMAND_PARSER(detachclient, createCurrentWindowCmd, void);
REGISTER_COMMAND_PARSER(windowmenu, createCurrentWindowCmd, void);

} // end anonymous namespace

void WindowHelperCmd::execute() {
  if (WindowCmd<void>::window() || FocusControl::focusedSbWindow() )
    real_execute();
}

ShyneboxWindow &WindowHelperCmd::sbwindow() {
  // will exist from execute above
  ShyneboxWindow *tmp = WindowCmd<void>::window();
  if (tmp)
    return *tmp;
  return *FocusControl::focusedSbWindow();
}

bool WindowHelperBoolCmd::execute() {
  if (WindowCmd<void>::window() || FocusControl::focusedSbWindow() )
    return real_execute();
  return false;
}

ShyneboxWindow &WindowHelperBoolCmd::sbwindow() {
  // will exist from execute above
  ShyneboxWindow *tmp = WindowCmd<void>::window();
  if (tmp)
    return *tmp;
  return *FocusControl::focusedSbWindow();
}

WinClient &WindowHelperBoolCmd::winclient() {
  // will exist from execute above
  WinClient *tmp = WindowCmd<void>::client();
  if (tmp)
    return *tmp;
  return *FocusControl::focusedWindow();
}

void CurrentWindowCmd::real_execute() {
  (sbwindow().*m_action)();
}

void ActivateTabCmd::real_execute() {
  Window root, last = 0,
         tab = Shynebox::instance()->lastEvent().xany.window;
  union {int i; unsigned int u;} jnk;
  WinClient *winclient = sbwindow().winClientOfLabelButtonWindow(tab);
  Display *dpy = Shynebox::instance()->display();
  while (!winclient && tab && tab != last) {
    last = tab;
    XQueryPointer(dpy, tab, &root, &tab, &jnk.i, &jnk.i, &jnk.i, &jnk.i, &jnk.u);
    winclient = sbwindow().winClientOfLabelButtonWindow(tab);
  }

  if (winclient && winclient != &sbwindow().winClient() )
    sbwindow().setCurrentClient(*winclient, true);
}

namespace {

tk::Command<void> *parseIntCmd(const string &command,
                  const string &args, bool trusted) {
  (void) trusted;
  int num = 1;
  extractNumber(args, num);

  if (command == "sethead")
    return new SetHeadCmd(num);
  else if (command == "tab")
    return new GoToTabCmd(num);
  else if (command == "sendtonextworkspace")
    return new SendToNextWorkspaceCmd(num);
  else if (command == "sendtoprevworkspace")
    return new SendToNextWorkspaceCmd(-num);
  else if (command == "taketonextworkspace")
    return new SendToNextWorkspaceCmd(num, true);
  else if (command == "taketoprevworkspace")
    return new SendToNextWorkspaceCmd(-num, true);
  else if (command == "sendtoworkspace")
    return new SendToWorkspaceCmd(num);
  else if (command == "taketoworkspace")
    return new SendToWorkspaceCmd(num, true);
  else if (command == "sendtonexthead")
    return new SendToNextHeadCmd(num);
  else if (command == "sendtoprevhead")
    return new SendToNextHeadCmd(-num);
  return 0;
}

REGISTER_COMMAND_PARSER(sethead, parseIntCmd, void);
REGISTER_COMMAND_PARSER(tab, parseIntCmd, void);
REGISTER_COMMAND_PARSER(sendtonextworkspace, parseIntCmd, void);
REGISTER_COMMAND_PARSER(sendtoprevworkspace, parseIntCmd, void);
REGISTER_COMMAND_PARSER(taketonextworkspace, parseIntCmd, void);
REGISTER_COMMAND_PARSER(taketoprevworkspace, parseIntCmd, void);
REGISTER_COMMAND_PARSER(sendtoworkspace, parseIntCmd, void);
REGISTER_COMMAND_PARSER(taketoworkspace, parseIntCmd, void);
REGISTER_COMMAND_PARSER(sendtonexthead, parseIntCmd, void);
REGISTER_COMMAND_PARSER(sendtoprevhead, parseIntCmd, void);

tk::Command<void> *parseFocusCmd(const string &command,
                    const string &args, bool trusted) {
  (void) command;
  (void) trusted;
  ClientPattern pat(args.c_str() );
  if (!pat.error() )
    return tk::CommandParser<void>::instance().parse("GoToWindow 1 " + args);
  return new CurrentWindowCmd((CurrentWindowCmd::Action)
                              &ShyneboxWindow::focus);
}

REGISTER_COMMAND_PARSER(activate, parseFocusCmd, void);
REGISTER_COMMAND_PARSER(focus, parseFocusCmd, void);


REGISTER_COMMAND(activatetab, ActivateTabCmd, void);

class SetXPropCmd: public WindowHelperCmd {
public:
  explicit SetXPropCmd(const tk::SbString& name, const tk::SbString& value) :
      m_name(name), m_value(value) { }

protected:
  void real_execute() {
    WinClient& client = sbwindow().winClient();
    Atom prop = XInternAtom(client.display(), m_name.c_str(), False);

    client.changeProperty(prop, XInternAtom(client.display(), "UTF8_STRING", False), 8,
                    PropModeReplace, (unsigned char*)m_value.c_str(), m_value.size() );
  }

private:
  tk::SbString m_name;
  tk::SbString m_value;
};

tk::Command<void> *parseSetXPropCmd(const string &command,
                       const string &args, bool trusted) {
  (void) command;
  SetXPropCmd* cmd = 0;

  if (trusted) {
    tk::SbString name = args;
    removeFirstWhitespace(name);
    removeTrailingWhitespace(name);

    if (name.size() > 1 && name[0] != '=') {  // the smallest valid argument is 'X='
      tk::SbString value;
      size_t eq = name.find('=');
      if (eq != name.npos && eq != name.size() ) {
        value.assign(name, eq + 1, name.size() );
        name.resize(eq);
      }
      cmd = new SetXPropCmd(name, value);
    }
  } // if trusted
  return cmd;
}

REGISTER_COMMAND_PARSER(setxprop, parseSetXPropCmd, void);

} // end anonymous namespace

void SetHeadCmd::real_execute() {
  int num = m_head;
  int total = sbwindow().screen().numHeads();
  if (total < 2)
    return;
  if (num < 0)
    num += total + 1;
  num = clamp(num, 1, total);
  sbwindow().setOnHead(num);
}

void SendToWorkspaceCmd::real_execute() {
  int num = m_workspace_num;
  int total = sbwindow().screen().numberOfWorkspaces();
  if (num < 0)
    num += total + 1;
  num = clamp(num, 1, total);
  sbwindow().screen().sendToWorkspace(num-1, &sbwindow(), m_take);
}

void SendToNextWorkspaceCmd::real_execute() {
  int total = sbwindow().screen().numberOfWorkspaces();
  const int ws_nr = (total + (sbwindow().workspaceNumber() + m_delta % total) ) % total;
  sbwindow().screen().sendToWorkspace(ws_nr, &sbwindow(), m_take);
}

void SendToNextHeadCmd::real_execute() {
  int total = sbwindow().screen().numHeads();
  if (total < 2)
    return;
  int num = (total + sbwindow().getOnHead() + (m_delta % total) ) % total;
  num = clamp(num, 1, total);
  sbwindow().setOnHead(num);
}

void GoToTabCmd::real_execute() {
  int num = m_tab_num;
  if (num < 0)
    num += sbwindow().numClients() + 1;
  num = clamp(num, 1, sbwindow().numClients() );

  ShyneboxWindow::ClientList::iterator it = sbwindow().clientList().begin();

  while (--num > 0) ++it;

  (*it)->focus();
}

REGISTER_COMMAND(startmoving, StartMovingCmd, void);

void StartMovingCmd::real_execute() {
  int x;
  int y;
  const XEvent &last = Shynebox::instance()->lastEvent();
  switch (last.type) {
  case ButtonPress:
    x = last.xbutton.x_root;
    y = last.xbutton.y_root;
    break;
  case MotionNotify:
    x = last.xmotion.x_root;
    y = last.xmotion.y_root;
    break;
  default:
    return;
  }
  sbwindow().startMoving(x, y);
}

tk::Command<void> *StartResizingCmd::parse(const string &cmd,
                          const string &args, bool trusted) {
  (void) cmd;
  (void) trusted;
  ShyneboxWindow::ResizeModel mode = ShyneboxWindow::BOTTOMRIGHTRESIZE;
  int corner_size_px = 0;
  int corner_size_pc = 0;
  vector<string> tokens;
  stringtok(tokens, args);

  if (!tokens.empty() ) {
    string arg = toLower(tokens[0]);
    if (arg == "center")
      mode = ShyneboxWindow::CENTERRESIZE;
    else if (arg == "topleft")
      mode = ShyneboxWindow::TOPLEFTRESIZE;
    else if (arg == "top")
      mode = ShyneboxWindow::TOPRESIZE;
    else if (arg == "topright")
      mode = ShyneboxWindow::TOPRIGHTRESIZE;
    else if (arg == "left")
      mode = ShyneboxWindow::LEFTRESIZE;
    else if (arg == "right")
      mode = ShyneboxWindow::RIGHTRESIZE;
    else if (arg == "bottomleft")
      mode = ShyneboxWindow::BOTTOMLEFTRESIZE;
    else if (arg == "bottom")
      mode = ShyneboxWindow::BOTTOMRESIZE;
    else if (arg == "nearestcorner") {
      mode = ShyneboxWindow::EDGEORCORNERRESIZE;
      corner_size_pc = 100;
    } else if (arg == "nearestedge") {
      mode = ShyneboxWindow::EDGEORCORNERRESIZE;
    } else if (arg == "nearestcorneroredge") {
      mode = ShyneboxWindow::EDGEORCORNERRESIZE;
      /* The NearestCornerOrEdge can be followed by a corner
       * size in one of three forms:
       *      <size in pixels>
       *      <size in pixels> <size in percent>
       *      <size in percent>%
       * If no corner size is given then it defaults to 50 pixels, 30%. */
      if (tokens.size() > 1) {
        const char * size1 = tokens[1].c_str();
        if (size1[strlen(size1)-1] == '%')
          corner_size_pc = atoi(size1);
        else {
          corner_size_px = atoi(size1);
          if (tokens.size() > 2)
            corner_size_pc = atoi(tokens[2].c_str() );
        }
      } else {
        corner_size_px = 50;
        corner_size_pc = 30;
      }
    } // nearestcorneredge
    //else if (arg == "bottomright") // default
    //  mode = ShyneboxWindow::BOTTOMRIGHTRESIZE;
  } // if tokens
  return new StartResizingCmd(mode, corner_size_px, corner_size_pc);
} // StartResizingCmd

REGISTER_COMMAND_PARSER(startresizing, StartResizingCmd::parse, void);

void StartResizingCmd::real_execute() {
  int x;
  int y;
  const XEvent &last = Shynebox::instance()->lastEvent();
  switch (last.type) {
  case ButtonPress:
    x = last.xbutton.x_root;
    y = last.xbutton.y_root;
    break;
  case MotionNotify:
    x = last.xmotion.x_root;
    y = last.xmotion.y_root;
    break;
  default:
    return;
  }

  x -= sbwindow().x() - sbwindow().frame().window().borderWidth();
  y -= sbwindow().y() - sbwindow().frame().window().borderWidth();

  sbwindow().startResizing(x, y, sbwindow().getResizeDirection(
            x, y, m_mode, m_corner_size_px, m_corner_size_pc) );
}

REGISTER_COMMAND(starttabbing, StartTabbingCmd, void);

void StartTabbingCmd::real_execute() {
  const XEvent &last = Shynebox::instance()->lastEvent();
  if (last.type == ButtonPress) {
    const XButtonEvent &be = last.xbutton;
    sbwindow().startTabbing(be);
  }
}

tk::Command<void> *MoveCmd::parse(const string &command,
                     const string &args, bool trusted) {
  (void) trusted;
  int dx = 0, dy = 0;
  vector<string> tokens;
  stringtok(tokens, args, " ");

  switch (tokens.size() ) {
  case 2:  extractNumber(tokens[1], dy);
  case 1:  extractNumber(tokens[0], dx);
  default: break;
  }

  if (command == "moveright")
    dy = 0;
  else if (command == "moveleft") {
    dy = 0;
    dx = -dx;
  } else if (command == "movedown") {
    dy = dx;
    dx = 0;
  } else if (command == "moveup") {
    dy = -dx;
    dx = 0;
  }

  return new MoveCmd(dx, dy);
}

REGISTER_COMMAND_PARSER(move, MoveCmd::parse, void);
REGISTER_COMMAND_PARSER(moveright, MoveCmd::parse, void);
REGISTER_COMMAND_PARSER(moveleft, MoveCmd::parse, void);
REGISTER_COMMAND_PARSER(moveup, MoveCmd::parse, void);
REGISTER_COMMAND_PARSER(movedown, MoveCmd::parse, void);

MoveCmd::MoveCmd(const int step_size_x, const int step_size_y) :
  m_step_size_x(step_size_x), m_step_size_y(step_size_y) { }

void MoveCmd::real_execute() {
  if (sbwindow().isMaximized() || sbwindow().isFullscreen() ) {
    if (sbwindow().screen().getMaxDisableMove() )
      return;
    sbwindow().setMaximizedState(WindowState::MAX_NONE);
  }

  sbwindow().move(sbwindow().x() + m_step_size_x, sbwindow().y() + m_step_size_y);
}

tk::Command<void> *ResizeCmd::parse(const string &command,
                       const string &args, bool trusted) {
  (void) trusted;
  int dx = 0, dy = 0;
  bool is_relative_x = false, is_relative_y = false,
       ignore_x = false, ignore_y = false;
  vector<string> tokens;
  stringtok(tokens, args);

  if (tokens.size() < 1)
    return 0;

  if (command == "resizehorizontal")
    dx = parseSizeToken(tokens[0], is_relative_x, ignore_x);
  else if (command == "resizevertical")
    dy = parseSizeToken(tokens[0], is_relative_y, ignore_y);
  else {
    if (tokens.size() < 2)
      return 0;
    dx = parseSizeToken(tokens[0], is_relative_x, ignore_x);
    dy = parseSizeToken(tokens[1], is_relative_y, ignore_y);
  }

  if (command == "resizeto")
    return new ResizeToCmd(dx, dy, is_relative_x, is_relative_y);

  return new ResizeCmd(dx, dy, is_relative_x, is_relative_y);
}

REGISTER_COMMAND_PARSER(resize, ResizeCmd::parse, void);
REGISTER_COMMAND_PARSER(resizeto, ResizeCmd::parse, void);
REGISTER_COMMAND_PARSER(resizehorizontal, ResizeCmd::parse, void);
REGISTER_COMMAND_PARSER(resizevertical, ResizeCmd::parse, void);

ResizeCmd::ResizeCmd(const int step_size_x, const int step_size_y,
                           bool is_relative_x, bool is_relative_y) :
    m_step_size_x(step_size_x), m_step_size_y(step_size_y),
    m_is_relative_x(is_relative_x), m_is_relative_y(is_relative_y) { }

void ResizeCmd::real_execute() {
  if ((sbwindow().isMaximized() || sbwindow().isFullscreen() )
      && sbwindow().screen().getMaxDisableResize() )
    return;

  disableMaximizationIfNeeded(sbwindow() );

  int dx = m_step_size_x, windowWidth = sbwindow().width(),
      dy = m_step_size_y, windowHeight = sbwindow().height();

  // NOTE: removed width/heightInc that was divisor after relcalc
  //       relcalc also rounded up before shynebox which made this
  //       even more oversized on odd-sized resolutions/struts
  if (m_is_relative_x)
    dx = static_cast<int>(tk::RelCalcHelper::calPercentageValueOf(windowWidth, m_step_size_x) );

  if (m_is_relative_y)
    dy = static_cast<int>(tk::RelCalcHelper::calPercentageValueOf(windowHeight, m_step_size_y) );

  int w = max<int>(static_cast<int>(windowWidth + dx),
                   sbwindow().frame().titlebarHeight() * 2 + 10);
  int h = max<int>(static_cast<int>(windowHeight + dy),
                   sbwindow().frame().titlebarHeight() + 10);

  sbwindow().resize(w, h);
} // ResizeCmd real_execute

tk::Command<void> *MoveToCmd::parse(const string &cmd,
                   const string &args, bool trusted) {
  (void) cmd;
  (void) trusted;
  ShyneboxWindow::ReferenceCorner refc = ShyneboxWindow::LEFTTOP;
  int x = 0, y = 0;
  bool ignore_x = false, ignore_y = false,
       is_relative_x = false, is_relative_y = false;
  vector<string> tokens;
  stringtok(tokens, args);

  if (tokens.size() < 2)
    return 0;

  x = parseSizeToken(tokens[0], is_relative_x, ignore_x);
  y = parseSizeToken(tokens[1], is_relative_y, ignore_y);

  if (tokens.size() >= 3)
    refc = ShyneboxWindow::getCorner(tokens[2]);

  return new MoveToCmd(x, y, ignore_x, ignore_y, is_relative_x, is_relative_y, refc);
}

REGISTER_COMMAND_PARSER(moveto, MoveToCmd::parse, void);

void MoveToCmd::real_execute() {
  if ((sbwindow().isMaximized() || sbwindow().isFullscreen() )
       && sbwindow().screen().getMaxDisableMove() )
    return;

  disableMaximizationIfNeeded(sbwindow() );

  int x = m_pos_x, y = m_pos_y;
  int head = sbwindow().getOnHead();

  if (m_ignore_x)
    x = sbwindow().x();
  else {
    if (m_is_relative_x)
      x = sbwindow().screen().calRelativeWidth(head, x);
    sbwindow().translateXCoords(x, m_corner);
  }

  if (m_ignore_y)
    y = sbwindow().y();
  else {
    if (m_is_relative_y)
      y = sbwindow().screen().calRelativeHeight(head, y);
    sbwindow().translateYCoords(y, m_corner);
  }

  sbwindow().move(x, y);
}


ResizeToCmd::ResizeToCmd(const int step_size_x, const int step_size_y,
                    const bool is_relative_x, const bool is_relative_y)
  : m_step_size_x(step_size_x), m_step_size_y(step_size_y),
    m_is_relative_x(is_relative_x), m_is_relative_y(is_relative_y) { }

void ResizeToCmd::real_execute() {
  if ((sbwindow().isMaximized() || sbwindow().isFullscreen() )
       && sbwindow().screen().getMaxDisableResize() )
    return;

  disableMaximizationIfNeeded(sbwindow() );

  int dx = m_step_size_x, dy = m_step_size_y;
  int head = sbwindow().getOnHead();

  if (m_is_relative_x) {
    dx = sbwindow().screen().calRelativeWidth(head, dx);
    dx -= 2 * sbwindow().frame().window().borderWidth();
    if (dx <= 0)
      dx = sbwindow().width();
  }

  if (m_is_relative_y) {
    dy = sbwindow().screen().calRelativeHeight(head, dy);
    dy -= 2 * sbwindow().frame().window().borderWidth();
    if (dy <= 0)
      dy = sbwindow().height();
  }

  if (dx == 0)
    dx = sbwindow().width();
  if (dy == 0)
    dy = sbwindow().height();

  sbwindow().resize(dx, dy);
}

REGISTER_COMMAND(fullscreen, FullscreenCmd, void);

void FullscreenCmd::real_execute() {
  sbwindow().setFullscreen(!sbwindow().isFullscreen() );
}

tk::Command<void> *SetLayerCmd::parse(const string &command,
                                      const string &args, bool trusted) {
  (void) command;
  (void) trusted;
  int l = tk::ResLayers_strnum.sz - 1;

  if (!extractNumber(args, l) ) {
    string v = toUpper(args);
    for (; l > 1 ; l--)
      if (tk::ResLayers_strnum.estr[l] == v)
        break;
  }

  return new SetLayerCmd(l);
}

REGISTER_COMMAND_PARSER(setlayer, SetLayerCmd::parse, void);

void SetLayerCmd::real_execute() {
  sbwindow().moveToLayer(m_layer);
}

tk::Command<void> *ChangeLayerCmd::parse(const string &command,
                            const string &args, bool trusted) {
  (void) trusted;
  int num = 1;
  extractNumber(args, num);

  if (command == "raiselayer")
    num *= -1;
  // "defaults" to "lowerlayer", less str compare
  return new ChangeLayerCmd(num);
}

REGISTER_COMMAND_PARSER(raiselayer, ChangeLayerCmd::parse, void);
REGISTER_COMMAND_PARSER(lowerlayer, ChangeLayerCmd::parse, void);

void ChangeLayerCmd::real_execute() {
  sbwindow().moveToLayer(sbwindow().layerNum() + m_diff);
}

namespace {
class SetTitleDialog: public TextDialog {
public:
  SetTitleDialog(ShyneboxWindow &win, const string &title):
      TextDialog(win.screen(), title), window(win) {
    setText(win.title() );
  }

private:
  void exec(const string &text) {
    if (window.winClient() == 0
        || window.winClient().sbwindow() == 0) {
      delete this;
      return;
    }
    bool managed = text == "" ? false : true;
    window.winClient().setTitle(text, managed);
    window.winClient().sbwindow()->frame().clearAll();
  }

  ShyneboxWindow &window;
};
} // end anonymous namespace

REGISTER_COMMAND(settitledialog, SetTitleDialogCmd, void);

void SetTitleDialogCmd::real_execute() {
  _SB_USES_NLS;
  SetTitleDialog *win = new SetTitleDialog(sbwindow(),
                     _SB_XTEXT(Windowmenu, SetTitle, "Set Title",
                    "Change the title of the window") );
  win->show();
}

REGISTER_COMMAND_WITH_ARGS(settitle, SetTitleCmd, void);

void SetTitleCmd::real_execute() {
  bool managed = title == "" ? false : true;
  sbwindow().winClient().setTitle(title, managed);
}

REGISTER_COMMAND_WITH_ARGS(setdecor, SetDecorCmd, void);

SetDecorCmd::SetDecorCmd(const string &args):
  m_mask(WindowState::getDecoMaskFromString(args) ) { }

void SetDecorCmd::real_execute() {
  sbwindow().setDecorationMask(m_mask);
}

REGISTER_COMMAND_WITH_ARGS(matches, MatchCmd, bool);

bool MatchCmd::real_execute() {
  return m_pat.match(winclient() );
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
