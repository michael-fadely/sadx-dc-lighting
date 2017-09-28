#pragma once
#include <string>

namespace filesystem
{
	bool exists(const std::string& path);
	bool is_directory(const std::string& path);
	bool is_file(const std::string& path);

	inline bool directory_exists(const std::string& path)
	{
		return exists(path) && is_directory(path);
	}
	inline bool file_exists(const std::string& path)
	{
		return exists(path) && is_file(path);
	}

	std::string get_directory(const std::string& path);
	bool create_directory(const std::string& path);
	bool remove_all(const std::string& path);
	bool remove(const std::string& path);
	std::string get_base_name(const std::string& path);
	void strip_extension(std::string& path);
	std::string get_extension(const std::string& path, bool include_dot = false);
	std::string get_working_directory();
	std::string combine_path(const std::string& path_a, const std::string& path_b);
}
