#version 460 core
#extension GL_ARB_fragment_shader_interlock : require
#extension GL_NV_fragment_shader_interlock : require
//来自openGL红宝书的实现算法
//这个开启的原因来自于透明物体的渲染不会写入到深度中，而且首先渲染的是不透明物体，因此不透明物体的背后的物体肯定不可见
//这样的话就可以防止链表中存放不可见的片段
//layout(early_fragment_tests) in;//开启提前深度测试，如果不可见就直接被discard，并不会运行片段着色器
//layout(pixel_interlock_unordered) in;

struct ListNode
{
	//不需要last指针,w分量保留
	uvec4 nodeData;//x存储next指针，y分量存储颜色，z存储深度(需要从浮点类型转换为int类型)
};

layout(std430,binding = 0) coherent restrict buffer ssbo
{
	ListNode nodes[];
};

layout(binding = 0, offset = 0) uniform atomic_uint uListNodeCounter;
layout(binding = 0, r32ui) uniform coherent restrict uimage2D uListHeadFirstTex;//用来存储深度最小的节点的2Dimage

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
	if(!isCurFirst)//设置当前index的next指针并且在必要的情况下修改其它index的next指针
	{
		if(isCurSmaller)
		{
			nextIndex = lastFirstIndex;
		}
		else//否则就从头开始找到插入的位置
		{
			uint tempLastIndex = firstIndex;
			uint tempNextIndex = firstIndex;
			while(tempNextIndex != 0 && (node.z > nodes[tempNextIndex].nodeData.z))
			{
				tempLastIndex = tempNextIndex;
				tempNextIndex = nodes[tempNextIndex].nodeData.x;
			}
			if(tempNextIndex == 0)//直接插入末尾
			{
				nextIndex = 0;
				nodes[tempLastIndex].nodeData.x = curIndex;
			}
			else//插入中间
			{
				nextIndex = tempNextIndex;
				//这种修改其它index的ssbo会出现同步问题，因为可能其它的invocation也在修改这个index的ssbo，无法确定修改的先后顺序
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
	float random = rand(gl_FragCoord.xy);//生成0-1的随机数
	//float random = rand(gl_FragCoord.xy+vec2(gl_FragCoord.z));//生成0-1的随机数
	float alphaCC = getAlphaCC(firstIndex,curDepth);
	float pb = calInsertPro(alphaCC,alphaCur);
	pb = abs(alphaCC);//alphaCC为0
	//pb = 0.9;
	if (random < pb)//随机数字比pb小，那么就插入这个片段
		return true; 
	return false;
}

float getAlphaCC(uint firstIndex,uint curDepth)//获取前面的层数的覆盖率的总和，就是用1-前面所有不覆盖的概率
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
	int bits = 2 * MAX_FRAGMENTS;//bit的数量为层数的两倍
	int s = int(alphaCC * bits);//alphaCC代表前面所有层的覆盖率的总和，变换到bits上的整数
	int t = int(alphaCur * bits);//代表当前层覆盖的bits的数量
	//相当于有一个正方形盘，一共16格，前面的覆盖了s格，当前的覆盖了t格，现在要求这t格不是全部属于s格的概率
	float pb = 0.0;
	//若t>s，那么肯定有不覆盖的
	if(t>s)	
	{
		pb = 1.0;
	}
	else//若t<=s，就用排列组合做
	{
		int denominator = 1;//分母
		for(int i=2;i<=bits;++i)
		{
			denominator *= i;
		}
		for(int i=2;i<=(s-t);++i)
		{
			denominator *= i;
		}
		int numerator = 1;//分子
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
		isCurFirst = false;//必须要加这两句话
		isCurSmaller = false;
		return firstIndex;
	}
}