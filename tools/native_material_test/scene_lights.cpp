#include "gpu/scene/native_scene_lights_source.h"
#include "gpu/scene/native_selected_lights_source.h"
#include "gpu/scene/native_rigid_inputs.h"
#include "gpu/scene/deferred_work.h"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <unordered_map>
using namespace bd::gpu::scene;
namespace {
void Require(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
}
void TestNativeSceneLights() {
  constexpr uint32_t manager = 1000, visual = 60000, other = 80000;
  constexpr NativeLightScoreParameters scoring{255.f,1.f,-.00001f,.00001f};
  std::unordered_map<uint64_t, uint32_t> words;
  const auto read = [&](uint64_t address) -> std::optional<uint32_t> {
    const auto it = words.find(address);
    return it == words.end() ? std::nullopt : std::optional(it->second);
  };
  const auto scalar = [&](uint64_t address, float value) { words[address] = std::bit_cast<uint32_t>(value); };
  words[manager] = 2; words[manager+46816] = 3;
  words[manager+48032] = ~0u;
  // Deliberately no primary-thread priority, dirty mask, selected slots, changed
  // list, live category owner, shader cached IDs or parameter descriptors exist.
  for (uint32_t n = 0; n < 3; ++n) {
    const uint64_t light = manager+24016+n*76;
    for (uint32_t offset = 0; offset < 76; offset += 4) words[light+offset] = 0;
    words[light] = uint32_t(n == 0 ? LitDirectional : n == 1 ? LitPoint : LitSpot) | 16;
    words[light+8] = 1; words[light+12] = n;
    scalar(light+40,1.f); scalar(light+44,.25f+float(n)*.25f);
    scalar(light+48,.5f); scalar(light+52,.75f);
    scalar(light+56,.5f); scalar(light+60,1.f); scalar(light+64,1.f); scalar(light+72,10.f);
  }
  const auto object = [&](uint32_t selection, uint32_t kind, float z) {
    words[selection] = kind; scalar(selection+200,0.f); scalar(selection+204,0.f);
    scalar(selection+208,z); scalar(selection+212,2.f);
  };
  words[visual+3380] = 0;
  object(visual+3132,0,8.f);
  auto input = ReadNativeObjectLightInputs(visual,0,read);
  auto lights = ReadNativeSceneLightSet(manager,false,scoring,2.f,read);
  Require(input && lights, "semantic handoff reads no legacy selection/cache fields");
  auto selection = SelectNativeSceneLights(*lights,*input,0);
  Require(selection && selection->lights[0].kind == LitDirectional && selection->lights[1].kind == LitPoint &&
      selection->lights[2].kind == LitSpot && selection->lights[2].cone_strength == 1.f,
      "directional/point/spot authored order and semantic composition");
  Require(SelectNativeSceneLights(*lights,*input,3)->lights[0].kind == LitDisabled,
      "render view three cannot be substituted for light-selection view zero");
  Require(!SelectNativeSceneLights(*lights,*input,16), "invalid light-selection view refuses");

  NativeSceneLightingPublication publication;
  std::vector<NativeNodeLightBinding> bindings{{11,93,0,*input}};
  Require(publication.Publish(7,*lights,bindings), "first coherent publication");
  const auto old_update = publication.Update(7);
  const auto old_packet = publication.Select(7,old_update,11,93,0,0);
  Require(old_packet && !publication.Select(8,old_update,11,93,0,0) &&
      !publication.Select(7,old_update,12,93,0,0) && !publication.Select(7,old_update,11,94,0,0) &&
      !publication.Select(7,old_update,11,93,1,0), "frame/instance/model/node identities cannot borrow lights");

  {
    NativeSceneLightingPublication ordered;
    auto authored = *lights;
    std::vector<NativeNodeLightBinding> nodes{{11,93,20,*input},{11,93,3,std::nullopt},
        {12,94,0,std::nullopt}};
    Require(ordered.Publish(7,authored,nodes), "ordered bindings import without inventing traversal order");
    auto next = [&] (uint32_t node, uint32_t view = 0) {
      return ordered.Prepare(7,ordered.Update(7),11,93,node,view);
    };
    Require(!next(3), "initial Keep has no guessed dark/default seed");
    const auto delayed_keep = ordered.Capture(7,ordered.Update(7),11,93,3,0);
    const auto delayed_bind = ordered.Capture(7,ordered.Update(7),11,93,20,0);
    const auto delayed_dark = ordered.Capture(7,ordered.Update(7),11,93,20,3);
    Require(delayed_keep && !delayed_keep->bind && delayed_bind && delayed_bind->bind &&
        delayed_dark && !ordered.Resolve(7,*delayed_keep),
        "capture an authored Keep without prematurely resolving inherited values");
    // Submission is Keep, Bind, dark Bind. Final sorted order is dark, Bind,
    // Keep. A producer-time ticket or inherited value would give the wrong light.
    const std::array recipes{*delayed_keep,*delayed_bind,*delayed_dark};
    std::array order{DeferredSortItem{1,0},DeferredSortItem{2,1},DeferredSortItem{3,2}};
    Require(OrderDeferredWork(order), "native deferred light order");
    std::array<NativeSceneLightTicket,3> emitted;
    for (size_t n=0;n<order.size();++n) {
      const auto ticket = ordered.Resolve(7,recipes[order[n].payload]);
      Require(ticket && ordered.Commit(7,*ticket), "resolve and commit at final draw position");
      emitted[n] = *ticket;
    }
    Require(emitted[0].lights[0].kind == LitDisabled && emitted[2].inherited &&
        emitted[2].lights[0].colour.x == .25f &&
        SameNativeSelectedLights(emitted[1].lights,emitted[2].lights),
        "sorted Keep consumes the immediately preceding bound values");
    Require(!ordered.Commit(7,emitted[0]) && !ordered.Resolve(8,*delayed_bind),
        "captured actions do not authorize stale tickets or another frame");
    ordered.InvalidateInherited();
    Require(!ordered.Resolve(7,*delayed_keep) && ordered.Resolve(7,*delayed_bind),
        "unowned late writer invalidates Keep without invalidating owned explicit values");
    const auto prepared = next(20);
    Require(prepared && !prepared->inherited && !next(3), "preflight/culled or suppressed work does not bind lights");
    Require(ordered.Commit(7,*prepared), "actual bound draw commits copied native values");
    auto keep = next(3,3);
    Require(keep && keep->inherited && SameNativeSelectedLights(keep->lights,prepared->lights),
        "Keep follows actual draw order, not node order or a new light-view selection");
    Require(!ordered.Commit(7,*prepared), "one ticket cannot be committed twice");
    const auto cross_object = ordered.Prepare(7,ordered.Update(7),12,94,0,0);
    Require(cross_object && cross_object->inherited, "copied global light values may cross object boundaries");
    Require(!ordered.Prepare(7,ordered.Update(7),12,93,0,0), "missing native identity is not an authored Keep");
    const auto changed = next(20,3); // no lights in view 3
    Require(changed && ordered.Commit(7,*changed) && !ordered.Commit(7,*keep),
        "intervening bind invalidates speculative Keep ticket");
    Require(next(3)->lights[0].kind == LitDisabled, "known disabled selection can be inherited");
    Require(ordered.Commit(7,*next(20)), "restore actual lit draw");
    std::array<uint32_t,48> shader_words{};
    for (uint32_t slot = 0; slot < 3; ++slot) {
      const auto values = NativeSelectedLightWords(prepared->lights[slot]);
      for (uint32_t n = 0; n < 16; ++n) shader_words[slot*16+n] = std::bit_cast<uint32_t>(values[n]);
    }
    ObserveNativeLightParameterWrite(ordered,false,20,12,shader_words.data());
    Require(next(3).has_value(), "matching compatibility flush preserves known native values");
    shader_words[15] = std::bit_cast<uint32_t>(99.f);
    ObserveNativeLightParameterWrite(ordered,false,20,12,shader_words.data());
    Require(next(3).has_value(), "unused cone lanes cannot corrupt semantic inheritance");
    shader_words[8] = std::bit_cast<uint32_t>(.99f);
    ObserveNativeLightParameterWrite(ordered,false,20,12,shader_words.data());
    Require(!next(3), "unowned changed shader write breaks the chain, never becomes its input");
    Require(ordered.Commit(7,*next(20)), "owned bind re-establishes chain after unowned writer");
    ObserveNativeLightParameterWrite(ordered,true,20,12,nullptr);
    ObserveNativeLightParameterWrite(ordered,false,3,1,nullptr);
    Require(next(3).has_value(), "unrelated vertex and material writes do not invalidate lighting");
    ObserveNativeLightParameterWrite(ordered,false,21,1,nullptr);
    Require(!next(3), "unknown relevant write invalidates without a register read");
    Require(ordered.Commit(7,*next(20)), "native bind restores chain");
    keep = next(3);
    authored.lights[0].value->colour.x = .8f;
    Require(ordered.Publish(8,authored,{{13,207,0,std::nullopt},{13,207,1,*input}}),
        "next handoff retires all previous source/model keys");
    Require(!ordered.Resolve(8,*delayed_bind) && !ordered.Resolve(8,*delayed_keep) &&
        delayed_bind->bind->at(0).colour.x == .25f,
        "handoff invalidates queued action eligibility without changing copied values");
    const auto inherited = ordered.Prepare(8,ordered.Update(8),13,207,0,0);
    Require(inherited && inherited->lights[0].colour.x == .25f &&
        !ordered.Commit(8,*keep), "handoff retains copied values, not stale ticket eligibility");
    const auto fresh = ordered.Prepare(8,ordered.Update(8),13,207,1,0);
    Require(fresh && fresh->lights[0].colour.x == .8f && ordered.Commit(8,*fresh),
        "next explicit bind consumes changed authored values even when IDs did not change");
    authored.mode = 3;
    Require(!ordered.Publish(9,authored,{{13,207,0,std::nullopt}}), "invalid producer breaks inherited chain");
    authored.mode = 2;
    Require(ordered.Publish(9,authored,{{13,207,0,std::nullopt}}) &&
        !ordered.Prepare(9,ordered.Update(9),13,207,0,0), "recovery cannot borrow pre-failure values");
    NativeRigidPassInputs retained;
    for (auto &fog : retained.fog) fog.disabled = true;
    retained.lights = inherited->lights;
    Require(BuildRigidPass(retained).has_value(), "inherited packet reaches GPU packing after chain/source retirement");
  }

  words[visual+3380] = 1; words[visual+3376] = 90000;
  words[90000] = other; words[90004] = 0;
  object(other,1,8.f);
  input = ReadNativeObjectLightInputs(visual,0,read);
  Require(input && input->object_class == 1 && !ReadNativeObjectLightInputs(visual,1,read),
      "per-node override replaces default; null does not borrow default spatial inputs");
  const auto inherited_source = ReadNativeObjectLightSource(visual,1,read);
  Require(inherited_source && !inherited_source->inputs && !inherited_source->selection,
      "null entry imports explicit Keep without source identity");
  words.erase(90004);
  Require(!ReadNativeObjectLightSource(visual,1,read), "unreadable entry is missing, not Keep");
  words[90004] = other+232; object(other+232,2,30.f);
  const auto second = ReadNativeObjectLightInputs(visual,1,read);
  Require(second && second->object_class == 2 && second->centre.z == 30.f, "distinct per-node spatial inputs");
  words[manager+24016+4] = 2; // Exclude object class 1 from directional light.
  scalar(manager+24016+76+44,.9f); // Same IDs, changed authored colour at the next handoff.
  lights = ReadNativeSceneLightSet(manager,false,scoring,2.f,read);
  bindings = {{11,93,0,*input},{11,93,1,*second}};
  Require(lights && publication.Publish(7,*lights,bindings), "late changes replace same-frame publication");
  const auto update = publication.Update(7);
  Require(update > old_update && !publication.Select(7,old_update,11,93,0,0), "stale pass update rejected within same frame");
  auto packet = publication.Select(7,update,11,93,0,0);
  Require(packet && packet->selection.slots[0].id == 1 && packet->lights[0].colour.x == .9f &&
      old_packet->lights[1].colour.x == .5f, "new source values with unchanged IDs; old packets stay immutable");
  Require(publication.Select(7,update,11,93,1,0)->lights[0].kind == LitDirectional,
      "another node uses its own class and distant spatial inputs");
  words.clear(); lights.reset(); bindings.clear(); input.reset();
  packet = publication.Select(7,update,11,93,0,0);
  Require(packet && packet->lights[0].colour.x == .9f, "consume after all source storage is destroyed");
  publication.Reset();
  Require(!publication.Select(7,update,11,93,0,0), "reset removes current eligibility");
  NativeRigidPassInputs gpu;
  for (auto &fog : gpu.fog) fog.disabled = true;
  gpu.lights = packet->lights;
  Require(BuildRigidPass(gpu).has_value(), "queued owned lights reach production GPU packing after retirement");

  NativeSceneLightSet scene;
  scene.count = 2; scene.mode = 2; scene.scoring = scoring;
  for (int n = 0; n < 2; ++n) {
    auto &light = scene.lights[n];
    light.candidate.id = n; light.candidate.kind = LitDirectional; light.candidate.enabled = true;
    light.candidate.views = 0xffff; light.candidate.intensity = n ? .25f : 1.f;
    LitLight semantic{}; semantic.kind = LitDirectional; light.value = semantic;
  }
  NativeObjectLightInputs ordinary{{},2.f,1};
  scene.lights[1].candidate.priority = true;
  Require(SelectNativeSceneLights(scene,ordinary,0)->selection.slots[0].id == 1, "priority before brightness");
  scene.priority_light = 0;
  Require(SelectNativeSceneLights(scene,ordinary,0)->selection.slots[0].id == 0, "designated render priority first");
  scene.lights[0].candidate.excluded_objects = 2;
  Require(SelectNativeSceneLights(scene,ordinary,0)->selection.slots[0].id == 1, "exclusion beats designated priority");
  scene.special_scene = true;
  Require(SelectNativeSceneLights(scene,ordinary,0)->selection.slots[0].id == 0 &&
      SelectNativeSceneLights(scene,ordinary,8)->selection.slots[0].id == 1, "special-scene exemption and view-eight exception");
  scene.mode = 1;
  Require(SelectNativeSceneLights(scene,ordinary,0)->selection.slots[0].id == 1, "directional-only authored priority behavior");
  scene.mode = 0;
  Require(SelectNativeSceneLights(scene,ordinary,0)->lights[0].kind == LitDisabled, "disabled mode is canonical dark");
  scene.mode = 3;
  Require(!SelectNativeSceneLights(scene,ordinary,0), "unsupported hold/cache mode is not invented native data");
  scene.mode = 2;
  bindings = {{12,207,0,ordinary}};
  Require(publication.Publish(8,scene,bindings) && !publication.Select(8,publication.Update(8),11,93,0,0),
      "reload native identities cannot alias retired source storage");
  scene.lights[0].candidate.id = 1;
  Require(!publication.Publish(8,scene,bindings) && !publication.Update(8), "ambiguous authored ID refuses and invalidates");
  scene.lights[0].candidate.id = 0;
  bindings.push_back(bindings.front());
  Require(!publication.Publish(8,scene,bindings), "duplicate native node binding refuses");
  bindings.resize(publication.kMaxBindings+1);
  Require(!publication.Publish(8,scene,std::move(bindings)), "aggregate node cap rejects before publication");
  scene.lights[0].value.reset();
  Require(!SelectNativeSceneLights(scene,ordinary,0), "selected invalid semantic light refuses");
  scene.lights[0].candidate.enabled = false;
  Require(SelectNativeSceneLights(scene,ordinary,0).has_value(), "unselected invalid light does not poison another object");

  words[visual+3380] = 0; object(visual+3132,32,8.f);
  Require(!ReadNativeObjectLightInputs(visual,0,read), "invalid class rejected at import");
  object(visual+3132,1,8.f); scalar(visual+3132+212,-1.f);
  Require(!ReadNativeObjectLightInputs(visual,0,read), "negative spatial radius rejected");
  scalar(visual+3132+212,1.f); scalar(visual+3132+200,std::numeric_limits<float>::quiet_NaN());
  Require(!ReadNativeObjectLightInputs(visual,0,read), "nonfinite spatial input rejected");
  Require(!ReadNativeObjectLightInputs(UINT32_MAX-3,0,read) &&
      !ReadNativeSceneLightSet(UINT32_MAX-3,false,scoring,2.f,read), "source address overflow rejected");

  {
    constexpr uint32_t owner = 1000, buffer = 5000, data = 6000;
    words.clear();
    words[buffer+12] = data;
    for (uint32_t n = 0; n < 48; ++n) words[data+n*4] = 0;
    for (uint32_t slot = 0; slot < 3; ++slot) {
      for (uint32_t vector = 0; vector < 4; ++vector) {
        const auto descriptor = owner+36+vector*60+slot*20;
        words[descriptor+4] = buffer; words[descriptor+12] = slot*4+vector;
        words[descriptor+16] = 0;
      }
      words[owner+276+slot*4] = 123;
    }
    words[owner+288] = 123; words[owner+292] = 456;
    const auto source_before = words;
    const auto mirror = PrepareNativeLightMirror(owner,old_packet->lights,read);
    Require(mirror && mirror->count == 44 && words == source_before,
        "native-to-compatibility mirror preflights all three slots and caches with no side effects");
    for (size_t n = 0; n < mirror->count; ++n) words[mirror->writes[n].address] = mirror->writes[n].word;
    std::array<uint32_t,48> exported{};
    for (uint32_t n = 0; n < 48; ++n) exported[n] = words[data+n*4];
    Require(MatchesNativeLightParameterWrite(old_packet->lights,20,exported),
        "next legacy inherited draw receives the exact native semantic light values");
    Require(words[owner+276] == uint32_t(-2) && words[owner+280] == uint32_t(-2) &&
        words[owner+284] == uint32_t(-2) && !words[owner+288] && words[owner+292] == ~0u,
        "later explicit legacy node cannot skip publication using pre-native caches");
    words = source_before;
    words[owner+36+12] = words[owner+96+12];
    Require(!PrepareNativeLightMirror(owner,old_packet->lights,read), "aliased vector destinations refuse before stores");
    words = source_before; words[buffer+12] = owner+36;
    Require(!PrepareNativeLightMirror(owner,old_packet->lights,read), "descriptor/control alias refuses");
    words = source_before; words[owner+216+16] = 4;
    Require(!PrepareNativeLightMirror(owner,old_packet->lights,read), "invalid scalar lane refuses");
    words = source_before; words.erase(data+188);
    Require(PrepareNativeLightMirror(owner,old_packet->lights,read).has_value(),
        "unwritten unused cone lanes need no destination");
    words.erase(data+176);
    Require(!PrepareNativeLightMirror(owner,old_packet->lights,read), "missing written destination refuses");
    Require(!PrepareNativeLightMirror(UINT32_MAX-3,old_packet->lights,read), "mirror address overflow refuses");
  }
  std::cout << "native scene lighting: coherent import, selection, late updates, node/reload lifetime and GPU packing passed\n";
}
