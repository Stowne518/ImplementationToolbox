#pragma once
#include <fstream>
#include <string>
struct UserSettings
{
private:
	int DarkMode = 0;
	float windowWidth = 1280;
	float windowHeight = 720;
	float windowPosX = 0;
	float windowPosY = 0;
	std::string filename = "ImplementationToolbox.ini";
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

	// Save settings to file
	void saveSettings(const std::string& filename) const
	{
		std::ofstream file(filename);
		if (file.is_open())
		{
			file << "DarkMode=" << DarkMode << "\n";
			file << "WindowWidth=" << windowWidth << "\n";
			file << "WindowHeight=" << windowHeight << "\n";
			file << "WindowPosX=" << windowPosX << "\n";
			file << "WindowPosY=" << windowPosY << "\n";
			file.close();
		}
		else
		{
			std::cerr << "Error opening file for writing: " << filename << std::endl;
		}
	}

	// Load settings from file
	void loadSettings(const std::string& filename)
	{
		std::ifstream file(filename);
		if (!file.is_open())
		{
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
					setWindowWidth(std::stof(line.substr(line.find('=') + 1)));	// Load and set window width
				}
				if (std::getline(file, line))
				{
					setWindowHeight(std::stof(line.substr(line.find('=') + 1)));	// Load and set window height
				}
				if (std::getline(file, line))
				{
					setWindowPosX(std::stof(line.substr(line.find('=') + 1)));	// Load and set window position X
				}
				if (std::getline(file, line))
				{
					setWindowPosY(std::stof(line.substr(line.find('=') + 1)));	// Load and set window position Y
				}
			}
		}
	}
};

