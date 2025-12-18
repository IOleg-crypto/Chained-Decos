#include "GameLayer.h"
#include <raylib.h>

GameLayer::GameLayer() : Layer("GameLayer")
{
}

void GameLayer::OnAttach()
{
    TraceLog(LOG_INFO, "GameLayer Attached");
}

void GameLayer::OnDetach()
{
    TraceLog(LOG_INFO, "GameLayer Detached");
}

void GameLayer::OnUpdate(float deltaTime)
{
    // Game logic would go here
}

void GameLayer::OnRender()
{
    // Game rendering would go here
}
