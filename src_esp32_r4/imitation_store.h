#ifndef WALLZ_IMITATION_STORE_H
#define WALLZ_IMITATION_STORE_H

#include "imitation_memory.h"

namespace imitation_store {
bool load(ImitationMemory& memory);
bool save(const ImitationMemory& memory);
void clear();
}

#endif // WALLZ_IMITATION_STORE_H
