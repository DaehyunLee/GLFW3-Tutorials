#include "stdafx.h"
#include "GLWIN_GLFW.h"


#include "GLFW\glfw3.h"

bool GLFWWin::isInitialized = false;


GLFWWin::GLFWWin(int width, int height)
	: m_uiWidth(width)
	, m_uiHeight(height)
{
	if (!isInitialized)
	{
		if (!glfwInit())
		{
			throw std::exception("glflwInit fail..");
		}

		isInitialized = true;
	}

	myWindow = glfwCreateWindow(m_uiWidth, m_uiHeight, "testing compute", nullptr, nullptr);
	MakeCurrent();
	// Init GLFW:
	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		throw std::exception("glew bailed.");
	}

	// Setup Matrix:
	m_m4Projection = glm::perspective(45.0f, float(m_uiWidth) / float(m_uiHeight), 0.1f, 1000.0f);
	m_m4ViewMatrix = glm::lookAt(glm::vec3(4, 4, 4), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
}


GLFWWin::~GLFWWin()
{
}

glm::ivec2 GLFWWin::getDimension() const
{
	return glm::ivec2(m_uiWidth, m_uiHeight);
}

glm::mat4 GLFWWin::getProjection() const
{
	return m_m4Projection;
}

glm::mat4 GLFWWin::getView() const
{
	return m_m4ViewMatrix;
}

void GLFWWin::MakeCurrent()
{
	glfwMakeContextCurrent(myWindow);
}

void GLFWWin::SwapBuffers()
{
	glfwSwapBuffers(myWindow);
}

void GLFWWin::PumpEvent()
{
	glfwPollEvents();
}

double GLFWWin::GetElapsedTime()
{
	return glfwGetTime();
}
