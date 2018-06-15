#pragma once
#include "ComputeUnit.h"


typedef CS_Class<1, 1> ComputeExtractFeature;

class ComputeShaders
{
private:
	ComputeShaders();

public:
	//------------ single compute shader//
	ComputeExtractFeature compute_extract;

	//------------ end
	virtual ~ComputeShaders();

	static ComputeShaders* Create();
private:
	int work_grp_cnt[3];
	int work_grp_size[3];
	int work_grp_inv;

	bool InitComputeShader();
	void DestroyComputeShader();
};

