#ifndef CH_LAYER_STACK_H
#define CH_LAYER_STACK_H

#include "layer.h"
#include <memory>
#include <vector>

namespace CHEngine
{
class LayerStack
{
public:
    using LayerStorage = std::vector<std::unique_ptr<Layer>>;

    LayerStack() = default;
    ~LayerStack();

    void Shutdown();

    // Preferred ownership-aware API.
    void PushLayer(std::unique_ptr<Layer> layer);
    void PushOverlay(std::unique_ptr<Layer> overlay);

    // Backward-compatible API; takes ownership of raw pointer.
    void PushLayer(Layer* layer);
    void PushOverlay(Layer* overlay);
    void PopLayer(Layer* layer);
    void PopOverlay(Layer* overlay);

    bool HasLayer(const std::string& name) const
    {
        for (const auto& layer : m_Layers)
        {
            if (layer && layer->GetName() == name)
            {
                return true;
            }
        }
        return false;
    }

public:
    LayerStorage::iterator begin()
    {
        return m_Layers.begin();
    }
    LayerStorage::iterator end()
    {
        return m_Layers.end();
    }
    LayerStorage::reverse_iterator rbegin()
    {
        return m_Layers.rbegin();
    }
    LayerStorage::reverse_iterator rend()
    {
        return m_Layers.rend();
    }

    LayerStorage& GetLayers()
    {
        return m_Layers;
    }

    std::vector<Layer*> GetLayerPointersSnapshot() const;

private:
    LayerStorage m_Layers;
    unsigned int m_LayerInsertIndex = 0;
};
} // namespace CHEngine

#endif // CH_LAYER_STACK_H
