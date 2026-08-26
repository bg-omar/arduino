#ifndef WALLZ_BRAIN_STORE_H
#define WALLZ_BRAIN_STORE_H

#include "brain_core.h"

namespace brain_store {
bool load(BrainCore& brain);
bool save(const BrainCore& brain);
void clear();
}

#endif
