// WorkspaceNameTool.hh for Shynebox Window Manager

/*
  Workspace Name tag button for the Toolbar.

  Right/Left clicks cycle workspaces.
*/

#ifndef WORKSPACENAMETOOL_HH
#define WORKSPACENAMETOOL_HH

#include "ToolbarItem.hh"

#include "tk/TextButton.hh"

class BScreen;
class ToolTheme;

namespace tk {
template <class T> class ThemeProxy;
}

class WorkspaceNameTool: public ToolbarItem {
public:
  WorkspaceNameTool(const tk::SbWindow &parent, tk::ThemeProxy<ToolTheme> &theme, BScreen &screen);
  virtual ~WorkspaceNameTool();

  void move(int x, int y);
  void resize(unsigned int width, unsigned int height);
  void moveResize(int x, int y,
                  unsigned int width, unsigned int height);

  void show();
  void hide();
  unsigned int width() const;
  unsigned int height() const;
  unsigned int borderWidth() const;

  tk::Button &button() { return m_button; }
  const tk::Button &button() const { return m_button; }
  void setOrientation(tk::Orientation orient);

  void parentMoved() { m_button.parentMoved(); }
  void update();

private:
  void renderTheme();
  void reRender();
  void updateSizing();
  tk::TextButton m_button;
  const tk::ThemeProxy<ToolTheme> &m_theme;
  BScreen &m_screen;
  Pixmap m_pixmap;
};

#endif // WORKSPACENAMETOOL_HH

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
