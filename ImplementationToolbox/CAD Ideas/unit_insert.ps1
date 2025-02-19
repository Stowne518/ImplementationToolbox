# Define your file path and SQL Server details
$File = "C:\Path\To\Your\Excel\File.xlsx"
$Instance = "YourSQLInstance"
$Database = "YourTargetDatabase"
 
# Extract the file name without extension
$fileName = [System.IO.Path]::GetFileNameWithoutExtension($File)
 
# Load Excel data into a DataTable
$excelData = Import-Excel -Path $File -WorksheetName "YourSheetName" | ConvertTo-DbaDataTable
 
# Establish a SQL connection
$connectionString = "Server=$Instance;Database=$Database;Integrated Security=True"
$connection = New-Object System.Data.SqlClient.SqlConnection
$connection.ConnectionString = $connectionString
 
# Bulk insert the DataTable into a SQL table
$bulkCopy = New-Object Data.SqlClient.SqlBulkCopy($connection)
$bulkCopy.DestinationTableName = "YourTargetTable"
$bulkCopy.WriteToServer($excelData)
 
# Close the connection
$connection.Close()