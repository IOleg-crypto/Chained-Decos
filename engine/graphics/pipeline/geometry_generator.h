#ifndef CH_GEOMETRY_GENERATOR_H
#define CH_GEOMETRY_GENERATOR_H

#include "engine/graphics/api/renderer_types.h"
#include "engine/assets/model_data.h"
#include "engine/assets/loaders/model_loader.h"
#include <string>

namespace Chained
{
	class GeometryGenerator
	{
	public:
		// Generates a sphere mesh
		static Mesh GenerateSphere(float radius, int slices, int stacks);

		// Generates a unit cube mesh for skyboxes/cubemaps (triangles)
		static Mesh GenerateUnitCube();

		// Generates a wireframe cube with 12 edges for debug drawing (GL_LINES)
		static Mesh GenerateWireCube();

		// Generates a grid mesh
		static Mesh GenerateGrid(int slices, float spacing);

		// Generates a fullscreen quad or a simple plane quad
		static Mesh GenerateQuad(float size);

		// Generates a capsule mesh (cylinder + two hemispheres)
		static Mesh GenerateCapsule(float radius, float height, int slices, int stacks);

		// Generate a cube mesh with given dimensions (for procedural cube generation)
		static Mesh GenerateCube(const glm::vec3& dimensions);

		// Generate raw CPU-side model data (de-interleaved vertices/normals/texcoords/indices + one
		// identity instance + a default material) for a procedural primitive. Does NOT create a VAO;
		// feed the result to ModelAsset::SetPendingData + OnLoaded (main thread) to finalize on GPU.
		// `type` uses ":cube:"/":sphere:"/":plane:"/":cylinder:"/... markers.
		static PendingModelData GeneratePrimitivePendingData(const std::string& type,
															 const ProceduralParameters& params);
	};
} // namespace Chained

#endif // CH_GEOMETRY_GENERATOR_H
