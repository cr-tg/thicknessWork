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
layout(binding = 1,r32ui) uniform coherent restrict uimage2D uListHeadPtrTex;//GL_TEXTURE_2D

struct ListNodeCam
{
	uvec3 nodeDataCam;//x�洢nextָ�룬y�����洢��ɫ��z�洢���(��Ҫ�Ӹ�������ת��Ϊint����)
};

layout(std430,binding = 0) coherent restrict buffer ssboCam
{
	ListNodeCam nodeCamSet[];
};

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
	//������Ҫ�ڵ���ɫ���ڵ�cam�µ���ȣ��ڵ�cam��old index���ڵ�light�µ���ȣ��ڵ�light�µ�old index
	//Ҫ5���������������һ��image������(���4������)��������һ��image����ʹ��ssboCam
	//������ʹ��һ�������pass����Ϊcam�ƶ��Ժ󣬿�����ԭ����lightCoordλ���£����Ѿ�û��Ƭ���ˣ���ʱ����������Ƭ����ɫ��
	uint oldIndex = imageAtomicExchange(uListHeadPtrTex, ivec2(gl_FragCoord.xy), currentIndex);
	uvec3 nodeCam;
	nodeCam.x = oldIndex;
	nodeCam.y = packUnorm4x8(uObjColor);
	nodeCam.z = floatBitsToUint(gl_FragCoord.z);
	nodeCamSet[currentIndex] = ListNodeCam(nodeCam);
}

//vec3 calFragCoordFromLight()
//{
//	vec3 lightFragCoord = vec3(0.0);
//	float depth = 0.0f;
//	float clipZ = lightFragPos.z;
//	float ndcZ = lightFragPos.z / lightFragPos.w;
//	float winZ = 0.5*ndcZ+0.5;
//	depth = winZ;
//	//vec2 ndcXY = lightFragPos.xy; //����͸�ӳ����м������Ե����ص�
//	vec2 ndcXY = lightFragPos.xy/lightFragPos.w;
//	vec2 winXY = (0.5 *ndcXY+0.5);
//	vec2 screenXY = winXY * uScreenSize;
//	lightFragCoord = vec3(screenXY,depth);
//	return lightFragCoord;
//}


