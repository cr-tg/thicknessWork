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
	uint orderIndex = atomicCounterIncrement(uPixelOrderCounter);//orderIndex��1��ʼ
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
	vec2 texcoords = vec2((orderIndex & (MAX_VALUE - 1)) / float(MAX_VALUE), 0.5f);//����д�������1
	vec4 result = texture(uColorOrderMap,texcoords);
	return result;
}

//256*256*256��ʮ����Ϊ16777216
//counter ��ֵ�Ǵ�0-256*256*255�ģ�>>(0,8,16)֮���ֵ����(256*256*255,256*255,255)��ʮ�����·ֱ����(16711680;65280;255)
//֮������0*0*255�����������ô���Ľ������(255,255,255)��ֻ������3��255�ֱ���*1��*256��*256*256�ġ�
