#include <cassert>
#include <iostream>
#include "shared_control.h"

int main() {
    assert(wallzControlOwner(true, false, false) == WallzControlOwner::Brain);
    assert(wallzControlOwner(true, true, false) == WallzControlOwner::Ps4Manual);
    // Manual override does not alter the armed state; once neutral, Brain owns it again.
    const bool stillArmed = true;
    assert(wallzControlOwner(stillArmed, false, false) == WallzControlOwner::Brain);
    assert(wallzControlOwner(true, false, true) == WallzControlOwner::RobotMode);
    assert(wallzControlOwner(false, false, false) == WallzControlOwner::Stopped);
    std::cout << "Wall-Z Brain v0.6 shared-control standalone tests: PASS\n";
}
