# PowerShell script to update include paths - PASS 2
# This time also update files in src/ directory

$replacements = @{
    # Quotes
    '"Input/Core/'             = '"servers/input/Core/'
    '"Render/Core/'            = '"servers/rendering/Core/'
    '"Render/Utils/'           = '"servers/rendering/Utils/'
    '"Render/Shader/'          = '"servers/rendering/Shader/'
    '"Collision/Core/'         = '"servers/physics/collision/Core/'
    '"Audio/Core/'             = '"servers/audio/Core/'
    '"Kernel/Core/'            = '"core/object/kernel/Core/'
    '"Module/Core/'            = '"core/object/module/Core/'
    '"Event/Core/'             = '"core/object/event/Core/'
    '"Config/Core/'            = '"core/config/Core/'
    '"Math/'                   = '"core/math/'
    '"World/Core/'             = '"scene/main/Core/'
    '"CameraController/Core/'  = '"scene/3d/camera/Core/'
    
    # Relative paths that are still wrong
    '"../../MapFileManager/'   = '"MapFileManager/'
}

# Get all C++ files including src/
$files = Get-ChildItem -Path @(
    "core",
    "servers", 
    "scene",
    "modules",
    "editor",
    "platform",
    "src"
) -Include *.h,*.cpp,*.hpp -Recurse -ErrorAction SilentlyContinue

$totalFiles = $files.Count
$processedFiles = 0
$modifiedFiles = 0

Write-Host "Found $totalFiles files to process (Pass 2)..."

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
    }
}

Write-Host "`nPass 2 Done! Modified $modifiedFiles out of $totalFiles files."
