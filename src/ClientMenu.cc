// ClientMenu.hh for Shynebox Window Manager

#include "ClientMenu.hh"

#include "Screen.hh"
#ifndef USE_TOOLBAR
#include "WinClient.hh"
#endif
#include "Window.hh"
#include "FocusControl.hh"

#include "tk/MenuItem.hh"
#include "tk/LayerManager.hh"

#include <X11/keysym.h>

namespace {

class ClientMenuItem: public tk::MenuItem {
public:
  ClientMenuItem(Focusable &client, ClientMenu &menu):
      tk::MenuItem(client.title(), menu),
      m_client(client) { }

  void click(int button, int time, unsigned int mods) {
    (void) button;
    (void) time;
    ShyneboxWindow *sbwin = m_client.sbwindow();
    if (sbwin == 0)
      return;

    // this MenuItem object can get destroyed as a result of focus(), so we
    // must get a local copy of anything we want to use here
    // AFTER ~ClientMenuItem() is called.
    tk::Menu *parent = menu();
    FocusControl& focus_control = m_client.screen().focusControl();

    if (WinClient *winc = dynamic_cast<WinClient*>(&m_client) )
      sbwin->setCurrentClient(*winc, false);

    m_client.focus();
    sbwin->raise();
    if ((mods & ControlMask) == 0) {
      // Ignore any focus changes due to this menu closing
      // (even in StrictMouseFocus mode)
      focus_control.ignoreAtPointer(true);
      parent->hide();
    }
  }

  const tk::BiDiString &label() const { return m_client.title(); }
  const tk::PixmapWithMask *icon() const {
    return m_client.screen().clientMenuUsePixmap() ? &m_client.icon() : 0;
  }

  bool isSelected() const {
    if (m_client.sbwindow() == 0)
      return false;
    if (m_client.sbwindow()->isFocused() == false)
      return false;
    return (&(m_client.sbwindow()->winClient() ) == &m_client);
  }

  // for updating menu when receiving a signal from client
  Focusable *client() { return &m_client; }

private:
  Focusable &m_client;
};

} // end anonymous namespace

ClientMenu::ClientMenu(BScreen &screen, Focusables &clients):
    SbMenu(screen.menuTheme(), screen.imageControl(),
           *screen.layerManager().getLayer((int)tk::ResLayers_e::MENU) ),
            m_list(clients) {
  updateMenu();
}

// previously there were more efficient calls to
// handle clients dying or titles changing
// now clientListChanged goes back to screen
// and simply calls this
void ClientMenu::updateMenu() {
  // remove all items and then add them again
  removeAll();

  for (auto &win_it : m_list) {
    // add every client per ShyneboxWindow to menu
    if (typeid(win_it) == typeid(ShyneboxWindow *) ) {
      ShyneboxWindow *win = win_it;
      for (auto &client_it : win->clientList() )
        insertItem(new ClientMenuItem(*client_it, *this) );
    } else
      insertItem(new ClientMenuItem(*win_it, *this) );
  }
  SbMenu::updateMenu();
}

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2007-2008 Fluxbox Team (fluxgen at fluxbox dot org)
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
