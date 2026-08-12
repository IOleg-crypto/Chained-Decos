#ifndef CH_NAMETAG_SYSTEM_H
#define CH_NAMETAG_SYSTEM_H

namespace Chained
{
	class Scene;
	class Renderer;

	namespace NametagSystem
	{
		void DrawNametags(Scene* scene, Renderer* renderer);
		void Shutdown();
	} // namespace NametagSystem
} // namespace Chained
#endif
