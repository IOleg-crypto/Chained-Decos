$ProjectName = "ChainedDecos"
$BuildDir = "build/bin"
$DistDir = "Dist/$ProjectName"

# Clean previous build
if (Test-Path $DistDir) {
    Remove-Item -Recurse -Force $DistDir
}
New-Item -ItemType Directory -Force -Path $DistDir | Out-Null

# Copy Game Executable (ChainedDecosGame.exe has scripts statically linked)
Copy-Item "$BuildDir/ChainedDecosGame.exe" -Destination "$DistDir/$ProjectName.exe"
Copy-Item "$BuildDir/libengine.dll" -Destination "$DistDir/"

# Copy Game DLL if exists (for C# or native script binding if dynamic)
if (Test-Path "$BuildDir/libChainedDecosGame.dll") {
    Copy-Item "$BuildDir/libChainedDecosGame.dll" -Destination "$DistDir/"
}

# Copy Project File
Copy-Item "game/chaineddecos/$ProjectName.chproject" -Destination "$DistDir/"

# Copy Assets
Copy-Item -Recurse "game/chaineddecos/assets" -Destination "$DistDir/"

# Copy Engine Resources (Shaders, Icons, etc.)
# We need to preserve the folder structure "engine/resources" relative to the runtime for now
# based on Project::Discover logic or AssetManager defaults.
New-Item -ItemType Directory -Force -Path "$DistDir/engine" | Out-Null
Copy-Item -Recurse "engine/resources" -Destination "$DistDir/engine/"

Write-Host "Distribution build created at $DistDir"
