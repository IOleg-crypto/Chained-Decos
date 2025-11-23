# PowerShell script to update all references from modules/chained_decos to project
$replacements = @{
    'modules/chained_decos'    = 'project'
    'modules\\chained_decos'   = 'project'
}

$files = Get-ChildItem -Path @(
    "core",
    "servers", 
    "scene",
    "project",
    "editor",
    "platform"
) -Include *.h,*.cpp,*.hpp,CMakeLists.txt -Recurse -ErrorAction SilentlyContinue

$modifiedFiles = 0

Write-Host "Updating references from modules/chained_decos to project..."

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

Write-Host "`nDone! Modified $modifiedFiles files."
