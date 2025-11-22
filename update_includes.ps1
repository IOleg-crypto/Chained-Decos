# PowerShell script to update include paths
$replacements = @{
    # Engine → servers/core
    '"Engine/Audio/'           = '"servers/audio/'
    '"Engine/Input/'           = '"servers/input/'
    '"Engine/Render/'          = '"servers/rendering/'
    '"Engine/Collision/'       = '"servers/physics/collision/'
    '"Engine/Physics/'         = '"servers/physics/dynamics/'
    
    # Engine → core
    '"Engine/Math/'            = '"core/math/'
    '"Engine/Config/'          = '"core/config/'
    '"Engine/Event/'           = '"core/object/event/'
    '"Engine/Kernel/'          = '"core/object/kernel/'
    '"Engine/Module/'          = '"core/object/module/'
    
    # Engine → scene
    '"Engine/World/'           = '"scene/main/'
    '"Engine/Model/'           = '"scene/resources/model/'
    '"Engine/Map/'             = '"scene/resources/map/'
    '"Engine/CameraController/' = '"scene/3d/camera/'
    '"Engine/GUI/'             = '"scene/gui/'
    
    # Angle brackets
    '<Engine/Audio/'           = '<servers/audio/'
    '<Engine/Input/'           = '<servers/input/'
    '<Engine/Render/'          = '<servers/rendering/'
    '<Engine/Collision/'       = '<servers/physics/collision/'
    '<Engine/Physics/'         = '<servers/physics/dynamics/'
    '<Engine/Math/'            = '<core/math/'
    '<Engine/Config/'          = '<core/config/'
    '<Engine/Event/'           = '<core/object/event/'
    '<Engine/Kernel/'          = '<core/object/kernel/'
    '<Engine/Module/'          = '<core/object/module/'
    '<Engine/World/'           = '<scene/main/'
    '<Engine/Model/'           = '<scene/resources/model/'
    '<Engine/Map/'             = '<scene/resources/map/'
    '<Engine/CameraController/' = '<scene/3d/camera/'
    '<Engine/GUI/'             = '<scene/gui/'
    
    # Direct references without Engine prefix
    '"Model/Core/'             = '"scene/resources/model/Core/'
    '"Map/'                    = '"scene/resources/map/'
    '<Model/Core/'             = '<scene/resources/model/Core/'
    
    # Relative paths
    '"../../Kernel/'           = '"core/object/kernel/'
    '"../Kernel/'              = '"core/object/kernel/'
}

# Get all C++ files in new directories
$files = Get-ChildItem -Path @(
    "core",
    "servers", 
    "scene",
    "modules",
    "editor",
    "platform"
) -Include *.h,*.cpp,*.hpp -Recurse -ErrorAction SilentlyContinue

$totalFiles = $files.Count
$processedFiles = 0
$modifiedFiles = 0

Write-Host "Found $totalFiles files to process..."

foreach ($file in $files) {
    $processedFiles++
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
        Write-Host "[$processedFiles/$totalFiles] Modified: $($file.FullName)"
    } else {
        Write-Host "[$processedFiles/$totalFiles] Unchanged: $($file.FullName)"
    }
}

Write-Host "`nDone! Modified $modifiedFiles out of $totalFiles files."
