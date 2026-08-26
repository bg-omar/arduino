#ifndef WALLZ_VISUAL_STORE_H
#define WALLZ_VISUAL_STORE_H
#include "visual_memory.h"
namespace visual_store {
bool load(VisualMemory& memory);
bool save(const VisualMemory& memory);
void clear();
}
#endif
