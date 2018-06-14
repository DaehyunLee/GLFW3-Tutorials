// ComputeDemo.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "GLWin.h"

/////////////////////////// Shaders ///////////////////////////////////
const char *c_szVertexShader = "#version 330\n"
"in vec4 Position;\n"
"in vec2 UV;\n"
"in vec4 Colour;\n"
"out vec2 vUV;\n"
"out vec4 vColour;\n"
"uniform mat4 Projection;\n"
"uniform mat4 View;\n"
"uniform mat4 Model;\n"
"void main()\n"
"{\n"
"vUV = UV;\n"
"vColour = Colour;"
"gl_Position = Projection * View * Model * Position;\n"
"}\n"
"\n";

const char *c_szPixelShader = "#version 330\n"
"in vec2 vUV;\n"
"in vec4 vColour;\n"
"out vec4 outColour;\n"
"uniform sampler2D diffuseTexture;\n"
"void main()\n"
"{\n"
"outColour = texture2D(diffuseTexture, vUV) + vColour;\n"
"}\n"
"\n";


struct Vertex
{
	glm::vec4 m_v4Position;
	glm::vec2 m_v2UV;
	glm::vec4 m_v4Colour;
};

struct Quad
{
	static const unsigned int	c_uiNoOfIndicies = 6;
	static const unsigned int	c_uiNoOfVerticies = 4;
	Vertex						m_Verticies[c_uiNoOfVerticies];
	unsigned int				m_uiIndicies[c_uiNoOfIndicies];
};

Quad CreateQuad()
{
	Quad geom;

	geom.m_Verticies[0].m_v4Position = glm::vec4(-2, 0, -2, 1);
	geom.m_Verticies[0].m_v2UV = glm::vec2(0, 0);
	geom.m_Verticies[0].m_v4Colour = glm::vec4(0, 1, 0, 1);
	geom.m_Verticies[1].m_v4Position = glm::vec4(2, 0, -2, 1);
	geom.m_Verticies[1].m_v2UV = glm::vec2(1, 0);
	geom.m_Verticies[1].m_v4Colour = glm::vec4(1, 0, 0, 1);
	geom.m_Verticies[2].m_v4Position = glm::vec4(2, 0, 2, 1);
	geom.m_Verticies[2].m_v2UV = glm::vec2(1, 1);
	geom.m_Verticies[2].m_v4Colour = glm::vec4(0, 1, 0, 1);
	geom.m_Verticies[3].m_v4Position = glm::vec4(-2, 0, 2, 1);
	geom.m_Verticies[3].m_v2UV = glm::vec2(0, 1);
	geom.m_Verticies[3].m_v4Colour = glm::vec4(0, 0, 1, 1);

	geom.m_uiIndicies[0] = 3;
	geom.m_uiIndicies[1] = 1;
	geom.m_uiIndicies[2] = 0;
	geom.m_uiIndicies[3] = 3;
	geom.m_uiIndicies[4] = 2;
	geom.m_uiIndicies[5] = 1;


	return geom;
}


unsigned int	m_shaderId;
unsigned int	m_textureId;
unsigned int	g_VBO = 0;
unsigned int	g_IBO = 0;
unsigned int	g_VAO = 0;

glm::mat4	g_ModelMatrix;

int main()
{
	IGLWin* test = IGLWin::Create();


	// create shader:
	GLint iSuccess = 0;
	GLchar acLog[256];
	GLuint vsHandle = glCreateShader(GL_VERTEX_SHADER);
	GLuint fsHandle = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vsHandle, 1, (const char**)&c_szVertexShader, 0);
	glCompileShader(vsHandle);
	glGetShaderiv(vsHandle, GL_COMPILE_STATUS, &iSuccess);
	glGetShaderInfoLog(vsHandle, sizeof(acLog), 0, acLog);
	if (iSuccess == GL_FALSE)
	{
		printf("Error: Failed to compile vertex shader!\n");
		printf(acLog);
		printf("\n");
	}

	glShaderSource(fsHandle, 1, (const char**)&c_szPixelShader, 0);
	glCompileShader(fsHandle);
	glGetShaderiv(fsHandle, GL_COMPILE_STATUS, &iSuccess);
	glGetShaderInfoLog(fsHandle, sizeof(acLog), 0, acLog);
	if (iSuccess == GL_FALSE)
	{
		printf("Error: Failed to compile fragment shader!\n");
		printf(acLog);
		printf("\n");
	}

	m_shaderId = glCreateProgram();
	glAttachShader(m_shaderId, vsHandle);
	glAttachShader(m_shaderId, fsHandle);
	glDeleteShader(vsHandle);
	glDeleteShader(fsHandle);

	// specify Vertex Attribs:
	glBindAttribLocation(m_shaderId, 0, "Position");
	glBindAttribLocation(m_shaderId, 1, "UV");
	glBindAttribLocation(m_shaderId, 2, "Colour");
	glBindFragDataLocation(m_shaderId, 0, "outColour");

	glLinkProgram(m_shaderId);
	glGetProgramiv(m_shaderId, GL_LINK_STATUS, &iSuccess);
	glGetProgramInfoLog(m_shaderId, sizeof(acLog), 0, acLog);
	if (iSuccess == GL_FALSE)
	{
		printf("Error: failed to link Shader Program!\n");
		printf(acLog);
		printf("\n");
	}

	glUseProgram(m_shaderId);

	Quad fQuad = CreateQuad();
	glm::vec4 *ptexData = new glm::vec4[256 * 256];
	for (int i = 0; i < 256 * 256; i += 256)
	{
		for (int j = 0; j < 256; ++j)
		{
			if (j % 2 == 0)
				ptexData[i + j] = glm::vec4(0, 0, 0, 1);
			else
				ptexData[i + j] = glm::vec4(1, 1, 1, 1);
		}
	}


	glGenTextures(1, &m_textureId);
	glBindTexture(GL_TEXTURE_2D, m_textureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 256, 256, 0, GL_RGBA, GL_FLOAT, ptexData);

	delete ptexData;

	// specify default filtering and wrapping
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// set the texture to use slot 0 in the shader
	GLuint texUniformID = glGetUniformLocation(m_shaderId, "diffuseTexture");
	glUniform1i(texUniformID, 0);

	// Create VBO/IBO
	glGenBuffers(1, &g_VBO);
	glGenBuffers(1, &g_IBO);
	glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_IBO);


	glBufferData(GL_ARRAY_BUFFER, fQuad.c_uiNoOfVerticies * sizeof(Vertex), fQuad.m_Verticies, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, fQuad.c_uiNoOfIndicies * sizeof(unsigned int), fQuad.m_uiIndicies, GL_STATIC_DRAW);



	// Setup VAO:
	glGenVertexArrays(1, &g_VAO);
	glBindVertexArray(g_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_IBO);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), ((char*)0) + 16);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), ((char*)0) + 24);


	// set OpenGL Options:
	glm::ivec2 dim = test->getDimension();
	glViewport(0, 0, dim[0], dim[1]);
	glClearColor(0, 0, 0.25f, 1);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);


	while (1)
	{
		// Keep Running!
		// get delta time for this iteration:
		float fDeltaTime = (float)glfwGetTime();
		glm::mat4 identity;
		g_ModelMatrix = glm::rotate(identity, fDeltaTime * 10.0f, glm::vec3(0.0f, 1.0f, 0.0f));

		// clear the backbuffer to our clear colour and clear the depth buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(m_shaderId);

		GLuint ProjectionID = glGetUniformLocation(m_shaderId, "Projection");
		GLuint ViewID = glGetUniformLocation(m_shaderId, "View");
		GLuint ModelID = glGetUniformLocation(m_shaderId, "Model");

		glUniformMatrix4fv(ProjectionID, 1, false, glm::value_ptr(test->getProjection()));
		glUniformMatrix4fv(ViewID, 1, false, glm::value_ptr(test->getProjection()));
		glUniformMatrix4fv(ModelID, 1, false, glm::value_ptr(g_ModelMatrix));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_textureId);
		glBindVertexArray(g_VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		test->SwapBuffers();

		// join second render thread
		glfwPollEvents(); // process events!
	}
    return 0;
}

