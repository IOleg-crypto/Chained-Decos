#!/bin/bash

echo "========================================"
echo "Running Chained Decos Engine Tests"
echo "========================================"

# Перевіряємо чи існує build директорія
if [ ! -d "../build" ]; then
    echo "Creating build directory..."
    mkdir -p "../build"
fi

# Переходимо в build директорію
cd "../build"

# Генеруємо проект
echo "Generating project with CMake..."
cmake ..

# Компілюємо
echo "Compiling tests..."
make -j$(nproc)

# Запускаємо всі тести
echo ""
echo "========================================"
echo "Running all tests..."
echo "========================================"
ctest --verbose

echo ""
echo "========================================"
echo "Running specific test suites..."
echo "========================================"

# Запускаємо окремі групи тестів
echo "Running ColorParser tests..."
ctest -R ColorParserTests --verbose

echo "Running Player tests..."
ctest -R PlayerTests --verbose

echo "Running Engine tests..."
ctest -R EngineTests --verbose

echo "Running Integration tests..."
ctest -R IntegrationTests --verbose

echo "Running Performance tests..."
ctest -R PerformanceTests --verbose

echo ""
echo "========================================"
echo "Test execution completed!"
echo "========================================" 