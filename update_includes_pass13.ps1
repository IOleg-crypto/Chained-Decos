# PowerShell script - PASS 13: Fix final Engine/Interfaces path
$replacements = @{
    '"Engine/Interfaces/'      = '"platform/windows/Interfaces/'
    '<Engine/Interfaces/'      = '<platform/windows/Interfaces/'
}

$files = Get-ChildItem -Path @(
    "project"
) -Include *.h,*.cpp,*.hpp -Recurse -ErrorAction SilentlyContinue

$modifiedFiles = 0

Write-Host "Pass 13: Fixing Engine/Interfaces paths..."

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

Write-Host "`nPass 13 Done! Modified $modifiedFiles files."
