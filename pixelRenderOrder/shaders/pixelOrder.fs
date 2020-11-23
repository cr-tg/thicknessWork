#version 460 core
#extension GL_ARB_fragment_shader_interlock : require
layout(binding = 0, offset = 0) uniform atomic_uint uPixelOrderCounter;

layout(binding = 0) uniform sampler2D uColorOrderMap;

uniform int uAllPixelNum;
out vec4 FragColor;

vec4 getColorMap(uint orderIndex);
vec4 getIndexColor(uint orderIndex);

void main()
{
	//beginInvocationInterlockARB();
	uint orderIndex = atomicCounterIncrement(uPixelOrderCounter);//orderIndex从1开始
	FragColor = getColorMap(orderIndex);
	//endInvocationInterlockARB();
}

vec4 getIndexColor(uint orderIndex)
{
	vec4 result = vec4(uvec3((orderIndex>>uvec3(0,8,16)) & 0xFF)/255.0f,1.0f);
	if(result.z > 0.0)
	{
		result.xy = vec2(0);
		result.z *= 10;
	}
	else if(result.z == 0.0 && result.y > 0.0)
	{
		result.x = 0.0;
	}
	return result;
}

vec4 getColorMap(uint orderIndex)
{
	uint MAX_VALUE = uAllPixelNum;
//	uint MAX_VALUE = uAllPixelNum/2;
	vec2 texcoords = vec2((orderIndex & (MAX_VALUE - 1)) / float(MAX_VALUE), 0.5f);//这样写避免大于1
	vec4 result = texture(uColorOrderMap,texcoords);
	return result;
}

//256*256*256的十进制为16777216
//counter 的值是从0-256*256*255的，>>(0,8,16)之后的值就是(256*256*255,256*255,255)。十进制下分别就是(16711680;65280;255)
//之后再与0*0*255做与操作，那么最后的结果就是(255,255,255)，只不过这3个255分别是*1，*256，*256*256的。
