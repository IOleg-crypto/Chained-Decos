#ifndef CH_LAYER_STACK_H
#define CH_LAYER_STACK_H

#include "layer.h"
#include <vector>

namespace CH
{
class LayerStack
{
public:
    LayerStack() = default;
    ~LayerStack();

    void PushLayer(Layer *layer);
    void PushOverlay(Layer *overlay);
    void PopLayer(Layer *layer);
    void PopOverlay(Layer *overlay);

public:
    std::vector<Layer *>::iterator begin()
    {
        return m_Layers.begin();
    }
    std::vector<Layer *>::iterator end()
    {
        return m_Layers.end();
    }
    std::vector<Layer *>::reverse_iterator rbegin()
    {
        return m_Layers.rbegin();
    }
    std::vector<Layer *>::reverse_iterator rend()
    {
        return m_Layers.rend();
    }

    std::vector<Layer *> &GetLayers()
    {
        return m_Layers;
    }

private:
    std::vector<Layer *> m_Layers;
    unsigned int m_LayerInsertIndex = 0;
};
} // namespace CH

#endif // CH_LAYER_STACK_H
