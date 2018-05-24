#include "stdafx.h"
#include "FileUtils.h"

template <typename T>
bool TReadFile(const char * file_name, std::vector<T>& dataBuffer)
{
	std::ifstream inFile;
	inFile.open(file_name, std::ios::in | std::ios::binary | std::ios::ate);

	if (!inFile.is_open())
	{
		std::cout << "Error. Unable to open file.\n";
		return false;
	}

	inFile.seekg(0, std::ios::end); // set the pointer to the end
	int size = inFile.tellg(); // get the length of the file
	std::cout << "Size of file: " << size << "\n";
	inFile.seekg(0, std::ios::beg); // set the pointer to the beginning

	dataBuffer.resize(size + 1);
	inFile.read((char*)dataBuffer.data(), size);
	dataBuffer.back() = '\0';
	return true;
}

namespace FileUtils {
	bool ReadFile(const char * filename, std::vector<char>& dataBuffer)
	{
		return TReadFile(filename, dataBuffer);
	}

	bool ReadFile(const char * filename, std::vector<unsigned char>& dataBuffer)
	{
		return TReadFile(filename, dataBuffer);
	}

};