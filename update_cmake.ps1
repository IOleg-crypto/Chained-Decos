# PowerShell script to update all CMakeLists.txt files
$cmakeFiles = Get-ChildItem -Path @(
    "servers/audio",
    "servers/input", 
    "servers/physics/collision",
    "servers/physics/dynamics",
    "scene/main",
    "scene/resources/model",
    "scene/resources/map",
    "scene/3d/camera",
    "scene/gui",
    "modules/chained_decos"
) -Filter "CMakeLists.txt" -ErrorAction SilentlyContinue

foreach ($file in $cmakeFiles) {
    $content = Get-Content $file.FullName -Raw
    
    # Add CMAKE_SOURCE_DIR if target_include_directories exists but doesn't have it
    if ($content -match 'target_include_directories' -and $content -notmatch '\$\{CMAKE_SOURCE_DIR\}') {
        # Find the target_include_directories block and add CMAKE_SOURCE_DIR
        $content = $content -replace '(target_include_directories\([^\)]+\s+PUBLIC\s*)', "`$1`n        `${CMAKE_SOURCE_DIR}`n        "
        Set-Content -Path $file.FullName -Value $content -NoNewline
        Write-Host "Updated: $($file.FullName)"
    }
}

Write-Host "Done updating CMakeLists.txt files!"
