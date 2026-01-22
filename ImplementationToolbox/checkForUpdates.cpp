#include "checkForUpdates.h"

bool checkForUpdates(const std::filesystem::path& source, const std::filesystem::path& destination, AppLog& log)
{
	// Assume we've already verified VPN connectivity before we get here.
	auto sourceTime = std::filesystem::last_write_time(source);
	auto destinationTime = std::filesystem::last_write_time(destination);
	std::string s_path = source.string();
	auto pos = s_path.find_last_of('\\') + 2;
	std::string s_path_name = s_path.substr(pos, s_path.size());
	std::string d_path = destination.string() + '\\' + s_path_name;
	if (sourceTime > destinationTime)
	{
		log.AddLog("[INFO] New update detected, updating local interface files at %s\n", destination.c_str());
		try 
		{
			std::filesystem::copy(source, destination, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
			log.AddLog("[INFO] Interface successfully auto-updated to: %s", destination.c_str());
			return true;
		}
		catch (std::filesystem::filesystem_error e)
		{
			log.AddLog("[ERROR] Filesystem error when attempting update: %s\n", e.what());
			return false;
		}
	}
	else if (!std::filesystem::exists(d_path)) // If local file isn't present at the time we check for auto-updates then we just copy it
	{
		try
		{
			std::filesystem::copy(source, destination, std::filesystem::copy_options::recursive);
			log.AddLog("[INFO] Interface successfully copied to: %s\n", destination.c_str());
			return true;
		}
		catch (std::filesystem::filesystem_error e)
		{
			log.AddLog("[ERROR] Filesystem error when attempting update: %s\n", e.what());
			return false;
		}
	}

	return false;
}