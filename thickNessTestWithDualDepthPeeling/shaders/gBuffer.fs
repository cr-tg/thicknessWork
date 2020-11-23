#version 460 core
layout (location = 0) out vec4 gPosition;

in vec4 worldPos;

void main()
{
	gPosition = worldPos;
}

