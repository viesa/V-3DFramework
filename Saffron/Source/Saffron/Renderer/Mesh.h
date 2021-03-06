#pragma once

#include "Saffron/Core/Math/SaffronMath.h"
#include "Saffron/Core/Time.h"
#include "Saffron/Core/Math/AABB.h"
#include "Saffron/Renderer/Material.h"
#include "Saffron/Renderer/IndexBuffer.h"
#include "Saffron/Renderer/Pipeline.h"
#include "Saffron/Renderer/Shader.h"
#include "Saffron/Renderer/VertexBuffer.h"

struct aiNode;
struct aiAnimation;
struct aiNodeAnim;
struct aiScene;

namespace Assimp
{
class Importer;
}

namespace Se
{

////////////////////////////////////////////////////////////////////////
/// Vertex
////////////////////////////////////////////////////////////////////////

struct Vertex
{
	Vector3f Position;
	Vector3f Normal;
	Vector3f Tangent;
	Vector3f Binormal;
	Vector2f TexCoord;
};


////////////////////////////////////////////////////////////////////////
/// Animated Vertex
////////////////////////////////////////////////////////////////////////

struct AnimatedVertex
{
	Vector3f Position{};
	Vector3f Normal{};
	Vector3f Tangent{};
	Vector3f Binormal{};
	Vector2f TexCoord{};

	Uint32 IDs[4] = { 0, 0,0, 0 };
	float Weights[4]{ 0.0f, 0.0f, 0.0f, 0.0f };

	void AddBoneData(Uint32 BoneID, float Weight);
};

static const int NumAttributes = 5;


////////////////////////////////////////////////////////////////////////
/// Index
////////////////////////////////////////////////////////////////////////

struct Index
{
	Uint32 V1, V2, V3;
};

static_assert(sizeof(Index) == 3 * sizeof(Uint32));


////////////////////////////////////////////////////////////////////////
/// Bone Info
////////////////////////////////////////////////////////////////////////

struct BoneInfo
{
	Matrix4f BoneOffset;
	Matrix4f FinalTransformation;
};


////////////////////////////////////////////////////////////////////////
/// Vertex Bone Data
////////////////////////////////////////////////////////////////////////

struct VertexBoneData
{
	Uint32 IDs[4]{};
	float Weights[4]{};

	VertexBoneData();

	void AddBoneData(Uint32 BoneID, float Weight);
};

struct Triangle
{
	Vertex V0, V1, V2;

	Triangle(const Vertex &v0, const Vertex &v1, const Vertex &v2)
		: V0(v0), V1(v1), V2(v2)
	{
	}
};

class Submesh
{
public:
	Uint32 BaseVertex{};
	Uint32 BaseIndex{};
	Uint32 MaterialIndex{};
	Uint32 IndexCount{};

	Matrix4f Transform{};
	AABB BoundingBox;

	String NodeName, MeshName;
};

class Mesh : public MemManaged<Mesh>
{
public:
	Mesh(String filename);
	~Mesh();

	void OnUpdate();
	void DumpVertexBuffer();

	ArrayList<Submesh> &GetSubmeshes() { return m_Submeshes; }
	const ArrayList<Submesh> &GetSubmeshes() const { return m_Submeshes; }

	const Matrix4f &GetLocalTransform() const { return m_LocalTransform; }
	void SetLocalTransform(Matrix4f localTransform) { m_LocalTransform = Move(localTransform); }

	ArrayList<AABB> GetBoundingBoxes(const Matrix4f &transform = Matrix4f(1));
	std::shared_ptr<Shader> GetMeshShader() const { return m_MeshShader; }
	std::shared_ptr<Material> GetMaterial() const { return m_BaseMaterial; }
	ArrayList<std::shared_ptr<MaterialInstance>> GetMaterials() const { return m_Materials; }
	const ArrayList<std::shared_ptr<Texture2D>> &GetTextures() const { return m_Textures; }
	const String &GetFilepath() const { return m_Filepath; }

	ArrayList<Triangle> GetTriangleCache(Uint32 index) const { return m_TriangleCache.at(index); }

private:
	void BoneTransform(float time);
	void ReadNodeHierarchy(float AnimationTime, const aiNode *pNode, const Matrix4f &ParentTransform);
	void TraverseNodes(aiNode *node, const Matrix4f &parentTransform = Matrix4f(1.0f), Uint32 level = 0);

	const aiNodeAnim *FindNodeAnim(const aiAnimation *animation, const String &nodeName);
	Uint32 FindPosition(float AnimationTime, const aiNodeAnim *pNodeAnim);
	Uint32 FindRotation(float AnimationTime, const aiNodeAnim *pNodeAnim);
	Uint32 FindScaling(float AnimationTime, const aiNodeAnim *pNodeAnim);
	Vector3f InterpolateTranslation(float animationTime, const aiNodeAnim *nodeAnim);
	glm::quat InterpolateRotation(float animationTime, const aiNodeAnim *nodeAnim);
	Vector3f InterpolateScale(float animationTime, const aiNodeAnim *nodeAnim);

private:
	ArrayList<Submesh> m_Submeshes;

	std::unique_ptr<Assimp::Importer> m_Importer;

	Matrix4f m_InverseTransform{};

	Uint32 m_BoneCount = 0;
	ArrayList<BoneInfo> m_BoneInfo;

	std::shared_ptr<Pipeline> m_Pipeline;
	std::shared_ptr<VertexBuffer> m_VertexBuffer;
	std::shared_ptr<IndexBuffer> m_IndexBuffer;

	ArrayList<Vertex> m_StaticVertices;
	ArrayList<AnimatedVertex> m_AnimatedVertices;
	ArrayList<Index> m_Indices;
	UnorderedMap<String, Uint32> m_BoneMapping;
	ArrayList<Matrix4f> m_BoneTransforms;
	const aiScene *m_Scene;

	// Materials
	std::shared_ptr<Shader> m_MeshShader;
	std::shared_ptr<Material> m_BaseMaterial;
	ArrayList<std::shared_ptr<Texture2D>> m_Textures;
	ArrayList<std::shared_ptr<Texture2D>> m_NormalMaps;
	ArrayList<std::shared_ptr<MaterialInstance>> m_Materials;
	Matrix4f m_LocalTransform;

	UnorderedMap<Uint32, ArrayList<Triangle>> m_TriangleCache;

	// Animation
	bool m_IsAnimated = false;
	float m_AnimationTime = 0.0f;
	float m_WorldTime = 0.0f;
	float m_TimeMultiplier = 1.0f;
	bool m_AnimationPlaying = true;

	String m_Filepath;

	friend class Renderer;
	friend class EntityPanel;
	friend class ScenePanel;
};
}
