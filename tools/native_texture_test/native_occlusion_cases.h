#pragma once
#include "gpu/native_occlusion.h"
namespace native_occlusion_tests {
inline void Run() {
  using namespace bd::gpu;
  scene::RenderMatrix identity{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
  NativeOcclusionView view{{identity,identity,identity}, {71,8,8,1}, 10};
  const NativeOcclusionIdentity node{12,34,5};
  const std::array<float,4> sphere{100,200,300.5f,.1f};
  view.camera.world_to_clip[12] = -100;
  view.camera.world_to_clip[13] = -200;
  view.camera.world_to_clip[14] = -300;
  auto packet = PrepareNativeOcclusion(view.camera,sphere);
  assert(packet && packet->world_to_clip[3] == -100 && packet->world_to_clip[11] == -300);
  assert(!PrepareNativeOcclusion(view.camera,{100,200,300.05f,.1f})); // near-clipped, far from origin
  auto translated = view.camera; translated.world_to_clip[14] = 2;
  assert(PrepareNativeOcclusion(translated,{0,0,0,.1f})); // world origin is not the eye
  auto rotated = view.camera;
  rotated.world_to_clip = {0,0,1,0, 0,1,0,0, -1,0,0,0, 0,0,-100,1};
  assert(PrepareNativeOcclusion(rotated,{100.5f,0,0,.1f}));
  assert(!PrepareNativeOcclusion(rotated,{100.05f,0,0,.1f}));
  auto perspective = translated;
  perspective.world_to_clip[15] = 0; perspective.world_to_clip[11] = 1;
  assert(!PrepareNativeOcclusion(perspective,{0,0,.05f,.1f})); // behind/through eye
  for (float invalid : {0.f,-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()})
    assert(!PrepareNativeOcclusion(view.camera,{100,200,300.5f,invalid}));
  auto invalid_camera = view.camera;
  invalid_camera.world_to_clip[0] = std::numeric_limits<float>::quiet_NaN();
  assert(!PrepareNativeOcclusion(invalid_camera,sphere));
  NativeOcclusionTracker tracker;
  std::vector<NativeOcclusionObservation> submitted;
  for (uint32_t frame : {10u,11u}) {
    view.frame = frame; tracker.Begin(frame);
    tracker.Note(node,view,sphere);
    tracker.Queries(view,[&](const auto &query) { submitted.push_back(query); });
    tracker.EndPass();
    assert(!tracker.Occluded(node,view));
  }
  assert(submitted.size() == 2);
  for (const auto &query : submitted) tracker.Collect(query,true); // delayed collection, original frame retained
  view.frame = 12; tracker.Begin(12);
  assert(!tracker.Occluded(node,view)); // old results alone never suppress a draw
  tracker.Note(node,view,sphere);
  assert(tracker.Occluded(node,view));
  auto changed_view = view; ++changed_view.scope.depth;
  assert(!tracker.Occluded(node,changed_view));
  changed_view = view; ++changed_view.scope.samples;
  assert(!tracker.Occluded(node,changed_view));
  changed_view = view; changed_view.camera.world_to_clip[12] += .01f;
  assert(!tracker.Occluded(node,changed_view));
  assert(!tracker.Occluded({12,35,5},view) && !tracker.Occluded({13,34,5},view));
  tracker.Note(node,std::nullopt,sphere);
  assert(!tracker.Occluded(node,view));
  tracker.Note(node,view,sphere); // invalidated identity remains invalid until next frame
  assert(!tracker.Occluded(node,view));
  view.frame = 13; tracker.Begin(13); tracker.Note(node,view,sphere);
  assert(tracker.Occluded(node,view));
  tracker.Note(node,view,{100,200,300.6f,.1f});
  assert(!tracker.Occluded(node,view)); // conflicting repeated identity cannot be queried
  unsigned count = 0; tracker.Queries(view,[&](const auto &) { ++count; }); assert(count == 0);
  view.frame = 16; tracker.Begin(16); tracker.Note(node,view,sphere);
  assert(!tracker.Occluded(node,view)); // result older than three frames
  NativeOcclusionHistory history;
  history.Collect(submitted[1],true); history.Collect(submitted[0],true);
  auto now = submitted[1]; now.frame = 12;
  assert(!history.Occluded(now)); // out-of-order/duplicate results cannot manufacture two zeros
  history.Collect(submitted[1],true); assert(!history.Occluded(now));
  history.Collect(now,false); ++now.frame; assert(!history.Occluded(now));
  tracker.Begin(20); view.frame = 20;
  assert(tracker.HistoryCount() == 0);
  for (uint32_t n=0;n<NativeOcclusionTracker::kQueries+1;++n) tracker.Note({1,1,n},view,sphere);
  assert(tracker.CurrentCount() == NativeOcclusionTracker::kQueries);
  for (uint32_t n=0;n<NativeOcclusionTracker::kHistory+1;++n) {
    auto query = submitted[0]; query.identity.node = n; query.frame = 20;
    tracker.Collect(query,true);
  }
  assert(tracker.HistoryCount() == NativeOcclusionTracker::kHistory);
  tracker.Begin(29); assert(tracker.HistoryCount() == 0 && tracker.CurrentCount() == 0);
}
}
