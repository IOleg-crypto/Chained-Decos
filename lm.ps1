# ==========================
# Megallm Runner for PowerShell
# ==========================

# 1️⃣ Set your Anthropic API key here
$AnthropicKey = "sk-mega-bdcfea1538c733d7385698a027b85b03aba0b73dc6064c5c8ac016308c069678"

# 2️⃣ Export as environment variable for current session
$env:ANTHROPIC_API_KEY = $AnthropicKey

# 3️⃣ Prompt user for input
$prompt = Read-Host "Enter your prompt"

# 4️⃣ Try running Claude via Megallm
try {
    Write-Host "Trying Claude API..." -ForegroundColor Cyan
    $claudeOutput = npx megallm run --model claude --input "$prompt" 2>&1

    if ($claudeOutput -match "Credit balance too low|invalid x-api-key") {
        throw "Claude API unavailable or invalid key"
    }

    Write-Host "`n✅ Claude output:`" -ForegroundColor Green
    Write-Host $claudeOutput
}
catch {
    Write-Host "`n⚠ Claude unavailable: $_" -ForegroundColor Yellow
    Write-Host "Falling back to local model gpt4all..." -ForegroundColor Cyan

    try {
        $localOutput = npx megallm run --model gpt4all --input "$prompt" 2>&1
        Write-Host "`n✅ Local model output:`" -ForegroundColor Green
        Write-Host $localOutput
    }
    catch {
        Write-Host "`n❌ Failed to run local model: $_" -ForegroundColor Red
    }
}
