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
layout(binding = 0,rgba32ui) uniform coherent restrict uimageBuffer linkedLists;//GL_TEXTURE_BUFFER
//layout(binding = 0,rgba32ui) uniform writeonly restrict uimageBuffer linkedLists;
layout(binding = 1,r32ui) uniform coherent restrict uimage2D uListHeadPtrTex;//GL_TEXTURE_2D

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
	//将当前的节点保存到头节点表中，然后将头节点表中的节点取出来
	//下面的操作必须要原子锁，主要是在imageLoad这一步有问题，比如1.0位置，同时来了两个节点1.1，1.2
	//这样就会有1.1.x = 1.0,1.2.x = 1.1,两者的x都是同一个节点1.0，正确的顺序应该是1.2.x = 1.1,1.1.x = 1.0
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
	imageStore(linkedLists,int(currentIndex),node);//将node存储到一维链表中的合适的位置
}
