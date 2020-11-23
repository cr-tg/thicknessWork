#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

layout(binding = 0)uniform sampler2D listColor;
layout(binding = 1)uniform sampler2D finalImageColor;
//uniform int uDepthLayer;

#define ySmallerSize 5.0

void main()
{  
	FragColor = vec4(texture(listColor,vec2(TexCoords))); 
	//FragColor = vec4(texelFetch(finalImageColor,ivec2(gl_FragCoord.xy),0)); 
	//FragColor = pow(vec4(texture(listColor,vec2(TexCoords))),vec4(1.0/2.2)); 
}