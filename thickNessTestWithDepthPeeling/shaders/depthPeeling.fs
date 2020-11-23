#version 460 core
//�ڴӹ�Դ�ӽ��µ�depth_peeling���ǲ���ҪoutColor��
layout(location = 0) out vec4 outColor;//��Ⱦ��֡�����е�����0
layout(binding = 0) uniform sampler2D uPreDepthMap;
uniform vec4 uObjColor;
uniform int uLayerIndex;

void main()
{
	//��˳����ƣ���i��ֻ���Ƶ�i���Ƭ��,ע������ڼ�������Ӱ�ڵ���û�й�ϵ��
	//���ָ��������ӽ��µĵڼ���
	if(uLayerIndex > 0)//��0�㶼���ù�
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
