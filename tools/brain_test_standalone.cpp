#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include "brain_protocol.h"
#include "brain_core.h"

int main(){
    BrainTelemetry t;
    assert(parseBrainTelemetry("T,100,500,1000,900,100,110,1,2,3,90,135,0,0,1", t));
    assert(t.ms==100 && t.distance_mm==500 && t.light_l==1000 && t.head_z==135 && t.brain_armed==1);
    assert(!parseBrainTelemetry("X,100", t));

    BrainCore b;
    for(int i=0;i<100;++i){
        t.ms=i*100; t.distance_mm=1000; t.light_l=500; t.light_r=500; t.mic_l=100; t.mic_r=100;
        b.observe(t);
    }
    assert(b.metrics().observations==100);
    assert(b.metrics().curiosity>=0.0f && b.metrics().curiosity<=1.0f);

    t.distance_mm=250;
    b.observe(t);
    assert(b.classify(t)==BrainContext::Obstacle);
    assert(b.suggest(t)==BrainAction::Stop);

    t.distance_mm=1000; t.light_l=2000; t.light_r=500;
    b.observe(t);
    assert(b.classify(t)==BrainContext::LightLeft);
    assert(b.suggest(t)==BrainAction::LookLeft);

    b.markAction(BrainContext::LightLeft, BrainAction::LookLeft);
    const float q0=b.q(BrainContext::LightLeft, BrainAction::LookLeft);
    b.reward(1.0f, BrainContext::Calm);
    const float q1=b.q(BrainContext::LightLeft, BrainAction::LookLeft);
    assert(q1>q0);

    BrainPersist p=b.persist();
    BrainCore restored;
    assert(restored.restore(p));
    assert(std::fabs(restored.q(BrainContext::LightLeft, BrainAction::LookLeft)-q1)<1e-6f);

    std::cout << "Wall-Z Brain v0.1 standalone tests: PASS\n";
    return 0;
}
