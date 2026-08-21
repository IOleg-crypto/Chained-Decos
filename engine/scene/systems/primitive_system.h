#ifndef CH_PRIMITIVE_SYSTEM_H
#define CH_PRIMITIVE_SYSTEM_H

#include <entt/entt.hpp>
#include <string>

namespace Chained
{

	/// @brief Manages the lifecycle of PrimitiveComponent entities.
	///
	/// Responsibilities:
	///  - On construct/update of PrimitiveComponent: bake geometry to a .chasset file
	///    under assets/primitives/ and keep ModelComponent.ModelPath in sync.
	///  - On destroy of PrimitiveComponent: remove the paired ModelComponent so the
	///    mesh disappears from the scene.
	///
	/// The generated file is always loaded through the standard ModelLoader pipeline;
	/// materials are then edited via the Material Editor exactly like any other mesh.
	namespace PrimitiveSystem
	{
		/// Register entt observers. Must be called once after the registry is created.
		/// @param primitiveDir  Absolute path to the directory where .chasset files are written
		///                      (typically <project>/assets/primitives/).
		void RegisterObservers(entt::registry& reg, const std::string& primitiveDir);

		/// Unregister all observers and free resources.
		void UnregisterObservers(entt::registry& reg);

	} // namespace PrimitiveSystem
} // namespace Chained

#endif // CH_PRIMITIVE_SYSTEM_H
