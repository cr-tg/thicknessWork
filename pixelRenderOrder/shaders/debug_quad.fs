#version 460 core
out vec4 FragColor;

layout(binding = 0) uniform sampler2D colorTex;
layout(binding = 1) uniform sampler2D shadowMap;
float near_plane = 0.1f, far_plane = 10.0f;//far_planer̫��ᵼ�¾��Ƚ���

in vec2 TexCoords;

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC 
	z = (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));
    return 	(z-near_plane)/(far_plane - near_plane);
}

vec4 outputDepth();
vec4 outputColor();

void main()
{
	FragColor = outputColor();
	//FragColor = vec4(0.0f);
}

vec4 outputDepth()
{
	float depth = texelFetch(shadowMap,ivec2(gl_FragCoord.xy),0).r;
	depth = LinearizeDepth(depth);
	return vec4(depth);
}

vec4 outputColor()
{
	//ʹ��texcoords�Ļ����Ϳ��Խ������С��ͼƬ��ȫ��ʾ������ʹ��texelFetch�Ļ���ֻ�ܽ�����Ļ��С�ֱ���һ�µ�ͼƬ��ȫ��ʾ����
	vec4 color = vec4(0.0f);
//	color = texelFetch(colorTex,ivec2(gl_FragCoord.xy),0);
	color = vec4(texture(colorTex,vec2(TexCoords))); 
	return color;
}

//color = texelFetch(colorTex,ivec2(gl_FragCoord.xy),0);
//texelFetch�Ǵ����½ǿ�ʼ���в�����
