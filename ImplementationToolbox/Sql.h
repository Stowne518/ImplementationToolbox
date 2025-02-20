#pragma once
#include <string>
#include "imgui.h"

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

	bool requiredInfo(std::string, std::string, std::string, std::string);
	void executeQuery(std::string);
	void DisplaySqlConfigWindow(bool*);
};

#endif // SQL_H