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



template <class bufferType>
bool CS_Class::SetUniformData4x3(const char* name, std::vector<bufferType>& buffer)
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

template <class bufferType>
bool CS_Class::SetUniformData3x3(const char* name, std::vector<bufferType>& buffer)
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