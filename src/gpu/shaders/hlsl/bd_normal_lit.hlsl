// bd_normal_ps (0xFB83DD3F5E67CEB7), the lit scene material, as a host
// shader substituted at link time (guest_shaders.cpp, bd_host_materials).
// The recompiled body (dump of 2026-09-03, hash above) with the host's
// shadow kernel: four gathers instead of thirty texture operations. The
// light/fog arithmetic now uses native_lit_shading.h's named, CPU-tested
// contract. Binding/register inputs and the texture/shadow front-end remain
// temporary adapters. Keep all material branches, including detail textures
// and reflections: a field census does not cover every authored material.

#include "thirdparty/XenosRecomp/XenosRecomp/shader_common.h"
#include "src/gpu/scene/native_lit_shading.h"


#ifdef __spirv__

#define ColorTexture_Texture2DDescriptorIndex BD_SHARED_U(0)
#define ColorTexture_Texture3DDescriptorIndex BD_SHARED_U(64)
#define ColorTexture_TextureCubeDescriptorIndex BD_SHARED_U(128)
#define ColorTexture_SamplerDescriptorIndex BD_SHARED_U(192)
#define ColorTexture1_Texture2DDescriptorIndex BD_SHARED_U(4)
#define ColorTexture1_Texture3DDescriptorIndex BD_SHARED_U(68)
#define ColorTexture1_TextureCubeDescriptorIndex BD_SHARED_U(132)
#define ColorTexture1_SamplerDescriptorIndex BD_SHARED_U(196)
#define ColorTexture2_Texture2DDescriptorIndex BD_SHARED_U(8)
#define ColorTexture2_Texture3DDescriptorIndex BD_SHARED_U(72)
#define ColorTexture2_TextureCubeDescriptorIndex BD_SHARED_U(136)
#define ColorTexture2_SamplerDescriptorIndex BD_SHARED_U(200)
#define CubeTexture_Texture2DDescriptorIndex BD_SHARED_U(20)
#define CubeTexture_Texture3DDescriptorIndex BD_SHARED_U(84)
#define CubeTexture_TextureCubeDescriptorIndex BD_SHARED_U(148)
#define CubeTexture_SamplerDescriptorIndex BD_SHARED_U(212)
#define NormalTexture_Texture2DDescriptorIndex BD_SHARED_U(16)
#define NormalTexture_Texture3DDescriptorIndex BD_SHARED_U(80)
#define NormalTexture_TextureCubeDescriptorIndex BD_SHARED_U(144)
#define NormalTexture_SamplerDescriptorIndex BD_SHARED_U(208)
#define ShadowTexture_Texture2DDescriptorIndex BD_SHARED_U(24)
#define ShadowTexture_Texture3DDescriptorIndex BD_SHARED_U(88)
#define ShadowTexture_TextureCubeDescriptorIndex BD_SHARED_U(152)
#define ShadowTexture_SamplerDescriptorIndex BD_SHARED_U(216)
#define g_vCameraPos g_PSC[1]
#define g_vColorK g_PSC[2]
#define g_MaterialTier BD_SHARED_U(348)
#define g_vFogColor1 g_PSC[34]
#define g_vFogColor2 g_PSC[37]
#define g_vFogDir1 g_PSC[32]
#define g_vFogDir2 g_PSC[35]
#define g_vFogPos1 g_PSC[33]
#define g_vFogPos2 g_PSC[36]
#define g_vLightAmbient g_PSC[0]
#define g_vLightDiffuse1 g_PSC[22]
#define g_vLightDiffuse2 g_PSC[26]
#define g_vLightDiffuse3 g_PSC[30]
#define g_vLightDir1 g_PSC[21]
#define g_vLightDir2 g_PSC[25]
#define g_vLightDir3 g_PSC[29]
#define g_vLightParam1 g_PSC[23]
#define g_vLightParam2 g_PSC[27]
#define g_vLightParam3 g_PSC[31]
#define g_vLightPos1 g_PSC[20]
#define g_vLightPos2 g_PSC[24]
#define g_vLightPos3 g_PSC[28]
#define g_vObjectDiffuse g_PSC[3]
#define g_vObjectRefFresnel g_PSC[6]
#define g_vObjectReflect g_PSC[5]
#define g_vObjectSpecular g_PSC[4]
#define g_vShadowEpsilon g_PSC[9]
#define g_vShadowSubColor g_PSC[7]
#define s3_Texture2DDescriptorIndex BD_SHARED_U(12)
#define s3_Texture3DDescriptorIndex BD_SHARED_U(76)
#define s3_TextureCubeDescriptorIndex BD_SHARED_U(140)
#define s3_SamplerDescriptorIndex BD_SHARED_U(204)
#define s7_Texture2DDescriptorIndex BD_SHARED_U(28)
#define s7_Texture3DDescriptorIndex BD_SHARED_U(92)
#define s7_TextureCubeDescriptorIndex BD_SHARED_U(156)
#define s7_SamplerDescriptorIndex BD_SHARED_U(220)
#define s8_Texture2DDescriptorIndex BD_SHARED_U(32)
#define s8_Texture3DDescriptorIndex BD_SHARED_U(96)
#define s8_TextureCubeDescriptorIndex BD_SHARED_U(160)
#define s8_SamplerDescriptorIndex BD_SHARED_U(224)
#define s9_Texture2DDescriptorIndex BD_SHARED_U(36)
#define s9_Texture3DDescriptorIndex BD_SHARED_U(100)
#define s9_TextureCubeDescriptorIndex BD_SHARED_U(164)
#define s9_SamplerDescriptorIndex BD_SHARED_U(228)
#define s10_Texture2DDescriptorIndex BD_SHARED_U(40)
#define s10_Texture3DDescriptorIndex BD_SHARED_U(104)
#define s10_TextureCubeDescriptorIndex BD_SHARED_U(168)
#define s10_SamplerDescriptorIndex BD_SHARED_U(232)
#define s11_Texture2DDescriptorIndex BD_SHARED_U(44)
#define s11_Texture3DDescriptorIndex BD_SHARED_U(108)
#define s11_TextureCubeDescriptorIndex BD_SHARED_U(172)
#define s11_SamplerDescriptorIndex BD_SHARED_U(236)
#define s12_Texture2DDescriptorIndex BD_SHARED_U(48)
#define s12_Texture3DDescriptorIndex BD_SHARED_U(112)
#define s12_TextureCubeDescriptorIndex BD_SHARED_U(176)
#define s12_SamplerDescriptorIndex BD_SHARED_U(240)
#define s13_Texture2DDescriptorIndex BD_SHARED_U(52)
#define s13_Texture3DDescriptorIndex BD_SHARED_U(116)
#define s13_TextureCubeDescriptorIndex BD_SHARED_U(180)
#define s13_SamplerDescriptorIndex BD_SHARED_U(244)
#define s14_Texture2DDescriptorIndex BD_SHARED_U(56)
#define s14_Texture3DDescriptorIndex BD_SHARED_U(120)
#define s14_TextureCubeDescriptorIndex BD_SHARED_U(184)
#define s14_SamplerDescriptorIndex BD_SHARED_U(248)
#define s15_Texture2DDescriptorIndex BD_SHARED_U(60)
#define s15_Texture3DDescriptorIndex BD_SHARED_U(124)
#define s15_TextureCubeDescriptorIndex BD_SHARED_U(188)
#define s15_SamplerDescriptorIndex BD_SHARED_U(252)

#else

cbuffer PixelShaderConstants : register(b1, space4)
{
	float4 g_vCameraPos : packoffset(c1);
	float4 g_vColorK : packoffset(c2);
	float4 g_vFogColor1 : packoffset(c34);
	float4 g_vFogColor2 : packoffset(c37);
	float4 g_vFogDir1 : packoffset(c32);
	float4 g_vFogDir2 : packoffset(c35);
	float4 g_vFogPos1 : packoffset(c33);
	float4 g_vFogPos2 : packoffset(c36);
	float4 g_vLightAmbient : packoffset(c0);
	float4 g_vLightDiffuse1 : packoffset(c22);
	float4 g_vLightDiffuse2 : packoffset(c26);
	float4 g_vLightDiffuse3 : packoffset(c30);
	float4 g_vLightDir1 : packoffset(c21);
	float4 g_vLightDir2 : packoffset(c25);
	float4 g_vLightDir3 : packoffset(c29);
	float4 g_vLightParam1 : packoffset(c23);
	float4 g_vLightParam2 : packoffset(c27);
	float4 g_vLightParam3 : packoffset(c31);
	float4 g_vLightPos1 : packoffset(c20);
	float4 g_vLightPos2 : packoffset(c24);
	float4 g_vLightPos3 : packoffset(c28);
	float4 g_vObjectDiffuse : packoffset(c3);
	float4 g_vObjectRefFresnel : packoffset(c6);
	float4 g_vObjectReflect : packoffset(c5);
	float4 g_vObjectSpecular : packoffset(c4);
	float4 g_vShadowEpsilon : packoffset(c9);
	float4 g_vShadowSubColor : packoffset(c7);
};

cbuffer SharedConstants : register(b2, space4)
{
	uint ColorTexture_Texture2DDescriptorIndex : packoffset(c0.x);
	uint ColorTexture_Texture3DDescriptorIndex : packoffset(c4.x);
	uint ColorTexture_TextureCubeDescriptorIndex : packoffset(c8.x);
	uint ColorTexture_SamplerDescriptorIndex : packoffset(c12.x);
	uint ColorTexture1_Texture2DDescriptorIndex : packoffset(c0.y);
	uint ColorTexture1_Texture3DDescriptorIndex : packoffset(c4.y);
	uint ColorTexture1_TextureCubeDescriptorIndex : packoffset(c8.y);
	uint ColorTexture1_SamplerDescriptorIndex : packoffset(c12.y);
	uint ColorTexture2_Texture2DDescriptorIndex : packoffset(c0.z);
	uint ColorTexture2_Texture3DDescriptorIndex : packoffset(c4.z);
	uint ColorTexture2_TextureCubeDescriptorIndex : packoffset(c8.z);
	uint ColorTexture2_SamplerDescriptorIndex : packoffset(c12.z);
	uint CubeTexture_Texture2DDescriptorIndex : packoffset(c1.y);
	uint CubeTexture_Texture3DDescriptorIndex : packoffset(c5.y);
	uint CubeTexture_TextureCubeDescriptorIndex : packoffset(c9.y);
	uint CubeTexture_SamplerDescriptorIndex : packoffset(c13.y);
	uint NormalTexture_Texture2DDescriptorIndex : packoffset(c1.x);
	uint NormalTexture_Texture3DDescriptorIndex : packoffset(c5.x);
	uint NormalTexture_TextureCubeDescriptorIndex : packoffset(c9.x);
	uint NormalTexture_SamplerDescriptorIndex : packoffset(c13.x);
	uint ShadowTexture_Texture2DDescriptorIndex : packoffset(c1.z);
	uint ShadowTexture_Texture3DDescriptorIndex : packoffset(c5.z);
	uint ShadowTexture_TextureCubeDescriptorIndex : packoffset(c9.z);
	uint ShadowTexture_SamplerDescriptorIndex : packoffset(c13.z);
	uint s3_Texture2DDescriptorIndex : packoffset(c0.w);
	uint s3_Texture3DDescriptorIndex : packoffset(c4.w);
	uint s3_TextureCubeDescriptorIndex : packoffset(c8.w);
	uint s3_SamplerDescriptorIndex : packoffset(c12.w);
	uint s7_Texture2DDescriptorIndex : packoffset(c1.w);
	uint s7_Texture3DDescriptorIndex : packoffset(c5.w);
	uint s7_TextureCubeDescriptorIndex : packoffset(c9.w);
	uint s7_SamplerDescriptorIndex : packoffset(c13.w);
	uint s8_Texture2DDescriptorIndex : packoffset(c2.x);
	uint s8_Texture3DDescriptorIndex : packoffset(c6.x);
	uint s8_TextureCubeDescriptorIndex : packoffset(c10.x);
	uint s8_SamplerDescriptorIndex : packoffset(c14.x);
	uint s9_Texture2DDescriptorIndex : packoffset(c2.y);
	uint s9_Texture3DDescriptorIndex : packoffset(c6.y);
	uint s9_TextureCubeDescriptorIndex : packoffset(c10.y);
	uint s9_SamplerDescriptorIndex : packoffset(c14.y);
	uint s10_Texture2DDescriptorIndex : packoffset(c2.z);
	uint s10_Texture3DDescriptorIndex : packoffset(c6.z);
	uint s10_TextureCubeDescriptorIndex : packoffset(c10.z);
	uint s10_SamplerDescriptorIndex : packoffset(c14.z);
	uint s11_Texture2DDescriptorIndex : packoffset(c2.w);
	uint s11_Texture3DDescriptorIndex : packoffset(c6.w);
	uint s11_TextureCubeDescriptorIndex : packoffset(c10.w);
	uint s11_SamplerDescriptorIndex : packoffset(c14.w);
	uint s12_Texture2DDescriptorIndex : packoffset(c3.x);
	uint s12_Texture3DDescriptorIndex : packoffset(c7.x);
	uint s12_TextureCubeDescriptorIndex : packoffset(c11.x);
	uint s12_SamplerDescriptorIndex : packoffset(c15.x);
	uint s13_Texture2DDescriptorIndex : packoffset(c3.y);
	uint s13_Texture3DDescriptorIndex : packoffset(c7.y);
	uint s13_TextureCubeDescriptorIndex : packoffset(c11.y);
	uint s13_SamplerDescriptorIndex : packoffset(c15.y);
	uint s14_Texture2DDescriptorIndex : packoffset(c3.z);
	uint s14_Texture3DDescriptorIndex : packoffset(c7.z);
	uint s14_TextureCubeDescriptorIndex : packoffset(c11.z);
	uint s14_SamplerDescriptorIndex : packoffset(c15.z);
	uint s15_Texture2DDescriptorIndex : packoffset(c3.w);
	uint s15_Texture3DDescriptorIndex : packoffset(c7.w);
	uint s15_TextureCubeDescriptorIndex : packoffset(c11.w);
	uint s15_SamplerDescriptorIndex : packoffset(c15.w);
	DEFINE_SHARED_CONSTANTS();
};

#endif
	#define g_bDebug0 BOOL_BIT(137)
	#define g_bDebug1 BOOL_BIT(138)
	#define g_bDiffuse BOOL_BIT(135)
	#define g_bEnvMap BOOL_BIT(132)
	#define g_bFog BOOL_BIT(134)
	#define g_bFogMode1 BOOL_BIT(148)
	#define g_bFogMode2 BOOL_BIT(152)
	#define g_bNMap BOOL_BIT(131)
	#define g_bShadowMap BOOL_BIT(133)
	#define g_bSpecular BOOL_BIT(136)
	#define g_bTexture0 BOOL_BIT(128)
	#define g_bTexture1 BOOL_BIT(129)
	#define g_bTexture2 BOOL_BIT(130)

LitLight ImportLitLight(float4 position, float4 direction, float4 colour, float4 parameters) {
	LitLight light;
	light.position = LitVec(position.x, position.y, position.z);
	light.direction = LitVec(direction.x, direction.y, direction.z);
	light.colour = LitVec(colour.x, colour.y, colour.z);
	light.inverse_range = colour.w;
	light.cone_cosine = parameters.x;
	light.cone_strength = direction.w;
	light.kind = position.w < .5 ? LitDisabled : position.w < 1.5 ? LitDirectional :
		position.w < 2.5 ? LitSpot : LitPoint;
	return light;
}
LitFog ImportLitFog(float4 position, float4 direction, float4 colour,
                   bool disabled, bool radial, bool blend, bool add) {
	LitFog fog;
	fog.origin = LitVec(position.x, position.y, position.z);
	fog.direction = LitVec(direction.x, direction.y, direction.z);
	fog.colour = LitVec(colour.x, colour.y, colour.z);
	fog.start = direction.w; fog.end = position.w; fog.opacity = colour.w;
	fog.disabled = disabled; fog.radial = radial;
	fog.blend = blend ? LitFogBlend : add ? LitFogAdd : LitFogSubtract;
	return fog;
}

#ifndef __spirv__
[shader("pixel")]
#endif
void main(
	in float4 iPos : SV_Position,
	in float4 iTexCoord0 : TEXCOORD0,
	in float4 iTexCoord1 : TEXCOORD1,
	in float4 iTexCoord2 : TEXCOORD2,
	in float4 iTexCoord3 : TEXCOORD3,
	in float4 iTexCoord4 : TEXCOORD4,
	in float4 iTexCoord5 : TEXCOORD5,
	in float4 iTexCoord6 : TEXCOORD6,
	in float4 iTexCoord7 : TEXCOORD7,
	in float4 iTexCoord8 : TEXCOORD8,
	in float4 iTexCoord9 : TEXCOORD9,
	in float4 iTexCoord10 : TEXCOORD10,
	in float4 iTexCoord11 : TEXCOORD11,
	in float4 iTexCoord12 : TEXCOORD12,
	in float4 iTexCoord13 : TEXCOORD13,
	in float4 iTexCoord14 : TEXCOORD14,
	in float4 iTexCoord15 : TEXCOORD15,
	in centroid float4 iColor0 : COLOR0,
	in centroid float4 iColor1 : COLOR1,
#ifdef __spirv__
	in bool iFace : SV_IsFrontFace
#else
	in uint iFace : SV_IsFrontFace
#endif
#ifdef __spirv__
	,in uint iViewID : SV_ViewID
#endif
,
	out float4 oC0 : SV_Target0)
{
#ifdef __spirv__
	g_ViewIndex = iViewID;
#endif
	float4 c248 = float4(asfloat(0x0u), asfloat(0x0u), asfloat(0x0u), asfloat(0x0u));
	float4 c249 = float4(asfloat(0x0u), asfloat(0x0u), asfloat(0x0u), asfloat(0x0u));
	float4 c250 = float4(asfloat(0x40200000u), asfloat(0x3F000000u), (0.5 + (asfloat(0x3F00547Bu) - 0.5) * g_ShadowPcfScale), asfloat(0x3E2AAAABu));
	float4 c251 = float4(asfloat(0x40000000u), asfloat(0x3ECCCCCDu), asfloat(0xBF800000u), (0.5 + (asfloat(0x3F002A3Du) - 0.5) * g_ShadowPcfScale));
	float4 c252 = float4(asfloat(0x3F800000u), asfloat(0x0u), (asfloat(0x3A800000u) * g_ShadowPcfScale), (0.5 + (asfloat(0x3EFF570Au) - 0.5) * g_ShadowPcfScale));
	float4 c253 = float4(asfloat(0x3F866666u), asfloat(0x3F7851ECu), asfloat(0x3FA28F5Cu), asfloat(0x3FC00000u));
	float4 c254 = float4((1 + (asfloat(0x3F7FC000u) - 1) * g_ShadowPcfScale), (1 + (asfloat(0x3F7FD5C3u) - 1) * g_ShadowPcfScale), (1 + (asfloat(0x3F800A8Fu) - 1) * g_ShadowPcfScale), (1 + (asfloat(0x3F804000u) - 1) * g_ShadowPcfScale));
	float4 c255 = float4((0.5 + (asfloat(0x3EFFAB85u) - 0.5) * g_ShadowPcfScale), asfloat(0x0u), asfloat(0x0u), asfloat(0x0u));

	float4 r0 = iTexCoord0;
	float4 r1 = iTexCoord1;
	float4 r2 = iTexCoord2;
	float4 r3 = iTexCoord3;
	float4 r4 = iTexCoord4;
	float4 r5 = iTexCoord5;
	float4 r6 = iTexCoord6;
	float4 r7 = iColor0;
	float4 r8 = 0.0;
	float4 r9 = 0.0;
	float4 r10 = 0.0;
	float4 r11 = 0.0;
	float4 r12 = 0.0;
	float4 r13 = 0.0;
	float4 r14 = 0.0;
	float4 r15 = 0.0;
	float4 r16 = 0.0;
	float4 r17 = 0.0;
	float4 r18 = 0.0;
	float4 r19 = 0.0;
	float4 r20 = 0.0;
	float4 r21 = 0.0;
	float4 r22 = 0.0;
	float4 r23 = 0.0;
	float4 r24 = 0.0;
	float4 r25 = 0.0;
	float4 r26 = 0.0;
	float4 r27 = 0.0;
	float4 r28 = 0.0;
	float4 r29 = 0.0;
	float4 r30 = 0.0;
	float4 r31 = 0.0;
	int a0 = 0;
	int aL = 0;
	bool p0 = false;
	float ps = 0.0;
	CubeMapData cubeMapData = (CubeMapData)0;
	float2 shadowTapUV[8] = (float2[8])0;

	r4.w = dot(r4.zxy, r4.zxy);
	r9.xyz = -r2.xyz + g_vCameraPos.xyz;
	r8.x = dot(r3.zxy, r3.zxy);
	r3.w = dot(r9.zxy, r9.zxy);
	ps = clamp(rsqrt(abs(r8.x)), FLT_MIN, FLT_MAX);
	r8.x = ps;
	r8.xyz = r8.xxx * r3.zyx;
	ps = clamp(rsqrt(abs(r4.w)), FLT_MIN, FLT_MAX);
	r3.x = ps;
	r4.xyz = r3.xxx * r4.zyx;
	ps = clamp(rsqrt(abs(r3.w)), FLT_MIN, FLT_MAX);
	r3.x = ps;
	r9.xyz = r9.zyx * r3.xxx;
	if (g_bNMap)
	{
		// Tier bit 1: past eight texels a pixel the normal map's detail is the
		// average of its mip, near a flat tangent-space normal (at two texels a
		// pixel the cliffs visibly flattened, 2026-09-04); the fetch is skipped
		// and (1, 0.5, 0.5) stands in for it (r10 = r3.yzx * 2 - 1 = +Z).
		// The footprint is the UV derivative against a 1024 map (the world
		// textures' size, host_mips.cpp); ALU only, the ALUs sit at 21%.
		float2 nmap_d = max(abs(ddx(r1.zw)), abs(ddy(r1.zw)));
		bool nmap_far = (g_MaterialTier & 1u) != 0u && max(nmap_d.x, nmap_d.y) > (8.0 / 1024.0);
		[branch] if (nmap_far)
			r3.xyz = float3(1.0, 0.5, 0.5);
		else
			r3.xyz = tfetch2D(NormalTexture_Texture2DDescriptorIndex, NormalTexture_SamplerDescriptorIndex, r1.zw, float2(0, 0)).xyz;
		r10.xyz = r3.yzx * c251.xxx + c251.zzz;
		r3.xy = r8.xx * r4.zy;
		r11.xyzw = r8.yyzz * r4.xzyx;
		r3.z = r11.x + -r3.y;
		ps = max(r3.x, r3.x);
		r3.x = r11.z + -r11.y;
		ps = -r11.w + ps;
		r3.y = ps;
	}
	if (g_bNMap)
	{
		r3.xyz = r10.xxx * r3.xyz;
		r3.xyz = r4.zxy * r10.zzz + r3.zxy;
		r3.yzw = r8.yzx * r10.yyy + r3.zxy;
		r3.x = dot(r3.wzy, r3.wzy);
		ps = clamp(rsqrt(abs(r3.x)), FLT_MIN, FLT_MAX);
		r3.x = ps;
		r8.xyz = r3.wyz * r3.xxx;
	}
	if (!g_bTexture0)
	{
		r10.xyzw = max(c252.xxxx, c252.xxxx);
	}
	else
	{
		ps = -abs(r7.x) > 0.0;
		r3.x = ps;
		if (g_bEnvMap)
		{
			r3.x = r9.x * r8.x;
			r3.y = -r9.x * r8.x;
			r10.x = r9.z * r8.z;
			r3.z = r9.y * r8.y;
			ps = max(-r9.z, -r9.z);
			r3.w = -r9.y * r8.y;
			ps = r8.z * ps;
			r10.y = ps;
			r3.zw = r10.xy + r3.zw;
			r10.xy = r3.zw + r3.xy;
			ps = r10.y + r10.y;
			r1.z = ps;
			r3.xyz = -r1.zzz * r8.yxz + -r9.yxz;
			r3.xyzw = cube(r3.zxyy, cubeMapData);
			r11.z = max(r3.w, r3.w);
			ps = clamp(rcp(abs(r3.z)), FLT_MIN, FLT_MAX);
			r1.z = ps;
			r11.xy = r3.yx * r1.zz + c253.ww;
			r3.xyzw = tfetchCube(CubeTexture_TextureCubeDescriptorIndex, CubeTexture_SamplerDescriptorIndex, r11.xyz, cubeMapData).xyzw;
			r4.w = -g_vObjectRefFresnel.x + c252.x;
			r1.z = max(r10.x, c252.y);
			ps = c252.x - r1.z;
			r1.z = ps;
			r10.xyzw = r3.xyzw * g_vObjectReflect.xyzw;
			ps = clamp(log2(abs(r1.z)), FLT_MIN, FLT_MAX);
			r1.z = ps;
			ps = g_vObjectRefFresnel.y * r1.z;
			r3.w = ps;
			r3.xyz = r10.xyz * g_vObjectRefFresnel.zzz;
			ps = exp2(r3.w);
			r3.w = ps;
			r3.w = r4.w * r3.w + g_vObjectRefFresnel.x;
			r3.w = max(r3.w, c252.y);
			r3.w = r10.w * r3.w;
		}
		else
		{
			r3.yzw = max(r3.xxx, r3.xxx);
		}
		r10.xyzw = tfetch2D(ColorTexture_Texture2DDescriptorIndex, ColorTexture_SamplerDescriptorIndex, r0.xy, float2(0, 0)).xyzw;
		r10.xyzw = select(-r0.xxxx > 0.0, r3.wzyx, r10.wzyx);
		if (g_bTexture1)
		{
			r11.xyzw = tfetch2D(ColorTexture1_Texture2DDescriptorIndex, ColorTexture1_SamplerDescriptorIndex, r0.zw, float2(0, 0)).xyzw;
			r0.xyzw = select(-r0.zzzz > 0.0, r3.xyzw, r11.xyzw);
			r11.xyz = -r10.ywz + r0.zxy;
			r10.yzw = r11.xzy * r0.www + r10.yzw;
			if (g_bTexture2)
			{
				r0.xyzw = tfetch2D(ColorTexture2_Texture2DDescriptorIndex, ColorTexture2_SamplerDescriptorIndex, r1.xy, float2(0, 0)).xyzw;
				r0.xyzw = select(-r1.xxxx > 0.0, r3.xyzw, r0.xyzw);
				r1.xyz = -r10.ywz + r0.zxy;
				r10.yzw = r1.xzy * r0.www + r10.yzw;
			}
		}
	}
	r0.xyzw = r10.xyzw * g_vObjectDiffuse.wzyx;
	r1.xyzw = r0.wzyx * r7.xyzw;
	ps = max(c252.x, c252.x);
	r7.y = ps;
	if (g_bShadowMap)
	{
		// The host shadow kernel (2026-09-03). The guest's was six depth fetches
		// and six four-load compares, thirty texture operations a fragment, on
		// taps spread +-1.3/1024 of the map times g_ShadowPcfScale (the host
		// holds that penumbra constant in world space, constant_buffers.h).
		// Four GatherRed calls of the D32 map at the corners of a quad half that
		// wide, each a bilinear compare, cover the same penumbra with sixteen
		// texels for four fetches. The projection (uv from the second set, v
		// flipped as D3D does), the depth-proportional and slope-scaled biases
		// and the "outside the map is lit" rule are the recompiled ones; r7.y
		// leaves this block as the lit fraction, which the diffuse block consumes.
		ps = clamp(rcp(r5.w), FLT_MIN, FLT_MAX);
		r7.x = ps;
		r3.yzw = r7.xxx * r5.zyx;
		r7.y = saturate(dot(r8.xzy, -g_vLightDir1.zxy));
		ps = clamp(rcp(r6.w), FLT_MIN, FLT_MAX);
		r7.x = ps;
		r0.zw = r6.yx * c250.yy * r7.xx;
		r3.x = c252.x - r7.y;
		r5.xyzw = r3.xyzw * g_vShadowEpsilon.xxwz;
		float shadow_ref = r7.x * r6.z - r5.y - r5.x * c251.y;
		float2 shadow_uv = float2(c250.y + r0.w, c250.y - r0.z);
		BD_TEX2D shadow_tex = g_Texture2DDescriptorHeap[ShadowTexture_Texture2DDescriptorIndex];
		float2 shadow_dim = float2(getTexture2DDimensions(shadow_tex));
		float shadow_o = 0.65 * c252.z; // c252.z is (1/1024) * g_ShadowPcfScale
		r7.y = 0.0;
		// Tier bit 2: where the shadow map is minified (its texel under a
		// screen pixel) the four-gather penumbra is narrower than a pixel and
		// one bilinear gather at the centre reads the same; three fetches
		// fewer on the far ground.
		float2 shadow_d = max(abs(ddx(shadow_uv)), abs(ddy(shadow_uv))) * shadow_dim;
		bool shadow_far = (g_MaterialTier & 2u) != 0u && max(shadow_d.x, shadow_d.y) > 1.0;
		[branch] if (shadow_far)
		{
			float4 shadow_taps = shadow_tex.GatherRed(g_SamplerDescriptorHeap[ShadowTexture_SamplerDescriptorIndex], BD_UV(shadow_uv));
			float4 shadow_lit = select(shadow_taps > shadow_ref.xxxx, float4(1.0, 1.0, 1.0, 1.0), float4(0.0, 0.0, 0.0, 0.0));
			float2 shadow_f = frac(shadow_uv * shadow_dim - 0.5);
			r7.y = lerp(lerp(shadow_lit.w, shadow_lit.z, shadow_f.x), lerp(shadow_lit.x, shadow_lit.y, shadow_f.x), shadow_f.y);
		}
		else
		{
		[unroll] for (int shadow_i = 0; shadow_i < 4; ++shadow_i)
		{
			float2 tap_uv = shadow_uv + float2((shadow_i & 1) ? shadow_o : -shadow_o, (shadow_i & 2) ? shadow_o : -shadow_o);
			float4 shadow_taps = shadow_tex.GatherRed(g_SamplerDescriptorHeap[ShadowTexture_SamplerDescriptorIndex], BD_UV(tap_uv));
			float4 shadow_lit = select(shadow_taps > shadow_ref.xxxx, float4(1.0, 1.0, 1.0, 1.0), float4(0.0, 0.0, 0.0, 0.0));
			float2 shadow_f = frac(tap_uv * shadow_dim - 0.5);
			r7.y += 0.25 * lerp(lerp(shadow_lit.w, shadow_lit.z, shadow_f.x), lerp(shadow_lit.x, shadow_lit.y, shadow_f.x), shadow_f.y);
		}
		}
		if (any(shadow_uv < 0.0) || any(shadow_uv > 1.0))
			r7.y = c252.x;
	}
	// Named material arithmetic; this file remains the temporary binding ABI.
	const LitVector native_position = LitVec(r2.x, r2.y, r2.z);
	const LitVector native_normal = LitVec(r8.z, r8.y, r8.x);
	const LitVector native_view = LitVec(r9.z, r9.y, r9.x);
	const LitVector native_camera = LitVec(g_vCameraPos.x, g_vCameraPos.y, g_vCameraPos.z);
	const LitLight light0 = ImportLitLight(g_vLightPos1, g_vLightDir1, g_vLightDiffuse1, g_vLightParam1);
	const LitLight light1 = ImportLitLight(g_vLightPos2, g_vLightDir2, g_vLightDiffuse2, g_vLightParam2);
	const LitLight light2 = ImportLitLight(g_vLightPos3, g_vLightDir3, g_vLightDiffuse3, g_vLightParam3);
	const LitResponse response0 = EvaluateLitLight(light0, native_position, native_normal, native_view, g_vObjectSpecular.w);
	const LitResponse response1 = EvaluateLitLight(light1, native_position, native_normal, native_view, g_vObjectSpecular.w);
	const LitResponse response2 = EvaluateLitLight(light2, native_position, native_normal, native_view, g_vObjectSpecular.w);
	LitSurface surface;
	surface.albedo = LitVec(r1.x, r1.y, r1.z);
	surface.specular = LitVec(g_vObjectSpecular.x, g_vObjectSpecular.y, g_vObjectSpecular.z);
	surface.ambient = LitVec(g_vLightAmbient.x, g_vLightAmbient.y, g_vLightAmbient.z);
	surface.shadow_colour = LitVec(g_vShadowSubColor.x, g_vShadowSubColor.y, g_vShadowSubColor.z);
	surface.shadow_strength = g_vShadowSubColor.w;
	surface.shadow_visibility = r7.y;
	surface.diffuse_enabled = g_bDiffuse;
	surface.specular_enabled = g_bSpecular;
	LitVector colour = ComposeLitSurface(surface, light0, light1, light2, response0, response1, response2);
	if (g_bFog) {
		colour = ApplyLitFog(colour, native_position, native_camera,
			ImportLitFog(g_vFogPos1, g_vFogDir1, g_vFogColor1, g_bFogMode1,
				BOOL_BIT(149), BOOL_BIT(150), BOOL_BIT(151)));
		colour = ApplyLitFog(colour, native_position, native_camera,
			ImportLitFog(g_vFogPos2, g_vFogDir2, g_vFogColor2, g_bFogMode2,
				BOOL_BIT(153), BOOL_BIT(154), BOOL_BIT(155)));
	}
	r1.xyz = float3(colour.x, colour.y, colour.z);
	r7.xyz = r1.xyz + g_vColorK.xyz;
	r1.xyz = r7.zyx * g_vColorK.www;
	if (g_bDebug0)
	{
		if (g_bDebug1)
		{
			r1.z = dot(r8.xzy, g_vLightDir1.zxy);
			ps = max(-r1.z, -r1.z);
			r1.y = ps;
		}
		if (!g_bDebug1)
		{
			r1.z = dot(r4.xzy, g_vLightDir1.zxy);
			ps = max(-r1.z, -r1.z);
			r1.y = ps;
		}
		r1.xw = max(c252.yx, c252.yx);
	}
	oC0.xyzw = max(r1.zyxw, r1.zyxw);
	[branch] if (g_SpecConstants() & SPEC_CONSTANT_CEL)		oC0.xyz = BD_CelBand(oC0.xyz);
	[branch] if (g_SpecConstants() & SPEC_CONSTANT_ALPHA_TEST)	{		clip(BD_AlphaPass(BD_AlphaMode(g_SpecConstants()), oC0.w, g_AlphaThreshold) ? 1.0 : -1.0);
	}	return;
}
