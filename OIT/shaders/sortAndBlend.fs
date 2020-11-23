 
 #version 460 core
layout(location = 0) out vec4 outColor;


layout(binding = 0,rgba32ui) uniform  uimageBuffer linkedListsT;//GL_TEXTURE_BUFFER
layout(binding = 1,r32ui) uniform  uimage2D uListHeadPtrTex;//GL_TEXTURE_2D

#define MAX_FRAGMENTS 15
uvec4 fragments[MAX_FRAGMENTS];//存储当前像素下的所有可见片元

int buildLocalFragmentList();//将当前像素下的所有片元保存到数组中，返回这个list的长度
void bubbleSortFragmentList(int fragCount);
void insertSortFragmentList(int fragCount);
vec4 calFinalColor(int fragCount);
vec4 blend(vec4 preColor,vec4 currentColor);

//有一个问题，比如透明物体ABC，由近到远，我的摄像机来到了AB之间，此时看到的是B，那么只需要在BC上的片段进行排序，A不需要
//这一点不需要担心，因为裁剪会剔除A，而裁剪发生在图元组装的时候，图元组装在光栅化之前

//场景复杂情况下，冒泡390FPS，插入490FPS

void main()
{
	int fragCount = 0;
	fragCount = buildLocalFragmentList();
	//bubbleSortFragmentList(fragCount);
	insertSortFragmentList(fragCount);
	outColor = calFinalColor(fragCount);
}

int buildLocalFragmentList()
{
	int fragCount = 0;
	uint curretIndex = uint(imageLoad(uListHeadPtrTex,ivec2(gl_FragCoord.xy)).r);
	while(curretIndex != 0 && fragCount < MAX_FRAGMENTS)
	{
		//uvec4 node = nodes[curretIndex].nodeData;
		uvec4 node = uvec4(imageLoad(linkedListsT,int(curretIndex)));
		fragments[fragCount] = node;
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
		//float depth_i = uintBitsToFloat(fragments[i].z);
		for(j = 0;j<fragCount-1-i;++j)
		{
			float depth_j = uintBitsToFloat(fragments[j].z);
			float depth_j1 = uintBitsToFloat(fragments[j+1].z);
			if(depth_j > depth_j1)//将i放到后面去
			{
				isInvert = true;
				uvec4 temp = fragments[j];
				fragments[j] = fragments[j+1];
				fragments[j+1] = temp;
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
			if(uintBitsToFloat(fragments[j].z) < uintBitsToFloat(fragments[j-1].z))//新来的数更小，就与前面的数交换
			{
				uvec4 temp = fragments[j-1];
				fragments[j-1] = fragments[j];
				fragments[j] = temp;
			}
			else break;//比前面的数更大，就证明整个序列是有序的
		}
	}
}

vec4 calFinalColor(int fragCount)
{
	if(fragCount == 0)
		return vec4(vec3(0.3),1.0);//背景的颜色
	vec4 preColor = unpackUnorm4x8(fragments[fragCount-1].y);//最后一层的颜色
	for(int i=fragCount-2;i>-1;--i)
	{
		vec4 currentColor = unpackUnorm4x8(fragments[i].y);//每个分量就是用f/255.0
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
