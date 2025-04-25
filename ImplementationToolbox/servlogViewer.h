#pragma once
#include <string>
#include <vector>
class Sql;
class AppLog;
class UserSettings;

void servlogViewer(bool*, Sql&, AppLog&, UserSettings&);

void StartTimerThread(AppLog& log);

void StopTimerThread(AppLog& log);

struct ServlogTable
{
private:
	std::vector<int> servlogid;
	std::vector<std::string> service;
	std::vector<std::string> product;
	std::vector<std::string> logtime;
	std::vector<char> logtype;
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

	void setLogtype(Sql& sql, int quant);
	std::vector<char> getLogtype();

	void setDescriptn(Sql& sql, int quant);
	std::vector<std::string> getDescriptn();

	void setComputer(Sql& sql, int quant);
	std::vector<std::string> getComputer();
};