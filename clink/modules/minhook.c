
// Just a cl wrapper for minhook Win32/Win64 files

#if defined(_WIN64)
#include "minhook/src/HDE/hde64.c"
#else
#include "minhook/src/HDE/hde32.c"
#endif
