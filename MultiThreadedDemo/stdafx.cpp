// stdafx.cpp : source file that includes just the standard includes
// Correspondence_Tool.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file
#include <stdlib.h>
#include <stdexcept>
#include <sstream>
#include <windows.h>
// TODO: reference any additional headers you need in STDAFX.H
// and not in this file

void EarlyExit(const char* file, int line, int exitcode, const char* condition)
{
	std::ostringstream stringStream;
	stringStream << file << " line :" << line << std::endl;
	stringStream << "code : " << exitcode << std::endl;
	if (condition)
	{
		stringStream << "failed condition : " << condition << std::endl;
	}

	throw std::exception(stringStream.str().c_str());
}

double Frequency;
double SecondsPerCycle;
void StartCounter()
{
	LARGE_INTEGER li;
	if (!QueryPerformanceFrequency(&li))
		EXIT(0);

	Frequency = double(li.QuadPart);
	SecondsPerCycle = 1.0 / Frequency;
}

double GetCounter()
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return double(li.QuadPart) / Frequency;
}

double GetSeconds()
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return li.QuadPart * SecondsPerCycle;
}
