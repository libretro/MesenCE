#include "pch.h"
#include <string>
#include <cstring>
#include <sstream>
#include "ZipWriter.h"
#include "FolderUtilities.h"

ZipWriter::ZipWriter()
{
}

ZipWriter::~ZipWriter()
{
}

size_t ZipWriter::WriteCallback(void* opaque, mz_uint64 fileOffset, const void* buffer, size_t size)
{
	ZipWriter* writer = (ZipWriter*)opaque;
	ofstream& file = writer->_zipFile;

	if(!file.good()) {
		return 0;
	}

	if(fileOffset != writer->_writePosition) {
		//miniz appends, so this is not expected to happen - handle it anyway
		file.seekp((std::streamoff)fileOffset, std::ios::beg);
		writer->_writePosition = fileOffset;
	}

	file.write((const char*)buffer, size);
	if(!file.good()) {
		return 0;
	}

	writer->_writePosition += size;
	return size;
}

bool ZipWriter::Initialize(string filename)
{
	_zipFilename = filename;
	memset(&_zipArchive, 0, sizeof(mz_zip_archive));

	//miniz's own file API goes straight to stdio - writing through an
	//ofstream keeps the path handling identical to the rest of the codebase
	_zipFile.open(filename, std::ios::out | std::ios::binary);
	if(!_zipFile) {
		return false;
	}
	_writePosition = 0;

	_zipArchive.m_pWrite = ZipWriter::WriteCallback;
	_zipArchive.m_pIO_opaque = this;
	return mz_zip_writer_init(&_zipArchive, 0) != 0;
}

bool ZipWriter::Save()
{
	bool result = mz_zip_writer_finalize_archive(&_zipArchive) != 0;
	result &= mz_zip_writer_end(&_zipArchive) != 0;

	if(_zipFile.is_open()) {
		_zipFile.close();
		result &= _zipFile.good();
	}

	return result;
}

void ZipWriter::AddFile(string filepath, string zipFilename)
{
	//Same reason as above: read through an ifstream rather than miniz's stdio helper
	ifstream file(filepath, std::ios::in | std::ios::binary);
	if(!file) {
		std::cout << "mz_zip_writer_add_file() failed!" << std::endl;
		return;
	}

	std::stringstream ss;
	ss << file.rdbuf();
	AddFile(ss, zipFilename);
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
