#version 450
#extension GL_ARB_separate_shader_objects : enable

#define BLENDMODE_OPAQUE 0
#define BLENDMODE_MASK 1
#define BLENDMODE_BLEND 2

#define POSION 1
#define NORMAL 1 << 1
#define TANGENT 1 << 2
#define TEXCOORD 1 << 3

#define DIFFUSE_TEX 1
#define NORMAL_TEX 1 << 1
#define METALLICROUGHNESS_TEX 1 << 2
#define EMISSIVE_TEX 1 << 3
#define OCCLUSION_TEX 1 << 4
#define OCCLUSION_IN_METALLICROUGHNESS_TEX 1 << 5
#define DFG_TEX 1 << 8
#define IBL_TEX 1 << 9
layout(constant_id = 0) const int VTX_STATE = POSION | NORMAL | TANGENT | TEXCOORD;
layout(constant_id = 1) const int TEXTURE_STATE = 
    DIFFUSE_TEX | 
    NORMAL_TEX | 
    METALLICROUGHNESS_TEX | 
    EMISSIVE_TEX | 
    OCCLUSION_TEX | 
    OCCLUSION_IN_METALLICROUGHNESS_TEX | 
    DFG_TEX |
    IBL_TEX;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragTangent;
layout(location = 3) in vec3 fragBinormal;
layout(location = 4) in vec4 fragPos;

layout(binding = 0) uniform UniformBufferObject
{
		mat4 modelMtx;
		mat4 viewMtx;
		mat4 projMtx;
		vec4 cameraPos;
		vec4 baseColorFactor;
		vec4 metallicRoughness; // y : roughness, z : metallic
		vec4 blendMode; // x : blendMode, y : alphaCutOff
		vec4 emissiveFactor;
} ubo;

struct LightInfo
{
	vec4 lightPos; // xyz: lightPos
	vec4 lightDir; // xyz: lightDir
	vec4 lightColor; // xyz: color, w : intensity
	vec4 lightInfo;// x: range, y: innerAngleCos, z: outerAngleCos, w: lightType
};

layout(binding = 1) uniform lightInfosUniformBufferObject
{
	LightInfo lightInfos[16];
	vec4 lightCount;
} lightInfosUbo;

layout(binding = 2) uniform sampler2D diffuseSampler;
layout(binding = 3) uniform sampler2D normalSampler;
layout(binding = 4) uniform sampler2D MetallicRoughnessSampler;
layout(binding = 5) uniform sampler2D EmissiveSampler;
layout(binding = 6) uniform sampler2D OcclusionSampler;
layout(binding = 9) uniform sampler2D DfgSampler;
layout(binding = 10) uniform samplerCube IBLSampler;

layout(location = 0) out vec4 outColor;

#define PI                 3.14159265359
/** @public-api */
#define HALF_PI            1.570796327

#define FLT_EPS            1e-5
#define saturateMediump(x) x

#define saturate(x)        clamp(x, 0.0, 1.0)
float pow5(float x) {
    float x2 = x * x;
    return x2 * x2 * x;
}

#define MIN_N_DOT_V 1e-4

float clampNoV(float NoV) {
    // Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
    return max(NoV, MIN_N_DOT_V);
}

vec3 F_Schlick(const vec3 f0, float f90, float VoH) {
    // Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"
    return f0 + (f90 - f0) * pow5(1.0 - VoH);
}

vec3 F_Schlick(const vec3 f0, float VoH) {
    float f = pow(1.0 - VoH, 5.0);
    return f + f0 * (1.0 - f);
}

float F_Schlick(float f0, float f90, float VoH) {
    return f0 + (f90 - f0) * pow5(1.0 - VoH);
}

float D_GGX(float roughness, float NoH, const vec3 h) {
    // Walter et al. 2007, "Microfacet Models for Refraction through Rough Surfaces"

    // In mediump, there are two problems computing 1.0 - NoH^2
    // 1) 1.0 - NoH^2 suffers floating point cancellation when NoH^2 is close to 1 (highlights)
    // 2) NoH doesn't have enough precision around 1.0
    // Both problem can be fixed by computing 1-NoH^2 in highp and providing NoH in highp as well

    // However, we can do better using Lagrange's identity:
    //      ||a x b||^2 = ||a||^2 ||b||^2 - (a . b)^2
    // since N and H are unit vectors: ||N x H||^2 = 1.0 - NoH^2
    // This computes 1.0 - NoH^2 directly (which is close to zero in the highlights and has
    // enough precision).
    // Overall this yields better performance, keeping all computations in mediump
    float oneMinusNoHSquared = 1.0 - NoH * NoH;

    float a = NoH * roughness;
    float k = roughness / (oneMinusNoHSquared + a * a);
    float d = k * k * (1.0 / PI);
    return saturateMediump(d);
}

float distribution(float roughness, float NoH, const vec3 h) {
    return D_GGX(roughness, NoH, h);
}

float V_SmithGGXCorrelated(float roughness, float NoV, float NoL) {
    // Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
    float a2 = roughness * roughness;
    // TODO: lambdaV can be pre-computed for all the lights, it should be moved out of this function
    float lambdaV = NoL * sqrt((NoV - a2 * NoV) * NoV + a2);
    float lambdaL = NoV * sqrt((NoL - a2 * NoL) * NoL + a2);
    float v = 0.5 / (lambdaV + lambdaL);
    // a2=0 => v = 1 / 4*NoL*NoV   => min=1/4, max=+inf
    // a2=1 => v = 1 / 2*(NoL+NoV) => min=1/4, max=+inf
    // clamp to the maximum value representable in mediump
    return saturateMediump(v);
}

float visibility(float roughness, float NoV, float NoL) {
    return V_SmithGGXCorrelated(roughness, NoV, NoL);
    //return V_SmithGGXCorrelated_Fast(roughness, NoV, NoL);
}

vec3 fresnel(const vec3 f0, float LoH) {
    float f90 = saturate(dot(f0, vec3(50.0 * 0.33)));
    f90 = 1.0f;
    return F_Schlick(f0, f90, LoH);
}

vec3 isotropicLobe(float roughness, const vec3 f0, const vec3 h,
        float NoV, float NoL, float NoH, float LoH) 
{
    float D = distribution(roughness, NoH, h);
    float V = visibility(roughness, NoV, NoL);
    vec3  F = fresnel(f0, LoH);

    return (D * V) * F;
}

vec3 specularLobe(float roughness, const vec3 f0, const vec3 h,
        float NoV, float NoL, float NoH, float LoH) 
{
    return isotropicLobe(roughness, f0, h, NoV, NoL, NoH, LoH);
}

float Fd_Burley(float roughness, float NoV, float NoL, float LoH) {
    // Burley 2012, "Physically-Based Shading at Disney"
    float f90 = 0.5 + 2.0 * roughness * LoH * LoH;
    float lightScatter = F_Schlick(1.0, f90, NoL);
    float viewScatter  = F_Schlick(1.0, f90, NoV);
    return lightScatter * viewScatter * (1.0 / PI);
}

float Fd_Lambert() {
    return 1.0 / PI;
}

#define DIFFUSE_BURLEY 1
#define BRDF_DIFFUSE DIFFUSE_BURLEY
float diffuse(float roughness, float NoV, float NoL, float LoH) {
#if BRDF_DIFFUSE == DIFFUSE_LAMBERT
    return Fd_Lambert();
#elif BRDF_DIFFUSE == DIFFUSE_BURLEY
    return Fd_Burley(roughness, NoV, NoL, LoH);
#else
    return 1;
#endif
}

vec3 diffuseLobe(vec3 diffuseColor, float roughness, float NoV, float NoL, float LoH) 
{
    return diffuseColor * diffuse(roughness, NoV, NoL, LoH);
}

float computeDielectricF0(float reflectance) {
    return 0.16 * reflectance * reflectance;
}

vec3 computeF0(const vec3 baseColor, float metallic, float reflectance) {
    return baseColor.rgb * metallic + (reflectance * (1.0 - metallic));
}

vec3 PrefilteredDFG_LUT(float lod, float NoV) {
    // coord = sqrt(linear_roughness), which is the mapping used by cmgen.
    return textureLod(DfgSampler, vec2(NoV, lod), 0.0).rgb;
}

vec3 computeDiffuseColor(vec3 baseColor, float metallic) {
    return baseColor.rgb * (1.0 - metallic);
}

vec3 LightFunction(
    vec3 toLightDir, 
    vec3 toViewDir, 
    vec3 normal, 
    vec3 f0,
    vec3 diffuseColor, 
    float roughness, 
    float NoV, 
    float occlusion, 
    vec4 lightColorIntensity)
{
    vec3 h = normalize(toViewDir + toLightDir);

    float NoL = saturate(dot(toLightDir, normal));
    float NoH = saturate(dot(normal, h));
    float LoH = saturate(dot(toLightDir, h));

    vec3 Fr = specularLobe(roughness, f0, h, NoV, NoL, NoH, LoH);
    vec3 Fd = diffuseLobe(diffuseColor, roughness, NoV, NoL, LoH);

    Fr = max(Fr, 0.0);
    Fd = max(Fd, 0.0);

    vec3 color = Fd + Fr;// * energyCompensation;
    color = (color * lightColorIntensity.rgb) *
        (lightColorIntensity.w * NoL * occlusion);

    return color;
}

vec3 DirectionalLight(
    vec3 toLightDir, 
    vec3 toViewDir, 
    vec3 normal, 
    vec3 f0,
    vec3 diffuseColor, 
    float roughness, 
    float NoV, 
    float occlusion, 
    vec4 lightColorIntensity)
{
    vec3 color = 
    LightFunction(
    toLightDir, 
    toViewDir, 
    normal, 
    f0,
    diffuseColor, 
    roughness, 
    NoV, 
    occlusion, 
    lightColorIntensity);

    return color;
}

vec3 PointLight(
    vec3 toLightDir, 
    vec3 toViewDir, 
    vec3 normal, 
    vec3 f0,
    vec3 diffuseColor, 
    float roughness, 
    float NoV, 
    float occlusion, 
    vec4 lightColorIntensity,
    float range)
{
    float attenuation = 1.0;
    float toLightDistance = length(toLightDir);

    if (range > 0.0)
    {
        attenuation = 
            max(min(1.0 - pow(toLightDistance / range, 4.0), 1.0), 0.0) / pow(toLightDistance, 2.0);
    }
    else
    {
        // negative range means unlimited
        attenuation = 1.0f;
    }

    vec3 color = 
    LightFunction(
    toLightDir, 
    toViewDir, 
    normal, 
    f0,
    diffuseColor, 
    roughness, 
    NoV, 
    occlusion, 
    lightColorIntensity);
    
    return color * attenuation;
}

vec3 SpotLight(
    vec3 lightDir, 
    vec3 toLightDir, 
    vec3 toViewDir, 
    vec3 normal, 
    vec3 f0,
    vec3 diffuseColor, 
    float roughness, 
    float NoV, 
    float occlusion, 
    vec4 lightColorIntensity,
    float range,
    float innerAngleCos,
    float outerAngleCos)
{
    float attenuation = 1.0f;
    float toLightDistance = length(toLightDir);

    if (range > 0.0)
    {
        attenuation = 
            max(min(1.0 - pow(toLightDistance / range, 4.0), 1.0), 0.0) / pow(toLightDistance, 2.0);
    }
    else
    {
        // negative range means unlimited
        attenuation = 1.0f;
    }

    float spotAttenuation = 0.0f;
    float actualCos = dot(normalize(lightDir), normalize(-toLightDir));
    if (actualCos > outerAngleCos)
    {
        if (actualCos < innerAngleCos)
        {
            spotAttenuation = smoothstep(outerAngleCos, innerAngleCos, actualCos);
        }
        else
        {
            spotAttenuation = 1.0;
        }
    }

    vec3 color = 
    LightFunction(
    toLightDir, 
    toViewDir, 
    normal, 
    f0,
    diffuseColor, 
    roughness, 
    NoV, 
    occlusion, 
    lightColorIntensity);
    
    return color * attenuation * spotAttenuation;
}

vec3 toneMapUncharted2Impl(vec3 color)
{
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;
    return ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
}

void main()
{
    vec3 t = fragTangent;
    vec3 b = fragBinormal;

    if((VTX_STATE & TANGENT) != TANGENT)
	{
        vec3 pos_dx = dFdx(fragPos.xyz);
        vec3 pos_dy = dFdy(fragPos.xyz);
        vec3 tex_dx = dFdx(vec3(fragTexCoord, 0.0));
        vec3 tex_dy = dFdy(vec3(fragTexCoord, 0.0));
        t = (tex_dy.t * pos_dx - tex_dx.t * pos_dy) / (tex_dx.s * tex_dy.t - tex_dy.s * tex_dx.t);

        vec3 ng = normalize(fragNormal);

        t = normalize(t - ng * dot(ng, t));
        b = normalize(-cross(ng, t));
    }
    
	//vec3 binormal = cross(fragTangent, fragNormal);
	//binormal = normalize(binormal);
	mat3 TBN = mat3(t, b, fragNormal);


    vec3 normal = fragNormal;
    if((TEXTURE_STATE & NORMAL_TEX) == NORMAL_TEX)
    {
        vec4 normalTex = texture(normalSampler, fragTexCoord);

        vec3 sampledNormal = 2.0f * normalTex.xyz - 1.0f - 0.00392f;
        sampledNormal.y *= -1.0;
        //sampledNormal.x *= -1.0;

	    normal = TBN * sampledNormal;
    }
	
	normal = normalize(normal);

    //============================================================
	vec4 diffuseTex = texture(diffuseSampler, fragTexCoord);

    vec3 baseColor = ubo.baseColorFactor.rgb * diffuseTex.xyz;
    float matMetallic = ubo.metallicRoughness.x;
    float matRoughness = ubo.metallicRoughness.y;
    vec3 matEmissiveFactor = ubo.emissiveFactor.xyz;
    float matReflectance = 1.0f;

    float blendMode = ubo.blendMode.x;
    float alphaMaskValue = ubo.blendMode.y;

    float occlusion = 1.0f;

    float perceptualRoughness = matRoughness;
    float roughness = matRoughness * matRoughness;
    float metallic = matMetallic;
    if((TEXTURE_STATE & METALLICROUGHNESS_TEX) == METALLICROUGHNESS_TEX)
    {
        vec4 metallicRoughnessTex = texture(MetallicRoughnessSampler, fragTexCoord);
        perceptualRoughness = matRoughness * metallicRoughnessTex.y;
        roughness = perceptualRoughness * perceptualRoughness;

        metallic = matMetallic * metallicRoughnessTex.z;
        
        if((TEXTURE_STATE & OCCLUSION_IN_METALLICROUGHNESS_TEX) == OCCLUSION_IN_METALLICROUGHNESS_TEX)
        {
            occlusion = metallicRoughnessTex.x;
        }
    }
    if((TEXTURE_STATE & OCCLUSION_TEX) == OCCLUSION_TEX)
    {
        occlusion = texture(OcclusionSampler, fragTexCoord).x;
    }

    vec3 emissive = matEmissiveFactor;
    if((TEXTURE_STATE & EMISSIVE_TEX) == EMISSIVE_TEX)
    {
        emissive = emissive * texture(EmissiveSampler, fragTexCoord).xyz;
    }



    float reflectance = computeDielectricF0(matReflectance);

    vec3 diffuseColor = computeDiffuseColor(baseColor, metallic);
    vec3 specularColor = baseColor.rgb * metallic;

    vec3 toViewDir = normalize(ubo.cameraPos.xyz - fragPos.xyz);
    float NoV = clampNoV(dot(toViewDir, normal));
    vec3 f0 = computeF0(baseColor, metallic, reflectance);

    //====================================================================

    vec3 dfg = PrefilteredDFG_LUT(roughness, NoV);
    vec3 energyCompensation = 1.0 + f0 * (1.0 / dfg.y - 1.0);
    
    //==========================================================================

    vec3 iblColor = vec3(0);
    if((TEXTURE_STATE & IBL_TEX) == IBL_TEX)
    {
        vec3 viewReflect = reflect(-toViewDir, normal);
        vec3 E = vec3(1.0f);//specularDFG(pixel);
        E = mix(dfg.xxx, dfg.yyy, f0);
        const int MipCount = 10;
        //float lod = 9 * perceptualRoughness * (2.0 - perceptualRoughness);
        float lod = clamp(MipCount * roughness,  0.0, MipCount);

        iblColor = textureLod(IBLSampler, viewReflect, lod).xyz;
        //iblColor = E * iblColor;
        //iblColor = (dfg.x + dfg.y) * iblColor;

    }

    //====================================================================

    vec3 lightResult = vec3(0,0,0);
    for(int i = 0; i < lightInfosUbo.lightCount.x; i++)
    {
        LightInfo light = lightInfosUbo.lightInfos[i];
        vec4 lightColorIntensity = light.lightColor;
        
        vec3 color = vec3(0);
        if(light.lightInfo.w == 0)
        {
            vec3 toLightDir = -light.lightDir.xyz;
            color = DirectionalLight(
                toLightDir, 
                toViewDir, 
                normal, 
                f0,
                diffuseColor, 
                roughness, 
                NoV, 
                occlusion, 
                lightColorIntensity);
        }
        else if(light.lightInfo.w == 1)
        {
            vec3 toLightDir = light.lightPos.xyz - fragPos.xyz;
            float range = light.lightInfo.x;
            color = PointLight(
                toLightDir, 
                toViewDir, 
                normal, 
                f0,
                diffuseColor, 
                roughness, 
                NoV, 
                occlusion, 
                lightColorIntensity,
                range);
        }
        else if(light.lightInfo.w == 2)
        {
            vec3 toLightDir = light.lightPos.xyz - fragPos.xyz;
            vec3 lightDir = light.lightDir.xyz;
            float range = light.lightInfo.x;
            float innerAngleCos = light.lightInfo.y;
            float outerAngleCos = light.lightInfo.z;
            color = SpotLight(
                lightDir, 
                toLightDir, 
                toViewDir, 
                normal, 
                f0,
                diffuseColor, 
                roughness, 
                NoV, 
                occlusion, 
                lightColorIntensity,
                range,
                innerAngleCos,
                outerAngleCos);
        }
        else
        {
        }
        
        lightResult += color;
    }

    
    vec3 Fa = diffuseColor * 0.1f * occlusion;

    //lightResult +=diffuseColor * iblColor;
    //lightResult += specularColor * mix(dfg.xxx, dfg.yyy, f0) * iblColor;
    lightResult += (specularColor * dfg.xxx + dfg.yyy) * iblColor;

    lightResult += Fa;
    lightResult += emissive;

    lightResult = toneMapUncharted2Impl(lightResult);

	//outColor = vec4(Fd, diffuseTex.a);      
    //return;
    if(blendMode == BLENDMODE_BLEND)
    {   
        int xIn = int(gl_FragCoord.x) % 4;
        int yIn = int(gl_FragCoord.y) % 4;

        mat4 thresholdMatrix =
        {
            vec4(1.0 / 17.0, 9.0 / 17.0, 3.0 / 17.0, 11.0 / 17.0),
            vec4(13.0 / 17.0, 5.0 / 17.0, 15.0 / 17.0, 7.0 / 17.0),
            vec4(4.0 / 17.0, 12.0 / 17.0, 2.0 / 17.0, 10.0 / 17.0),
            vec4(16.0 / 17.0, 8.0 / 17.0, 14.0 / 17.0, 6.0 / 17.0)
        };
        
        float a = 0.5f;
        if(diffuseTex.a < thresholdMatrix[xIn][yIn])
        {
            discard;
        }
	    outColor = vec4(lightResult, 1.0f);
    }
    else if(blendMode == BLENDMODE_MASK )
    {
        if(diffuseTex.a < alphaMaskValue)
        {
            discard;
        }
        else
        {
            outColor = vec4(lightResult, 1.0f);
        }
    }
    else
    {
	    outColor = vec4(lightResult, 1.0f);
    }

	//outColor = vec4(specularColor, diffuseTex.a);
	//outColor = vec4(vec3(occlusion), diffuseTex.a);
	//outColor = vec4(vec3(fragTexCoord.x, fragTexCoord.y, 0.0), diffuseTex.a);
    //outColor = gl_FragCoord;
    //outColor.a = 1;
}