#ifndef CH_VIEWPORT_ICONS_H
#define CH_VIEWPORT_ICONS_H

#include <cstdint>
#include <memory>
#include <glm/glm.hpp>

namespace Chained
{
	struct Camera3D;
	class TextureAsset;

	// Renders billboard editor icons for cameras, lights, spawns, and audio sources.
	// Icons are rendered directly into the currently-bound framebuffer (HDR pass).
	class ViewportIcons
	{
	public:
		// Renders all icon categories. Called during the HDR render pass.
		static void RenderAll(entt::registry& registry, const Camera3D& camera);

	private:
		static void RenderLightIcons(entt::registry& registry, const Camera3D& camera, float iconMin, float iconMax,
									 float iconScale);
		static void DrawBillboard(const Camera3D& camera, uint32_t textureId, const glm::vec3& worldPos, float iconSize,
								  const glm::vec4& tint);

		static uint32_t GetIconHandle(const std::shared_ptr<TextureAsset>& icon);
		static float ComputeIconSize(const glm::vec3& worldPos, const glm::vec3& cameraPos, float minSize,
									 float maxSize, float scale);
	};

} // namespace Chained

#endif // CH_VIEWPORT_ICONS_H
