#pragma once
#include "imgui.h"
#include <iostream>
#include <windows.h>
#include <sqlext.h>
#include <sql.h>
#include <string>
#include <sqlConnectionHandler.h>
#include <sqlConnectionHandler.cpp>

#using <SqlConnectionHandler.dll>

class Sql
{
private:
	std::string source,
		database,
		username,
		password;
	bool 
		connectionAttempt,
		connected;

public:
	// Setters
	void SetSource(std::string s) {
		source = s;
	}
	void SetDatabase(std::string db) {
		database = db;
	}
	void SetUsername(std::string un) {
		username = un;
	}
	void SetPassword(std::string pw) {
		password = pw;
	}
	void SetConnectionAttempt() {
		connectionAttempt = true;
	}
	void SetConnected(bool c) {
		connected = c;
	}
	// Getters
	std::string GetSource() {
		return source;
	}
	std::string GetDatabase() {
		return database;
	}
	std::string GetUsername() {
		return username;
	}
	std::string GetPassword() {
		return password;
	}
	bool GetConnectionAttempt() {
		return connectionAttempt;
	}
	bool GetConnected() const {
		return connected;
	}

	bool requiredInfo(std::string s, std::string db, std::string u, std::string p) {
		// Should check if any field is blank on the SQL Connection form and if so not allow the connection attempt
		if (s == "" || db == "" || u == "" || p == "")
			return false;

		return true;
	}

	// Convert C++ to C# strings for connection handler
	String^ convertString(std::string s) {
		String^ cs = marshal_as<String^>(s);
		return cs;
	}

	void connect() {
		connectionAttempt = true;
		String^ connectionString = SqlConnectionHandler::sqlConnectionHandler(convertString(GetSource()), convertString(GetDatabase()), convertString(GetUsername()), convertString(GetPassword()));
		SetConnected(SqlConnectionHandler::connectionTest(connectionString));
	}
};

