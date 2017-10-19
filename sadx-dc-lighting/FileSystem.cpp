#include "stdafx.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

#include "FileSystem.h"

bool filesystem::exists(const std::string& path)
{
	return PathFileExistsA(path.c_str()) != 0;
}

bool filesystem::is_directory(const std::string& path)
{
	return PathIsDirectoryA(path.c_str()) != 0;
}

bool filesystem::is_file(const std::string& path)
{
	return !is_directory(path);
}

bool filesystem::remove_all(const std::string& path)
{
	WIN32_FIND_DATAA find_data {};

	const std::string str_search(move(combine_path(path, "*.*")));
	const HANDLE find_handle = FindFirstFileA(str_search.c_str(), &find_data);

	if (find_handle == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	do
	{
		const std::string file_name = find_data.cFileName;
		if (file_name == "." || file_name == "..")
		{
			continue;
		}

		const std::string file_path(move(combine_path(path, find_data.cFileName)));

		if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			remove_all(file_path);
		}
		else
		{
			remove(file_path);
		}

	} while (FindNextFileA(find_handle, &find_data) == TRUE);

	FindClose(find_handle);
	return remove(path);
}

bool filesystem::remove(const std::string& path)
{
	if (!exists(path))
	{
		return false;
	}

	if (is_directory(path))
	{
		return !!RemoveDirectoryA(path.c_str());
	}

	return !!DeleteFileA(path.c_str());
}

std::string filesystem::get_directory(const std::string& path)
{
	const auto npos = path.npos;

	auto slash = path.find_last_of('\\');
	if (slash == npos)
	{
		slash = path.find_last_of('/');

		if (slash == npos)
		{
			return std::string();
		}
	}

	if (slash != path.size() - 1)
	{
		return path.substr(0, slash);
	}

	const auto last = slash;

	slash = path.find_last_of('\\', last);
	if (slash == npos)
	{
		slash = path.find_last_of('/', last);
	}

	if (slash == npos)
	{
		return std::string();
	}

	return path.substr(last);
}

bool filesystem::create_directory(const std::string& path)
{
	return !!CreateDirectoryA(path.c_str(), nullptr);
}

std::string filesystem::get_base_name(const std::string& path)
{
	const auto npos = path.npos;

	auto slash = path.find_last_of('\\');
	if (slash == npos)
	{
		slash = path.find_last_of('/');
		if (slash == npos)
		{
			return path;
		}
	}

	if (slash != path.size() - 1)
	{
		return path.substr(++slash);
	}

	const auto last = slash - 1;

	slash = path.find_last_of('\\', last);
	if (slash == npos)
	{
		slash = path.find_last_of('/', last);
	}

	if (!slash || slash == npos)
	{
		return std::string();
	}

	return path.substr(slash + 1, last - slash);
}

void filesystem::strip_extension(std::string& path)
{
	const auto dot = path.find('.');
	if (dot == path.npos)
	{
		return;
	}

	path = path.substr(0, dot);
}

std::string filesystem::get_extension(const std::string& path, bool include_dot)
{
	auto dot = path.find('.');
	if (dot == path.npos)
	{
		return std::string();
	}

	if (!include_dot)
	{
		++dot;
	}

	return path.substr(dot);
}

std::string filesystem::get_working_directory()
{
	const auto length = GetCurrentDirectoryA(0, nullptr);

	if (length < 1)
	{
		return std::string();
	}

	const auto buffer = new char[length];
	GetCurrentDirectoryA(length, buffer);
	std::string result(buffer);
	delete[] buffer;
	return result;
}

std::string filesystem::combine_path(const std::string& path_a, const std::string& path_b)
{
	char buffer[MAX_PATH] {};
	const LPSTR result = PathCombineA(buffer, path_a.c_str(), path_b.c_str());

	if (result == nullptr)
	{
		return std::string();
	}

	return std::string(result);
}
