#include "brain_store.h"
#include <Preferences.h>

namespace {
constexpr const char* kNamespace = "wallzbrain";
constexpr const char* kKey = "state01";
}

namespace brain_store {
bool load(BrainCore& brain) {
    Preferences p;
    if (!p.begin(kNamespace, true)) return false;
    BrainPersist state;
    const size_t got = p.getBytes(kKey, &state, sizeof(state));
    p.end();
    return got == sizeof(state) && brain.restore(state);
}

bool save(const BrainCore& brain) {
    Preferences p;
    if (!p.begin(kNamespace, false)) return false;
    const BrainPersist state = brain.persist();
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
