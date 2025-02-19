//#include "Sql.h"
// Convert C++ to C# strings for connection handler
//System::String^ convertString(std::string s) {
//	String^ cs = marshal_as<String^>(s);
//	return cs;
//}
//
//String^ GetSqlConnectionString(Sql& sql) {
//	return SqlConnectionHandler::sqlConnectionHandler(convertString(sql.GetSource()), convertString(sql.GetDatabase()), convertString(sql.GetUsername()), convertString(sql.GetPassword()));
//}
//
//void connect(Sql& sql) {
//	
//	String^ connString = GetSqlConnectionString(sql);
//	bool s_connected = SqlConnectionHandler::connectionTest(connString);
//	sql.SetConnected(s_connected);
//}
//
//void runSQLQuery(Sql& sql) {
//	SqlConnectionHandler::executeUpdateQuery(GetSqlConnectionString(sql), "UPDATE genexprt SET exportname = \"testupdate\" WHERE genexprtid = 2");
////}
//
//String^ Sql::convertString(std::string s)
//{
//	String^ cs = marshal_as<String^>(s);
//	return cs;
//}