//֮ǰ��depth_peeling��OIT������ʵ��Դλ�ò��䣬Ȼ��ӹ�Դλ�ó������������ǵ����λ��Ҳʼ�ղ����Ҵ��ڳ�����ǰ��
//������������ĺ��ʼ���ǹ��ߴ��������������ĺ�ȣ����Ҽ�ʹ���λ���ƶ��ˣ����ɵĺ��Ҳ����ı䣬���ɵĺ�Ȳ����ǵ�ǰ�����������ɫ��ĺ��
//����һ���̶�λ�õ������������ɫ��ĺ�ȣ���������������Ϊ�˲��Ծ�������׼ȷ���

//���������ֱ�Ӵ���ľ��ǹ̶���006�����λ�ã�Ȼ���������¿���������Ƭ�ξ�����ɫ��
//������˵�������ڹ̶�����ӽ�����defferd shading����ȥworldPos��Ȼ��ʹ�������棬�������pass�ж�������в�������
#version 460 core
layout(binding = 0) uniform sampler2D gPosition;//��ɫ�����������
layout(binding = 1) uniform sampler2DArray frontAndBackDepthSet;
layout(binding = 2) uniform sampler2D frontAndBackTex;
layout(binding = 0,r32ui) uniform restrict writeonly uimage2D uThickImage;


uniform mat4 realLightProjection;
uniform mat4 realLightView;
uniform float real_near_plane;
uniform float real_far_plane;
uniform int uDepthNum;


#define PROJECTION_MODE PROJECTION_PERSPECTIVE

//1e-3 �ż��ĳ���
#define bias 1e-3
#define defaultDepth 1.0f
#define maxLayer 10

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
float getThickInClipSpace(float oriLinearDepth,float dstLinearDepth,vec2 sampleCoord);
float getThickInWorldSpace(float oriDepth,vec2 clipPosUV,vec4 worldSpaceDst);

void main()
{
	vec4 worldSpaceDst = texelFetch(gPosition,ivec2(gl_FragCoord.xy),0);
	//testColor = vec4(worldSpaceDst.xy,worldSpaceDst.z/10.f,0.0f);
	mat4 realLightPV = realLightProjection * realLightView;
	vec4 lightClipSpaceDst = realLightPV * worldSpaceDst;//�ü����꣬-w,w�ķ�Χ������w�Ժ󣬱任��-1,1�ķ�Χ
	vec3 sampleCoord = lightClipSpaceDst.xyz / lightClipSpaceDst.w;// �������꣬��shadowmap�н��в������õ�DST��ORI��zֵ��
	sampleCoord = sampleCoord * 0.5 +0.5;//xyҲ����[-1��1]
	vec2 clipPosUV = lightClipSpaceDst.xy/lightClipSpaceDst.w;
	float dstLinearDepth = linearizeDepth(sampleCoord.z);
	
	float pos1 = -texture(frontAndBackDepthSet,vec3(sampleCoord.xy,0)).r;
	float pos2 = -texture(frontAndBackDepthSet,vec3(sampleCoord.xy,1)).r;
	float pos3 = -texture(frontAndBackDepthSet,vec3(sampleCoord.xy,2)).r;
	float pos4 = texture(frontAndBackDepthSet,vec3(sampleCoord.xy,2)).g;
	float pos5 = texture(frontAndBackDepthSet,vec3(sampleCoord.xy,1)).g;
	float pos6 = texture(frontAndBackDepthSet,vec3(sampleCoord.xy,0)).g;
	pos1 = linearizeDepth(pos1);
	pos2 = linearizeDepth(pos2);
	pos3 = linearizeDepth(pos3); 
	pos4 = linearizeDepth(pos4);
	pos5 = linearizeDepth(pos5);
	pos6 = linearizeDepth(pos6);
	float testPos = pos4;
	//if(testPos + bias <= defaultDepth)
	if(testPos > bias)
		testColor = vec4(testPos,testPos,testPos,0.0);
	else
		testColor = vec4(1.0f,0.0f,0.0f,0.0);
	float oriDepth = -texture(frontAndBackDepthSet,vec3(sampleCoord.xy,0)).r;
	//float oriDepth = texture(frontAndBackDepthSet,vec3(sampleCoord.xy,0)).g;
	float oriLinearDepth = linearizeDepth(oriDepth);
	//testColor = vec4(oriLinearDepth);
	if(abs(oriLinearDepth-1.0)< bias) return;
	//vec4 worldSpaceOri = getWorldPosFromDepth(oriDepth,clipPosUV);
	//float allthick = length(worldSpaceDst.xyz - worldSpaceOri.xyz);
	float allthick = getThickInWorldSpace(oriDepth,clipPosUV,worldSpaceDst);
	testColor = vec4(allthick/(255.0f)*100.0f);
	imageStore(uThickImage,ivec2(gl_FragCoord.xy),ivec4(floatBitsToUint(allthick)));
}

float getThickInWorldSpace(float oriDepth,vec2 clipPosUV,vec4 worldSpaceDst)
{
	vec4 worldSpaceOri = getWorldPosFromDepth(oriDepth,clipPosUV);
	vec2 sampleCoord = clipPosUV*0.5 +vec2(0.5f);
	float allthick = length(worldSpaceDst.xyz - worldSpaceOri.xyz);
	float outObjDistance = 0.0f;
	int i = 1;
	float depth[maxLayer] = {oriDepth,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f};
	int curMaxLayer = 1;
	while(i<2*uDepthNum-1)
	{
		if(i<uDepthNum)
		{
			float curDepth = -texture(frontAndBackDepthSet,vec3(sampleCoord.xy,i)).r;
			float linearCurDepth = linearizeDepth(curDepth);
			if(linearCurDepth + bias <= defaultDepth)
			{
				depth[curMaxLayer] = curDepth;
				curMaxLayer++;
			}
		}
		else
		{
			float curDepth = texture(frontAndBackDepthSet,vec3(sampleCoord.xy,(2*uDepthNum - 1 - i))).g;
			float linearCurDepth = linearizeDepth(curDepth);
			if(linearCurDepth > bias)
			{
				depth[curMaxLayer] = curDepth;
				curMaxLayer++;
			}
		}
		i++;
	}
	for(int index = 1;index < curMaxLayer-1;index +=2)
	{
		vec4 worldPos1 = getWorldPosFromDepth(depth[index],clipPosUV);
		vec4 worldPos2 = getWorldPosFromDepth(depth[index+1],clipPosUV);//��һ�����������
		if(length((worldPos1.xyz + worldPos2.xyz)*0.5 - worldSpaceOri.xyz) >= allthick) break;
		outObjDistance += length(worldPos1.xyz - worldPos2.xyz);
	}
//	for(i = 1;i<uDepthNum;i+=2)
//	{
//		float depth1 = -texture(frontAndBackDepthSet,vec3(sampleCoord.xy,i)).r;
//		if(abs(depth1-1.0)< bias) continue;
//		float depth2 = -texture(frontAndBackDepthSet,vec3(sampleCoord.xy,i+1)).r;
//		vec4 worldPos1 = getWorldPosFromDepth(depth1,clipPosUV);
//		vec4 worldPos2 = getWorldPosFromDepth(depth2,clipPosUV);
//		if(length((worldPos1.xyz + worldPos2.xyz)*0.5 - worldSpaceOri.xyz) >= allthick) break;
//		outObjDistance += length(worldPos1.xyz - worldPos2.xyz);
//	}
//	if(i == uDepthNum)
//	{
//		for(i = uDepthNum-1;i>0;i-=2)
//		{
//			float depth1 = texture(frontAndBackDepthSet,vec3(sampleCoord.xy,i)).g;
//			if(abs(depth1-1.0)< bias) continue;
//			float depth2 = texture(frontAndBackDepthSet,vec3(sampleCoord.xy,i-1)).g;
//			vec4 worldPos1 = getWorldPosFromDepth(depth1,clipPosUV);
//			vec4 worldPos2 = getWorldPosFromDepth(depth2,clipPosUV);
//			if(length((worldPos1.xyz + worldPos2.xyz)*0.5 - worldSpaceOri.xyz) >= allthick) break;
//			outObjDistance += length(worldPos1.xyz - worldPos2.xyz);
//		}
//	}
	//allthick = curMaxLayer;
	//allthick = outObjDistance;
	allthick -= outObjDistance;
	return allthick;
}

float getThickInClipSpace(float oriLinearDepth,float dstLinearDepth,vec2 sampleCoord)
{
	float dstClipDepth = getClipDepthFromLinearZ(dstLinearDepth);
	float oriClipDepth = getClipDepthFromLinearZ(oriLinearDepth);
	float allthick = dstClipDepth - oriClipDepth;
	for(int i = 1;i<uDepthNum;i+=2)
	{
		float depth1 = texture(frontAndBackDepthSet,vec3(sampleCoord.xy,i)).r;
		depth1 = linearizeDepth(depth1);
		if(abs(depth1-1.0)< bias) continue;
		float depth2 = texture(frontAndBackDepthSet,vec3(sampleCoord.xy,i+1)).r;
		depth2 = linearizeDepth(depth2);
		if((depth1 +depth2)*0.5 > dstLinearDepth) break;
		allthick -= getClipDepthFromLinearZ(depth2) - getClipDepthFromLinearZ(depth1);
	}
	return allthick;
}

vec4 getWorldPosFromDepth(float depth,vec2 clipPosUV)
{
	vec4 worldPos = vec4(0.0f);
	float oriLinearDepth = linearizeDepth(depth);//0-1
	float zc = -real_near_plane+oriLinearDepth*(real_near_plane+real_far_plane);
	float ze = returnZe(depth);
	float wc = -ze;
	float xc = clipPosUV.x * wc;
	float yc = clipPosUV.y * wc;
	vec4 clipPos = vec4(xc,yc,zc,wc);
	worldPos = inverse(realLightProjection * realLightView) * clipPos;
	return worldPos;
}


//��������任�Ĺ���
//0.͸��ͶӰ������Ƶ�����������ͶӰ����Ļ����ϣ��Ƚ�Զƽ�����ɺͽ�ƽ��һ����С
//1.���Ƚ���һ������Ҫ�����⣬Ϊʲôview space�µ�z�ķ�ΧΪ-n��-f��������Ϊview space��dir�ᣬҲ����z�ᣬ����ָ��target��
//����ָ��target�ķ�����ģ��������ܹ�������real_near_plane,real_far_plane֮������壬������view space�¶���λ��z��ĸ������ϵģ�Ҳ����-n,-f
//2.д��ͶӰ�����������ʽ(���ԳƵ����)������view�ռ��µ����꣬���Եó�wc = -ze��Ȼ��zc = -(f+n)/(f-n) * ze - 2fn/(f - n)
//3.������Ϊ֮��Ҫ��zc/wc�Ĳ�������˻����͸��ͶӰ�µ�zֵ�������Եģ��任�����Եķ�������ze = 2fn/((zndc)(f-n)-(f+n)),ע�������ze��-n��-f
//4.��˿��Կ�������������Ǹ��任����ĸ���෴�ģ���˷��ص���-ze����ΧΪreal_near_plane��real_far_plane
//5.��zc = -2fn * zndc/(zndc(f-n)-(f+n)),���ʹ��linearz��ʾ�Ļ���zc = -n+zl(f+n),zc��ȡֵ��Χ��[-n,f]



