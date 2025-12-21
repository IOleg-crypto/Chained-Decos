# Engine Tests

This directory contains comprehensive tests for the Chained Decos engine.

## Test Structure

### 1. ColorParser Tests

- **ParseValidColors**: Tests correct parsing of all supported colors
- **ParseInvalidColors**: Tests handling of invalid color names
- **ParseCaseSensitiveColors**: Tests case sensitivity

### 2. Player Tests

- **Constructor**: Tests player creation
- **SpeedOperations**: Tests setting and getting speed
- **Movement**: Tests player movement in 3D space
- **PlayerData**: Tests getting player position data
- **ModelManager**: Tests player model manager interaction
- **Jump**: Tests jump functionality
- **PositionHistory**: Tests position history tracking
- **ApplyInput**: Tests input application
- **LoadModelPlayer**: Tests player model loading

### 3. Models Tests

- **Constructor**: Tests model manager creation
- **LoadModelsFromJson**: Tests loading models from JSON files
- **DrawAllModels**: Tests rendering all models
- **GetModelByName**: Tests model lookup by name
- **AddInstance**: Tests adding model instances

### 4. Engine Tests

- **Constructor**: Tests engine creation (default and parameterized)
- **Initialization**: Tests engine initialization
- **InputInitialization**: Tests input system initialization
- **FontInitialization**: Tests ImGui font initialization
- **KeyboardShortcuts**: Tests keyboard shortcuts
- **Update**: Tests update loop
- **Render**: Tests rendering system
- **DrawScene3D**: Tests 3D scene rendering
- **LoadPlayerModel**: Tests player model loading
- **DebugInfo**: Tests debug information display

### 5. InputManager Tests

- **Constructor**: Tests input manager creation
- **RegisterAction**: Tests action registration
- **ProcessInput**: Tests input processing
- **MultipleActions**: Tests multiple action handling

### 6. Menu Tests

- **Constructor**: Tests menu creation
- **GetAction**: Tests action retrieval
- **ResetAction**: Tests action reset
- **Update**: Tests menu update
- **Render**: Tests menu rendering
- **MenuActionEnum**: Tests menu action enum values

### 7. SceneLoader Tests

- **Constructor**: Tests map loader creation
- **LoadMapNonExistent**: Tests loading non-existent maps
- **LoadMapEmptyFile**: Tests loading empty files
- **SceneLoaderStruct**: Tests map loader structure
- **LoadMapInvalidJson**: Tests invalid JSON handling

### 8. CameraController Tests

- **Constructor**: Tests camera controller creation
- **GetCamera**: Tests camera reference access
- **GetCameraMode**: Tests camera mode retrieval
- **SetCameraMode**: Tests camera mode setting
- **Update**: Tests camera update
- **CameraModeConsistency**: Tests mode setting/getting consistency

### 9. ModelInstance Tests

- **ConstructorWithAllParameters**: Tests full constructor
- **ConstructorWithColor**: Tests constructor with color
- **ConstructorMinimal**: Tests minimal constructor
- **GetProperties**: Tests all getter methods

### 10. Integration Tests

- **EnginePlayerInteraction**: Tests engine-player interaction
- **ColorParsingInContext**: Tests color parsing in context
- **InputManagerMenuInteraction**: Tests input-manu interaction
- **CameraControllerPlayerInteraction**: Tests camera-player interaction
- **ModelInstanceModelsInteraction**: Tests model instance interaction

### 11. Performance Tests

- **ColorParsingSpeed**: Tests color parsing performance
- **InputManagerSpeed**: Tests input manager performance
- **ModelInstanceCreation**: Tests model instance creation speed

### 12. Edge Case Tests

- **EmptyColorName**: Tests empty color name handling
- **VeryLongColorName**: Tests very long color names
- **SpecialCharactersInColorName**: Tests special characters in color names
- **InputManagerEdgeCases**: Tests input manager edge cases
- **MenuEdgeCases**: Tests menu edge cases
- **CameraControllerEdgeCases**: Tests camera controller edge cases
- **ModelInstanceEdgeCases**: Tests model instance edge cases
- **SceneLoaderEdgeCases**: Tests map loader edge cases

### 13. Stress Tests

- **MultipleInputManagers**: Tests multiple input managers
- **MultipleMenus**: Tests multiple menus
- **MultipleCameraControllers**: Tests multiple camera controllers
- **MultipleModelInstances**: Tests multiple model instances

## Running Tests

### Compilation and Execution

```bash
# From project root directory
mkdir build
cd build
cmake ..
make

# Run all tests
./tests/unit_tests
```

### Running Specific Tests

```bash
# Run only ColorParser tests
./tests/unit_tests --gtest_filter="ColorParserTest*"

# Run only Player tests
./tests/unit_tests --gtest_filter="PlayerTest*"

# Run only Engine tests
./tests/unit_tests --gtest_filter="EngineTest*"

# Run only Integration tests
./tests/unit_tests --gtest_filter="IntegrationTest*"

# Run only Performance tests
./tests/unit_tests --gtest_filter="PerformanceTest*"
```

### Running with Verbose Output

```bash
./tests/unit_tests --gtest_verbose
```

### Running with XML Report

```bash
./tests/unit_tests --gtest_output=xml:test_results.xml
```

### Using CMake Test Framework

```bash
# Run all tests via CMake
ctest --verbose

# Run specific test suites
ctest -R ColorParserTests --verbose
ctest -R PlayerTests --verbose
ctest -R EngineTests --verbose
```

## Adding New Tests

### Test Structure

```cpp
TEST(TestSuiteName, TestName) {
    // Arrange - prepare data
    // Act - execute action
    // Assert - verify result
    EXPECT_EQ(expected, actual);
}
```

### Types of Assertions

- `EXPECT_EQ(expected, actual)` - checks equality
- `EXPECT_NE(expected, actual)` - checks inequality
- `EXPECT_GT(val1, val2)` - checks that val1 > val2
- `EXPECT_LT(val1, val2)` - checks that val1 < val2
- `EXPECT_TRUE(condition)` - checks that condition is true
- `EXPECT_FALSE(condition)` - checks that condition is false
- `EXPECT_NO_THROW(statement)` - checks that no exception is thrown
- `EXPECT_ANY_THROW(statement)` - checks that an exception is thrown

## Test Configuration

The `test_config.json` file contains test configuration:

- Test models
- Test colors
- Test scenarios

## Test Coverage

Current tests cover:

- ✅ ColorParser - full coverage
- ✅ Player - core functions
- ✅ Models - basic operations
- ✅ Engine - all public methods
- ✅ InputManager - constructor and methods
- ✅ Menu - constructor and methods
- ✅ SceneLoader - constructor and methods
- ✅ CameraController - constructor and methods
- ✅ ModelInstance - all constructors and methods

## Known Issues

1. Some tests may fail if Raylib is not initialized
2. Tests requiring graphics context may not work in CI/CD
3. Model tests require model files to be present

## Recommendations

1. Run tests before each commit
2. Add tests for new functions
3. Use mocks for test isolation
4. Test edge cases and error conditions
5. Maintain test documentation

## Test Categories

### Unit Tests

- Test individual components in isolation
- Fast execution
- High reliability

### Integration Tests

- Test component interactions
- Verify system behavior
- Catch integration issues

### Performance Tests

- Measure execution time
- Detect performance regressions
- Ensure acceptable performance

### Edge Case Tests

- Test boundary conditions
- Test error handling
- Test invalid inputs

### Stress Tests

- Test with high load
- Test memory usage
- Test system stability

## Continuous Integration

### GitHub Actions Example

```yaml
name: Tests
on: [push, pull_request]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake g++ libasound2-dev
      - name: Build and test
        run: |
          mkdir build && cd build
          cmake ..
          make
          ctest --verbose
```

### Local Development

```bash
# Quick test run
./tests/unit_tests --gtest_filter="*Test*"

# Full test suite
./tests/unit_tests

# Generate coverage report
./tests/unit_tests --gtest_output=xml:coverage.xml
```

## Troubleshooting

### Common Issues

1. **Compilation Errors**

   - Ensure all dependencies are installed
   - Check include paths
   - Verify CMake configuration

2. **Runtime Errors**

   - Check if Raylib is properly initialized
   - Verify file paths for models/textures
   - Check memory allocation

3. **Test Failures**
   - Review test logic
   - Check expected vs actual values
   - Verify test environment

### Debug Tips

1. Use `--gtest_verbose` for detailed output
2. Use `--gtest_break_on_failure` for debugging
3. Use `--gtest_filter` to run specific tests
4. Check test logs for detailed error messages

## Contributing

1. Write tests for new features
2. Update existing tests when changing functionality
3. Follow the established test naming conventions
4. Document complex test scenarios
5. Ensure tests are deterministic and reliable
