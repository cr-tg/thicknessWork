#version 330 core

layout(location = 0) out vec4 outColor;//��Ⱦ��֡�����е�����0

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} fs_in;

uniform sampler2DArrayShadow uShadowMapLinear;
uniform sampler2D uWoodTexture;
uniform sampler2DArray testMap;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec4 uLightProjColor;
uniform vec4 uObjColor;

uniform float uNearPlane;
uniform float uFarPlane;
uniform bool uIsLight;
uniform bool uIsLightProj;
uniform bool uIsPlan;
uniform int uPcfCount;
uniform bool usePcf;

uniform vec2 offsetVec[13];

#define PI 3.1415926
#define USEPCF false

vec3 shadowCal(vec3 normal,vec3 lightDir);
vec2 rotate(vec2 v, float a) { return cos(a)*v + sin(a)*vec2(v.y, -v.x); }


void main()
{
	if(uIsLight)
	{
		outColor = vec4(0.0f,1.0f,1.0f,1.0f);
		return;
	}
	if(uIsLightProj)
	{
		outColor = uLightProjColor;
		return;
	}
	vec3 lightDir = normalize(lightPos - fs_in.FragPos);
	vec3 normal = normalize(fs_in.Normal);
	vec3 reflectDir = normalize(reflect(-lightDir,normal));
	vec3 viewDir = normalize(viewPos - fs_in.FragPos);

	vec4 diffuseTex;
	if(uIsPlan)
		diffuseTex = vec4(1.0);
		//diffuseTex = texture(uWoodTexture,fs_in.TexCoords);
	else
		diffuseTex = uObjColor;
	
	//ambient
	vec3 ambient = 0.03* lightColor * diffuseTex.rgb;
	//diffuse
	vec3 diffuse = diffuseTex.rgb * max(dot(lightDir,normal),0.0) * lightColor;
	//spcular
	vec3 specular = diffuseTex.rgb * pow(max(dot(reflectDir,normal),0.0),32.0) * lightColor;
	//blinn-phong
	vec3 halfDir = normalize(viewDir + lightDir);
	vec3 blinnSpecular = diffuseTex.rgb * pow(max(dot(halfDir,normal),0.0),32.0) * lightColor;
	vec3 visibility = shadowCal(normal,lightDir);
	diffuse *= visibility;
	blinnSpecular *= visibility;
	outColor = vec4(ambient+diffuse+blinnSpecular,diffuseTex.a);
	outColor.rgb = pow(outColor.rgb,vec3(1.0/2.2));
	//outColor.rgb = pow(visibility,vec3(1.0/2.2));
}

vec3 shadowCal(vec3 normal,vec3 lightDir)
{
	vec3 texcoords = fs_in.FragPosLightSpace.xyz / fs_in.FragPosLightSpace.w;
	texcoords = texcoords *0.5+0.5;
	if(texcoords.z > 1.0)
        return vec3(0.0);//��Դ��ľ�����Ӱ֮��
	//��Ӱ������ȱ��ڵ���������Ӱ֮�У����Ϊ���õذ岻����Ӱ֮�У���С���ǵ����
	//�����������ƽ�е�ʱ�򣬴�ʱ��biasӦ�����
	float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
	vec3 shadowResult =  vec3(0.0f);
	vec3 texelSize = 1.0 / textureSize(uShadowMapLinear, 0);
	for(int i = 0;i<3;++i)
	{
		//float visibility = texture(uShadowMapLinear,vec4(texcoords.xy,i,texcoords.z - bias));
		float visibility = 0.0;
		float angle = noise1(gl_FragCoord.xy) * 2.0 * PI;
		if(usePcf)
		{
			int pcfCount = 0;
			//int pcfCount = uPcfCount;
			for(int x = -pcfCount; x <= pcfCount; ++x)
			{
				for(int y = -pcfCount; y <= pcfCount; ++y)
				{
					vec2 offset = rotate(vec2(x, y), angle);
					visibility += texture(uShadowMapLinear, vec4(texcoords.xy+ offset * texelSize.xy,i,texcoords.z - bias)); 
				}    
			}
			visibility /= ((1.0+2.0*pcfCount)*(1.0+2.0*pcfCount));
		}
		else
		{
			float totalWeight = 0.0;
			for(int j = 0;j<1;++j)//��׼����13��
			{
				vec2 offset = offsetVec[j];
				vec2 jitter = (mod(offsetVec[j],vec2(2,2)) - vec2(1,1))/(6*texcoords.xy);
				//֮ǰ�ѹ�ʽ�����ˣ������˺�ǳ����Ӱ����Ϊ��������ȫ�ܵ���������
				//vec2 jitter = (mod(offsetVec[j],vec2(2,2)) - vec2(1,1))/(6*offset.xy);
				offset += jitter;
				float weight = max(1.0 - length(offset)/8.0,0.1);//��Զ��offsetΪ7,0������С��8.0
				weight = 1.0;//ʹ��Ȩ�ض��ڽ��Ӱ�첻��
				totalWeight += weight;
				visibility += texture(uShadowMapLinear, vec4(texcoords.xy+ offset * texelSize.xy,i,texcoords.z - bias)) * weight; 
			}
			visibility /= totalWeight;
		}
	
		shadowResult[i] = visibility;
	}
	return shadowResult;
}