# reforger-rpc

Windows-only C++ version of the local bridge between Arma Reforger and Discord Rich Presence.

- Author: Matheus (ffx64)
- Version: 1.0.0
- License: Personal and non-commercial use only; modification, redistribution, and monetization require authorization.

## Build

### Dependencies

This project uses vcpkg dependencies:

- Crow
- nlohmann/json

If vcpkg is not installed, run this from PowerShell:

```powershell
git clone https://github.com/microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat
.\vcpkg\vcpkg.exe integrate install
```

Visual Studio/MSBuild installs the packages declared in `vcpkg.json` for
`x64-windows` when building with manifest mode enabled.

### Visual Studio

Open `reforger-rpc.sln` in Visual Studio and build either `Debug|x64` or `Release|x64`.

## Usage

```bat
reforger-rpc.exe [-debug] [-serve] [-port 7453] [-version] [-author] [-license] [-about]
```

Endpoints:

- `POST /endpoint/update`
- `POST /endpoint/close`

The local server starts when `ArmaReforgerSteam.exe` is detected. With `-debug`, it monitors `ArmaReforgerSteamDiag.exe`.

Use `-serve` to start the HTTP server immediately without waiting for the game process. This is useful for local testing.

## Test Request

With Discord open and `reforger-rpc.exe` running, send a test update:

```powershell
.\reforger-rpc.exe -serve
$body = @{
  appId = "YOUR_DISCORD_APPLICATION_ID"
  details = "Playing Arma Reforger"
  state = "In Game"
  largeImageKey = "arma_reforger"
} | ConvertTo-Json

Invoke-RestMethod -Method Post -Uri http://127.0.0.1:7453/endpoint/update -ContentType "application/json" -Body $body
```

To close the RPC session after the update:

```powershell
Invoke-RestMethod -Method Post -Uri http://127.0.0.1:7453/endpoint/close
```
