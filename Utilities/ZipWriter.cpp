#include "pch.h"
#include <string>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include "ZipWriter.h"
#include "FolderUtilities.h"
#ifdef LIBRETRO
	#include "VirtualFile.h"
#endif

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
#ifdef LIBRETRO
	//miniz's file API goes straight to stdio, which bypasses the frontend VFS -
	//build the archive in memory and write it out through the stream layer
	return mz_zip_writer_init_heap(&_zipArchive, 0, 0) != 0;
#else
	return mz_zip_writer_init_file(&_zipArchive, _zipFilename.c_str(), 0) != 0;
#endif
}

bool ZipWriter::Save()
{
#ifdef LIBRETRO
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
#else
	bool result = mz_zip_writer_finalize_archive(&_zipArchive) != 0;
	result &= mz_zip_writer_end(&_zipArchive) != 0;
	return result;
#endif
}

void ZipWriter::AddFile(string filepath, string zipFilename)
{
#ifdef LIBRETRO
	//Same reason as above: read the file through the VFS-aware stream layer
	VirtualFile file(filepath);
	vector<uint8_t> fileData;
	if(!file.ReadFile(fileData)) {
		std::cout << "mz_zip_writer_add_file() failed!" << std::endl;
		return;
	}
	AddFile(fileData, zipFilename);
#else
	if(!mz_zip_writer_add_file(&_zipArchive, zipFilename.c_str(), filepath.c_str(), "", 0, MZ_BEST_COMPRESSION)) {
		std::cout << "mz_zip_writer_add_file() failed!" << std::endl;
	}
#endif
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
