#include "imgui.h"
#include "beatImport.h"
#include "Beat.h"
#include "Beats.h"
#include "genericDataImport.h"
#include <fstream>
#include <sstream>

void beatImport(std::string dir)
{
	// Init Beats object
	static Beats beats;

	// Vector to hold each beat and their data
	static std::vector<Beat> beat;
	static std::vector<std::string> rows;
	static std::vector<std::string> cols;

	// Buffers to read the data into
	static char code_fbi[30][CODE_KEY_LEN];
	static char code_sbi[30][CODE_KEY_LEN];
	static char code_key[30][CODE_KEY_LEN];
	static char code_agcy[30][CODE_KEY_LEN];
	static char descriptn[30][55];
	static char sys_msg[30][40];
	static char sys_use[30][30];
	static char notes[30][1000];
	static int internal[30][1];

	// Value for successful entries
	static int recordCount = 0;

	// Arrays to store column data into for drag&drop
	static const char* cad_labels[]
	{
		"code_fbi", "code_sbi", "code_agcy", "code_key", "descriptn", "interrnal", "sys_msg", "sys_use", "notes"
	};
	static const char* cad_targets[]
	{
		"", "", "", "", "", "", "", "", ""
	};

	try
	{
		ImGui::Text("Select a file to import: "); ImGui::SameLine();
		std::string beatFileName = displayBeatFiles(dir);
		beats.setFileName(dir + beatFileName);
	}
	catch (std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
	}

	static bool beat_imported = false;
	ImGui::SameLine();
	if (!beat_imported && beats.getFileName() != "C:\\ImplementationToolbox\\Maps\\Beats\\")
	{
		if (ImGui::Button("Import CSV##beats"))
			beat_imported = true;
	}
	if (beat_imported || beats.getFileName() == "C:\\ImplementationToolbox\\Maps\\Beats\\")
	{
		ImGui::BeginDisabled();
		ImGui::Button("Import CSV");
		ImGui::EndDisabled();
	}
	if (beat_imported)
	{
		static bool read_cols = false;
		static bool read_rows = false;
		while (!read_cols)
		{
			readBeatColumns(beats);
			// If we find out we have less than 9 columns imported populate the remaining with blanks
			if (beats.getColsCount() < 9)
			{
				for (int i = beats.getColsCount(); i < IM_ARRAYSIZE(cad_labels); i++)
				{
					beats.setCols("");
				}
			}
			static int columncount = beats.getColsCount();
			for(int i = 0; i < columncount; i++)
			{
				cols.push_back(beats.getCols(i));
			}			
				read_cols = true;
		}
		while (!read_rows)
		{
			readBeatRows(beats);
			rows = beats.getRows();
			beat = buildBeats(beats);
			for (int i = 0; i < beat.size(); i++)
			{
				strncpy_s(code_fbi[i], beat[i].getCodeFbi(), IM_ARRAYSIZE(code_fbi[i]));
				strncpy_s(code_sbi[i], beat[i].getCodeSbi(), IM_ARRAYSIZE(code_sbi[i]));
				strncpy_s(code_agcy[i], beat[i].getCodeAgcy(), IM_ARRAYSIZE(code_agcy[i]));
				strncpy_s(code_key[i], beat[i].getCodeKey(), IM_ARRAYSIZE(code_key[i]));
				strncpy_s(descriptn[i], beat[i].getDescriptn(), IM_ARRAYSIZE(descriptn[i]));
				strncpy_s(sys_msg[i], beat[i].getSysMsg(), IM_ARRAYSIZE(sys_msg[i]));
				strncpy_s(sys_use[i], beat[i].getSysUse(), IM_ARRAYSIZE(sys_use[i]));
				strncpy_s(notes[i], beat[i].getNotes(), IM_ARRAYSIZE(notes[i]));
			}
			read_rows = true;
		}


		if (ImGui::CollapsingHeader("Geotab1 column mapping")) {
			enum Mode
			{
				Mode_Copy,
				Mode_Move,
				Mode_Swap
			};
			static int mode = 0;		// Defaulting to copy for now
			/*if (ImGui::RadioButton("Copy", mode == Mode_Copy)) { mode = Mode_Copy; } ImGui::SameLine();
			if (ImGui::RadioButton("Move", mode == Mode_Move)) { mode = Mode_Move; } ImGui::SameLine();
			if (ImGui::RadioButton("Swap", mode == Mode_Swap)) { mode = Mode_Swap; }*/

			// TODO Add handling for less than 9 columns in the CSV import file
			// TODO Add handling for column names that are numbers only
			// Create blank buttons to assign column names to
			if (ImGui::BeginTable("CAD_Fields", 3, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Borders, ImVec2(ImGui::GetContentRegionMax().x / 2, 0)))
			{
				ImGui::TableSetupColumn("CAD Column", ImGuiTableColumnFlags_None, .2);
				ImGui::TableSetupColumn("Mapped Column", ImGuiTableColumnFlags_None, .4);
				ImGui::TableSetupColumn("Imported Column", ImGuiTableColumnFlags_None, .4);
				ImGui::TableHeadersRow();
				for (int i = 0; i < IM_ARRAYSIZE(cad_labels); i++)
				{
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::Text("%s", cad_labels[i]);
					ImGui::TableNextColumn();
					ImGui::Button(cad_targets[i], ImVec2(120, 30));

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Imported_column_names"))
						{
							IM_ASSERT(payload->DataSize == sizeof(int));
							int payload_n = *(const int*)payload->Data;
							if (mode == Mode_Copy)
							{
								cad_targets[i] = cols[payload_n].c_str();
							}
							if (mode == Mode_Move)
							{
								cad_targets[i] = cols[payload_n].c_str();
								cols[payload_n] = "";
							}
							if (mode == Mode_Swap)
							{
								const char* tmp = cad_targets[i];
								cad_targets[i] = cols[payload_n].c_str();
								cols[payload_n] = tmp;
							}
						}
						ImGui::EndDragDropTarget();
					}
					ImGui::TableNextColumn();
					ImGui::Button(("%s", cols[i]).c_str(), ImVec2(120, 30));

					// Buttons for both drag sources and drag targets here
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
					{
						ImGui::SetDragDropPayload("Imported_column_names", &i, sizeof(int));

						// Display preview
						if (mode == Mode_Copy || mode == Mode_Move || mode == Mode_Swap) { ImGui::Text("Mapping column %s", cols[i]); }
						ImGui::EndDragDropSource();
					}
				}
				// End column table
				ImGui::EndTable();
			}
		}
	}
}

/// <summary>
/// Function to read in file names and display in an ImGui::ComboBox for selection
/// </summary>
/// <param name="dir">is the directory path to the unit_csv files</param>
/// <returns>selected file name</returns>
std::string displayBeatFiles(std::string dir) {
	static std::string chosenName;
	std::vector<std::string> fileNames;
	for (const auto& entry : std::filesystem::directory_iterator(dir))
	{
		std::string filename = entry.path().filename().string();
		size_t pos = filename.find(".csv");
		if (pos != std::string::npos) {
			std::string displayName = filename;
			fileNames.push_back(displayName);
		}
	}

	// Convert std::vector<std::string> to array of const char* for ImGui display
	std::vector<const char*> fileNameCStr;
	for (const auto& name : fileNames) {
		fileNameCStr.push_back(name.c_str());
	}

	// Display the Combo box if we find a profile has been created otherwise we show text instead
	static int currentItem = 0;
	ImGui::SetNextItemWidth(250);
	if (!fileNameCStr.empty() && ImGui::BeginCombo(("##Select beat csv" + dir).c_str(), fileNameCStr[currentItem])) {
		for (int n = 0; n < fileNameCStr.size(); n++) {
			bool isSelected = (currentItem == n);
			if (ImGui::Selectable(fileNameCStr[n], isSelected)) {
				currentItem = n;
				chosenName = fileNameCStr[n];
			}
			if (isSelected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		// End Combo Box for profile drop down
		ImGui::EndCombo();
	}
	else if (fileNameCStr.empty())
		ImGui::TextWrapped("No CSV files found. Place a .csv file in %s", dir.c_str());

	// Return the currently selected name in the combo box
	return chosenName;
}

void readBeatColumns(Beats& beats) {
	std::string dir = beats.getFileName();
	std::ifstream beatData;

	beatData.open(dir);
	std::string line;
	std::getline(beatData, line);
	std::stringstream ss(line);
	std::string token;
	// Read comma separated ints into a single string, then parse them out with getline and pass each one individually to setSelected function
	while (std::getline(ss, token, ',')) {
		beats.setCols(token);
	}
}

void readBeatRows(Beats& beats)
{
	std::string dir = beats.getFileName();
	std::ifstream beatData;
	beatData.open(dir);

	if (!beatData)
		return;
	beatData.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	std::string line;
	while (std::getline(beatData, line))
	{
		beats.setRows(line);
	}
}

// Need to rebuild function since it needs to take custom columns anyway. We'll only pass it columns that are mapped in the future and refer to columns within function in vague terms so it doesn't crash when the specific columns aren't present
std::vector<Beat> buildBeats(Beats& beats)
{
	std::vector<Beat> beat;
	std::vector<std::string> rows = beats.getRows();
	std::string value;

	// Buffers
	static char code_fbi[30][CODE_KEY_LEN];
	static char code_sbi[30][CODE_KEY_LEN];
	static char code_key[30][CODE_KEY_LEN];
	static char code_agcy[30][CODE_KEY_LEN];
	static char descriptn[30][55];
	static char sys_msg[30][40];
	static char sys_use[30][30];
	static char notes[30][1000];

	for (int i = 0; i < rows.size(); i++)
	{
		std::vector<std::string> values;
		std::stringstream ss(rows[i]);
		while (std::getline(ss, value, ','))
		{
			values.push_back(value);
		}
		// strncpy_s(code_fbi.c_str(), values[0].c_str(), code_fbi.size());
		strncpy_s(code_sbi[i], values[1].c_str(), IM_ARRAYSIZE(code_sbi[i]));
		strncpy_s(code_agcy[i], values[2].c_str(), IM_ARRAYSIZE(code_agcy[i]));
		strncpy_s(code_key[i], values[3].c_str(), IM_ARRAYSIZE(code_key[i]));
		strncpy_s(descriptn[i], values[4].c_str(), IM_ARRAYSIZE(descriptn[i]));
		strncpy_s(sys_msg[i], values[5].c_str(), IM_ARRAYSIZE(sys_msg[i]));
		strncpy_s(sys_use[i], values[6].c_str(), IM_ARRAYSIZE(sys_use[i]));
		strncpy_s(notes[i], values[7].c_str(), IM_ARRAYSIZE(notes[i]));
		beat.push_back(Beat(code_fbi[i], code_sbi[i], code_agcy[i], code_key[i], descriptn[i], sys_msg[i], sys_use[i], notes[i], std::stoi(values[8])));
	}
	return beat;
}