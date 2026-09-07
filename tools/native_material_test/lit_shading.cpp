#include "gpu/scene/native_lit_shading.h"
#include "gpu/scene/native_rigid_inputs.h"
#include "gpu/scene/native_selected_lights_source.h"
#include "gpu/scene/native_fog_source.h"
#include <unordered_map>
#include <limits>
#include <array>
#include <iostream>
#include <stdexcept>
using namespace bd::gpu::scene;
namespace {
void Require(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
bool Near(float a, double b) { return std::isfinite(a) && std::abs(double(a)-b) <= 2e-5 * (1+std::abs(b)); }
using Vec = std::array<double, 3>;
Vec V(LitVector v) { return {v.x,v.y,v.z}; }
double Dot(Vec a, Vec b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
Vec Unit(Vec v) {
  const double length = std::sqrt(Dot(v,v));
  for (auto &x : v) x = length ? x / length : 0;
  return v;
}
double Saturate(double x) { return std::clamp(x,0.0,1.0); }
// Independent double-precision reference derived from the original material's
// three light blocks. No shader registers or native evaluator calls.
std::array<double,2> ReferenceLight(LitLight light, LitVector p, LitVector n, LitVector v, float power) {
  if (light.kind == LitDisabled) return {};
  Vec direction{};
  double attenuation = 1;
  for (size_t i=0;i<3;++i)
    direction[i] = light.kind == LitDirectional ? -V(light.direction)[i] : V(light.position)[i]-V(p)[i];
  if (light.kind != LitDirectional) {
    attenuation = 1-Saturate(light.inverse_range*std::sqrt(Dot(direction,direction)));
    direction = Unit(direction);
    if (light.kind == LitSpot)
      attenuation *= Saturate(light.cone_strength/(1.0-light.cone_cosine) *
                               Saturate(-Dot(direction,V(light.direction))-light.cone_cosine));
  }
  Vec half{};
  for (size_t i=0;i<3;++i) half[i]=direction[i]+V(v)[i];
  const double diffuse=std::max(0.0,Dot(V(n),direction));
  const double specular=std::pow(std::max(0.0,Dot(V(n),Unit(half))),power);
  return {diffuse*attenuation,specular*attenuation};
}
Vec ReferenceFog(Vec colour, LitVector p, LitVector camera, LitFog fog) {
  Vec delta{};
  for (size_t i=0;i<3;++i) delta[i]=V(p)[i]-V(fog.radial?camera:fog.origin)[i];
  const double distance=fog.radial?std::sqrt(Dot(delta,delta)):Dot(delta,V(fog.direction));
  const double amount=fog.disabled?0:Saturate((distance-fog.start)/(fog.end-fog.start));
  for(size_t i=0;i<3;++i) {
    const double target=V(fog.colour)[i]*amount, opacity=fog.opacity*amount;
    colour[i] += (fog.blend==LitFogBlend?target-colour[i]:fog.blend==LitFogAdd?target:-target)*opacity;
  }
  return colour;
}
}
void TestNativeLitShading() {
  {
    std::unordered_map<uint64_t, uint32_t> words;
    constexpr uint32_t owner = 1000;
    const auto setup = [&] {
      words.clear(); words[owner+8] = 0x01000000;
      for (uint32_t n = 0; n < 12; ++n) words[owner+12+n*4] = std::bit_cast<uint32_t>(float(n+1));
      words[owner+60] = 1; words[owner+64] = 0;
      for (uint32_t n = 0; n < 7; ++n) {
        const auto descriptor = owner+(n < 3 ? 96+n*20 : 156+(n-3)*16);
        words[descriptor+4] = 2000+n*32; words[descriptor+12] = 2;
        words[2000+n*32+12] = 4000+n*64;
        for (uint32_t part = 0; part < 16; part += 4) words[4000+n*64+(n < 3 ? 32 : 8)+part] = 0;
      }
    };
    const auto read = [&](uint64_t address) -> std::optional<uint32_t> {
      const auto it = words.find(address);
      return it == words.end() ? std::nullopt : std::optional(it->second);
    };
    const auto prepare = [&](const std::optional<LitFog> &previous = {}) {
      return PrepareNativeFog(owner, previous, read);
    };
    setup();
    const auto owned = prepare();
    Require(owned && owned->count == 16 && owned->fog && owned->fog->radial && !owned->fog->disabled &&
            owned->fog->start == 11 && owned->fog->end == 12 && owned->fog->opacity == 10,
            "authored fog produces owned named inputs and a bounded complete staging plan");
    for (size_t n = 0; n < owned->count; ++n) words[owned->writes[n].address] = owned->writes[n].word;
    Require(words[4032] == std::bit_cast<uint32_t>(1.f) && words[4044] == std::bit_cast<uint32_t>(11.f) &&
            words[4108] == std::bit_cast<uint32_t>(12.f) && words[4172] == std::bit_cast<uint32_t>(10.f) &&
            words[4200] == 0 && words[4264] == 1 && words[4328] == 1 && words[4392] == 0,
            "original float and boolean descriptor packing");
    for (uint32_t mode : {0u,1u,2u,UINT32_MAX}) for (uint32_t blend : {0u,1u,2u,UINT32_MAX}) {
      setup(); words[owner+60] = mode; words[owner+64] = blend;
      const auto result = prepare();
      Require(result && result->fog->disabled == (mode == 0) &&
              (mode == 0 ? result->fog->end == 0 && result->fog->opacity == 0 :
                result->fog->radial == (mode == 1) && result->fog->blend == (blend == 0 ? LitFogBlend : blend == 1 ? LitFogAdd : LitFogSubtract)),
              "disabled/radial/planar and blend/add/subtract authored modes");
    }
    words.clear(); words[owner+8] = 0;
    Require(prepare() && !prepare()->fog && !prepare()->count, "inactive unknown fog is not invented");
    auto inactive = prepare(owned->fog);
    Require(inactive && inactive->fog->start == 11 && !inactive->count,
            "inactive fog preserves the last publication without touching authored fields/descriptors");
    words.clear();
    NativeRigidPassInputs pass;
    pass.fog = {*owned->fog, *ComposeNativeFog({})};
    Require(BuildRigidPass(pass).has_value(), "fog pass packs after source destruction");
    setup(); words[owner+52] = std::bit_cast<uint32_t>(12.f); words[owner+56] = std::bit_cast<uint32_t>(11.f);
    const auto descending = prepare();
    Require(descending && descending->fog && descending->fog->start == 12 && descending->fog->end == 11,
            "descending authored fog range is valid, not a singular range");
    words.clear(); pass.fog[0] = *descending->fog;
    Require(BuildRigidPass(pass).has_value(), "native GPU pass preserves descending fog endpoints");
    const auto colour = LitVec(.8f,.5f,.1f), p = LitVec(0,0,11.5f), camera = LitVec(0,0,0);
    const auto actual = ApplyLitFog(colour,p,camera,*descending->fog);
    const auto expected = ReferenceFog(V(colour),p,camera,*descending->fog);
    Require(Near(actual.x,expected[0]) && Near(actual.y,expected[1]) && Near(actual.z,expected[2]),
            "descending fog falloff matches the independent reference");
    setup(); words[owner+52] = words[owner+56];
    Require(!prepare(), "singular active fog range refused");
    setup(); words[owner+36] = std::bit_cast<uint32_t>(std::numeric_limits<float>::infinity());
    Require(!prepare(), "nonfinite fog refused");
    setup(); words[2000+12] = owner+12-32;
    Require(!prepare(), "fog source/output aliases rejected before writes");
    setup(); words[2000+12] = owner+68-32; words[owner+68] = 1;
    Require(!prepare(), "fog destination cannot alias owner identity");
    setup(); words[2000+32+12] = words[2000+12];
    Require(!prepare(), "overlapping fog outputs refused");
    setup(); words.erase(owner+100);
    Require(!prepare(), "missing fog buffer descriptor refused");
    setup(); words[owner+108] = UINT32_MAX;
    Require(!prepare() && !PrepareNativeFog(UINT32_MAX-3, {}, read) && !PrepareNativeFog(0, {}, read),
            "fog address overflow and null owners refused");
  }
  {
    std::unordered_map<uint64_t, uint32_t> words;
    const uint32_t owner = 1000, selection = 2000, records = 4000;
    auto setup = [&] {
      words.clear();
      for (uint32_t slot = 0; slot < 3; ++slot) {
        words[selection+8+slot*4] = (slot == 2 ? 0xffffu : slot) << 16;
        words[owner+276+slot*4] = uint32_t(-2);
        for (uint32_t part = 0; part < 4; ++part) {
          const auto descriptor = owner+36+part*60+slot*20;
          const auto buffer = 8000+part*100+slot*20;
          const auto data = 10000+part*256+slot*64;
          words[descriptor+4] = buffer; words[descriptor+12] = 2;
          if (part == 3) words[descriptor+16] = slot;
          words[buffer+12] = data;
          for (uint32_t n = 0; n < 4; ++n) words[data+32+n*4] = 0x42c80000;
        }
        const uint64_t source = records+slot*76;
        words[source] = slot+1;
        for (uint32_t n = 20; n <= 72; n += 4) words[source+n] = std::bit_cast<uint32_t>(float(n));
        words[source+56] = std::bit_cast<uint32_t>(1.04719755f);
        words[source+60] = std::bit_cast<uint32_t>(3.0f);
        words[source+72] = std::bit_cast<uint32_t>(100.0f);
      }
    };
    const auto read = [&](uint64_t address) -> std::optional<uint32_t> {
      const auto it = words.find(address);
      return it == words.end() ? std::nullopt : std::optional(it->second);
    };
    auto prepare = [&](const SelectedLightSourceState &state = {}) {
      return PrepareSelectedLights(owner, selection, 0, records, 3, 2, state, read,
                                   [](double angle) { return std::cos(angle); });
    };
    setup();
    const auto publication = prepare();
    Require(publication && publication->count == 33 && publication->changed == 3 &&
            publication->state.known == 7 && publication->state.lights[0].kind == LitDirectional &&
            publication->state.lights[0].inverse_range == 0 &&
            publication->state.lights[1].kind == LitSpot && publication->state.lights[1].cone_strength == .5f &&
            Near(publication->state.lights[1].cone_cosine, .5) &&
            publication->state.lights[2].kind == LitDisabled, "authored three-light semantic production");
    for (size_t n = 0; n < publication->count; ++n) {
      const auto &write = publication->writes[n]; words[write.address] = write.word;
    }
    Require(words[10000+32+12] == std::bit_cast<uint32_t>(1.0f) &&
            words[10000+256+64+32+12] == std::bit_cast<uint32_t>(.5f) &&
            words[10000+512+64+32+12] == std::bit_cast<uint32_t>(.01f) &&
            words[10000+512+128+32] == 0 && words[10000+512+128+32+12] == 0x42c80000,
            "descriptor/lane packing and inactive partial-write semantics");
    auto same = prepare(publication->state);
    Require(same && !same->count && same->state.known == 7, "unchanged IDs reuse owned light values");
    Require(prepare()->state.known == 0, "unchanged unknown payload cannot be invented from source");
    words[records+44] = std::bit_cast<uint32_t>(999.0f);
    Require(prepare(publication->state)->state.lights[0].colour.x == 44,
            "unannounced source update does not replace an unchanged selected value");
    words[owner+276] = uint32_t(-2);
    Require(prepare(publication->state)->state.lights[0].colour.x == 999,
            "snapshot invalidation republishes the changed authored light");
    words.clear();
    std::optional<NativeNodeSelectedLights> node_lights = NativeNodeSelectedLights{64, publication->state.lights};
    const auto selected = SelectNativeObjectLights({}, node_lights, 64);
    Require(selected && !SelectNativeObjectLights({}, node_lights, 65),
            "node publication cannot be borrowed by a different primitive owner");
    node_lights.reset();
    Require(selected->at(1).cone_strength == .5f &&
            !SelectNativeObjectLights({}, node_lights, 64), "retired node publication leaves retained copies alive");
    Require(SelectNativeObjectLights(publication->state.lights, {}, 64).has_value(),
            "ordinary object defaults remain available without per-node overrides");
    NativeRigidPassInputs owned_pass;
    for (auto &fog : owned_pass.fog) fog.disabled = true;
    owned_pass.lights = publication->state.lights;
    Require(BuildRigidPass(owned_pass).has_value(), "native pass packs lights after source destruction");
    setup(); words[selection+8] = 3u << 16;
    Require(!prepare(), "out-of-range selected ID rejected before writes");
    setup(); words[records+76+72] = 0;
    Require(!prepare(), "singular active point/spot range refused");
    setup(); words[8000+12] = records+20-32;
    Require(!prepare(), "source/destination alias refused transactionally");
    setup(); words[8000+12] = owner+276-32;
    Require(!prepare(), "shader destination cannot alias selected identity cache");
    setup(); words[8000+100+12] = words[8000+12];
    Require(!prepare(), "overlapping output records refused");
    setup(); words.erase(owner+36+4);
    Require(!prepare(), "missing control descriptor refused");
    Require(!PrepareSelectedLights(owner,selection,16,records,3,2,{},read,[](double x){return std::cos(x);}) &&
            !PrepareSelectedLights(owner,selection,0,records,301,2,{},read,[](double x){return std::cos(x);}) &&
            !PrepareSelectedLights(UINT32_MAX-3,selection,0,records,3,2,{},read,[](double x){return std::cos(x);}),
            "view/count/address bounds refuse before mutation");
  }
  const RenderMatrix world{2,0,0,0, 0,4,0,0, 0,0,.5f,0, 3,5,7,1};
  const auto object = BuildRigidObject(world, {1,2,3,1}, {.1f,.2f,.3f,8}, {2,3,4,5}, RigidDiffuse);
  Require(object && object->normal_rows[0].x == .5f && object->normal_rows[1].y == .25f &&
          object->normal_rows[2].z == 2 && object->world.rows[3].z == 7, "rigid row-vector/normal packing");
  auto bad_world = world; bad_world[0] = 0;
  Require(!BuildRigidObject(bad_world,{1,1,1,1},{0,0,0,8},{1,1,0,0},0), "singular rigid matrix refused");
  bad_world = world; bad_world[3] = 1;
  Require(!BuildRigidObject(bad_world,{1,1,1,1},{0,0,0,8},{1,1,0,0},0), "projective object refused");
  Require(!BuildRigidObject(world,{1,1,1,1},{0,0,0,8},{1,1,0,0},64), "unknown rigid flags refused");
  const std::array<RigidFloat4,2> layer_uv{{{1,2,3,4},{5,6,7,8}}};
  for (uint32_t layers=0;layers<=3;++layers) {
    const auto layered = BuildRigidObject(world,{1,1,1,1},{0,0,0,8},{1,1,0,0},layers?RigidAlbedo:0,
        layer_uv,layers?layers-1:0);
    Require(layered && layered->flags.y == layers && layered->detail_uv_scale_offset[1].z == 7,
            "zero-to-three explicit layers and independent UVs in production GPU layout");
  }
  Require(!BuildRigidObject(world,{1,1,1,1},{0,0,0,8},{1,1,0,0},0,layer_uv,1) &&
          !BuildRigidObject(world,{1,1,1,1},{0,0,0,8},{1,1,0,0},RigidAlbedo,layer_uv,3),
          "detail layers require base enable and bounded count");
  auto invalid_uv = layer_uv; invalid_uv[1].w = std::numeric_limits<float>::quiet_NaN();
  Require(!BuildRigidObject(world,{1,1,1,1},{0,0,0,8},{1,1,0,0},RigidAlbedo,invalid_uv,2),
          "nonfinite secondary layer UV refused");
  NativeRigidPassInputs pass;
  pass.world_to_clip = {world, world}; pass.world_to_shadow = world;
  for (auto &fog : pass.fog) fog.disabled = true;
  pass.lights[0] = {LitVec(1,2,3),LitVec(4,5,6),LitVec(7,8,9),.25f,.5f,.75f,LitSpot};
  pass.fog[0] = {LitVec(1,2,3),LitVec(0,1,0),LitVec(.2f,.3f,.4f),5,10,.8f,false,true,LitFogSubtract};
  const auto packed = BuildRigidPass(pass);
  Require(packed && packed->lights[0].position_range.w == .25f && packed->lights[0].kind.x == LitSpot &&
          packed->fog[0].origin_start.w == 5 && packed->fog[0].mode.z == LitFogSubtract &&
          packed->fog[0].mode.y == 1 && packed->fog[0].mode.w == 0, "explicit light/fog packing");
  pass.fog[0].end = 5; Require(!BuildRigidPass(pass), "empty fog range refused"); pass.fog[0].end = 10;
  pass.lights[0].kind = 99; Require(!BuildRigidPass(pass), "unknown light refused"); pass.lights[0].kind = LitSpot;
  pass.lights[0].cone_cosine = 1; Require(!BuildRigidPass(pass), "singular cone refused"); pass.lights[0].cone_cosine = .5f;
  pass.cameras[1].x = std::numeric_limits<float>::quiet_NaN();
  Require(!BuildRigidPass(pass), "nonfinite second eye refused");
  LitLight light{};
  light.direction=LitVec(0,0,-1); light.position=LitVec(0,0,5);
  light.colour=LitVec(.4f,.6f,.8f); light.kind=LitDirectional;
  const auto origin=LitVec(0,0,0), facing=LitVec(0,0,1);
  auto result=EvaluateLitLight(light,origin,facing,facing,32);
  Require(Near(result.diffuse,1)&&Near(result.specular,1),"directional lighting");
  light.kind=LitPoint; light.inverse_range=.1f;
  result=EvaluateLitLight(light,origin,facing,facing,32);
  Require(Near(result.diffuse,.5)&&Near(result.specular,.5),"point attenuation");
  light.kind=LitSpot; light.cone_cosine=.5f; light.cone_strength=1;
  result=EvaluateLitLight(light,origin,facing,facing,32);
  Require(Near(result.diffuse,.5)&&Near(result.specular,.5),"spot cone attenuation");
  light.direction=LitVec(1,0,0);
  Require(EvaluateLitLight(light,origin,facing,facing,32).diffuse==0,"outside spot cone");
  light.kind=LitDisabled;
  Require(EvaluateLitLight(light,origin,facing,facing,0).specular==0,"disabled does not emit black-to-zero-power highlight");
  Require(LitShininess(0,0)==1&&LitShininess(-1,4)==0,"clamped log and zero shininess semantics");
  const auto zero=LitNormalize(origin);
  Require(zero.x==0&&zero.y==0&&zero.z==0,"zero vector remains finite");

  for(int i=0;i<1200;++i) {
    light.kind=i%4; light.position=LitVec(float(i%7)+1,float(i%11)-5,float(i%13)+2);
    light.direction=LitNormalize(LitVec(float(i%5)-2,.75f,float(i%3)-1));
    light.inverse_range=.001f*float(i%30); light.cone_cosine=.1f*float(i%9);
    light.cone_strength=.25f*float(i%7);
    const auto p=LitVec(.1f*float(i%17),-.2f,.4f), n=LitNormalize(LitVec(.2f,float(i%9)-4,1));
    const auto v=LitNormalize(LitVec(1,.3f,float(i%5)-2));
    const float power=float(i%33);
    result=EvaluateLitLight(light,p,n,v,power);
    const auto expected=ReferenceLight(light,p,n,v,power);
    Require(Near(result.diffuse,expected[0])&&Near(result.specular,expected[1]),"native light/reference matrix");
  }

  LitSurface surface{};
  surface.albedo=LitVec(.8f,.7f,.6f); surface.specular=LitVec(.2f,.3f,.4f);
  surface.ambient=LitVec(.1f,.2f,.3f); surface.shadow_colour=LitVec(.3f,.2f,.1f);
  surface.shadow_strength=.8f; surface.shadow_visibility=.25f;
  light.colour=LitVec(.4f,.6f,.8f);
  LitLight second=light, third=light; second.colour=LitVec(.3f,.7f,.2f); third.colour=LitVec(.8f,.1f,.3f);
  const LitResponse a{.5f,.25f},b{.2f,.3f},c{.1f,.5f};
  for(int flags=0;flags<4;++flags) {
    surface.diffuse_enabled=(flags&1)!=0; surface.specular_enabled=(flags&2)!=0;
    const auto actual=V(ComposeLitSurface(surface,light,second,third,a,b,c));
    for(size_t i=0;i<3;++i) {
      const double primary=V(light.colour)[i]*a.diffuse, shade=1-surface.shadow_visibility;
      const double subtraction=V(surface.shadow_colour)[i]*(shade+(primary*shade-shade)*surface.shadow_strength);
      double expected=V(surface.albedo)[i];
      if(surface.diffuse_enabled) expected*=primary+V(surface.ambient)[i]+V(second.colour)[i]*b.diffuse+V(third.colour)[i]*c.diffuse-subtraction;
      if(surface.specular_enabled) expected+=(a.specular*surface.shadow_visibility*V(light.colour)[i]+b.specular*V(second.colour)[i]+c.specular*V(third.colour)[i])*std::array<double,3>{1.05,.97,1.27}[i]*V(surface.specular)[i];
      Require(Near(float(actual[i]),expected),"coloured shadow subtraction and three-light highlight");
    }
  }
  LitFog fog{};
  fog.origin=origin; fog.direction=facing; fog.colour=LitVec(.2f,.4f,.7f);
  fog.start=1; fog.end=11; fog.opacity=.8f;
  const auto base=LitVec(.8f,.5f,.1f), camera=LitVec(0,0,-1);
  for(int mode=0;mode<12;++mode) {
    fog.blend=mode%3; fog.radial=(mode/3)%2; fog.disabled=mode>=6;
    for(int depth=-2;depth<15;++depth) {
      const auto p=LitVec(.3f,.4f,float(depth));
      const auto actual=ApplyLitFog(base,p,camera,fog);
      const auto expected=ReferenceFog(V(base),p,camera,fog);
      Require(Near(actual.x,expected[0])&&Near(actual.y,expected[1])&&Near(actual.z,expected[2]),"fog distance/mode reference matrix");
      const auto twice=ApplyLitFog(actual,p,camera,fog);
      const auto two_expected=ReferenceFog(expected,p,camera,fog);
      Require(Near(twice.x,two_expected[0])&&Near(twice.y,two_expected[1])&&Near(twice.z,two_expected[2]),"ordered two-layer fog");
    }
  }
  std::cout<<"native named lighting, coloured shadows and fog match independent references\n";
}
