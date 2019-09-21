#pragma once
#define DLL_EXPORT __declspec(dllexport)

extern "C" {
	DLL_EXPORT bool __stdcall OpenWaveOutDevice(int _samplingRate, int _channel, int _blockSize, int _bufferMultiplicity);
	DLL_EXPORT bool __stdcall LoadPlugin(const char* _vstPath);
}


