#version 460 core
#extension GL_ARB_fragment_shader_interlock : require
#extension GL_NV_fragment_shader_interlock : require
//����openGL�챦���ʵ���㷨
//���������ԭ��������͸���������Ⱦ����д�뵽����У�����������Ⱦ���ǲ�͸�����壬��˲�͸������ı��������϶����ɼ�
//�����Ļ��Ϳ��Է�ֹ�����д�Ų��ɼ���Ƭ��
//layout(early_fragment_tests) in;//������ǰ��Ȳ��ԣ�������ɼ���ֱ�ӱ�discard������������Ƭ����ɫ��
//layout(pixel_interlock_unordered) in;

struct ListNode
{
	//����Ҫlastָ��,w��������
	uvec4 nodeData;//x�洢nextָ�룬y�����洢��ɫ��z�洢���(��Ҫ�Ӹ�������ת��Ϊint����)
};

layout(std430,binding = 0) coherent restrict buffer ssbo
{
	ListNode nodes[];
};

layout(binding = 0, offset = 0) uniform atomic_uint uListNodeCounter;
layout(binding = 0, r32ui) uniform coherent restrict uimage2D uListHeadFirstTex;//�����洢�����С�Ľڵ��2Dimage

uniform vec4 uObjColor;

uniform int uMaxListNode;
#define PI 3.1415926
#define MAX_FRAGMENTS 15

bool checkIfInsert(uint firstIndex,float alphaCur,uint curDepth); 
float calInsertPro(float alphaCC,float alphaCur);
float getAlphaCC(uint firstIndex,uint curDepth);
uint getFirstIndex(uint curIndex,uint curDepth,out bool isCurFirst,out bool isCurSmaller);




void main()
{
	uint curIndex = atomicCounterIncrement(uListNodeCounter);
	if (curIndex >= uMaxListNode) discard;
	uint nextIndex = 0;
	uvec4 node;
	node.x = 0;
	node.y = packUnorm4x8(uObjColor);
	node.z = floatBitsToUint(gl_FragCoord.z);
	node.w = 0;
	bool isCurFirst = false;
	bool isCurSmaller = false;
	beginInvocationInterlockARB();
	uint lastFirstIndex = uint(imageLoad(uListHeadFirstTex,ivec2(gl_FragCoord.xy)).r);;
	if(!checkIfInsert(lastFirstIndex,uObjColor.a,node.z)) discard;
	uint firstIndex = getFirstIndex(curIndex,node.z,isCurFirst,isCurSmaller);
	if(!isCurFirst)//���õ�ǰindex��nextָ�벢���ڱ�Ҫ��������޸�����index��nextָ��
	{
		if(isCurSmaller)
		{
			nextIndex = lastFirstIndex;
		}
		else//����ʹ�ͷ��ʼ�ҵ������λ��
		{
			uint tempLastIndex = firstIndex;
			uint tempNextIndex = firstIndex;
			while(tempNextIndex != 0 && (node.z > nodes[tempNextIndex].nodeData.z))
			{
				tempLastIndex = tempNextIndex;
				tempNextIndex = nodes[tempNextIndex].nodeData.x;
			}
			if(tempNextIndex == 0)//ֱ�Ӳ���ĩβ
			{
				nextIndex = 0;
				nodes[tempLastIndex].nodeData.x = curIndex;
			}
			else//�����м�
			{
				nextIndex = tempNextIndex;
				//�����޸�����index��ssbo�����ͬ�����⣬��Ϊ����������invocationҲ���޸����index��ssbo���޷�ȷ���޸ĵ��Ⱥ�˳��
				nodes[tempLastIndex].nodeData.x = curIndex;
			}
		}
	}
	endInvocationInterlockARB();
	node.x = nextIndex;
	nodes[curIndex] = ListNode(node);
	//endInvocationInterlockARB();
}

float rand(vec2 co)
{
	return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453);
}

bool checkIfInsert(uint firstIndex,float alphaCur,uint curDepth)
{
	if(firstIndex == 0) return true;
	float random = rand(gl_FragCoord.xy);//����0-1�������
	//float random = rand(gl_FragCoord.xy+vec2(gl_FragCoord.z));//����0-1�������
	float alphaCC = getAlphaCC(firstIndex,curDepth);
	float pb = calInsertPro(alphaCC,alphaCur);
	pb = abs(alphaCC);//alphaCCΪ0
	//pb = 0.9;
	if (random < pb)//������ֱ�pbС����ô�Ͳ������Ƭ��
		return true; 
	return false;
}

float getAlphaCC(uint firstIndex,uint curDepth)//��ȡǰ��Ĳ����ĸ����ʵ��ܺͣ�������1-ǰ�����в����ǵĸ���
{
	float alphaCC = 1.0;
	while(firstIndex != 0)
	{
		float alpha = unpackUnorm4x8(nodes[firstIndex].nodeData.y).w;
		alphaCC *= (1-alpha);
		//firstIndex = nodes[firstIndex].nextIndex;
		firstIndex = nodes[firstIndex].nodeData.x;
	}
	alphaCC = 1.0 - alphaCC;
	return alphaCC;
}

float calInsertPro(float alphaCC,float alphaCur)
{
	int bits = 2 * MAX_FRAGMENTS;//bit������Ϊ����������
	int s = int(alphaCC * bits);//alphaCC����ǰ�����в�ĸ����ʵ��ܺͣ��任��bits�ϵ�����
	int t = int(alphaCur * bits);//����ǰ�㸲�ǵ�bits������
	//�൱����һ���������̣�һ��16��ǰ��ĸ�����s�񣬵�ǰ�ĸ�����t������Ҫ����t����ȫ������s��ĸ���
	float pb = 0.0;
	//��t>s����ô�϶��в����ǵ�
	if(t>s)	
	{
		pb = 1.0;
	}
	else//��t<=s���������������
	{
		int denominator = 1;//��ĸ
		for(int i=2;i<=bits;++i)
		{
			denominator *= i;
		}
		for(int i=2;i<=(s-t);++i)
		{
			denominator *= i;
		}
		int numerator = 1;//����
		for(int i=2;i<=s;++i)
		{
			numerator *= i;
		}
		for(int i=2;i<=(bits - t);++i)
		{
			numerator *= i;
		}
		pb = 1.0 - numerator/denominator;
	}
	return pb;
}

uint getFirstIndex(uint curIndex,uint curDepth,out bool isCurFirst,out bool isCurSmaller)
{
	uint firstIndex = uint(imageLoad(uListHeadFirstTex,ivec2(gl_FragCoord.xy)).r);
	if(firstIndex == 0)
	{
		imageAtomicExchange(uListHeadFirstTex, ivec2(gl_FragCoord.xy), curIndex);
		isCurFirst = true;
		isCurSmaller = false;
		return curIndex;
	}
	else if(curDepth <= nodes[firstIndex].nodeData.z)
	{
		imageAtomicExchange(uListHeadFirstTex, ivec2(gl_FragCoord.xy), curIndex);
		isCurFirst = false;
		isCurSmaller = true;
		return curIndex;
	}
	else
	{
		isCurFirst = false;//����Ҫ�������仰
		isCurSmaller = false;
		return firstIndex;
	}
}