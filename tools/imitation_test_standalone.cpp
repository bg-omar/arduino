#include <cassert>
#include <iostream>
#include "brain_protocol.h"
#include "manual_demo.h"
#include "imitation_memory.h"

static BrainTelemetry baseTelemetry() {
    BrainTelemetry t;
    t.distance_mm = 1200;
    t.light_l = 700; t.light_r = 650;
    t.mic_l = 100; t.mic_r = 110;
    t.head_xy = 90; t.head_z = 135;
    t.vision_online = 1; t.vision_motion = 80; t.vision_x = -200; t.vision_familiarity = 850;
    return t;
}

int main() {
    ManualDemonstration d;
    assert(parseManualDemonstration("D,1234,-25,91,0,0,1,0", d));
    assert(d.ms == 1234 && d.lx == -25 && d.ly == 91 && d.drive_active == 1);
    assert(ImitationMemory::actionFromManual(d) == ImitationAction::Forward);

    ImitationMemory m;
    d = ManualDemonstration{}; d.drive_active = 1; d.ly = 100;
    BrainTelemetry a=baseTelemetry();
    BrainTelemetry b=baseTelemetry(); b.distance_mm=2200; b.vision_x=-550;
    BrainTelemetry c=baseTelemetry(); c.distance_mm=3600; c.light_l=1300; c.light_r=500;
    assert(m.learn(d,a,ImitationMemory::tagFromLabel("person")));
    assert(m.learn(d,b,ImitationMemory::tagFromLabel("person")));
    assert(m.learn(d,c,ImitationMemory::tagFromLabel("person")));
    BrainTelemetry q=baseTelemetry(); q.distance_mm=1350;
    const auto pred=m.predict(q,ImitationMemory::tagFromLabel("person"));
    std::cout << "prediction=" << ImitationMemory::actionName(pred.action)
              << " confidence=" << pred.confidence << " nearest=" << pred.nearest_distance << "\n";
    assert(pred.action == ImitationAction::Forward);
    assert(pred.confidence >= WALLZ_IMITATION_EXEC_THRESHOLD);
    const auto state=m.persist(); ImitationMemory restored; assert(restored.restore(state));
    assert(restored.count()==3);

    ImitationMemory dedup;
    assert(dedup.learn(d,a,0));
    assert(!dedup.learn(d,a,0));
    assert(dedup.rejectedDuplicate()==1);
    std::cout << "Wall-Z Brain v0.4 imitation standalone tests: PASS\n";
}
