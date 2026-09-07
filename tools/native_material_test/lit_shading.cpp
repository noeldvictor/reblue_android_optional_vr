#include "gpu/scene/native_lit_shading.h"
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
