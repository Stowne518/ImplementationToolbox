#pragma once
#include "imgui.h"
#include <ctype.h>
#include <stdio.h>
#include <map>
#include <string>
#include <chrono>
#include <ostream>
#include <iomanip>
#include <sstream>
#include <ctime>
struct AppLog
{
	ImGuiTextBuffer		Buf;
	ImGuiTextFilter		Filter;
	ImVector<int>		LineOffsets; // Index to lines offset	
	bool				AutoScroll;

	// Map to store previous states of boolean variables
	std::map<std::string, bool> previousStates;

	AppLog()
	{
		AutoScroll = true;
		Clear();
	}

	void Clear()
	{
		Buf.clear();
		LineOffsets.clear();
		LineOffsets.push_back(0);
	}
	
    void AddLog(const char* fmt, ...) IM_FMTARGS(2)
    {
       int old_size = Buf.size();

       // Get current system time
       auto now = std::chrono::system_clock::now();
       auto time_t_now = std::chrono::system_clock::to_time_t(now);
       std::tm local_tm;
    #ifdef _WIN32
       localtime_s(&local_tm, &time_t_now); // Windows-specific
    #else
       localtime_r(&time_t_now, &local_tm); // POSIX-compliant
    #endif

       // Format time as [mm/dd/yyyy hh:mm:ss]
       std::ostringstream timeStream;
       timeStream << "[" << std::put_time(&local_tm, "%m/%d/%Y %H:%M:%S") << "] ";

       // Append timestamp to the buffer
       Buf.append(timeStream.str().c_str());

       // Append the log message
       va_list args;
       va_start(args, fmt);
       Buf.appendfv(fmt, args);
       va_end(args);

       // Update line offsets
       for (int new_size = Buf.size(); old_size < new_size; old_size++)
           if (Buf[old_size] == '\n')
               LineOffsets.push_back(old_size + 1);
    }

	void Draw(const char* title, bool* p_open = NULL)
	{
		if (!ImGui::Begin(title, p_open))
		{
			ImGui::End();
			return;
		}

		// Options Menu
		if (ImGui::BeginPopup("Options"))
		{
			ImGui::Checkbox("Auto-scroll", &AutoScroll);
			ImGui::EndPopup();
		}

		// Main Window
		
		bool clear = ImGui::Button("Clear");
		ImGui::SameLine();
		bool copy = ImGui::Button("Copy");
		ImGui::Separator();
		if (ImGui::Button("Options"))
			ImGui::OpenPopup("Options");
		ImGui::SameLine();
		Filter.Draw("Filter", -100.0f);

		ImGui::Separator();

		if (ImGui::BeginChild("Scrolling", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_AlwaysHorizontalScrollbar))
		{
			if (clear)
				Clear();
			if (copy)
				ImGui::LogToClipboard();

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			const char* buf = Buf.begin();
			const char* buf_end = Buf.end();
			// If filter is active we can't use list clipper or unformatted text
			if (Filter.IsActive())
			{
				for (int line_no = 0; line_no < LineOffsets.Size; line_no++)
				{
					const char* line_start = buf + LineOffsets[line_no];
					const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
					if (Filter.PassFilter(line_start, line_end))
						ImGui::TextUnformatted(line_start, line_end);
				}
			}
			else
			{
				// Use clipper if we don't have filter active
				// This  lets us format items the same way and handle large sets of text data for almost no cost
				ImGuiListClipper clipper;
				clipper.Begin(LineOffsets.Size);
				while (clipper.Step())
				{
					for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
					{
						const char* line_start = buf + LineOffsets[line_no];
						const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
						ImGui::TextUnformatted(line_start, line_end);
					}
				}
				clipper.End();
			}
			ImGui::PopStyleVar();
			if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
				ImGui::SetScrollHereY(1.0f);
		}
		ImGui::EndChild();
		ImGui::End();
	}	

	// Function to log message only once when boolean state changes
	void logStateChange(const std::string& varName, bool currentState)
	{
		if (previousStates.find(varName) == previousStates.end())
		{
			// Initialize previous state if not found
			previousStates[varName] = !currentState;
		}

		if (previousStates[varName] != currentState)
		{
			// Log the message if state changes
			AddLog("[DEBUG] State of \'%s\' changed to \'%s\'\n", varName.c_str(), currentState ? "true" : "false");
			// Update the previous state
			previousStates[varName] = currentState;
		}
	}
};

static void ShowAppLog(bool* p_open)
{
	static AppLog log;
	ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
	ImGui::Begin("Debug Log", p_open);
	// ImGui::End();
	log.Draw("Debug Log", p_open);
}

