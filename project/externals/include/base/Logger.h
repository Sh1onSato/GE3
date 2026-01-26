#pragma once
#include<string>
#include<Windows.h>
#include <iostream>

namespace Logger{
	void Log(const std::string& message);

	void Log(std::ostream& os, const std::string& message);
};

