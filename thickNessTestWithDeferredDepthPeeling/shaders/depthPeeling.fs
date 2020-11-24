#version 460 core
//�ڴӹ�Դ�ӽ��µ�depth_peeling���ǲ���ҪoutColor��
layout(location = 0) out vec4 outThick;//��Ⱦ��֡�����е�����0
layout(binding = 0) uniform sampler2DArray depthTexSet;
layout(binding = 1) uniform sampler2D thickTex;
uniform int uLayerIndex;

void main()
{
	//��˳����ƣ���i��ֻ���Ƶ�i���Ƭ��,ע������ڼ�������Ӱ�ڵ���û�й�ϵ��
	//���ָ��������ӽ��µĵڼ���
	if(uLayerIndex > 0)//��0�㶼���ù�
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
