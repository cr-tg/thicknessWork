#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

layout(binding = 0) uniform sampler2D listColor;

void main()
{  
	FragColor = vec4(texture(listColor,vec2(TexCoords))); 
	//FragColor = pow(vec4(texture(listColor,vec2(TexCoords))),vec4(1.0/2.2)); 
}