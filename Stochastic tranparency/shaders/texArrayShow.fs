#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2DArray uTextureArray;
uniform int uLayer;

void main()
{
	FragColor = texture(uTextureArray,vec3(TexCoords,uLayer));
}
