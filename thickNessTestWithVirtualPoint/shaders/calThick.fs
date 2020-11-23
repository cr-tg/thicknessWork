//之前的depth_peeling和OIT都是真实光源位置不变，然后从光源位置出发，假设我们的相机位置也始终不变且处于场景的前端
//这样计算出来的厚度始终是光线穿过了整个场景的厚度，而且即使相机位置移动了，生成的厚度也不会改变，生成的厚度并不是当前相机看到的着色点的厚度
//而是一个固定位置的相机看到的着色点的厚度，这样做的理由是为了测试距离计算的准确与否

//因此这里我直接传入的就是固定在006的相机位置，然后这个相机下看到的所有片段就是着色点
//具体来说，就是在固定相机视角下做defferd shading传出去worldPos，然后使用纹理保存，再在这个pass中对纹理进行采样即可
#version 460 core
layout(binding = 0) uniform sampler2D gPosition;//着色点的世界坐标
layout(binding = 1) uniform sampler2D shadowMapRealLight;//真实光源下的shadow map
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
	//uvec2 nodeDataLight;//x存储next指针，y深度
};

layout(std430,binding = 1) coherent restrict buffer ssboLight
{
	ListNodeLight nodeLightSet[];
};


out vec4 testColor;

float returnZe(float depth, float near, float far)
{
	float z = depth * 2.0 - 1.0; // back to NDC 
	//ndcz = -1，ze = -near
	z = (2.0 * near * far) / (z * (far - near) - (far + near)); // range: -near...-far,
	return z;
}

float linearizeDepth(float depth, float near, float far)
{
	float z = -returnZe(depth, near, far);
	z = (z - near) / (far - near); // range: 0...1
	return z;
}

//根据目标点的非线性深度，投影矩阵的远近平面，在裁剪坐标系下的UV坐标和pv矩阵求出这个点的世界坐标
vec4 getWorldPosFromDepth(float depth,float nearPlane,float farPlane,vec2 clipPosUV,mat4 pvMat4)
{
	vec4 worldPos = vec4(0.0f);
	float oriLinearDepth = linearizeDepth(depth,nearPlane,farPlane);//0-1
	float bias = 1e-5;
	if(oriLinearDepth >= 1.0 -bias)//减少在深度图以外的采样，g3d中似乎从gbuffer中拿片段的时候就已经自动做了这一步
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
	vec4 lightClipSpaceDst = realLightPV * worldSpaceDst;//裁剪坐标，-w,w的范围，除以w以后，变换到-1,1的范围
	vec2 sampleCoord = lightClipSpaceDst.xy / lightClipSpaceDst.w;// 采样坐标，从shadowmap中进行采样，得到DST的ORI的z值。
	sampleCoord = sampleCoord * 0.5 +0.5;//xy也是在[-1，1]
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
				float toDst = t;//从采样点到dst的距离，保持从ori指向dst为正
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
				float virFragCoordz = virtualSpaceSamplePos.z; //记录在fragCoord中的z值，是非线性的

				ivec2 isampleCoord = ivec2(lightHeadImageSize * virtualSpaceSamplePos.xy);
				uint index = imageLoad(uLightHeadPtrTex, isampleCoord).r;//这句话的性能损失相当大

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
				//修改currentStep
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
	//testColor = worldSpaceOri/255.0f*10.0f;//都是22 -> 2.2
	//testColor = worldSpaceDst/255.0f*10.0f;//都是42 -> 4.2
	//testColor = worldSpaceDst/255.0f*10.0f;//都是42 -> 4.2
	allthick *= (inObjectDis / disInWorld);
	//imageStore(uThickImage,ivec2(gl_FragCoord.xy),ivec4(floatBitsToUint(inObjectDis)));
	//imageStore(uThickImage,ivec2(gl_FragCoord.xy),ivec4(inTimes));
	//imageStore(uThickImage,ivec2(gl_FragCoord.xy),ivec4(floatBitsToUint(times)));
	testColor = vec4(allthick/255.0f*100.0f);
	imageStore(uThickImage,ivec2(gl_FragCoord.xy),ivec4(floatBitsToUint(allthick)));
}


//关于坐标变换的过程
//0.透视投影矩阵的推导就是在正交投影矩阵的基础上，先将远平面拉成和近平面一样大小
//1.首先解释一个最重要的问题，为什么view space下的z的范围为-n，-f，这是因为view space的dir轴，也就是z轴，不是指向target的
//而是指向target的反方向的，因此相机能够看到的near,far之间的物体，他们在view space下都是位于z轴的负半轴上的，也就是-n,-f
//2.写出投影矩阵的特殊形式(即对称的情况)，乘以view空间下的坐标，可以得出wc = -ze，然后zc = -(f+n)/(f-n) * ze - 2fn/(f - n)
//3.正是因为之后要做zc/wc的操作，因此会造成透视投影下的z值不是线性的，变换成线性的方法就是ze = 2fn/((zndc)(f-n)-(f+n)),注意这里的ze是-n，-f
//4.因此可以看到我们上面的那个变换，分母是相反的，因此返回的是-ze，范围为near，far
//5.而zc = -2fn * zndc/(zndc(f-n)-(f+n)),如果使用linearz表示的话，zc = -n+zl(f+n),zc的取值范围是[-n,f]



