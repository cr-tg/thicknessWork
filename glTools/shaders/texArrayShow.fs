#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2DArray uTextureArray;
uniform int uLayer;

float real_near_plane = 0.01f, real_far_plane = 5.0f;
float returnZe(float depth);
float linearizeDepth(float depth);

void main()
{
	FragColor = texture(uTextureArray,vec3(TexCoords,uLayer));
	float minDepth = -texture(uTextureArray,vec3(TexCoords,uLayer)).r;
	minDepth = linearizeDepth(minDepth);
	float maxDepth = texture(uTextureArray,vec3(TexCoords,uLayer)).g;
	maxDepth = linearizeDepth(maxDepth);
	FragColor = vec4(minDepth,maxDepth,0.0,1.0);
}


float returnZe(float depth)
{
	float z = depth * 2.0 - 1.0; // back to NDC 
	z = (2.0 * real_near_plane * real_far_plane) / (z * (real_far_plane - real_near_plane) - (real_far_plane + real_near_plane)); // range: -real_near_plane...-real_far_plane,
	return z;
}

float linearizeDepth(float depth)
{
	float z = -returnZe(depth);
	z = (z - real_near_plane) / (real_far_plane - real_near_plane); // range: 0...1
	return z;
}
