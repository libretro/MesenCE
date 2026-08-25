#pragma once

#ifdef LIBRETRO

// Frontend VFS support for the libretro build.
//
// RetroArch (and other frontends) can hand the core paths that are not real
// filesystem paths at all - the typical case is an Android SAF `content://`
// URI, but the same applies to paths inside frontend-managed archives. Those
// can only be opened through the `retro_vfs_interface` obtained via
// RETRO_ENVIRONMENT_GET_VFS_INTERFACE, so every file access made by the core
// has to go through this layer.
//
// The core reads/writes files exclusively through `utf8::ifstream` and
// `utf8::ofstream` (see UTF8Util.h), so routing those two through
// `VfsStreamBuf` covers ROMs, patches, firmware, battery saves, save states,
// screenshots and recordings. Directory handling (FolderUtilities) uses the
// v3 stat/mkdir/opendir calls when the frontend provides them.
//
// When no VFS interface is available (standalone builds, or a frontend that
// declines the handshake) everything transparently falls back to stdio.

#include <cstdint>
#include <cstdio>
#include <ios>
#include <streambuf>
#include <string>
#include <vector>

struct retro_vfs_interface;

namespace VfsIo
{
	struct DirEntry
	{
		std::string Name;
		bool IsFolder;
	};

	//Called by the libretro glue once RETRO_ENVIRONMENT_GET_VFS_INTERFACE succeeded
	void SetInterface(retro_vfs_interface* iface, uint32_t version);

	//True when file I/O must go through the frontend
	bool IsAvailable();

	//True when the frontend also provides the v3 stat/mkdir/opendir calls
	bool SupportsFolderOps();

	bool Exists(const std::string& path);
	bool IsFolder(const std::string& path);
	int64_t GetSize(const std::string& path);
	bool CreateFolder(const std::string& path);
	bool Remove(const std::string& path);
	bool Rename(const std::string& oldPath, const std::string& newPath);
	bool ReadFolder(const std::string& path, std::vector<DirEntry>& entries);
}

//streambuf backed by the frontend VFS, with a stdio fallback
class VfsStreamBuf : public std::streambuf
{
public:
	VfsStreamBuf();
	VfsStreamBuf(const VfsStreamBuf&) = delete;
	VfsStreamBuf& operator=(const VfsStreamBuf&) = delete;
	~VfsStreamBuf() override;

	bool open(const std::string& path, std::ios_base::openmode mode);
	bool is_open() const { return _handle != nullptr || _file != nullptr; }
	bool close();

protected:
	int_type underflow() override;
	int_type overflow(int_type ch) override;
	int sync() override;
	std::streamsize xsgetn(char* data, std::streamsize count) override;
	std::streamsize xsputn(const char* data, std::streamsize count) override;
	pos_type seekoff(off_type offset, std::ios_base::seekdir dir, std::ios_base::openmode which) override;
	pos_type seekpos(pos_type pos, std::ios_base::openmode which) override;

private:
	static constexpr int BufferSize = 8192;

	int64_t ReadRaw(char* data, int64_t count);
	int64_t TellRaw();
	int64_t WriteRaw(const char* data, int64_t count);
	int64_t SeekRaw(int64_t offset, int whence);
	bool FlushWriteBuffer();

	void* _handle = nullptr; //retro_vfs_file_handle*
	std::FILE* _file = nullptr;
	bool _readable = false;
	bool _writable = false;
	char* _readBuffer = nullptr;
	char* _writeBuffer = nullptr;
};

#endif // LIBRETRO
