// Orientation.hh for Shynebox Window Manager

/*
  Just defines for Justify and Orientation.

  Note: 180 orientation is never used (upside down rotation)
*/

#ifndef TK_ORIENTATION_HH
#define TK_ORIENTATION_HH

namespace tk {

enum Justify {LEFT, RIGHT, CENTER};

// < 2 is horz, >= 2 is vert
enum Orientation { ROT0=0, ROT180=1, ROT90=2, ROT270=3 };

} // end namespace tk

#endif // TK_ORIENTATION_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2008 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
