@echo off
setlocal

REM SQLite CLI example
sqlbot.exe --mcp=mcp://127.0.0.1:8080 --db=sqlite:sample.db "Show all customers from Germany"

REM SQL Server CLI example
sqlbot.exe --mcp=mcp://127.0.0.1:8080 --db=sqlserver:"Driver={ODBC Driver 18 for SQL Server};Server=tcp:localhost,1433;Database=Sales;Encrypt=yes;TrustServerCertificate=no;UID=sa;******" "Top 5 products by revenue"

REM GUI mode
sqlbot.exe --gui --mcp=mcp://127.0.0.1:8080 --db=sqlite:sample.db

endlocal
