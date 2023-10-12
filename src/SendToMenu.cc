// SendToMenu.cc for Shynebox Window Manager

#include "SendToMenu.hh"

#include "Window.hh"
#include "Screen.hh"
#include "Workspace.hh"
#include "shynebox.hh"

#include "tk/Command.hh"
#include "tk/LayerManager.hh"

class SendToCmd: public tk::Command<void> {
public:
  SendToCmd(unsigned int workspace, bool follow):
    m_workspace(workspace),
    m_follow(follow) { }
  void execute() {
    if (SbMenu::window() != 0)
      SbMenu::window()->screen().sendToWorkspace(m_workspace, SbMenu::window(), m_follow);
  }

private:
  const unsigned int m_workspace;
  const bool m_follow;
};

SendToMenu::SendToMenu(BScreen &screen):
    SbMenu(screen.menuTheme(),
           screen.imageControl(),
           *screen.layerManager().getLayer((int)tk::ResLayers_e::MENU) ) {
  // no title for this menu, it should be a submenu in the window menu.
  disableTitle();
  rebuildMenu();
} // SendToMenuu class init

SendToMenu::~SendToMenu() { } // SendToClass destroy

void SendToMenu::rebuildMenu() {
  removeAll();
  BScreen *screen = Shynebox::instance()->findScreen(screenNumber() );
  const BScreen::Workspaces &wlist = screen->getWorkspacesList();

  for (size_t i = 0; i < wlist.size(); ++i) {
    tk::Command<void> *sendto_cmd(new SendToCmd(i, false) );
    insertCommand(wlist[i]->name(), *sendto_cmd);
  }

  updateMenu();
}

void SendToMenu::show() {
  rebuildMenu();
  if (SbMenu::window() != 0) {
    for (unsigned int i=0; i < numberOfItems(); ++i)
      setItemEnabled(i, true);
    // update the workspace for the current window
    setItemEnabled(SbMenu::window()->workspaceNumber(), false);
    updateMenu();
  }
  tk::Menu::show();
}

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2003 - 2008 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
