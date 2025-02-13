#include "imgui.h"
#include <iostream>
#include <windows.h>
#include <sqlext.h>
#include <sql.h>
#include <string>
#include <sqlConnectionHandler.h>
#include <sqlConnectionHandler.cpp>

#using <SqlConnectionHandler.dll>

using namespace System;
using namespace SqlConnectionHandler;

bool sqlConnection(std::string& source, std::string& database, std::string& user, std::string& pass) {

	// Convert cpp strings into C# strings
	String^ s_source = marshal_as<String^>(source);
	String^ s_database = marshal_as<String^>(database);
	String^ s_user = marshal_as<String^>(user);
	String^ s_pass = marshal_as<String^>(pass);

	if (!sqlConnectionHandler(s_source, s_database, s_user, s_pass))
		return false;
	else {	
		runSelectQuery(s_source, s_database, s_user, s_pass);
		return true;
	}
}