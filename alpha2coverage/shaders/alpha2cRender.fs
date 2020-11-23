#version 460 core
layout(binding = 0) uniform sampler2D uDepthMap;
uniform vec4 uObjColor;

out vec4 FragColor;

void main()
{
	float currentDepth = gl_FragCoord.z;
	vec2 texSize = textureSize(uDepthMap,0);
	vec2 texcoord = vec2(gl_FragCoord.xy/texSize);
	float nearestDepth = texture(uDepthMap,texcoord).r;
	float bias = 1e-4;
	//去除掉背景颜色，背景颜色是(1.0,1.0,1.0)，让它用第二种混合方法*alpha，那么就为0了
	if(currentDepth <= nearestDepth + bias)
	{
		FragColor = uObjColor;
	}
	else
	{
		FragColor = vec4(uObjColor.rgb * uObjColor.a,uObjColor.a);
	}
	
}