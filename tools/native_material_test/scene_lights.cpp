#include "gpu/scene/native_scene_lights_source.h"
#include "gpu/scene/native_rigid_inputs.h"
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

  words[visual+3380] = 1; words[visual+3376] = 90000;
  words[90000] = other; words[90004] = 0;
  object(other,1,8.f);
  input = ReadNativeObjectLightInputs(visual,0,read);
  Require(input && input->object_class == 1 && !ReadNativeObjectLightInputs(visual,1,read),
      "per-node override replaces default; null inheritance refuses rather than borrowing");
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
  std::cout << "native scene lighting: coherent import, selection, late updates, node/reload lifetime and GPU packing passed\n";
}
