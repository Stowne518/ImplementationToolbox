#include "UserSettings.h"
#include "AppLog.h"

void UserSettings::saveSettings(const std::string& filename, AppLog& log) const
{
	std::ofstream file(filename);
	if (file.is_open())
	{
		log.AddLog("[INFO] Saving new window settings to file: C:\\ImplementationToolbox\\%s\n", filename.c_str());
		file << "DarkMode=" << DarkMode << "\n";
		file << "WindowWidth=" << windowWidth << "\n";
		file << "WindowHeight=" << windowHeight << "\n";
		file << "WindowPosX=" << windowPosX << "\n";
		file << "WindowPosY=" << windowPosY << "\n";
		file << "GettingStarted=" << gettingStarted << "\n";
		file << "HealthCheck=" << healthCheck << "\n";
		file << "RecentUpdates=" << recentUpdates << "\n";
		file << "DebugLog=" << debugLog << "\n";
		file << "Modules=" << modules << "\n";
		file.close();
	}
	else
	{
		log.AddLog("[ERROR] Error opening file for saving settings: %s\n", filename);
	}
}