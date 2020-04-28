#version 450
#extension GL_ARB_separate_shader_objects : enable

#define POSION 1
#define NORMAL 1 << 1
#define TANGENT 1 << 2
#define TEXCOORD 1 << 3
layout(constant_id = 0) const int VTX_STATE = (POSION | NORMAL | TANGENT | TEXCOORD);

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec2 inUv;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragTangent;
layout(location = 3) out vec3 fragBinormal;
layout(location = 4) out vec4 fragPos;

layout(binding = 0) uniform UniformBufferObject
{
		mat4 modelMtx;
		mat4 viewMtx;
		mat4 projMtx;
} ubo;

void main()
{
    gl_Position = ubo.projMtx * ubo.viewMtx * ubo.modelMtx * vec4(inPos, 1.0);
	fragTexCoord = inUv;

	fragPos = ubo.modelMtx * vec4(inPos, 1.0);

	vec4 normal = ubo.modelMtx * vec4(inNormal, 0.0);
	fragNormal = normalize(normal.xyz);

  	//vec3 tangent = -vec3(abs(inNormal.y) + abs(inNormal.z), abs(inNormal.x), 0);
  	//vec3 binormal = cross(tangent, inNormal);
  	//fragTangent = normalize(ubo.modelMtx * vec4(cross(inNormal, binormal), 0.0)).xyz;
  	//fragBinormal = normalize(ubo.modelMtx * vec4(binormal, 0.0)).xyz;

	if((VTX_STATE & TANGENT) == TANGENT)
	{
		//HAS TANGENT
		vec4 tangent = ubo.modelMtx * vec4(inTangent.xyz, 0.0);
		fragTangent = normalize(tangent.xyz);

		vec3 binormal = cross(fragTangent, fragNormal) * inTangent.w;
		//vec3 binormal = cross(inTangent.xyz, inNormal.xyz);
		fragBinormal = normalize(binormal);//(ubo.modelMtx * vec4(binormal, 0.0)).xyz;
	}

} 