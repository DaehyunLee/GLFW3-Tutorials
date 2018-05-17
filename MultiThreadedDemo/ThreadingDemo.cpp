// Note that the following includes must be defined in order:
#include "GL\glew.h"
#include "GLFW\glfw3.h"
#include "ThreadingDemo.h"

// Note the the following Includes do not need to be defined in order:
#include <cstdio>
#include <map>
#include <list>
#include <thread>
#include <mutex>
#include <future>
#include <atomic>
#include "glm\glm.hpp"
#include "glm\ext.hpp"
#include <iostream>

// info: http://www.baptiste-wicht.com/2012/04/c11-concurrency-tutorial-advanced-locking-and-condition-variables/
//////////////////////// global Vars //////////////////////////////
std::mutex window_counter_mtx;
unsigned int	g_uiWindowCounter = 0;							// used to set window IDs

std::list<WindowHandle>					g_lWindows;
std::map<unsigned int, unsigned int>	g_mVAOs;
std::map<std::thread::id, WindowHandle> g_mCurrentContextMap;

unsigned int g_VBO = 0;
unsigned int g_IBO = 0;
glm::mat4	g_ModelMatrix;

std::thread *g_tpWin2 = nullptr;
std::mutex g_RenderLock;
GLsync g_MainThreadFenceSync;
GLsync g_SecondThreadFenceSync;
std::atomic_bool g_bShouldClose;
std::atomic_bool g_bDoWork;

std::map<unsigned int, FPSData*> m_mFPSData;
/*LockFreeQueue<cv::Mat> main_T1_queue;
LockFreeQueue<cv::Mat> T1_main_queue;
LockFreeQueue<cv::Mat> main_T2_queue;
LockFreeQueue<cv::Mat> T2_main_queue;*/

//////////////////////// Function Declerations //////////////////////////////
Quad CreateQuad();

int Init();
int MainLoop();
void Render(WindowHandle a_toWindow);
int IndependentRenderLoop();
int ShutDown();

void GLFWErrorCallback(int a_iError, const char* a_szDiscription);
void GLFWWindowSizeCallback(GLFWwindow* a_pWindow, int a_iWidth, int a_iHeight);
void APIENTRY GLErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, void* userParam);
GLEWContext* glewGetContext();   // This needs to be defined for GLEW MX to work, along with the GLEW_MX define in the perprocessor!
void CalcFPS(WindowHandle a_hWindowHandle);

WindowHandle  CreateWindoww(int a_iWidth, int a_iHeight, const std::string& a_szTitle, GLFWmonitor* a_pMonitor, WindowHandle a_hShare);
void MakeContextCurrent(WindowHandle a_hWindowHandle);
bool ShouldClose(WindowHandle a_toWindow);
void RandomWork();


//////////////////////// Function Definitions //////////////////////////////


int main()
{
	int iReturnCode =EC_NO_ERROR;
	//TODO: Add queues
	// Init GLFW:
	// Setup Our GLFW error callback, we do this before Init so we know what goes wrong with init if it fails:
	glfwSetErrorCallback(GLFWErrorCallback);
	if (!glfwInit())
		return EC_GLFW_INIT_FAIL;
	// create a window:
	std::cout << "Master threa (ID " << std::this_thread::get_id() <<") is creating two slave threads\n";
	std::thread renderWindow1(&IndependentRenderLoop);
	std::thread renderWindow2(&IndependentRenderLoop);
	renderWindow1.join();
	renderWindow2.join();


	iReturnCode = ShutDown();
	if (iReturnCode != EC_NO_ERROR)
		return iReturnCode;

	return iReturnCode;
}

void InitShader(WindowHandle a_hWindowHandle) 
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

	a_hWindowHandle->m_shaderId = glCreateProgram();
	glAttachShader(a_hWindowHandle->m_shaderId, vsHandle);
	glAttachShader(a_hWindowHandle->m_shaderId, fsHandle);
	glDeleteShader(vsHandle);
	glDeleteShader(fsHandle);

	// specify Vertex Attribs:
	glBindAttribLocation(a_hWindowHandle->m_shaderId, 0, "Position");
	glBindAttribLocation(a_hWindowHandle->m_shaderId, 1, "UV");
	glBindAttribLocation(a_hWindowHandle->m_shaderId, 2, "Colour");
	glBindFragDataLocation(a_hWindowHandle->m_shaderId, 0, "outColour");

	glLinkProgram(a_hWindowHandle->m_shaderId);
	glGetProgramiv(a_hWindowHandle->m_shaderId, GL_LINK_STATUS, &iSuccess);
	glGetProgramInfoLog(a_hWindowHandle->m_shaderId, sizeof(acLog), 0, acLog);
	if (iSuccess == GL_FALSE)
	{
		printf("Error: failed to link Shader Program!\n");
		printf(acLog);
		printf("\n");
	}

}

void InitTexAndVertex(WindowHandle a_hWindowHandle)
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


	glGenTextures(1, &a_hWindowHandle->m_textureId);
	glBindTexture(GL_TEXTURE_2D, a_hWindowHandle->m_textureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 256, 256, 0, GL_RGBA, GL_FLOAT, ptexData);

	delete ptexData;

	// specify default filtering and wrapping
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// set the texture to use slot 0 in the shader
	GLuint texUniformID = glGetUniformLocation(a_hWindowHandle->m_shaderId, "diffuseTexture");
	glUniform1i(texUniformID, 0);

	// Create VBO/IBO
	glGenBuffers(1, &g_VBO);
	glGenBuffers(1, &g_IBO);
	glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_IBO);


	glBufferData(GL_ARRAY_BUFFER, fQuad.c_uiNoOfVerticies * sizeof(Vertex), fQuad.m_Verticies, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, fQuad.c_uiNoOfIndicies * sizeof(unsigned int), fQuad.m_uiIndicies, GL_STATIC_DRAW);

}

void InitWindow(WindowHandle& a_hWindowHandle)
{
	MakeContextCurrent(a_hWindowHandle);
	// start creating our quad data for later use:
	InitShader(a_hWindowHandle);
	glUseProgram(a_hWindowHandle->m_shaderId);
	InitTexAndVertex(a_hWindowHandle);
}

int Init(WindowHandle& a_toWindow)
{
	// Init GLFW:
	// Setup Our GLFW error callback, we do this before Init so we know what goes wrong with init if it fails:
	/*glfwSetErrorCallback(GLFWErrorCallback);
	if (!glfwInit())
		return EC_GLFW_INIT_FAIL;*/
	// create a window:
	a_toWindow = CreateWindoww(c_iDefaultScreenWidth, c_iDefaultScreenHeight, c_szDefaultWindowTitle, nullptr, nullptr);
	
	if (a_toWindow == nullptr)
	{
		return EC_GLFW_FIRST_WINDOW_CREATION_FAIL;
	}
	
	InitWindow(a_toWindow);

	std::cout << "Init completed on thread ID: " << std::this_thread::get_id() << std::endl;

	return EC_NO_ERROR;
}




int IndependentRenderLoop()
{
	int iReturnCode = EC_NO_ERROR;
	WindowHandle a_toWindow;
	iReturnCode = Init(a_toWindow);
	if (iReturnCode != EC_NO_ERROR) {
		return iReturnCode;
	}
	// Setup VAO:
	g_mVAOs[a_toWindow->m_uiID] = 0;
	glGenVertexArrays(1, &(g_mVAOs[a_toWindow->m_uiID]));
	glBindVertexArray(g_mVAOs[a_toWindow->m_uiID]);
	glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_IBO);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), ((char*)0) + 16);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), ((char*)0) + 24);

	// Setup Matrix:
	a_toWindow->m_m4Projection = glm::perspective(45.0f, float(a_toWindow->m_uiWidth) / float(a_toWindow->m_uiHeight), 0.1f, 1000.0f);
	a_toWindow->m_m4ViewMatrix = glm::lookAt(glm::vec3(a_toWindow->m_uiID * 8, 8, 8), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

	// set OpenGL Options:
	glViewport(0, 0, a_toWindow->m_uiWidth, a_toWindow->m_uiHeight);
	glClearColor(0.0f, 0.0f, 0.25f, 1);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// setup FPS Data
	FPSData* fpsData = new FPSData();
	fpsData->m_fFPS = 0;
	fpsData->m_fTimeBetweenChecks = 3.0f;	// calc fps every 3 seconds!!
	fpsData->m_fFrameCount = 0;
	fpsData->m_fTimeElapsed = 0.0f;
	fpsData->m_fCurrnetRunTime = (float)glfwGetTime();
	m_mFPSData[a_toWindow->m_uiID] = fpsData;
	while (!ShouldClose(a_toWindow))
	{
		// Keep Running!
		// get delta time for this iteration:
		float fDeltaTime = (float)glfwGetTime();

		glm::mat4 identity;
		g_ModelMatrix = glm::rotate(identity, fDeltaTime * 10.0f, glm::vec3(0.0f, 1.0f, 0.0f));

		// render threaded.
		Render(a_toWindow);
		glfwPollEvents();
	}
	std::cout << "Exiting main loop on thread ID: " << std::this_thread::get_id() << std::endl;
	return EC_NO_ERROR;
}

void Render(WindowHandle a_toWindow)
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
		delete window->m_pGLEWContext;
		glfwDestroyWindow(window->m_pWindow);

		delete window;
	}

	glfwTerminate();

	return EC_NO_ERROR;
}


GLEWContext* glewGetContext()
{
	//return g_hCurrentContext->m_pGLEWContext;
	std::thread::id thread = std::this_thread::get_id();

	return g_mCurrentContextMap[thread]->m_pGLEWContext;
}


void MakeContextCurrent(WindowHandle a_hWindowHandle)
{
	if (a_hWindowHandle != nullptr)
	{
		std::thread::id thread = std::this_thread::get_id();

		glfwMakeContextCurrent(a_hWindowHandle->m_pWindow);
		if (g_mCurrentContextMap.find(thread) == g_mCurrentContextMap.end()) {
			g_mCurrentContextMap[thread] = a_hWindowHandle;
		}		
	}
}


WindowHandle CreateWindoww(int a_iWidth, int a_iHeight, const std::string& a_szTitle, GLFWmonitor* a_pMonitor, WindowHandle a_hShare)
{
	// save current active context info so we can restore it later!
	std::thread::id thread = std::this_thread::get_id();

	// create new window data:
	WindowHandle newWindow = new Window();
	if (newWindow == nullptr)
		return nullptr;

	newWindow->m_pGLEWContext = nullptr;
	newWindow->m_pWindow = nullptr;
	window_counter_mtx.lock();
	newWindow->m_uiID = ++g_uiWindowCounter;		// set ID and Increment Counter!
	newWindow->m_uiWidth = a_iWidth;
	newWindow->m_uiHeight = a_iHeight;

	// if compiling in debug ask for debug context:
#ifdef _DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLU_TRUE);
#endif
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GLFW_VERSION_MAJOR);
	std::cout << "Creating window called " << a_szTitle.c_str() << " with ID " << newWindow->m_uiID << std::endl;

	// Create Window:
	if (a_hShare != nullptr) // Check that the Window Handle passed in is valid.
	{
		newWindow->m_pWindow = glfwCreateWindow(a_iWidth, a_iHeight, a_szTitle.c_str(), a_pMonitor, a_hShare->m_pWindow);  // Window handle is valid, Share its GL Context Data!
	}
	else
	{
		newWindow->m_pWindow = glfwCreateWindow(a_iWidth, a_iHeight, (a_szTitle + std::to_string(newWindow->m_uiID)).c_str(), a_pMonitor, nullptr); // Window handle is invlad, do not share!
	}
	// Confirm window was created successfully:
	if (newWindow->m_pWindow == nullptr)
	{
		printf("Error: Could not Create GLFW Window!\n");
		delete newWindow;
		window_counter_mtx.unlock();
		return nullptr;
	}

	// create GLEW Context:
	newWindow->m_pGLEWContext = new GLEWContext();
	if (newWindow->m_pGLEWContext == nullptr)
	{
		printf("Error: Could not create GLEW Context!\n");
		delete newWindow;
		window_counter_mtx.unlock();
		return nullptr;
	}

	MakeContextCurrent(newWindow);	// Must be done before init of GLEW for this new windows context, and must be made current too :)
	// Init GLEW for this context:
	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		// a problem occured when trying to init glew, report it:
		printf("GLEW Error occured, Description: %s\n", glewGetErrorString(err));
		glfwDestroyWindow(newWindow->m_pWindow);
		delete newWindow;
		window_counter_mtx.unlock();
		return nullptr;
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
            glDebugMessageCallback(GLErrorCallback, NULL);                        // define the callback function.
    }
	g_lWindows.push_back(newWindow);
	window_counter_mtx.unlock();
	return newWindow;
}


void CalcFPS(WindowHandle a_hWindowHandle)
{
	FPSData* data = m_mFPSData[a_hWindowHandle->m_uiID];
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
			std::cout << "Thread id: " << std::this_thread::get_id() << "  Window: " <<  a_hWindowHandle->m_uiID << " FPS = " << (int)data->m_fFPS << std::endl;
		}
	}
}


bool ShouldClose(WindowHandle a_toWindow)
{
	if (glfwWindowShouldClose(a_toWindow->m_pWindow))
	{
		return true;
	}
	else {
		return false;
	}	
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
	WindowHandle window = nullptr;
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

	std::thread::id thread = std::this_thread::get_id();
	MakeContextCurrent(window);
	glViewport(0, 0, a_iWidth, a_iHeight);
}

void RandomWork() {
	Sleep(5000);
}
