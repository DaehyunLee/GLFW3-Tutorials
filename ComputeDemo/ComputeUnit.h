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

struct BoundBufferInfo
{
	unsigned int sizeInBytes;

	GLuint Id;
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

	std::vector<UniformInfo> myUniformInfo;
	std::vector<BufferInfo> myBufferInfo;
	std::vector<BoundBufferInfo> myBoundBufferInfo;

	//execution params.
	GLuint dispatchDimensionX;
	GLuint dispatchDimensionY;
	GLuint dispatchDimensionZ;

	//initialization parameters.
	std::string sourceName;

	//initialized values
	GLuint programId;

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

	void Dump(int index, std::vector<float>& buffer);
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

	bool CalculateBufferSize(const std::vector<BufferInfo>& bufInfo, unsigned int numElementes, std::vector<unsigned int>& requestSize);
	bool GenerateBuffers(const std::vector<BufferInfo>& bufInfo, const std::vector<unsigned int>& requestSize, std::vector<GLuint>& idList);
	void UploadDataToBuffer(unsigned int bufferId, const std::vector<float>& data);

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
	std::vector<unsigned int> bufferSizeInBytes;
	ComputeHelper::CalculateBufferSize(myBufferInfo, 1000, bufferSizeInBytes);

	std::vector<GLuint> generatedIds;
	ComputeHelper::GenerateBuffers(myBufferInfo, bufferSizeInBytes, generatedIds);
	assert(generatedIds.size() == myBufferInfo.size());
	myBoundBufferInfo.resize(generatedIds.size());
	for (int i = 0; i < generatedIds.size(); i++) {
		myBoundBufferInfo[i].Id = generatedIds[i];
		myBoundBufferInfo[i].sizeInBytes = bufferSizeInBytes[i];
	}

#else
#endif //TRYING_TO_REMOVE_TEMPLATE
	return ret;
}

template <int numInput, int numOutputBuffer>
void CS_Class<numInput, numOutputBuffer>::Dispatch(GLuint dimX, GLuint dimY, GLuint dimZ)
{
	//todo.. neeed to merge all params in one array.
	//although would be easy to force orders of types of param in thee shader. hmmm. .. :S

#if TRYING_TO_REMOVE_TEMPLATE
	for (int i = 0; i < myBufferInfo.size(); i++)
	{
		switch (myBufferInfo[i].type)
		{
		case GL_FLOAT_VEC4:
		case GL_INT_VEC4:
		case GL_FLOAT: //ComputeInit::TYPE_FLOAT
		{
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i /*might need to use values from the program info*/, myBoundBufferInfo[i].Id);
		}break;
		case GL_ATOMIC_COUNTER_BUFFER:
		{
			glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, i, myBoundBufferInfo[i].Id);
			GLuint zero = 0;
			glClearBufferData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, GL_RED, GL_UNSIGNED_INT, &zero);
		} break;
		default:
			assert(0);
			break;
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
#else
#endif
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
void CS_Class<numInput, numOutputBuffer>::Dump(int index, std::vector<float>& buffer)
{
	assert(index < myBoundBufferInfo.size());

	glBindBuffer(GL_ARRAY_BUFFER, myBoundBufferInfo[index].Id);
	buffer.resize(myBoundBufferInfo[index].sizeInBytes/sizeof(float));
	void* pStuff = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
	memcpy(buffer.data(), pStuff, myBoundBufferInfo[index].sizeInBytes);

	glUnmapBuffer(GL_ARRAY_BUFFER);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
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


