#version 460 core
//layout(binding = 0) uniform writeonly uimage2D uInputImage;
layout(binding = 0) uniform writeonly image2D InputImage;

void main()
{
	vec4 fragColor = vec4(1.0,0.5,0.2,1.0);
	uvec4 uFragColor = uvec4(fragColor*255);
	//imageStore(uInputImage,ivec2(gl_FragCoord.xy),uvec4(255));
	imageStore(InputImage,ivec2(gl_FragCoord.xy),fragColor);
}