#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2DMS uMsaaTexture;

void main()
{
	ivec2 p = ivec2(gl_FragCoord.xy);
	FragColor = texelFetch(uMsaaTexture,p,0);
}
