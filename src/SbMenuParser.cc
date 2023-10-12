// SbMenuParser.cc for Shynebox Window Manager

#include "SbMenuParser.hh"

#include "tk/StringUtil.hh"

const SbMenuParser::Item SbMenuParser::s_empty_item = {"", ""};

bool SbMenuParser::open(const std::string &filename) {
  m_file.open(filename.c_str() );
  m_curr_pos = 0;
  m_row = 0;
  m_curr_token = DONE;
  return isLoaded();
}

SbMenuParser &SbMenuParser::operator >> (SbMenuParser::Item &out) {
  if (eof() ) {
    out = SbMenuParser::s_empty_item;
    return *this;
  }

  if (m_curr_line.empty() )
    m_curr_token = DONE; // try next line

  char first = '[';
  char second = ']';

  switch (m_curr_token) {
  case TYPE:
    first = '[';
    second = ']';
    break;
  case NAME:
    first = '(';
    second = ')';
    break;
  case ARGUMENT:
    first = '{';
    second = '}';
    break;
  case ICON:
    first = '<';
    second = '>';
    break;
  case DONE: // get new line and call this again
    if (!nextLine() ) {
      out = SbMenuParser::s_empty_item;
      return *this;
    }
    return (*this)>>out;
    break;
  }

  std::string key;
  int err = tk::StringUtil::
      getStringBetween(key, m_curr_line.c_str() + m_curr_pos,
                       first, second);
  if (err <= 0) {
    switch (m_curr_token) {
      case TYPE:
        m_curr_token = NAME;
        break;
      case NAME:
        m_curr_token = ARGUMENT;
        break;
      case ARGUMENT:
        m_curr_token = ICON;
        break;
      case ICON:
        m_curr_token = DONE;
        break;
      case DONE:
      default:
        break;
    }

    out = SbMenuParser::s_empty_item;
    return *this;
  }

  m_curr_pos += err; // update current position in current line

  // set value
  out.second = key;

  // set type and next token to be read
  switch (m_curr_token) {
  case TYPE:
      out.first = "TYPE";
      m_curr_token = NAME;
      break;
  case NAME:
      out.first = "NAME";
      m_curr_token = ARGUMENT;
      break;
  case ARGUMENT:
      out.first = "ARGUMENT";
      m_curr_token = ICON;
      break;
  case ICON:
      out.first = "ICON";
      m_curr_token = DONE;
      break;
  case DONE:
      break;
  }
  return *this;
} // SbMenuParser operator >>

SbMenuParser::Item SbMenuParser::nextItem() {
  SbMenuParser::Item item;
  (*this)>>item;
  return item;
}

bool SbMenuParser::nextLine() {
  if (!std::getline(m_file, m_curr_line) )
    return false;

  m_row++;
  m_curr_pos = 0;
  m_curr_token = TYPE;

  return true;
}

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2004 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
