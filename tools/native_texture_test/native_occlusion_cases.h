#pragma once
#include "gpu/native_occlusion.h"
namespace native_occlusion_tests {
inline bd::gpu::scene::NativeBounds Box(float x, float y, float z, float r) {
  return {{x-r,y-r,z-r},{x+r,y+r,z+r}};
}
inline void Run() {
  using namespace bd::gpu;
  scene::RenderMatrix identity{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
  NativeOcclusionView view{{identity,identity,identity}, {71,8,8,1}, 10};
  const NativeOcclusionIdentity node{12,34,5};
  const auto bounds = Box(100,200,300.5f,.1f);
  view.camera.world_to_clip[12] = -100;
  view.camera.world_to_clip[13] = -200;
  view.camera.world_to_clip[14] = -300;
  auto packet = PrepareNativeOcclusion(view.camera,bounds);
  assert(packet && packet->world_to_clip[3] == -100 && packet->world_to_clip[11] == -300);
  assert(!PrepareNativeOcclusion(view.camera,Box(100,200,300.05f,.1f))); // near-clipped, far from origin
  auto translated = view.camera; translated.world_to_clip[14] = 2;
  assert(PrepareNativeOcclusion(translated,Box(0,0,0,.1f))); // world origin is not the eye
  auto rotated = view.camera;
  rotated.world_to_clip = {0,0,1,0, 0,1,0,0, -1,0,0,0, 0,0,-100,1};
  assert(PrepareNativeOcclusion(rotated,Box(100.5f,0,0,.1f)));
  assert(!PrepareNativeOcclusion(rotated,Box(100.05f,0,0,.1f)));
  auto perspective = translated;
  perspective.world_to_clip[15] = 0; perspective.world_to_clip[11] = 1;
  assert(!PrepareNativeOcclusion(perspective,Box(0,0,.05f,.1f))); // behind/through eye
  for (float invalid : {-1.f,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()})
    assert(!PrepareNativeOcclusion(view.camera,Box(100,200,300.5f,invalid)));
  auto invalid_camera = view.camera;
  invalid_camera.world_to_clip[0] = std::numeric_limits<float>::quiet_NaN();
  assert(!PrepareNativeOcclusion(invalid_camera,bounds));
  // A wide, shallow mesh is not an isotropic node sphere. Its true box can be
  // queried without crossing the near plane, including a flat triangle.
  assert(PrepareNativeOcclusion(translated,{{-100,-2,0},{100,2,0}}));
  auto narrow = PrepareNativeOcclusion(view.camera,{{99,199,300.49f},{101,201,300.51f}});
  assert(narrow && narrow->extent[0] > 1 && narrow->extent[2] < .02f);
  for (unsigned axis=0;axis<3;++axis) {
    assert(double(packet->center[axis])-packet->extent[axis] <= bounds.min[axis]);
    assert(double(packet->center[axis])+packet->extent[axis] >= bounds.max[axis]);
  }
  NativeOcclusionTracker tracker;
  using D = NativeOcclusionDecision;
  std::vector<NativeOcclusionObservation> submitted;
  for (uint32_t frame : {10u,11u}) {
    view.frame = frame; tracker.Begin(frame);
    assert(!tracker.HasQueries(view)); // no native consumer means no queries
    assert(tracker.Request(node,view,bounds) == D::NoHistory);
    tracker.Queries(view,[&](const auto &query) { submitted.push_back(query); });
    tracker.EndPass();
    assert(!tracker.HasQueries(view));
  }
  assert(submitted.size() == 2);
  for (const auto &query : submitted) tracker.Collect(query,true); // delayed collection, original frame retained
  view.frame = 12; tracker.Begin(12);
  assert(!tracker.HasQueries(view)); // old results alone do not request work
  assert(tracker.Request(node,view,bounds) == D::Occluded);
  assert(tracker.Request({12,34,5,1},view,bounds) == D::NoHistory); // sibling cannot inherit zero
  tracker.EndPass(); assert(tracker.Request(node,view,bounds) == D::Occluded);
  assert(tracker.HasQueries(view)); // culled nodes still need fresh visibility results
  auto changed_view = view; ++changed_view.scope.depth;
  tracker.EndPass(); assert(tracker.Request(node,changed_view,bounds) == D::ChangedDepth);
  changed_view = view; ++changed_view.scope.samples;
  tracker.EndPass(); assert(tracker.Request(node,changed_view,bounds) == D::ChangedDepth);
  changed_view = view; changed_view.camera.world_to_clip[12] += .01f;
  tracker.EndPass(); assert(tracker.Request(node,changed_view,bounds) == D::ChangedCamera);
  tracker.EndPass(); assert(tracker.Request(node,view,Box(100,200,300.6f,.1f)) == D::ChangedBounds);
  assert(tracker.Request({12,35,5},view,bounds) == D::NoHistory);
  assert(tracker.Request({13,34,5},view,bounds) == D::NoHistory);
  tracker.EndPass(); assert(tracker.Request(node,view,bounds) == D::Occluded);
  assert(tracker.Request(node,std::nullopt,bounds) == D::InvalidView);
  assert(tracker.Request(node,view,bounds) == D::Ambiguous); // cannot revive this pass
  view.frame = 13; tracker.Begin(13);
  assert(tracker.Request(node,view,bounds) == D::Occluded);
  assert(tracker.Request(node,view,Box(100,200,300.6f,.1f)) == D::Ambiguous);
  unsigned count = 0; tracker.Queries(view,[&](const auto &) { ++count; }); assert(count == 0);
  view.frame = 16; tracker.Begin(16);
  assert(tracker.Request(node,view,bounds) == D::Stale); // older than three frames
  assert(tracker.Request(node,view,std::nullopt) == D::InvalidBounds);
  assert(tracker.Request(node,view,bounds) == D::Ambiguous);
  tracker.EndPass(); changed_view = view; ++changed_view.frame;
  assert(tracker.Request(node,changed_view,bounds) == D::InvalidView);
  changed_view = view; changed_view.scope.samples = 0;
  assert(tracker.Request(node,changed_view,bounds) == D::InvalidView);
  assert(tracker.Request(node,view,Box(100,200,300.05f,.1f)) == D::InvalidBounds);
  assert(!tracker.CurrentCount());
  NativeOcclusionHistory history;
  history.Collect(submitted[1],true); history.Collect(submitted[0],true);
  auto now = submitted[1]; now.frame = 12;
  assert(history.Decide(now) == D::Warming); // out-of-order cannot manufacture two zeros
  history.Collect(submitted[1],true); assert(!history.Occluded(now));
  history.Collect(now,false); ++now.frame; assert(history.Decide(now) == D::Visible);
  tracker.Begin(20); view.frame = 20;
  assert(tracker.HistoryCount() == 0);
  for (uint32_t n=0;n<NativeOcclusionTracker::kQueries;++n)
    assert(tracker.Request({1,1,n},view,bounds) == D::NoHistory);
  assert(tracker.Request({1,1,NativeOcclusionTracker::kQueries},view,bounds) == D::Capacity);
  assert(tracker.CurrentCount() == NativeOcclusionTracker::kQueries);
  for (uint32_t n=0;n<NativeOcclusionTracker::kHistory+1;++n) {
    auto query = submitted[0]; query.identity.node = n; query.frame = 20;
    tracker.Collect(query,true);
  }
  assert(tracker.HistoryCount() == NativeOcclusionTracker::kHistory);
  tracker.Begin(29); assert(tracker.HistoryCount() == 0 && tracker.CurrentCount() == 0);
}
}
