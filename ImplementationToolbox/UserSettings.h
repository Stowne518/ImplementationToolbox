#pragma once
#include <fstream>
#include <string>

struct AppLog;

struct UserSettings
{
private:
	int DarkMode = 0;
	float windowWidth = 1280;
	float windowHeight = 720;
	float windowPosX = 0;
	float windowPosY = 0;
	std::string filename = "ImplementationToolbox.ini";
	char gettingStarted;
	char healthCheck;
	char recentUpdates;
	char debugLog;
	char modules;
public:
	int getDarkMode() const { return DarkMode; }
	void setDarkMode(int mode) { DarkMode = mode; }

	float getWindowWidth() const { return windowWidth; }
	void setWindowWidth(float width) { windowWidth = width; }

	float getWindowHeight() const { return windowHeight; }
	void setWindowHeight(float height) { windowHeight = height; }

	float getWindowPosX() const { return windowPosX; }
	void setWindowPosX(float posX) { windowPosX = posX; }

	float getWindowPosY() const { return windowPosY; }
	void setWindowPosY(float posY) { windowPosY = posY; }

	bool getModules() const
	{
		if (modules == 'Y')
			return true;
		else
			return false;
	}

	// Getters for debug log
	bool getDebugLog() const
	{
		if (debugLog == 'Y')
			return true;
		else
			return false;
	}

	// Getters for other settings
	bool getGettingStarted() const 
	{ 
		if (gettingStarted == 'Y')
			return true;
		else
			return false;
	}
	bool getHealthCheck() const 
	{ 
		if (healthCheck == 'Y')
			return true;
		else
			return false;
	}
	bool getRecentUpdates() const 
	{ 
		if (recentUpdates == 'Y')
			return true;
		else
			return false;
	}
	// Setters for other settings
	void setGettingStarted(char value) { gettingStarted = value; }
	void setHealthCheck(char value) { healthCheck = value; }
	void setRecentUpdates(char value) {	recentUpdates = value; }
	void setDebugLog(char value) { debugLog = value; }
	void setModules(char value) { modules = value; }

	// Save settings to file
	void saveSettings(const std::string& filename, AppLog&) const;

	// Load settings from file
	void loadSettings(const std::string& filename)
	{
		std::ifstream file(filename);
		if (!file.is_open())
		{
			// Build default settings file for first time launch
			std::ofstream defaultFile(filename);
			if (defaultFile.is_open())
			{
				defaultFile << "DarkMode=0\n";			// Default to light mode
				defaultFile << "WindowWidth=1280\n";	// Default width
				defaultFile << "WindowHeight=720\n";	// Default height
				defaultFile << "WindowPosX=0\n";		// Default position X
				defaultFile << "WindowPosY=0\n";		// Default position Y
				defaultFile << "GettingStarted=Y\n";	// Default getting started setting
				defaultFile << "HealthCheck=Y\n";		// Default health check setting
				defaultFile << "RecentUpdates=Y\n";		// Default recent updates setting
				defaultFile << "DebugLog=N\n";			// Default debug log setting
				defaultFile << "Modules=Y\n";				// Default modules setting
				defaultFile.close();
			}
			return;
		}
		else
		{
			while (file)
			{
				std::string line;
				if (std::getline(file, line))
				{
					setDarkMode(std::stoi(line.substr(line.find('=') + 1)));		// Load and set darkmode setting 0 == light, 1 == dark
				}
				if (std::getline(file, line))
				{
					setWindowWidth(std::stof(line.substr(line.find('=') + 1)));		// Load and set window width
				}
				if (std::getline(file, line))
				{
					setWindowHeight(std::stof(line.substr(line.find('=') + 1)));	// Load and set window height
				}
				if (std::getline(file, line))
				{
					setWindowPosX(std::stof(line.substr(line.find('=') + 1)));		// Load and set window position X
				}
				if (std::getline(file, line))
				{
					setWindowPosY(std::stof(line.substr(line.find('=') + 1)));		// Load and set window position Y
				}
				if (std::getline(file, line))
				{
					setGettingStarted(line.substr(line.find('=') + 1)[0]);			// Load and set getting started setting
				}
				if (std::getline(file, line))
				{
					setHealthCheck(line.substr(line.find('=') + 1)[0]);				// Load and set health check setting
				}
				if (std::getline(file, line))
				{
					setRecentUpdates(line.substr(line.find('=') + 1)[0]);			// Load and set recent updates setting
				}
				if (std::getline(file, line))
				{
					setDebugLog(line.substr(line.find('=') + 1)[0]);				// Load and set debug log setting
				}
				if (std::getline(file, line))
				{
					setModules(line.substr(line.find('=') + 1)[0]);					// Load and set module setting
				}
			}
		}
	}
};

