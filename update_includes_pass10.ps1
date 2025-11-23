# PowerShell script - PASS 10: Fix Engine/Engine.h references
$replacements = @{
    '"Engine/Engine.h"'        = '"platform/windows/Core/EngineApplication.h"'
    '<Engine/Engine.h>'        = '<platform/windows/Core/EngineApplication.h>'
}

$files = Get-ChildItem -Path @(
    "project"
) -Include *.h,*.cpp,*.hpp -Recurse -ErrorAction SilentlyContinue

$modifiedFiles = 0

Write-Host "Pass 10: Fixing Engine/Engine.h references..."

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

Write-Host "`nPass 10 Done! Modified $modifiedFiles files."
