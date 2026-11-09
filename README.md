# literate-octo-disco

`sqlbot` is a lightweight C++23 client that converts natural-language prompts into SQL through an MCP server, then executes the SQL against a selected RDBMS.

Supported runtime database targets:
- SQLite (native sqlite3 connector when built with `SQLBOT_ENABLE_SQLITE3=ON`)
- Microsoft SQL Server (Windows ODBC)
- Google BigQuery (connector scaffold with explicit static gRPC/OAuth2 integration point)

See `/home/runner/work/literate-octo-disco/literate-octo-disco/BUILD.md` for build details and `/home/runner/work/literate-octo-disco/literate-octo-disco/example.bat` for usage examples.
