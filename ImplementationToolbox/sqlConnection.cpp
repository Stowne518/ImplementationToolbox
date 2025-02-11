#include "imgui.h"

#include <iostream>
#include <windows.h>
#include <sqlext.h>
#include <sql.h>
#include <string>

void printSQLError(SQLHANDLE handle, SQLSMALLINT handleType) {
    SQLWCHAR sqlState[6], message[SQL_MAX_MESSAGE_LENGTH];
    SQLINTEGER nativeError;
    SQLSMALLINT messageLength;
    SQLRETURN ret;

    int i = 1;
    while ((ret = SQLGetDiagRec(handleType, handle, i, sqlState, &nativeError, message, sizeof(message), &messageLength)) != SQL_NO_DATA) {
        std::cerr << "SQL Error: " << (char*)message << " (SQL State: " << (char*)sqlState << ", Native Error: " << nativeError << ")" << std::endl;
        i++;
    }
}


bool testSQLConnection(char* dataBase, char* serverName, char* userName, char* password) {
    SQLHENV hEnv;
    SQLHDBC hDbc;
    SQLHSTMT hStmt;
    SQLRETURN ret;
    SQLWCHAR outstr[1024];
    SQLSMALLINT outstrlen;
    char* buffer = (char*)malloc(8000);
    sprintf(buffer, "DRIVER={ODBC Driver 17 for SQL Server};SERVER=%s;Database=%s;UID=%s;PWD=%s;", serverName, dataBase, userName, password);


    // Allocate environment handle
    ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    if (ret != SQL_SUCCESS) {
        std::cerr << "Error allocating environment handle" << std::endl;
        return false;
    }

    // Set the ODBC version environment attribute
    ret = SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
    if (ret != SQL_SUCCESS) {
        std::cerr << "Error setting ODBC version" << std::endl;
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        return false;
    }

    // Allocate connection handle
    ret = SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
    if (ret != SQL_SUCCESS) {
        std::cerr << "Error allocating connection handle" << std::endl;
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        return false;
    }

    // Set connection string
    ret = SQLDriverConnect(hDbc, NULL, (SQLWCHAR*)(buffer), SQL_NTS, outstr, sizeof(outstr), &outstrlen, SQL_DRIVER_NOPROMPT);
    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
        std::cout << "Connected to the database successfully!" << std::endl;
        return true;
    }
    else {
        std::cerr << "Failed to connect to the database" << std::endl;
        printSQLError(hDbc, SQL_HANDLE_DBC);
        return false;
    }

    // Disconnect and free handles
    SQLDisconnect(hDbc);
    SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
    SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
}
