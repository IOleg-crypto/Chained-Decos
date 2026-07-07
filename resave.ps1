Add-Type -AssemblyName System.Drawing
try {
    $img = [System.Drawing.Image]::FromFile("D:\gitnext\Chained Decos\game\chaineddecos\assets\materials\300px-Gamelogo.jpg")
    $img.Save("D:\gitnext\Chained Decos\game\chaineddecos\assets\materials\300px-Gamelogo_fixed.png", [System.Drawing.Imaging.ImageFormat]::Png)
    $img.Dispose()
    Write-Host "Success"
} catch {
    Write-Host "Error: $_"
}
