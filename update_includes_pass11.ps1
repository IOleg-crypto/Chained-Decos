# PowerShell script - PASS 11: Fix remaining paths
$replacements = @{
    '"Game/Menu/Menu.h"'       = '"project/chained_decos/Menu/Menu.h"'
    '<Game/Menu/Menu.h>'       = '<project/chained_decos/Menu/Menu.h>'
}

$files = Get-ChildItem -Path @(
    "project"
) -Include *.h,*.cpp,*.hpp -Recurse -ErrorAction SilentlyContinue

$modifiedFiles = 0

Write-Host "Pass 11: Fixing Game/Menu paths..."

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

Write-Host "`nPass 11 Done! Modified $modifiedFiles files."
