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
	virtual void PumpEvent() = 0;
	virtual double GetElapsedTime() = 0;

	static IGLWin* Create();
};

