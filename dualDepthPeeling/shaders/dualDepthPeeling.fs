#version 460 core

layout(location = 0) out vec2 outDepth;
layout(location = 1) out vec4 outFrontColor;
layout(location = 2) out vec4 outBackColor;

layout(binding = 0) uniform sampler2D uPreDepthMap;
uniform vec4 uObjColor;

#define bias 1e-3

void main()
{
	float currentDepth = gl_FragCoord.z;
	vec2 texsize = textureSize(uPreDepthMap,0);
	vec2 texcoord = vec2(gl_FragCoord.xy/texsize.xy);
	vec2 preDepth = texture(uPreDepthMap,texcoord).rg;
	float preFrontDepth = -preDepth.x;
	float preBackDepth = preDepth.y;

	//outBackColor = vec4(preBackDepth,preFrontDepth,0.0f,1.0f);
//	outFrontColor = vec4(0.0f,1.0f,0.0f,1.0f);
//
	if(currentDepth < (preFrontDepth - bias) || currentDepth > (preBackDepth + bias))
	{
		discard;
	}

	//记录这次剥离的深度
	if(currentDepth > (preFrontDepth + bias) && currentDepth < (preBackDepth - bias))
	{
		outDepth = vec2(-currentDepth,currentDepth);
		return;
	}
	//记录上次剥离的颜色
	if(currentDepth > (preFrontDepth - bias) && currentDepth < (preFrontDepth + bias))
	{
		outFrontColor = uObjColor;
		//outFrontColor = vec4(0.0f,1.0f,0.0f,1.0f);
	}
	else
	{
		//outBackColor = vec4(1.0f,0.0f,0.0f,1.0f);
		outBackColor = uObjColor;
	}

}
