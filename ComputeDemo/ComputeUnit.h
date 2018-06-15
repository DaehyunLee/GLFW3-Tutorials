#pragma once

struct UniformInfo {
	std::array<char, 512> name;
	int size;
	GLenum type;
};

struct BufferInfo {
	std::string name;
	GLenum type;
	int stride;
};

struct InputInfo {
	GLenum type;
	int stride;
};

class ComputeInit
{
public:
	enum varigableType
	{
		TYPE_TEXTURE,
		TYPE_FLOAT,
		TYPE_VEC4,
		TYPE_ATOMICCOUNTER,

		NUM_TYPES
	};

	varigableType type;
	
	int width;
	int height;

	bool createBuffer;
};

template <int numInput, int numOutputBuffer>
class CS_Class
{
public:
	CS_Class()
	{};

	//init params
	typedef std::array<ComputeInit, numInput> InitParamList_In;
	typedef std::array<ComputeInit, numOutputBuffer> InitParamList_Out;
	//initialized values
	typedef std::array<GLuint, numInput> InputIDList;
	typedef std::array<GLuint, numOutputBuffer> BufferIDList;

	std::vector<UniformInfo> myUniformInfo;
	std::vector<BufferInfo> myBufferInfo;

	typedef std::array<std::vector<float>, numOutputBuffer> debugBuffer;

	//execution params.
	GLuint dispatchDimensionX;
	GLuint dispatchDimensionY;
	GLuint dispatchDimensionZ;

	//initialization parameters.
	std::string sourceName;
	InitParamList_In inInitParams;
	InitParamList_Out outInitParams;

	//initialized values
	GLuint programId;
	InputIDList inputs;
	BufferIDList ids;

	bool Init();
	void Dispatch()
	{
		Dispatch(dispatchDimensionX, dispatchDimensionY, dispatchDimensionZ);
	}
	void Dispatch(GLuint dimX, GLuint dimY, GLuint dimZ);

	template <class bufferType>
	bool SetUniformData4x3(const char* name, std::vector<bufferType>& buffer);

	template <class bufferType>
	bool SetUniformData3x3(const char* name, std::vector<bufferType>& buffer);

	//we should separate the buffer and the CS shader. 
	void UploadData(int index, std::vector<float>& data);

	void Dump(debugBuffer& buffer);
	void Dump(std::vector<float>& buffer, int index);
	void Dump(int& atomicCount);
};



///-----------------------------------------------------------------------------------------------
// Implemenation.
// consider moving to an hpp when it gets too big.
///-----------------------------------------------------------------------------------------------
namespace ComputeHelper
{
	bool CompileShader(const char* filename, GLuint& progId);
	void PrintProgramInfo(std::ostream& out, GLuint progId);
	void PrintShaderInfo(std::ostream& out, GLuint shaderId);

	bool GenerateBuffers(std::vector<BufferInfo>& bufInfo, std::vector<GLuint>& idList);

	void LoadUniformInfo(std::vector<UniformInfo>& data, GLuint progId);
	void LoadBufferInfo(std::vector<BufferInfo>& data, GLuint progId);
	void LoadInputInfo(std::vector<InputInfo>& data, GLuint progId);
}

template <int numInput, int numOutputBuffer>
bool CS_Class<numInput, numOutputBuffer>::Init()
{
	bool ret = ComputeHelper::CompileShader(sourceName.c_str(), programId);

	if (ret)
	{
		ComputeHelper::LoadUniformInfo(myUniformInfo, programId);
		ComputeHelper::LoadBufferInfo(myBufferInfo, programId);
	}

#define TRYING_TO_REMOVE_TEMPLATE 1
#if TRYING_TO_REMOVE_TEMPLATE
	std::vector<GLuint> generatedIds;
	ComputeHelper::GenerateBuffers(myBufferInfo, generatedIds);
	assert(generatedIds.size() == inputs.size() + ids.size());
	inputs[0] = generatedIds[0];
	ids[0] = generatedIds[1];
	//copy.
#else
//this buffer generation should be removed aside from template
	//Generate inputs
	for (int i = 0; i < numInput; i++)
	{
		auto& param = inInitParams[i];

		if (param.createBuffer)
		{
			glGenBuffers(1, &inputs[i]);
			if (param.type == ComputeInit::TYPE_VEC4)
			{
				glBindBuffer(GL_ARRAY_BUFFER, inputs[i]);
				glBufferData(GL_ARRAY_BUFFER, param.width * param.height * 4 * sizeof(GLuint), 0, GL_DYNAMIC_DRAW);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}
			else if (param.type == ComputeInit::TYPE_FLOAT)
			{
				glBindBuffer(GL_ARRAY_BUFFER, inputs[i]);
				glBufferData(GL_ARRAY_BUFFER, param.width * param.height * sizeof(GLuint), 0, GL_DYNAMIC_DRAW);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}
		}
	}

	//Generate Outputs
	glGenBuffers(numOutputBuffer, ids.data());
	for (int i = 0; i < numOutputBuffer; i++)
	{
		auto& param = outInitParams[i];

		if (param.type == ComputeInit::TYPE_ATOMICCOUNTER)
		{
			glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, ids[i]);
			glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
		}
		else if (param.type == ComputeInit::TYPE_VEC4)
		{
			glBindBuffer(GL_ARRAY_BUFFER, ids[i]);
			glBufferData(GL_ARRAY_BUFFER, param.width * param.height * 4 * sizeof(GLuint), 0, GL_DYNAMIC_READ);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
	}
#endif //TRYING_TO_REMOVE_TEMPLATE
	return ret;
}

template <int numInput, int numOutputBuffer>
void CS_Class<numInput, numOutputBuffer>::Dispatch(GLuint dimX, GLuint dimY, GLuint dimZ)
{
	//todo.. neeed to merge all params in one array.
	//although would be easy to force orders of types of param in thee shader. hmmm. .. :S

	int position = 0;
	for (int i = 0; i < numInput; i++) {
		if (inInitParams[i].type == ComputeInit::TYPE_TEXTURE) {
			//this is a temporary implementation for feature extraction shader.
			//hope to remove texture connection for later
			assert(position == 0 && i==0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, inputs[0]);
			glBindImageTexture(0, inputs[0], 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8UI);
		}
		else if (inInitParams[i].type == ComputeInit::TYPE_VEC4) {
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, position+i, inputs[i]);
		}
		else if (inInitParams[i].type == ComputeInit::TYPE_FLOAT) {
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, position + i, inputs[i]);
		}
		else {
			assert(0 && "unsupported type");
		}
	}

	position += numInput;
	for (int i = 0; i < numOutputBuffer; i++)
	{
		int id = ids[i];
		auto& param = outInitParams[i];
		if (param.type == ComputeInit::TYPE_ATOMICCOUNTER)
		{
			glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, position+i, id);

			GLuint zero = 0;
			glClearBufferData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &zero);
		}
		else
		{
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, position + i, id);
		}
	}

	{ // launch compute shaders!
		glUseProgram(programId);
		glDispatchCompute(dimX, dimY, dimZ);
	}

	glUseProgram(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// make sure writing to image has finished before read
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

template <int numInput, int numOutputBuffer>
template <class bufferType>
bool CS_Class<numInput, numOutputBuffer>::SetUniformData4x3(const char* name, std::vector<bufferType>& buffer)
{
	//should remove bufferType template
	GLuint loc = glGetUniformLocation(programId, name);
	assert(loc != 0);
	assert(loc != -1);

	assert(buffer.size == 12);

	glUseProgram(programId);
	glUniformMatrix4x3dv(loc, 1, true, buffer.data());
	assert(glGetError() == 0);
	glUseProgram(0);
	return true;
}

template <int numInput, int numOutputBuffer>
template <class bufferType>
bool CS_Class<numInput, numOutputBuffer>::SetUniformData3x3(const char* name, std::vector<bufferType>& buffer)
{
	//should remove bufferType template
	GLuint loc = glGetUniformLocation(programId, name);
	assert(loc != -1);
	assert(buffer.size() == 9);

	glUseProgram(programId);
	glUniformMatrix3fv(loc, 1, true, buffer.data());
	assert(glGetError() == 0);
	glUseProgram(0);
	return true;
}

template <int numInput, int numOutputBuffer>
void CS_Class<numInput, numOutputBuffer>::UploadData(int index, std::vector<float>& data)
{
	assert(index < numOutputBuffer);
	assert(outInitParams[index].type == ComputeInit::TYPE_VEC4);

	glBindBuffer(GL_ARRAY_BUFFER, inputs[index]);

	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*data.size(), data.data());
	//void* pStuff = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	//memcpy(pStuff, data.data(), sizeof(float)*data.size());

	glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}


template <int numInput, int numOutputBuffer>
void CS_Class<numInput, numOutputBuffer>::Dump(debugBuffer& buffers)
{
	for (int i = 0; i < numOutputBuffer; i++)
	{
		if (outInitParams[i].type == ComputeInit::TYPE_VEC4)
		{
			glBindBuffer(GL_ARRAY_BUFFER, ids[i]);

			buffers[i].resize(outInitParams[i].width*outInitParams[i].height * 4);

			void* pStuff = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
			float* pFloat = (float*)pStuff;
			memcpy(buffers[i].data(), pStuff, sizeof(float)*buffers[i].size());

			glUnmapBuffer(GL_ARRAY_BUFFER);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
		else if (outInitParams[i].type == ComputeInit::TYPE_ATOMICCOUNTER)
		{
			glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, ids[i]);

			buffers[i].resize(1);
			void* pStuff = glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_ONLY);
			unsigned int data = *(unsigned int*)pStuff;

			buffers[i][0] = data;

			glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
			glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

		}
	}
}

template <int numInput, int numOutputBuffer>
void CS_Class<numInput, numOutputBuffer>::Dump(std::vector<float>& buffer, int index)
{

	ChkFence();
	assert(index < numOutputBuffer);
	ChkFence();
	assert(outInitParams[index].type == ComputeInit::TYPE_VEC4);
	ChkFence();

	glBindBuffer(GL_ARRAY_BUFFER, ids[index]);
	ChkFence();

	buffer.resize(outInitParams[index].width*outInitParams[index].height * 4);
	void* pStuff = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
	float* pFloat = (float*)pStuff;
	memcpy(buffer.data(), pStuff, sizeof(float)*buffer.size());
	ChkFence();

	glUnmapBuffer(GL_ARRAY_BUFFER);
	ChkFence();
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	ChkFence();
}


template <int numInput, int numOutputBuffer>
void CS_Class<numInput, numOutputBuffer>::Dump(int& atomicCount)
{
	//glMapBufferRange
	ChkFence();
	for (int i = 0; i < numOutputBuffer; i++)
	{
		ChkFence();
		if (outInitParams[i].type == ComputeInit::TYPE_ATOMICCOUNTER)
		{
			ChkFence();
			glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, ids[i]);

			ChkFence();
			void* pStuff = glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_ONLY);
			atomicCount = *(uint*)pStuff;
			ChkFence();

			glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
			ChkFence();
			glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
			ChkFence();
		}
	}
}


