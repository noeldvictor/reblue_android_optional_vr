#include "gpu/scene/native_light_selection_source.h"
#include "gpu/scene/native_selected_lights_source.h"
#include "gpu/scene/native_rigid_inputs.h"
#include <algorithm>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <vector>
using namespace bd::gpu::scene;
namespace {
void Require(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
constexpr NativeLightScoreParameters parameters{255.f, 1.f, -.00001f, .00001f};
NativeLightCandidate Directional(int id, float intensity = 1.f) {
  NativeLightCandidate value;
  value.id = id; value.kind = LitDirectional; value.enabled = true;
  value.views = 0xffff; value.intensity = intensity;
  return value;
}
const auto classify = [](const NativeLightSelection &s) -> std::optional<uint8_t> {
  return s.slots[0].id < 0 ? 0 : s.slots[1].id < 0 ? 1 : 3;
};
}
void TestNativeLightSelection() {
  NativeLightSelectionInputs input;
  input.mode = 2; input.radius = 2;
  NativeLightSelection selected;
  const auto insert = [&](NativeLightCandidate light) {
    return InsertNativeLight(selected, light, input, parameters, classify);
  };
  Require(insert(Directional(1,.5f)) && insert(Directional(2,.5f)) && insert(Directional(3,.25f)) &&
          insert(Directional(4,.5f)), "native selection accepts ordinary candidates");
  Require(selected.slots[0].id == 1 && selected.slots[1].id == 2 && selected.slots[2].id == 4,
          "ties preserve the existing authored order, evicting only a lower score");
  auto priority = Directional(5,.1f); priority.priority = true;
  Require(insert(priority) && selected.slots[0].id == 5 && selected.slots[0].priority == 1,
          "priority insertion precedes brighter non-priority lights");
  input.priority_light = 6;
  Require(insert(Directional(6,.05f)) && selected.slots[0].id == 6 && selected.slots[1].id == 5,
          "sun insertion precedes other priority lights");
  const auto before = selected;
  auto excluded = Directional(7); excluded.excluded_objects = 1;
  Require(insert(excluded) && selected == before, "object exclusion mask");
  excluded.excluded_objects = 2; input.object_class = 1; input.special_scene = true;
  Require(insert(excluded) && selected.slots[2].id == 7, "special scene exemption for classes one through eight");
  selected = before; input.view = 8;
  Require(insert(excluded) && selected == before, "view eight still obeys object exclusion");
  input.special_scene = false; input.view = 0; input.object_class = 0; input.mode = 0;
  Require(insert(Directional(8)), "disabled lighting mode");
  for (size_t n = 0; n < 3; ++n)
    Require(selected.slots[n].id == -1 && selected.slots[n].weight == before.slots[n].weight &&
            selected.slots[n].priority == before.slots[n].priority, "mode zero changes IDs only");
  Require(selected.category == before.category, "mode zero preserves category");
  input.mode = 1; selected = {}; selected.category = 6;
  priority.id = 9;
  Require(insert(priority) && selected.slots[0] == NativeLightSelectionSlot{9,0,1} && selected.category == 6,
          "directional-only priority is zero-weight and leaves category unchanged");
  Require(insert(Directional(10)) && selected.slots[0].id == 9, "directional priority remains pinned");
  auto point = Directional(0); point.kind = LitPoint; point.range = 10;
  input.centre = LitVec(0,0,11); input.radius = 2;
  Require(ScoreNativeLight(point,input,parameters) == 127, "point overlap score and truncation");
  input.centre.z = 10; input.radius = 0;
  Require(ScoreNativeLight(point,input,parameters) == 0, "zero-radius boundary uses ordered NaN attenuation");
  input.centre.z = 9;
  Require(ScoreNativeLight(point,input,parameters) == 255, "zero-radius interior remains lit");
  input.radius = 2; input.centre.z = 8;
  auto spot = point; spot.kind = LitSpot; spot.direction = LitVec(0,0,1); spot.cone_angle = .1f;
  Require(ScoreNativeLight(spot,input,parameters) == 255, "spot inside cone");
  spot.direction.z = -1;
  Require(ScoreNativeLight(spot,input,parameters) == 0, "spot behind cone");
  input.centre = LitVec(8,0,0); spot.direction = LitVec(0,0,1);
  Require(ScoreNativeLight(spot,input,parameters) == 0, "spot outside cone");
  input.centre = {}; spot.direction = {};
  Require(ScoreNativeLight(spot,input,parameters) == 255, "near-light sphere skips the cone cutoff");
  point.intensity = std::numeric_limits<float>::quiet_NaN();
  Require(!ScoreNativeLight(point,input,parameters), "invalid active intensity is refused");
  Require(ScoreNativeLight(Directional(0,-1),input,parameters) == 0 &&
          ScoreNativeLight(Directional(0,10),input,parameters) == 255, "score endpoints saturate safely");

  // Independent stable ranking reference for mode2: sun first, authored
  // priority next, then descending quantized brightness with stable ties.
  std::mt19937 random(0x8218A8C8);
  for (unsigned trial = 0; trial < 4000; ++trial) {
    input = {}; input.mode = 2; input.priority_light = int(random()%32);
    selected = {};
    std::vector<NativeLightCandidate> reference;
    for (int id = 0; id < 32; ++id) {
      auto light = Directional(id, float(1+random()%15)/16.f);
      light.priority = random()%8 == 0;
      Require(insert(light), "randomized candidate insertion");
      reference.push_back(light);
    }
    std::stable_sort(reference.begin(),reference.end(),[&](const auto &a,const auto &b) {
      const int a_group = a.id == input.priority_light ? 0 : a.priority ? 1 : 2;
      const int b_group = b.id == input.priority_light ? 0 : b.priority ? 1 : 2;
      if (a_group != b_group) return a_group < b_group;
      return a_group == 2 && int(a.intensity*255) > int(b.intensity*255);
    });
    for (size_t n = 0; n < 3; ++n)
      Require(selected.slots[n].id == reference[n].id, "128000 candidates match independent stable ranking");
  }

  std::unordered_map<uint64_t,uint32_t> words;
  const auto read = [&](uint64_t address) -> std::optional<uint32_t> {
    const auto it = words.find(address);
    return it == words.end() ? std::nullopt : std::optional(it->second);
  };
  const auto scalar = [&](uint64_t address, float value) { words[address] = std::bit_cast<uint32_t>(value); };
  NativeLightSelectionSource source{1000,60000,70000,3,true,false,parameters};
  const auto setup = [&] {
    words.clear(); source.view = 3; source.live_owner = 70000;
    words[1000] = 2; words[1000+46816] = 3; words[1000+48020] = 0;
    words[1000+12007*4] = words[70000+12007*4] = uint32_t(-1);
    words[60000] = 0; words[60004] = 0x80000008;
    words[60216] = 0xabcdef99;
    for (unsigned n = 0; n < 3; ++n) {
      words[60008+3*12+n*4] = 0xffff0000;
      scalar(60200+n*4,0.f);
      const uint64_t record = 1000+24016+n*76;
      for (unsigned at = 0; at < 76; at += 4) words[record+at] = 0;
      words[record] = 17; words[record+8] = 0xffff; words[record+12] = n;
      scalar(record+64,float(3-n)/4.f); scalar(record+72,10.f);
      words[70000+n*76+8] = 1;
    }
    scalar(60212,1.f);
  };
  setup();
  const auto plan = PrepareNativeLightSelection(source,read);
  Require(plan && plan->rebuilt && plan->candidates == 3 && plan->selection.slots[0].id == 0 &&
          plan->selection.slots[2].id == 2 && plan->writes[0].after == 0x80000000 &&
          plan->writes[4].after == 0xabcdef03, "BE selector import preserves other view bits/category bytes");
  for (const auto &write : plan->writes) words[write.address] = write.after;
  const auto unchanged = PrepareNativeLightSelection(source,read);
  Require(unchanged && !unchanged->rebuilt && !unchanged->candidates && unchanged->selection == plan->selection,
          "clean unchanged selection needs no candidates");
  words[1000+48020] = 1; words[1000+46820] = 1000+24016+76;
  scalar(1000+24016+76+64,1.f);
  const auto reselected = PrepareNativeLightSelection(source,read);
  Require(reselected && reselected->rebuilt && reselected->selection.slots[0].id == 1,
          "change to a selected ID forces full rebuild with updated intensity");
  // A second changed record can match a newly inserted ID, not only the entry set.
  setup(); words[60004] = 0; words[1000+48020] = 2;
  words[1000+46820] = words[1000+46824] = 1000+24016;
  Require(PrepareNativeLightSelection(source,read)->rebuilt, "incremental lookup sees the evolving selection");
  setup(); words[1000+46816] = 1; words[70000+8] = 2; words[70000+12007*4] = 0;
  const auto live_kind = PrepareNativeLightSelection(source,read);
  Require(live_kind && live_kind->selection.category == 6,
          "category uses live kind and live priority, not snapshot kind/manager priority");
  // Run941 observed view0 dirty FFFFFFFF instead of planned FFFFFFFE after
  // reference execution. Preserve that failure; a later invalidation is not
  // equivalent output even if the three selected IDs still match. This models
  // the boundary condition, not proof of which writer changed the live run.
  for (uint32_t view = 0; view < 16; ++view) {
    setup(); source.view = view; words[60004] = ~0u;
    words[60216+(view & ~3u)] = 0;
    for (uint32_t slot = 0; slot < 3; ++slot) words[60008+view*12+slot*4] = 0xffff0000;
    const auto expected = PrepareNativeLightSelection(source,read);
    Require(expected && expected->writes[0].after == (~0u & ~(1u << view)), "rebuild clears only its view bit");
    for (const auto &write : expected->writes) words[write.address] = write.after;
    Require(!FindNativeLightSelectionMismatch(*expected,read), "exact completed selection matches");
    words[60004] = ~0u;
    const auto mismatch = FindNativeLightSelectionMismatch(*expected,read);
    Require(mismatch && mismatch->write.address == 60004 && mismatch->actual == ~0u &&
            mismatch->write.after == (~0u & ~(1u << view)), "late dirty invalidation remains a strict mismatch");
    words.erase(60004);
    Require(!FindNativeLightSelectionMismatch(*expected,read)->actual, "missing output is not zero or successful comparison");
  }
  setup(); words[1000+46816] = 301;
  Require(!PrepareNativeLightSelection(source,read), "snapshot bound refuses before writes");
  setup(); words[1000+48020] = 301; words[60004] = 0;
  Require(!PrepareNativeLightSelection(source,read), "changed-list bound refuses before writes");
  setup(); words.erase(1000+24016+64);
  Require(!PrepareNativeLightSelection(source,read), "unreadable candidate refuses the whole selection");
  setup(); words[60004] = 0; words[1000+48020] = 1; words[1000+46820] = 60004-12;
  Require(!PrepareNativeLightSelection(source,read), "candidate/output alias refuses transactionally");
  setup(); source.view = 16;
  Require(!PrepareNativeLightSelection(source,read), "invalid view refused");
  setup(); source.live_owner = UINT32_MAX-3;
  Require(!PrepareNativeLightSelection(source,read), "live owner address overflow refused");

  // Actual boundary chain: selector output -> authored descriptor publication ->
  // owned lights -> native GPU pass, after source storage is destroyed.
  setup(); words[1000+46816] = 1;
  const auto first = PrepareNativeLightSelection(source,read);
  Require(first.has_value(), "cold single-light selection");
  constexpr uint32_t publisher = 130000;
  for (uint32_t slot = 0; slot < 3; ++slot) {
    words[publisher+276+slot*4] = uint32_t(-2);
    for (uint32_t part = 0; part < 4; ++part) {
      const auto descriptor = publisher+36+part*60+slot*20;
      const auto buffer = 140000+part*100+slot*20;
      const auto data = 150000+part*256+slot*64;
      words[descriptor+4] = buffer; words[descriptor+12] = 0; words[descriptor+16] = 0;
      words[buffer+12] = data;
      for (uint32_t n = 0; n < 4; ++n) words[data+n*4] = 0;
    }
  }
  const auto overlay = [&](uint64_t address) -> std::optional<uint32_t> {
    for (const auto &write : first->writes) if (address == write.address) return write.after;
    return read(address);
  };
  const auto publication = PrepareSelectedLights(publisher,60000,3,1000+24016,1,2,{},overlay,
      [](double a) { return std::cos(a); });
  Require(publication && publication->state.known == 7 && publication->state.lights[0].kind == LitDirectional &&
          publication->state.lights[1].kind == LitDisabled, "first selection reaches the real publication consumer");
  const auto before_preview = words;
  const auto preview = [&] (const SelectedLightSourceState &prior = {}) -> std::optional<NativeSelectedLights> {
    const auto value = PrepareSelectedLights(publisher,60000,3,1000+24016,1,2,prior,overlay,
        [](double a) { return std::cos(a); });
    return value && value->state.known == 7 ? std::optional(value->state.lights) : std::nullopt;
  };
  const auto native = preview();
  Require(native && (*native)[0].kind == LitDirectional && words == before_preview,
      "legacy adapter preview uses proposed IDs without selection or shader writes");
  words[publisher+276] = uint32_t(first->selection.slots[0].id);
  Require(!preview(), "unchanged unknown slot cannot warm direct admission");
  Require(preview(publication->state).has_value(), "owned unchanged slot remains valid");
  words = before_preview;
  words.erase(publisher+36+4);
  const auto incomplete = words;
  Require(!preview() && words == incomplete, "missing descriptor refuses without partial source writes");
  words.clear();
  NativeRigidPassInputs pass;
  for (auto &fog : pass.fog) fog.disabled = true;
  pass.lights = publication->state.lights;
  Require(BuildRigidPass(pass).has_value(), "native pass owns first-selection lights after source retirement");
  std::cout << "native light selection: 128000 ranked candidates and source-to-pass boundaries passed\n";
}
