#pragma once
#include "pch.h"
#include "miniz.h"

class ZipWriter
{
private:
	mz_zip_archive _zipArchive;
	string _zipFilename;
	ofstream _zipFile;
	mz_uint64 _writePosition = 0;

	static size_t WriteCallback(void* opaque, mz_uint64 fileOffset, const void* buffer, size_t size);

public:
	ZipWriter();
	~ZipWriter();

	bool Initialize(string filename);
	bool Save();

	void AddFile(string filepath, string zipFilename);
	void AddFile(vector<uint8_t>& fileData, string zipFilename);
	void AddFile(std::stringstream& filestream, string zipFilename);
};