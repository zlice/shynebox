// Command.hh for Shynebox Window Manager

/*
  Interface class for commands.
  Holds a templated function to call for the command.
  Can be shared (mainly for ConfigMenu items).
*/

#ifndef TK_COMMAND_HH
#define TK_COMMAND_HH

#include "NotCopyable.hh"

namespace tk {

  // slot boilerplate classes for command
  // function calling magic
  class SlotBase: private tk::NotCopyable {
  public:
    virtual ~SlotBase() { }
  };

  template<typename RetType>
  class Slot: public SlotBase {
  public:
    virtual RetType operator()() = 0;
  };

  template<typename Func, typename RetType>
  class SlotImpl: public Slot<RetType>{
  public:
    virtual RetType operator()() {
      return static_cast<RetType>(m_functor() );
    }

    SlotImpl(Func functor) : m_functor(functor) { }

  private:
    Func m_functor;
  };

template <typename Ret=void>
class Command: public Slot<Ret> {
public:
  virtual Ret execute() = 0;
  virtual Ret operator()() { return execute(); }
  void set_is_shared() { m_is_shared = true; }
  bool get_is_shared() { return m_is_shared; }
private:
  bool m_is_shared = false;
};

} // end namespace tk




// kept for reference from old slot class/file

// for multiple arguments, not used without signals
/*
// multi args
template<typename RetType, typename... Args>
class Slot: public FuncImpl::SlotBase {
public:
  virtual RetType operator()(Args...) = 0;
};

// 0 args
template<typename RetType>
class Slot<RetType, FuncImpl::EmptyArg>: public FuncImpl::SlotBase {
public:
  virtual RetType operator()() = 0;
};

// multi args
template<typename Func, typename RetType, typename... Args>
class SlotImpl: public Slot<RetType, Args...> {
public:
  virtual RetType operator()(Args... args) {
    return static_cast<RetType>(m_functor(args...) );
  }

  SlotImpl(Func functor) : m_functor(functor) { }

private:
  Func m_functor;
};

// 0 args
template<typename Func, typename RetType>
class SlotImpl<Func, RetType, FuncImpl::EmptyArg>: public Slot<RetType>{
public:
  virtual RetType operator()() {
    return static_cast<RetType>(m_functor() );
  }

  SlotImpl(Func functor) : m_functor(functor) { }

private:
  Func m_functor;
};
*/



#endif // TK_COMMAND_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2002 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
