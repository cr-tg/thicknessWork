#version 460 core
#extension GL_ARB_fragment_shader_interlock : require
#extension GL_NV_fragment_shader_interlock : require

//����openGL�챦���ʵ���㷨
//���������ԭ��������͸���������Ⱦ����д�뵽����У�����������Ⱦ���ǲ�͸�����壬��˲�͸������ı��������϶����ɼ�
//�����Ļ��Ϳ��Է�ֹ�����д�Ų��ɼ���Ƭ��
//ʵ��֤������͸���Ĳ��ַ�����ǰ��ʱ����������600FPS��
layout(early_fragment_tests) in;//������ǰ��Ȳ��ԣ�������ɼ���ֱ�ӱ�discard������������Ƭ����ɫ��
//layout(pixel_interlock_unordered) in; //��fs��Ƭ�εĴ�����Ҫ����ͼԪ��˳��
//�ؼ��ֶ���Ч�ʵ�Ӱ�첻����Ҫ��coherent�ؼ��ֶ���imageͬ���ȽϹؼ�
layout(binding = 0, offset = 0) uniform atomic_uint uListNodeCounter;
layout(binding = 0,rgba32ui) uniform coherent restrict uimageBuffer linkedLists;//GL_TEXTURE_BUFFER
//layout(binding = 0,rgba32ui) uniform writeonly restrict uimageBuffer linkedLists;
layout(binding = 1,r32ui) uniform coherent restrict uimage2D uListHeadPtrTex;//GL_TEXTURE_2D

uniform vec4 uObjColor;

uniform int uMaxListNode;
#define PI 3.1415926

void main()
{
	uint currentIndex = atomicCounterIncrement(uListNodeCounter);//currentIndex��1��ʼ
	if (currentIndex > uMaxListNode)
	{
		discard;//����ֻ��ʹ��discard��openGL��������return�Ժ�call begin()/end()
	}
	//����ǰ�Ľڵ㱣�浽ͷ�ڵ���У�Ȼ��ͷ�ڵ���еĽڵ�ȡ����
	//����Ĳ�������Ҫԭ��������Ҫ����imageLoad��һ�������⣬����1.0λ�ã�ͬʱ���������ڵ�1.1��1.2
	//�����ͻ���1.1.x = 1.0,1.2.x = 1.1,���ߵ�x����ͬһ���ڵ�1.0����ȷ��˳��Ӧ����1.2.x = 1.1,1.1.x = 1.0
	//beginInvocationInterlockARB();
	//uint oldIndex = imageLoad(uListHeadPtrTex, ivec2(gl_FragCoord.xy)).r;
	//imageStore(uListHeadPtrTex,ivec2(gl_FragCoord.xy),uvec4(currentIndex));
	//endInvocationInterlockARB();
	uint oldIndex = imageAtomicExchange(uListHeadPtrTex, ivec2(gl_FragCoord.xy), currentIndex);
	uvec4 node;
	node.x = oldIndex;
	node.y = packUnorm4x8(uObjColor);
	node.z = floatBitsToUint(gl_FragCoord.z);
	node.w = 0;
	imageStore(linkedLists,int(currentIndex),node);//��node�洢��һά�����еĺ��ʵ�λ��
}
