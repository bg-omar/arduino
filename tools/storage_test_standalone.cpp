#include <cassert>
#include <iostream>
#include "wallz_camera_storage_protocol.h"
#include "camera_storage_policy.h"

int main() {
    CameraBrainContext c;
    assert(wallzParseBrainContext("C,CTX,ball,850,150,-200", c));
    assert(std::string(c.label) == "ball");
    CameraStoreRequest r;
    assert(wallzParseStoreRequest("C,SAVE,reward_pos,ball", r));
    CameraStorageStatus s;
    assert(wallzParseStorageStatus("VS,SD,1,32000,100,3,4,0", s));
    assert(s.mounted && s.frames == 3);

    CameraStoragePolicy p;
    CameraStoragePolicyInput in;
    in.now_ms=6000; in.motion=250; in.brain_context_fresh=false;
    assert(p.evaluate(in)==CameraAutoSaveReason::MotionOffline);
    in.now_ms=12000; in.motion=100; in.brain_context_fresh=true; in.familiarity=400; in.novelty=100;
    assert(p.evaluate(in)==CameraAutoSaveReason::UnknownMotion);
    std::cout << "Wall-Z Brain v0.5 camera storage standalone tests: PASS\n";
}
