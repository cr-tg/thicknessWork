#version 460 core
//layout(binding = 0,r32ui) uniform uimage2D uTestImage0;
layout(binding = 0,rgba8) uniform image2D uTestImage0;
layout(binding = 0) uniform sampler2D uTestImage0T;
layout(binding = 1,r32ui) uniform uimage2D uTestImage1;
layout(binding = 1) uniform sampler2D uTestImage1T;

in vec2 TexCoords;
out vec4 FragColor;

void main()
{
	vec2 size = imageSize(uTestImage0);
	FragColor = imageLoad(uTestImage0,ivec2(size * TexCoords));
	//FragColor= texelFetch(uTestImage0T,ivec2(gl_FragCoord.xy),0);
	//FragColor = imageLoad(uTestImage0,ivec2(gl_FragCoord.xy));
	//FragColor = vec4(0.0f,1.0,1.0,1.0);
}