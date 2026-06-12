#include "layer_stack.h"
#include <algorithm>

namespace Chained
{
LayerStack::~LayerStack()
{
    Shutdown();
}

void LayerStack::Shutdown()
{
    for (auto& layer : m_Layers)
    {
        if (layer)
        {
            layer->OnDetach();
        }
    }
    m_Layers.clear();
    m_LayerInsertIndex = 0;
}

void LayerStack::PushLayer(std::unique_ptr<Layer> layer)
{
    m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, std::move(layer));
    m_LayerInsertIndex++;
}

void LayerStack::PushOverlay(std::unique_ptr<Layer> overlay)
{
    m_Layers.emplace_back(std::move(overlay));
}

} // namespace Chained
