# PowerShell script - PASS 7: Final cleanup of all remaining paths
$replacements = @{
    # World paths
    '"World/Core/'             = '"scene/main/Core/'
    '<World/Core/'             = '<scene/main/Core/'
    
    # ImGui paths
    '"imgui/imgui.h"'          = '"imgui.h"'
    '<imgui/imgui.h>'          = '<imgui.h>'
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

Write-Host "Pass 7: Final cleanup..."

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

Write-Host "`nPass 7 Done! Modified $modifiedFiles files."
Write-Host "`nTotal files updated across all passes: 100+"
