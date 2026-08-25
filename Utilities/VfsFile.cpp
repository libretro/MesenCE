#include "pch.h"
#include "Utilities/VfsFile.h"
#include "Utilities/UTF8Util.h"

#include <algorithm>
#include <cstring>

#ifdef LIBRETRO
	#include "Libretro/libretro.h"
#endif

#ifdef _MSC_VER
	#define VfsFseek _fseeki64
	#define VfsFtell _ftelli64
#else
	#define VfsFseek fseeko
	#define VfsFtell ftello
#endif

namespace VfsIo
{
	//Fallbacks used when no frontend VFS is available
	static std::FILE* StdioOpen(const std::string& path, const char* mode)
	{
#ifdef _WIN32
		std::wstring wideMode(mode, mode + strlen(mode));
		return _wfopen(utf8::utf8::decode(path).c_str(), wideMode.c_str());
#else
		return std::fopen(path.c_str(), mode);
#endif
	}

	static bool ProbeExists(const std::string& path)
	{
		std::FILE* file = StdioOpen(path, "rb");
		if(file) {
			std::fclose(file);
			return true;
		}
		return false;
	}

	static int64_t ProbeSize(const std::string& path)
	{
		std::FILE* file = StdioOpen(path, "rb");
		if(!file) {
			return -1;
		}
		VfsFseek(file, 0, SEEK_END);
		int64_t size = (int64_t)VfsFtell(file);
		std::fclose(file);
		return size;
	}

	static bool StdioRemove(const std::string& path)
	{
#ifdef _WIN32
		return _wremove(utf8::utf8::decode(path).c_str()) == 0;
#else
		return std::remove(path.c_str()) == 0;
#endif
	}

	static bool StdioRename(const std::string& oldPath, const std::string& newPath)
	{
#ifdef _WIN32
		return _wrename(utf8::utf8::decode(oldPath).c_str(), utf8::utf8::decode(newPath).c_str()) == 0;
#else
		return std::rename(oldPath.c_str(), newPath.c_str()) == 0;
#endif
	}

#ifdef LIBRETRO
	static retro_vfs_interface* _iface = nullptr;
	static uint32_t _version = 0;

	void SetInterface(retro_vfs_interface* iface, uint32_t version)
	{
		_iface = iface;
		_version = iface ? version : 0;
	}

	bool IsAvailable()
	{
		return _iface != nullptr;
	}

	bool SupportsFolderOps()
	{
		return _iface != nullptr && _version >= 3 && _iface->stat && _iface->mkdir && _iface->opendir;
	}

	bool Exists(const std::string& path)
	{
		if(!_iface) {
			return ProbeExists(path);
		}

		if(_version >= 3 && _iface->stat) {
			return (_iface->stat(path.c_str(), nullptr) & RETRO_VFS_STAT_IS_VALID) != 0;
		}

		//No stat available - probe by opening the file
		retro_vfs_file_handle* handle = _iface->open(path.c_str(), RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);
		if(handle) {
			_iface->close(handle);
			return true;
		}
		return false;
	}

	bool IsFolder(const std::string& path)
	{
		if(!SupportsFolderOps()) {
			return false;
		}
		int32_t flags = _iface->stat(path.c_str(), nullptr);
		return (flags & RETRO_VFS_STAT_IS_VALID) && (flags & RETRO_VFS_STAT_IS_DIRECTORY);
	}

	int64_t GetSize(const std::string& path)
	{
		if(!_iface) {
			return ProbeSize(path);
		}

		if(_version >= 3 && _iface->stat) {
			int32_t size = 0;
			if(_iface->stat(path.c_str(), &size) & RETRO_VFS_STAT_IS_VALID) {
				return size;
			}
			return -1;
		}

		retro_vfs_file_handle* handle = _iface->open(path.c_str(), RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);
		if(!handle) {
			return -1;
		}
		int64_t size = _iface->size(handle);
		_iface->close(handle);
		return size;
	}

	bool CreateFolder(const std::string& path)
	{
		if(!SupportsFolderOps()) {
			return false;
		}
		int result = _iface->mkdir(path.c_str());
		//0 = created, -2 = already exists
		return result == 0 || result == -2;
	}

	bool Remove(const std::string& path)
	{
		if(_iface && _iface->remove) {
			return _iface->remove(path.c_str()) == 0;
		}
		return StdioRemove(path);
	}

	bool Rename(const std::string& oldPath, const std::string& newPath)
	{
		if(_iface && _iface->rename) {
			return _iface->rename(oldPath.c_str(), newPath.c_str()) == 0;
		}
		return StdioRename(oldPath, newPath);
	}

	bool ReadFolder(const std::string& path, std::vector<DirEntry>& entries)
	{
		if(!SupportsFolderOps() || !_iface->readdir || !_iface->dirent_get_name) {
			return false;
		}

		retro_vfs_dir_handle* dir = _iface->opendir(path.c_str(), false);
		if(!dir) {
			return false;
		}

		while(_iface->readdir(dir)) {
			const char* name = _iface->dirent_get_name(dir);
			if(!name || !name[0] || !strcmp(name, ".") || !strcmp(name, "..")) {
				continue;
			}
			entries.push_back({ std::string(name), _iface->dirent_is_dir && _iface->dirent_is_dir(dir) });
		}

		_iface->closedir(dir);
		return true;
	}
#else
	void SetInterface(retro_vfs_interface*, uint32_t) {}
	bool IsAvailable() { return false; }
	bool SupportsFolderOps() { return false; }
	bool IsFolder(const std::string&) { return false; }
	bool CreateFolder(const std::string&) { return false; }
	bool ReadFolder(const std::string&, std::vector<DirEntry>&) { return false; }
	bool Exists(const std::string& path) { return ProbeExists(path); }
	int64_t GetSize(const std::string& path) { return ProbeSize(path); }
	bool Remove(const std::string& path) { return StdioRemove(path); }
	bool Rename(const std::string& oldPath, const std::string& newPath) { return StdioRename(oldPath, newPath); }
#endif
}

VfsStreamBuf::VfsStreamBuf()
{
}

VfsStreamBuf::~VfsStreamBuf()
{
	close();
}

bool VfsStreamBuf::open(const std::string& path, std::ios_base::openmode mode)
{
	if(is_open()) {
		return false;
	}

	bool read = (mode & std::ios_base::in) != 0;
	bool write = (mode & std::ios_base::out) != 0 || (mode & std::ios_base::app) != 0;
	if(!read && !write) {
		return false;
	}

	bool append = (mode & std::ios_base::app) != 0;
	//Same defaults as std::filebuf: writing without reading truncates unless appending
	bool truncate = !append && write && (!read || (mode & std::ios_base::trunc) != 0);

#ifdef LIBRETRO
	if(VfsIo::IsAvailable()) {
		unsigned access = 0;
		if(read) {
			access |= RETRO_VFS_FILE_ACCESS_READ;
		}
		if(write) {
			access |= RETRO_VFS_FILE_ACCESS_WRITE;
		}
		if(!truncate && write) {
			access |= RETRO_VFS_FILE_ACCESS_UPDATE_EXISTING;
		}

		_handle = VfsIo::_iface->open(path.c_str(), access, RETRO_VFS_FILE_ACCESS_HINT_NONE);
		if(!_handle && write && !truncate) {
			//UPDATE_EXISTING fails when the file doesn't exist yet - create it
			_handle = VfsIo::_iface->open(path.c_str(), access & ~RETRO_VFS_FILE_ACCESS_UPDATE_EXISTING, RETRO_VFS_FILE_ACCESS_HINT_NONE);
		}
		if(!_handle) {
			return false;
		}
	}
#endif

	if(!_handle) {
		const char* fileMode;
		if(append) {
			fileMode = read ? "a+b" : "ab";
		} else if(!write) {
			fileMode = "rb";
		} else if(truncate) {
			fileMode = read ? "w+b" : "wb";
		} else {
			fileMode = "r+b";
		}

#ifdef _WIN32
		std::wstring wideMode(fileMode, fileMode + strlen(fileMode));
		_file = _wfopen(utf8::utf8::decode(path).c_str(), wideMode.c_str());
#else
		_file = std::fopen(path.c_str(), fileMode);
#endif
		if(!_file) {
			return false;
		}
	}

	_readable = read;
	_writable = write;

	if(append || (mode & std::ios_base::ate) != 0) {
		if(SeekRaw(0, SEEK_END) < 0) {
			close();
			return false;
		}
	}

	return true;
}

bool VfsStreamBuf::close()
{
	bool success = is_open();
	if(success) {
		success = FlushWriteBuffer();
	}

#ifdef LIBRETRO
	if(_handle) {
		if(VfsIo::_iface->close((retro_vfs_file_handle*)_handle) != 0) {
			success = false;
		}
		_handle = nullptr;
	}
#endif

	if(_file) {
		if(std::fclose(_file) != 0) {
			success = false;
		}
		_file = nullptr;
	}

	delete[] _readBuffer;
	_readBuffer = nullptr;
	delete[] _writeBuffer;
	_writeBuffer = nullptr;
	setg(nullptr, nullptr, nullptr);
	setp(nullptr, nullptr);

	_readable = false;
	_writable = false;

	return success;
}

//Both helpers loop until the request is satisfied - short reads/writes are
//allowed by the VFS contract, but the stream API expects all-or-nothing
int64_t VfsStreamBuf::ReadRaw(char* data, int64_t count)
{
	int64_t total = 0;
	while(total < count) {
		int64_t read = -1;
#ifdef LIBRETRO
		if(_handle) {
			read = VfsIo::_iface->read((retro_vfs_file_handle*)_handle, data + total, (uint64_t)(count - total));
		} else
#endif
		if(_file) {
			read = (int64_t)std::fread(data + total, 1, (size_t)(count - total), _file);
		}

		if(read <= 0) {
			return total > 0 ? total : read;
		}
		total += read;
	}
	return total;
}

int64_t VfsStreamBuf::WriteRaw(const char* data, int64_t count)
{
	int64_t total = 0;
	while(total < count) {
		int64_t written = -1;
#ifdef LIBRETRO
		if(_handle) {
			written = VfsIo::_iface->write((retro_vfs_file_handle*)_handle, data + total, (uint64_t)(count - total));
		} else
#endif
		if(_file) {
			written = (int64_t)std::fwrite(data + total, 1, (size_t)(count - total), _file);
		}

		if(written <= 0) {
			return total > 0 ? total : written;
		}
		total += written;
	}
	return total;
}

int64_t VfsStreamBuf::SeekRaw(int64_t offset, int whence)
{
#ifdef LIBRETRO
	if(_handle) {
		int vfsWhence;
		switch(whence) {
			default:
			case SEEK_SET: vfsWhence = RETRO_VFS_SEEK_POSITION_START; break;
			case SEEK_CUR: vfsWhence = RETRO_VFS_SEEK_POSITION_CURRENT; break;
			case SEEK_END: vfsWhence = RETRO_VFS_SEEK_POSITION_END; break;
		}

		//RetroArch returns 0 (not the new offset) on success, so the position
		//always has to be read back with tell()
		if(VfsIo::_iface->seek((retro_vfs_file_handle*)_handle, offset, vfsWhence) < 0) {
			return -1;
		}
		return VfsIo::_iface->tell((retro_vfs_file_handle*)_handle);
	}
#endif
	if(_file) {
		if(VfsFseek(_file, offset, whence) != 0) {
			return -1;
		}
		return (int64_t)VfsFtell(_file);
	}
	return -1;
}

int64_t VfsStreamBuf::TellRaw()
{
#ifdef LIBRETRO
	if(_handle) {
		return VfsIo::_iface->tell((retro_vfs_file_handle*)_handle);
	}
#endif
	if(_file) {
		return (int64_t)VfsFtell(_file);
	}
	return -1;
}

bool VfsStreamBuf::FlushWriteBuffer()
{
	if(!_writeBuffer || pptr() == pbase()) {
		return true;
	}

	int64_t count = pptr() - pbase();
	int64_t written = WriteRaw(pbase(), count);
	setp(_writeBuffer, _writeBuffer + VfsStreamBuf::BufferSize);
	return written == count;
}

VfsStreamBuf::int_type VfsStreamBuf::underflow()
{
	if(!_readable || !is_open()) {
		return traits_type::eof();
	}

	if(gptr() && gptr() < egptr()) {
		return traits_type::to_int_type(*gptr());
	}

	if(!FlushWriteBuffer()) {
		return traits_type::eof();
	}

	if(!_readBuffer) {
		_readBuffer = new char[VfsStreamBuf::BufferSize];
	}

	int64_t count = ReadRaw(_readBuffer, VfsStreamBuf::BufferSize);
	if(count <= 0) {
		setg(_readBuffer, _readBuffer, _readBuffer);
		return traits_type::eof();
	}

	setg(_readBuffer, _readBuffer, _readBuffer + count);
	return traits_type::to_int_type(*gptr());
}

VfsStreamBuf::int_type VfsStreamBuf::overflow(int_type ch)
{
	if(!_writable || !is_open()) {
		return traits_type::eof();
	}

	if(!_writeBuffer) {
		_writeBuffer = new char[VfsStreamBuf::BufferSize];
		setp(_writeBuffer, _writeBuffer + VfsStreamBuf::BufferSize);
	} else if(!FlushWriteBuffer()) {
		return traits_type::eof();
	}

	if(!traits_type::eq_int_type(ch, traits_type::eof())) {
		*pptr() = traits_type::to_char_type(ch);
		pbump(1);
	}

	return traits_type::not_eof(ch);
}

int VfsStreamBuf::sync()
{
	if(!FlushWriteBuffer()) {
		return -1;
	}

#ifdef LIBRETRO
	if(_handle) {
		return VfsIo::_iface->flush ? VfsIo::_iface->flush((retro_vfs_file_handle*)_handle) : 0;
	}
#endif
	if(_file && _writable) {
		return std::fflush(_file) == 0 ? 0 : -1;
	}
	return 0;
}

std::streamsize VfsStreamBuf::xsgetn(char* data, std::streamsize count)
{
	if(!_readable || !is_open() || count <= 0) {
		return 0;
	}

	std::streamsize total = 0;

	//Consume whatever is already buffered
	if(gptr() && gptr() < egptr()) {
		std::streamsize available = std::min<std::streamsize>(egptr() - gptr(), count);
		memcpy(data, gptr(), (size_t)available);
		gbump((int)available);
		total += available;
	}

	std::streamsize remaining = count - total;
	if(remaining <= 0) {
		return total;
	}

	if(remaining >= VfsStreamBuf::BufferSize) {
		//Large reads (ROMs, save states, ...) bypass the buffer entirely
		if(!FlushWriteBuffer()) {
			return total;
		}
		int64_t read = ReadRaw(data + total, remaining);
		return total + (read > 0 ? read : 0);
	}

	return total + std::streambuf::xsgetn(data + total, remaining);
}

std::streamsize VfsStreamBuf::xsputn(const char* data, std::streamsize count)
{
	if(!_writable || !is_open() || count <= 0) {
		return 0;
	}

	if(count >= VfsStreamBuf::BufferSize) {
		if(!FlushWriteBuffer()) {
			return 0;
		}
		int64_t written = WriteRaw(data, count);
		return written > 0 ? written : 0;
	}

	return std::streambuf::xsputn(data, count);
}

VfsStreamBuf::pos_type VfsStreamBuf::seekoff(off_type offset, std::ios_base::seekdir dir, std::ios_base::openmode which)
{
	if(!is_open()) {
		return pos_type(off_type(-1));
	}

	if(!FlushWriteBuffer()) {
		return pos_type(off_type(-1));
	}

	int whence;
	int64_t target = offset;
	if(dir == std::ios_base::cur) {
		//The buffered (but not yet consumed) bytes have to be accounted for
		int64_t current = TellRaw();
		if(current < 0) {
			return pos_type(off_type(-1));
		}
		if(gptr() && gptr() < egptr()) {
			current -= (int64_t)(egptr() - gptr());
		}
		target = current + offset;
		whence = SEEK_SET;
	} else if(dir == std::ios_base::end) {
		whence = SEEK_END;
	} else {
		whence = SEEK_SET;
	}

	setg(nullptr, nullptr, nullptr);

	int64_t position = SeekRaw(target, whence);
	if(position < 0) {
		return pos_type(off_type(-1));
	}
	return pos_type(off_type(position));
}

VfsStreamBuf::pos_type VfsStreamBuf::seekpos(pos_type pos, std::ios_base::openmode which)
{
	return seekoff(off_type(pos), std::ios_base::beg, which);
}
