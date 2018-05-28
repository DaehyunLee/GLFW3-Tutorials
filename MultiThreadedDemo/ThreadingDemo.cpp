// Note that the following includes must be defined in order:
#include "GL\glew.h"
#include "GLFW\glfw3.h"
#include "ThreadingDemo.h"

// Note the the following Includes do not need to be defined in order:
#include <cstdio>
#include <map>
#include <list>
#include <array>
#include <thread>
#include <future>
#include <atomic>
#include "glm\glm.hpp"
#include "glm\ext.hpp"
#include <iostream>

// info: http://www.baptiste-wicht.com/2012/04/c11-concurrency-tutorial-advanced-locking-and-condition-variables/
//////////////////////// global Vars //////////////////////////////
unsigned int	g_uiWindowCounter = 0;							// used to set window IDs

std::list<Window*>					g_lWindows;
std::map<unsigned int, unsigned int>	g_mVAOs;

unsigned int g_VBO = 0;
unsigned int g_IBO = 0;
glm::mat4	g_ModelMatrix;
std::mutex g_RenderLock;
thread_local Window* g_currentThreadContext;

std::map<unsigned int, FPSData*> m_mFPSData;

//////////////////////// Function Declerations //////////////////////////////
Quad CreateQuad();

int Init();
int MainLoopBAD();
void Render(Window* a_toWindow);
void IndependantRenderLoop(Window* a_toWindow);
int ShutDown();

void GLFWErrorCallback(int a_iError, const char* a_szDiscription);
void GLFWWindowSizeCallback(GLFWwindow* a_pWindow, int a_iWidth, int a_iHeight);
void APIENTRY GLErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, void* userParam);
#ifdef GLEW_MX
GLEWContext* glewGetContext();   // This needs to be defined for GLEW MX to work, along with the GLEW_MX define in the perprocessor!
#endif
void CalcFPS(Window* a_hWindow);

Window*  CreateWindow(int a_iWidth, int a_iHeight, const std::string& a_szTitle, GLFWmonitor* a_pMonitor, Window* a_hShare);
void MakeContextCurrent(Window* a_hWindow);
bool ShouldClose();


//////////////////////// Function Definitions //////////////////////////////
int main()
{
	int iReturnCode =EC_NO_ERROR;

	iReturnCode = Init();
	if (iReturnCode != EC_NO_ERROR)
		return iReturnCode;

	// use this to simulate 3ms of work per window.
	iReturnCode = MainLoopBAD();
	if (iReturnCode != EC_NO_ERROR)
		return iReturnCode;

	iReturnCode = ShutDown();
	if (iReturnCode != EC_NO_ERROR)
		return iReturnCode;

	return iReturnCode;
}

void InitShader(Window* a_hWindow) 
{
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

	a_hWindow->m_shaderId = glCreateProgram();
	glAttachShader(a_hWindow->m_shaderId, vsHandle);
	glAttachShader(a_hWindow->m_shaderId, fsHandle);
	glDeleteShader(vsHandle);
	glDeleteShader(fsHandle);

	// specify Vertex Attribs:
	glBindAttribLocation(a_hWindow->m_shaderId, 0, "Position");
	glBindAttribLocation(a_hWindow->m_shaderId, 1, "UV");
	glBindAttribLocation(a_hWindow->m_shaderId, 2, "Colour");
	glBindFragDataLocation(a_hWindow->m_shaderId, 0, "outColour");

	glLinkProgram(a_hWindow->m_shaderId);
	glGetProgramiv(a_hWindow->m_shaderId, GL_LINK_STATUS, &iSuccess);
	glGetProgramInfoLog(a_hWindow->m_shaderId, sizeof(acLog), 0, acLog);
	if (iSuccess == GL_FALSE)
	{
		printf("Error: failed to link Shader Program!\n");
		printf(acLog);
		printf("\n");
	}

}

void InitTexAndVertex(Window* a_hWindow)
{
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


	glGenTextures(1, &a_hWindow->m_textureId);
	glBindTexture(GL_TEXTURE_2D, a_hWindow->m_textureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 256, 256, 0, GL_RGBA, GL_FLOAT, ptexData);

	delete ptexData;

	// specify default filtering and wrapping
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// set the texture to use slot 0 in the shader
	GLuint texUniformID = glGetUniformLocation(a_hWindow->m_shaderId, "diffuseTexture");
	glUniform1i(texUniformID, 0);

	// Create VBO/IBO
	glGenBuffers(1, &g_VBO);
	glGenBuffers(1, &g_IBO);
	glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_IBO);


	glBufferData(GL_ARRAY_BUFFER, fQuad.c_uiNoOfVerticies * sizeof(Vertex), fQuad.m_Verticies, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, fQuad.c_uiNoOfIndicies * sizeof(unsigned int), fQuad.m_uiIndicies, GL_STATIC_DRAW);

}

void InitWindow(Window* a_hWindow)
{
	MakeContextCurrent(a_hWindow);
	// start creating our quad data for later use:
	InitShader(a_hWindow);
	glUseProgram(a_hWindow->m_shaderId);
	InitTexAndVertex(a_hWindow);
}

void PrintGLVersions(Window* a_hWindow)
{
	// Print out GLFW, OpenGL version and GLEW Version:
	int iOpenGLMajor = glfwGetWindowAttrib(a_hWindow->m_pWindow, GLFW_CONTEXT_VERSION_MAJOR);
	int iOpenGLMinor = glfwGetWindowAttrib(a_hWindow->m_pWindow, GLFW_CONTEXT_VERSION_MINOR);
	int iOpenGLRevision = glfwGetWindowAttrib(a_hWindow->m_pWindow, GLFW_CONTEXT_REVISION);
	printf("Status: Using GLFW Version %s\n", glfwGetVersionString());
	printf("Status: Using OpenGL Version: %i.%i, Revision: %i\n", iOpenGLMajor, iOpenGLMinor, iOpenGLRevision);
	printf("Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
}

int Init()
{
	// Setup Our GLFW error callback, we do this before Init so we know what goes wrong with init if it fails:
	glfwSetErrorCallback(GLFWErrorCallback);

	// Init GLFW:
	if (!glfwInit())
		return EC_GLFW_INIT_FAIL;

	// create our first window:
	if (nullptr == CreateWindow(c_iDefaultScreenWidth, c_iDefaultScreenHeight, "Threading Demo - Primary Window", nullptr, nullptr))
	{
		glfwTerminate();
		return EC_GLFW_FIRST_WINDOW_CREATION_FAIL;
	}
	PrintGLVersions(g_lWindows.front());
	// create our second or third or however many windows:
	CreateWindow(c_iDefaultScreenWidth, c_iDefaultScreenHeight, "Threading Demo - secondary Window", nullptr, nullptr);
	CreateWindow(c_iDefaultScreenWidth, c_iDefaultScreenHeight, "Threading Demo - third Window", nullptr, nullptr);
	CreateWindow(c_iDefaultScreenWidth, c_iDefaultScreenHeight, "Threading Demo - fourth Window", nullptr, nullptr);
	CreateWindow(c_iDefaultScreenWidth, c_iDefaultScreenHeight, "Threading Demo - fifth Window", nullptr, nullptr);
	CreateWindow(c_iDefaultScreenWidth, c_iDefaultScreenHeight, "Threading Demo - sixth Window", nullptr, nullptr);
	CreateWindow(c_iDefaultScreenWidth, c_iDefaultScreenHeight, "Threading Demo - seventh Window", nullptr, nullptr);
	CreateWindow(c_iDefaultScreenWidth, c_iDefaultScreenHeight, "Threading Demo - eighth Window", nullptr, nullptr);

	const std::array<float[3], 3> colors = { {
		{ 0.0f, 0.0f, 0.25f },
		{ 0.25f, 0.0f, 0.25f },
		{ 0.0f, 0.25f, 0.f },
	} };

	// Now do window specific stuff, including:
	// --> Creating a VAO with the VBO/IBO created above!
	// --> Setting Up Projection and View Matricies!
	// --> Specifing OpenGL Options for the window!
	for (auto window : g_lWindows)
	{
		InitWindow(window);
		MakeContextCurrent(window);
		
		// Setup VAO:
		g_mVAOs[window->m_uiID] = 0;
		glGenVertexArrays(1, &(g_mVAOs[window->m_uiID]));
		glBindVertexArray(g_mVAOs[window->m_uiID]);
		glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_IBO);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), ((char*)0) + 16);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), ((char*)0) + 24);

		// Setup Matrix:
		window->m_m4Projection = glm::perspective(45.0f, float(window->m_uiWidth)/float(window->m_uiHeight), 0.1f, 1000.0f);
		window->m_m4ViewMatrix = glm::lookAt(glm::vec3(window->m_uiID * 8,8,8), glm::vec3(0,0,0), glm::vec3(0,1,0));

		// set OpenGL Options:
		glViewport(0, 0, window->m_uiWidth, window->m_uiHeight);
		int idx = window->m_uiID%colors.size();
		glClearColor(colors[idx][0], colors[idx][1], colors[idx][2], 1);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);

		// setup FPS Data
		FPSData* fpsData = new FPSData();
		fpsData->m_fFPS = 0;
		fpsData->m_fTimeBetweenChecks = 3.0f;	// calc fps every 3 seconds!!
		fpsData->m_fFrameCount = 0;
		fpsData->m_fTimeElapsed = 0.0f;
		fpsData->m_fCurrnetRunTime = (float)glfwGetTime();
		m_mFPSData[window->m_uiID] = fpsData;
	}
	std::cout << "Init completed on thread ID: " << std::this_thread::get_id() << std::endl;

	return EC_NO_ERROR;
}





int MainLoopBAD()
{
	std::cout << "Entering main loop on thread ID: " << std::this_thread::get_id() << std::endl;

	glfwMakeContextCurrent(NULL);
	std::vector<std::thread> renderThreads;
	for (auto& window : g_lWindows)
	{
		renderThreads.push_back(std::thread(&IndependantRenderLoop, window));
	}

	while (!ShouldClose())
	{
		// Keep Running!
		// get delta time for this iteration:
		float fDeltaTime = (float)glfwGetTime();

		// join second render thread
		glfwPollEvents(); // process events!
	}
	std::for_each(renderThreads.begin(), renderThreads.end(), [](auto& t) {
		t.join();
	});
	std::cout << "Exiting main loop on thread ID: " << std::this_thread::get_id() << std::endl;

	return EC_NO_ERROR;
}



void IndependantRenderLoop(Window* a_toWindow)
{
	MakeContextCurrent(a_toWindow);
	//this seems to be turning of the vsync, but it's a bit hard to tell. my cpu is hitting 100% before gpu goes beyond 60%
	// in the future, if this doesn't seem like its doing the job done, try turning it off on Nvidia control panel.
	glfwSwapInterval(0);

	while (!ShouldClose())
	{
		// Keep Running!
		// get delta time for this iteration:
		float fDeltaTime = (float)glfwGetTime();

		glm::mat4 identity;
		g_ModelMatrix = glm::rotate(identity, fDeltaTime * 10.0f, glm::vec3(0.0f, 1.0f, 0.0f));

		// render threaded.
		Render(a_toWindow);
	}

	std::cout << "Exiting main loop on thread ID: " << std::this_thread::get_id() << std::endl;
}

void Render(Window* a_toWindow)
{
		
	// clear the backbuffer to our clear colour and clear the depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(a_toWindow->m_shaderId);

	GLuint ProjectionID = glGetUniformLocation(a_toWindow->m_shaderId,"Projection");
	GLuint ViewID = glGetUniformLocation(a_toWindow->m_shaderId,"View");
	GLuint ModelID = glGetUniformLocation(a_toWindow->m_shaderId,"Model");

	glUniformMatrix4fv(ProjectionID, 1, false, glm::value_ptr(a_toWindow->m_m4Projection));
	glUniformMatrix4fv(ViewID, 1, false, glm::value_ptr(a_toWindow->m_m4ViewMatrix));
	glUniformMatrix4fv(ModelID, 1, false, glm::value_ptr(g_ModelMatrix));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture( GL_TEXTURE_2D, a_toWindow->m_textureId);
	glBindVertexArray(g_mVAOs[a_toWindow->m_uiID]);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glfwSwapBuffers(a_toWindow->m_pWindow);  // make this loop through all current windows??

	//CheckForGLErrors("Render Error");
}


int ShutDown()
{
	// delete the FPS data:
	for (auto itr : m_mFPSData)
	{
		delete itr.second;
	}

	// cleanup any remaining windows:
	for (auto& window :g_lWindows)
	{
#ifdef GLEW_MX
		delete window->m_pGLEWContext;
#endif
		glfwDestroyWindow(window->m_pWindow);

		delete window;
	}

	// terminate GLFW:
	glfwTerminate();

	return EC_NO_ERROR;
}


#ifdef GLEW_MX
GLEWContext* glewGetContext()
{
	return g_currentThreadContext->m_pGLEWContext;
}
#endif

void MakeContextCurrent(Window* a_hWindow)
{
	if (a_hWindow != nullptr)
	{
		glfwMakeContextCurrent(a_hWindow->m_pWindow);
		g_currentThreadContext = a_hWindow;
		
		if (a_hWindow->m_pWindow != glfwGetCurrentContext())
		{
			printf("context not as expected\n");
		}
	}
	else
	{
		printf("something's mess up\n");
	}
}


Window* CreateWindow(int a_iWidth, int a_iHeight, const std::string& a_szTitle, GLFWmonitor* a_pMonitor, Window* a_hShare)
{
	// create new window data:
	Window* newWindow = new Window();
	if (newWindow == nullptr)
		return nullptr;

#ifdef GLEW_MX
	newWindow->m_pGLEWContext = nullptr;
#endif
	newWindow->m_pWindow = nullptr;
	newWindow->m_uiID = g_uiWindowCounter++;		// set ID and Increment Counter!
	newWindow->m_uiWidth = a_iWidth;
	newWindow->m_uiHeight = a_iHeight;

	// if compiling in debug ask for debug context:
#ifdef _DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLU_TRUE);
#endif

	std::cout << "Creating window called " << a_szTitle.c_str() << " with ID " << newWindow->m_uiID << std::endl;

	// Create Window:
	if (a_hShare != nullptr) // Check that the Window Handle passed in is valid.
	{
		newWindow->m_pWindow = glfwCreateWindow(a_iWidth, a_iHeight, a_szTitle.c_str(), a_pMonitor, a_hShare->m_pWindow);  // Window handle is valid, Share its GL Context Data!
	}
	else
	{
		newWindow->m_pWindow = glfwCreateWindow(a_iWidth, a_iHeight, a_szTitle.c_str(), a_pMonitor, nullptr); // Window handle is invlad, do not share!
	}

	// Confirm window was created successfully:
	if (newWindow->m_pWindow == nullptr)
	{
		printf("Error: Could not Create GLFW Window!\n");
		delete newWindow;
		return nullptr;
	}

	// create GLEW Context:
#ifdef GLEW_MX
	newWindow->m_pGLEWContext = new GLEWContext();
	if (newWindow->m_pGLEWContext == nullptr)
	{
		printf("Error: Could not create GLEW Context!\n");
		delete newWindow;
		return nullptr;
	}
#endif
	glfwMakeContextCurrent(newWindow->m_pWindow);   // Must be done before init of GLEW for this new windows Context!
	MakeContextCurrent(newWindow);					// and must be made current too :)
	
	// Init GLEW for this context:
	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		// a problem occured when trying to init glew, report it:
		printf("GLEW Error occured, Description: %s\n", glewGetErrorString(err));
		glfwDestroyWindow(newWindow->m_pWindow);
		delete newWindow;
		return nullptr;
	}

	if (glfwExtensionSupported("WGL_EXT_swap_control_tear"))
	{
		std::cout << "WGL_EXT_swap_control_tear\n";
	}

	if (glfwExtensionSupported("GLX_EXT_swap_control_tear"))
	{
		std::cout << "GLX_EXT_swap_control_tear\n";
	}
	
	// setup callbacks:
	// setup callback for window size changes:
	glfwSetWindowSizeCallback(newWindow->m_pWindow, GLFWWindowSizeCallback);

	 // setup openGL Error callback:
	const bool use_glErrCallback = false;
    if (use_glErrCallback&&GLEW_ARB_debug_output) // test to make sure we can use the new callbacks, they wer added as an extgension in 4.1 and as a core feture in 4.3
    {
            #ifdef _DEBUG
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);                        // this allows us to set a break point in the callback function, no point to it if in release mode.
            #endif
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);        // tell openGl what errors we want (all).
//            glDebugMessageCallback(GLErrorCallback, NULL);                        // define the callback function.
    }

	// add new window to the map and increment handle counter:
	g_lWindows.push_back(newWindow);

	return newWindow;
}


void CalcFPS(Window* a_hWindow)
{
	FPSData* data = m_mFPSData[a_hWindow->m_uiID];
	if (data != nullptr)
	{
		data->m_fFrameCount++;
		data->m_fPreviousRunTime = data->m_fCurrnetRunTime;
		data->m_fCurrnetRunTime = (float)glfwGetTime();
		data->m_fTimeElapsed += data->m_fCurrnetRunTime - data->m_fPreviousRunTime;
		if (data->m_fTimeElapsed >= data->m_fTimeBetweenChecks)
		{
			data->m_fFPS = data->m_fFrameCount / data->m_fTimeElapsed;
			data->m_fTimeElapsed = 0.0f;
			data->m_fFrameCount = 0;
			std::cout << "Thread id: " << std::this_thread::get_id() << "  Window: " <<  a_hWindow->m_uiID << " FPS = " << (int)data->m_fFPS << std::endl;
		}
	}
}


bool ShouldClose()
{
	for (const auto& window : g_lWindows)
	{
		if (glfwWindowShouldClose(window->m_pWindow))
		{
			return true;
		}
	}

	return false;
}


Quad CreateQuad()
{
	Quad geom;

	geom.m_Verticies[0].m_v4Position = glm::vec4(-2,0,-2,1);
	geom.m_Verticies[0].m_v2UV = glm::vec2(0,0);
	geom.m_Verticies[0].m_v4Colour = glm::vec4(0,1,0,1);
	geom.m_Verticies[1].m_v4Position = glm::vec4(2,0,-2,1);
	geom.m_Verticies[1].m_v2UV = glm::vec2(1,0);
	geom.m_Verticies[1].m_v4Colour = glm::vec4(1,0,0,1);
	geom.m_Verticies[2].m_v4Position = glm::vec4(2,0,2,1);
	geom.m_Verticies[2].m_v2UV = glm::vec2(1,1);
	geom.m_Verticies[2].m_v4Colour = glm::vec4(0,1,0,1);
	geom.m_Verticies[3].m_v4Position = glm::vec4(-2,0,2,1);
	geom.m_Verticies[3].m_v2UV = glm::vec2(0,1);
	geom.m_Verticies[3].m_v4Colour = glm::vec4(0,0,1,1);

	geom.m_uiIndicies[0] = 3;
	geom.m_uiIndicies[1] = 1;
	geom.m_uiIndicies[2] = 0;
	geom.m_uiIndicies[3] = 3;
	geom.m_uiIndicies[4] = 2;
	geom.m_uiIndicies[5] = 1;

	printf("Created quad on thread ID: %i\n", std::this_thread::get_id());

	return geom;
}


void GLFWErrorCallback(int a_iError, const char* a_szDiscription)
{
	printf("GLFW Error occured, Error ID: %i, Description: %s\n", a_iError, a_szDiscription);
}


void APIENTRY GLErrorCallback(GLenum /* source */, GLenum type, GLuint id, GLenum severity, GLsizei /* length */, const GLchar* message, void* /* userParam */)
{
	std::cout << "---------------------opengl-callback-start------------" << std::endl;
	std::cout << "Message: " << message << std::endl;
	std::cout << "Type: "; 
    switch (type) 
        {
    case GL_DEBUG_TYPE_ERROR:
		std::cout << "ERROR" << std::endl; 
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		std::cout << "DEPRECATED_BEHAVIOR" << std::endl; 
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		std::cout << "UNDEFINED_BEHAVIOR" << std::endl; 
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
		std::cout << "PORTABILITY" << std::endl; 
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
		std::cout << "PERFORMANCE" << std::endl; 
        break;
    case GL_DEBUG_TYPE_OTHER:
		std::cout << "OTHER" << std::endl; 
        break;
    }

	std::cout << "ID: " << id << ", Severity: ";
    switch (severity)
        {
    case GL_DEBUG_SEVERITY_LOW:
		std::cout << "LOW" << std::endl; 
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
		std::cout << "MEDIUM" << std::endl; 
        break;
    case GL_DEBUG_SEVERITY_HIGH:
		std::cout << "HIGH" << std::endl; 
        break;
    }

	std::cout << "---------------------opengl-callback-end--------------" << std::endl;
}


void GLFWWindowSizeCallback(GLFWwindow* a_pWindow, int a_iWidth, int a_iHeight)
{
	// find the window data corrosponding to a_pWindow;
	return;
	Window* window = nullptr;
	for (auto& itr : g_lWindows)
	{
		if (itr->m_pWindow == a_pWindow)
		{
			window = itr;
			window->m_uiWidth = a_iWidth;
			window->m_uiHeight = a_iHeight;
			window->m_m4Projection = glm::perspective(45.0f, float(a_iWidth)/float(a_iHeight), 0.1f, 1000.0f);
		}
	}

	MakeContextCurrent(window);
	glViewport(0, 0, a_iWidth, a_iHeight);
}
