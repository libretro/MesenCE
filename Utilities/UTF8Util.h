#pragma once

#include <fstream>

#ifdef LIBRETRO
	#include <istream>
	#include <ostream>
	#include <string>
	#include "Libretro/VfsFile.h"
#endif

namespace utf8
{
	class utf8
	{
	public:
		static std::wstring decode(const std::string& str);
		static std::string encode(const std::wstring& wstr);
		static std::string encode(const std::u16string& wstr);
	};

#ifdef LIBRETRO
	//In libretro builds all file access has to go through the frontend VFS when
	//one is available (Android SAF content:// URIs cannot be opened otherwise).
	//VfsStreamBuf falls back to stdio when the frontend provides no interface.
	class ifstream : public std::istream
	{
	public:
		//Out-of-line so this is the class's key function: std::istream inherits
		//virtually, so deriving from it produces a VTT and construction vtables
		//alongside the vtable. Left implicit, every translation unit that sees
		//this header emits its own copy into a .gnu.linkonce comdat, and the
		//mingw LTO link then rejects them as multiple definitions because the
		//group keys do not match between objects.
		~ifstream() override;

		ifstream() : std::istream(nullptr) { rdbuf(&_buf); }

		ifstream(const std::string& path, ios_base::openmode mode = ios_base::in) : std::istream(nullptr)
		{
			rdbuf(&_buf);
			open(path, mode);
		}

		void open(const std::string& path, ios_base::openmode mode = ios_base::in)
		{
			if(_buf.open(path, mode | ios_base::in)) {
				clear();
			} else {
				setstate(ios_base::failbit);
			}
		}

		bool is_open() const { return _buf.is_open(); }

		void close()
		{
			if(!_buf.close()) {
				setstate(ios_base::failbit);
			}
		}

	private:
		VfsStreamBuf _buf;
	};

	class ofstream : public std::ostream
	{
	public:
		//Key function, see utf8::ifstream above.
		~ofstream() override;

		ofstream() : std::ostream(nullptr) { rdbuf(&_buf); }

		ofstream(const std::string& path, ios_base::openmode mode = ios_base::out) : std::ostream(nullptr)
		{
			rdbuf(&_buf);
			open(path, mode);
		}

		void open(const std::string& path, ios_base::openmode mode = ios_base::out)
		{
			if(_buf.open(path, mode | ios_base::out)) {
				clear();
			} else {
				setstate(ios_base::failbit);
			}
		}

		bool is_open() const { return _buf.is_open(); }

		void close()
		{
			if(!_buf.close()) {
				setstate(ios_base::failbit);
			}
		}

	private:
		VfsStreamBuf _buf;
	};
#elif defined(_WIN32)
	class ifstream : public std::ifstream
	{
	public:
		ifstream(const std::string& _Str, ios_base::openmode _Mode = ios_base::in) : std::ifstream(utf8::decode(_Str), _Mode) {}
		ifstream() : std::ifstream() {}
		void open(const std::string& _Str, ios_base::openmode _Mode = ios_base::in) { std::ifstream::open(utf8::decode(_Str), _Mode); }
	};

	class ofstream : public std::ofstream
	{
	public:
		ofstream(const std::string& _Str, ios_base::openmode _Mode = ios_base::in) : std::ofstream(utf8::decode(_Str), _Mode) {}
		ofstream() : std::ofstream() {}
		void open(const std::string& _Str, ios_base::openmode _Mode = ios_base::in) { std::ofstream::open(utf8::decode(_Str), _Mode); }
	};
#else
	using std::ifstream;
	using std::ofstream;
#endif
}
