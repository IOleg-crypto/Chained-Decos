#include "core/macros.h"
#include "core/object/Object.h"
#include <iostream>

// Test class using macros
class TestObject : public Object
{
    REGISTER_CLASS(TestObject, Object)
    DISABLE_COPY_AND_MOVE(TestObject)

public:
    TestObject() = default;
    ~TestObject() = default;

    // Using PROPERTY macro
    PROPERTY(int, Health)
    PROPERTY(float, Speed)
};

int main()
{
    TestObject obj;

    // Test type info
    std::cout << "Class name: " << obj.GetClassName() << "\n";

    // Test name property
    obj.SetName("TestObject1");
    std::cout << "Object name: " << obj.GetName() << "\n";

    // Test PROPERTY macro
    obj.SetHealth(100);
    obj.SetSpeed(5.5f);

    std::cout << "Health: " << obj.GetHealth() << "\n";
    std::cout << "Speed: " << obj.GetSpeed() << "\n";

    std::cout << "\n All macros working correctly!\n";

    return 0;
}
