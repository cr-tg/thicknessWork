#version 460 core
uniform vec4 uObjColor;
out vec4 fragColor;

void main()
{
	fragColor = vec4(uObjColor);
}