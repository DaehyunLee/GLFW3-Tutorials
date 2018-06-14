#pragma once

namespace FileUtils
{
	bool ReadFile(const char * file_name, std::vector<char>& dataBuffer);
	bool ReadFile(const char * file_name, std::vector<unsigned char>& dataBuffer);
}
