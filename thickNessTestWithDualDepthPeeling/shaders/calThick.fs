//之前的depth_peeling和OIT都是真实光源位置不变，然后从光源位置出发，假设我们的相机位置也始终不变且处于场景的前端
//这样计算出来的厚度始终是光线穿过了整个场景的厚度，而且即使相机位置移动了，生成的厚度也不会改变，生成的厚度并不是当前相机看到的着色点的厚度
//而是一个固定位置的相机看到的着色点的厚度，这样做的理由是为了测试距离计算的准确与否

//因此这里我直接传入的就是固定在006的相机位置，然后这个相机下看到的所有片段就是着色点
//具体来说，就是在固定相机视角下做defferd shading传出去worldPos，然后使用纹理保存，再在这个pass中对纹理进行采样即可
#version 460 core
layout(binding = 0) uniform sampler2D gPosition;//着色点的世界坐标
layout(binding = 1) uniform sampler2DArray frontAndBackDepthSet;
layout(binding = 2) uniform sampler2D frontAndBackTex;
layout(binding = 0,r32ui) uniform restrict writeonly uimage2D uThickImage;


uniform mat4 realLightProjection;
uniform mat4 realLightView;
uniform float real_near_plane;
uniform float real_far_plane;
uniform int uDepthNum;


#define PROJECTION_MODE PROJECTION_PERSPECTIVE

//1e-3 才检测的出来
#define bias 1e-3
#define defaultDepth 1.0f
#define maxLayer 10

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
float getThickInClipSpace(float oriLinearDepth,float dstLinearDepth,vec2 sampleCoord);
float getThickInWorldSpace(float oriDepth,vec2 clipPosUV,vec4 worldSpaceDst);

void main()
{
	vec4 worldSpaceDst = texelFetch(gPosition,ivec2(gl_FragCoord.xy),0);
	//testColor = vec4(worldSpaceDst.xy,worldSpaceDst.z/10.f,0.0f);
	mat4 realLightPV = realLightProjection * realLightView;
	vec4 lightClipSpaceDst = realLightPV * worldSpaceDst;//裁剪坐标，-w,w的范围，除以w以后，变换到-1,1的范围
	vec3 sampleCoord = lightClipSpaceDst.xyz / lightClipSpaceDst.w;// 采样坐标，从shadowmap中进行采样，得到DST的ORI的z值。
	sampleCoord = sampleCoord * 0.5 +0.5;//xy也是在[-1，1]
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
		vec4 worldPos2 = getWorldPosFromDepth(depth[index+1],clipPosUV);//后一层的世界坐标
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


//关于坐标变换的过程
//0.透视投影矩阵的推导就是在正交投影矩阵的基础上，先将远平面拉成和近平面一样大小
//1.首先解释一个最重要的问题，为什么view space下的z的范围为-n，-f，这是因为view space的dir轴，也就是z轴，不是指向target的
//而是指向target的反方向的，因此相机能够看到的real_near_plane,real_far_plane之间的物体，他们在view space下都是位于z轴的负半轴上的，也就是-n,-f
//2.写出投影矩阵的特殊形式(即对称的情况)，乘以view空间下的坐标，可以得出wc = -ze，然后zc = -(f+n)/(f-n) * ze - 2fn/(f - n)
//3.正是因为之后要做zc/wc的操作，因此会造成透视投影下的z值不是线性的，变换成线性的方法就是ze = 2fn/((zndc)(f-n)-(f+n)),注意这里的ze是-n，-f
//4.因此可以看到我们上面的那个变换，分母是相反的，因此返回的是-ze，范围为real_near_plane，real_far_plane
//5.而zc = -2fn * zndc/(zndc(f-n)-(f+n)),如果使用linearz表示的话，zc = -n+zl(f+n),zc的取值范围是[-n,f]



