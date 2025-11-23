# PowerShell script - PASS 14: Fix CollisionStructures path
$replacements = @{
    '"Collision/Structures/'      = '"servers/physics/collision/Structures/'
    '<Collision/Structures/'      = '<servers/physics/collision/Structures/'
}

$files = Get-ChildItem -Path @(
    "project",
    "scene"
) -Include *.h,*.cpp,*.hpp -Recurse -ErrorAction SilentlyContinue

$modifiedFiles = 0

Write-Host "Pass 14: Fixing Collision/Structures paths..."

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

Write-Host "`nPass 14 Done! Modified $modifiedFiles files."
