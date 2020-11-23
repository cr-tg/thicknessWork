#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

layout(binding = 0)uniform sampler2D listColor;
layout(binding = 1)uniform sampler2D realLightDepthMap;
layout(binding = 3,r32ui) uniform restrict uimage2D uThickImage;
layout(binding = 4,r32ui) uniform restrict uimage2D uSdfThickImage;
layout(binding = 5,r32f) uniform restrict image2D ErrorImage;

#define renderingShowSize 0.0f


uniform float far_plane;
uniform float near_plane;
uniform vec2 uScreenSize;

float LinearizeDepth(float depth);
vec4 getError();
vec4 getThick();

void main()
{  
	if(!true)
	{
		float depth = texture(realLightDepthMap,vec2(TexCoords)).r;
		//float depth = texelFetch(realLightDepthMap,ivec2(gl_FragCoord.xy),0).r;
		depth = LinearizeDepth(depth);
		FragColor = vec4(depth);
		return;
	}
	if(gl_FragCoord.x > uScreenSize.x * renderingShowSize && gl_FragCoord.y > uScreenSize.y * renderingShowSize)
	{
		FragColor = getError();
		//FragColor = getThick();
		return;
	}
	FragColor = vec4(texture(listColor,vec2(TexCoords)));
}

vec4 getError()
{
	float error = imageLoad(ErrorImage,ivec2((gl_FragCoord.xy-renderingShowSize*uScreenSize) * (1.0/(1.0-renderingShowSize)))).r;
	vec4 showError = vec4(error * 100.0f/255.0f);
	return showError;
}

vec4 getThick()
{
//	float thick = uintBitsToFloat(imageLoad(uSdfThickImage,ivec2((gl_FragCoord.xy-renderingShowSize*uScreenSize) * (1.0/(1.0-renderingShowSize)))).r);
	float thick = uintBitsToFloat(imageLoad(uThickImage,ivec2((gl_FragCoord.xy-renderingShowSize*uScreenSize) * (1.0/(1.0-renderingShowSize)))).r);
	vec4 showThick = vec4(thick/255.0f*100.0f);
	return showThick;
}

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // Back to NDC 
	z = (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));
	z = (z-near_plane)/(far_plane - near_plane);
    return z;
}