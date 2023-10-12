// ButtonTool.cc for Shynebox Window Manager

#include "ButtonTool.hh"
#include "ButtonTheme.hh"
#include "ToolTheme.hh"
#include "tk/Button.hh"
#include "tk/ImageControl.hh"
#include "tk/TextButton.hh"
#include "tk/SbWindow.hh" // string

ButtonTool::ButtonTool(tk::Button *button,
                       ToolbarItem::Type type,
                       tk::ThemeProxy<ButtonTheme> &theme,
                       tk::ImageControl &img_ctrl) :
       ToolbarItem(type),
       m_cache_pm(0),
       m_cache_pressed_pm(0),
       m_image_ctrl(img_ctrl),
       m_window(button),
       m_theme(dynamic_cast<tk::ThemeProxy<ToolTheme> &>(theme) ) {
  if (m_window == 0)
    throw std::string("ButtonTool/GenericTool: Error! Tried to create a tool with window = 0");
} // ButtonTool class init

ButtonTool::~ButtonTool() {
  if (m_cache_pm)
    m_image_ctrl.removeImage(m_cache_pm);

  if (m_cache_pressed_pm)
    m_image_ctrl.removeImage(m_cache_pressed_pm);

  delete m_window;
} // ButtonTool class destroy

void ButtonTool::updateSizing() {
  tk::Button &btn = static_cast<tk::Button &>(window() );
  int bw = theme()->border().width();
  btn.setBorderWidth(bw);
  if (tk::TextButton *txtBtn = dynamic_cast<tk::TextButton*>(&btn) ) {
    bw += 2; // extra padding, seems somehow required...

    unsigned int new_width = theme()->font().textWidth(txtBtn->text() ) + 2*bw;
    unsigned int new_height = theme()->font().height() + 2*bw;

    //if (orientation() == tk::ROT0 || orientation() == tk::ROT180)
    if (orientation() <= tk::ROT180) // horz
      resize(new_width, new_height);
    else
      resize(new_height, new_width);
  }
}

void ButtonTool::renderTheme() {
  tk::Button &btn = static_cast<tk::Button &>(window() );

  btn.setGC(static_cast<const ButtonTheme &>(*theme() ).gc() );
  btn.setBorderColor(theme()->border().color() );
  btn.setBorderWidth(theme()->border().width() );

  Pixmap old_pm = m_cache_pm;
  if (!theme()->texture().usePixmap() ) {
    m_cache_pm = 0;
    btn.setBackgroundColor(theme()->texture().color() );
  } else {
    m_cache_pm = m_image_ctrl.renderImage(width(), height(),
                                          theme()->texture(), orientation() );
    btn.setBackgroundPixmap(m_cache_pm);
  }
  if (old_pm)
    m_image_ctrl.removeImage(old_pm);

  old_pm = m_cache_pressed_pm;
  if (! static_cast<const ButtonTheme &>(*theme() ).pressed().usePixmap() ) {
    m_cache_pressed_pm = 0;
    btn.setPressedColor(static_cast<const ButtonTheme &>(*theme() ).pressed().color() );
  } else {
    m_cache_pressed_pm = m_image_ctrl.renderImage(width(), height(),
                                                  static_cast<const ButtonTheme &>(*theme() ).pressed(),
                                                  orientation() );
    btn.setPressedPixmap(m_cache_pressed_pm);
  }

  if (old_pm)
    m_image_ctrl.removeImage(old_pm);

  btn.clear();
} // renderTheme

void ButtonTool::setOrientation(tk::Orientation orient) {
  tk::Button &btn = static_cast<tk::Button &>(window() );
  btn.setOrientation(orient);
  ToolbarItem::setOrientation(orient);
}

//////// old generictool

void ButtonTool::move(int x, int y) {
  m_window->move(x, y);
}

void ButtonTool::resize(unsigned int width, unsigned int height) {
  m_window->resize(width, height);
}

void ButtonTool::moveResize(int x, int y,
                            unsigned int width, unsigned int height) {
  m_window->moveResize(x, y, width, height);
}

void ButtonTool::show() {
  m_window->show();
}

void ButtonTool::hide() {
  m_window->hide();
}

unsigned int ButtonTool::width() const {
  return m_window->width();
}

unsigned int ButtonTool::height() const {
  return m_window->height();
}

unsigned int ButtonTool::borderWidth() const {
  return m_window->borderWidth();
}

void ButtonTool::themeReconfigured() {
  m_window->clear();
}

void ButtonTool::parentMoved() {
  m_window->parentMoved();
}

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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
