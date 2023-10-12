// IconbarTool.cc for Shynebox Window Manager

#include "IconbarTool.hh"

#include "shynebox.hh"
#include "Screen.hh"
#include "IconbarTheme.hh"
#include "Window.hh"
#include "IconButton.hh"
#include "SbMenu.hh"
#include "WinClient.hh"
#include "FocusControl.hh"
#include "SbCommands.hh"
#include "Debug.hh"

#include "tk/I18n.hh"
#include "tk/ImageControl.hh"
#include "tk/BoolMenuItem.hh"
#include "tk/LayerManager.hh"
#include "tk/MacroCommand.hh"
#include "tk/Menu.hh"
#include "tk/MenuSeparator.hh"
#include "tk/RadioMenuItem.hh"
#include "tk/SimpleCommand.hh"

#include <typeinfo>
#include <iterator>
#include <cstring>

using std::string;
using std::list;

#define BTAlignEnum tk::ButtonTrainAlignment_e

namespace {

class ToolbarModeMenuItem : public tk::RadioMenuItem {
public:
  ToolbarModeMenuItem(const tk::SbString &label, IconbarTool &handler,
                      string mode) :
        tk::RadioMenuItem(label),
        m_handler(handler), m_mode(mode) {
    setCloseOnClick(false);
  }
  bool isSelected() const { return m_handler.mode() == m_mode; }
  void click(int button, int time, unsigned int mods) {
    m_handler.setMode(m_mode);
    tk::RadioMenuItem::click(button, time, mods);
    Shynebox::instance()->save_rc();
  }

private:
  IconbarTool &m_handler;
  string m_mode;
};

class ToolbarAlignMenuItem: public tk::RadioMenuItem {
public:
  ToolbarAlignMenuItem(const tk::SbString &label, IconbarTool &handler,
                      BTAlignEnum mode) :
        tk::RadioMenuItem(label),
        m_handler(handler), m_mode(mode) {
    setCloseOnClick(false);
  }
  bool isSelected() const { return m_handler.alignment() == m_mode; }
  void click(int button, int time, unsigned int mods) {
    m_handler.setAlignment(m_mode);
    tk::RadioMenuItem::click(button, time, mods);
    Shynebox::instance()->save_rc();
  }

private:
  IconbarTool &m_handler;
  BTAlignEnum m_mode;
};

enum {
    L_TITLE = 0,
    L_MODE_NONE,
    L_MODE_ICONS,
    L_MODE_NO_ICONS,
    L_MODE_ICONS_WORKSPACE,
    L_MODE_NOICONS_WORKSPACE,
    L_MODE_WORKSPACE,
    L_MODE_ALL,

    L_LEFT,
    L_CENTER,
    L_RELATIVE,
    L_RELATIVE_SMART,
    L_RIGHT,
};

void setupModeMenu(tk::Menu &menu, IconbarTool &handler) {
  using namespace tk;
  _SB_USES_NLS;

  static const SbString _labels[] = {
      _SB_XTEXT(Toolbar, IconbarMode, "Iconbar", "Menu title - chooses which set of icons are shown in the iconbar"),
      _SB_XTEXT(Toolbar, IconbarModeNone, "None", "No icons are shown in the iconbar"),
      _SB_XTEXT(Toolbar, IconbarModeIcons, "Icons", "Iconified windows from all workspaces are shown"),
      _SB_XTEXT(Toolbar, IconbarModeNoIcons, "NoIcons", "No iconified windows from all workspaces are shown"),
      _SB_XTEXT(Toolbar, IconbarModeWorkspaceIcons, "WorkspaceIcons", "Iconified windows from this workspace are shown"),
      _SB_XTEXT(Toolbar, IconbarModeWorkspaceNoIcons, "WorkspaceNoIcons", "No iconified windows from this workspace are shown"),
      _SB_XTEXT(Toolbar, IconbarModeWorkspace, "Workspace", "Normal and iconified windows from this workspace are shown"),
      _SB_XTEXT(Toolbar, IconbarModeAllWindows, "All Windows", "All windows are shown"),

      _SB_XTEXT(Align, Left, "Left", "Align to the left"),
      _SB_XTEXT(Align, Center, "Center", "Align to the center"), // not in NLS
      _SB_XTEXT(Align, Relative, "Relative", "Align relative to the width"),
      _SB_XTEXT(Align, RelativeSmart, "Relative (Smart)", "Align relative to the width, but let elements vary according to size of title"),
      _SB_XTEXT(Align, Right, "Right", "Align to the right"),
  };

  menu.setLabel(_labels[L_TITLE]);
  menu.insertItem(new ToolbarModeMenuItem(_labels[L_MODE_NONE], handler, "none") );
  menu.insertItem(new ToolbarModeMenuItem(_labels[L_MODE_ICONS], handler, "{static groups} (minimized=yes)") );
  menu.insertItem(new ToolbarModeMenuItem(_labels[L_MODE_NO_ICONS], handler, "{static groups} (minimized=no)") );
  menu.insertItem(new ToolbarModeMenuItem(_labels[L_MODE_ICONS_WORKSPACE], handler, "{static groups} (minimized=yes) (workspace)") );
  menu.insertItem(new ToolbarModeMenuItem(_labels[L_MODE_NOICONS_WORKSPACE], handler, "{static groups} (minimized=no) (workspace)") );
  menu.insertItem(new ToolbarModeMenuItem(_labels[L_MODE_WORKSPACE], handler, "{static groups} (workspace)") );
  menu.insertItem(new ToolbarModeMenuItem(_labels[L_MODE_ALL], handler, "{static groups}") );

  menu.insertItem(new tk::MenuSeparator() );

  menu.insertItem(new ToolbarAlignMenuItem(_labels[L_LEFT], handler, BTAlignEnum::LEFT) );
  menu.insertItem(new ToolbarAlignMenuItem(_labels[L_CENTER], handler, BTAlignEnum::CENTER) );
  menu.insertItem(new ToolbarAlignMenuItem(_labels[L_RELATIVE], handler, BTAlignEnum::RELATIVE) );
  menu.insertItem(new ToolbarAlignMenuItem(_labels[L_RELATIVE_SMART], handler, BTAlignEnum::RELATIVE_SMART) );
  menu.insertItem(new ToolbarAlignMenuItem(_labels[L_RIGHT], handler, BTAlignEnum::RIGHT) );

  menu.insertItem(new tk::MenuSeparator() );

  menu.updateMenu();
} // setupModeMenu

class ShowMenu: public tk::Command<void> {
public:
  explicit ShowMenu(ShyneboxWindow &win):m_win(win) { }
  void execute() {
    tk::Menu::hideShownMenu();
    // get last button pos
    const XEvent &e = Shynebox::instance()->lastEvent();
    m_win.popupMenu(e.xbutton.x_root, e.xbutton.y_root);
  }
private:
  ShyneboxWindow &m_win;
};

class FocusCommand: public tk::Command<void> {
public:
  explicit FocusCommand(Focusable &win): m_win(win) { }
  void execute() {
    tk::Menu::hideShownMenu();
    // this needs to be a local variable, as this object could be destroyed
    // if the workspace is changed.
    ShyneboxWindow *sbwin = m_win.sbwindow();
    if (!sbwin)
      return;
    if (m_win.isFocused() )
      sbwin->iconify();
    else {
      m_win.focus();
      sbwin->raise();
    }
  }

private:
  Focusable &m_win;
};

} // end anonymous namespace

std::string IconbarTool::s_iconifiedDecoration[2];

IconbarTool::IconbarTool(const tk::SbWindow &parent, IconbarTheme &theme,
                         tk::ThemeProxy<IconbarTheme> &focused_theme,
                         tk::ThemeProxy<IconbarTheme> &unfocused_theme,
                         BScreen &screen, tk::Menu &menu):
      ToolbarItem(ToolbarItem::RELATIVE),
      m_screen(screen),
      m_icon_container(parent, false),
      m_theme(theme),
      m_focused_theme(focused_theme),
      m_unfocused_theme(unfocused_theme),
      m_empty_pm(screen.imageControl() ),
      m_winlist(new FocusableList(screen) ),
      m_mode("none"),
      m_rc_mode(*screen.m_cfgmap["iconbar.mode"]),
      m_rc_alignment((BTAlignEnum&)(int&)(*screen.m_cfgmap["iconbar.alignment"]) ),
      m_rc_client_width(*screen.m_cfgmap["iconbar.iconWidth"]),
      m_rc_client_padding(*screen.m_cfgmap["iconbar.iconTextPadding"]),
      m_rc_use_pixmap(*screen.m_cfgmap["iconbar.usePixmap"]),
      m_menu(screen.menuTheme(), screen.imageControl(),
             *screen.layerManager().getLayer((int)tk::ResLayers_e::MENU) ) {

  updateIconifiedPattern();

  // setup mode menu
  setupModeMenu(m_menu, *this);
  _SB_USES_NLS;
  using namespace tk;

  // setup use pixmap item to reconfig iconbar and save resource on click
  MacroCommand *sv_recfg = new MacroCommand();
  sv_recfg->add(*(Command<void>*)(new SimpleCommand<IconbarTool>(*this, &IconbarTool::renderTheme) ) );
  sv_recfg->add(*(Command<void>*)(Shynebox::instance()->getSharedSaveRC() ) );
  m_menu.insertItem(new tk::BoolMenuItem(_SB_XTEXT(Toolbar, ShowIcons,
                  "Show Pictures", "chooses if little icons are shown next to title in the iconbar"),
                  m_rc_use_pixmap, *sv_recfg) );
  m_menu.updateMenu();

  // add iconbar menu to toolbar menu
  menu.insertSubmenu(m_menu.label().logical(), &m_menu);

  // must be internal menu, otherwise toolbar main menu tries to delete it.
  m_menu.setInternalMenu();

  m_locker_timer.setTimeout(10 * tk::SbTime::IN_MILLISECONDS); // 10 millisec
  m_locker_timer.fireOnce(true);
  tk::SimpleCommand<IconbarTool> *usig(new tk::SimpleCommand<IconbarTool>(*this, &IconbarTool::unlockSig) );
  m_locker_timer.setCommand(*usig);
  themeReconfigured();

  m_icon_container.setAlignment((BTAlignEnum)m_rc_alignment);
} // IconbarTool class init

IconbarTool::~IconbarTool() {
  deleteIcons();
  delete m_winlist;
} // IconbarTool class destroy

void IconbarTool::move(int x, int y) {
  m_icon_container.move(x, y);
}

void IconbarTool::updateMaxSizes(unsigned int width, unsigned int height) {
  const unsigned int maxsize = (m_icon_container.orientation() >= tk::ROT90) ? height : width;
  m_icon_container.setMaxTotalSize(maxsize);

  // LEFT/CENTER/RIGHT
  if (m_rc_alignment <= BTAlignEnum::RIGHT) {
    m_rc_client_width = std::clamp(m_rc_client_width, 10, 400);
    m_icon_container.setMaxSizePerClient(m_rc_client_width);
  } else
    m_icon_container.setMaxSizePerClient(maxsize/std::max(1, m_icon_container.size() ) );
}

void IconbarTool::resize(unsigned int width, unsigned int height) {
  m_icon_container.resize(width, height);
  renderTheme();
}

void IconbarTool::moveResize(int x, int y,
                             unsigned int width, unsigned int height) {
  m_icon_container.moveResize(x, y, width, height);
  renderTheme();
}

void IconbarTool::show() {
  m_icon_container.show();
}

void IconbarTool::hide() {
  m_icon_container.hide();
}

// for config menu
void IconbarTool::setAlignment(BTAlignEnum align) {
  m_rc_alignment = align;
  m_icon_container.setAlignment((BTAlignEnum)m_rc_alignment);
  update(ALIGN, 0);
  m_menu.reconfigure();
}

void IconbarTool::setMode(string mode) {
  if (mode == m_mode)
    return;

  m_rc_mode = m_mode = mode;

  // lock graphics update
  m_icon_container.setUpdateLock(true);

  if (m_winlist)
    delete m_winlist;

  if (mode == "none")
    m_winlist = new FocusableList(m_screen);
  else
    m_winlist = new FocusableList(m_screen, mode + " (iconhidden=no)");

  reset();

  // unlock graphics update
  m_icon_container.setUpdateLock(false);
  m_icon_container.update();
  m_icon_container.showSubwindows();

  renderTheme();

  m_menu.reconfigure();
} // setMode

// for incremental changes, back off for a bit
// prevents slowdown from say, making 200 windows
void IconbarTool::resetLock() {
  m_lock_gfx = true;
  m_locker_timer.start();
}

void IconbarTool::unlockSig() {
  m_lock_gfx = false;
  m_icon_container.setUpdateLock(false);
  m_icon_container.update();
  m_icon_container.showSubwindows();
  update(ALIGN, 0);
}

unsigned int IconbarTool::width() const {
  return m_icon_container.width();
}

unsigned int IconbarTool::preferredWidth() const {
  // border and paddings
  unsigned int w = 2*borderWidth() + m_rc_client_padding * m_icons.size();

  // the buttons
  for (auto it : m_icons)
    w += it.second->preferredWidth();

  return w;
}

unsigned int IconbarTool::height() const {
  return m_icon_container.height();
}

unsigned int IconbarTool::borderWidth() const {
  return m_icon_container.borderWidth();
}

void IconbarTool::themeReconfigured() {
  setMode(m_rc_mode);
}

void IconbarTool::update(UpdateReason reason, Focusable *win) {
  // ignore updates if we're shutting down
  if (m_screen.isShuttingdown() ) {
    if (!m_icons.empty() )
      deleteIcons();
    return;
  }

  // the problem with FF popups
  // is that the window list isnt reset before this call(?)
  // FIX: added 'istransient' check below
  // not sure why focusablelist didnt get that window before?
  // clientlistsig was in 1.3.7 f-box-class and focuscontrol
  // createwindow (which calls this with ADD) emit'ed clientlistsig
  // must have been lucky order of ops? no way to tell with signal twine-yarn

  // lock graphic update
  m_icon_container.setUpdateLock(true);

  switch(reason) {
    case LIST_ADD:
      if (!win->isTransient() )
        insertWindow(*win);
      resetLock();
      break;
    case LIST_REMOVE:
      removeWindow(*win);
      resetLock();
      break;
    case LIST_RESET:
      reset();
      break;
    case ALIGN:
      break;
  }

  updateMaxSizes(width(), height() );

  if (m_lock_gfx)
    return;

  // unlock container and update graphics
  m_icon_container.setUpdateLock(false);
  m_icon_container.update();
  m_icon_container.showSubwindows();

  // renderTheme() is needed due to multiple alignment types
  // (smart)relative will require a update for every button
  // focus/unfocus always requires at least 2 button updates
  // may be able to reduce overhead for focus or fixed aligns
  // but not really worth it
  renderTheme();
} // update

void IconbarTool::updateIconifiedPattern() {
  std::string p = *m_screen.m_cfgmap["iconbar.iconifiedPattern"];
  size_t tidx = p.find("%t");
  s_iconifiedDecoration[0].clear();
  s_iconifiedDecoration[1].clear();
  if (tidx != std::string::npos) {
    s_iconifiedDecoration[0] = p.substr(0, tidx);
    s_iconifiedDecoration[1] = p.substr(tidx+2);
  }
}

void IconbarTool::insertWindow(Focusable &win) {
  IconButton *button = 0;

  IconMap::iterator icon_it = m_icons.find(&win);
  if (icon_it != m_icons.end() )
    button = icon_it->second;

  if (button)
    m_icon_container.removeItem(button);
  else
    button = makeButton(win);

  if (!button) return;

  m_icon_container.insertItem(button);
}

void IconbarTool::reset() {
  deleteIcons();
  m_winlist->reset();

  for (auto it : m_winlist->clientList() )
    if (it->sbwindow() )
      insertWindow(*it);

  renderTheme();
}

// called right below and by toolbar for loop
void IconbarTool::updateSizing() {
  m_icon_container.setBorderWidth(m_theme.border().width() );
  m_icon_container.setBorderColor(m_theme.border().color() );
}

void IconbarTool::renderTheme() {
  // update button sizes before container gets max width per client!
  updateSizing();

  // if we dont have any icons then we should render empty texture
  if (!m_theme.emptyTexture().usePixmap() ) {
    m_empty_pm.reset(0);
    m_icon_container.setBackgroundColor(m_theme.emptyTexture().color() );
  } else {
    m_empty_pm.reset(m_screen.imageControl().
                     renderImage(m_icon_container.width(),
                                 m_icon_container.height(),
                                 m_theme.emptyTexture(), orientation() ) );
    m_icon_container.setBackgroundPixmap(m_empty_pm);
  }

  // update buttons
  for (auto icon_it : m_icons)
    renderButton(*icon_it.second);
}

void IconbarTool::renderButton(IconButton &button, bool clear) {
  button.setPixmap(m_rc_use_pixmap);
  button.setTextPadding(m_rc_client_padding);
  button.reconfigTheme(); // NOTE: this is where multiple redraws get slow

  if (clear)
    button.clear();
}

void IconbarTool::deleteIcons() {
  m_icon_container.removeAll();
  for (auto it : m_icons)
    delete it.second;
  m_icons.clear();
}

void IconbarTool::removeWindow(Focusable &win) {
  // got window die signal, lets find and remove the window
  IconMap::iterator it = m_icons.find(&win);
  if (it == m_icons.end() )
    return;

  sbdbg<<"IconbarTool::"<<__FUNCTION__<<"( 0x"<<&win<<" title = "<<win.title().logical()<<") found!\n";

  // remove from list and render theme again
  IconButton *button = it->second;
  m_icons.erase(it);
  m_icon_container.removeItem(button);
  delete button;
}

IconButton *IconbarTool::makeButton(Focusable &win) {
  // we just want windows that have clients
  ShyneboxWindow *sbwin = win.sbwindow();
  if (!sbwin || sbwin->clientList().empty() )
    return 0;

  sbdbg<<"IconbarTool::makeButton(0x"<<&win<<" title = "<<win.title().logical()<<")\n";

  IconButton *button = new IconButton(m_icon_container, m_focused_theme,
                                      m_unfocused_theme, win);

  FocusCommand *focus_cmd(new ::FocusCommand(win) );
  ShowMenu *menu_cmd(new ::ShowMenu(*sbwin) );
  button->setOnClick(*focus_cmd, 1);
  button->setOnClick(*menu_cmd, 3);

  renderButton(*button, false); // update the attributes, but don't clear it
  m_icons[&win] = button;
  return button;
}

void IconbarTool::setOrientation(tk::Orientation orient) {
  m_icon_container.setOrientation(orient);
  ToolbarItem::setOrientation(orient);
}

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
