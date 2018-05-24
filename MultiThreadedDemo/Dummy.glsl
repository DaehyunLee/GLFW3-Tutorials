#version 430
//#extension GL_ARB_shader_storage_buffer_object : enable
#extension GL_ARB_separate_shader_objects : enable
#extension    GL_ARB_shader_image_load_store : enable
#extension GL_ARB_shader_atomic_counters : enable
#extension GL_ARB_explicit_uniform_location : enable

layout (local_size_x = 1, local_size_y = 1) in;
//const
layout(location = 3) uniform mat3x3 K;
layout(location = 4) uniform mat3x3 D;

layout(std430, binding = 0) buffer InputData { float inputData[]; };
layout( std430, binding=1 ) buffer CenterMatch { vec4 outData[]; };

void main () {
	uint index = gl_GlobalInvocationID.z * gl_NumWorkGroups.x * gl_NumWorkGroups.y +
		gl_GlobalInvocationID.y * gl_NumWorkGroups.x +
		gl_GlobalInvocationID.x;

	vec3 val = vec3(gl_GlobalInvocationID.x, gl_GlobalInvocationID.y, gl_GlobalInvocationID.z);
	vec3 ret = val*K;

	//outData[index] = vec4( ret.xyz, index);
	//outData[index] = inputData[index];
	outData[index].x = inputData[index];
	outData[index].y = inputData[4];
	outData[index].w = index;
}
