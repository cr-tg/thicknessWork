//֮ǰ��depth_peeling��OIT������ʵ��Դλ�ò��䣬Ȼ��ӹ�Դλ�ó������������ǵ����λ��Ҳʼ�ղ����Ҵ��ڳ�����ǰ��
//������������ĺ��ʼ���ǹ��ߴ��������������ĺ�ȣ����Ҽ�ʹ���λ���ƶ��ˣ����ɵĺ��Ҳ����ı䣬���ɵĺ�Ȳ����ǵ�ǰ�����������ɫ��ĺ��
//����һ���̶�λ�õ������������ɫ��ĺ�ȣ���������������Ϊ�˲��Ծ�������׼ȷ���

//���������ֱ�Ӵ���ľ��ǹ̶���006�����λ�ã�Ȼ���������¿���������Ƭ�ξ�����ɫ��
//������˵�������ڹ̶�����ӽ�����defferd shading����ȥworldPos��Ȼ��ʹ�������棬�������pass�ж�������в�������
#version 460 core
layout(binding = 0) uniform sampler2D gPosition;//��ɫ�����������
layout(binding = 1) uniform sampler2D shadowMapRealLight;//��ʵ��Դ�µ�shadow map
layout(binding = 2,r32ui) uniform restrict uimage2D uLightHeadPtrTex;
layout(binding = 3,r32ui) uniform restrict writeonly uimage2D uThickImage;

uniform mat4 realLightProjection;
uniform mat4 realLightView;
uniform float real_near_plane;
uniform float real_far_plane;

uniform mat4 virLightProjection;
uniform mat4 virLightView;
uniform float vir_near_plane;
uniform float vir_far_plane;
//0.004f 500
#define VIRTUAL_MIN_SAMPLE_STEP 0.002f
#define VIRTUAL_MOST_STEP_NUM 1000
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

float returnZe(float depth, float near, float far)
{
	float z = depth * 2.0 - 1.0; // back to NDC 
	//ndcz = -1��ze = -near
	z = (2.0 * near * far) / (z * (far - near) - (far + near)); // range: -near...-far,
	return z;
}

float linearizeDepth(float depth, float near, float far)
{
	float z = -returnZe(depth, near, far);
	z = (z - near) / (far - near); // range: 0...1
	return z;
}

//����Ŀ���ķ�������ȣ�ͶӰ�����Զ��ƽ�棬�ڲü�����ϵ�µ�UV�����pv���������������������
vec4 getWorldPosFromDepth(float depth,float nearPlane,float farPlane,vec2 clipPosUV,mat4 pvMat4)
{
	vec4 worldPos = vec4(0.0f);
	float oriLinearDepth = linearizeDepth(depth,nearPlane,farPlane);//0-1
	float bias = 1e-5;
	if(oriLinearDepth >= 1.0 -bias)//���������ͼ����Ĳ�����g3d���ƺ���gbuffer����Ƭ�ε�ʱ����Ѿ��Զ�������һ��
		return vec4(2.0 * farPlane);
	float zc = -nearPlane+oriLinearDepth*(nearPlane+farPlane);
	float ze = returnZe(depth,nearPlane,farPlane);
	float wc = -ze;
	float xc = clipPosUV.x * wc;
	float yc = clipPosUV.y * wc;
	vec4 clipPos = vec4(xc,yc,zc,wc);
	worldPos = inverse(pvMat4) * clipPos;
	return worldPos;
}


void main()
{
	vec4 worldSpaceDst = texelFetch(gPosition,ivec2(gl_FragCoord.xy),0);
	mat4 realLightPV = realLightProjection * realLightView;
	vec4 lightClipSpaceDst = realLightPV * worldSpaceDst;//�ü����꣬-w,w�ķ�Χ������w�Ժ󣬱任��-1,1�ķ�Χ
	vec2 sampleCoord = lightClipSpaceDst.xy / lightClipSpaceDst.w;// �������꣬��shadowmap�н��в������õ�DST��ORI��zֵ��
	sampleCoord = sampleCoord * 0.5 +0.5;//xyҲ����[-1��1]
	float oriDepth = texture(shadowMapRealLight,sampleCoord).r;
	float oriLinearDepth = linearizeDepth(oriDepth,real_near_plane,real_far_plane);//0-1


	vec2 clipSpaceUV = lightClipSpaceDst.xy/lightClipSpaceDst.w;
	vec4 worldSpaceOri = getWorldPosFromDepth(oriDepth,real_near_plane,real_far_plane,clipSpaceUV,realLightPV);
	if(worldSpaceOri == vec4(2.0 * real_far_plane))
		return;
	float disInWorld = length(worldSpaceDst.xyz - worldSpaceOri.xyz);//2.0
	vec3 dir = normalize(worldSpaceDst.xyz - worldSpaceOri.xyz);
	float allthick = disInWorld;

	float minStep = max(VIRTUAL_MIN_SAMPLE_STEP,disInWorld/VIRTUAL_MOST_STEP_NUM);
	float currentSampleStep = minStep;
	float currentStep = currentSampleStep;
	float maxStep = 16.0 * minStep;
	float inObjectDis = 0.0f;
	int prevOcclusionCount = 1;
	vec4 samplePosDst = worldSpaceDst;
	float estimatedThickness = 0.0f;
	mat4 virtualLightViewProjection = virLightProjection * virLightView;
	ivec2 lightHeadImageSize = imageSize(uLightHeadPtrTex);
	uint times = 0;
	uint inTimes = 0;
	if(disInWorld <= minStep)
		estimatedThickness = disInWorld;
	else
	{
		for (vec3 samplePos = worldSpaceOri.xyz + minStep * dir; ; samplePos += currentSampleStep * dir)
			{
				times++;
				currentSampleStep = currentStep;
				float t = dot(samplePosDst.xyz - samplePos, dir);
				float toDst = t;//�Ӳ����㵽dst�ľ��룬���ִ�oriָ��dstΪ��
				if (t <= 0.0)
				{
					if (t <= 0.0)
						toDst = abs(currentSampleStep + t);
					if (currentStep <= minStep)
					{
						inObjectDis += toDst;
						break;
					}
				}

				vec4 virtualSpaceSamplePos = virtualLightViewProjection * vec4(samplePos, 1.0f);
				virtualSpaceSamplePos.xyz = virtualSpaceSamplePos.xyz / virtualSpaceSamplePos.w;
				virtualSpaceSamplePos.xyz = virtualSpaceSamplePos.xyz * 0.5 + vec3(0.5);
				float virFragCoordz = virtualSpaceSamplePos.z; //��¼��fragCoord�е�zֵ���Ƿ����Ե�

				ivec2 isampleCoord = ivec2(lightHeadImageSize * virtualSpaceSamplePos.xy);
				uint index = imageLoad(uLightHeadPtrTex, isampleCoord).r;//��仰��������ʧ�൱��

				int occlusionCount = 0;
				int maxInside = 100;
				int inside = 0;
			
				while (index != 0 && inside < maxInside)
				{
					ListNodeLight node = nodeLightSet[index];
					if (uintBitsToFloat(node.depth) >= virFragCoordz) break;
					occlusionCount++;
					index = node.next;
					inside++;
				}
#if ENABLE_ADAPTIVE_SAMPLING == 1
				//�޸�currentStep
#else
				if(occlusionCount % 2 == 1)
				{
					inTimes++;
					inObjectDis += currentStep;
				}
#endif
				prevOcclusionCount = occlusionCount;
			}
	}

//	testColor = vec4(oriDepth);//117 OK
	
//	testColor = vec4(abs(worldSpaceDst.xyz - worldSpaceOri.xyz)/255.0f*10.0f,1.0);//2.0
//	testColor = vec4(disInWorld/255.0f*10.0f);//2.0
	//testColor = worldSpaceOri/255.0f*10.0f;//����22 -> 2.2
	//testColor = worldSpaceDst/255.0f*10.0f;//����42 -> 4.2
	//testColor = worldSpaceDst/255.0f*10.0f;//����42 -> 4.2
	allthick *= (inObjectDis / disInWorld);
	//imageStore(uThickImage,ivec2(gl_FragCoord.xy),ivec4(floatBitsToUint(inObjectDis)));
	//imageStore(uThickImage,ivec2(gl_FragCoord.xy),ivec4(inTimes));
	//imageStore(uThickImage,ivec2(gl_FragCoord.xy),ivec4(floatBitsToUint(times)));
	testColor = vec4(allthick/255.0f*100.0f);
	imageStore(uThickImage,ivec2(gl_FragCoord.xy),ivec4(floatBitsToUint(allthick)));
}


//��������任�Ĺ���
//0.͸��ͶӰ������Ƶ�����������ͶӰ����Ļ����ϣ��Ƚ�Զƽ�����ɺͽ�ƽ��һ����С
//1.���Ƚ���һ������Ҫ�����⣬Ϊʲôview space�µ�z�ķ�ΧΪ-n��-f��������Ϊview space��dir�ᣬҲ����z�ᣬ����ָ��target��
//����ָ��target�ķ�����ģ��������ܹ�������near,far֮������壬������view space�¶���λ��z��ĸ������ϵģ�Ҳ����-n,-f
//2.д��ͶӰ�����������ʽ(���ԳƵ����)������view�ռ��µ����꣬���Եó�wc = -ze��Ȼ��zc = -(f+n)/(f-n) * ze - 2fn/(f - n)
//3.������Ϊ֮��Ҫ��zc/wc�Ĳ�������˻����͸��ͶӰ�µ�zֵ�������Եģ��任�����Եķ�������ze = 2fn/((zndc)(f-n)-(f+n)),ע�������ze��-n��-f
//4.��˿��Կ�������������Ǹ��任����ĸ���෴�ģ���˷��ص���-ze����ΧΪnear��far
//5.��zc = -2fn * zndc/(zndc(f-n)-(f+n)),���ʹ��linearz��ʾ�Ļ���zc = -n+zl(f+n),zc��ȡֵ��Χ��[-n,f]



