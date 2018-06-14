// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once
#define WIN32_LEAN_AND_MEAN

#define __STDC_LIMIT_MACROS

#define GPU_FEATURE_WIDTH 108
#define GPU_FEATURE_HEIGHT 86


#include <strsafe.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <assert.h>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <list>
#include <set>
#include <iostream>
#include <sstream>
#include <array>
#include <numeric>
#include <memory>
#include <ppltasks.h>

#include <type_traits>


//TODO: reduce cross referencing code between projects.
#include "GL\glew.h"
#include "GLFW\glfw3.h"

void EarlyExit(const char* file, int line, int exitcode, const char* condition = nullptr);
#define EXIT(code) EarlyExit( __FILE__, __LINE__, code);
#define CHECK(condition) if (!(condition)) EarlyExit( __FILE__, __LINE__, 0, #condition);

void StartCounter();
double GetCounter();
double GetSeconds();
// TODO: reference additional headers your program requires here


class RegisteredLog
{
public:
	RegisteredLog(const char* msg, const char* name, int line);
	static void Flush();
	static std::vector<std::string> Log;
	static double lastLogged;
};

constexpr bool enable_debug_messages = false;

#define TODO(MSG) static RegisteredLog hopefully_unique_name(MSG , __FILE__, __LINE__)
#define FLUSH_TODO RegisteredLog::Flush();

GLEWContext* glewGetContext();   // This needs to be defined for GLEW MX to work, along with the GLEW_MX define in the perprocessor!

#pragma optimize("", off)