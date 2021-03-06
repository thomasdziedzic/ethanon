/*--------------------------------------------------------------------------------------
 Ethanon Engine (C) Copyright 2008-2012 Andre Santee
 http://www.asantee.net/ethanon/

	Permission is hereby granted, free of charge, to any person obtaining a copy of this
	software and associated documentation files (the "Software"), to deal in the
	Software without restriction, including without limitation the rights to use, copy,
	modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so, subject to the
	following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
	INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
	PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
	OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
--------------------------------------------------------------------------------------*/

#include "FileLogger.h"
#include "Platform.h"

#include <fstream>
#include <iostream>

#ifdef ANDROID
#include "android/AndroidLog.h"
#endif

namespace Platform {

bool AppendToFile(const gs2d::str_type::string& fileName, const gs2d::str_type::string& str)
{
	gs2d::str_type::ofstream ofs(fileName.c_str(), std::ios::out | std::ios::app);
	if (ofs.is_open())
	{
		ofs << str << std::endl;
		ofs.close();
		return true;
	}
	else
	{
		return false;
	}
}

FileLogger::FileLogger(const gs2d::str_type::string& fileName) : m_fileName(fileName)
{
	gs2d::str_type::ofstream ofs(fileName.c_str());
	if (ofs.is_open())
	{
		ofs << GS_L("[") << fileName << GS_L("]") << std::endl;
		ofs.close();
	}
}

void FileLogger::WriteToErrorLog(const gs2d::str_type::string& str)
{
	static bool errorLogFileCreated = false;
	if (!errorLogFileCreated)
	{
		gs2d::str_type::ofstream ofs(GetErrorLogFilePath().c_str());
		if (ofs.is_open())
		{
			ofs << GS_L("[") << GetErrorLogFilePath() << GS_L("]") << std::endl;
			ofs.close();
		}
	}
	AppendToFile(GetErrorLogFilePath(), str);
	errorLogFileCreated = true;
}

void FileLogger::WriteToWarningLog(const gs2d::str_type::string& str)
{
	static bool warningLogFileCreated = false;
	if (!warningLogFileCreated)
	{
		gs2d::str_type::ofstream ofs(GetWarningLogFilePath().c_str());
		if (ofs.is_open())
		{
			ofs << GS_L("[") << GetWarningLogFilePath() << GS_L("]") << std::endl;
			ofs.close();
		}
	}
	AppendToFile(GetWarningLogFilePath(), str);
	warningLogFileCreated = true;
}

bool FileLogger::Log(const gs2d::str_type::string& str, const TYPE& type) const
{
	#if defined(WIN32) || defined(APPLE_IOS)
	GS2D_COUT << str << std::endl;
	if (type == ERROR)
	{
		GS2D_CERR << GS_L("\x07");
	}
	#endif
	if (type == ERROR)
		WriteToErrorLog(str);
	else if (type == WARNING)
		WriteToWarningLog(str);

	#ifdef ANDROID
		switch (type)
		{
		case ERROR:
		case WARNING:
			LOGE(str.c_str());
			break;
		default:
			LOGI(str.c_str());
		}
	#endif
	return AppendToFile(m_fileName, str);
}

gs2d::str_type::string FileLogger::GetErrorLogFilePath()
{
	return GetLogPath() + GS_L("_error.log.txt");
}

gs2d::str_type::string FileLogger::GetWarningLogFilePath()
{
	return GetLogPath() + GS_L("_warning.log.txt");
}

} // namespace Platform