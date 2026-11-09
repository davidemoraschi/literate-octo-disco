# Build instructions

## Prerequisites
- Windows 10/11 x64
- Visual Studio Community 2022 (MSVC) or clang-cl toolchain with Windows SDK
- CMake 3.20+

## Configure and build
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Optional native SQLite support
To enable SQLite connector implementation, provide sqlite3 static headers/libs and configure:
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DSQLBOT_ENABLE_SQLITE3=ON
```

## Binary
- Output executable: `build/sqlbot.exe` (Windows)
- Runtime modes:
  - CLI: `sqlbot.exe --db=sqlite:test.db "Top 5 products by revenue"`
  - GUI: `sqlbot.exe --gui --db=sqlserver:Driver={ODBC Driver 18 for SQL Server};Server=tcp:localhost,1433;Encrypt=yes;TrustServerCertificate=no;UID=user;******

## MCP version negotiation
- Client starts with `MCP/1.1 IDENTIFY`
- Falls back automatically if server replies with `MCP/1.0 READY`

## Security defaults
- 30 second network timeout default (`--timeout=<seconds>`)
- Multi-statement SQL is rejected by default
- TLS endpoint usage can be intentionally downgraded with `--insecure`
