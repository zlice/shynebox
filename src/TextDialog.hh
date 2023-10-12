// TextDialog.hh for Shynebox Window Manager

/*
  Prompt for user input.
  Used to set current window title and workspace names.
*/

#ifndef TEXTDIALOG_HH
#define TEXTDIALOG_HH

#include "tk/TextBox.hh"
#include "tk/TextButton.hh"
#include "tk/GContext.hh"

class BScreen;

class TextDialog: public tk::SbWindow, public tk::EventHandler {
public:
  TextDialog(BScreen &screen, const std::string &title);
  virtual ~TextDialog();

  // Sets the default entry text.
  void setText(const tk::BiDiString& text);

  void show();
  void hide();

  void exposeEvent(XExposeEvent &event);
  void motionNotifyEvent(XMotionEvent &event);
  void buttonPressEvent(XButtonEvent &event);
  void handleEvent(XEvent &event);
  void keyPressEvent(XKeyEvent &event);

protected:
  virtual void exec(const std::string &text) = 0;
  virtual void tabComplete() { }

  void render();
  void updateSizes();

  tk::TextBox m_textbox;  // entry field
  tk::TextButton m_label; // text in the titlebar
  tk::GContext m_gc;
  BScreen &m_screen;
  int m_move_x, m_move_y;
  Pixmap m_pixmap;
};

#endif // TEXTDIALOG_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2008 Fluxbox Team (fluxgen at fluxbox dot org)
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
