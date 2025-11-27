#include "core/events/EventBus.h"
#include "core/events/GameEvents.h"
#include <iostream>

// Приклад використання Event Bus

// Сервіс 1: Player (публікує події)
class Player
{
    Vector3 m_position = {0, 0, 0};

public:
    void Move(Vector3 newPos)
    {
        Vector3 oldPos = m_position;
        m_position = newPos;

        // Публікуємо подію - не знаємо хто слухає!
        EventBus::Instance().Publish(PlayerMovedEvent{
            .oldPosition = oldPos,
            .newPosition = newPos,
            .velocity = {newPos.x - oldPos.x, newPos.y - oldPos.y, newPos.z - oldPos.z}});
    }
};

// Сервіс 2: Camera (слухає події)
class Camera
{
    Vector3 m_target = {0, 0, 0};
    int m_subscriptionId = -1;

public:
    void Initialize()
    {
        // Підписуємось на рух гравця
        m_subscriptionId = EventBus::Instance().Subscribe<PlayerMovedEvent>(
            [this](const PlayerMovedEvent &event)
            {
                // Оновлюємо камеру
                m_target = event.newPosition;
                std::cout << "Camera following player to: " << event.newPosition.x << ", "
                          << event.newPosition.y << ", " << event.newPosition.z << std::endl;
            });
    }

    void Shutdown()
    {
        EventBus::Instance().Unsubscribe(m_subscriptionId);
    }
};

// Сервіс 3: AudioManager (також слухає події)
class AudioManager
{
    int m_subscriptionId = -1;

public:
    void Initialize()
    {
        // Підписуємось на той самий event
        m_subscriptionId = EventBus::Instance().Subscribe<PlayerMovedEvent>(
            [this](const PlayerMovedEvent &event)
            {
                // Оновлюємо позицію слухача
                std::cout << "Audio listener moved to: " << event.newPosition.x << ", "
                          << event.newPosition.y << ", " << event.newPosition.z << std::endl;
            });
    }

    void Shutdown()
    {
        EventBus::Instance().Unsubscribe(m_subscriptionId);
    }
};

// Приклад використання
int main()
{
    Player player;
    Camera camera;
    AudioManager audio;

    // Ініціалізація - підписка на події
    camera.Initialize();
    audio.Initialize();

    // Гравець рухається - автоматично повідомляє всіх
    player.Move({10, 0, 5});
    // Виведе:
    // Camera following player to: 10, 0, 5
    // Audio listener moved to: 10, 0, 5

    player.Move({20, 0, 10});
    // Виведе:
    // Camera following player to: 20, 0, 10
    // Audio listener moved to: 20, 0, 10

    // Cleanup
    camera.Shutdown();
    audio.Shutdown();

    return 0;
}
