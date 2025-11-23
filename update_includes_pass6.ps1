# PowerShell script - PASS 6: Fix specific remaining paths
$replacements = @{
    # Collision paths without prefix
    '"Collision/Core/'         = '"servers/physics/collision/Core/'
    '<Collision/Core/'         = '<servers/physics/collision/Core/'
    
    # Physics paths without prefix  
    '"Physics/Components/'     = '"servers/physics/dynamics/Components/'
    '<Physics/Components/'     = '<servers/physics/dynamics/Components/'
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

Write-Host "Pass 6: Fixing specific remaining paths..."

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

Write-Host "`nPass 6 Done! Modified $modifiedFiles files."
