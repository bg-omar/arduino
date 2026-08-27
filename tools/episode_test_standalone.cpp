#include <cassert>
#include <cstring>
#include <iostream>
#include "wallz_episode_protocol.h"
#include "episode_capture_policy.h"

int main() {
    EpisodeRequest req;
    assert(wallzParseEpisodeRequest("C,EP,reward_pos,person,2000,3000", req));
    assert(std::strcmp(req.reason, "reward_pos") == 0);
    assert(std::strcmp(req.label, "person") == 0);
    assert(req.pre_ms == 2000 && req.post_ms == 3000);
    assert(!wallzParseEpisodeRequest("C,EP,bad reason,person,2000,3000", req));

    EpisodeEvent ev;
    assert(wallzParseEpisodeEvent("VE,BEGIN,4,12,teach,ball,10,15", ev));
    assert(ev.type == EpisodeEvent::Type::Begin);
    assert(ev.session == 4 && ev.episode == 12);
    assert(ev.pre_frames == 10 && ev.post_frames == 15);
    assert(wallzParseEpisodeEvent("VE,DONE,4,12,teach,ball,26,0", ev));
    assert(ev.type == EpisodeEvent::Type::Done);
    assert(ev.total_frames == 26 && ev.errors == 0);

    EpisodeCapturePolicy p;
    assert(p.sampleDue(100));
    assert(!p.sampleDue(299));
    assert(p.sampleDue(300));
    assert(p.canTrigger(300));
    p.begin(300, 3000);
    assert(p.active());
    assert(!p.postComplete(3299));
    assert(p.postComplete(3300));
    p.notePostFrame();
    assert(p.postFrames() == 1);
    p.finish();
    assert(!p.active());
    assert(!p.canTrigger(1000));
    assert(p.canTrigger(1500));

    std::cout << "PASS episode protocol + timing policy\n";
    return 0;
}
