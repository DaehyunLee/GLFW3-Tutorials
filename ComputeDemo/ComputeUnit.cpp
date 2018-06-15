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

		std::vector<UniformInfo> data;
		LoadUniformInfo(data, progId);
		PrintProgramInfo(std::cout, progId);
		return true;
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

}