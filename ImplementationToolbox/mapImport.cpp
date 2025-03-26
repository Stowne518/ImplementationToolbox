#include "imgui.h"
#include "mapImport.h"
#include "Sql.h"
#include "beatImport.h"

void mapImport(Sql& sql, std::string dir)
{
	// Function variables
	const char
		beatLabel[] = "Import Beats",
		districtLabel[] = "Import Districts",
		stationLabel[] = "Import Stations",
		geoproxLabel[] = "Import Geoproximity",
		responseplanLabel[] = "Import Response Plans",
		reportingareaLabel[] = "Import Reporting Areas";

	std::string 
		map_dir = dir + "Maps\\",
		beats_dir = map_dir + "Beats\\",
		district_dir = map_dir + "Districts\\",
		station_dir = map_dir + "Stations\\",
		geoprox_dir = map_dir + "GeoProx\\",
		responseplan_dir = map_dir + "ResponsePlans\\",
		reportingarea_dir = map_dir + "ReportingAreas\\";

	// Map import tree
	if (ImGui::CollapsingHeader(beatLabel))
	{
		beatImport(beats_dir);
	}
	if (ImGui::CollapsingHeader(districtLabel))
	{

	}
	if (ImGui::CollapsingHeader(stationLabel))
	{

	}
	if (ImGui::CollapsingHeader(geoproxLabel))
	{

	}
	if (ImGui::CollapsingHeader(responseplanLabel))
	{

	}
	if (ImGui::CollapsingHeader(reportingareaLabel))
	{

	}
}