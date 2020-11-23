#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

layout(binding = 0)uniform sampler2D listColor;
layout(binding = 0,r32ui) uniform restrict uimage2D uThickImage;
layout(binding = 4,r32ui) uniform restrict uimage2D uSdfThickImage;
layout(binding = 5,r32f) uniform restrict image2D ErrorImage;

uniform float near_plane;//光源的near/far
uniform float far_plane;
uniform vec2 uScreenSize;

#define renderingShowSize 0.0f

void main()
{  
	if(false)
	{ 
		uint thick = imageLoad(uThickImage,ivec2(gl_FragCoord.xy)).r;
		float showThick = uintBitsToFloat(thick)/far_plane;
		//FragColor = vec4(thick);
		FragColor = vec4(showThick);
		return;
	}
	if(gl_FragCoord.x > uScreenSize.x * renderingShowSize && gl_FragCoord.y > uScreenSize.y * renderingShowSize)
	{
		uint thick = imageLoad(uSdfThickImage,ivec2((gl_FragCoord.xy-renderingShowSize*uScreenSize) * (1.0/(1.0-renderingShowSize)))).r;
		//uint thick = imageLoad(uThickImage,ivec2((gl_FragCoord.xy-renderingShowSize*uScreenSize) * (1.0/(1.0-renderingShowSize)))).r;
		float showThick = uintBitsToFloat(thick)/255.0f*100.0f;
		float error = imageLoad(ErrorImage,ivec2((gl_FragCoord.xy-renderingShowSize*uScreenSize) * (1.0/(1.0-renderingShowSize)))).r;
		FragColor = vec4(error * 10000.0f/255.0f);//正面
		//FragColor = vec4(error * 1000.0f/255.0f);//侧面
		//FragColor = vec4(showThick);
		return;
	}
	FragColor = vec4(texture(listColor,vec2(TexCoords)));
	//FragColor = vec4(texture(listColor,vec2(TexCoords))/255.0f*10.0f);
}