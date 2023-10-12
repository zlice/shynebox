// ConfigMenu.cc for Shynebox Window Manager

#include "ConfigMenu.hh"
#include "MenuCreator.hh"
#include "Screen.hh"
#include "shynebox.hh"
#include "SbCommands.hh"

#include "FocusModelMenuItem.hh"
#include "ScreenPlacement.hh"
#include "SbMenu.hh"
#include "tk/Menu.hh"
#include "tk/BoolMenuItem.hh"
#include "tk/IntMenuItem.hh"
#include "tk/MenuSeparator.hh"
#include "tk/RadioMenuItem.hh"

#include "tk/I18n.hh"

using namespace tk;

#define TabPlaceEnum TabPlacement_e
namespace {

class TabPlacementMenuItem: public RadioMenuItem {
public:
  TabPlacementMenuItem(const SbString & label, BScreen &screen,
                       TabPlaceEnum place, Menu &host_menu) :
        RadioMenuItem(label, host_menu),
        m_screen(screen),
        m_place(place) {
    setCloseOnClick(false);
  }

  bool isSelected() const { return m_screen.getTabPlacement() == m_place; }
  void click(int button, int time, unsigned int mods) {
    m_screen.saveTabPlacement(m_place);
    Shynebox::instance()->save_rc();
    Shynebox::instance()->reconfigure();
    menu()->frameWindow().updateBackground();
    menu()->clearWindow();
  }

private:
  BScreen &m_screen;
  TabPlaceEnum m_place;
};

} // anonymous namespace

// creates a config menu per screen
// static
void ConfigMenu::setup(Menu& menu, BScreen &screen) {
  _SB_USES_NLS;

  tk::CFGMAP &cfgmap = screen.m_cfgmap;
  Command<void> *shared_saverc = Shynebox::instance()->getSharedSaveRC();
  // shared commands
  // theyre all just calling funcs and it was a waste of space and overhead
  Command<void> *shared_sv_rcfg_macro = Shynebox::instance()->getSharedSaveRcfgMacro();

  /*          focus menu            */
  // we don't set this to internal menu so will
  // be deleted toghether with the parent
  SbString foc_model_label = _SB_XTEXT(Configmenu, FocusModel, "Focus Model",
                                     "Method used to give focus to windows");
  SbMenu *fm = MenuCreator::createMenu(foc_model_label, screen);

#define _FOCUSITEM(a, b, c, d, e, f) \
  fm->insertItem(new FocusModelMenuItem(_SB_XTEXT(a, b, c, d), screen.focusControl(), e, *f) )

  _FOCUSITEM(Configmenu, ClickFocus,
             "Click To Focus", "Click to focus",
             MainFocusModel_e::CLICKFOCUS, shared_sv_rcfg_macro);
  _FOCUSITEM(Configmenu, MouseFocus,
             "Mouse Focus (Keyboard Friendly)",
             "Mouse Focus (Keyboard Friendly)",
             MainFocusModel_e::MOUSEFOCUS, shared_sv_rcfg_macro);
  _FOCUSITEM(Configmenu, StrictMouseFocus,
             "Mouse Focus (Strict)",
             "Mouse Focus (Strict)",
             MainFocusModel_e::STRICTMOUSEFOCUS, shared_sv_rcfg_macro);
#undef _FOCUSITEM

  fm->insertItem(new MenuSeparator() );
  fm->insertItem(new TabFocusModelMenuItem(_SB_XTEXT(Configmenu,
      ClickTabFocus, "ClickTabFocus", "Click tab to focus windows"),
      screen.focusControl(), TabFocusModel_e::CLICKTABFOCUS, *shared_sv_rcfg_macro) );
  fm->insertItem(new TabFocusModelMenuItem(_SB_XTEXT(Configmenu,
      MouseTabFocus, "MouseTabFocus", "Hover over tab to focus windows"),
      screen.focusControl(), TabFocusModel_e::MOUSETABFOCUS, *shared_sv_rcfg_macro) );
  fm->insertItem(new MenuSeparator() );

  fm->insertItem(new BoolMenuItem(_SB_XTEXT(Configmenu, FocusNew,
      "Focus New Windows", "Focus newly created windows"),
      *cfgmap["focusNewWindows"], *shared_sv_rcfg_macro) );

  fm->insertItem(new tk::BoolMenuItem(_SB_XTEXT(Configmenu, FocusSameHead,
      "Keep Head", "Only revert focus on same head"),
      *cfgmap["focusSameHead"], *shared_sv_rcfg_macro) );

#define _BOOLITEM(m, a, b, c, d, e, f) (m).insertItem(new BoolMenuItem(_SB_XTEXT(a, b, c, d), e, f) )
  _BOOLITEM(*fm, Configmenu, AutoRaise,
            "Auto Raise Windows", "Auto Raise windows on sloppy",
            *cfgmap["autoRaiseWindows"], *shared_sv_rcfg_macro);
  _BOOLITEM(*fm, Configmenu, ClickRaises,
            "Click Raises Windows", "Click Raises",
            *cfgmap["clickRaisesWindows"], *shared_sv_rcfg_macro);

  menu.insertSubmenu(foc_model_label, fm);
  fm->updateMenu();
  /*          focus menu          */
  /*          max menu            */
  SbString max_label = _SB_XTEXT(Configmenu, MaxMenu,
          "Maximize Options", "heading for maximization options");
  Menu* mm = MenuCreator::createMenu(max_label, screen);

  _BOOLITEM(*mm, Configmenu, FullMax,
            "Full Maximization", "Maximise over slit, toolbar, etc",
            *cfgmap["fullMaximization"], *shared_saverc);
  _BOOLITEM(*mm, Configmenu, MaxIgnoreInc,
            "Ignore Resize Increment",
            "Maximizing Ignores Resize Increment (e.g. xterm)",
            *cfgmap["maxIgnoreIncrement"], *shared_saverc);
  _BOOLITEM(*mm, Configmenu, MaxDisableMove,
            "Disable Moving", "Don't Allow Moving While Maximized",
            *cfgmap["maxDisableMove"], *shared_saverc);
  _BOOLITEM(*mm, Configmenu, MaxDisableResize,
            "Disable Resizing", "Don't Allow Resizing While Maximized",
            *cfgmap["maxDisableResize"], *shared_saverc);

  menu.insertSubmenu(max_label, mm);
  mm->updateMenu();
  /*          max menu            */
  /*          tab menu            */
  SbString tab_opt_label = _SB_XTEXT(Configmenu, TabMenu, "Tab Options", "heading for tab-related options");
  SbString p_label = _SB_XTEXT(Menu, Placement, "Placement", "Title of Placement menu");
  Menu* tab_menu = MenuCreator::createMenu(tab_opt_label, screen);
  Menu* p_menu = MenuCreator::createMenu(p_label, screen);

  tab_menu->insertSubmenu(p_label, p_menu);

  tab_menu->insertItem(new BoolMenuItem(_SB_XTEXT(Configmenu, TabsInTitlebar,
            "Tabs in Titlebar", "Tabs in Titlebar"),
            *cfgmap["tabs.inTitlebar"], *shared_sv_rcfg_macro) );
  tab_menu->insertItem(new BoolMenuItem(_SB_XTEXT(Common, MaximizeOver,
            "Maximize Over", "Maximize over this thing when maximizing"),
            *cfgmap["tabs.maxOver"], *shared_sv_rcfg_macro) );
  tab_menu->insertItem(new BoolMenuItem(_SB_XTEXT(Toolbar, ShowIcons,
            "Show Pictures", "chooses if little icons are shown next to title in the iconbar"),
            *cfgmap["tabs.usePixmap"], *shared_sv_rcfg_macro) );

  MenuItem *tab_width_item =
      new IntMenuItem(_SB_XTEXT(Configmenu, ExternalTabWidth,
                                     "External Tab Width",
                                     "Width of external-style tabs"),
                             *cfgmap["tabs.width"], 10, 3000, /* silly number */
                             *tab_menu);
  tab_width_item->setCommand(*shared_sv_rcfg_macro);
  tab_menu->insertItem(tab_width_item);

  // menu is 3 wide, 5 down
  struct PlacementP {
       const SbString label;
       TabPlaceEnum placement;
  };
  static const PlacementP place_menu[] = {
      { _SB_XTEXT(Align, TopLeft, "Top Left", "Top Left"), TabPlaceEnum::TOPLEFT},
      { _SB_XTEXT(Align, LeftTop, "Left Top", "Left Top"), TabPlaceEnum::LEFTTOP},
      { _SB_XTEXT(Align, LeftCenter, "Left Center", "Left Center"), TabPlaceEnum::LEFT},
      { _SB_XTEXT(Align, LeftBottom, "Left Bottom", "Left Bottom"), TabPlaceEnum::LEFTBOTTOM},
      { _SB_XTEXT(Align, BottomLeft, "Bottom Left", "Bottom Left"), TabPlaceEnum::BOTTOMLEFT},
      { _SB_XTEXT(Align, TopCenter, "Top Center", "Top Center"), TabPlaceEnum::TOP},
      { "", TabPlaceEnum::TOPLEFT},
      { "", TabPlaceEnum::TOPLEFT},
      { "", TabPlaceEnum::TOPLEFT},
      { _SB_XTEXT(Align, BottomCenter, "Bottom Center", "Bottom Center"), TabPlaceEnum::BOTTOM},
      { _SB_XTEXT(Align, TopRight, "Top Right", "Top Right"), TabPlaceEnum::TOPRIGHT},
      { _SB_XTEXT(Align, RightTop, "Right Top", "Right Top"), TabPlaceEnum::RIGHTTOP},
      { _SB_XTEXT(Align, RightCenter, "Right Center", "Right Center"), TabPlaceEnum::RIGHT},
      { _SB_XTEXT(Align, RightBottom, "Right Bottom", "Right Bottom"), TabPlaceEnum::RIGHTBOTTOM},
      { _SB_XTEXT(Align, BottomRight, "Bottom Right", "Bottom Right"), TabPlaceEnum::BOTTOMRIGHT}
  };

  p_menu->setMinimumColumns(3);
  // create items in sub menu
  for (size_t i=0; i< sizeof(place_menu)/sizeof(PlacementP); ++i) {
    const PlacementP& p = place_menu[i];
    if (p.label == "") {
      p_menu->insert(p.label);
      p_menu->setItemEnabled(i, false);
    } else
      p_menu->insertItem(new TabPlacementMenuItem(p.label,
              screen, p.placement, *p_menu) ); // dynamic_cast<Menu&>(*p_menu) );
  } // for placements
  menu.insertSubmenu(tab_opt_label, tab_menu);
  p_menu->updateMenu();
  tab_menu->updateMenu();
  /*          tab menu            */

  _BOOLITEM(menu, Configmenu, OpaqueMove,
            "Opaque Window Moving",
            "Window Moving with whole window visible (as opposed to outline moving)",
            *cfgmap["opaqueMove"], *shared_saverc);
  _BOOLITEM(menu, Configmenu, OpaqueResize,
            "Opaque Window Resizing",
            "Window Resizing with whole window visible (as opposed to outline resizing)",
            *cfgmap["opaqueResize"], *shared_saverc);
  _BOOLITEM(menu, Configmenu, WorkspaceWarping,
            "Warp Horizontal",
            "Warp Horizontal - dragging windows left-right warps workspaces",
            *cfgmap["workspaceWarpingHorizontal"], *shared_saverc);
  _BOOLITEM(menu, Configmenu, WorkspaceWarping,
            "Warp Vertical",
            "Warp Vertical - dragging windows up-down warps workspaces",
            *cfgmap["workspaceWarpingVertical"], *shared_saverc);
  _BOOLITEM(menu, Configmenu, FocusSameHead, // XINERAMA leftover of NLS
            "Obey Multi-Head",
            "Obey Multi-Head - Use randr's dimensions of heads(monitors) for positioning",
            *cfgmap["obeyHeads"], *shared_sv_rcfg_macro);
#undef _BOOLITEM
  menu.updateMenu();
} // ConfigMenu::setup

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2015 - Mathias Gumz <akira@fluxbox.org>
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
