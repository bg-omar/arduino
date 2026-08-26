#include "visual_store.h"
#include <Preferences.h>
namespace {
constexpr const char* kNamespace = "wallzvision";
constexpr const char* kKey = "concept01";
}
namespace visual_store {
bool load(VisualMemory& memory) {
    Preferences p;
    if (!p.begin(kNamespace, true)) return false;
    VisualMemoryPersist state;
    const size_t got = p.getBytes(kKey, &state, sizeof(state));
    p.end();
    return got == sizeof(state) && memory.restore(state);
}
bool save(const VisualMemory& memory) {
    Preferences p;
    if (!p.begin(kNamespace, false)) return false;
    const VisualMemoryPersist state = memory.persist();
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
