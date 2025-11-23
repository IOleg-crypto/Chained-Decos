# PowerShell script - PASS 12: Fix final Console path
$replacements = @{
    '"Game/Menu/Console/'      = '"project/chained_decos/Menu/Console/'
    '<Game/Menu/Console/'      = '<project/chained_decos/Menu/Console/'
}

$files = Get-ChildItem -Path @(
    "project"
) -Include *.h,*.cpp,*.hpp -Recurse -ErrorAction SilentlyContinue

$modifiedFiles = 0

Write-Host "Pass 12: Fixing Game/Menu/Console paths..."

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

Write-Host "`nPass 12 Done! Modified $modifiedFiles files."
