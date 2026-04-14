#include "layer_stack.h"
#include <algorithm>

namespace CHEngine
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

void LayerStack::PushLayer(Layer* layer)
{
    PushLayer(std::unique_ptr<Layer>(layer));
}

void LayerStack::PushOverlay(Layer* overlay)
{
    PushOverlay(std::unique_ptr<Layer>(overlay));
}

void LayerStack::PopLayer(Layer* layer)
{
    auto it = std::find_if(m_Layers.begin(), m_Layers.begin() + m_LayerInsertIndex,
                           [layer](const std::unique_ptr<Layer>& candidate) {
                               return candidate.get() == layer;
                           });
    if (it != m_Layers.begin() + m_LayerInsertIndex)
    {
        (*it)->OnDetach();
        m_Layers.erase(it);
        m_LayerInsertIndex--;
    }
}

void LayerStack::PopOverlay(Layer* overlay)
{
    auto it = std::find_if(m_Layers.begin() + m_LayerInsertIndex, m_Layers.end(),
                           [overlay](const std::unique_ptr<Layer>& candidate) {
                               return candidate.get() == overlay;
                           });
    if (it != m_Layers.end())
    {
        (*it)->OnDetach();
        m_Layers.erase(it);
    }
}

std::vector<Layer*> LayerStack::GetLayerPointersSnapshot() const
{
    std::vector<Layer*> layers;
    layers.reserve(m_Layers.size());
    for (const auto& layer : m_Layers)
    {
        layers.emplace_back(layer.get());
    }
    return layers;
}
} // namespace CHEngine
