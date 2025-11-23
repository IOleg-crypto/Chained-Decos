# PowerShell script - PASS 5: Fix final legacy paths
$replacements = @{
    # CameraController
    '"CameraController/Core/'  = '"scene/3d/camera/Core/'
    '<CameraController/Core/'  = '<scene/3d/camera/Core/'
    
    # Collision paths (more specific)
    '"Collision/System/'       = '"servers/physics/collision/System/'
    '"Collision/Debug/'        = '"servers/physics/collision/Debug/'
    '"Collision/Structures/'   = '"servers/physics/collision/Structures/'
    '<Collision/System/'       = '<servers/physics/collision/System/'
    '<Collision/Debug/'        = '<servers/physics/collision/Debug/'
    
    # MapFileManager - fix the path
    '"scene/resources/map/MapFileManager/' = '"scene/resources/map/MapFileManager/'
    
    # Add core to include raylib for ConfigManager
    # This will be handled via CMake
}

$files = Get-ChildItem -Path @(
    "core",
    "servers", 
    "scene",
    "modules",
    "editor",
    "platform",
    "src"
) -Include *.h,*.cpp,*.hpp -Recurse -ErrorAction SilentlyContinue

$modifiedFiles = 0

Write-Host "Pass 5: Fixing final include paths..."

foreach ($file in $files) {
    $content = Get-Content $file.FullName -Raw -ErrorAction SilentlyContinue
    if ($null -eq $content) { continue }
    
    $originalContent = $content
    
    foreach ($old in $replacements.Keys) {
        $new = $replacements[$old]
        $content = $content -replace [regex]::Escape($old), $new
    }
    
    if ($content -ne $originalContent) {
        Set-Content -Path $file.FullName -Value $content -NoNewline
        $modifiedFiles++
        Write-Host "Modified: $($file.FullName)"
    }
}

Write-Host "`nPass 5 Done! Modified $modifiedFiles files."
