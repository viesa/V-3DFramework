#include "Saffron/SaffronPCH.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/attrdefs.h>

#include <utility>

#include "Saffron/Scene/Entity.h"
#include "Saffron/Gui/Gui.h"
#include "Saffron/Script/ScriptEngine.h"
#include "Saffron/Script/ScriptEngineRegistry.h"

namespace Se
{

///////////////////////////////////////////////////////////////////////////
/// Helper functions
///////////////////////////////////////////////////////////////////////////

static Uint32 GetFieldSize(FieldType type)
{
	switch ( type )
	{
	case FieldType::Float:       return 4;
	case FieldType::Int:         return 4;
	case FieldType::UnsignedInt: return 4;
		// case FieldType::String:   return 8; // TODO
	case FieldType::Vec2:        return 4 * 2;
	case FieldType::Vec3:        return 4 * 3;
	case FieldType::Vec4:        return 4 * 4;
	default:
		SE_CORE_ASSERT(false, "Unknown field type!");
		return 0;
	}
}

static FieldType GetHazelFieldType(MonoType *monoType)
{
	const int type = mono_type_get_type(monoType);
	switch ( type )
	{
	case MONO_TYPE_R4:			return FieldType::Float;
	case MONO_TYPE_I4:			return FieldType::Int;
	case MONO_TYPE_U4:			return FieldType::UnsignedInt;
	case MONO_TYPE_STRING:		return FieldType::String;
	case MONO_TYPE_VALUETYPE:
	{
		char *name = mono_type_get_name(monoType);
		if ( strcmp(name, "Hazel.Vector2") == 0 )	return FieldType::Vec2;
		if ( strcmp(name, "Hazel.Vector3") == 0 )	return FieldType::Vec3;
		if ( strcmp(name, "Hazel.Vector4") == 0 )	return FieldType::Vec4;
	}
	default:					return FieldType::None;
	}
}

const char *FieldTypeToString(FieldType type)
{
	switch ( type )
	{
	case FieldType::Float:			return "Float";
	case FieldType::Int:			return "Int";
	case FieldType::UnsignedInt:	return "UnsignedInt";
	case FieldType::String:			return "String";
	case FieldType::Vec2:			return "Vec2";
	case FieldType::Vec3:			return "Vec3";
	case FieldType::Vec4:			return "Vec4";
	default:						return "Unknown";
	}
}



///////////////////////////////////////////////////////////////////////////
/// Mono declarations
///////////////////////////////////////////////////////////////////////////

static MonoDomain *sMonoDomain = nullptr;
static std::string sAssemblyPath;
static Ref<Scene> sSceneContext;

// Assembly images
MonoImage *sAppAssemblyImage = nullptr;
MonoImage *sCoreAssemblyImage = nullptr;

static MonoAssembly *sAppAssembly = nullptr;
static MonoAssembly *sCoreAssembly = nullptr;

static EntityInstanceMap sEntityInstanceMap;

static MonoMethod *GetMethod(MonoImage *image, const std::string &methodDesc);


///////////////////////////////////////////////////////////////////////////
/// Entity Script Class
///////////////////////////////////////////////////////////////////////////

struct EntityScriptClass
{
	std::string FullName;
	std::string ClassName;
	std::string NamespaceName;

	MonoClass *Class = nullptr;
	MonoMethod *OnCreateMethod = nullptr;
	MonoMethod *OnDestroyMethod = nullptr;
	MonoMethod *OnUpdateMethod = nullptr;

	// Physics
	MonoMethod *OnCollision2DBeginMethod = nullptr;
	MonoMethod *OnCollision2DEndMethod = nullptr;

	void InitClassMethods(MonoImage *image)
	{
		OnCreateMethod = GetMethod(image, FullName + ":OnCreate()");
		OnUpdateMethod = GetMethod(image, FullName + ":OnUpdate(single)");

		// Physics (Entity class)
		OnCollision2DBeginMethod = GetMethod(sCoreAssemblyImage, "Hazel.Entity:OnCollision2DBegin(single)");
		OnCollision2DEndMethod = GetMethod(sCoreAssemblyImage, "Hazel.Entity:OnCollision2DEnd(single)");
	}
};


///////////////////////////////////////////////////////////////////////////
/// Mono functions / Setup procedures
///////////////////////////////////////////////////////////////////////////

static std::unordered_map<std::string, EntityScriptClass> sEntityClassMap;

MonoAssembly *LoadAssemblyFromFile(const char *filepath)
{
	if ( filepath == nullptr )
	{
		return nullptr;
	}

	const HANDLE file = CreateFileA(filepath, FILE_READ_ACCESS, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if ( file == INVALID_HANDLE_VALUE )
	{
		return nullptr;
	}

	const DWORD file_size = GetFileSize(file, nullptr);
	if ( file_size == INVALID_FILE_SIZE )
	{
		CloseHandle(file);
		return nullptr;
	}

	void *file_data = malloc(file_size);
	if ( file_data == nullptr )
	{
		CloseHandle(file);
		return nullptr;
	}

	DWORD read = 0;
	ReadFile(file, file_data, file_size, &read, nullptr);
	if ( file_size != read )
	{
		free(file_data);
		CloseHandle(file);
		return nullptr;
	}

	MonoImageOpenStatus status;
	MonoImage *image = mono_image_open_from_data_full(static_cast<char *>(file_data), file_size, 1, &status, 0);
	if ( status != MONO_IMAGE_OK )
	{
		return nullptr;
	}
	const auto assemb = mono_assembly_load_from_full(image, filepath, &status, 0);
	free(file_data);
	CloseHandle(file);
	mono_image_close(image);
	return assemb;
}

static void InitMono()
{
	mono_set_assemblies_path("mono/lib");
	// mono_jit_set_trace_options("--verbose");
	auto *domain = mono_jit_init("Hazel");

	char *name = static_cast<char *>("HazelRuntime");
	sMonoDomain = mono_domain_create_appdomain(name, nullptr);
}

static void ShutdownMono()
{
	mono_jit_cleanup(sMonoDomain);
}

static MonoAssembly *LoadAssembly(const std::string &path)
{
	MonoAssembly *assembly = LoadAssemblyFromFile(path.c_str());

	if ( !assembly )
		SE_ERROR("Could not load assembly: {0}", path);
	else
		SE_INFO("Successfully loaded assembly: {0}", path);

	return assembly;
}

static MonoImage *GetAssemblyImage(MonoAssembly *assembly)
{
	MonoImage *image = mono_assembly_get_image(assembly);
	if ( !image )
		SE_ERROR("mono_assembly_get_image failed");

	return image;
}

static MonoClass *GetClass(MonoImage *image, const EntityScriptClass &scriptClass)
{
	MonoClass *monoClass = mono_class_from_name(image, scriptClass.NamespaceName.c_str(), scriptClass.ClassName.c_str());
	if ( !monoClass )
		SE_ERROR("mono_class_from_name failed");

	return monoClass;
}

static Uint32 Instantiate(EntityScriptClass &scriptClass)
{
	MonoObject *instance = mono_object_new(sMonoDomain, scriptClass.Class);
	if ( !instance )
		SE_ERROR("mono_object_new failed");

	mono_runtime_object_init(instance);
	const Uint32 handle = mono_gchandle_new(instance, false);
	return handle;
}

static MonoMethod *GetMethod(MonoImage *image, const std::string &methodDesc)
{
	MonoMethodDesc *desc = mono_method_desc_new(methodDesc.c_str(), NULL);
	if ( !desc )
		SE_ERROR("mono_method_desc_new failed");

	MonoMethod *method = mono_method_desc_search_in_image(desc, image);
	if ( !method )
		SE_ERROR("mono_method_desc_search_in_image failed");

	return method;
}

static MonoObject *CallMethod(MonoObject *object, MonoMethod *method, void **params = nullptr)
{
	MonoObject *pException = nullptr;
	MonoObject *result = mono_runtime_invoke(method, object, params, &pException);
	return result;
}

static void PrintClassMethods(MonoClass *monoClass)
{
	MonoMethod *iter;
	void *ptr = nullptr;
	while ( (iter = mono_class_get_methods(monoClass, &ptr)) != nullptr )
	{
		printf("--------------------------------\n");
		const char *name = mono_method_get_name(iter);
		MonoMethodDesc *methodDesc = mono_method_desc_from_method(iter);

		const char *paramNames = "";
		mono_method_get_param_names(iter, &paramNames);

		//TODO: Perhaps this should be printed in imgui?
		SE_INFO("Name: {0}", name);
		SE_INFO("Full name: {0}", mono_method_full_name(iter, true));
	}
}

static void PrintClassProperties(MonoClass *monoClass)
{
	MonoProperty *iter;
	void *ptr = nullptr;
	while ( (iter = mono_class_get_properties(monoClass, &ptr)) != nullptr )
	{
		SE_INFO("--------------------------------\n");
		SE_INFO("Name: {0}", mono_property_get_name(iter));
	}
}


static MonoString *GetName()
{
	return mono_string_new(sMonoDomain, "Hello!");
}


///////////////////////////////////////////////////////////////////////////
/// Entity Instance
///////////////////////////////////////////////////////////////////////////

MonoObject *EntityInstance::GetInstance() const
{
	SE_CORE_ASSERT(Handle, "Entity has not been instantiated!");
	return mono_gchandle_get_target(Handle);
}


///////////////////////////////////////////////////////////////////////////
/// Public Field
///////////////////////////////////////////////////////////////////////////

PublicField::PublicField(std::string name, FieldType type)
	: Name(std::move(name)), Type(type)
{
	m_StoredValueBuffer = AllocateBuffer(type);
}

PublicField::PublicField(PublicField &&other) noexcept
{
	Name = std::move(other.Name);
	Type = other.Type;
	m_EntityInstance = other.m_EntityInstance;
	m_MonoClassField = other.m_MonoClassField;
	m_StoredValueBuffer = other.m_StoredValueBuffer;

	other.m_EntityInstance = nullptr;
	other.m_MonoClassField = nullptr;
	other.m_StoredValueBuffer = nullptr;
}

PublicField::~PublicField()
{
	delete[] m_StoredValueBuffer;
}

void PublicField::CopyStoredValueToRuntime() const
{
	SE_CORE_ASSERT(m_EntityInstance->GetInstance());
	mono_field_set_value(m_EntityInstance->GetInstance(), m_MonoClassField, m_StoredValueBuffer);

}

bool PublicField::IsRuntimeAvailable() const
{
	return m_EntityInstance->Handle != 0;
}

void PublicField::SetStoredValueRaw(void *src) const
{
	const Uint32 size = GetFieldSize(Type);
	memcpy(m_StoredValueBuffer, src, size);
}

Uint8 *PublicField::AllocateBuffer(FieldType type)
{
	const Uint32 size = GetFieldSize(type);
	auto *buffer = new Uint8[size];
	memset(buffer, 0, size);
	return buffer;
}

void PublicField::GetStoredValue_Internal(void *outValue) const
{
	const Uint32 size = GetFieldSize(Type);
	memcpy(outValue, m_StoredValueBuffer, size);
}

void PublicField::GetRuntimeValue_Internal(void *outValue) const
{
	SE_CORE_ASSERT(m_EntityInstance->GetInstance());
	mono_field_get_value(m_EntityInstance->GetInstance(), m_MonoClassField, outValue);
}

void PublicField::SetStoredValue_Internal(void *value) const
{
	const Uint32 size = GetFieldSize(Type);
	memcpy(m_StoredValueBuffer, value, size);
}

void PublicField::SetRuntimeValue_Internal(void *value) const
{
	SE_CORE_ASSERT(m_EntityInstance->GetInstance());
	mono_field_set_value(m_EntityInstance->GetInstance(), m_MonoClassField, value);
}


///////////////////////////////////////////////////////////////////////////
/// Script Engine
///////////////////////////////////////////////////////////////////////////

void ScriptEngine::Init(const std::string &assemblyPath)
{
	sAssemblyPath = assemblyPath;

	InitMono();

	LoadSaffronRuntimeAssembly(sAssemblyPath);
}

void ScriptEngine::Shutdown()
{
	sSceneContext = nullptr;
	sEntityInstanceMap.clear();
}

void ScriptEngine::OnSceneDestruct(UUID sceneID)
{
	if ( sEntityInstanceMap.find(sceneID) != sEntityInstanceMap.end() )
	{
		sEntityInstanceMap.at(sceneID).clear();
		sEntityInstanceMap.erase(sceneID);
	}
}

void ScriptEngine::LoadSaffronRuntimeAssembly(const std::string &path)
{
	MonoDomain *domain = nullptr;
	bool cleanup = false;
	if ( sMonoDomain )
	{
		domain = mono_domain_create_appdomain("Hazel Runtime", nullptr);
		mono_domain_set(domain, false);
		cleanup = true;
	}

	sCoreAssembly = LoadAssembly("assets/scripts/Saffron-ScriptCore.dll");
	sCoreAssemblyImage = GetAssemblyImage(sCoreAssembly);

	auto *appAssembly = LoadAssembly(path);
	auto *appAssemblyImage = GetAssemblyImage(appAssembly);
	ScriptEngineRegistry::RegisterAll();

	if ( cleanup )
	{
		mono_domain_unload(sMonoDomain);
		sMonoDomain = domain;
	}

	sAppAssembly = appAssembly;
	sAppAssemblyImage = appAssemblyImage;
}

void ScriptEngine::ReloadAssembly(const std::string &path)
{
	LoadSaffronRuntimeAssembly(path);
	if ( !sEntityInstanceMap.empty() )
	{
		Ref<Scene> scene = GetCurrentSceneContext();
		SE_CORE_ASSERT(scene, "No active scene!");

		if ( sEntityInstanceMap.find(scene->GetUUID()) != sEntityInstanceMap.end() )
		{
			// TODO: How does this even work?
			auto &entityMap = sEntityInstanceMap.at(scene->GetUUID());
			for ( auto &[entityID, entityInstanceData] : entityMap )
			{
				const auto &entityMap = scene->GetEntityMap();
				SE_CORE_ASSERT(entityMap.find(entityID) != entityMap.end(), "Invalid entity ID or entity doesn't exist in scene!");
				InitScriptEntity(entityMap.at(entityID));
			}
		}
	}
}

void ScriptEngine::SetSceneContext(const Ref<Scene> &scene)
{
	mono_domain_unload(sMonoDomain);
}

const Ref<Scene> &ScriptEngine::GetCurrentSceneContext()
{
	return sSceneContext;
}

void ScriptEngine::CopyEntityScriptData(UUID dst, UUID src)
{
	SE_CORE_ASSERT(sEntityInstanceMap.find(dst) != sEntityInstanceMap.end());
	SE_CORE_ASSERT(sEntityInstanceMap.find(src) != sEntityInstanceMap.end());

	auto &dstEntityMap = sEntityInstanceMap.at(dst);
	auto &srcEntityMap = sEntityInstanceMap.at(src);

	for ( auto &[entityID, entityInstanceData] : srcEntityMap )
	{
		for ( auto &[moduleName, srcFieldMap] : srcEntityMap[entityID].ModuleFieldMap )
		{
			auto &dstModuleFieldMap = dstEntityMap[entityID].ModuleFieldMap;
			for ( auto &[fieldName, field] : srcFieldMap )
			{
				SE_CORE_ASSERT(dstModuleFieldMap.find(moduleName) != dstModuleFieldMap.end());
				auto &fieldMap = dstModuleFieldMap.at(moduleName);
				SE_CORE_ASSERT(fieldMap.find(fieldName) != fieldMap.end());
				fieldMap.at(fieldName).SetStoredValueRaw(field.m_StoredValueBuffer);
			}
		}
	}
}

void ScriptEngine::OnCreateEntity(Entity entity)
{
	OnCreateEntity(entity.m_Scene->GetUUID(), entity.GetComponent<IDComponent>().ID);
}

void ScriptEngine::OnCreateEntity(UUID sceneID, UUID entityID)
{
	EntityInstance &entityInstance = GetEntityInstanceData(sceneID, entityID).Instance;
	if ( entityInstance.ScriptClass->OnCreateMethod )
		CallMethod(entityInstance.GetInstance(), entityInstance.ScriptClass->OnCreateMethod);
}

void ScriptEngine::OnUpdateEntity(UUID sceneID, UUID entityID, Time ts)
{
	EntityInstance &entityInstance = GetEntityInstanceData(sceneID, entityID).Instance;
	if ( entityInstance.ScriptClass->OnUpdateMethod )
	{
		void *args[] = { &ts };
		CallMethod(entityInstance.GetInstance(), entityInstance.ScriptClass->OnUpdateMethod, args);
	}
}

void ScriptEngine::OnCollision2DBegin(Entity entity)
{
	OnCollision2DBegin(entity.m_Scene->GetUUID(), entity.GetComponent<IDComponent>().ID);
}

void ScriptEngine::OnCollision2DBegin(UUID sceneID, UUID entityID)
{
	EntityInstance &entityInstance = GetEntityInstanceData(sceneID, entityID).Instance;
	if ( entityInstance.ScriptClass->OnCollision2DBeginMethod )
	{
		float value = 5.0f;
		void *args[] = { &value };
		CallMethod(entityInstance.GetInstance(), entityInstance.ScriptClass->OnCollision2DBeginMethod, args);
	}
}

void ScriptEngine::OnCollision2DEnd(Entity entity)
{
	OnCollision2DEnd(entity.m_Scene->GetUUID(), entity.GetComponent<IDComponent>().ID);
}

void ScriptEngine::OnCollision2DEnd(UUID sceneID, UUID entityID)
{
	EntityInstance &entityInstance = GetEntityInstanceData(sceneID, entityID).Instance;
	if ( entityInstance.ScriptClass->OnCollision2DEndMethod )
	{
		float value = 5.0f;
		void *args[] = { &value };
		CallMethod(entityInstance.GetInstance(), entityInstance.ScriptClass->OnCollision2DEndMethod, args);
	}
}

void ScriptEngine::OnScriptComponentDestroyed(UUID sceneID, UUID entityID)
{
	SE_CORE_ASSERT(sEntityInstanceMap.find(sceneID) != sEntityInstanceMap.end());
	auto &entityMap = sEntityInstanceMap.at(sceneID);
	SE_CORE_ASSERT(entityMap.find(entityID) != entityMap.end());
	entityMap.erase(entityID);
}

bool ScriptEngine::ModuleExists(const std::string &moduleName)
{
	std::string NamespaceName, ClassName;
	if ( moduleName.find('.') != std::string::npos )
	{
		NamespaceName = moduleName.substr(0, moduleName.find_last_of('.'));
		ClassName = moduleName.substr(moduleName.find_last_of('.') + 1);
	}
	else
	{
		ClassName = moduleName;
	}

	MonoClass *monoClass = mono_class_from_name(sAppAssemblyImage, NamespaceName.c_str(), ClassName.c_str());
	return monoClass != nullptr;
}

void ScriptEngine::InitScriptEntity(Entity entity)
{
	Scene *scene = entity.m_Scene;
	const UUID id = entity.GetComponent<IDComponent>().ID;
	auto &moduleName = entity.GetComponent<ScriptComponent>().ModuleName;
	if ( moduleName.empty() )
		return;

	if ( !ModuleExists(moduleName) )
	{
		SE_CORE_ERROR("Entity references non-existent script module '{0}'", moduleName);
		return;
	}

	EntityScriptClass &scriptClass = sEntityClassMap[moduleName];
	scriptClass.FullName = moduleName;
	if ( moduleName.find('.') != std::string::npos )
	{
		scriptClass.NamespaceName = moduleName.substr(0, moduleName.find_last_of('.'));
		scriptClass.ClassName = moduleName.substr(moduleName.find_last_of('.') + 1);
	}
	else
	{
		scriptClass.ClassName = moduleName;
	}

	scriptClass.Class = GetClass(sAppAssemblyImage, scriptClass);
	scriptClass.InitClassMethods(sAppAssemblyImage);

	EntityInstanceData &entityInstanceData = sEntityInstanceMap[scene->GetUUID()][id];
	EntityInstance &entityInstance = entityInstanceData.Instance;
	entityInstance.ScriptClass = &scriptClass;
	ScriptModuleFieldMap &moduleFieldMap = entityInstanceData.ModuleFieldMap;
	auto &fieldMap = moduleFieldMap[moduleName];

	// Save old fields
	std::unordered_map<std::string, PublicField> oldFields;
	oldFields.reserve(fieldMap.size());
	for ( auto &[fieldName, field] : fieldMap )
		oldFields.emplace(fieldName, std::move(field));
	fieldMap.clear();

	// Retrieve public fields (TODO: cache these fields if the module is used more than once)
	{
		MonoClassField *iter;
		void *ptr = nullptr;
		while ( (iter = mono_class_get_fields(scriptClass.Class, &ptr)) != nullptr )
		{
			const char *name = mono_field_get_name(iter);
			const Uint32 flags = mono_field_get_flags(iter);
			if ( (flags & MONO_FIELD_ATTR_PUBLIC) == 0 )
				continue;

			MonoType *fieldType = mono_field_get_type(iter);
			const FieldType hazelFieldType = GetHazelFieldType(fieldType);

			// TODO: Attributes
			MonoCustomAttrInfo *attr = mono_custom_attrs_from_field(scriptClass.Class, iter);

			if ( oldFields.find(name) != oldFields.end() )
			{
				fieldMap.emplace(name, std::move(oldFields.at(name)));
			}
			else
			{
				PublicField field = { name, hazelFieldType };
				field.m_EntityInstance = &entityInstance;
				field.m_MonoClassField = iter;
				fieldMap.emplace(name, std::move(field));
			}
		}
	}
}

void ScriptEngine::ShutdownScriptEntity(Entity entity, const std::string &moduleName)
{
	EntityInstanceData &entityInstanceData = GetEntityInstanceData(entity.GetSceneUUID(), entity.GetUUID());
	ScriptModuleFieldMap &moduleFieldMap = entityInstanceData.ModuleFieldMap;
	if ( moduleFieldMap.find(moduleName) != moduleFieldMap.end() )
		moduleFieldMap.erase(moduleName);
}

void ScriptEngine::InstantiateEntityClass(Entity entity)
{
	Scene *scene = entity.m_Scene;
	UUID id = entity.GetComponent<IDComponent>().ID;
	auto &moduleName = entity.GetComponent<ScriptComponent>().ModuleName;

	EntityInstanceData &entityInstanceData = GetEntityInstanceData(scene->GetUUID(), id);
	EntityInstance &entityInstance = entityInstanceData.Instance;
	SE_CORE_ASSERT(entityInstance.ScriptClass);
	entityInstance.Handle = Instantiate(*entityInstance.ScriptClass);

	MonoProperty *entityIDPropery = mono_class_get_property_from_name(entityInstance.ScriptClass->Class, "ID");
	mono_property_get_get_method(entityIDPropery);
	MonoMethod *entityIDSetMethod = mono_property_get_set_method(entityIDPropery);
	void *param[] = { &id };
	CallMethod(entityInstance.GetInstance(), entityIDSetMethod, param);

	// Set all public fields to appropriate values
	ScriptModuleFieldMap &moduleFieldMap = entityInstanceData.ModuleFieldMap;
	if ( moduleFieldMap.find(moduleName) != moduleFieldMap.end() )
	{
		auto &publicFields = moduleFieldMap.at(moduleName);
		for ( auto &[name, field] : publicFields )
			field.CopyStoredValueToRuntime();
	}

	// Call OnCreate function (if exists)
	OnCreateEntity(entity);
}

EntityInstanceMap &ScriptEngine::GetEntityInstanceMap()
{
	return sEntityInstanceMap;
}

EntityInstanceData &ScriptEngine::GetEntityInstanceData(UUID sceneID, UUID entityID)
{
	SE_CORE_ASSERT(sEntityInstanceMap.find(sceneID) != sEntityInstanceMap.end(), "Invalid scene ID!");
	auto &entityIDMap = sEntityInstanceMap.at(sceneID);

	SE_CORE_ASSERT(entityIDMap.find(entityID) != entityIDMap.end(), "Invalid entity ID!");
	return entityIDMap.at(entityID);
}

void ScriptEngine::OnImGuiRender()
{
	ImGui::Begin("Script Engine Debug");
	for ( auto &[sceneID, entityMap] : sEntityInstanceMap )
	{
		bool opened = ImGui::TreeNode(reinterpret_cast<void *>(static_cast<Uint64>(sceneID)), "Scene (%llx)", static_cast<Uint64>(sceneID));
		if ( opened )
		{
			Ref<Scene> scene = Scene::GetScene(sceneID);
			for ( auto &[entityID, entityInstanceData] : entityMap )
			{
				Entity entity = scene->GetScene(sceneID)->GetEntityMap().at(entityID);
				std::string entityName = "Unnamed Entity";
				if ( entity.HasComponent<TagComponent>() )
					entityName = entity.GetComponent<TagComponent>().Tag;
				opened = ImGui::TreeNode(reinterpret_cast<void *>(static_cast<Uint64>(entityID)), "%s (%llx)", entityName.c_str(), static_cast<Uint64>(entityID));
				if ( opened )
				{
					for ( auto &[moduleName, fieldMap] : entityInstanceData.ModuleFieldMap )
					{
						opened = ImGui::TreeNode(moduleName.c_str());
						if ( opened )
						{
							for ( auto &[fieldName, field] : fieldMap )
							{

								opened = ImGui::TreeNodeEx(static_cast<void *>(&field), ImGuiTreeNodeFlags_Leaf, fieldName.c_str());
								if ( opened )
								{

									ImGui::TreePop();
								}
							}
							ImGui::TreePop();
						}
					}
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
	}
	ImGui::End();
}
}