# Patch to fix GLFW window creation crash
# This script comments out the GLFW monitor detection code in Menu constructor
# which was causing "Assertion failed: window != NULL" error

$file = "project\chaineddecos\Menu\Menu.cpp"
$content = Get-Content $file -Raw

# Replace the GLFW monitor detection block with a comment
$content = $content -replace '(?s)    // Get available monitor resolutions using GLFW\r\n    GLFWmonitor \*monitor = glfwGetPrimaryMonitor\(\);.*?    \}', @'
    // DISABLED: GLFW monitor detection moved to avoid crash before window creation
    // TODO: Re-enable this after window is created, possibly in Initialize()
    /*
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    if (monitor != nullptr)
    {
        int modesCount = 0;
        const GLFWvidmode *modes = glfwGetVideoModes(monitor, &modesCount);

        if (modes != nullptr && modesCount > 0)
        {
            for (int i = 0; i < modesCount; ++i)
            {
                resolutionSet.insert(std::to_string(modes[i].width) + "x" +
                                     std::to_string(modes[i].height));
            }
        }
    }
    */'@

$content | Set-Content $file -NoNewline
Write-Host "Patch applied successfully"
