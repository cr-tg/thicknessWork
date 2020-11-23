#version 460 core

layout(location = 0) out vec2 depth;

uniform vec4 uObjColor;


void main()
{
	depth = vec2(-gl_FragCoord.z,gl_FragCoord.z);
	//depth = vec2(-depthZero2One,depthZero2One);
}