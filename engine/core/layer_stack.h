#ifndef CH_LAYER_STACK_H
#define CH_LAYER_STACK_H

#include "layer.h"
#include <memory>
#include <vector>

namespace Chained
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
		const LayerStorage& GetLayers() const
		{
			return m_Layers;
		}

		size_t GetLayerCount() const
		{
			return m_Layers.size();
		}

		Layer* GetLayerAt(size_t index) const
		{
			return index < m_Layers.size() ? m_Layers[index].get() : nullptr;
		}

	private:
		LayerStorage m_Layers;
		unsigned int m_LayerInsertIndex = 0;
	};
} // namespace Chained

#endif // CH_LAYER_STACK_H
