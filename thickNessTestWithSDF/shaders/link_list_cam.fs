#version 460 core
#extension GL_ARB_fragment_shader_interlock : require
#extension GL_NV_fragment_shader_interlock : require

//来自openGL红宝书的实现算法
//这个开启的原因来自于透明物体的渲染不会写入到深度中，而且首先渲染的是不透明物体，因此不透明物体的背后的物体肯定不可见
//这样的话就可以防止链表中存放不可见的片段
//实验证明将不透明的部分放在最前方时，可以提升600FPS。
layout(early_fragment_tests) in;//开启提前深度测试，如果不可见就直接被discard，并不会运行片段着色器
//layout(pixel_interlock_unordered) in; //让fs对片段的处理不需要按照图元的顺序
//关键字对于效率的影响不大，主要是coherent关键字对于image同步比较关键
layout(binding = 0, offset = 0) uniform atomic_uint uListNodeCounter;
layout(binding = 1,r32ui) uniform coherent restrict uimage2D uListHeadPtrTex;//GL_TEXTURE_2D

struct ListNodeCam
{
	uvec3 nodeDataCam;//x存储next指针，y分量存储颜色，z存储深度(需要从浮点类型转换为int类型)
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
	uint currentIndex = atomicCounterIncrement(uListNodeCounter);//currentIndex从1开始
	if (currentIndex > uMaxListNode)
	{
		discard;//这里只能使用discard，openGL不允许在return以后call begin()/end()
	}
	//现在需要节点颜色，节点cam下的深度，节点cam的old index，节点light下的深度，节点light下的old index
	//要5个区域，因此现在用一张image不够了(最多4个区域)，因此添加一张image或者使用ssboCam
	//必须再使用一个额外的pass，因为cam移动以后，可能在原来的lightCoord位置下，就已经没有片段了，此时都不会运行片段着色器
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
//	//vec2 ndcXY = lightFragPos.xy; //不做透视除法有极其明显的像素点
//	vec2 ndcXY = lightFragPos.xy/lightFragPos.w;
//	vec2 winXY = (0.5 *ndcXY+0.5);
//	vec2 screenXY = winXY * uScreenSize;
//	lightFragCoord = vec3(screenXY,depth);
//	return lightFragCoord;
//}


