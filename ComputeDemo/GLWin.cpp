#include "stdafx.h"
#include "GLWin.h"
#include "GLWIN_GLFW.h"

IGLWin* IGLWin::Create()
{
	return new GLFWWin(1024, 1024);
}
