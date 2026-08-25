#include "pch.h"

class WaveRecorder
{
private:
#ifdef LIBRETRO
	//utf8::ofstream is not movable in libretro builds (it owns its streambuf),
	//so the stream is opened in place - see the constructor
	ofstream _stream;
#else
	std::ofstream _stream;
#endif
	uint32_t _streamSize;
	uint32_t _sampleRate;
	bool _isStereo;
	string _outputFile;

	void WriteHeader();
	void UpdateSizeValues();
	void CloseFile();

public:
	WaveRecorder(string outputFile, uint32_t sampleRate, bool isStereo);
	~WaveRecorder();

	bool WriteSamples(int16_t* samples, uint32_t sampleCount, uint32_t sampleRate, bool isStereo);
};