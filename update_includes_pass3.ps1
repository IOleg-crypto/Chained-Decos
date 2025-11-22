# PowerShell script - PASS 3: Fix ALL remaining paths including src/
$replacements = @{
    # Model/Map paths
    '<Model/Parser/'           = '<scene/resources/model/Parser/'
    '<Model/Core/'             = '<scene/resources/model/Core/'
    '<Map/Core/'               = '<scene/resources/map/Core/'
    '<Color/Parser/'           = '<src/Engine/Color/Parser/'
    
    # Game paths
    '"Game/Player/'            = '"modules/chained_decos/Player/'
    '"Menu/'                   = '"modules/chained_decos/Menu/'
    
    # Engine paths in src/
    '"Engine/CommandLineHandler/' = '"src/Engine/CommandLineHandler/'
    '"Editor/Editor.h"'        = '"editor/plugins/map_editor/Editor/Editor.h"'
    
    # Relative Event paths
    '"../Event/Core/'          = '"core/object/event/Core/'
    '"Event.h"'                = '"core/object/event/Core/Event.h"'
    '"EventDispatcher.h"'      = '"core/object/event/Core/EventDispatcher.h"'
    '"../Core/Event.h"'        = '"core/object/event/Core/Event.h"'
    
    # Application paths
    '"EngineApplication.h"'    = '"platform/windows/Core/EngineApplication.h"'
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
    }
}

Write-Host "Pass 3 Done! Modified $modifiedFiles files."
