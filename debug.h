#pragma once

#if !defined(__BORLANDC__) // C++ Builder does not have overloaded operator new with 4 parameters (4th parameters is Size?)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#ifdef _DEBUG
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
// Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the allocations to be of _CLIENT_BLOCK type
#else
#define DBG_NEW new
#endif
#else
#define DBG_NEW new
#endif

