#ifndef CH_GEOMETRY_FACTORY_H
#define CH_GEOMETRY_FACTORY_H

#include "engine/common/base.h"
#include "engine/graphics/api/vertex_array.h"
#include <memory>

namespace Chained
{

	/// @brief Owns shared GPU geometry used across the rendering pipeline.
	/// Centralizes creation and lifecycle of common quads (fullscreen, billboard/sprite)
	/// so that Renderer doesn't need to manage them inline.
	class CH_API GeometryFactory
	{
	public:
		GeometryFactory() = default;
		~GeometryFactory() = default;

		/// @brief Create the fullscreen quad and billboard/sprite quad VAOs.
		void Initialize();

		/// @brief Release all GPU resources.
		void Shutdown();

		/// @brief Fullscreen quad covering the entire screen (position + UV only).
		std::shared_ptr<VertexArray>& GetFullscreenQuad()
		{
			return m_FullscreenQuadVAO;
		}

		/// @brief Centered unit quad with position + UV + normal (used by Billboard and Sprite).
		std::shared_ptr<VertexArray>& GetQuad()
		{
			return m_QuadVAO;
		}

		bool HasQuad() const
		{
			return m_QuadVAO != nullptr;
		}

	private:
		std::shared_ptr<VertexArray> m_FullscreenQuadVAO;
		std::shared_ptr<VertexArray> m_QuadVAO;
	};

} // namespace Chained

#endif // CH_GEOMETRY_FACTORY_H
