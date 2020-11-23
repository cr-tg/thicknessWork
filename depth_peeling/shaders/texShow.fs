#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D uTexture;

uniform float near_plane;
uniform float far_plane;

// required when using a perspective projection matrix
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC 
    return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));	
}




void main()
{
	FragColor = texture(uTexture,TexCoords);
	float depth = texture(uTexture,TexCoords).r;//深度图
	float linearDepth = LinearizeDepth(depth);
	//FragColor = vec4(vec3(depth),1.0f);//深度图
	//FragColor = vec4(vec3(linearDepth),1.0f);//深度图
}
