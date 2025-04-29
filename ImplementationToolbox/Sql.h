#pragma once
#include <string>
#include <vector>
#include "imgui.h"

#ifndef SQL_H
#define SQL_H

struct AppLog;

class Sql
{
private:
	std::string source,
		database,
		username,
		password,
		connectionString;
	bool 
		connectionAttempt,
		connected,
		savedString;

	// TODO Add a string variable that gets passed as a reference to any query to a database. Create a separate function in SqlHelper that always runs first called
	// queryTester which will run the current query first using count or begin/rollback query to see if it executes successfully, if not, throw an error and return that
	// error so we can capture it in a our application log and display to user instead of just in console window
public:
	// Setters
	void _SetSource(std::string s) {
		source = s;
	}
	void _SetDatabase(std::string db) {
		database = db;
	}
	void _SetUsername(std::string un) {
		username = un;
	}
	void _SetPassword(std::string pw) {
		password = pw;
	}
	void _SetConnectionAttempt() {
		connectionAttempt = true;
	}
	void _SetConnected(bool c) {
		connected = c;
	}
	void _SetConnectionString() {
		connectionString = "Data Source=" + source + ";Initial Catalog=" + database + ";User ID=" + username + ";Password=" + password + ";";
	}
	// Getters
	std::string _GetSource() {
		return source;
	}
	std::string _GetDatabase() {
		return database;
	}
	std::string _GetUsername() {
		return username;
	}
	std::string _GetPassword() {
		return password;
	}
	bool _GetConnectionAttempt() {
		return connectionAttempt;
	}
	bool _GetConnected() const {
		return connected;
	}
	std::string _GetConnectionString() {
		return connectionString;
	}
	bool _GetSavedString() {
		return savedString;
	}
	void _SetSavedString(bool s) {
		savedString = s;
	}

	bool requiredInfo(std::string, std::string, std::string, std::string);
	bool executeQuery(std::string);
	void DisplaySqlConfigWindow(bool*, std::string, AppLog&);
	void saveConnString(std::string, std::string name, AppLog&);
	void saveConnString(std::string dir, std::string name, int servertype);
	int returnRecordCount(std::string table, std::string column);
	int returnRecordCount(std::string table, std::string column, std::string value);
	std::vector<std::vector<std::string>> getTableColumns(std::string connStr, std::string table);
	int returnTableCount(std::string connStr);
	std::vector<std::string> getTableNames(std::string connStr, std::string database);
	std::string displayConnectionName(const std::string& directory);
	void readConnString(const std::string dir, char* source, char* db, char* un, char* pw, bool);
	bool readConnString(const std::string);
	std::vector<std::string> returnStrQry(const std::string connectionstring, const std::string column, const std::string table, int quantity, int ascdesc);
	std::vector<std::string> returnDtStrQry(const std::string connectionstring, const std::string column, const std::string table, int quantity);
	std::vector<int> returnIntQry(const std::string connectionstring, const std::string column, const std::string table, int quantity);
	void servLogQuery
	(
		std::string connectionString,				// String containing connection info for SQL
		std::string column,							// Column that is being used in the order by clause
		int quantity,								// Number of rows expected in return
		int ascdesc,								// 0 = ascending order, 1 = descending order to return results in
		std::vector<int>& servlogid,				// Reference to servlog servlogid
		std::vector<std::string>& service,			// Reference to servlog service
		std::vector<std::string>& product,			// Reference to servlog product
		std::vector<std::string>& logtime,			// Reference to servlog logtime
		std::vector<std::string>& descriptn,		// Reference to servlog descriptn
		std::vector<std::string>& computer,			// Reference to servlog computer
		std::vector<std::string>& logtype			// Reference to servlog logtype
	);
	void servLogQuery
	(
		std::string connectionString,				// String containing connection info for SQL
		std::string column,							// Column that is being used in the order by clause
		int quantity,								// Number of rows expected in return
		int ascdesc,								// 0 = ascending order, 1 = descending order to return results in
		std::string where,							// Pass custom where statement to servlog select
		std::vector<int>& servlogid,				// Reference to servlog servlogid
		std::vector<std::string>& service,			// Reference to servlog service
		std::vector<std::string>& product,			// Reference to servlog product
		std::vector<std::string>& logtime,			// Reference to servlog logtime
		std::vector<std::string>& descriptn,		// Reference to servlog descriptn
		std::vector<std::string>& computer,			// Reference to servlog computer
		std::vector<std::string>& logtype			// Reference to servlog logtype
	);
};

#endif // SQL_H