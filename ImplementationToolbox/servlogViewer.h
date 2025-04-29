#pragma once
#include <string>
#include <vector>
#include "imgui.h"
class Sql;
class AppLog;
class UserSettings;

void servlogViewer(bool*, Sql&, AppLog&, UserSettings&);

void StartTimerThread(AppLog& log);

void StopTimerThread(AppLog& log);

void SaveFilteredDataToCSV(const std::vector<int>& servlogid, const std::vector<std::string>& service, const std::vector<std::string>& product, const std::vector<std::string>& logtime, const std::vector<std::string>& logtype, const std::vector<std::string>& descriptn, const std::vector<std::string>& computer, const ImGuiTextFilter& filter, AppLog& log);

std::string AddWhere(AppLog& log, bool& open);

void LogChanges(AppLog& log, int* column, int* order);

struct ServlogTable
{
private:
	std::vector<int> servlogid;
	std::vector<std::string> service;
	std::vector<std::string> product;
	std::vector<std::string> logtime;
	std::vector<std::string> logtype;
	std::vector<std::string> descriptn;
	std::vector<std::string> computer;
public:
	void setServlogid(Sql& sql, int quant);
	std::vector<int> getServlogid();

	void setService(Sql& sql, int quant);
	std::vector<std::string> getService();

	void setProduct(Sql& sql, int quant);
	std::vector<std::string> getProduct();

	void setLogtime(Sql& sql, int quant);
	std::vector<std::string> getLogtime();

	//void setLogtype(Sql& sql, int quant);
	std::vector<std::string> getLogtype();

	void setDescriptn(Sql& sql, int quant);
	std::vector<std::string> getDescriptn();

	void setComputer(Sql& sql, int quant);
	std::vector<std::string> getComputer();

	void setServlogData
	(
		Sql& sql,
		std::string column,
		int quantity,
		int ascdesc
	);
	void setServlogData
	(
		Sql& sql,
		std::string column,
		int quantity,
		int ascdesc,
		std::string where
	);
};