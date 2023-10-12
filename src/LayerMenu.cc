// LayerMenu.cc for Shynebox Window Manager

#include "LayerMenu.hh"

#include "shynebox.hh"
#include "SbCommands.hh"
#include "tk/I18n.hh"

#define RLEnum (int)tk::ResLayers_e

LayerMenu::LayerMenu(tk::ThemeProxy<tk::MenuTheme> &tm,
                     tk::ImageControl &imgctrl,
                     tk::Layer &layer, LayerObject *object, bool save_rc):
      SbMenu(tm, imgctrl, layer) {
  _SB_USES_NLS;

  struct {
      int set;
      int base;
      tk::SbString default_str;
      int layernum;
  } layer_menuitems[]  = {
      //TODO: nls
      {0, 0, _SB_XTEXT(Layer, AboveDock, "Above Dock", "Layer above dock"), RLEnum::ABOVE_DOCK},
      {0, 0, _SB_XTEXT(Layer, Dock, "Dock", "Layer dock"), RLEnum::DOCK},
      {0, 0, _SB_XTEXT(Layer, Top, "Top", "Layer top"), RLEnum::TOP},
      {0, 0, _SB_XTEXT(Layer, Normal, "Normal", "Layer normal"), RLEnum::NORMAL},
      {0, 0, _SB_XTEXT(Layer, Bottom, "Bottom", "Layer bottom"), RLEnum::BOTTOM},
      {0, 0, _SB_XTEXT(Layer, Desktop, "Desktop", "Layer desktop"), RLEnum::DESKTOP},
  };

  for (size_t i=0; i < 6; ++i) {
    // TODO: fetch nls string
    if (save_rc) { // toolbar
      insertItem(new LayerMenuItem(layer_menuitems[i].default_str,
                               object, layer_menuitems[i].layernum,
                               *(tk::Command<void>*)(Shynebox::instance()->getSharedSaveRcfgMacro() ) ) );
    } else // any random window
      insertItem(new LayerMenuItem(layer_menuitems[i].default_str,
                               object, layer_menuitems[i].layernum) );
  }
  updateMenu();
}

// update which items appear disabled whenever we show the menu
void LayerMenu::show() {
  frameWindow().updateBackground();
  clearWindow();
  tk::Menu::show();
}

void LayerMenu::buttonReleaseEvent(XButtonEvent &ev) {
  // do redraw of other items
  SbMenu::buttonReleaseEvent(ev);

  // since this menu consist of toggle menu items
  // that relate to each other, we need to redraw
  // the items each time we get a button release
  // event so that the last toggled item gets
  // redrawn as not toggled.
  if (ev.window == frameWindow() )
    frameWindow().updateBackground();
    // force full foreground update

  clearWindow();
}

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2005 - 2006 Fluxbox Team (fluxgen at fluxbox dot org)
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
