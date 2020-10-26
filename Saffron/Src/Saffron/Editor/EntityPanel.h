#pragma once

#include "Saffron/Editor/ScriptPanel.h"
#include "Saffron/Entity/Entity.h"
#include "Saffron/Renderer/Mesh.h"
#include "Saffron/Scene/Scene.h"

namespace Se
{
class EntityPanel : public ReferenceCounted
{
public:
	explicit EntityPanel(const Shared<Scene> &context);

	void OnGuiRender(const Shared<ScriptPanel> &scriptPanel);

	void SetContext(const Shared<Scene> &context);
	void SetSelected(Entity entity);
	void SetSelectionChangedCallback(const Function<void(Entity)> &func) { m_SelectionChangedCallback = func; }
	void SetEntityDeletedCallback(const Function<void(Entity)> &func) { m_EntityDeletedCallback = func; }

private:
	void OnGuiRenderSceneHierarchy(const Shared<ScriptPanel> &scriptPanel);
	void OnGuiRenderMaterial();
	void OnGuiRenderMeshDebug();

	void DrawEntityNode(Entity entity);
	void DrawMeshNode(const Shared<Mesh> &mesh, UUID &entityUUID) const;
	void MeshNodeHierarchy(const Shared<Mesh> &mesh, aiNode *node, const Matrix4f &parentTransform = Matrix4f(1.0f), Uint32 level = 0) const;
	void DrawComponents(Entity entity);

private:
	Shared<Scene> m_Context;
	Entity m_SelectionContext;

	Map<String, Shared<Texture2D>> m_TexStore;

	Function<void(Entity)> m_SelectionChangedCallback, m_EntityDeletedCallback;
};
}