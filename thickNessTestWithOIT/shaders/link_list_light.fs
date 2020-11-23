#version 460 core
layout(early_fragment_tests) in;//������ǰ��Ȳ��ԣ�������ɼ���ֱ�ӱ�discard������������Ƭ����ɫ��
//layout(pixel_interlock_unordered) in; //��fs��Ƭ�εĴ�����Ҫ����ͼԪ��˳��
//�ؼ��ֶ���Ч�ʵ�Ӱ�첻����Ҫ��coherent�ؼ��ֶ���imageͬ���ȽϹؼ�
layout(binding = 0, offset = 0) uniform atomic_uint uListNodeCounter;
layout(binding = 2,r32ui) uniform coherent restrict uimage2D uLightHeadPtrTex;//GL_TEXTURE_2D

struct ListNodeLight
{
	uint next;
	uint depth;
	//uvec2 nodeDataLight;//x�洢nextָ�룬y���
};

layout(std430,binding = 1) coherent restrict buffer ssboLight
{
	ListNodeLight nodeLightSet[];
};

uniform int uMaxListNode;
#define PI 3.1415926

void main()
{
	uint currentIndex = atomicCounterIncrement(uListNodeCounter);//currentIndex��1��ʼ
	if (currentIndex > uMaxListNode)
	{
		discard;//����ֻ��ʹ��discard��openGL��������return�Ժ�call begin()/end()
	}
	uint oldIndex = imageAtomicExchange(uLightHeadPtrTex, ivec2(gl_FragCoord.xy), currentIndex);
	nodeLightSet[currentIndex] = ListNodeLight(oldIndex,floatBitsToUint(gl_FragCoord.z));
}

