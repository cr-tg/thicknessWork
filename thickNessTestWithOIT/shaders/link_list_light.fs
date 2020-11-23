#version 460 core
layout(early_fragment_tests) in;//开启提前深度测试，如果不可见就直接被discard，并不会运行片段着色器
//layout(pixel_interlock_unordered) in; //让fs对片段的处理不需要按照图元的顺序
//关键字对于效率的影响不大，主要是coherent关键字对于image同步比较关键
layout(binding = 0, offset = 0) uniform atomic_uint uListNodeCounter;
layout(binding = 2,r32ui) uniform coherent restrict uimage2D uLightHeadPtrTex;//GL_TEXTURE_2D

struct ListNodeLight
{
	uint next;
	uint depth;
	//uvec2 nodeDataLight;//x存储next指针，y深度
};

layout(std430,binding = 1) coherent restrict buffer ssboLight
{
	ListNodeLight nodeLightSet[];
};

uniform int uMaxListNode;
#define PI 3.1415926

void main()
{
	uint currentIndex = atomicCounterIncrement(uListNodeCounter);//currentIndex从1开始
	if (currentIndex > uMaxListNode)
	{
		discard;//这里只能使用discard，openGL不允许在return以后call begin()/end()
	}
	uint oldIndex = imageAtomicExchange(uLightHeadPtrTex, ivec2(gl_FragCoord.xy), currentIndex);
	nodeLightSet[currentIndex] = ListNodeLight(oldIndex,floatBitsToUint(gl_FragCoord.z));
}

