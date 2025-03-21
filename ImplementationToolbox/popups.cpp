#include "imgui.h"
#include "popups.h"

void genExportInfoModPop(bool* open) {
	ImGui::SetNextWindowSize(ImVec2(550, 200));
	// Begin information window
	ImGui::Begin("Generic Export Information", open, ImGuiWindowFlags_NoResize);

	ImGui::TextWrapped("This tool is designed to help generate and edit generic exports in ONESolution JMS easily, quickly, and correctly every time using SQL scripts.");
	ImGui::TextWrapped("Before any script from this tool can be run you MUST log in to JMS and add then saving a blank generic export."
		"No infomration is required to be added to it, but the generic export ID MUST be generated, and the fields must also be populated before attempting to run these queries.");
	ImGui::TextWrapped("To start, expand the generic export configuration header to begin configuring the generic export.");
	ImGui::Text("* = required field");

	// End Gen Export info window
	ImGui::End();	
}

static void genExportHelpModPop() {

}