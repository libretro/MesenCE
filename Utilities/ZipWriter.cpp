#include "pch.h"
#include <string>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include "ZipWriter.h"
#include "FolderUtilities.h"
#include "VirtualFile.h"

ZipWriter::ZipWriter()
{
}

ZipWriter::~ZipWriter()
{
}

bool ZipWriter::Initialize(string filename)
{
	_zipFilename = filename;
	memset(&_zipArchive, 0, sizeof(mz_zip_archive));
	//The archive is built in memory and written out in Save() - miniz's file
	//API uses stdio directly, which bypasses the frontend VFS
	return mz_zip_writer_init_heap(&_zipArchive, 0, 0) != 0;
}

bool ZipWriter::Save()
{
	void* buffer = nullptr;
	size_t size = 0;
	bool result = mz_zip_writer_finalize_heap_archive(&_zipArchive, &buffer, &size) != 0;

	if(result) {
		ofstream file(_zipFilename, std::ios::out | std::ios::binary);
		if(file) {
			file.write((char*)buffer, size);
			file.close();
			result = file.good();
		} else {
			result = false;
		}
	}

	free(buffer);
	result &= mz_zip_writer_end(&_zipArchive) != 0;
	return result;
}

void ZipWriter::AddFile(string filepath, string zipFilename)
{
	VirtualFile file(filepath);
	vector<uint8_t> fileData;
	if(!file.ReadFile(fileData)) {
		std::cout << "mz_zip_writer_add_file() failed!" << std::endl;
		return;
	}
	AddFile(fileData, zipFilename);
}

void ZipWriter::AddFile(vector<uint8_t>& fileData, string zipFilename)
{
	if(!mz_zip_writer_add_mem(&_zipArchive, zipFilename.c_str(), fileData.data(), fileData.size(), MZ_BEST_COMPRESSION)) {
		std::cout << "mz_zip_writer_add_file() failed!" << std::endl;
	}
}

void ZipWriter::AddFile(std::stringstream& filestream, string zipFilename)
{
	filestream.seekg(0, std::ios::end);
	size_t bufferSize = (size_t)filestream.tellg();
	filestream.seekg(0, std::ios::beg);

	vector<uint8_t> buffer(bufferSize);
	filestream.read((char*)buffer.data(), bufferSize);

	AddFile(buffer, zipFilename);
}
