 
 #version 460 core
layout(location = 0) out vec4 outColor;
layout(binding = 0,r32ui) uniform restrict uimage2D uListHeadFirstTex;//�����洢�����С�Ľڵ��2Dimage

struct ListNode
{
	uvec4 nodeData;//x�洢nextָ�룬y�洢rgba��ֵ��z�洢���(��Ҫ�Ӹ�������ת��Ϊint����)
};

layout(std430,binding = 0) restrict buffer ssbo
{
	ListNode nodes[];
};

#define MAX_FRAGMENTS 15
vec4 fragments[MAX_FRAGMENTS];//�洢��ǰ�����µ����пɼ�ƬԪ����ɫ

int buildLocalFragmentList(uint firstIndex);
vec4 calFinalColor(int fragCount);
vec4 blend(vec4 preColor,vec4 currentColor);
int buildLocalFragmentList();

void main()
{
	int fragCount = 0;
	uint firstIndex = uint(imageLoad(uListHeadFirstTex,ivec2(gl_FragCoord.xy)).r);
	if(firstIndex == 0)
	{
		outColor = vec4(vec3(1.0f),0.0f);//������ɫ
		return;
	}
	fragCount = buildLocalFragmentList(firstIndex);
	outColor = calFinalColor(fragCount);
	//outColor = vec4(fragCount/25.5);
}

int buildLocalFragmentList(uint firstIndex)
{
	int fragCount = 0;
	while(firstIndex != 0 && fragCount < MAX_FRAGMENTS)
	{
		ListNode node = nodes[firstIndex];
		fragments[fragCount] = unpackUnorm4x8(node.nodeData.y);
		fragCount++;
		firstIndex = node.nodeData.x;
	}
	return fragCount;
}

vec4 calFinalColor(int fragCount)
{
	if(fragCount == 0)
		return vec4(vec3(0.3),1.0);//��������ɫ
	vec4 preColor = fragments[fragCount-1];//���һ�����ɫ
	for(int i=fragCount-2;i>-1;--i)
	{
		vec4 currentColor = fragments[i];
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
