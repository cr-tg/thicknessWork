#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

layout(binding = 0) uniform sampler2D outputTex;

void main()
{
	FragColor = texture(outputTex,TexCoords);
}