#ifndef SQLCONNECTIONHANDLER_H
#define SQLCONNECTIONHANDLER_H

#include <string>
#using <SqlConnector.dll>
#using <System.dll>
#using <mscorlib.dll>
#include <msclr/marshal.h>
#include <msclr/marshal_cppstd.h>
#include <vector>

using namespace System;
using namespace msclr::interop;
using namespace SqlConnector;

namespace SqlConnectionHandler {

	std::string ConvertStringToStdString(System::String^ managedString)
	{
		return msclr::interop::marshal_as<std::string>(managedString);
	}

	bool sqlConnectionHandler(std::string source, std::string database, std::string user, std::string pass) {
		SqlHelper^ helper = gcnew SqlHelper();
		helper->SetSource(marshal_as<String^>(source));
		helper->SetDatabase(marshal_as<String^>(database));
		helper->SetUserName(marshal_as<String^>(user));
		helper->SetPassword(marshal_as<String^>(pass));
		helper->SetConnectionString();
		String^ connectionString = (helper->GetConnectionstring());

		bool connected = helper->ConnectToDatabase(connectionString);

		return connected;
	}

	bool executeUpdateQuery(std::string connectionString, std::string query) {
		SqlHelper^ helper = gcnew SqlHelper();
		String^ c_connectionString = marshal_as<String^>(connectionString);
		String^ s_query = marshal_as<String^>(query);

		if (!helper->ExecuteUpdateQuery(c_connectionString, s_query))
			return false;
		else
			return true;
	}

	int getRecordCount(std::string connectionString, std::string database, std::string table, std::string column) {
		SqlHelper^ helper = gcnew SqlHelper();
		String^ c_connectionString = marshal_as<String^>(connectionString);
		String^ s_database = marshal_as<String^>(database);
		String^ s_table = marshal_as<String^>(table);
		String^ s_column = marshal_as<String^>(column);

		int count = helper->ReturnCount(c_connectionString, s_database, s_table, s_column);

		return count;
	}

	int getRecordCount(std::string connectionString, std::string database, std::string table, std::string column, std::string value) {
		SqlHelper^ helper = gcnew SqlHelper();
		String^ c_connectionString = marshal_as<String^>(connectionString);
		String^ s_database = marshal_as<String^>(database);
		String^ s_table = marshal_as<String^>(table);
		String^ s_column = marshal_as<String^>(column);
		String^ s_value = marshal_as<String^>(value);

		int count = helper->ReturnSingleCount(c_connectionString, s_database, s_column, s_table, s_value);

		return count;
	}

	std::vector<std::string> getColumns(std::string connectionString, std::string table)
	{
		SqlHelper^ helper = gcnew SqlHelper();
		String^ c_connectionString = marshal_as<String^>(connectionString);
		String^ c_table = marshal_as<String^>(table);
		std::vector<std::string> v_columns;
		array<String^>^ columns = helper->ReturnColumns(c_connectionString, c_table);
		for (int i = 0; i < columns->Length; i++)
		{
			v_columns.push_back(ConvertStringToStdString(columns[i]));
		}
		return v_columns;
	}	

	int getTableCount(std::string connectionString, std::string database) 
	{
		SqlHelper^ helper = gcnew SqlHelper();
		String^ c_connectionString = marshal_as<String^>(connectionString);
		String^ c_database = marshal_as<String^>(database);
		int count = helper->ReturnTableCount(c_connectionString, c_database);
		return count;
	}

	std::vector<std::string> getTables(std::string connectionString, std::string database) 
	{
		SqlHelper^ helper = gcnew SqlHelper();
		String^ c_connectionString = marshal_as<String^>(connectionString);
		String^ c_database = marshal_as<String^>(database);
		std::vector<std::string> v_tables;
		array<String^>^ tables = helper->ReturnTableNames(c_connectionString, c_database);
		for (int i = 0; i < tables->Length; i++)
		{
			v_tables.push_back(ConvertStringToStdString(tables[i]));
		}
		return v_tables;
	}
}

#endif // !SQLCONNECTIONHANDLER_H
