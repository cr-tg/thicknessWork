//之前的depth_peeling和OIT都是真实光源位置不变，然后从光源位置出发，假设我们的相机位置也始终不变且处于场景的前端
//这样计算出来的厚度始终是光线穿过了整个场景的厚度，而且即使相机位置移动了，生成的厚度也不会改变，生成的厚度并不是当前相机看到的着色点的厚度
//而是一个固定位置的相机看到的着色点的厚度，这样做的理由是为了测试距离计算的准确与否

//因此这里我直接传入的就是固定在006的相机位置，然后这个相机下看到的所有片段就是着色点
//具体来说，就是在固定相机视角下做defferd shading传出去worldPos，然后使用纹理保存，再在这个pass中对纹理进行采样即可
#version 460 core
layout(binding = 0) uniform sampler2D gPosition;//着色点的世界坐标
layout(binding = 1) uniform sampler2D shadowMapRealLight;//真实光源下的shadow map
layout(binding = 2) uniform sampler2D gNormal;//着色点的世界坐标
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
	//uvec2 nodeDataLight;//x存储next指针，y深度
};

layout(std430,binding = 1) coherent restrict buffer ssboLight
{
	ListNodeLight nodeLightSet[];
};


out vec4 testColor;

float returnZe(float depth)
{
	float z = depth * 2.0 - 1.0; // back to NDC 
	//ndcz = -1，ze = -real_near_plane
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

//根据目标点的非线性深度，投影矩阵的远近平面，在裁剪坐标系下的UV坐标和pv矩阵求出这个点的世界坐标
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
	vec4 lightClipSpaceDst = realLightPV * worldSpaceDst;//裁剪坐标，-w,w的范围，除以w以后，变换到-1,1的范围
	vec3 sampleCoord = lightClipSpaceDst.xyz / lightClipSpaceDst.w;// 采样坐标，从shadowmap中进行采样，得到DST的ORI的z值。
	sampleCoord = sampleCoord * 0.5 +0.5;//xy也是在[-1，1]
	ivec2 headPtrImageSize = imageSize(uLightHeadPtrTex);
	ivec2 iSampleCoord = ivec2(headPtrImageSize * sampleCoord.xy);
	uint firstIndex = imageLoad(uLightHeadPtrTex,iSampleCoord).r;
//	if(firstIndex == 0) //如果写了这句话，会导致一些本来应该在圆环上的坐标消失了，还是采样导致的误差
//		return;
	float oriDepth = uintBitsToFloat(nodeLightSet[firstIndex].depth);//存储的是gl_FragCoord.z
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
	float allthick = dstClipDepth - oriClipDepth; //使用的是裁剪坐标系下的z值的差值
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





//关于坐标变换的过程
//0.透视投影矩阵的推导就是在正交投影矩阵的基础上，先将远平面拉成和近平面一样大小
//1.首先解释一个最重要的问题，为什么view space下的z的范围为-n，-f，这是因为view space的dir轴，也就是z轴，不是指向target的
//而是指向target的反方向的，因此相机能够看到的real_near_plane,real_far_plane之间的物体，他们在view space下都是位于z轴的负半轴上的，也就是-n,-f
//2.写出投影矩阵的特殊形式(即对称的情况)，乘以view空间下的坐标，可以得出wc = -ze，然后zc = -(f+n)/(f-n) * ze - 2fn/(f - n)
//3.正是因为之后要做zc/wc的操作，因此会造成透视投影下的z值不是线性的，变换成线性的方法就是ze = 2fn/((zndc)(f-n)-(f+n)),注意这里的ze是-n，-f
//4.因此可以看到我们上面的那个变换，分母是相反的，因此返回的是-ze，范围为real_near_plane，real_far_plane
//5.而zc = -2fn * zndc/(zndc(f-n)-(f+n)),如果使用linearz表示的话，zc = -n+zl(f+n),zc的取值范围是[-n,f]



