#include "getInterfaceFiles.h"
#include "AppLog.h"
#include "systemFunctions.h"
#include "imgui.h"
#include "imgui_stdlib.h"


void getInterfaceFiles(bool* open, AppLog& log)
{
	// Steps of this program
	// 0. Get local interface specific directory from user
	// 1. Get the product line from user
	// 2. Build and return file path of the interfaces for that product line
	// 3. Iterate over the file path for all folders and store them in a container
	// 4. Display the list of interfaces in a table that allows multi-selection
	// 5. Confirm selections made on the table and lock-in choices
	// 6. Check if any directories do not exist and create them if not
	// 7. Copy the files from the source directory to the destination directory
	// 8. Inform the user that their files have been copied successfully or if there was an error
	// Potential problem: Need to pass windows credentials or use integrated security to access source directory

	// Variables
	const std::string ROOTSOURCEDIR = "\\\\yoda\\cleaninstall\\Update\\";		// Constant root path to interfaces directory on the network share

	bool vpnConnected = testDirectory(ROOTSOURCEDIR);	// Test VPN connection to the network share
	static std::filesystem::path destinationDir;			// Local directory to copy files to
	static std::string destinationDirString;	// String to hold the local directory path
	static std::string productName;				// Product line name to be used in the path
	std::filesystem::path pathToInterfaces = getFilePath(log, productName, ROOTSOURCEDIR);			// Created path to interfaces directory using rootSource and user provided product line
	static std::vector<std::string> listOfInterfaces;	// List of interfaces to display to user
	static std::vector<std::string> selectedInterfaces;	// List of selected interfaces to copy down

	// Functionality
	ImGui::Text("VPN Connection: %s", vpnConnected ? "Connected" : "Not Connected");		// Try to hit the network path on the fileshare to determine vpn access
	if (!vpnConnected)
		ImGui::BeginDisabled();
	// 0. Get directory information
	ImGui::SeparatorText("Directories");
	ImGui::Text("Local interfaces directory:"); ImGui::SameLine();
	ImGui::SetNextItemWidth(425);
	ImGui::InputText("##localinterfacedir", &destinationDirString);
	if(ImGui::IsItemDeactivatedAfterEdit()) // Check path after user edits it to see if we need to add an end slash and update the filesystem path
	{
		if (!destinationDirString.empty() && std::find(destinationDirString.rbegin(), destinationDirString.rend(), '\\') != destinationDirString.rbegin())	// if the last character in the path isn't a forward slash append one
			destinationDirString += '\\';
		destinationDir = destinationDirString;		// Convert string to filesystem path
	}
	if(!destinationDirString.empty())
		ImGui::SetItemTooltip("%s", destinationDirString.c_str());	// Show full path when hovering text box when it has text in it
	
	if (testDirectory(destinationDirString))		// Test that the directory exists
	{
		ImGui::SameLine();
		DisplayColoredText("Directory valid.", true);
	}
	else if (!destinationDirString.empty() && !testDirectory(destinationDirString))	// If the directory is not empty and does not exist
	{
		ImGui::SameLine();
		DisplayColoredText("Directory does not exist. Create it?", false);
		ImGui::SameLine();
		if (addButton("Create Directory"))
		{
			createLocalDirs(log, destinationDir);	// Create the directory if it does not exist
		}
	}
	ImGui::Text("Root interfaces directory: %s", ROOTSOURCEDIR.c_str());

	// 1. Get product to search for interfaces for
	productName = getProductLine(log);		// Get product line from user
	ImGui::Text("Product Line: %s", productName.c_str());

	// Display final information from what user selected and confirm the source path and destination path are correct
	ImGui::SeparatorText("Final Paths");
	ImGui::Text("Local interface directory: %s", destinationDir.string().c_str());
	ImGui::Text("Path to interfaces: %s", pathToInterfaces.string().c_str());
	ImGui::SameLine();
	if (addButton("Confirm"))
	{
		if (productName.empty())
		{
			log.AddLog("[ERROR] No product line selected. Please select a product line.\n");
		}
		else
		{
			// 2. Get list of all directories to interfaces for the product line
			listOfInterfaces = getInterfaceNames(log, pathToInterfaces);
		}
	}
	
	// For now create static text to display all folders in this directory
	if(listOfInterfaces.size() > 0)
	{
		ImGui::BeginChild("ListOfInterfaces", ImVec2(0, 200), ImGuiChildFlags_Borders);
		ImGui::Text("List of available interfaces: ");
		displayListOfInterfaces(log, listOfInterfaces, selectedInterfaces, pathToInterfaces);
		// End list of interfaces child window
		ImGui::EndChild();
		// 3. Show list of interfaces in the product folder
		ImGui::BeginChild("SelectedInterfaces", ImVec2(0,200), ImGuiChildFlags_Borders);
		ImGui::Text("Selected interfaces: ");
		for (int i = 0; i < selectedInterfaces.size(); i++)
		{
			ImGui::Text(selectedInterfaces[i].c_str());
		}
		// End selected interfaces child window
		ImGui::EndChild();

		if (addButton("Copy Inteface Files"))
		{
			if (vpnConnected && !destinationDirString.empty() && !productName.empty())
			{
				// 4. Check if local directories exist and create them if not
				if (checkLocalDirs(log, destinationDir))
				{
					log.AddLog("[INFO] Local directory exists: %s\n", destinationDir.string().c_str());
				}
				else
				{
					log.AddLog("[ERROR] Local directory does not exist: %s\n", destinationDir.string().c_str());
					if (createLocalDirs(log, destinationDir))
					{
						log.AddLog("[INFO] Created local directory: %s\n", destinationDir.string().c_str());
					}
					else
					{
						log.AddLog("[ERROR] Failed to create local directory: %s\n", destinationDir.string().c_str());
						return;
					}
				}
				// 5. Copy files from source to destination
				for (const auto& _interface : selectedInterfaces)
				{
					auto pos = _interface.find_last_of('\\') + 2;
					std::string interfaceName = _interface.substr(pos, _interface.size());
					std::filesystem::path sourcePath = pathToInterfaces / _interface;
					std::filesystem::path destinationPath = destinationDir.string() + interfaceName;
					moveFiles(log, sourcePath, destinationPath);					
				}
			}
			else
			{
				log.AddLog("[ERROR] Cannot copy files. Please ensure you are connected to the VPN and have selected a valid product line and local directory.\n");
			}
		}
	}

	if (!vpnConnected)
		ImGui::EndDisabled();
	
}

bool testDirectory(const std::string& path)
{
	// Test that we can reach the directory
	if (!std::filesystem::exists(path))
		return false;
	else
		return true;
}

std::string getProductLine(AppLog& log)
{
	static int products;
	ImGui::SeparatorText("Select a product");
	ImGui::RadioButton("RMS_JMS", &products, 0); ImGui::SameLine();
	ImGui::RadioButton("MOBILE", &products, 1); ImGui::SameLine();
	ImGui::RadioButton("CAD", &products, 2);
	switch (products)
	{
	case 0: return "RMS_JMS_Interfaces"; break;
	case 1: return "MOBILE"; break;
	case 2: return "ONESolutionCAD_Services"; break;
	default: log.AddLog("[ERROR] Invalid product selection.\n"); return ""; break;
	}
}

std::filesystem::path getFilePath(AppLog& log, const std::string& productName, const std::string& rootSourcePath)
{
	std::string pathString = rootSourcePath + productName + "\\";
	std::filesystem::path pathToInterfaces = pathString;
	return pathToInterfaces;
}

std::vector<std::string> getInterfaceNames(AppLog& log, std::filesystem::path& pathToInterfaces)
{
	// TODO find a way to detect if the folder has more subfolders called EA or GA, then when it's selected display a popup window for the user to browse further in and select the EA/GA version they want
	try {
		if (std::filesystem::exists(pathToInterfaces) && std::filesystem::is_directory(pathToInterfaces))
		{
			std::vector<std::string> interfaces;
			for (const auto& entry : std::filesystem::directory_iterator(pathToInterfaces))
			{
				if (entry.is_directory())
				{
					interfaces.push_back(entry.path().filename().string());
				}
			}
			return interfaces;
		}
		else
		{
			log.AddLog("[ERROR] Path to interfaces does not exist or is not a directory: %s\n", pathToInterfaces.string().c_str());
			return std::vector<std::string>();
		}
	}
	catch (const std::filesystem::filesystem_error& e) {
		log.AddLog("[ERROR] Filesystem error: %s\n", e.what());
	}
	return std::vector<std::string>();
}

void displayListOfInterfaces(AppLog& log, const std::vector<std::string>& interfaces, std::vector<std::string>& selectedInterfaces, const std::filesystem::path& pathToInterfaces)
{
	static bool selectedInterface[1000];
	static ImGuiTextFilter filter;
	ImGui::SetNextItemWidth(200);
	// Clear selections and filter value
	if (ImGui::Button("Clear"))
	{
		for (int i = 0; i < 1000; i++)
		{
			selectedInterface[i] = false;
		}
		filter.Clear();
	} ImGui::SameLine();

	// Create filter input box
	filter.Draw("Filter");

	for (int i = 0; i < interfaces.size(); i++)
	{
		if(filter.PassFilter(interfaces[i].c_str()))
			ImGui::Selectable(interfaces[i].c_str(), &selectedInterface[i]);
	}
	for (int i = 0; i < interfaces.size(); i++)
	{
		if(selectedInterface[i] &&
			std::find(selectedInterfaces.begin(), selectedInterfaces.end(), pathToInterfaces.string() + interfaces[i]) == selectedInterfaces.end() &&
			interfaces[i].substr(0, 1) != "_")
		{
			selectedInterfaces.push_back(pathToInterfaces.string() + interfaces[i]);
			log.AddLog("[INFO] Selected interface: %s\n", interfaces[i].c_str());
		}
		else if(!selectedInterface[i])
		{
			auto it = std::find(selectedInterfaces.begin(), selectedInterfaces.end(), pathToInterfaces.string() + interfaces[i]);
			if (it != selectedInterfaces.end())
			{
				selectedInterfaces.erase(it);
				log.AddLog("[INFO] Deselected interface: %s\n", interfaces[i].c_str());
			}
		}
	}
}

bool checkLocalDirs(AppLog& log, const std::filesystem::path& path)
{
	try {
		if (std::filesystem::exists(path))
		{
			log.AddLog("[INFO] Directory exists: %s\n", path.string().c_str());
			return true;
		}
		else
		{
			log.AddLog("[WARN] Directory DOES NOT exist: %s\n", path.string().c_str());
			return false;
		}
	}
	catch (std::filesystem::filesystem_error& err) {
		log.AddLog("[ERROR] Filesystem error: %s\n", err.what());
		return false;
	}
}

bool createLocalDirs(AppLog& log, const std::filesystem::path& localDir)
{
	try {
		if (std::filesystem::create_directory(localDir))
		{
			log.AddLog("[INFO] Created local directory: %s\n", localDir.string().c_str());
			return true;
		}
		else
			return false;
	} catch (std::filesystem::filesystem_error& err) {
		log.AddLog("[ERROR] Failed to create directory: %s\n", err.what());
		return false;
	}
}

bool moveFiles(AppLog& log, const std::filesystem::path& sourceDir, const std::filesystem::path& destinationDir)
{
	try 
	{
		if (!std::filesystem::exists(destinationDir))
		{
			createLocalDirs(log, destinationDir);	// Create the destination directory with interface folder if it does not exist
		}
		else 
		{
			log.AddLog("[ERROR] Couldn't create directory: %s\n", destinationDir.string().c_str());
			return false;
		}

		// Copy from source to destination recursivley and overwrite any existing matching files in destination
		std::filesystem::copy(sourceDir, destinationDir, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);

		// Check that it exists after copy to confirm that it was successful
		if (std::filesystem::exists(destinationDir))
		{
			log.AddLog("[INFO] Successfully copied files from %s to %s\n", sourceDir.string().c_str(), destinationDir.string().c_str());
			return true;
		}
		else
		{
			log.AddLog("[ERROR] Failed to copy files from %s to %s\n", sourceDir.string().c_str(), destinationDir.string().c_str());
			return false;
		}
	}
	catch (std::filesystem::filesystem_error& err)
	{
		log.AddLog("[ERROR] Filesystem error: %s\n", err.what());
		return false;
	}
	catch (std::exception& e)
	{
		log.AddLog("[ERROR] General error: %s\n", e.what());
		return false;
	}
	return false;	// Default return
}
