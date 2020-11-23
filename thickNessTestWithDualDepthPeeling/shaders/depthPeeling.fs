#version 460 core
//在从光源视角下的depth_peeling，是不需要outColor的
layout(location = 0) out vec4 outColor;//渲染到帧缓冲中的纹理0
layout(binding = 0) uniform sampler2D uPreDepthMap;
uniform vec4 uObjColor;
uniform int uLayerIndex;

void main()
{
	//按顺序绘制，第i次只绘制第i层的片段,注意这个第几层与阴影遮挡是没有关系的
	//这个指的是相机视角下的第几层
	if(uLayerIndex > 0)//第0层都不用管
	{
		float currentDepth = gl_FragCoord.z;
		vec2 texsize = textureSize(uPreDepthMap,0);
		vec2 texcoord = vec2(gl_FragCoord.xy/texsize.xy);
		float preDepth = texture(uPreDepthMap,texcoord).r;
		float addBias = 1e-4;
		if(currentDepth <= preDepth+addBias) discard;
	}
	outColor = uObjColor;
}
