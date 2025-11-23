# PowerShell script - PASS 8: Fix remaining legacy paths after src/ removal
$replacements = @{
    # Engine.h is gone - need to check what needs it
    '"Engine.h"'                = '"platform/windows/Core/EngineApplication.h"'
    '<Engine.h>'                = '<platform/windows/Core/EngineApplication.h>'
    
    # Color parser moved to scene/resources
    '"src/Engine/Color/Parser/' = '"scene/resources/color/'
    '<src/Engine/Color/Parser/' = '<scene/resources/color/'
    
    # Render interfaces
    '"Render/Interfaces/'       = '"servers/rendering/Interfaces/'
    '<Render/Interfaces/'       = '<servers/rendering/Interfaces/'
}

$files = Get-ChildItem -Path @(
    "core",
    "servers", 
    "scene",
    "project",
    "editor",
    "platform"
) -Include *.h,*.cpp,*.hpp -Recurse -ErrorAction SilentlyContinue

$modifiedFiles = 0

Write-Host "Pass 8: Fixing paths after src/ removal..."

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

Write-Host "`nPass 8 Done! Modified $modifiedFiles files."
