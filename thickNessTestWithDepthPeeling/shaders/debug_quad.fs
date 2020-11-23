#version 460 core
out vec4 FragColor;
in vec2 TexCoords;

//注意colorTexSet中存放的是每层blend以后的结果了，因为在blend的pass每次都会更新colorTexSet的值
layout(binding = 0) uniform sampler2DArray colorTexSet;
layout(binding = 1) uniform sampler2DArray depthTexSet;
layout(binding = 0,r32ui) uniform restrict uimage2D uThickImage;
layout(binding = 5,r32f) uniform restrict image2D ErrorImage;


#define ySmallerSize 5.0
uniform int uDepthNum;
uniform vec2 uScreenSize;
uniform float near_plane;
uniform float far_plane;

#define renderingShowSize 0.0f
#define depthShowLayer 5

// required when using a perspective projection matrix
//1.0 ->farplane 0.0 ->near_plane
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC 
	z = (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));	
    return (z-near_plane)/(far_plane - near_plane);
}

void main()
{
	if(!true)
	{
		float depth = texture(depthTexSet, vec3(TexCoords, depthShowLayer), 0).r;
		depth = LinearizeDepth(depth);
		FragColor = vec4(depth);
		return;
	}
	if(false)
	{ 
		float thick = 0.0f;
		float bias = 1e-4;
		for(int i = uDepthNum-1;i>0;i-=2)
		{
			float lastDepth = texelFetch(depthTexSet,ivec3(gl_FragCoord.xy,i),0).r;
			if(abs(lastDepth-1.0)< bias) continue;
			lastDepth = LinearizeDepth(lastDepth);
			float preDepth = texelFetch(depthTexSet,ivec3(gl_FragCoord.xy,i-1),0).r;
			preDepth = LinearizeDepth(preDepth);
			thick += abs(lastDepth - preDepth);
		}
		if(abs(thick-0.0)< bias)
			FragColor = vec4(1.0f);
		else
			FragColor = vec4(thick/far_plane);
		return;
	}
	if(gl_FragCoord.x > uScreenSize.x * renderingShowSize && gl_FragCoord.y > uScreenSize.y * renderingShowSize)
	{
//		float thick = 0.0f;
//		float bias = 1e-4;
//		for(int i = uDepthNum-1;i>-1;i-=2)
//		{
//			float lastDepth = texelFetch(depthTexSet,ivec3((gl_FragCoord.xy-renderingShowSize*uScreenSize)*(1.0/(1.0-renderingShowSize)),i),0).r;
//			lastDepth = LinearizeDepth(lastDepth);
//			if(abs(lastDepth-1.0)< bias) continue;
//			float preDepth = texelFetch(depthTexSet,ivec3((gl_FragCoord.xy-renderingShowSize*uScreenSize)*(1.0/(1.0-renderingShowSize)),i-1),0).r;
//			preDepth = LinearizeDepth(preDepth);
//			thick += abs(lastDepth - preDepth);
//		}
//		if(abs(thick-0.0)< bias)
//			FragColor = vec4(0.0f);
//		else
//			//float showThick = uintBitsToFloat(thick)/255.0f*100.0f;
//			FragColor = vec4(thick/255.0f*10.0f);
		uint thick = imageLoad(uThickImage,ivec2((gl_FragCoord.xy-renderingShowSize*uScreenSize) * (1.0/(1.0-renderingShowSize)))).r;
		float showThick = uintBitsToFloat(thick)/255.0f*100.0f;
		float error = imageLoad(ErrorImage,ivec2((gl_FragCoord.xy-renderingShowSize*uScreenSize) * (1.0/(1.0-renderingShowSize)))).r;
		FragColor = vec4(error/255.0f*100.0f);//正面的误差
		//FragColor = vec4(error * 10000.0f/255.0f);//正面的误差
		//FragColor = vec4(error * 1000.0f/255.0f);//侧面的误差
		return;
	}
	if(gl_FragCoord.y > uScreenSize.y * 0.0)//0.2,0.2
	{
		FragColor = vec4(texture(colorTexSet,vec3(TexCoords,0.0))); 
		return;
	}
	vec4 color = vec4(0.0);
	float interval = 1.0/uDepthNum;
	for(int i=1;i<=uDepthNum;++i)
	{
		if(TexCoords.x < float(i) * interval)
		{
			//color = texture(colorTexSet, vec3((TexCoords - vec2((i-1.0) * interval, 0.0)) * vec2(uDepthNum, ySmallerSize), i-1));
			float depth = texture(depthTexSet, vec3((TexCoords - vec2((i-1.0) * interval, 0.0)) * vec2(uDepthNum, ySmallerSize), i-1)).r;
			depth = LinearizeDepth(depth);
			color = vec4(depth);
			break;
		}
	}
   FragColor = vec4(pow(vec3(color.rgb),vec3(1.0)), color.a); 
   //FragColor = vec4(pow(vec3(color.rgb),vec3(1.0/2.2)), color.a); 
}