#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2DArray colorTexSet;
uniform int uLayerIndex;
uniform int uAllDepthLayer;

void main()
{  
	vec4 currentColor = texture(colorTexSet,vec3(TexCoords,uLayerIndex));
	if(uLayerIndex == (uAllDepthLayer-1))//Ҫ�ж�ǰһ���Ƿ�Ϊ���һ�㣬�ǵĻ��Ͳ����л��
		FragColor = currentColor;
	else
	{
		vec4 preColor = texture(colorTexSet,vec3(TexCoords,uLayerIndex+1));
		//��ϣ�Դ*Դ��alpha+Ŀ��*(1-Դ��alpha)�����Դ��alphaΪ1����û��Ŀ�����ɫ��Դ����current color
		if(preColor.a != 0.0)//����ȡ��һ���ɣ����ǽ�������alphaֵ����Ϊ0.0�������Ļ��Ϳ��Բ��뱳�����л����
		{
			FragColor = vec4(currentColor.rgb*currentColor.a+preColor.rgb*(1-currentColor.a),currentColor.a);
		}
		else
			FragColor = currentColor;
	}

}