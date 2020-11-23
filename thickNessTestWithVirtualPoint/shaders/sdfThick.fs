#version 460 core
layout(binding = 0) uniform sampler2D gPosition;//��ɫ�����������
layout(binding = 1) uniform sampler2D shadowMapRealLight;//��ʵ��Դ�µ�shadow map
layout(binding = 4,r32ui) uniform restrict writeonly uimage2D uSdfThickImage;

in vec2 texcoords;


uniform vec3 uLightPos;
uniform vec2 uScreenSize;
uniform mat4 realLightProjection;
uniform mat4 realLightView;
uniform float real_near_plane;
uniform float real_far_plane;

const int MAX_MARCHING_STEPS = 255*12;
const float EPSILON = 1e-2;//1e-5
const float BIAS = 1e-3;
const int MAX_DEPTH = 30;

struct ListNodeLight
{
	uint next;
	uint depth;
};

layout(std430,binding = 1) coherent restrict buffer ssboLight
{
	ListNodeLight nodeLightSet[];
};

out vec4 testColor;

float returnZe(float depth, float near, float far);
float linearizeDepth(float depth, float near, float far);
//����Ŀ���ķ�������ȣ�ͶӰ�����Զ��ƽ�棬�ڲü�����ϵ�µ�UV�����pv���������������������
vec4 getWorldPosFromDepth(float depth,float nearPlane,float farPlane,vec2 clipPosUV,mat4 pvMat4);


//�����е����Բ�ķֱ�Ϊvec3(0.0f, 0.2f, 4.0f),vec3(0.0f, 0.2f, 3.2f),vec3(0.0f, 0.2f, 2.4f),radius����0.2f
float sphereSDF(vec3 p,vec3 circleCenter,float radius);
//�����е�Բ��������Ϊvec3(0.0f, 0.2f, 4.0f)������뾶�ֱ�Ϊ1.0��0.1f�����ߺ��Ϊ1.1-0.9=0.2f
float tourSDF(vec3 pos, vec3 tourCenter,float radius,float cs_radius);
float cubeSDF(vec3 p,vec3 circleCenter,float cubeLen);
float intersectSDF(float distA, float distB);
float unionSDF(float distA, float distB);
float differenceSDF(float distA, float distB);
float sceneSDF(vec3 samplePoint);
vec3 rayDirection(float fieldOfView, vec2 size, vec2 fragCoord);
float shortestDistanceToSurface(vec3 ro,vec3 rd,float start,float end);
 

float calThick(vec3 ro,vec3 rd,vec3 rend)
{
    float hitDepths[MAX_DEPTH];
    float totalDist = 0.0f;
    int depthIdx = 0;
	int marchingStep = 0;
    while(depthIdx < MAX_DEPTH && marchingStep < MAX_MARCHING_STEPS)
    {
        vec3  samplePoint = ro + rd * totalDist;
		float t = dot(rend - samplePoint, rd);
        if(t<0.0) break;
        float dist = abs(sceneSDF(samplePoint));
        totalDist += dist;
		marchingStep++;
        if(dist < EPSILON)
        {
            hitDepths[depthIdx] = totalDist;
            totalDist += 2.0 * EPSILON;
          //  totalDist += 100.0 * EPSILON;//��һ�󲽣�����һ�㱻��¼Ϊ����,100��Ч��Ҫ��һ��
            depthIdx++;
        }
    }
    float thickness = 0.0;
    for(int i = 0;i<depthIdx -1;i+=2)
    { 
       thickness += hitDepths[i+1] - hitDepths[i];
    }

//	testColor = vec4(0.0,1.0,0.0,1.0);
	if(depthIdx == 0)
    {
		testColor = vec4(0.0,0.0,0.0,1.0);
		// testColor = vec4(marchingStep/255.0f,0.0,totalDist/255.0f*100.0f,1.0);
    }
	else if(depthIdx == 1)
    {
		 testColor = vec4(marchingStep/255.0f,0.2,totalDist/255.0f*100.0f,1.0);
		//testColor = vec4(0.0,1.0,0.0,1.0);
    }
    else if(depthIdx == 2)
    {
		 testColor = vec4(marchingStep/255.0f,thickness,totalDist/255.0f*100.0f,1.0);
		//testColor = vec4(0.0,1.0,0.0,1.0);
    }
	else
		testColor = vec4(vec2(float(depthIdx)/255.0f*50.0f),0.5f,1.0f);
    testColor = vec4(thickness/255.0f*100.0f);
	return thickness;
}


vec3 drawTours(vec3 samplePoint)
{
    vec3 color = vec3(0.0,0.0,1.0f);
    vec3 tourCenter1 = vec3(0.0,0.2,4.0);
	float tourDist = tourSDF(samplePoint,tourCenter1,1.0,0.1);
    float blur = 0.01;
    float colorFactor = step(blur,tourDist);//tourDist>blur,colorFactor=1.0
    //float colorFactor = smoothstep(0.0f,blur,tourDist);//<0.0,colorFactor = 0.0;>blur,colorFactor=1.0
    return color * (1.0 - colorFactor);
}

void main()
{
    if(!true)
    {
       // vec2 texcoords = vec2(0.4039,0.5569);
    	float depth = texture(shadowMapRealLight,vec2(texcoords)).r;
		//float depth = texelFetch(shadowMapRealLight,ivec2(630,410),0).r;
		depth = linearizeDepth(depth,real_near_plane,real_far_plane);
		testColor = vec4(depth);
        return;
    }
    //����ط���gbuffer�л�ȡʵ�ʻ��Ƶ�������ΪĿ����������겻�У������൱����ʹ�û��Ƴ�����������Ϊ����
	vec4 worldSpaceDst = texture(gPosition,texcoords);
    if(!true)
    {
        testColor = vec4(drawTours(worldSpaceDst.xyz),1.0f);
        return;
    }
    vec3 worldDirTest = normalize(worldSpaceDst.xyz - uLightPos);
    mat4 realLightPV = realLightProjection * realLightView;
	vec4 lightClipSpaceDst = realLightPV * worldSpaceDst;//�ü����꣬-w,w�ķ�Χ������w�Ժ󣬱任��-1,1�ķ�Χ
	vec2 sampleCoord = lightClipSpaceDst.xy / lightClipSpaceDst.w;// �������꣬��shadowmap�н��в������õ�DST��ORI��zֵ��
	sampleCoord = sampleCoord * 0.5 +0.5;//xyҲ����[-1��1]
//	float oriDepth = lightClipSpaceDst.z/lightClipSpaceDst.w *0.5 +0.5;
	float oriDepth = texture(shadowMapRealLight,sampleCoord).r;
	float oriLinearDepth = linearizeDepth(oriDepth,real_near_plane,real_far_plane);//0-1
//    if(oriLinearDepth > 1.0 - BIAS)
//    {
//        testColor = vec4(sampleCoord,0.5f,0.0f);
//        return;
//    }
//    if(oriLinearDepth > 200.0/255.0f)
//    {
//        testColor = vec4(sampleCoord * uScreenSize/255.0f/5.0f,oriLinearDepth,0.0f);
//        return;
//    }
	vec2 clipSpaceUV = lightClipSpaceDst.xy/lightClipSpaceDst.w;
	vec4 worldSpaceOri = getWorldPosFromDepth(oriDepth,real_near_plane,real_far_plane,clipSpaceUV,realLightPV);
    float dis2Ori = length(worldSpaceDst.xyz - worldSpaceOri.xyz);
    //float maxDist = dis2Ori + 0.1f;
//    testColor = vec4(oriLinearDepth);
//    if(oriLinearDepth < BIAS)
//        testColor = vec4(1.0f);
  //  testColor = vec4(worldSpaceDst);

    float maxDist = 100.0f;
    float minDist = 0.0f;
    vec3 rd = worldDirTest;
    vec3 ro = uLightPos;
    float outerDist = 0.01f;
    vec3 rend = worldSpaceDst.xyz + rd*outerDist;
	float allthick = calThick(ro,rd,rend);
 //   testColor = vec4(dis2Ori/255.0f*100.0f);
	imageStore(uSdfThickImage,ivec2(gl_FragCoord.xy),ivec4(floatBitsToUint(allthick)));
}

float tourSDF(vec3 pos, vec3 tourCenter,float radius,float cs_radius)
{
	vec3 p = pos - tourCenter;
	vec2 q = vec2(length(p.xz)-radius,p.y);
	return length(q)-cs_radius;
}

//�����е����Բ�ķֱ�Ϊvec3(0.0f, 0.2f, 4.0f),vec3(0.0f, 0.2f, 3.2f),vec3(0.0f, 0.2f, 2.4f),radius����0.2f
float sphereSDF(vec3 p,vec3 circleCenter,float radius)
{
	return length(p - circleCenter) - radius;//���ص��Ǹ�������������ڲ������ص���������������ⲿ
}

float cubeSDF(vec3 p,vec3 circleCenter,float cubeLen) {
    // If d.x < 0, then -1 < p.x < 1, and same logic applies to p.y, p.z
    // So if all components of d are negative, then p is inside the unit cube
    vec3 d = abs(p - circleCenter) - vec3(0.5 * cubeLen);
    
    // Assuming p is inside the cube, how far is it from the surface?
    // Result will be negative or zero.
    float insideDistance = min(max(d.x, max(d.y, d.z)), 0.0);
    
    // Assuming p is outside the cube, how far is it from the surface?
    // Result will be positive or zero.
    float outsideDistance = length(max(d, 0.0));
    //��������ڲ�����ôoutsideDis����0��insideDistance������������һ��ά�ȵľ���ĸ���
	//��������ⲿ����ôinsideDis����0��outsideDistance���ǵ�cube�ϵ����е�������ľ���
    return insideDistance + outsideDistance;
}


//��ΪΪ0�ĵ�������ͼ�εı߽�㣬���ڲ��ĵ���С��0�ĵ㣬��ֵԽ�󣬾�Խ��������ͼ�ε��ڲ������������max����ȡ����ͼ�εĽ���
//����һ������A��һ������B������֮�䲻�ཻ�Ļ���intersect�Ľ������Զ������0����û��ͼ��
float intersectSDF(float distA, float distB) {
    return max(distA, distB);
}

//��֮��ȡ��Сֵ�Ļ�����˵ֻҪ��������һ��ͼ�ε��ڲ�����ô���ڽ�����ڲ��У������󲢼�
float unionSDF(float distA, float distB) {
    return min(distA, distB);
}

//���Ҳ���󽻼���ֻ��������ͼ��B�����ڲ�Ϊ�����ⲿΪ������˼������ͼ��B�ⲿ��ͼ��A�Ľ�����
//����������A�м�����һ��B
float differenceSDF(float distA, float distB) {
    return max(distA, -distB);
}

float sceneSDF(vec3 samplePoint) {
	vec3 circleCenter1 = vec3(0.0,0.2,4.0);
	vec3 circleCenter2 = vec3(0.0,0.2,3.2);
	vec3 circleCenter3 = vec3(0.0,0.2,2.4);
	float radius = 0.2f;
    float sphereDist1 = sphereSDF(samplePoint,circleCenter1,radius);
    float sphereDist2 = sphereSDF(samplePoint,circleCenter2,radius);
    float sphereDist3 = sphereSDF(samplePoint,circleCenter3,radius);
	vec3 tourCenter1 = vec3(0.0,0.2,4.0);
	float tourDist = tourSDF(samplePoint,tourCenter1,1.0,0.1);
	return tourDist;
	//return unionSDF(sphereDist1,unionSDF(sphereDist2,sphereDist3));
}

vec3 rayDirection(float fieldOfView, vec2 size, vec2 fragCoord) {
    vec2 xy = fragCoord - size / 2.0;
    float z = size.y / tan(radians(fieldOfView) / 2.0);
    return normalize(vec3(xy, -z));
}


float shortestDistanceToSurface(vec3 ro,vec3 rd,float start,float end)
{
    float shortestDis = start;
    for(int i = 0 ;i <MAX_MARCHING_STEPS;++i)
    {
        float dist = sceneSDF(ro + rd*shortestDis);
        if(dist < EPSILON)
        {
            return shortestDis;
        }
       shortestDis += dist;
        if(shortestDis >= end)
        {
            return end;
        }
    }
    return end;
}

float returnZe(float depth, float near, float far)
{
	float z = depth * 2.0 - 1.0; // back to NDC 
	//ndcz = -1��ze = -near
	z = (2.0 * near * far) / (z * (far - near) - (far + near)); // range: -near...-far,
	return z;
}

float linearizeDepth(float depth, float near, float far)
{
	float z = -returnZe(depth, near, far);
	z = (z - near) / (far - near); // range: 0...1
	return z;
}

vec4 getWorldPosFromDepth(float depth,float nearPlane,float farPlane,vec2 clipPosUV,mat4 pvMat4)
{
	vec4 worldPos = vec4(0.0f);
	float oriLinearDepth = linearizeDepth(depth,nearPlane,farPlane);//0-1
	float bias = 1e-5;
	if(oriLinearDepth >= 1.0 -bias)//���������ͼ����Ĳ�����g3d���ƺ���gbuffer����Ƭ�ε�ʱ����Ѿ��Զ�������һ��
		return vec4(2.0 * farPlane);
	float zc = -nearPlane+oriLinearDepth*(nearPlane+farPlane);
	float ze = returnZe(depth,nearPlane,farPlane);
	float wc = -ze;
	float xc = clipPosUV.x * wc;
	float yc = clipPosUV.y * wc;
	vec4 clipPos = vec4(xc,yc,zc,wc);
	worldPos = inverse(pvMat4) * clipPos;
	return worldPos;
}