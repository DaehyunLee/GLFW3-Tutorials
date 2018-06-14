#pragma once
/*
no judgement for making nasty wrapper class.
this is a desparate hope to replace some gl framework on another project.
*/

class IGLWin
{
public:
	virtual glm::ivec2 getDimension() const = 0;
	virtual glm::mat4 getProjection() const = 0;
	virtual glm::mat4 getView() const = 0;
	virtual void MakeCurrent() = 0;
	virtual void SwapBuffers() = 0;

	static IGLWin* Create();
};


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

	GLFWwindow*	myWindow;
	unsigned int	m_uiWidth;
	unsigned int	m_uiHeight;

	glm::mat4	m_m4Projection;
	glm::mat4	m_m4ViewMatrix;
};



