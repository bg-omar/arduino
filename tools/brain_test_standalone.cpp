#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <string>
#include "brain_protocol.h"
#include "brain_core.h"
#include "vision_protocol.h"
#include "vision_grid.h"
#include "vision_grid_protocol.h"
#include "visual_memory.h"

int main(){
    BrainTelemetry t;
    assert(parseBrainTelemetry("T,100,500,1000,900,100,110,1,2,3,90,135,0,0,1", t));
    assert(t.ms==100 && t.distance_mm==500 && t.light_l==1000 && t.head_z==135 && t.brain_armed==1);
    assert(t.vision_online==0);
    assert(!parseBrainTelemetry("X,100", t));

    VisionTelemetry v;
    assert(parseVisionTelemetry("V,200,81,-420,110,127,44,198,7", v));
    assert(v.ms==200 && v.motion==81 && v.x==-420 && v.fps_x10==198 && v.flags==7);
    assert(!parseVisionTelemetry("X,200", v));

    std::string gridLine = "G,300,7,100,80,";
    static const char hx[] = "0123456789ABCDEF";
    for (int i=0;i<WALLZ_SNAPSHOT_GRID_CELLS;++i) {
        const uint8_t q=static_cast<uint8_t>(i);
        gridLine.push_back(hx[q>>4]); gridLine.push_back(hx[q&15]);
    }
    VisionGridSnapshot gs;
    assert(parseVisionGridSnapshot(gridLine.c_str(), gs));
    assert(gs.ms==300 && gs.seq==7 && gs.grid[0]==0 && gs.grid[255]==255);


    // Synthetic visual-grid test: introduce a bright patch on the left.
    uint8_t prevGrid[WALLZ_VISION_GRID_CELLS] = {};
    bool haveGrid = false;
    uint8_t frame0[WALLZ_VISION_FRAME_W * WALLZ_VISION_FRAME_H];
    uint8_t frame1[WALLZ_VISION_FRAME_W * WALLZ_VISION_FRAME_H];
    memset(frame0, 80, sizeof(frame0));
    memset(frame1, 80, sizeof(frame1));
    auto g0 = wallzAnalyzeGrayFrame(frame0, WALLZ_VISION_FRAME_W, WALLZ_VISION_FRAME_H, prevGrid, haveGrid, 18);
    (void)g0;
    for (int y=30; y<90; ++y) for (int x=8; x<56; ++x) frame1[y*WALLZ_VISION_FRAME_W+x]=220;
    auto g1 = wallzAnalyzeGrayFrame(frame1, WALLZ_VISION_FRAME_W, WALLZ_VISION_FRAME_H, prevGrid, haveGrid, 18);
    assert(g1.motion > 45);
    assert(g1.x < -150);

    VisualMemory vm;
    VisionGridSnapshot teach{};
    memset(teach.grid,40,sizeof(teach.grid));
    for (int y=3;y<=12;++y) for (int x=5;x<=12;++x) teach.grid[y*WALLZ_SNAPSHOT_GRID_W+x]=210;
    vm.observe(teach);
    assert(vm.teach("person"));
    VisionGridSnapshot shifted{}; memset(shifted.grid,40,sizeof(shifted.grid));
    for (int y=3;y<=12;++y) for (int x=6;x<=13;++x) shifted.grid[y*WALLZ_SNAPSHOT_GRID_W+x]=210;
    auto rec=vm.observe(shifted);
    assert(rec.index>=0 && std::strcmp(rec.label,"person")==0 && rec.score>=WALLZ_VISUAL_RECOGNIZE_THRESHOLD);
    VisionGridSnapshot horizontal{}; memset(horizontal.grid,40,sizeof(horizontal.grid));
    for (int y=5;y<=9;++y) for (int x=2;x<=17;++x) horizontal.grid[y*WALLZ_SNAPSHOT_GRID_W+x]=210;
    auto unknown=vm.observe(horizontal);
    assert(unknown.index<0);
    vm.observe(shifted);
    vm.rewardCurrent(1.0f); assert(vm.current().value_milli>0);
    auto vp=vm.persist(); VisualMemory vm2; assert(vm2.restore(vp)); assert(vm2.conceptCount()==1);

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

    // Visual motion has priority as an attention source but does not bypass
    // RA safety: the Brain only suggests a head turn here.
    t.light_l=500; t.light_r=500;
    t.vision_online=1; t.vision_motion=120; t.vision_x=-650; t.vision_y=0;
    b.observe(t);
    assert(b.classify(t)==BrainContext::MotionEvent);
    assert(b.suggest(t)==BrainAction::LookLeft);
    t.vision_x=700;
    assert(b.suggest(t)==BrainAction::LookRight);

    t.vision_online=0; t.vision_motion=0;
    t.light_l=2000; t.light_r=500;
    b.markAction(BrainContext::LightLeft, BrainAction::LookLeft);
    const float q0=b.q(BrainContext::LightLeft, BrainAction::LookLeft);
    b.reward(1.0f, BrainContext::Calm);
    const float q1=b.q(BrainContext::LightLeft, BrainAction::LookLeft);
    assert(q1>q0);

    BrainPersist p=b.persist();
    BrainCore restored;
    assert(restored.restore(p));
    assert(std::fabs(restored.q(BrainContext::LightLeft, BrainAction::LookLeft)-q1)<1e-6f);

    std::cout << "Wall-Z Brain v0.3 standalone tests: PASS\n";
    return 0;
}
