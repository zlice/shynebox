// ClientPattern.cc for Shynebox Window Manager

#include "ClientPattern.hh"

#include "FocusControl.hh"
#include "Screen.hh"
#include "WinClient.hh"
#include "Workspace.hh"

#include "tk/App.hh"
#include "tk/StringUtil.hh"

#include <fstream>
#include <regex>
#include <cstdio>
#include <cstring>

// needed as well for index on some systems (e.g. solaris)
#include <strings.h>

using std::string;

#define WP tk::WinProperty_e
#define SUTIL tk::StringUtil

/**
 * This is the type of the actual pattern we want to match against
 * We have a "term" in the whole expression which is the full pattern
 * we also need to keep track of the uncompiled regular expression
 * for final output
 */
struct ClientPattern::Term {
  Term(const tk::SbString& _regstr, WP _prop, bool _negate, const tk::SbString& _xprop) :
        regstr(_regstr),
        xpropstr(_xprop),
        regexp(_regstr, true),
        prop(_prop),
        negate(_negate) {
    xprop = XInternAtom(tk::App::instance()->display(), xpropstr.c_str(), False);
  }
  // (title=.*bar) or (@FOO=.*bar)
  tk::SbString regstr;     // .*bar
  tk::SbString xpropstr;   // @FOO=.*bar
  Atom xprop;              // Atom of 'FOO'
  tk::RegExp regexp;       // compiled version of '.*bar'
  WP prop;                 // WinProperty enum moved to tk/Config.hh strnum
  bool negate;
};

ClientPattern::ClientPattern():
    m_matchlimit(0),
    m_nummatches(0) { }

// parse the given pattern (to end of line)
ClientPattern::ClientPattern(const char *str):
    m_matchlimit(0),
    m_nummatches(0) {
  /* A rough grammar of a pattern is:
     PATTERN ::= MATCH+ LIMIT?
     MATCH ::= '(' word ')'
               | '(' propertyname '=' word ')'
     LIMIT ::= '{' number '}'

     i.e. one or more match definitions, followed by
          an optional limit on the number of apps to match to

     Match definitions are enclosed in parentheses, and if no
     property name is given, then CLASSNAME is assumed.
     If no limit is specified, no limit is applied (i.e. limit = infinity)
  */

  bool had_error = false;
  int pos = 0;
  string match;
  int err = 1; // for starting first loop
  while (!had_error && err > 0) {
    err = SUTIL::getStringBetween(match,
                                  str + pos,
                                  '(', ')', " \t\n", true);

    if (err > 0) {
      WP prop = WP::NAME;
      std::string expr;
      std::string xprop;
      bool negate = false;

      // need to determine the property used, potential patterns:
      //
      //  A) foo               (short for 'title=foo')
      //  B) foo=bar
      //  C) foo!=bar
      //
      //  D) @foo=bar          (xproperty 'foo' equal to 'bar')
      //

      string propstr = match;
      string::size_type eq = propstr.find_first_of('=');

      if (eq == propstr.npos) // A, defaults to title
        expr = "[current]";
      else {                // B or C, so strip away the '='
        // 'bar'
        expr.assign(propstr.begin() + eq + 1, propstr.end() );

        // 'foo' or 'foo!'
        propstr.resize(eq);
        if (propstr.rfind("!", propstr.npos, 1) != propstr.npos) { // C 'foo!'
          negate = true;
          propstr.resize(propstr.size()-1);
        }
      }

      if (propstr[0] != '@') { // not D
        int p = tk::WinProperty_strnum.sz - 1;
        propstr = SUTIL::toUpper(propstr);
        for (; p > 0 ; p--)
          if (tk::WinProperty_strnum.estr[p] == propstr)
            break;

        if (p)
          prop = (WP)p;
        else
          expr = match;
      } else { // D
        prop = WP::XPROP;
        xprop.assign(propstr, 1, propstr.size() );
      }

      had_error = !addTerm(expr, prop, negate, xprop);
      pos += err;
    } // if err > 0
  } // while ! errors parsing strings

  if (pos == 0 && !had_error) // no match terms given, this is not allowed
    had_error = true;

  if (!had_error) { // otherwise, we check for a number
    string number;
    err = SUTIL::getStringBetween(number, str+pos, '{', '}');
    if (err > 0) {
      SUTIL::extractNumber(number, m_matchlimit);
      pos+=err;
    }
    // we don't care if there isn't one

    // there shouldn't be anything else on the line
    match = str + pos;
    size_t uerr; // need a special type here
    uerr = match.find_first_not_of(" \t\n", pos);
    if (uerr != match.npos)
      had_error = true; // found something, not good
  } // ! had_error

  if (had_error) {
    for (auto it : m_terms)
      delete it;
    m_terms.clear();
  }
} // ClientPattern class init (parse)

ClientPattern::~ClientPattern() {
  for (auto it : m_terms)
    delete it;
  m_terms.clear();
} // ClientPattern class destroy

// return a string representation of this pattern
string ClientPattern::toString() const {
  string result;
  for (auto &term : m_terms) {
    result.append(" (");
    result.append(tk::WinProperty_strnum.estr[(int)term->prop]);
    if (term->prop == WP::XPROP)
      result.append(term->xpropstr);
    result.append(term->negate ? "!=" : "=");
    result.append(term->regstr);
    result.append(")");
  }

  if (m_matchlimit > 0) {
    result.append(" {");
    result.append(SUTIL::number2String(m_matchlimit) );
    result.append("}");
  }
  return result;
}

// for readability, because the boolean is already weird enough with XORs
#define _TERM_XPROP ((term->regexp.match(win.getTextProperty(term->xprop) ) ) \
                   || term->regexp.match(SUTIL::number2String(win.getCardinalProperty(term->xprop) ) ) )
#define _WIN_ON_CUR_WS  (getProperty(term->prop, win) \
                         == SUTIL::number2String(win.screen().currentWorkspaceID() ) )
#define _WS_NAME_MATCH  (getProperty(term->prop, win) == w->name() )
#define _WIN_IS_FOCUSED (getProperty(term->prop, win) == getProperty(term->prop, *focused) )
#define _TERM_PROP      (term->regexp.match(getProperty(term->prop, win) ) )

// NOTE: this works but brings increased exec size
//       and is lower performance without regex lib
//if ((!term->negate ^ (std::regex_match(win.getTextProperty(term->xprop), (std::regex)term->regstr) ) ||

bool ClientPattern::match(const Focusable &win) const {
  if (m_matchlimit != 0 && m_nummatches >= m_matchlimit)
    return false; // already matched out
    // i highly suspect this doesn't work but left it in
    // just in case it does and i'm using it wrong.
    // not documented anyway. -zlice

  // regmatch everything
  // currently, we use an "AND" policy for multiple terms
  // changing to OR would require minor modifications in this function only
  for (auto &term : m_terms) {
    if (term->prop == WP::XPROP) {
      if (!term->negate ^ _TERM_XPROP)
        return false;
    } else if (term->regstr == "[current]") {
      WinClient *focused = FocusControl::focusedWindow();
      if (term->prop == WP::WORKSPACE) {
        if (!term->negate ^ _WIN_ON_CUR_WS)
          return false;
      } else if (term->prop == WP::WORKSPACENAME) {
        const Workspace *w = win.screen().currentWorkspace(); // !w shouldn't be possible
        if (!w || (!term->negate ^ _WS_NAME_MATCH) )
          return false;
      } else if (!focused || (!term->negate ^ _WIN_IS_FOCUSED) )
          return false;
    // [current]
    } else if (term->prop == WP::HEAD && term->regstr == "[mouse]") {
      if (!term->negate ^
          (getProperty(term->prop, win) == SUTIL::number2String(win.screen().getCurHead() ) ) )
        return false;
    } else if (!term->negate ^ _TERM_PROP) // e.g. pattern [title=".*urxvt.*"]
      return false;
  } // for m_terms
  return true;
} // match
#undef TERM_XPROP
#undef WIN_ON_CUR_WS
#undef WS_NAME_MATCH
#undef WIN_IS_FOCUSED
#undef TERM_PROP

bool ClientPattern::dependsOnFocusedWindow() const {
  for (auto it : m_terms)
    if (it->prop != WP::WORKSPACE && it->prop != WP::WORKSPACENAME
        && it->regstr == "[current]")
      return true;
  return false;
}

bool ClientPattern::dependsOnCurrentWorkspace() const {
  for (auto it : m_terms)
    if ((it->prop == WP::WORKSPACE || it->prop == WP::WORKSPACENAME)
        && it->regstr == "[current]")
      return true;
  return false;
}

// add an expression to match against
// The first argument is a regular expression, the second is the member
// function that we wish to match against.
bool ClientPattern::addTerm(const tk::SbString &str, WP prop, bool negate, const tk::SbString& xprop) {
  bool rc = false;
  Term* term = new Term(str, prop, negate, xprop);

  if (!term)
    return rc;

  if ((rc = !term->regexp.error() ) )
    m_terms.push_back(term);
  else
    delete term;

  return rc;
}

tk::SbString ClientPattern::getProperty(WP prop, const Focusable &client) {
  tk::SbString result;

  // we need this for some of the window properties
  const ShyneboxWindow *sbwin = client.sbwindow();

  switch (prop) {
  case WP::TITLE:
    result = client.title().logical();
    break;
  case WP::CLASS:
    result = client.getWMClassClass();
    break;
  case WP::ROLE:
    result = client.getWMRole();
    break;
  case WP::TRANSIENT:
    result = client.isTransient() ? "yes" : "no";
    break;
  case WP::MAXIMIZED:
    result = sbwin && sbwin->isMaximized() ? "yes" : "no";
    break;
  case WP::MINIMIZED:
    result = sbwin && sbwin->isIconic() ? "yes" : "no";
    break;
  case WP::FULLSCREEN:
    result = sbwin && sbwin->isFullscreen() ? "yes" : "no";
    break;
  case WP::VERTMAX:
    result = sbwin && sbwin->isMaximizedVert() ? "yes" : "no";
    break;
  case WP::HORZMAX:
    result = sbwin && sbwin->isMaximizedHorz() ? "yes" : "no";
    break;
  case WP::SHADED:
    result = sbwin && sbwin->isShaded() ? "yes" : "no";
    break;
  case WP::STUCK:
    result = sbwin && sbwin->isStuck() ? "yes" : "no";
    break;
  case WP::FOCUSHIDDEN:
    result = sbwin && sbwin->isFocusHidden() ? "yes" : "no";
    break;
  case WP::ICONHIDDEN:
    result = sbwin && sbwin->isIconHidden() ? "yes" : "no";
    break;
  case WP::VIEWABLE:
    result = sbwin && sbwin->isInView() ? "yes" : "no";
    break;
  case WP::WORKSPACE: {
    unsigned int wsnum = (sbwin ? sbwin->workspaceNumber()
                       : client.screen().currentWorkspaceID() );
    result = SUTIL::number2String(wsnum);
    break;
    }
  case WP::WORKSPACENAME: {
    const Workspace *w = (sbwin ?
            client.screen().getWorkspace(sbwin->workspaceNumber() ) :
            client.screen().currentWorkspace() );
    if (w)
      result = w->name();
    break;
    }
  case WP::HEAD:
    if (sbwin)
      result = SUTIL::number2String(client.screen().getHead(sbwin->sbWindow() ) );
    break;
  case WP::LAYER:
    if (sbwin)
      result = tk::ResLayers_strnum.estr[sbwin->layerNum() ];
    break;
  case WP::SCREEN:
    result = SUTIL::number2String(client.screen().screenNumber() );
    break;
  case WP::XPROP:
    break;
  case WP::NAME:
  default:
    result = client.getWMClassName();
    break;
  } // switch (prop)
  return result;
} // getProperty

bool ClientPattern::operator ==(const ClientPattern &pat) const {
  if (std::size(m_terms) != std::size(pat.m_terms) )
    return false;

  // we require the terms to be identical (order too)
  auto it = m_terms.begin();
  auto it_end = m_terms.end();
  auto other_it = pat.m_terms.begin();
  auto other_it_end = pat.m_terms.end();
  for (; it != it_end && other_it != other_it_end; ++it, ++other_it) {
    const Term& i = *(*it);
    const Term& o = *(*other_it);
    if (i.regstr != o.regstr
        || i.negate != o.negate
        || i.xpropstr != o.xpropstr)
      return false;
  }
  return true;
}

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
