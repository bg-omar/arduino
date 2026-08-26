#include <cassert>
#include <iostream>
#include "user_mode_protocol.h"

int main() {
    assert(parseRaUserModeRequest("U,MODE,BRAIN") == RaUserModeRequest::Brain);
    assert(parseRaUserModeRequest("U,MODE,IMITATION") == RaUserModeRequest::Imitation);
    assert(parseRaUserModeRequest("U,MODE,SENSE") == RaUserModeRequest::SenseReact);
    assert(parseRaUserModeRequest("U,MODE,FREE_ROAM") == RaUserModeRequest::FreeRoam);
    assert(parseRaUserModeRequest("U,MODE,STOP") == RaUserModeRequest::Stop);
    assert(parseRaUserModeRequest("T,123") == RaUserModeRequest::None);
    std::cout << "Wall-Z Brain v0.6 user-mode protocol tests: PASS\n";
}
