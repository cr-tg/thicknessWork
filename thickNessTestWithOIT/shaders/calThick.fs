//֮ǰ��depth_peeling��OIT������ʵ��Դλ�ò��䣬Ȼ��ӹ�Դλ�ó������������ǵ����λ��Ҳʼ�ղ����Ҵ��ڳ�����ǰ��
//������������ĺ��ʼ���ǹ��ߴ��������������ĺ�ȣ����Ҽ�ʹ���λ���ƶ��ˣ����ɵĺ��Ҳ����ı䣬���ɵĺ�Ȳ����ǵ�ǰ�����������ɫ��ĺ��
//����һ���̶�λ�õ������������ɫ��ĺ�ȣ���������������Ϊ�˲��Ծ�������׼ȷ���

//���������ֱ�Ӵ���ľ��ǹ̶���006�����λ�ã�Ȼ���������¿���������Ƭ�ξ�����ɫ��
//������˵�������ڹ̶�����ӽ�����defferd shading����ȥworldPos��Ȼ��ʹ�������棬�������pass�ж�������в�������
#version 460 core
layout(binding = 0) uniform sampler2D gPosition;//��ɫ�����������
layout(binding = 1) uniform sampler2D shadowMapRealLight;//��ʵ��Դ�µ�shadow map
layout(binding = 2) uniform sampler2D gNormal;//��ɫ�����������
layout(binding = 2,r32ui) uniform restrict uimage2D uLightHeadPtrTex;
layout(binding = 3,r32ui) uniform restrict writeonly uimage2D uThickImage;

uniform mat4 realLightProjection;
uniform mat4 realLightView;
uniform float real_near_plane;
uniform float real_far_plane;
uniform vec3 uLightPos;

#define PROJECTION_MODE PROJECTION_PERSPECTIVE

struct ListNodeLight
{
	uint next;
	uint depth;
	//uvec2 nodeDataLight;//x�洢nextָ�룬y���
};

layout(std430,binding = 1) coherent restrict buffer ssboLight
{
	ListNodeLight nodeLightSet[];
};


out vec4 testColor;

float returnZe(float depth)
{
	float z = depth * 2.0 - 1.0; // back to NDC 
	//ndcz = -1��ze = -real_near_plane
	z = (2.0 * real_near_plane * real_far_plane) / (z * (real_far_plane - real_near_plane) - (real_far_plane + real_near_plane)); // range: -real_near_plane...-real_far_plane,
	return z;
}

float linearizeDepth(float depth)
{
	float z = -returnZe(depth);
	z = (z - real_near_plane) / (real_far_plane - real_near_plane); // range: 0...1
	return z;
}

float getClipDepthFromLinearZ(float linearZ)
{
	return -real_near_plane+linearZ*(real_near_plane+real_far_plane);
}

//����Ŀ���ķ�������ȣ�ͶӰ�����Զ��ƽ�棬�ڲü�����ϵ�µ�UV�����pv���������������������
vec4 getWorldPosFromDepth(float depth,vec2 clipPosUV);
float getThickInClipSpace(float oriClipDepth,float dstClipDepth,uint firstIndex);
float getThickInWorldSpace(vec4 worldSpaceOri,vec4 worldSpaceDst,vec2 clipPosUV,uint firstIndex);
float calThick(vec4 worldSpaceDst,vec2 clipPosUV,uint firstIndex);

void main()
{
	vec4 worldSpaceDst = texelFetch(gPosition,ivec2(gl_FragCoord.xy),0);
	vec3 worldDstNormal = normalize(texelFetch(gNormal,ivec2(gl_FragCoord.xy),0).xyz);
	testColor = vec4(worldDstNormal,1.0f);
//	testColor = worldSpaceDst;
	return;
	mat4 realLightPV = realLightProjection * realLightView;
	vec4 lightClipSpaceDst = realLightPV * worldSpaceDst;//�ü����꣬-w,w�ķ�Χ������w�Ժ󣬱任��-1,1�ķ�Χ
	vec3 sampleCoord = lightClipSpaceDst.xyz / lightClipSpaceDst.w;// �������꣬��shadowmap�н��в������õ�DST��ORI��zֵ��
	sampleCoord = sampleCoord * 0.5 +0.5;//xyҲ����[-1��1]
	ivec2 headPtrImageSize = imageSize(uLightHeadPtrTex);
	ivec2 iSampleCoord = ivec2(headPtrImageSize * sampleCoord.xy);
	uint firstIndex = imageLoad(uLightHeadPtrTex,iSampleCoord).r;
//	if(firstIndex == 0) //���д����仰���ᵼ��һЩ����Ӧ����Բ���ϵ�������ʧ�ˣ����ǲ������µ����
//		return;
	float oriDepth = uintBitsToFloat(nodeLightSet[firstIndex].depth);//�洢����gl_FragCoord.z
	float oriLinearDepth = linearizeDepth(oriDepth);
	float oriClipDepth = getClipDepthFromLinearZ(oriLinearDepth);
	float allthick =0.0f;
//	allthick = getThickInClipSpace(oriClipDepth,lightClipSpaceDst.z,firstIndex);
	
	vec2 clipPosUV = lightClipSpaceDst.xy/lightClipSpaceDst.w;
	vec4 worldSpaceOri = getWorldPosFromDepth(oriDepth,clipPosUV);
	//allthick = getThickInWorldSpace(worldSpaceOri,worldSpaceDst,clipPosUV,firstIndex);
	float allthick1 = calThick(worldSpaceDst,clipPosUV,firstIndex);
	//testColor = vec4(abs(allthick1-allthick)/(255.0f)*100.0f);
	//testColor = vec4(allthick1/(255.0f)*100.0f);
	//testColor = vec4(allthick/(255.0f)*100.0f);
	imageStore(uThickImage,ivec2(gl_FragCoord.xy),ivec4(floatBitsToUint(allthick)));
}

float calThick(vec4 worldSpaceDst,vec2 clipPosUV,uint firstIndex)
{
    int depthIdx = 0;
	vec3 rd = normalize(worldSpaceDst.xyz - uLightPos);
	float outerDist = 0.05f;
	vec3 rend = worldSpaceDst.xyz + outerDist * rd;
	uint index = firstIndex;
    float thickness = 0.0;
	vec3 testPos = vec3(0.0f);
    while(index != 0)
    {
		float d1 = uintBitsToFloat(nodeLightSet[index].depth);
		vec4 worldPos1 = getWorldPosFromDepth(d1,clipPosUV);
		float t = dot(rend - worldPos1.xyz, rd);
        if(t<0.0) break;
		depthIdx++;
		index = nodeLightSet[index].next;
		if(index == 0)
			break;
		float d2 = uintBitsToFloat(nodeLightSet[index].depth);
		vec4 worldPos2 = getWorldPosFromDepth(d2,clipPosUV);
		if(depthIdx == 1)
		{
			testPos = worldPos2.xyz;
		}
		t = dot(rend - worldPos2.xyz, rd);
        if(t<0.0) 
		{
//			if(depthIdx == 1)
//			{
//				testColor = vec4(1.0,abs(testPos.yz/255.0f*50.0f),1.0);
//				testColor = vec4(1.0,abs(rend.yz/255.0f*50.0f),1.0);
//			}
//			if(depthIdx == 3)
//			{
//				testColor = vec4(0.0,abs(testPos.yz/255.0f*50.0f),1.0);
//			}
			break;
		}
		depthIdx++;
		thickness += length(worldPos2.xyz - worldPos1.xyz);
		index = nodeLightSet[index].next;
    }
	if(depthIdx == 1)
	{
			testColor = vec4(1.0,abs(testPos.yz/255.0f*50.0f),1.0);
	}
	else
	{
		testColor = vec4(depthIdx/255.0f,abs(testPos.yz/255.0f*50.0f),1.0);
		//testColor = vec4(0.0,abs(rend.yz/255.0f*50.0f),1.0);
		//testColor = vec4(thickness/(255.0f)*100.0f,depthIdx/255.0f,0.0,1.0);
	}

//	testColor = vec4(rend/255.0f*50.0f,1.0);
//	testColor = vec4(thickness/(255.0f)*100.0f);
//	testColor = vec4(rend/255.0f*50.0f,1.0);
	return thickness;
}

float getThickInWorldSpace(vec4 worldSpaceOri,vec4 worldSpaceDst,vec2 clipPosUV,uint firstIndex)
{
	float allthick = length(worldSpaceDst.xyz - worldSpaceOri.xyz); 
	vec3 dir = normalize(worldSpaceDst.xyz - worldSpaceOri.xyz);
	float outObjDistance = 0.0f;
	uint index = nodeLightSet[firstIndex].next;
	int depth = 0;
	while(index != 0)
	{
		float d1 = uintBitsToFloat(nodeLightSet[index].depth);
		vec4 worldPos1 = getWorldPosFromDepth(d1,clipPosUV);
		index = nodeLightSet[index].next;
		if(index == 0)
			break;
		float d2 = uintBitsToFloat(nodeLightSet[index].depth);
		vec4 worldPos2 = getWorldPosFromDepth(d2,clipPosUV);
		if(length((worldPos1.xyz + worldPos2.xyz)*0.5 - worldSpaceOri.xyz) >= allthick)
		{
			break;
		}
		outObjDistance += length(worldPos2.xyz - worldPos1.xyz);
		index = nodeLightSet[index].next;
	}
	allthick -= outObjDistance;
	testColor = vec4(allthick/(255.0f)*100.0f);
	return allthick;
}




vec4 getWorldPosFromDepth(float depth,vec2 clipPosUV)
{
	vec4 worldPos = vec4(0.0f);
	float oriLinearDepth = linearizeDepth(depth);//0-1
	float bias = 1e-5;
	float zc = -real_near_plane+oriLinearDepth*(real_near_plane+real_far_plane);
	float ze = returnZe(depth);
	float wc = -ze;
	float xc = clipPosUV.x * wc;
	float yc = clipPosUV.y * wc;
	vec4 clipPos = vec4(xc,yc,zc,wc);
	worldPos = inverse(realLightProjection * realLightView) * clipPos;
	return worldPos;
}

float getThickInClipSpace(float oriClipDepth,float dstClipDepth,uint firstIndex)
{
	float allthick = dstClipDepth - oriClipDepth; //ʹ�õ��ǲü�����ϵ�µ�zֵ�Ĳ�ֵ
	float outObjDistance = 0.0f;
	uint index = nodeLightSet[firstIndex].next;
	while(index != 0)
	{
		float d1 = uintBitsToFloat(nodeLightSet[index].depth);
		d1 = linearizeDepth(d1);
		index = nodeLightSet[index].next;
		if(index == 0)
			break;
		float d2 = uintBitsToFloat(nodeLightSet[index].depth);
		d2 = linearizeDepth(d2);
		if(getClipDepthFromLinearZ(0.5*(d1+d2)) > dstClipDepth) break;
		outObjDistance += getClipDepthFromLinearZ(d2-d1);
		index = nodeLightSet[index].next;
	}

	allthick -= outObjDistance;
	return allthick;
}





//��������任�Ĺ���
//0.͸��ͶӰ������Ƶ�����������ͶӰ����Ļ����ϣ��Ƚ�Զƽ�����ɺͽ�ƽ��һ����С
//1.���Ƚ���һ������Ҫ�����⣬Ϊʲôview space�µ�z�ķ�ΧΪ-n��-f��������Ϊview space��dir�ᣬҲ����z�ᣬ����ָ��target��
//����ָ��target�ķ�����ģ��������ܹ�������real_near_plane,real_far_plane֮������壬������view space�¶���λ��z��ĸ������ϵģ�Ҳ����-n,-f
//2.д��ͶӰ�����������ʽ(���ԳƵ����)������view�ռ��µ����꣬���Եó�wc = -ze��Ȼ��zc = -(f+n)/(f-n) * ze - 2fn/(f - n)
//3.������Ϊ֮��Ҫ��zc/wc�Ĳ�������˻����͸��ͶӰ�µ�zֵ�������Եģ��任�����Եķ�������ze = 2fn/((zndc)(f-n)-(f+n)),ע�������ze��-n��-f
//4.��˿��Կ�������������Ǹ��任����ĸ���෴�ģ���˷��ص���-ze����ΧΪreal_near_plane��real_far_plane
//5.��zc = -2fn * zndc/(zndc(f-n)-(f+n)),���ʹ��linearz��ʾ�Ļ���zc = -n+zl(f+n),zc��ȡֵ��Χ��[-n,f]



