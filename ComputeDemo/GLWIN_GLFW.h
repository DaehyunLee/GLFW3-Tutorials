#pragma once
#include "GLWin.h"


class GLFWWin : public IGLWin
{
public:
	static bool isInitialized;
	GLFWWin(int width, int height);
	~GLFWWin();

	virtual glm::ivec2 getDimension() const override;
	virtual glm::mat4 getProjection() const override;
	virtual glm::mat4 getView() const override;
	virtual void MakeCurrent() override;
	virtual void SwapBuffers() override;
	virtual void PumpEvent() override;
	virtual double GetElapsedTime() override;

	class GLFWwindow*	myWindow;
	unsigned int	m_uiWidth;
	unsigned int	m_uiHeight;

	glm::mat4	m_m4Projection;
	glm::mat4	m_m4ViewMatrix;
};



