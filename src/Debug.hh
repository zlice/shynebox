#ifndef DEBUG_HH
#define DEBUG_HH

#include <iostream>
#ifdef DEBUG
#define sbdbg std::cerr<<__FILE__<<"("<<__LINE__<< "): "
#else
#define sbdbg if (false) std::cerr
#endif // DEBUG

#endif // DEBUG_HH
