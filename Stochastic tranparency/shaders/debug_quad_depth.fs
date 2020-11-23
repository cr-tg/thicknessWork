#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2DArray colorTexSet;
uniform sampler2D colorTex;
uniform sampler2D shadowMap;
float near_plane = 0.1f, far_plane = 10.0f;//far_planer太大会导致精度降低


// required when using a perspective projection matrix
//1.0 ->farplane 0.0 ->2.0 * near_plane /(1.0 + near_plane/far_plane)
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC 
    return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));	
}

void main()
{
	float depth = texture(shadowMap,TexCoords).r;
	//depth = LinearizeDepth(depth)/far_plane;
	FragColor = vec4(depth);
	FragColor = texture(colorTex,TexCoords);
}