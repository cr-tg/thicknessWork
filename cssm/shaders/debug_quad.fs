#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2DArray testMap;
uniform sampler2D colorTex;

uniform float near_plane;
uniform float far_plane;//far_planer太大会导致精度降低

// required when using a perspective projection matrix
//1.0 ->farplane 0.0 ->near_plane
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC 
    return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));	
}

void main()
{  
	float depth0 = texture(testMap,vec3(TexCoords,0)).r;
	float depth1 = texture(testMap,vec3(TexCoords,1)).r;
	float depth2 = texture(testMap,vec3(TexCoords,2)).r;
	vec4 color = texture(colorTex,TexCoords);
	//float linearDepth = LinearizeDepth(depth);
	//FragColor = vec4(vec3(depth0,depth1,depth2),1.0);
	FragColor = color;
}