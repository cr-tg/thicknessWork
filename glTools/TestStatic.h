#pragma once
#include "setting.h"

class CTestStatic
{
public:
	static GLTOOLSDLL_API CTestStatic* getOrCreateWindowInstance()
	{
		if (m_pTestStatic == nullptr)
		{
			m_pTestStatic = new CTestStatic();
		}
		return m_pTestStatic;
	}

	GLTOOLSDLL_API int getNum() const
	{
		return num;
	}

private:
	CTestStatic();

	static CTestStatic* m_pTestStatic;

	int num;
};

