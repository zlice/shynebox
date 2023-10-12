// SbWinFrame.cc for Shynebox Window Manager

#include "SbWinFrame.hh"

#include "Keys.hh"
#include "SbWinFrameTheme.hh"
#include "Screen.hh"
#include "FocusableTheme.hh"
#include "IconButton.hh"
#include "RectangleUtil.hh"

#include "tk/ImageControl.hh"
#include "tk/LayerItem.hh"
#include "tk/LayerManager.hh"
#include "tk/EventManager.hh"
#include "tk/SimpleCommand.hh"
#include "tk/TextUtils.hh"
#include "tk/Shape.hh"
#include "tk/Config.hh"

#include <X11/X.h>

#define ContAlignEnum tk::ButtonTrainAlignment_e
#define TabPlaceEnum tk::TabPlacement_e
using std::max;
using std::string;

namespace {

enum { UNFOCUS = 0, FOCUS, PRESSED };

const long s_mask = ButtonPressMask | ButtonReleaseMask | ButtonMotionMask | EnterWindowMask | LeaveWindowMask;

const struct {
    tk::Orientation orient;
    ContAlignEnum align;
} s_place[] = { // ordered by TabPlaceEnum
    { /* TabPlaceEnum::TOPLEFT,     */ tk::ROT0,   ContAlignEnum::LEFT,   },
    { /* TabPlaceEnum::TOP,         */ tk::ROT0,   ContAlignEnum::CENTER, },
    { /* TabPlaceEnum::TOPRIGHT,    */ tk::ROT0,   ContAlignEnum::RIGHT,  },
    { /* TabPlaceEnum::BOTTOMLEFT,  */ tk::ROT0,   ContAlignEnum::LEFT,   },
    { /* TabPlaceEnum::BOTTOM,      */ tk::ROT0,   ContAlignEnum::CENTER, },
    { /* TabPlaceEnum::BOTTOMRIGHT, */ tk::ROT0,   ContAlignEnum::RIGHT,  },
    { /* TabPlaceEnum::LEFTTOP,     */ tk::ROT270, ContAlignEnum::RIGHT,  },
    { /* TabPlaceEnum::LEFT,        */ tk::ROT270, ContAlignEnum::CENTER, },
    { /* TabPlaceEnum::LEFTBOTTOM,  */ tk::ROT270, ContAlignEnum::LEFT,   },
    { /* TabPlaceEnum::RIGHTTOP,    */ tk::ROT90,  ContAlignEnum::LEFT,   },
    { /* TabPlaceEnum::RIGHT,       */ tk::ROT90,  ContAlignEnum::LEFT,   },
    { /* TabPlaceEnum::RIGHTBOTTOM, */ tk::ROT90,  ContAlignEnum::LEFT,   },
};

// renders to pixmap or sets color
void render(tk::Color &col, Pixmap &pm, unsigned int width, unsigned int height,
            const tk::Texture &tex,
            tk::ImageControl& ictl,
            tk::Orientation orient = tk::ROT0) {
  Pixmap tmp = pm;
  if (!tex.usePixmap() ) {
    pm = None;
    col = tex.color();
  } else
    pm = ictl.renderImage(width, height, tex, orient);

  if (tmp)
    ictl.removeImage(tmp);
}

void bg_pm_or_color(tk::SbWindow& win, const Pixmap& pm, const tk::Color& color) {
  if (pm)
    win.setBackgroundPixmap(pm);
  else
    win.setBackgroundColor(color);
}

} // end anonymous

SbWinFrame::SbWinFrame(BScreen &screen, int client_depth,
                       WindowState &state,
                       FocusableTheme<SbWinFrameTheme> &theme):
    m_screen(screen),
    m_theme(theme),
    m_imagectrl(screen.imageControl() ),
    m_state(state),
    m_window(theme->screenNum(), state.x, state.y, state.width, state.height, s_mask, true, false,
        client_depth, InputOutput,
        (client_depth == screen.rootWindow().maxDepth() ? screen.rootWindow().visual() : CopyFromParent),
        (client_depth == screen.rootWindow().maxDepth() ? screen.rootWindow().colormap() : CopyFromParent) ),
    m_layeritem(new tk::LayerItem(window(), *screen.layerManager().getLayer((int)tk::ResLayers_e::NORMAL) ) ),
    m_titlebar(m_window, 0, 0, 100, 16, s_mask, false, false,
        screen.rootWindow().decorationDepth(), InputOutput,
        screen.rootWindow().decorationVisual(),
        screen.rootWindow().decorationColormap() ),
    m_tab_container(m_titlebar),
    m_label(m_titlebar, m_theme->font(), tk::BiDiString("") ),
    m_handle(m_window, 0, 0, 100, 5, s_mask, false, false,
        screen.rootWindow().decorationDepth(), InputOutput,
        screen.rootWindow().decorationVisual(),
        screen.rootWindow().decorationColormap() ),
    m_grip_right(m_handle, 0, 0, 10, 4, s_mask, false, false,
        screen.rootWindow().decorationDepth(), InputOutput,
        screen.rootWindow().decorationVisual(),
        screen.rootWindow().decorationColormap() ),
    m_grip_left(m_handle, 0, 0, 10, 4, s_mask, false, false,
        screen.rootWindow().decorationDepth(), InputOutput,
        screen.rootWindow().decorationVisual(),
        screen.rootWindow().decorationColormap() ),
    m_clientarea(m_window, 0, 0, 100, 100, s_mask),
    m_bevel(1),
    m_use_titlebar(true),
    m_use_tabs(true),
    m_use_handle(true),
    m_visible(false),
    m_tabmode(screen.getDefaultInternalTabs()?INTERNAL:EXTERNAL),
    m_active_orig_client_bw(0),
    m_need_render(true),
    m_button_size(1),
    m_shape(new tk::Shape(m_window, theme->shapePlace() ) ) {

  init();
} // SbWinFrame class init

SbWinFrame::~SbWinFrame() {
  removeEventHandler();
  removeAllButtons();
  delete m_layeritem;
  delete m_shape;
} // SbWinFrame class destroy

// this function acts as an updater for the tab container
bool SbWinFrame::setTabMode(TabMode tabmode) {
  if (m_tabmode == tabmode)
    return false;

  tk::ButtonTrain& tabs = tabcontainer();
  bool ret = true;

  // setting tabmode to notset forces it through when
  // something is likely to change
  if (tabmode == NOTSET)
    tabmode = m_tabmode;

  m_tabmode = tabmode;

  // reparent tab container
  if (tabmode == EXTERNAL) {
    m_label.show();
    tabs.setBorderWidth(m_window.borderWidth() );
    tabs.setEventMask(s_mask);
    alignTabs();

    if (m_use_tabs && m_visible)
      tabs.show();
    else {
      ret = false;
      tabs.hide();
    }
  } else {
    tabs.setUpdateLock(true);

    tabs.setAlignment(ContAlignEnum::RELATIVE);
    tabs.setOrientation(tk::ROT0);
    // when you toggle internal/external this can happen
    if (tabs.parent()->window() == m_screen.rootWindow().window() ) {
      m_layeritem->removeWindow(m_tab_container);
      tabs.hide();
      tabs.reparent(m_titlebar, m_label.x(), m_label.y() );
      tabs.invalidateBackground();
      tabs.resize(m_label.width(), m_label.height() );
      tabs.raise();
    }
    tabs.setBorderWidth(0);
    tabs.setMaxTotalSize(0);
    tabs.setUpdateLock(false);
    tabs.setMaxSizePerClient(0);

    renderTabs();
    applyTabs();

    tabs.clear();
    tabs.raise();
    tabs.show();

    if (!m_use_tabs)
      ret = false;

    m_label.hide();
  }
  return ret;
} // setTabMode

void SbWinFrame::hide() {
  m_window.hide();
  if (m_tabmode == EXTERNAL && m_use_tabs)
    m_tab_container.hide();

  m_visible = false;
}

void SbWinFrame::show() {
  m_visible = true;

  if (m_need_render) {
    renderAll();
    applyAll();
    clearAll();
  }

  if (m_tabmode == EXTERNAL && m_use_tabs)
    m_tab_container.show();

  m_window.showSubwindows();
  m_window.show();
}

void SbWinFrame::move(int x, int y) {
  moveResize(x, y, 0, 0, true, false);
}

void SbWinFrame::resize(unsigned int width, unsigned int height) {
  moveResize(0, 0, width, height, false, true);
}

// need an atomic moveresize where possible
void SbWinFrame::moveResizeForClient(int x, int y,
                                     unsigned int width, unsigned int height,
                                     int win_gravity,
                                     unsigned int client_bw,
                                     bool move, bool resize) {
  if (resize) // these fns check if the elements are "on"
    height += titlebarHeight() + handleHeight();

  setActiveGravity(win_gravity, client_bw);
  moveResize(x, y, width, height, move, resize);
}

void SbWinFrame::resizeForClient(unsigned int width, unsigned int height,
                                 int win_gravity, unsigned int client_bw) {
  moveResizeForClient(0, 0, width, height, win_gravity, client_bw, false, true);
}

void SbWinFrame::moveResize(int x, int y, unsigned int width, unsigned int height,
                                               bool move, bool resize, bool force) {
  if (!force && move && x == window().x() && y == window().y() )
    move = false;

  if (!force && resize
      && width == SbWinFrame::width() && height == SbWinFrame::height() )
    resize = false;

  if (!move && !resize)
    return;

  if (move && resize)
    m_window.moveResize(x, y, width, height);
  else if (move) // this stuff will be caught by reconfigure if resized
    m_window.move(x, y);
  else
    m_window.resize(width, height);

  m_state.saveGeometry(window().x(), window().y(),
                       window().width(), window().height() );

  if (move || (resize && m_screen.getTabPlacement() != TabPlaceEnum::TOPLEFT
                      && m_screen.getTabPlacement() != TabPlaceEnum::LEFTTOP) )
    alignTabs();

  if (resize) {
    if (m_tabmode == EXTERNAL) {
      unsigned int s = width;
      if (s_place[(int)m_screen.getTabPlacement()].orient != tk::ROT0)
        s = height;
      m_tab_container.setMaxTotalSize(s);
    }
    reconfigure();
  }
} // moveResize

void SbWinFrame::quietMoveResize(int x, int y,
                                 unsigned int width, unsigned int height) {
  m_window.moveResize(x, y, width, height);
  m_state.saveGeometry(window().x(), window().y(),
                       window().width(), window().height() );
  if (m_tabmode == EXTERNAL) {
    unsigned int s = width;
    if (s_place[(int)m_screen.getTabPlacement()].orient != tk::ROT0)
      s = height;
    m_tab_container.setMaxTotalSize(s);
    alignTabs();
  }
}

void SbWinFrame::alignTabs() {
  if (m_tabmode != EXTERNAL)
    return;

  tk::ButtonTrain& tabs = tabcontainer();
  tk::Orientation orig_orient = tabs.orientation();
  const unsigned int orig_tabwidth = tabs.maxWidthPerClient();

  tabs.setMaxSizePerClient(m_screen.getTabWidth() );

  int bw = window().borderWidth();
  int size = width();
  int tab_x = x();
  int tab_y = y();

  TabPlaceEnum p = m_screen.getTabPlacement();
  if (orig_orient != s_place[(int)p].orient)
    tabs.hide();
  if (s_place[(int)p].orient != tk::ROT0)
    size = height();
  tabs.setOrientation(s_place[(int)p].orient);
  tabs.setAlignment(s_place[(int)p].align);
  tabs.setMaxTotalSize(size);

  int w = static_cast<int>(width() );
  int h = static_cast<int>(height() );
  int xo = xOffset();
  int yo = yOffset();
  int tw = static_cast<int>(tabs.width() );
  int th = static_cast<int>(tabs.height() );

  switch (p) {
  case TabPlaceEnum::TOPLEFT:                          tab_y -= yo;         break;
  case TabPlaceEnum::TOP:         tab_x += (w - tw)/2; tab_y -= yo;         break;
  case TabPlaceEnum::TOPRIGHT:    tab_x +=  w - tw;    tab_y -= yo;         break;
  case TabPlaceEnum::BOTTOMLEFT:                       tab_y +=  h + bw;    break;
  case TabPlaceEnum::BOTTOM:      tab_x += (w - tw)/2; tab_y +=  h + bw;    break;
  case TabPlaceEnum::BOTTOMRIGHT: tab_x +=  w - tw;    tab_y +=  h + bw;    break;
  case TabPlaceEnum::LEFTTOP:     tab_x -=  xo;                             break;
  case TabPlaceEnum::LEFT:        tab_x -=  xo;        tab_y += (h - th)/2; break;
  case TabPlaceEnum::LEFTBOTTOM:  tab_x -=  xo;        tab_y +=  h - th;    break;
  case TabPlaceEnum::RIGHTTOP:    tab_x +=  w + bw;                         break;
  case TabPlaceEnum::RIGHT:       tab_x +=  w + bw;    tab_y += (h - th)/2; break;
  case TabPlaceEnum::RIGHTBOTTOM: tab_x +=  w + bw;    tab_y +=  h - th;    break;
  }

  if (tabs.orientation() != orig_orient
      || tabs.maxWidthPerClient() != orig_tabwidth) {
    renderTabs();
    if (m_visible && m_use_tabs) {
      applyTabs();
      tabs.clear();
      tabs.show();
    }
  }

  if (tabs.parent()->window() != m_screen.rootWindow().window() ) {
    tabs.reparent(m_screen.rootWindow(), tab_x, tab_y);
    tabs.clear();
    m_layeritem->addWindow(tabs);
  } else
    tabs.move(tab_x, tab_y);
} // alignTabs

void SbWinFrame::clearAll() {
  if (m_use_titlebar) {
    redrawTitlebar(); // only called here
    for (auto it : m_buttons_left)
      it->clear();
    for (auto it : m_buttons_right )
      it->clear();
  } else if (m_tabmode == EXTERNAL && m_use_tabs)
    m_tab_container.clear();

  if (m_use_handle) {
    m_handle.clear();
    m_grip_left.clear();
    m_grip_right.clear();
  }
}

void SbWinFrame::setFocus(bool newvalue) {
  if (m_state.focused == newvalue)
    return;

  m_state.focused = newvalue;
  setBorderWidth();
  applyAll();
  clearAll();
}

void SbWinFrame::applyState() {
  applyDecorations(false);

  const int head = m_screen.getHead(window() );
  const bool full_max  = m_screen.doFullMax(),
             mind_tabs = !full_max && !m_screen.getMaxOverTabs();
  int new_x = m_state.x, new_y = m_state.y;
  unsigned int new_w = m_state.width, new_h = m_state.height;

  if (m_state.isMaximizedVert() ) {
    new_y = m_screen.maxTop(head, full_max);
    new_h = m_screen.maxBottom(head, full_max)
          - new_y - 2*window().borderWidth();
    if (mind_tabs) {
      new_y += yOffset();
      new_h -= heightOffset();
    }
  }
  if (m_state.isMaximizedHorz() ) {
    new_x = m_screen.maxLeft(head, full_max);
    new_w = m_screen.maxRight(head, full_max)
          - new_x - 2*window().borderWidth();
    if (mind_tabs) {
      new_x += xOffset();
      new_w -= widthOffset();
    }
  }

  if (m_state.shaded)
    new_h = m_titlebar.height();
  else if (m_state.fullscreen) {
    new_x = m_screen.getHeadX(head);
    new_y = m_screen.getHeadY(head);
    new_w = m_screen.getHeadWidth(head);
    new_h = m_screen.getHeadHeight(head);
  } else if (m_state.maximized == WindowState::MAX_NONE || !m_screen.getMaxIgnoreIncrement() )
    applySizeHints(new_w, new_h, true);

  moveResize(new_x, new_y, new_w, new_h, true, true, true);
} // applyState

void SbWinFrame::addLeftButton(tk::Button *btn) {
  if (btn == 0) // valid button?
    return;

  applyButton(*btn); // setup theme and other stuff
  m_buttons_left.push_back(btn);
}

void SbWinFrame::addRightButton(tk::Button *btn) {
  if (btn == 0) // valid button?
    return;

  applyButton(*btn); // setup theme and other stuff
  m_buttons_right.push_back(btn);
}

void SbWinFrame::removeAllButtons() {
  for (auto it : m_buttons_left)
    delete it;
  m_buttons_left.clear();
  for (auto it : m_buttons_right)
    delete it;
  m_buttons_right.clear();
}

void SbWinFrame::createTab(tk::Button &button) {
  button.show();
  button.setEventMask(ExposureMask | ButtonPressMask |
                      ButtonReleaseMask | ButtonMotionMask |
                      EnterWindowMask);
  tk::EventManager::instance()->add(button, button.window() );

  m_tab_container.insertItem(&button);
}

void SbWinFrame::removeTab(IconButton *btn) {
  if (m_tab_container.removeItem(btn) )
    delete btn;
}

void SbWinFrame::moveLabelButtonLeft(tk::TextButton &btn) {
  m_tab_container.moveItem(&btn, -1);
}

void SbWinFrame::moveLabelButtonRight(tk::TextButton &btn) {
  m_tab_container.moveItem(&btn, +1);
}

void SbWinFrame::moveLabelButtonLeftOf(tk::TextButton &btn, const tk::TextButton &dest) {
  int dest_pos = m_tab_container.find(&dest);
  int cur_pos = m_tab_container.find(&btn);
  if (dest_pos < 0 || cur_pos < 0)
    return;

  int movement=dest_pos - cur_pos;
  if (movement > 0)
    movement-=1;

  m_tab_container.moveItem(&btn, movement);
}

void SbWinFrame::moveLabelButtonRightOf(tk::TextButton &btn, const tk::TextButton &dest) {
  int dest_pos = m_tab_container.find(&dest);
  int cur_pos = m_tab_container.find(&btn);
  if (dest_pos < 0 || cur_pos < 0 )
    return;

  int movement=dest_pos - cur_pos;
  if (movement < 0)
    movement+=1;

  m_tab_container.moveItem(&btn, movement);
}

void SbWinFrame::setClientWindow(tk::SbWindow &win) {
  win.setBorderWidth(0);

  XChangeSaveSet(win.display(), win.window(), SetModeInsert);

  m_window.setEventMask(NoEventMask);

  // we need to mask this so we don't get unmap event
  win.setEventMask(NoEventMask);
  win.reparent(m_window, clientArea().x(), clientArea().y() );

  m_window.setEventMask(SubstructureRedirectMask | ButtonMotionMask
           | ButtonPressMask | ButtonReleaseMask | VisibilityChangeMask
           | EnterWindowMask | LeaveWindowMask);

  XFlush(win.display() );

  // remask window so we get events
  XSetWindowAttributes attrib_set;
  attrib_set.event_mask = PropertyChangeMask | StructureNotifyMask | FocusChangeMask | KeyPressMask;
  attrib_set.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask | ButtonMotionMask;
  XChangeWindowAttributes(win.display(), win.window(), CWEventMask|CWDontPropagate, &attrib_set);

  if (isVisible() )
    win.show();
  win.raise();
  m_window.showSubwindows();
}

bool SbWinFrame::hideTabs() {
  if (m_tabmode == INTERNAL || !m_use_tabs) {
    m_use_tabs = false;
    return false;
  }

  m_use_tabs = false;
  m_tab_container.hide();
  return true;
}

bool SbWinFrame::showTabs() {
  if (m_tabmode == INTERNAL || m_use_tabs) {
    m_use_tabs = true;
    return false; // nothing changed
  }

  m_use_tabs = true;
  if (m_visible)
    m_tab_container.show();
  return true;
}

bool SbWinFrame::hideTitlebar() {
  if (!m_use_titlebar)
    return false;

  m_titlebar.hide();
  m_use_titlebar = false;

  int h = height();
  int th = m_titlebar.height();
  int tbw = m_titlebar.borderWidth();

  // only take away one borderwidth
  // (as the other border is still the "top" border)
  h = std::max(1, h - th - tbw);
  m_window.resize(m_window.width(), h); // bork

  return true;
}

bool SbWinFrame::showTitlebar() {
  if (m_use_titlebar)
    return false;

  m_titlebar.show();
  m_use_titlebar = true;

  // only add one borderwidth
  // (as the other border is still the "top" border)
  m_window.resize(m_window.width(),
       m_window.height() + m_titlebar.height() +
       m_titlebar.borderWidth() );

  return true;
}

bool SbWinFrame::hideHandle() {
  if (!m_use_handle)
    return false;

  m_handle.hide();
  m_grip_left.hide();
  m_grip_right.hide();
  m_use_handle = false;

  int h = m_window.height();
  int hh = m_handle.height();
  int hbw = m_handle.borderWidth();

  // only take away one borderwidth
  // (as the other border is still the "top" border)
  h = std::max(1, h - hh - hbw);
  m_window.resize(m_window.width(), h);

  return true;
}

bool SbWinFrame::showHandle() {
  if (m_use_handle || theme()->handleWidth() == 0)
    return false;

  m_use_handle = true;

  // weren't previously rendered...
  renderHandles();
  applyHandles();

  m_handle.show();
  m_handle.showSubwindows(); // shows grips

  m_window.resize(m_window.width(),
      m_window.height() + m_handle.height() +
      m_handle.borderWidth() );

  return true;
}

// Set new event handler for the frame's windows
void SbWinFrame::setEventHandler(tk::EventHandler &evh) {
  tk::EventManager &evm = *tk::EventManager::instance();
  evm.add(evh, m_tab_container);
  evm.add(evh, m_label);
  evm.add(evh, m_titlebar);
  evm.add(evh, m_handle);
  evm.add(evh, m_grip_right);
  evm.add(evh, m_grip_left);
  evm.add(evh, m_window);
}

// remove event handler from windows
void SbWinFrame::removeEventHandler() {
  tk::EventManager &evm = *tk::EventManager::instance();
  evm.remove(m_tab_container);
  evm.remove(m_label);
  evm.remove(m_titlebar);
  evm.remove(m_handle);
  evm.remove(m_grip_right);
  evm.remove(m_grip_left);
  evm.remove(m_window);
}

void SbWinFrame::exposeEvent(XExposeEvent &event) {
  tk::SbWindow* win = 0;
  if (m_titlebar == event.window)
    win = &m_titlebar;
  else if (m_tab_container == event.window)
    win = &m_tab_container;
  else if (m_label == event.window)
    win = &m_label;
  else if (m_handle == event.window)
    win = &m_handle;
  else if (m_grip_left == event.window)
    win = &m_grip_left;
  else if (m_grip_right == event.window)
    win = &m_grip_right;
  else {
    if (m_tab_container.tryExposeEvent(event) )
      return;

    for (auto &lr : {m_buttons_left, m_buttons_right} ) {
    for (auto &it : lr) {
      if (it->window() == event.window) {
        it->exposeEvent(event);
        return;
      }
    }
    }
    return;
  }

  win->clearArea(event.x, event.y, event.width, event.height);
} // exposeEvent

void SbWinFrame::handleEvent(XEvent &event) {
  if (event.type == ConfigureNotify && event.xconfigure.window == window().window() )
    configureNotifyEvent(event.xconfigure);
}

void SbWinFrame::configureNotifyEvent(XConfigureEvent &event) {
  resize(event.width, event.height);
}

void SbWinFrame::reconfigure() {
  if (m_tab_container.empty() )
    return;

  int grav_x=0, grav_y=0;
  // negate gravity
  gravityTranslate(grav_x, grav_y, -sizeHints().win_gravity, m_active_orig_client_bw);

  m_bevel = theme()->bevelWidth();

  unsigned int orig_handle_h = handle().height();
  if (m_use_handle && orig_handle_h != theme()->handleWidth() )
    m_window.resize(m_window.width(), m_window.height() -
                    orig_handle_h + theme()->handleWidth() );

  handle().resize(handle().width(), theme()->handleWidth() );
  gripLeft().resize(buttonHeight(), theme()->handleWidth() );
  gripRight().resize(gripLeft().width(), gripLeft().height() );

  // align titlebar and render it
  if (m_use_titlebar) {
    reconfigureTitlebar();
    m_titlebar.raise();
  } else
    m_titlebar.lower();

  if (m_tabmode == EXTERNAL) {
    unsigned int h = buttonHeight();
    unsigned int w = m_tab_container.width();
    if (s_place[(int)m_screen.getTabPlacement()].orient != tk::ROT0) {
      w = m_tab_container.height();
      std::swap(w, h);
    }
    m_tab_container.resize(w, h);
    alignTabs();
  }

  // leave client+grips alone if we're shaded (it'll get fixed when we unshade)
  if (!m_state.shaded || m_state.fullscreen) {
    int client_top = 0;
    int client_height = m_window.height();
    if (m_use_titlebar) {
      // only one borderwidth as titlebar is really at -borderwidth
      int titlebar_height = m_titlebar.height() + m_titlebar.borderWidth();
      client_top += titlebar_height;
      client_height -= titlebar_height;
    }

    // align handle and grips
    const int grip_height = m_handle.height();
    const int grip_width = 20; // hardcoded grip width
    const int handle_bw = static_cast<signed>(m_handle.borderWidth() );

    int ypos = m_window.height();

    // if the handle isn't on, it's actually below the window
    if (m_use_handle)
      ypos -= grip_height + handle_bw;

    // we do handle settings whether on or not so that if they get toggled
    // then things are ok...
    m_handle.invalidateBackground();
    m_handle.moveResize(-handle_bw, ypos,
                        m_window.width(), grip_height);

    m_grip_left.invalidateBackground();
    m_grip_left.moveResize(-handle_bw, -handle_bw,
                           grip_width, grip_height);

    m_grip_right.invalidateBackground();
    m_grip_right.moveResize(m_handle.width() - grip_width - handle_bw, -handle_bw,
                            grip_width, grip_height);

    if (m_use_handle) {
      m_handle.raise();
      client_height -= m_handle.height() + m_handle.borderWidth();
    } else
      m_handle.lower();

    m_clientarea.moveResize(0, client_top,
                            m_window.width(), client_height);
  } // if !shaded or is fullscreen

  gravityTranslate(grav_x, grav_y, sizeHints().win_gravity, m_active_orig_client_bw);
  // if the location changes, shift it
  if (grav_x != 0 || grav_y != 0)
    move(grav_x + x(), grav_y + y() );

  // render the theme
  if (isVisible() ) {
    renderAll();
    applyAll();
    clearAll();
  } else
    m_need_render = true;

  m_shape->setPlaces(getShape() );
  m_shape->setShapeOffsets(0, titlebarHeight() );
  // titlebar stuff rendered already by reconftitlebar
} // reconfigure

void SbWinFrame::updateShape() {
  m_shape->update();
}

void SbWinFrame::setShapingClient(tk::SbWindow *win, bool always_update) {
  m_shape->setShapeSource(win, 0, titlebarHeight(), always_update);
}

unsigned int SbWinFrame::buttonHeight() const {
  return m_titlebar.height() - m_bevel*2;
}

// private funcs below here

void SbWinFrame::redrawTitlebar() {
  if (!m_use_titlebar || m_tab_container.empty() )
    return;

  m_tab_container.clear();
  m_label.clear();
  m_titlebar.clear();
}

void SbWinFrame::reconfigureTitlebar() {
  int orig_height = m_titlebar.height();
  // resize titlebar to window size with font height
  int title_height = theme()->font().height() == 0 ? 16 :
      theme()->font().height() + m_bevel*2 + 2;
  if (theme()->titleHeight() != 0)
    title_height = theme()->titleHeight();

  // if the titlebar grows in size, make sure the whole window does too
  if (orig_height != title_height)
    m_window.resize(m_window.width(), m_window.height()-orig_height+title_height);
  m_titlebar.invalidateBackground();
  m_titlebar.moveResize(-m_titlebar.borderWidth(), -m_titlebar.borderWidth(),
                        m_window.width(), title_height);

  // draw left buttons first
  unsigned int next_x = m_bevel;
  unsigned int button_size = buttonHeight();
  m_button_size = button_size;
  for (size_t i=0; i < m_buttons_left.size(); i++, next_x += button_size + m_bevel) {
    // probably on theme reconfigure, leave bg alone for now
    m_buttons_left[i]->invalidateBackground();
    m_buttons_left[i]->moveResize(next_x, m_bevel,
                                  button_size, button_size);
  }

  next_x += m_bevel;

  // space left on titlebar between left and right buttons
  int space_left = m_titlebar.width() - next_x;

  if (!m_buttons_right.empty() )
    space_left -= m_buttons_right.size() * (button_size + m_bevel);

  space_left -= m_bevel;

  if (space_left <= 0)
    space_left = 1;

  m_label.invalidateBackground();
  m_label.moveResize(next_x, m_bevel, space_left, button_size);

  m_tab_container.invalidateBackground();
  if (m_tabmode == INTERNAL)
    m_tab_container.moveResize(next_x, m_bevel, space_left, button_size);
  else {
    if (m_use_tabs) {
      if (m_tab_container.orientation() == tk::ROT0)
        m_tab_container.resize(m_tab_container.width(), button_size);
      else
        m_tab_container.resize(button_size, m_tab_container.height() );
    }
  }

  next_x += m_label.width() + m_bevel;

  // finaly set new buttons to the right
  for (size_t i=0; i < m_buttons_right.size();
       ++i, next_x += button_size + m_bevel) {
    m_buttons_right[i]->invalidateBackground();
    m_buttons_right[i]->moveResize(next_x, m_bevel,
                                   button_size, button_size);
  }

  m_titlebar.raise(); // always on top
} // reconfigureTitlebar

void SbWinFrame::renderAll() {
  m_need_render = false;

  renderTitlebar();
  renderHandles();
  renderTabs();
}

void SbWinFrame::applyAll() {
  applyTitlebar();
  applyHandles();
  applyTabs();
}

void SbWinFrame::renderTitlebar() {
  if (!m_use_titlebar)
    return;

  if (!isVisible() ) {
    m_need_render = true;
    return;
  }

  typedef tk::ThemeProxy<SbWinFrameTheme> TP;
  TP& ft = theme().focusedTheme();
  TP& uft = theme().unfocusedTheme();

  // render pixmaps
  render(m_title_face.color[FOCUS], m_title_face.pm[FOCUS], m_titlebar.width(), m_titlebar.height(),
         ft->titleTexture(), m_imagectrl);

  render(m_title_face.color[UNFOCUS], m_title_face.pm[UNFOCUS], m_titlebar.width(), m_titlebar.height(),
         uft->titleTexture(), m_imagectrl);

  if (m_tabmode == INTERNAL)
    return; // skip label if tabs are in titlebar

  render(m_label_face.color[FOCUS], m_label_face.pm[FOCUS], m_label.width(), m_label.height(),
         ft->iconbarTheme()->texture(), m_imagectrl);

  render(m_label_face.color[UNFOCUS], m_label_face.pm[UNFOCUS], m_label.width(), m_label.height(),
         uft->iconbarTheme()->texture(), m_imagectrl);
}

void SbWinFrame::renderTabs() {
  if (!isVisible() ) {
    m_need_render = true;
    return;
  }

  typedef tk::ThemeProxy<SbWinFrameTheme> TP;
  TP& ft = theme().focusedTheme();
  TP& uft = theme().unfocusedTheme();
  tk::ButtonTrain& tabs = tabcontainer();
  const tk::Texture *tc_focused = &ft->iconbarTheme()->texture();
  const tk::Texture *tc_unfocused = &uft->iconbarTheme()->texture();

  if (m_tabmode == EXTERNAL && tc_focused->type() & tk::Texture::PARENTRELATIVE)
    tc_focused = &ft->titleTexture();
  if (m_tabmode == EXTERNAL && tc_unfocused->type() & tk::Texture::PARENTRELATIVE)
    tc_unfocused = &uft->titleTexture();

  render(m_tabcontainer_face.color[FOCUS], m_tabcontainer_face.pm[FOCUS],
         tabs.width(), tabs.height(), *tc_focused, m_imagectrl, tabs.orientation() );

  render(m_tabcontainer_face.color[UNFOCUS], m_tabcontainer_face.pm[UNFOCUS],
         tabs.width(), tabs.height(), *tc_unfocused, m_imagectrl, tabs.orientation() );

  renderButtons();
}

void SbWinFrame::applyTitlebar() {
  if (!m_use_titlebar)
    return;

  int f = m_state.focused;

  if (m_tabmode != INTERNAL) {
    m_label.setGC(theme()->iconbarTheme()->text().textGC() );
    m_label.setJustify(theme()->iconbarTheme()->text().justify() );

    bg_pm_or_color(m_label, m_label_face.pm[f], m_label_face.color[f]);
  }

  bg_pm_or_color(m_titlebar, m_title_face.pm[f], m_title_face.color[f]);
  applyButtons();
}


void SbWinFrame::renderHandles() {
  if (!m_use_handle)
    return;

  if (!isVisible() ) {
    m_need_render = true;
    return;
  }

  typedef tk::ThemeProxy<SbWinFrameTheme> TP;
  TP& ft = theme().focusedTheme();
  TP& uft = theme().unfocusedTheme();

  render(m_handle_face.color[FOCUS], m_handle_face.pm[FOCUS],
         m_handle.width(), m_handle.height(),
         ft->handleTexture(), m_imagectrl);

  render(m_handle_face.color[UNFOCUS], m_handle_face.pm[UNFOCUS],
         m_handle.width(), m_handle.height(),
         uft->handleTexture(), m_imagectrl);

  render(m_grip_face.color[FOCUS], m_grip_face.pm[FOCUS],
         m_grip_left.width(), m_grip_left.height(),
         ft->gripTexture(), m_imagectrl);

  render(m_grip_face.color[UNFOCUS], m_grip_face.pm[UNFOCUS],
         m_grip_left.width(), m_grip_left.height(),
         uft->gripTexture(), m_imagectrl);
}

void SbWinFrame::applyHandles() {
  bool f = m_state.focused;

  bg_pm_or_color(m_handle, m_handle_face.pm[f], m_handle_face.color[f]);
  bg_pm_or_color(m_grip_left, m_grip_face.pm[f], m_grip_face.color[f]);
  bg_pm_or_color(m_grip_right, m_grip_face.pm[f], m_grip_face.color[f]);
}

void SbWinFrame::renderButtons() {
  if (!isVisible() ) {
    m_need_render = true;
    return;
  }

  typedef tk::ThemeProxy<SbWinFrameTheme> TP;
  TP& ft = theme().focusedTheme();
  TP& uft = theme().unfocusedTheme();

  render(m_button_face.color[UNFOCUS], m_button_face.pm[UNFOCUS],
         m_button_size, m_button_size,
         uft->buttonTexture(), m_imagectrl);

  render(m_button_face.color[FOCUS], m_button_face.pm[FOCUS],
         m_button_size, m_button_size,
         ft->buttonTexture(), m_imagectrl);

  render(m_button_face.color[PRESSED], m_button_face.pm[PRESSED],
         m_button_size, m_button_size,
         theme()->buttonPressedTexture(), m_imagectrl);
}

void SbWinFrame::applyButtons() {
  // setup left and right buttons
  for (size_t i=0; i < m_buttons_left.size(); ++i)
    applyButton(*m_buttons_left[i]);

  for (size_t i=0; i < m_buttons_right.size(); ++i)
    applyButton(*m_buttons_right[i]);
}

// this init is separate because theme complains about not being initialized
void SbWinFrame::init() {
  if (theme()->handleWidth() == 0)
    m_use_handle = false;

  m_handle.showSubwindows();

  // clear pixmaps
  m_title_face.pm[UNFOCUS] = m_title_face.pm[FOCUS] = 0;
  m_label_face.pm[UNFOCUS] = m_label_face.pm[FOCUS] = 0;
  m_tabcontainer_face.pm[UNFOCUS] = m_tabcontainer_face.pm[FOCUS] = 0;
  m_handle_face.pm[UNFOCUS] = m_handle_face.pm[FOCUS] = 0;
  m_button_face.pm[UNFOCUS] = m_button_face.pm[FOCUS] = m_button_face.pm[PRESSED] = 0;
  m_grip_face.pm[UNFOCUS] = m_grip_face.pm[FOCUS] = 0;

  m_button_size = 26; // hardcoded button size

  m_label.setBorderWidth(0);

  setTabMode(NOTSET);

  m_label.setEventMask(ExposureMask | ButtonPressMask |
                       ButtonReleaseMask | ButtonMotionMask |
                       EnterWindowMask);

  showHandle();

  // Note: we don't show clientarea yet

  setEventHandler(*this);

  // setup cursors for resize grips
  gripLeft().setCursor(theme()->lowerLeftAngleCursor() );
  gripRight().setCursor(theme()->lowerRightAngleCursor() );

  setBorderWidth(true);
}

void SbWinFrame::applyButton(tk::Button &btn) {
  SbWinFrame::BtnFace& face = m_button_face;

  if (m_button_face.pm[PRESSED])
    btn.setPressedPixmap(face.pm[PRESSED]);
  else
    btn.setPressedColor(face.color[PRESSED]);

  bool f = m_state.focused;
  btn.setGC(theme()->buttonPicGC() );
  bg_pm_or_color(btn, face.pm[f], face.color[f]);
}

void SbWinFrame::applyTabs() {
  tk::ButtonTrain& tabs = tabcontainer();
  SbWinFrame::Face& face = m_tabcontainer_face;

  bg_pm_or_color(tabs, face.pm[m_state.focused], face.color[m_state.focused]);

  // and the labelbuttons in it
  for (auto btn_it : m_tab_container) {
    IconButton *btn = static_cast<IconButton *>(btn_it);
    btn->reconfigTheme();
  }
}

int SbWinFrame::getShape() const {
  int shape = theme()->shapePlace();
  if (!m_state.useTitlebar() )
    shape &= ~(tk::Shape::TOPRIGHT|tk::Shape::TOPLEFT);
  if (!m_state.useHandle() )
    shape &= ~(tk::Shape::BOTTOMRIGHT|tk::Shape::BOTTOMLEFT);
  return shape;
}

void SbWinFrame::applyDecorations(bool do_move) {
  int grav_x=0, grav_y=0;
  // negate gravity
  gravityTranslate(grav_x, grav_y, -sizeHints().win_gravity,
                   m_active_orig_client_bw);

  setBorderWidth(false);

  // tab deocration only affects if we're external
  // must do before the setTabMode in case it goes
  // to external and is meant to be hidden
  if (m_state.useTabs() )
    showTabs();
  else
    hideTabs();

  // NOTE: this uses setTabMode() which ACTUALLY updates tab 'decore'
  if (m_state.useTitlebar() ) {
    showTitlebar();
    if (m_screen.getDefaultInternalTabs() )
      setTabMode(INTERNAL);
    else
      setTabMode(EXTERNAL);
  } else {
    hideTitlebar();
    if (m_state.useTabs() )
      setTabMode(EXTERNAL);
  }

  if (m_state.useHandle() )
    showHandle();
  else
    hideHandle();

  // apply gravity once more
  gravityTranslate(grav_x, grav_y, sizeHints().win_gravity,
                            m_active_orig_client_bw);

  if (do_move) {
    // if the location changes, shift it
    if (grav_x != 0 || grav_y != 0)
      move(grav_x + x(), grav_y + y() );
    reconfigure();
    m_state.saveGeometry(x(), y(), width(), height() );
  }
} // applyDecorations

bool SbWinFrame::setBorderWidth(bool do_move) {
  unsigned int border_width = theme()->border().width();
  unsigned int win_bw = m_state.useBorder() ? border_width : 0;

  if (border_width
      && theme()->border().color().pixel() != window().borderColor() ) {
    tk::Color c = theme()->border().color();
    window().setBorderColor(c);
    titlebar().setBorderColor(c);
    handle().setBorderColor(c);
    gripLeft().setBorderColor(c);
    gripRight().setBorderColor(c);
    tabcontainer().setBorderColor(c);
  }

  if (border_width == handle().borderWidth()
      && win_bw == window().borderWidth() )
    return false;

  int grav_x=0, grav_y=0;
  // negate gravity
  if (do_move)
    gravityTranslate(grav_x, grav_y, -sizeHints().win_gravity,
                     m_active_orig_client_bw);

  int bw_changes = 0;
  // we need to change the size of the window
  // if the border width changes...
  if (m_use_titlebar)
    bw_changes += static_cast<signed>(border_width - titlebar().borderWidth() );
  if (m_use_handle)
    bw_changes += static_cast<signed>(border_width - handle().borderWidth() );

  window().setBorderWidth(win_bw);

  setTabMode(NOTSET);

  titlebar().setBorderWidth(border_width);
  handle().setBorderWidth(border_width);
  gripLeft().setBorderWidth(border_width);
  gripRight().setBorderWidth(border_width);

  if (bw_changes != 0)
    resize(width(), height() + bw_changes);

  if (m_tabmode == EXTERNAL)
    alignTabs();

  if (do_move) {
    gravityTranslate(grav_x, grav_y, sizeHints().win_gravity,
                              m_active_orig_client_bw);
    // if the location changes, shift it
    if (grav_x != 0 || grav_y != 0)
      move(grav_x + x(), grav_y + y() );
  }

  return true;
} // setBorderWidth

// this function translates its arguments according to win_gravity
// if win_gravity is negative, it does an inverse translation
// This function should be used when a window is mapped/unmapped/pos configured
void SbWinFrame::gravityTranslate(int &x, int &y, int win_gravity,
                          unsigned int client_bw) {
  bool invert = false;
  if (win_gravity < 0) {
    invert = true;
    win_gravity = -win_gravity; // make +ve
  }

  /* Ok, so, gravity says which point of the frame is put where the
   * corresponding bit of window would have been
   * Thus, x,y always refers to where top left of the WINDOW would be placed
   * but given that we're wrapping it in a frame, we actually place
   * it so that the given reference point is in the same spot as the
   * window's reference point would have been.
   * i.e. east gravity says that the centre of the right hand side of the
   * frame is placed where the centre of the rhs of the window would
   * have been if there was no frame.
   * Hope that makes enough sense.
   *
   * NOTE: the gravity calculations are INDEPENDENT of the client
   *       window width/height.
   *
   * If you get confused with the calculations, draw a picture.
   *
   */

  // We calculate offsets based on the gravity and frame aspects
  // and at the end apply those offsets +ve or -ve depending on 'invert'

  // These will be set to the resulting offsets for adjusting the frame position
  int x_offset = 0;
  int y_offset = 0;

  // These are the amount that the frame is larger than the client window
  // Note that the client window's x,y is offset by it's borderWidth, which
  // is removed by shynebox, so the gravity needs to account for this change

  // these functions already check if the title/handle is used
  int bw = static_cast<int>(m_window.borderWidth() );
  int bw_diff = static_cast<int>(client_bw) - bw;
  int height_diff = 2*bw_diff - static_cast<int>(titlebarHeight() ) - static_cast<int>(handleHeight() );
  int width_diff = 2*bw_diff;

  if (win_gravity == SouthWestGravity
      || win_gravity == SouthGravity || win_gravity == SouthEastGravity)
    y_offset = height_diff;

  if (win_gravity == WestGravity
      || win_gravity == CenterGravity || win_gravity == EastGravity)
    y_offset = height_diff/2;

  if (win_gravity == NorthEastGravity
      || win_gravity == EastGravity || win_gravity == SouthEastGravity)
    x_offset = width_diff;

  if (win_gravity == NorthGravity
      || win_gravity == CenterGravity || win_gravity == SouthGravity)
    x_offset = width_diff/2;

  if (win_gravity == StaticGravity) {
    x_offset = bw_diff;
    y_offset = bw_diff - titlebarHeight();
  }

  if (invert) {
    x_offset = -x_offset;
    y_offset = -y_offset;
  }

  x += x_offset;
  y += y_offset;
} // gravityTranslate

int SbWinFrame::widthOffset() const {
  if (m_tabmode != EXTERNAL || !m_use_tabs)
    return 0;
  if (s_place[(int)m_screen.getTabPlacement()].orient == tk::ROT0)
    return 0;
  return m_tab_container.width() + m_window.borderWidth();
}

int SbWinFrame::heightOffset() const {
  if (m_tabmode != EXTERNAL || !m_use_tabs)
    return 0;
  if (s_place[(int)m_screen.getTabPlacement()].orient != tk::ROT0)
    return 0;
  return m_tab_container.height() + m_window.borderWidth();
}

int SbWinFrame::xOffset() const {
  if (m_tabmode != EXTERNAL || !m_use_tabs)
    return 0;
  TabPlaceEnum p = m_screen.getTabPlacement();
  if (p == TabPlaceEnum::LEFTTOP || p == TabPlaceEnum::LEFT
      || p == TabPlaceEnum::LEFTBOTTOM)
    return m_tab_container.width() + m_window.borderWidth();
  return 0;
}

int SbWinFrame::yOffset() const {
  if (m_tabmode != EXTERNAL || !m_use_tabs)
    return 0;
  TabPlaceEnum p = m_screen.getTabPlacement();
  if (p == TabPlaceEnum::TOPLEFT || p == TabPlaceEnum::TOP
      || p == TabPlaceEnum::TOPRIGHT)
    return m_tab_container.height() + m_window.borderWidth();
  return 0;
}

void SbWinFrame::applySizeHints(unsigned int &width, unsigned int &height,
                                bool maximizing) const {
  const int h = height - titlebarHeight() - handleHeight();
  height = max(h, static_cast<int>(titlebarHeight() + handleHeight() ) );
  sizeHints().apply(width, height, maximizing);
  height += titlebarHeight() + handleHeight();
}

void SbWinFrame::displaySize(unsigned int width, unsigned int height) const {
  unsigned int i, j;
  sizeHints().displaySize(i, j,
                          width, height - titlebarHeight() - handleHeight() );
  m_screen.showGeometry(i, j);
}

bool SbWinFrame::insideTitlebar(Window win) const {
  return gripLeft().window() != win
         && gripRight().window() != win
         && window().window() != win;
}

int SbWinFrame::getContext(Window win, int x, int y, int last_x, int last_y, bool doBorders) {
  int context = 0;
  if (gripLeft().window()  == win) return Keys::ON_LEFTGRIP;
  if (gripRight().window() == win) return Keys::ON_RIGHTGRIP;
  if (doBorders) {
    using RectangleUtil::insideBorder;
    int borderw = window().borderWidth();
    // if mouse is currently on the window border, ignore it
    // or if mouse was on border when it was last clicked
    if ((!insideBorder(window(), x, y, borderw)
         && (externalTabMode() || ! insideBorder(tabcontainer(), x, y, borderw) ) )
       || (!insideBorder(window(), last_x, last_y, borderw)
         && (externalTabMode() || ! insideBorder(tabcontainer(), last_x, last_y, borderw ) ) )
       )
      context = Keys::ON_WINDOWBORDER;
  } // if doBorders

  if (window().window()    == win)
    return context | Keys::ON_WINDOW;
  if (handle().window()    == win) {
    const unsigned int px = x - this->x() - window().borderWidth();
    if (px < gripLeft().x() + gripLeft().width() || px > (unsigned)gripRight().x() )
      return context; // one of the corners
    return Keys::ON_WINDOWBORDER | Keys::ON_WINDOW;
  }
  if (titlebar().window()  == win) {
    const unsigned int px = x - this->x() - window().borderWidth();
    if (px < (unsigned)label().x() || px > label().x() + label().width() )
      return context; // one of the buttons, asked from a grabbed event
    return context | Keys::ON_TITLEBAR;
  }
  if (label().window()     == win)
    return context | Keys::ON_TITLEBAR;
  // internal tabs are on title bar
  if (tabcontainer().window() == win)
    return context | Keys::ON_TAB | (externalTabMode()?0:Keys::ON_TITLEBAR);

  for (auto it : tabcontainer() )
    if (it->window() == win)
      return context | Keys::ON_TAB | (externalTabMode()?0:Keys::ON_TITLEBAR);

  return context;
} // getContext

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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
