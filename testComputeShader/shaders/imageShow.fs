#version 460 core
layout(binding = 0,r32ui) uniform uimage2D uListHeadPtrTex;
layout(binding = 0) uniform sampler2D uListHeadPtrTexT;

//layout(binding = 0,rgba8) uniform image2D uListHeadPtrTex;

in vec2 TexCoords;
out vec4 FragColor;

void main()
{
	uint t = uint(imageLoad(uListHeadPtrTex,ivec2(gl_FragCoord.xy)).r);//640亿
	uint curretIndex = uint(texelFetch(uListHeadPtrTexT,ivec2(gl_FragCoord.xy),0).r);//0
	uint oldIndex = imageAtomicExchange(uListHeadPtrTex, ivec2(gl_FragCoord.xy), 0);

	if(curretIndex != 0)
		FragColor = vec4(0.0f,1.0,1.0,1.0);
	else	
		FragColor = vec4(t)/1000000000.0f;
	//FragColor = vec4(t/255.0,t/100000.0f,0.0,1.0f);
	//vec2 size = imageSize(uListHeadPtrTex);
	//FragColor = imageLoad(uListHeadPtrTex,ivec2(size * TexCoords));
	//FragColor = imageLoad(uListHeadPtrTex,ivec2(gl_FragCoord.xy));
	//读出来的数字是600,0000,0000 f
	//42,9496,7295
	//687,1947,6735
	//FragColor = vec4(0.0f,1.0,1.0,1.0);
}