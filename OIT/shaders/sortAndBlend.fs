 
 #version 460 core
layout(location = 0) out vec4 outColor;


layout(binding = 0,rgba32ui) uniform  uimageBuffer linkedListsT;//GL_TEXTURE_BUFFER
layout(binding = 1,r32ui) uniform  uimage2D uListHeadPtrTex;//GL_TEXTURE_2D

#define MAX_FRAGMENTS 15
uvec4 fragments[MAX_FRAGMENTS];//�洢��ǰ�����µ����пɼ�ƬԪ

int buildLocalFragmentList();//����ǰ�����µ�����ƬԪ���浽�����У��������list�ĳ���
void bubbleSortFragmentList(int fragCount);
void insertSortFragmentList(int fragCount);
vec4 calFinalColor(int fragCount);
vec4 blend(vec4 preColor,vec4 currentColor);

//��һ�����⣬����͸������ABC���ɽ���Զ���ҵ������������AB֮�䣬��ʱ��������B����ôֻ��Ҫ��BC�ϵ�Ƭ�ν�������A����Ҫ
//��һ�㲻��Ҫ���ģ���Ϊ�ü����޳�A�����ü�������ͼԪ��װ��ʱ��ͼԪ��װ�ڹ�դ��֮ǰ

//������������£�ð��390FPS������490FPS

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

void bubbleSortFragmentList(int fragCount)//����ȴ�С������������
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
			if(depth_j > depth_j1)//��i�ŵ�����ȥ
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

void insertSortFragmentList(int fragCount)//��������
{
	int i,j;
	for(int i = 0;i<fragCount-1;++i)
	{
		for(j = i+1;j>0;--j)
		{
			if(uintBitsToFloat(fragments[j].z) < uintBitsToFloat(fragments[j-1].z))//����������С������ǰ���������
			{
				uvec4 temp = fragments[j-1];
				fragments[j-1] = fragments[j];
				fragments[j] = temp;
			}
			else break;//��ǰ��������󣬾�֤�����������������
		}
	}
}

vec4 calFinalColor(int fragCount)
{
	if(fragCount == 0)
		return vec4(vec3(0.3),1.0);//��������ɫ
	vec4 preColor = unpackUnorm4x8(fragments[fragCount-1].y);//���һ�����ɫ
	for(int i=fragCount-2;i>-1;--i)
	{
		vec4 currentColor = unpackUnorm4x8(fragments[i].y);//ÿ������������f/255.0
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
