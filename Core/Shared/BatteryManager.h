#pragma once
#include "pch.h"

class IBatteryProvider
{
public:
	virtual vector<uint8_t> LoadBattery(string extension) = 0;
};

class IBatteryRecorder
{
public:
	virtual void OnLoadBattery(string extension, vector<uint8_t> batteryData) = 0;
};

class BatteryManager
{
private:
	string _romName;
	bool _hasBattery = false;
#ifdef LIBRETRO
	using LibretroBatteryHandler = bool (*)(const string&, uint8_t*, uint32_t, bool);
	LibretroBatteryHandler _libretroBatteryHandler = nullptr;
#endif

	std::weak_ptr<IBatteryProvider> _provider;
	std::weak_ptr<IBatteryRecorder> _recorder;

	string GetBasePath(string& extension);

public:
	void Initialize(string romName, bool setBatteryFlag = false);

	bool HasBattery() { return _hasBattery; }

	void SetBatteryProvider(shared_ptr<IBatteryProvider> provider);
	void SetBatteryRecorder(shared_ptr<IBatteryRecorder> recorder);
#ifdef LIBRETRO
	void SetLibretroBatteryHandler(LibretroBatteryHandler handler) { _libretroBatteryHandler = handler; }
#endif

	void SaveBattery(string extension, uint8_t* data, uint32_t length);

	vector<uint8_t> LoadBattery(string extension);
	void LoadBattery(string extension, uint8_t* data, uint32_t length);
	uint32_t GetBatteryFileSize(string extension);
};