#include "imitation_store.h"
#include <Preferences.h>

namespace {
constexpr const char* kNamespace = "wallzimit";
constexpr const char* kKey = "state04";
}

namespace imitation_store {
bool load(ImitationMemory& memory) {
    Preferences p;
    if (!p.begin(kNamespace, true)) return false;
    ImitationPersist state;
    const size_t got = p.getBytes(kKey, &state, sizeof(state));
    p.end();
    return got == sizeof(state) && memory.restore(state);
}

bool save(const ImitationMemory& memory) {
    Preferences p;
    if (!p.begin(kNamespace, false)) return false;
    const ImitationPersist state = memory.persist();
    const size_t wrote = p.putBytes(kKey, &state, sizeof(state));
    p.end();
    return wrote == sizeof(state);
}

void clear() {
    Preferences p;
    if (!p.begin(kNamespace, false)) return;
    p.remove(kKey);
    p.end();
}
}
