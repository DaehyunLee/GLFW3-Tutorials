#include "stdafx.h"

#include "ComputeUnit.h"
#include "FileUtils.h"

GLint PrintProgInt(std::ostream& out, GLuint progId, GLuint param, const char* paramName) {
	GLint value = 0;
	glGetProgramiv(progId, param, &value);
	out << paramName <<" "<< value << std::endl;
	return value;
};
#define PrintProgINTparam(out, progId, param) PrintProgInt(out, progId, param, #param)

GLint PrintShaderInt(std::ostream& out, GLuint progId, GLuint param, const char* paramName) {
	GLint value = 0;
	glGetProgramiv(progId, param, &value);
	out << paramName << " " << value << std::endl;
	return value;
};
#define PrintShaderINTparam(out, progId, param) PrintShaderInt(out, progId, param, #param)

namespace ComputeHelper {

	void PrintShaderInfo(std::ostream& out, GLuint shaderId)
	{
		out << "print shader info " << shaderId << std::endl;


	}


	//need to come back here when we need more debugging.
	//https://www.khronos.org/opengl/wiki/Program_Introspection
	void PrintProgramInfo(std::ostream& out, GLuint progId)
	{
		out << "print prog info " << progId << std::endl;

		GLint maxLength = PrintProgINTparam(out, progId, GL_INFO_LOG_LENGTH);
		if (maxLength > 0) {
			std::vector<GLchar> infoLog(maxLength);
			glGetProgramInfoLog(progId, maxLength, &maxLength, &infoLog[0]);
			out << infoLog.data() << std::endl;
		}

		PrintProgINTparam(out, progId, GL_ACTIVE_ATOMIC_COUNTER_BUFFERS);
		PrintProgINTparam(out, progId, GL_ACTIVE_ATTRIBUTES);
		PrintProgINTparam(out, progId, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH);
		PrintProgINTparam(out, progId, GL_ACTIVE_UNIFORMS);
		PrintProgINTparam(out, progId, GL_ACTIVE_UNIFORM_MAX_LENGTH);



		int attachedShaders = PrintProgINTparam(out, progId, GL_ATTACHED_SHADERS);

		std::vector< GLuint > shaderIds(attachedShaders);
		GLsizei numRead = -1;
		glGetAttachedShaders(progId, shaderIds.size(), &numRead, shaderIds.data());

		for (auto& shaderId : shaderIds) {
			PrintShaderInfo(out, shaderId);
		}
	}

	bool CompileShader(const char* filename, GLuint& progId)
	{
		std::cout << std::endl << "compiling " << filename << std::endl;
		std::vector<char> glslBuffer;
		FileUtils::ReadFile(filename, glslBuffer);


		GLuint compute_shader = glCreateShader(GL_COMPUTE_SHADER);
		const char* shaderContent = glslBuffer.data();
		glShaderSource(compute_shader, 1, &shaderContent, NULL);
		glCompileShader(compute_shader);
		// check for compilation errors as per normal here

		GLuint compute_program = glCreateProgram();
		glAttachShader(compute_program, compute_shader);
		glLinkProgram(compute_program);
		// check for linking errors and validate program as per normal here

		GLint paramsShader;
		glGetShaderiv(compute_shader, GL_COMPILE_STATUS, &paramsShader);
		if (paramsShader != GL_TRUE)
		{
			std::cout << "compile failed!!\n";
			int len = 0;
			std::array<char, 1024> buffer;
			glGetShaderInfoLog(compute_shader, buffer.size(), &len, buffer.data());
			std::cout << buffer.data() << std::endl;
			return false;
		}

		GLint paramsProg;
		glGetProgramiv(compute_program, GL_LINK_STATUS, &paramsProg);
		if (paramsProg == GL_TRUE)
		{
			std::cout << "link success!!\n";
		}
		else
		{
			std::cout << "link failed!!\n";
			int len = 0;
			std::array<char, 1024> buffer;
			glGetProgramInfoLog(compute_program, buffer.size(), &len, buffer.data());
			std::cout << buffer.data() << std::endl;
			return false;
		}

		progId = compute_program;

		std::vector<InputInfo> dataInput;
		LoadInputInfo(dataInput, progId);

		{
			//just dumping stuff..
			std::vector<int> test = { 
				GL_UNIFORM, 
				GL_UNIFORM_BLOCK,
				GL_ATOMIC_COUNTER_BUFFER,
				GL_PROGRAM_INPUT,
				GL_PROGRAM_OUTPUT, 
				GL_TRANSFORM_FEEDBACK_VARYING,
				GL_BUFFER_VARIABLE,
				GL_SHADER_STORAGE_BLOCK,
				GL_TRANSFORM_FEEDBACK_BUFFER,
			};

			for (int i = 0; i < test.size(); i++) {
				int numSomething = 0;
				glGetProgramInterfaceiv(progId, test[i], GL_ACTIVE_RESOURCES, &numSomething);
				printf("[%d] => %d\n", i, numSomething);
			}
			printf("dump complete\n");
		}

		PrintProgramInfo(std::cout, progId);
		return true;
	}

	bool CalculateBufferSize(const std::vector<BufferInfo>& bufInfo, unsigned int numElementes, std::vector<unsigned int>& requestSize)
	{
		requestSize.resize(bufInfo.size());
		for (int i = 0; i < bufInfo.size(); i++)
		{
			switch (bufInfo[i].type)
			{
			case GL_FLOAT_VEC4:
			{
				requestSize[i] = sizeof(float) * numElementes * 4;
			} break;
			case GL_INT_VEC4:
			case GL_FLOAT: 
			{
				requestSize[i] = sizeof(float) * numElementes;
			} break;
			case GL_ATOMIC_COUNTER_BUFFER:
			{
				requestSize[i] = sizeof(GLuint);
			} break;
			default:
				assert(0);
				return false;
				break;
			}
		}

		return true;
	}

	bool GenerateBuffers(const std::vector<BufferInfo>& bufInfo, const std::vector<unsigned int>& requestSizeInBytes, std::vector<GLuint>& idList)
	{
		idList.resize(bufInfo.size());
		glGenBuffers(bufInfo.size(), idList.data());
		for (int i = 0; i < bufInfo.size(); i++)
		{
			switch (bufInfo[i].type)
			{
			case GL_FLOAT_VEC4:
			case GL_INT_VEC4:
			case GL_FLOAT:
			{
				glBindBuffer(GL_ARRAY_BUFFER, idList[i]);
				glBufferData(GL_ARRAY_BUFFER, requestSizeInBytes[i], 0, GL_DYNAMIC_DRAW);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
			}break;
			case GL_ATOMIC_COUNTER_BUFFER:
			{
				glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, idList[i]);
				glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
				glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
			} break;
			default:
				break;
			}
		}

		return true;
	}

	void UploadDataToBuffer(unsigned int bufferId, const std::vector<float>& data)
	{
		glBindBuffer(GL_ARRAY_BUFFER, bufferId);

		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*data.size(), data.data());

		glUnmapBuffer(GL_ARRAY_BUFFER);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

	}


	void LoadUniformInfo(std::vector<UniformInfo>& data, GLuint progId)
	{
		int numUniforms = 0;
		glGetProgramiv(progId, GL_ACTIVE_UNIFORMS, &numUniforms);
		if (numUniforms > 0)
		{
			data.resize(numUniforms);
			for (int i = 0; i < numUniforms; i++)
			{
				GLsizei lenRet = 0;
				glGetActiveUniform(progId, i, data[i].name.size(), &(data[i].size), &lenRet, &(data[i].type), data[i].name.data());
			}
		}
	}

	void LoadBufferInfo(std::vector<BufferInfo>& data, GLuint progId)
	{
		int numBuffers = 0;
		glGetProgramInterfaceiv(progId, GL_BUFFER_VARIABLE, GL_ACTIVE_RESOURCES, &numBuffers);
		if (numBuffers > 0)
		{
			data.resize(numBuffers);
			for (int i = 0; i < numBuffers; i++)
			{
				GLsizei lenRet = 0;
				std::vector<GLenum> props = {
					GL_NAME_LENGTH, 
					GL_TYPE,
					GL_ARRAY_SIZE,
					GL_OFFSET,
					GL_BLOCK_INDEX,
					GL_ARRAY_STRIDE,
					GL_MATRIX_STRIDE,
					GL_IS_ROW_MAJOR,
					GL_REFERENCED_BY_VERTEX_SHADER,
					GL_REFERENCED_BY_TESS_CONTROL_SHADER,
					GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
					GL_REFERENCED_BY_GEOMETRY_SHADER,
					GL_REFERENCED_BY_FRAGMENT_SHADER,
					GL_REFERENCED_BY_COMPUTE_SHADER,
					GL_TOP_LEVEL_ARRAY_SIZE,
					GL_TOP_LEVEL_ARRAY_STRIDE,
				};

				std::vector<GLint> ret(props.size());

				int len = 0;
				glGetProgramResourceiv(progId, GL_BUFFER_VARIABLE, i, props.size(), props.data(), ret.size(), &len, ret.data());
				data[i].name.resize(ret[0]);
				data[i].type = ret[1];
				data[i].stride = ret[5];

				glGetProgramResourceName(progId, GL_BUFFER_VARIABLE, i, ret[0], &len, &(data[i].name[0]));

			}
		}
	}

	void LoadInputInfo(std::vector<InputInfo>& data, GLuint progId)
	{
		int num = 0;
		glGetProgramInterfaceiv(progId, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &num);
		if (num > 0)
		{
			data.resize(num);
			for (int i = 0; i < num; i++)
			{
				GLsizei lenRet = 0;
				std::vector<GLenum> props = {
					GL_TYPE,
					GL_ARRAY_SIZE,
					GL_REFERENCED_BY_VERTEX_SHADER,
					GL_REFERENCED_BY_TESS_CONTROL_SHADER,
					GL_REFERENCED_BY_TESS_EVALUATION_SHADER,
					GL_REFERENCED_BY_GEOMETRY_SHADER,
					GL_REFERENCED_BY_FRAGMENT_SHADER,
					GL_REFERENCED_BY_COMPUTE_SHADER,
					GL_LOCATION,
					GL_IS_PER_PATCH,
					GL_LOCATION_COMPONENT,
				};

				std::vector<GLint> ret(props.size());

				int len = 0;
				glGetProgramResourceiv(progId, GL_PROGRAM_INPUT, i, props.size(), props.data(), sizeof(GLint)*ret.size(), &len, ret.data());
			}
		}
	}
}
