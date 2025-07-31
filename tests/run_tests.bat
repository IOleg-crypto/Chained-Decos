@echo off
echo ========================================
echo Running Chained Decos Engine Tests
echo ========================================

REM Перевіряємо чи існує build директорія
if not exist "..\build" (
    echo Creating build directory...
    mkdir "..\build"
)

REM Переходимо в build директорію
cd "..\build"

REM Генеруємо проект
echo Generating project with CMake...
cmake ..

REM Компілюємо
echo Compiling tests...
cmake --build . --config Debug

REM Запускаємо всі тести
echo.
echo ========================================
echo Running all tests...
echo ========================================
ctest --verbose

echo.
echo ========================================
echo Running specific test suites...
echo ========================================

REM Запускаємо окремі групи тестів
echo Running ColorParser tests...
ctest -R ColorParserTests --verbose

echo Running Player tests...
ctest -R PlayerTests --verbose

echo Running Engine tests...
ctest -R EngineTests --verbose

echo Running Integration tests...
ctest -R IntegrationTests --verbose

echo Running Performance tests...
ctest -R PerformanceTests --verbose

echo.
echo ========================================
echo Test execution completed!
echo ========================================

pause 