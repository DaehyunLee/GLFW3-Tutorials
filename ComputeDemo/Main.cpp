// ComputeDemo.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "GLWin.h"
#include "RenderSample.h"


int main()
{
	IGLWin* test = IGLWin::Create();
	RenderSample* sample = RenderSample::Create();



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

		// clear the backbuffer to our clear colour and clear the depth buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 identity;
		sample->g_ModelMatrix = glm::rotate(identity, fDeltaTime * 10.0f, glm::vec3(0.0f, 1.0f, 0.0f));
		sample->Render(test->getProjection(), test->getView());
		test->SwapBuffers();

		// join second render thread
		glfwPollEvents(); // process events!
	}
    return 0;
}

