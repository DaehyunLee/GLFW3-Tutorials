#include "stdafx.h"
#include "ComputeShaders.h"

#include "FileUtils.h"

/*
https://software.intel.com/en-us/articles/opengl-performance-tips-atomic-counter-buffers-versus-shader-storage-buffer-objects
http://www.geeks3d.com/20120309/opengl-4-2-atomic-counter-demo-rendering-order-of-fragments/
*/

const static int tex_w = 512, tex_h = 512;
GLuint CreateTexforCompute()
{
	// dimensions of the image
	GLuint tex_output;
	glGenTextures(1, &tex_output);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_output);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, tex_w, tex_h);/*/
															   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tex_w, tex_h, 0, GL_RGBA, GL_FLOAT, NULL);/*/
	glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	return tex_output;
}

ComputeShaders::ComputeShaders()
{
}


ComputeShaders::~ComputeShaders()
{
	DestroyComputeShader();
}

ComputeShaders* ComputeShaders::Create()
{
	ComputeShaders* ret = new ComputeShaders();
	if (ret->InitComputeShader())
	{
		return ret;
	}
	else
	{
		std::exception("failed to init compute shader...");
	}
	return nullptr;
}

/*intensionally not using typedefed type to pick up changes*/
//might be able to improve this by iterating through the compile result of glsl
void InitExtractDepthParams(CS_Class<1, 1>& shader) {
	//lets sit on these ducttape codes a bit..
	//before we generalize anything
	const int IMAGE_WIDTH = 1000;
	const int IMAGE_HEIGHT = 1000;
	shader.inInitParams = { {
		{ ComputeInit::TYPE_FLOAT, IMAGE_WIDTH, IMAGE_HEIGHT, true },
		} };

	shader.outInitParams = { {
		{ ComputeInit::TYPE_VEC4, IMAGE_WIDTH, IMAGE_HEIGHT, true },
		} };

	shader.dispatchDimensionX = IMAGE_WIDTH;
	shader.dispatchDimensionY = IMAGE_HEIGHT;
	shader.dispatchDimensionZ = 1;

	shader.sourceName = "Dummy.glsl";
}


bool ComputeShaders::InitComputeShader()
{
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);

	printf("\tmax global (total) work group size x:%i y:%i z:%i\n", work_grp_cnt[0], work_grp_cnt[1], work_grp_cnt[2]);

	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);

	printf("\tmax local (in one shader) work group sizes x:%i y:%i z:%i\n", work_grp_size[0], work_grp_size[1], work_grp_size[2]);

	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
	printf("\tmax local work group invocations %i\n", work_grp_inv);

	GLint max_counters;
	glGetIntegerv(GL_MAX_ATOMIC_COUNTER_BUFFER_SIZE, &max_counters);
	printf("\tmax atomic counter %i\n", max_counters);

	///////////////////
	GLuint tex_output;
	glGenTextures(1, &tex_output);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex_output);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, tex_w, tex_h);/*/
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tex_w, tex_h, 0, GL_RGBA, GL_FLOAT, NULL);/*/
	glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8UI);
	/////////////////////////////

	InitExtractDepthParams(compute_extract);
	compute_extract.Init();
	
	return true;
}

void ComputeShaders::DestroyComputeShader()
{
	glDeleteProgram(compute_extract.programId);// maybe move this to the destructor?
}
