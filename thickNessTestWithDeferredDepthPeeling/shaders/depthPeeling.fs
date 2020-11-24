#version 460 core
//在从光源视角下的depth_peeling，是不需要outColor的
layout(location = 0) out vec4 outThick;//渲染到帧缓冲中的纹理0
layout(binding = 0) uniform sampler2DArray depthTexSet;
layout(binding = 1) uniform sampler2D thickTex;
uniform int uLayerIndex;

void main()
{
	//按顺序绘制，第i次只绘制第i层的片段,注意这个第几层与阴影遮挡是没有关系的
	//这个指的是相机视角下的第几层
	if(uLayerIndex > 0)//第0层都不用管
	{
		float currentDepth = gl_FragCoord.z;
		vec2 texsize = textureSize(depthTexSet,0).xy;
		vec2 texcoord = vec2(gl_FragCoord.xy/texsize.xy);
		float preDepth = texture(depthTexSet,vec3(texcoord,uLayerIndex-1)).r;
		
		float addBias = 1e-4;
		if(currentDepth <= preDepth+addBias) discard;
	}
	//outColor = uObjColor;
}
