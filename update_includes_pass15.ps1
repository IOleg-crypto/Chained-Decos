# PowerShell script - PASS 15: Fix Game/Systems path
$replacements = @{
    '"Game/Systems/'      = '"project/chained_decos/Systems/'
    '<Game/Systems/'      = '<project/chained_decos/Systems/'
}

$files = Get-ChildItem -Path @(
    "project"
) -Include *.h,*.cpp,*.hpp -Recurse -ErrorAction SilentlyContinue

$modifiedFiles = 0

Write-Host "Pass 15: Fixing Game/Systems paths..."

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

Write-Host "`nPass 15 Done! Modified $modifiedFiles files."
