# PowerShell script - PASS 9: Final fixes for build errors
$replacements = @{
    # CommandLineHandler doesn't exist anymore - remove or comment out
    '"src/Engine/CommandLineHandler/' = '"// REMOVED: src/Engine/CommandLineHandler/'
    
    # EngineApplication path was wrong
    '"platform/windows/Core/EngineApplication.h"' = '"core/object/kernel/Core/Kernel.h"'
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

Write-Host "Pass 9: Final build error fixes..."

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

Write-Host "`nPass 9 Done! Modified $modifiedFiles files."
