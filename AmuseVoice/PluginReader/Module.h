#pragma once
#include <Windows.h>
#include <string>

class Module {
private:
	HMODULE module_;
	std::string path_;

public:
	Module() {}
	Module(LPCSTR _filePath) {
		module_ = LoadLibrary(_filePath);
		path_ = _filePath;
	}

	bool Load(LPCSTR _filePath) {
		module_ = LoadLibrary(_filePath);
		path_ = _filePath;
	}

	HMODULE GetModule() { return module_; }
	LPCSTR GetPath() { return path_.c_str(); }

	template<typename _FuncType>
	_FuncType* GetFunction(LPCSTR _funcName) {
		return(_FuncType*)(GetProcAddress(module_, _funcName));
	}

	operator bool() { return module_ != 0; }
};
