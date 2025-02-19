#include "imgui.h"
#include <iostream>
#include <windows.h>
#include <sqlext.h>
#include <sql.h>
#include <string>
#include "sqlConnectionHandler.h"

using namespace SqlConnectionHandler;


#ifndef SQL_H
#define SQL_H

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
		connected;

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

	bool requiredInfo(std::string s, std::string db, std::string u, std::string p) {
		// Should check if any field is blank on the SQL Connection form and if so not allow the connection attempt
		if (s == "" || db == "" || u == "" || p == "")
			return false;

		return true;
	}
};


//
//void connect(Sql&);
//
//void runSQLQuery(Sql&/*std::string*/);

#endif // SQL_H