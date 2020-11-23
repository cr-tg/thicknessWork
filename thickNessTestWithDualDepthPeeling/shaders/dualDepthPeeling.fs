#version 460 core

layout(location = 0) out vec2 outDepth;

layout(binding = 0) uniform sampler2D uPreDepthMap;

uniform int uLayerIndex;
uniform vec4 uObjColor;

//bias Ϊ1e-4ʱ�����������ؿ�ĵ�23���㶼��127��Ϊ1e-5ʱ��ֻ��һ�����ؿ�Ϊ1e-6ʱ���ͻ���û����
#define bias 1e-6

void main()
{
	float currentDepth = gl_FragCoord.z;
	if(uLayerIndex == 0)
	{
		outDepth = vec2(-currentDepth,currentDepth);
		return;
	}

//	if(uLayerIndex == 1)
//	{
//		outDepth = vec2(-0.2f,0.3f);
//		return;
//	}

	vec2 texsize = textureSize(uPreDepthMap,0);
	vec2 texcoord = vec2(gl_FragCoord.xy/texsize.xy);
	vec2 preDepth = texture(uPreDepthMap,texcoord).rg;
	float preFrontDepth = -preDepth.x;
	float preBackDepth = preDepth.y;

	if(currentDepth < (preFrontDepth + bias) || currentDepth > (preBackDepth - bias))
	{
		discard;
	}

	outDepth = vec2(-currentDepth,currentDepth);
	return;

	//��¼��ΰ������� ��仰����outDepth��r����Ϊ0��ԭ��δ֪
//	if(currentDepth > (preFrontDepth + bias) && currentDepth < (preBackDepth - bias))
//	{
//		outDepth = vec2(-0.2f,0.3f);
//		//outDepth = vec2(-currentDepth,currentDepth);
//		return;
//	}
}
