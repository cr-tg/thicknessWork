#version 460 core

layout(location = 0) out vec2 outDepth;

layout(binding = 0) uniform sampler2D uPreDepthMap;

uniform int uLayerIndex;
uniform vec4 uObjColor;

//bias 为1e-4时，有三个像素宽的第23两层都是127，为1e-5时，只有一个像素宽，为1e-6时，就基本没有了
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

	//记录这次剥离的深度 这句话会让outDepth的r分量为0，原因未知
//	if(currentDepth > (preFrontDepth + bias) && currentDepth < (preBackDepth - bias))
//	{
//		outDepth = vec2(-0.2f,0.3f);
//		//outDepth = vec2(-currentDepth,currentDepth);
//		return;
//	}
}
