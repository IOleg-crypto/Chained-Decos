# PowerShell script - PASS 4: Fix ALL remaining legacy paths
$replacements = @{
    # Vector/Math paths
    '"Vector3.h"'              = '"core/math/Vector/Vector3.h"'
    '"Matrix4x4.h"'            = '"core/math/Matrix/Matrix4x4.h"'
    '<Vector3.h>'              = '<core/math/Vector/Vector3.h>'
    '<Matrix4x4.h>'            = '<core/math/Matrix/Matrix4x4.h>'
    
    # Model paths
    '"Model/Config/'           = '"scene/resources/model/Config/'
    '"Model/Parser/'           = '"scene/resources/model/Parser/'
    '"Model/Utils/'            = '"scene/resources/model/Utils/'
    '<Model/Config/'           = '<scene/resources/model/Config/'
    '<Model/Parser/'           = '<scene/resources/model/Parser/'
    
    # Map paths  
    '"MapFileManager/'         = '"scene/resources/map/MapFileManager/'
    '"Map/Converter/'          = '"scene/resources/map/Converter/'
    '"Map/Renderer/'           = '"scene/resources/map/Renderer/'
    '"Map/Skybox/'             = '"scene/resources/map/Skybox/'
    
    # Color paths (stays in src/Engine for now)
    '"../../Color/Parser/'     = '"src/Engine/Color/Parser/'
    '<../../Color/Parser/'     = '<src/Engine/Color/Parser/'
    '"Color/Parser/'           = '"src/Engine/Color/Parser/'
    '<Color/Parser/'           = '<src/Engine/Color/Parser/'
    
    # Physics paths
    '"Physics/Components/'     = '"servers/physics/dynamics/Components/'
    '"../Physics/Components/'  = '"servers/physics/dynamics/Components/'
    
    # Collision paths
    '"Collision/Core/'         = '"servers/physics/collision/Core/'
    
    # World paths
    '"World/Core/'             = '"scene/main/Core/'
    
    # Asset paths (stays in src/Engine)
    '"Asset/'                  = '"src/Engine/Asset/'
    '<Asset/'                  = '<src/Engine/Asset/'
}

$files = Get-ChildItem -Path @(
    "core",
    "servers", 
    "scene",
    "modules",
    "editor",
    "platform"
) -Include *.h,*.cpp,*.hpp -Recurse -ErrorAction SilentlyContinue

$modifiedFiles = 0
$totalFiles = $files.Count

Write-Host "Pass 4: Fixing remaining include paths..."
Write-Host "Found $totalFiles files to process"

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

Write-Host "`nPass 4 Done! Modified $modifiedFiles out of $totalFiles files."
