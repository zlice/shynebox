// Config.hh for Shynebox Window Manager

/*
  This class defines Config item types.
  strstr : struct to hold type/key/val for a simpler 'map' to hold defaults
  strnum : serves as a string to enum converter
           which this file holds all config related enums & values
           this makes some ugly casting in other areas
  cfgval : union to hold int/uint/strnum/str config values
  cfgitm : class to wrap access operators and hold cfgvals
  CFGMAP : actual map<string,cfgitm> that can hold a key/val pair
  ConfigManager : the interface that loads and saves the cfgmap
*/

#ifndef TK_CONFIG_HH
#define TK_CONFIG_HH

#include <string>
#include <map>

namespace tk {

// enum to represent strnum types
enum : char {
  T_COLPLACE = 'C',
  T_FOCMODEL = 'F',
  T_IBARALGN = 'A',
  T_MENUSRCH = 'M',
  T_ROWPLACE = 'R',
  T_TABFMODL = 'O',
  T_TABPLACE = 'B',
  T_TBRLAYER = 'T',
  T_TBRPLACE = 'P',
  T_WINPLACE = 'W'
};

// used for default config list
struct strstr {
// dont think this is needed
  char type = 's'; // s = string (default)
                   // b = bool
                   // e = strnum - see above enum:char table
                   // i = int
                   // u = uint
  const char *key, *val;
};

////////////////////////////////////////////////////
// strnum

#define CNT(...) INTERNAL_CNT(0, ## __VA_ARGS__, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define INTERNAL_CNT(_0, _1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, _17_,  _18_, _19_, _20_, _21_, _22_, count, ...) count


struct strnum {
  // const so iso c++11 is happy
  // define all values, because compiler won't
  // auto-fill/repeat padding for highest size
  const char *estr[22] = { "", "", "", "", "",
                           "", "", "", "", "",
                           "", "", "", "", "",
                           "", "",
                           "", "", "", "", ""};
  int sz = 0;  // varies by 'type'
  int val = 0; // default

  // string operator can't be reference
  operator int&()         {     return val;    }
  operator const std::string() { return this->estr[val]; }
  void operator= (int e)  {      val = e;      }
  void operator= (const char *c) {
    int i = sz - 1;
    const char *c_up, *e_cmp;

    // compare against all upper cased estr[]
    // (note: saving convert back to UPPER)
    // while not end of strings and chars match
    for (; i > 0 ; i--) {
      c_up=c;
      e_cmp=estr[i];
      while (*c_up && *e_cmp && toupper(*c_up) == *e_cmp)
        { c_up++; e_cmp++; }
      if (*c_up == 0 && *e_cmp == 0)
        break; // match
    } // for estr values

    val = i;
  } // assign by string
  void operator= (std::string s) { (*this) = s.c_str(); }
};

#define EXP_STR(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22, ...) #_1, #_2, #_3, #_4, #_5, #_6, #_7, #_8, #_9, #_10, #_11, #_12, #_13, #_14, #_15, #_16, #_17, #_18, #_19, #_20, #_21, #_22
// the enum class is used inside switch()es and the like
// config uses strictly strnum for str<>int conversions
// (compiler says these are 'unused', thus the attribute)
#define S_EXP(_name, cnt, ...) enum class _name##_e : int { __VA_ARGS__ }; \
      __attribute__((unused)) static strnum _name##_strnum { \
        .estr = { EXP_STR(__VA_ARGS__, ,,,,,,,,,,,,,,,,,,,,,,) }, \
        .sz = cnt \
      };

#define ESTR(name, ...) S_EXP(name, CNT(__VA_ARGS__), __VA_ARGS__);

// Tab and Toolbar Placement are the same 'enum'
// but strnum holds separate values for each
ESTR(TabPlacement, \
      TOPLEFT, TOP, TOPRIGHT, \
      BOTTOMLEFT, BOTTOM, BOTTOMRIGHT, \
      LEFTBOTTOM, LEFT, LEFTTOP, \
      RIGHTBOTTOM, RIGHT, RIGHTTOP);

ESTR(ToolbarPlacement, \
      TOPLEFT, TOPCENTER, TOPRIGHT, \
      BOTTOMLEFT, BOTTOMCENTER, BOTTOMRIGHT, \
      LEFTBOTTOM, LEFTCENTER, LEFTTOP, \
      RIGHTBOTTOM, RIGHTCENTER, RIGHTTOP);

ESTR(ScreenPlacementPolicy, \
      ROWSMARTPLACEMENT, COLSMARTPLACEMENT, \
      COLMINOVERLAPPLACEMENT, ROWMINOVERLAPPLACEMENT, \
      CASCADEPLACEMENT, UNDERMOUSEPLACEMENT, \
      CENTERPLACEMENT, AUTOTABPLACEMENT);

// previously there were more layers
// maybe for tk/Layer unimplemented funcs?
// and they were index 0, 2, 4 ...
// but only these seem to work fine
ESTR(ResLayers, \
      MENU,    ABOVE_DOCK, \
      DOCK,    TOP, \
      NORMAL,  BOTTOM, \
      DESKTOP, NUM_LAYERS);

ESTR(RowDirection, \
      LEFTRIGHT, RIGHTLEFT);

ESTR(ColDirection, \
      TOPBOTTOM, BOTTOMTOP);

ESTR(ButtonTrainAlignment, \
      LEFT, CENTER, RIGHT, RELATIVE, RELATIVE_SMART);

ESTR(MainFocusModel, \
      MOUSEFOCUS, CLICKFOCUS, STRICTMOUSEFOCUS);

ESTR(TabFocusModel, \
      MOUSETABFOCUS, CLICKTABFOCUS);

ESTR(MenuMode, \
      NOWHERE, ITEMSTART, SOMEWHERE);

// ! non-config item ! for ClientPattern (and Remember)
ESTR(WinProperty, \
    ERROR, TITLE, CLASS, NAME, ROLE, TRANSIENT, MAXIMIZED, \
    MINIMIZED, SHADED, STUCK, FOCUSHIDDEN, ICONHIDDEN, \
    WORKSPACE, WORKSPACENAME, HEAD, LAYER, SCREEN, \
    XPROP, FULLSCREEN, VERTMAX, HORZMAX, VIEWABLE);

// strnum
////////////////////////////////////////////////////
// cfgval

// see 'strstr' types above
union cfgval {
  bool b;
  strnum *e;
  int i;
  unsigned int u;
  std::string *s; // default
};

////////////////////////////////////////////////////
// CfgItm
// Real config item class

/*
 * unfortunately map is cursed
 * https://devblogs.microsoft.com/oldnewthing/20190227-00/?p=101072
 *
 * trying to assign values will call destructor (even with 'emplace' afaict)
 * this leads to delete and assign issues because of extra copy/moves
 * or in the case of pointers, 0 values that need to be checked
*/

/*
 * usage: cfgmap["something"] = new CfgItm('i')
 *        cfgmap[key]->setFromString("5");
 *
 * operators used to treat 'union' values as references
*/
class CfgItm {
public:
  CfgItm();
  CfgItm(char t);

  ~CfgItm();

  void chk_del_str();

  // used after init() when reading config
  void setFromString(const char *strval);
  void setFromString(std::string strval);

  std::string getString() const;

  /* accessors (previous class)
  * TYPE SHOULDN'T CHANGE
  * strnum operators are covered in their struct def
  * and are best assigned by string
  * you could assume strnum type and cast/assign for INT
  * but as they are changed one at a time manually through
  * menu/reconfigure so there is little benefit
  */
  void operator= (bool b)         { if (type == 'b') m_val.b = b; }
  void operator= (int i)          { if (type == 'i') m_val.i = i; }
  void operator= (unsigned int u) { if (type == 'u') m_val.u = u; }
  void operator= (const char *c)  { setFromString(c); }
  void operator= (std::string s)  { setFromString(s); }

  // references
  operator strnum&()       { return *m_val.e; }
  operator bool&()         { return m_val.b; }
  operator int&()          { if (type == 'i') return m_val.i;
                             // assume strnum
                             return (*m_val.e).val;
                           }
  operator unsigned int&() { return m_val.u; }
  operator std::string&()  { return *m_val.s; }

private:
  char type = 0;
  cfgval m_val { }; // init with 0s, 'memset'
}; // CfgItm

// CfgItm
////////////////////////////////////////////////////
// ConfigManager

typedef std::map<std::string, CfgItm*> CFGMAP;

class ConfigManager {
public:
  ConfigManager(const char *filename);
  ~ConfigManager();

  // NOTE: the bool values are kind of bunk
  //       WM class checks bool and prints errors
  //       but you'd have to have serious issues that
  //       would be breaking your system before this
  //       flubs I'd assume
  // loads file and defaults into config map
  bool load(const char *filename);

  /// saves config map to file
  bool save(const char *filename);

  CFGMAP &get_cfgmap() { return cfgmap; }

private:
  CFGMAP cfgmap;
  std::string m_filename;
}; // ConfigManager

} // namespace tk

#endif // TK_CONFIG_HH

// Copyright (c) 2023 Shynebox - zlice
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
