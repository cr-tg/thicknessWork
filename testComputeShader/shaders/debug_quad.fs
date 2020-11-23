#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

layout(binding = 0)uniform sampler2D inputImage;
layout(binding = 1)uniform sampler2D outputImage;
//uniform int uDepthLayer;

#define ySmallerSize 5.0

void main()
{  
	FragColor = vec4(texelFetch(inputImage,ivec2(gl_FragCoord.xy),0)); 
	//FragColor = vec4(texelFetch(outputImage,ivec2(gl_FragCoord.xy),0)); 
	//FragColor = vec4(texture(listColor,vec2(TexCoords))); 
	//FragColor = pow(vec4(texture(listColor,vec2(TexCoords))),vec4(1.0/2.2)); 
}