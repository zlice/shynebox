// WorkspaceMenu.cc for Shynebox Window Manager

#include "WorkspaceMenu.hh"

#include "Screen.hh"
#include "Workspace.hh"
#include "WorkspaceCmd.hh"
#include "MenuCreator.hh"
#include "shynebox.hh"

#include "tk/I18n.hh"
#include "tk/LayerManager.hh"
#include "tk/MacroCommand.hh"
#include "tk/MenuItem.hh"
#include "tk/MenuSeparator.hh"
#include "tk/SimpleCommand.hh"

#include "Debug.hh"

namespace {

// the menu consists of (* means static)
//   - icons               * 0
//   --------------------- * 1
//   - workspaces            2
//   --------------------- * 3
//   - new workspace       * 4
//   - edit workspace name * 5
//   - remove last         * 6
//

const unsigned int IDX_AFTER_ICONS = 2;
const unsigned int NR_STATIC_ITEMS = 6;

void add_workspaces(WorkspaceMenu& menu, BScreen& screen) {
  for (size_t i = 0; i < screen.numberOfWorkspaces(); ++i) {
    Workspace* w = screen.getWorkspace(i);
    //w->menu().setInternalMenu(); // already done by ws

    menu.insertSubmenu(w->name(), &w->menu(), i + IDX_AFTER_ICONS);
  }
}

} // end of anonymous namespace

WorkspaceMenu::WorkspaceMenu(BScreen &screen):
       SbMenu(screen.menuTheme(),
              screen.imageControl(),
              *screen.layerManager().getLayer((int)tk::ResLayers_e::MENU) ) {
  using namespace tk;
  _SB_USES_NLS;

  removeAll();

  setLabel(_SB_XTEXT(Workspace, MenuTitle, "Workspaces", "Title of main workspace menu") );
  icon_menu = new ClientMenu(screen, screen.iconList() );
  insertSubmenu(_SB_XTEXT(Menu, Icons, "Icons", "Iconic windows menu title"), icon_menu);
  insertItem(new tk::MenuSeparator() );

  ::add_workspaces(*this, screen);
  setItemSelected(screen.currentWorkspaceID() + IDX_AFTER_ICONS, true);
  #define WS_CMD(func) *(tk::Command<void>*) new SimpleCommand<BScreen>(screen, (SimpleCommand<BScreen>::Action) func )

  Command<void> *shared_saverc = Shynebox::instance()->getSharedSaveRC();
  MacroCommand *new_workspace_macro = new MacroCommand();
  new_workspace_macro->add(WS_CMD(&BScreen::addWorkspace) );
  new_workspace_macro->add(*shared_saverc);

  MacroCommand *remove_workspace_macro = new MacroCommand();
  remove_workspace_macro->add(WS_CMD(&BScreen::removeLastWorkspace) );
  remove_workspace_macro->add(*shared_saverc);

  Command<void> *start_edit = new WorkspaceNameDialogCmd();

  insertItem(new tk::MenuSeparator() );
  insertCommand(_SB_XTEXT(Workspace, NewWorkspace, "New Workspace",
        "Add a new workspace"), *new_workspace_macro);
  insertCommand(_SB_XTEXT(Toolbar, EditWkspcName, "Edit current workspace name",
        "Edit current workspace name"), *start_edit);
  insertCommand(_SB_XTEXT(Workspace, RemoveLast, "Remove Last",
        "Remove the last workspace"), *remove_workspace_macro);
  #undef WS_CMD

  updateMenu();
} // WorkspaceMenu init

void WorkspaceMenu::workspaceInfoChanged(BScreen& screen) {
  while (numberOfItems() > NR_STATIC_ITEMS)
    remove(IDX_AFTER_ICONS);

  ::add_workspaces(*this, screen);
  updateMenu();
}

void WorkspaceMenu::workspaceChanged(BScreen& screen) {
  tk::MenuItem *item = 0;
  for (unsigned int i = 0; i < screen.numberOfWorkspaces(); ++i) {
    item = find(i + IDX_AFTER_ICONS);
    if (item && item->isSelected() ) {
      setItemSelected(i + IDX_AFTER_ICONS, false);
      updateMenu();
      break;
    }
  }
  setItemSelected(screen.currentWorkspaceID() + IDX_AFTER_ICONS, true);
  updateMenu();
}

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2004 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
