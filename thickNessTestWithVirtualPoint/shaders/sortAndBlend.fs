 
 #version 460 core
layout(location = 0) out vec4 outColor;


//layout(binding = 0,rgba32ui) uniform uimageBuffer linkedListsT;//GL_TEXTURE_BUFFER
layout(binding = 1,r32ui) uniform uimage2D uListHeadPtrTex;
layout(binding = 2,r32ui) uniform restrict coherent uimage2D uLightHeadPtrTex;
layout(binding = 3,r32ui) uniform restrict writeonly uimage2D uThickImage;

struct ListNodeCam
{
	uvec3 nodeDataCam;//x存储next指针，y分量存储颜色，z存储深度(需要从浮点类型转换为int类型)
};

layout(std430,binding = 0) coherent restrict buffer ssboCam
{
	ListNodeCam nodeCamSet[];
};

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

#define MAX_FRAGMENTS 15
uvec3 fragmentsCam[MAX_FRAGMENTS];//存储当前像素下的所有可见片元
uvec3 fragmentsLight[MAX_FRAGMENTS];//x分量是nextIndex，z分量用来保存当前片段的index

uniform float near_plane;//光源的near/far
uniform float far_plane;

#define BACKCOLOR vec3(0.1f)

// required when using a perspective projection matrix
//1.0 ->farplane 0.0 ->near_plane
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC 
    return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));	
}

int buildLocalFragmentList();//将当前像素下的所有片元保存到数组中，返回这个list的长度
void bubbleSortFragmentList(int fragCount);
void insertSortFragmentList(int fragCount);
vec4 calFinalColor(int fragCount);
vec4 blend(vec4 preColor,vec4 currentColor);
void calThick(int fragCount);
int buildFragmentListFromLight();
void SortFragmentListFromLight(int fragCount);
void sortSsboWithFragmentList(int fragCount);//将排好序的fragmentList插回到ssbo中，好用来之后的pass中进行采样点的内外判断
void testCalThick(int fragCount);//证明ssbo的排序正确

//有一个问题，比如透明物体ABC，由近到远，我的摄像机来到了AB之间，此时看到的是B，那么只需要在BC上的片段进行排序，A不需要
//这一点不需要担心，因为裁剪会剔除A，而裁剪发生在图元组装的时候，图元组装在光栅化之前

//场景复杂情况下，冒泡390FPS，插入490FPS

void main()
{
	int fragCountCam = 0;
	fragCountCam = buildLocalFragmentList();
	insertSortFragmentList(fragCountCam);
	outColor = calFinalColor(fragCountCam);
	int fragCountLight = 0;
	fragCountLight = buildFragmentListFromLight();
	SortFragmentListFromLight(fragCountLight);
	//calThick(fragCountLight);
	//outColor = vec4(fragCountLight/255.0f*20.0f);
	sortSsboWithFragmentList(fragCountLight);
	//testCalThick(fragCountLight); //看一下构建的ssbo是否有序
}

void sortSsboWithFragmentList(int fragCount)
{
	if(fragCount == 0)
		return;
	uint firstIndex = fragmentsLight[0].z;
	imageAtomicExchange(uLightHeadPtrTex, ivec2(gl_FragCoord.xy), firstIndex);
	for(int i=0;i<fragCount;++i)
	{
		uint currentIndex = fragmentsLight[i].z;
		uint nextIndex = 0;
		if(i != fragCount -1)
		{
			nextIndex = fragmentsLight[i+1].z;
		}
		nodeLightSet[currentIndex] = ListNodeLight(nextIndex,fragmentsLight[i].y);
	}
}

int buildFragmentListFromLight()
{
	int fragCount = 0;
	uint curretIndex = uint(imageLoad(uLightHeadPtrTex,ivec2(gl_FragCoord.xy)).r);
	while(curretIndex != 0 && fragCount < MAX_FRAGMENTS)
	{
		uvec2 node = uvec2(nodeLightSet[curretIndex].next,nodeLightSet[curretIndex].depth);
		//uvec4 node = uvec4(imageLoad(linkedListsT,int(curretIndex)));
		fragmentsLight[fragCount] = uvec3(node,curretIndex);
		fragCount++;
		curretIndex = node.x;
	}
	return fragCount;
}

void calThick(int fragCount)
{
	if(fragCount <= 1)
		return;
	float thick = 0.0f;
	float bias = 1e-4;
	for(int i = fragCount-1;i>0;i-=2)
	{
		float lastDepth = uintBitsToFloat(fragmentsLight[i].y);
		if(abs(lastDepth-1.0)< bias) continue;
		lastDepth = LinearizeDepth(lastDepth);
		float preDepth = uintBitsToFloat(fragmentsLight[i-1].y);
		preDepth = LinearizeDepth(preDepth);
		thick += abs(lastDepth - preDepth);
	}
	imageStore(uThickImage,ivec2(gl_FragCoord.xy),ivec4(floatBitsToUint(thick)));
}

void testCalThick(int fragCount)
{
	if(fragCount <= 1)
		return;
	float thick = 0.0f;
	float bias = 1e-4;
	uint preIndex = imageLoad(uLightHeadPtrTex,ivec2(gl_FragCoord.xy)).r;
	for(int i = 0;i<fragCount-1;i+=2)
	{
		float preDepth = uintBitsToFloat(nodeLightSet[preIndex].depth);
		preDepth = LinearizeDepth(preDepth);
		uint nextIndex = nodeLightSet[preIndex].next;
		float lastDepth = uintBitsToFloat(nodeLightSet[nextIndex].depth);
		if(abs(lastDepth-1.0)< bias) continue;
		lastDepth = LinearizeDepth(lastDepth);
		thick += abs(lastDepth - preDepth);
		preIndex = nodeLightSet[nextIndex].next;
	}
	imageStore(uThickImage,ivec2(gl_FragCoord.xy),ivec4(floatBitsToUint(thick)));
}

void SortFragmentListFromLight(int fragCount)
{
	int i,j;
	for(int i = 0;i<fragCount-1;++i)
	{
		for(j = i+1;j>0;--j)
		{
			if(uintBitsToFloat(fragmentsLight[j].y) < uintBitsToFloat(fragmentsLight[j-1].y))//新来的数更小，就与前面的数交换
			{
				uvec3 temp = fragmentsLight[j-1];
				fragmentsLight[j-1] = fragmentsLight[j];
				fragmentsLight[j] = temp;
			}
			else break;//比前面的数更大，就证明整个序列是有序的
		}
	}
}

int buildLocalFragmentList()
{
	int fragCount = 0;
	uint curretIndex = uint(imageLoad(uListHeadPtrTex,ivec2(gl_FragCoord.xy)).r);
	while(curretIndex != 0 && fragCount < MAX_FRAGMENTS)
	{
		uvec3 node = nodeCamSet[curretIndex].nodeDataCam;
		//uvec4 node = uvec4(imageLoad(linkedListsT,int(curretIndex)));
		fragmentsCam[fragCount] = node;
		fragCount++;
		curretIndex = node.x;
	}
	return fragCount;
}

void bubbleSortFragmentList(int fragCount)//将深度从小到大排列起来
{
	int i,j;
	for(int i = 0;i<fragCount;++i)
	{
		bool isInvert = false;
		//float depth_i = uintBitsToFloat(fragmentsCam[i].z);
		for(j = 0;j<fragCount-1-i;++j)
		{
			float depth_j = uintBitsToFloat(fragmentsCam[j].z);
			float depth_j1 = uintBitsToFloat(fragmentsCam[j+1].z);
			if(depth_j > depth_j1)//将i放到后面去
			{
				isInvert = true;
				uvec3 temp = fragmentsCam[j];
				fragmentsCam[j] = fragmentsCam[j+1];
				fragmentsCam[j+1] = temp;
			}
		}
		if(!isInvert)
			break;
	}
}

void insertSortFragmentList(int fragCount)//插入排序
{
	int i,j;
	for(int i = 0;i<fragCount-1;++i)
	{
		for(j = i+1;j>0;--j)
		{
			if(uintBitsToFloat(fragmentsCam[j].z) < uintBitsToFloat(fragmentsCam[j-1].z))//新来的数更小，就与前面的数交换
			{
				uvec3 temp = fragmentsCam[j-1];
				fragmentsCam[j-1] = fragmentsCam[j];
				fragmentsCam[j] = temp;
			}
			else break;//比前面的数更大，就证明整个序列是有序的
		}
	}
}

vec4 calFinalColor(int fragCount)
{
	if(fragCount == 0)
		return vec4(BACKCOLOR,1.0);//背景的颜色
	vec4 preColor = unpackUnorm4x8(fragmentsCam[fragCount-1].y);//最后一层的颜色
	for(int i=fragCount-2;i>-1;--i)
	{
		vec4 currentColor = unpackUnorm4x8(fragmentsCam[i].y);//每个分量就是用f/255.0
		preColor = blend(preColor,currentColor);
	}
	return preColor;
}

vec4 blend(vec4 preColor,vec4 currentColor)
{
	vec4 blendColor;
	blendColor = vec4(currentColor.rgb*currentColor.a+preColor.rgb*(1-currentColor.a),currentColor.a);
	return blendColor;
}
