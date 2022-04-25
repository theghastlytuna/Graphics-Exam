#include "DefaultSceneLayer.h"

// GLM math library
#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>
#include <GLM/gtc/type_ptr.hpp>
#include <GLM/gtc/random.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <GLM/gtx/common.hpp> // for fmod (floating modulus)

#include <filesystem>

// Graphics
#include "Graphics/Buffers/IndexBuffer.h"
#include "Graphics/Buffers/VertexBuffer.h"
#include "Graphics/VertexArrayObject.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Textures/Texture2D.h"
#include "Graphics/Textures/TextureCube.h"
#include "Graphics/VertexTypes.h"
#include "Graphics/Font.h"
#include "Graphics/GuiBatcher.h"
#include "Graphics/Framebuffer.h"

// Utilities
#include "Utils/MeshBuilder.h"
#include "Utils/MeshFactory.h"
#include "Utils/ObjLoader.h"
#include "Utils/ImGuiHelper.h"
#include "Utils/ResourceManager/ResourceManager.h"
#include "Utils/FileHelpers.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/StringUtils.h"
#include "Utils/GlmDefines.h"

// Gameplay
#include "Gameplay/Material.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Gameplay/Components/Light.h"

// Components
#include "Gameplay/Components/IComponent.h"
#include "Gameplay/Components/Camera.h"
#include "Gameplay/Components/RotatingBehaviour.h"
#include "Gameplay/Components/JumpBehaviour.h"
#include "Gameplay/Components/RenderComponent.h"
#include "Gameplay/Components/MaterialSwapBehaviour.h"
#include "Gameplay/Components/TriggerVolumeEnterBehaviour.h"
#include "Gameplay/Components/SimpleCameraControl.h"
#include "Gameplay/Components/FirstPersonCamera.h"
#include "Gameplay/Components/PlayerControl.h"
#include "Gameplay/Components/EnemyBehaviour.h"

// Physics
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/Colliders/BoxCollider.h"
#include "Gameplay/Physics/Colliders/PlaneCollider.h"
#include "Gameplay/Physics/Colliders/SphereCollider.h"
#include "Gameplay/Physics/Colliders/ConvexMeshCollider.h"
#include "Gameplay/Physics/Colliders/CylinderCollider.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Graphics/DebugDraw.h"

// GUI
#include "Gameplay/Components/GUI/RectTransform.h"
#include "Gameplay/Components/GUI/GuiPanel.h"
#include "Gameplay/Components/GUI/GuiText.h"
#include "Gameplay/InputEngine.h"

#include "Application/Application.h"
#include "Gameplay/Components/ParticleSystem.h"
#include "Graphics/Textures/Texture3D.h"
#include "Graphics/Textures/Texture1D.h"
#include "Application/Layers/ImGuiDebugLayer.h"
#include "Application/Windows/DebugWindow.h"
#include "Gameplay/Components/ShadowCamera.h"

DefaultSceneLayer::DefaultSceneLayer() :
	ApplicationLayer()
{
	Name = "Default Scene";
	Overrides = AppLayerFunctions::OnAppLoad |
				AppLayerFunctions::OnWindowResize;
}

DefaultSceneLayer::~DefaultSceneLayer() = default;

void DefaultSceneLayer::OnAppLoad(const nlohmann::json& config) {
	_CreateScene();
	SetActive(false);
}

void DefaultSceneLayer::SetActive(bool active)
{
	_active = active;
}

void DefaultSceneLayer::RepositionUI()
{
	Application& app = Application::Get();

	Gameplay::GameObject::Sptr winScreen = app.CurrentScene()->FindObjectByName("Win Text");
	Gameplay::GameObject::Sptr loseScreen = app.CurrentScene()->FindObjectByName("Lose Text");

	winScreen->Get<RectTransform>()->SetMin({ 0, 0 });
	winScreen->Get<RectTransform>()->SetMax({app.GetWindowSize().x, app.GetWindowSize().y });

	loseScreen->Get<RectTransform>()->SetMin({ 0, 0 });
	loseScreen->Get<RectTransform>()->SetMax({ app.GetWindowSize().x, app.GetWindowSize().y });

}

bool DefaultSceneLayer::IsActive()
{
	return _active;
}

void DefaultSceneLayer::_CreateScene()
{
	using namespace Gameplay;
	using namespace Gameplay::Physics;

	Application& app = Application::Get();

	bool loadScene = false;
	// For now we can use a toggle to generate our scene vs load from file
	if (loadScene && std::filesystem::exists("scene.json")) {
		app.LoadScene("scene.json");
	} else {
		 
		// Basic gbuffer generation with no vertex manipulation
		ShaderProgram::Sptr deferredForward = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/basic.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/deferred_forward.glsl" }
		});
		deferredForward->SetDebugName("Deferred - GBuffer Generation");  

		// Our foliage shader which manipulates the vertices of the mesh
		ShaderProgram::Sptr foliageShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/foliage.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/deferred_forward.glsl" }
		});  
		foliageShader->SetDebugName("Foliage");   

		// This shader handles our multitexturing example
		ShaderProgram::Sptr multiTextureShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/vert_multitextured.glsl" },  
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/frag_multitextured.glsl" }
		});
		multiTextureShader->SetDebugName("Multitexturing"); 

		// This shader handles our displacement mapping example
		ShaderProgram::Sptr displacementShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/displacement_mapping.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/deferred_forward.glsl" }
		});
		displacementShader->SetDebugName("Displacement Mapping");

		// This shader handles our cel shading example
		ShaderProgram::Sptr celShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/displacement_mapping.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/cel_shader.glsl" }
		});
		celShader->SetDebugName("Cel Shader");


		// Load in the meshes
		MeshResource::Sptr monkeyMesh = ResourceManager::CreateAsset<MeshResource>("Monkey.obj");
		MeshResource::Sptr shipMesh   = ResourceManager::CreateAsset<MeshResource>("fenrir.obj");
		MeshResource::Sptr snakeMesh = ResourceManager::CreateAsset<MeshResource>("snake.obj");
		MeshResource::Sptr ballMesh = ResourceManager::CreateAsset<MeshResource>("hernia.obj");
		MeshResource::Sptr treeMesh = ResourceManager::CreateAsset<MeshResource>("tree.obj");

		// Load in some textures
		Texture2D::Sptr    boxTexture   = ResourceManager::CreateAsset<Texture2D>("textures/box-diffuse.png");
		Texture2D::Sptr    boxSpec      = ResourceManager::CreateAsset<Texture2D>("textures/box-specular.png");
		Texture2D::Sptr    monkeyTex    = ResourceManager::CreateAsset<Texture2D>("textures/monkey-uvMap.png");
		Texture2D::Sptr    leafTex      = ResourceManager::CreateAsset<Texture2D>("textures/leaves.png");
		Texture2D::Sptr    snakeTex = ResourceManager::CreateAsset<Texture2D>("textures/snake.png");
		Texture2D::Sptr    ballTex = ResourceManager::CreateAsset<Texture2D>("textures/hernia.png");
		Texture2D::Sptr    treeTex = ResourceManager::CreateAsset<Texture2D>("textures/DioramaUVTextures.png");

		leafTex->SetMinFilter(MinFilter::Nearest);
		leafTex->SetMagFilter(MagFilter::Nearest);
		Texture2D::Sptr    grassTex = ResourceManager::CreateAsset<Texture2D>("textures/minegrass.png");
		grassTex->SetMinFilter(MinFilter::Nearest);
		grassTex->SetMagFilter(MagFilter::Nearest);

		Texture2DArray::Sptr particleTex = ResourceManager::CreateAsset<Texture2DArray>("textures/particles.png", 2, 2);


		// Load some images for drag n' drop
		ResourceManager::CreateAsset<Texture2D>("textures/flashlight.png");
		ResourceManager::CreateAsset<Texture2D>("textures/flashlight-2.png");
		ResourceManager::CreateAsset<Texture2D>("textures/light_projection.png");

		DebugWindow::Sptr debugWindow = app.GetLayer<ImGuiDebugLayer>()->GetWindow<DebugWindow>();

#pragma region Basic Texture Creation
		Texture2DDescription singlePixelDescriptor;
		singlePixelDescriptor.Width = singlePixelDescriptor.Height = 1;
		singlePixelDescriptor.Format = InternalFormat::RGB8;

		float normalMapDefaultData[3] = { 0.5f, 0.5f, 1.0f };
		Texture2D::Sptr normalMapDefault = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		normalMapDefault->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, normalMapDefaultData);

		float solidGrey[3] = { 0.5f, 0.5f, 0.5f };
		float solidBlack[3] = { 0.0f, 0.0f, 0.0f };
		float solidWhite[3] = { 1.0f, 1.0f, 1.0f };

		Texture2D::Sptr solidBlackTex = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		solidBlackTex->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, solidBlack);

		Texture2D::Sptr solidGreyTex = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		solidGreyTex->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, solidGrey);

		Texture2D::Sptr solidWhiteTex = ResourceManager::CreateAsset<Texture2D>(singlePixelDescriptor);
		solidWhiteTex->LoadData(1, 1, PixelFormat::RGB, PixelType::Float, solidWhite);

#pragma endregion 

		// Loading in a 1D LUT
		Texture1D::Sptr toonLut = ResourceManager::CreateAsset<Texture1D>("luts/toon-1D.png"); 
		toonLut->SetWrap(WrapMode::ClampToEdge);

		// Here we'll load in the cubemap, as well as a special shader to handle drawing the skybox
		TextureCube::Sptr testCubemap = ResourceManager::CreateAsset<TextureCube>("cubemaps/sky/sky.jpg");
		ShaderProgram::Sptr      skyboxShader = ResourceManager::CreateAsset<ShaderProgram>(std::unordered_map<ShaderPartType, std::string>{
			{ ShaderPartType::Vertex, "shaders/vertex_shaders/skybox_vert.glsl" },
			{ ShaderPartType::Fragment, "shaders/fragment_shaders/skybox_frag.glsl" } 
		});
		
		// Create an empty scene
		Scene::Sptr scene = std::make_shared<Scene>();  
		scene->SetAmbientLight(glm::vec3(0.1f));

		// Setting up our enviroment map
		scene->SetSkyboxTexture(testCubemap); 
		scene->SetSkyboxShader(skyboxShader);
		// Since the skybox I used was for Y-up, we need to rotate it 90 deg around the X-axis to convert it to z-up 
		scene->SetSkyboxRotation(glm::rotate(MAT4_IDENTITY, glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)));

		// Loading in a color lookup table
		Texture3D::Sptr lut = ResourceManager::CreateAsset<Texture3D>("luts/cool.CUBE");   

		// Configure the color correction LUT
		scene->SetColorLUT(lut);

		// Create our materials
		// This will be our box material, with no environment reflections
		Material::Sptr boxMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			boxMaterial->Name = "Box";
			boxMaterial->Set("u_Material.AlbedoMap", boxTexture);
			boxMaterial->Set("u_Material.Shininess", 0.1f);
			boxMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr snakeMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			snakeMaterial->Name = "Snake";
			snakeMaterial->Set("u_Material.AlbedoMap", snakeTex);
			snakeMaterial->Set("u_Material.Shininess", 0.7f);
			snakeMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr ballMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			ballMaterial->Name = "Ball";
			ballMaterial->Set("u_Material.AlbedoMap", ballTex);
			ballMaterial->Set("u_Material.Shininess", 0.2f);
			ballMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr treeMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			treeMaterial->Name = "Tree";
			treeMaterial->Set("u_Material.AlbedoMap", treeTex);
			treeMaterial->Set("u_Material.Shininess", 0.2f);
			treeMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr grassMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			boxMaterial->Name = "grassMat";
			boxMaterial->Set("u_Material.AlbedoMap", grassTex);
			boxMaterial->Set("u_Material.Shininess", 0.1f);
			boxMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		// This will be the reflective material, we'll make the whole thing 90% reflective
		Material::Sptr monkeyMaterial = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			monkeyMaterial->Name = "Monkey";
			monkeyMaterial->Set("u_Material.AlbedoMap", monkeyTex);
			monkeyMaterial->Set("u_Material.NormalMap", normalMapDefault);
			monkeyMaterial->Set("u_Material.Shininess", 0.5f);
		}

		// This will be the reflective material, we'll make the whole thing 50% reflective
		Material::Sptr testMaterial = ResourceManager::CreateAsset<Material>(deferredForward); 
		{
			testMaterial->Name = "Box-Specular";
			testMaterial->Set("u_Material.AlbedoMap", boxTexture); 
			testMaterial->Set("u_Material.Specular", boxSpec);
			testMaterial->Set("u_Material.NormalMap", normalMapDefault);
		}

		// Our foliage vertex shader material 
		Material::Sptr foliageMaterial = ResourceManager::CreateAsset<Material>(foliageShader);
		{
			foliageMaterial->Name = "Foliage Shader";
			foliageMaterial->Set("u_Material.AlbedoMap", leafTex);
			foliageMaterial->Set("u_Material.Shininess", 0.1f);
			foliageMaterial->Set("u_Material.DiscardThreshold", 0.1f);
			foliageMaterial->Set("u_Material.NormalMap", normalMapDefault);

			foliageMaterial->Set("u_WindDirection", glm::vec3(1.0f, 1.0f, 0.0f));
			foliageMaterial->Set("u_WindStrength", 0.5f);
			foliageMaterial->Set("u_VerticalScale", 1.0f);
			foliageMaterial->Set("u_WindSpeed", 1.0f);
		}

		// Our toon shader material
		Material::Sptr toonMaterial = ResourceManager::CreateAsset<Material>(celShader);
		{
			toonMaterial->Name = "Toon"; 
			toonMaterial->Set("u_Material.AlbedoMap", boxTexture);
			toonMaterial->Set("u_Material.NormalMap", normalMapDefault);
			toonMaterial->Set("s_ToonTerm", toonLut);
			toonMaterial->Set("u_Material.Shininess", 0.1f); 
			toonMaterial->Set("u_Material.Steps", 8);
		}


		Material::Sptr displacementTest = ResourceManager::CreateAsset<Material>(displacementShader);
		{
			Texture2D::Sptr displacementMap = ResourceManager::CreateAsset<Texture2D>("textures/displacement_map.png");
			Texture2D::Sptr normalMap       = ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png");
			Texture2D::Sptr diffuseMap      = ResourceManager::CreateAsset<Texture2D>("textures/bricks_diffuse.png");

			displacementTest->Name = "Displacement Map";
			displacementTest->Set("u_Material.AlbedoMap", diffuseMap);
			displacementTest->Set("u_Material.NormalMap", normalMap);
			displacementTest->Set("s_Heightmap", displacementMap);
			displacementTest->Set("u_Material.Shininess", 0.5f);
			displacementTest->Set("u_Scale", 0.1f);
		}

		Material::Sptr grey = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			grey->Name = "Grey";
			grey->Set("u_Material.AlbedoMap", solidGreyTex);
			grey->Set("u_Material.Specular", solidBlackTex);
			grey->Set("u_Material.NormalMap", normalMapDefault);
		}

		Material::Sptr polka = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			polka->Name = "Polka";
			polka->Set("u_Material.AlbedoMap", ResourceManager::CreateAsset<Texture2D>("textures/polka.png"));
			polka->Set("u_Material.Specular", solidBlackTex);
			polka->Set("u_Material.NormalMap", normalMapDefault);
			polka->Set("u_Material.EmissiveMap", ResourceManager::CreateAsset<Texture2D>("textures/polka.png"));
		}

		Material::Sptr whiteBrick = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			whiteBrick->Name = "White Bricks";
			whiteBrick->Set("u_Material.AlbedoMap", ResourceManager::CreateAsset<Texture2D>("textures/displacement_map.png"));
			whiteBrick->Set("u_Material.Specular", solidGrey);
			whiteBrick->Set("u_Material.NormalMap", ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png"));
		}

		Material::Sptr normalmapMat = ResourceManager::CreateAsset<Material>(deferredForward);
		{
			Texture2D::Sptr normalMap       = ResourceManager::CreateAsset<Texture2D>("textures/normal_map.png");
			Texture2D::Sptr diffuseMap      = ResourceManager::CreateAsset<Texture2D>("textures/bricks_diffuse.png");

			normalmapMat->Name = "Tangent Space Normal Map";
			normalmapMat->Set("u_Material.AlbedoMap", diffuseMap);
			normalmapMat->Set("u_Material.NormalMap", normalMap);
			normalmapMat->Set("u_Material.Shininess", 0.5f);
			normalmapMat->Set("u_Scale", 0.1f);
		}

		Material::Sptr multiTextureMat = ResourceManager::CreateAsset<Material>(multiTextureShader);
		{
			Texture2D::Sptr sand  = ResourceManager::CreateAsset<Texture2D>("textures/terrain/sand.png");
			Texture2D::Sptr grass = ResourceManager::CreateAsset<Texture2D>("textures/terrain/grass.png");

			multiTextureMat->Name = "Multitexturing";
			multiTextureMat->Set("u_Material.DiffuseA", sand);
			multiTextureMat->Set("u_Material.DiffuseB", grass);
			multiTextureMat->Set("u_Material.NormalMapA", normalMapDefault);
			multiTextureMat->Set("u_Material.NormalMapB", normalMapDefault);
			multiTextureMat->Set("u_Material.Shininess", 0.5f);
			multiTextureMat->Set("u_Scale", 0.1f); 
		}

		// Create some lights for our scene
		GameObject::Sptr light = scene->CreateGameObject("Light");
		light->SetPosition(glm::vec3(-13, -10.f, 3));
		Light::Sptr lightComponent = light->Add<Light>();
		lightComponent->SetColor(glm::vec3(1.f));
		lightComponent->SetRadius(15.f);
		lightComponent->SetIntensity(18.f);

		// We'll create a mesh that is a simple plane that we can resize later
		MeshResource::Sptr planeMesh = ResourceManager::CreateAsset<MeshResource>();
		planeMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(1.0f)));
		planeMesh->GenerateMesh();

		MeshResource::Sptr sphere = ResourceManager::CreateAsset<MeshResource>();
		sphere->AddParam(MeshBuilderParam::CreateIcoSphere(ZERO, ONE, 5));
		sphere->GenerateMesh();

		// Set up the scene's camera
		GameObject::Sptr camera = scene->MainCamera->GetGameObject()->SelfRef();
		{
			camera->SetPosition({ -9, -6, 15 });
			camera->LookAt(glm::vec3(0.0f));

			camera->Add<SimpleCameraControl>();
			// This is now handled by scene itself!
			//Camera::Sptr cam = camera->Add<Camera>();
			// Make sure that the camera is set as the scene's main camera!
			//scene->MainCamera = cam;
		}

		GameObject::Sptr detachedCam = scene->CreateGameObject("Detached Camera");
		{
			detachedCam->SetPosition(glm::vec3(0.f, 3.5f, 0.4f));

			FirstPersonCamera::Sptr cameraControl = detachedCam->Add<FirstPersonCamera>();

			Camera::Sptr cam = detachedCam->Add<Camera>();
			cam->SetFovDegrees(60);
			scene->MainCamera = cam;
		}

		GameObject::Sptr player = scene->CreateGameObject("Player");
		{
			player->SetPosition(glm::vec3(0.f, 0.f, 4.f));
			player->SetRotation(glm::vec3(0.f, 0.f, 0.f));

			player->SetScale(glm::vec3(1.f));

			RigidBody::Sptr physics = player->Add<RigidBody>(RigidBodyType::Dynamic);
			physics->AddCollider(BoxCollider::Create(glm::vec3(0.2f)))->SetPosition(glm::vec3(0.0f, 0.95f, 0.0f));
			physics->SetAngularFactor(glm::vec3(0.f));
			physics->SetLinearDamping(0.6f);
			physics->SetMass(1.f);

			player->Add<JumpBehaviour>();

			PlayerControl::Sptr controller = player->Add<PlayerControl>();

			player->AddChild(detachedCam);
		}

		GameObject::Sptr enemy = scene->CreateGameObject("Enemy");
		{
			enemy->SetPosition(glm::vec3(-28, -0.1, 11));

			MeshResource::Sptr cube = ResourceManager::CreateAsset<MeshResource>();
			cube->AddParam(MeshBuilderParam::CreateCube(ZERO, ONE));
			cube->GenerateMesh();

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = enemy->Add<RenderComponent>();
			renderer->SetMesh(ballMesh);
			renderer->SetMaterial(ballMaterial);

			enemy->Add<EnemyBehaviour>();

			// Example of a trigger that interacts with static and kinematic bodies as well as dynamic bodies
			TriggerVolume::Sptr trigger = enemy->Add<TriggerVolume>();
			trigger->SetFlags(TriggerTypeFlags::Dynamics);
			trigger->AddCollider(BoxCollider::Create(glm::vec3(2.0f)));

			enemy->Add<TriggerVolumeEnterBehaviour>();
		}

		GameObject::Sptr snake = scene->CreateGameObject("Snake");
		{
			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = snake->Add<RenderComponent>();
			renderer->SetMesh(snakeMesh);
			renderer->SetMaterial(snakeMaterial);

			snake->SetPosition(glm::vec3(-35, 0.f, 2.f));
			snake->SetScale(glm::vec3(3.f));
			snake->SetRotation(glm::vec3(90.f, 0.f, 0.f));
		}

		GameObject::Sptr tree = scene->CreateGameObject("Tree");
		{
			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = tree->Add<RenderComponent>();
			renderer->SetMesh(treeMesh);
			renderer->SetMaterial(treeMaterial);

			tree->SetPosition(glm::vec3(-28.f, -40.0f, 0.0f));
			tree->SetScale(glm::vec3(3.f));
			tree->SetRotation(glm::vec3(90.f, 0.f, 0.f));
		}

		// Set up all our sample objects
		GameObject::Sptr plane = scene->CreateGameObject("Plane");
		{
			// Make a big tiled mesh
			MeshResource::Sptr tiledMesh = ResourceManager::CreateAsset<MeshResource>();
			tiledMesh->AddParam(MeshBuilderParam::CreatePlane(ZERO, UNIT_Z, UNIT_X, glm::vec2(100.0f), glm::vec2(50.0f)));
			tiledMesh->GenerateMesh();

			// Create and attach a RenderComponent to the object to draw our mesh
			RenderComponent::Sptr renderer = plane->Add<RenderComponent>();
			renderer->SetMesh(tiledMesh);
			renderer->SetMaterial(boxMaterial);

			// Attach a plane collider that extends infinitely along the X/Y axis
			RigidBody::Sptr physics = plane->Add<RigidBody>(/*static by default*/);
			physics->AddCollider(BoxCollider::Create(glm::vec3(50.0f, 50.0f, 1.0f)))->SetPosition({ 0,0,-1 });
		}

		GameObject::Sptr winTrigger = scene->CreateGameObject("Win Trigger");
		{
			// Set position in the scene
			winTrigger->SetPosition(glm::vec3(-40.f, -30.0f, 2.0f));

			// Example of a trigger that interacts with static and kinematic bodies as well as dynamic bodies
			TriggerVolume::Sptr trigger = winTrigger->Add<TriggerVolume>();
			trigger->SetFlags(TriggerTypeFlags::Dynamics);
			trigger->AddCollider(BoxCollider::Create(glm::vec3(2.0f)));

			winTrigger->Add<TriggerVolumeEnterBehaviour>();
		}

		GameObject::Sptr shadowCaster = scene->CreateGameObject("Shadow Light");
		{
			// Set position in the scene
			shadowCaster->SetPosition(glm::vec3(22.0f, -31.0f, 33.f));
			shadowCaster->SetRotation(glm::vec3(28, 1, 60));

			// Create and attach a renderer for the monkey
			ShadowCamera::Sptr shadowCam = shadowCaster->Add<ShadowCamera>();
			shadowCam->SetProjection(glm::perspective(glm::radians(120.0f), 1.0f, 0.1f, 100.0f));
		}

		GameObject::Sptr ballParticles = scene->CreateGameObject("Particles"); 
		{
			ballParticles->SetPosition({ 0.0f, 0.0f, 0.0f });

			ParticleSystem::Sptr particleManager = ballParticles->Add<ParticleSystem>();  
			particleManager->Atlas = particleTex;

			ParticleSystem::ParticleData emitter;
			emitter.Type = ParticleType::SphereEmitter;
			emitter.TexID = 4;
			emitter.Position = glm::vec3(0.0f);
			emitter.Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			emitter.Lifetime = 0.0f;
			emitter.SphereEmitterData.Timer = 1.0f / 30.0f;
			emitter.SphereEmitterData.Velocity = 0.5f;
			emitter.SphereEmitterData.LifeRange = { 2.5f, 4.5f };
			emitter.SphereEmitterData.Radius = 2.38f;
			emitter.SphereEmitterData.SizeRange = { 0.5f, 2.f };

			particleManager->AddEmitter(emitter);

			enemy->AddChild(ballParticles);
			

		}

		GameObject::Sptr snakeParticles = scene->CreateGameObject("snakeFire");
		{
			snakeParticles->SetPosition({-28,-0.1,11});
			snakeParticles->SetRotation({ 0,90,0});

			ParticleSystem::Sptr particleManager = snakeParticles->Add<ParticleSystem>();
			particleManager->Atlas = particleTex;
			particleManager->_gravity = glm::vec3(0);
			ParticleSystem::ParticleData emitter;
			emitter.Type = ParticleType::ConeEmitter;
			emitter.TexID = 2;
			emitter.Position = glm::vec3(0.0f);
			emitter.Color = glm::vec4(1.f, 0.1f, 0.0f, 1.0f);
			emitter.Lifetime = 0.0f;


			emitter.ConeEmitterData.Velocity = glm::vec3(0, 0, 2.f);
			emitter.ConeEmitterData.Angle = 40.f;
			emitter.ConeEmitterData.Timer = 1.0f / 3.0f;
			emitter.ConeEmitterData.SizeRange = { 3.5, 5.5 };
			emitter.ConeEmitterData.LifeRange = { 1.0f, 5.0f };


			particleManager->AddEmitter(emitter);
		}

		GameObject::Sptr treeParticles = scene->CreateGameObject("leaves");
		{
			treeParticles->SetPosition({ -30,-26.5, 21.5 });
			treeParticles->SetRotation({ -177,0,0 });

			ParticleSystem::Sptr particleManager = treeParticles->Add<ParticleSystem>();
			particleManager->Atlas = particleTex;
			particleManager->_gravity = glm::vec3(0,0,-9.81);
			ParticleSystem::ParticleData emitter;
			emitter.Type = ParticleType::ConeEmitter;
			emitter.TexID = 1;
			emitter.Position = glm::vec3(0.0f);
			emitter.Color = glm::vec4(1.f, 0.5f, 0.0f, 1.0f);
			emitter.Lifetime = 0.0f;


			emitter.ConeEmitterData.Velocity = glm::vec3(0, 0, -2.f);
			emitter.ConeEmitterData.Angle = 90.f;
			emitter.ConeEmitterData.Timer = 1.0f / 3.0f;
			emitter.ConeEmitterData.SizeRange = { 1.5, 3.5 };
			emitter.ConeEmitterData.LifeRange = { 3.0f, 7.0f };


			particleManager->AddEmitter(emitter);
		}


		//UI
		GameObject::Sptr winText = scene->CreateGameObject("Win Text");
		{
			RectTransform::Sptr transform = winText->Add<RectTransform>();
			transform->SetMin({ 0, 0 });
			transform->SetMax({ app.GetWindowSize().x, app.GetWindowSize().y });

			GuiPanel::Sptr panel = winText->Add<GuiPanel>();
			panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/win_screen.png"));

			panel->SetTransparency(0.0f);
		}
		GameObject::Sptr loseText = scene->CreateGameObject("Lose Text");
		{
			RectTransform::Sptr transform = loseText->Add<RectTransform>();
			transform->SetMin({ 0, 0 });
			transform->SetMax({ app.GetWindowSize().x, app.GetWindowSize().y });

			GuiPanel::Sptr panel = loseText->Add<GuiPanel>();
			panel->SetTexture(ResourceManager::CreateAsset<Texture2D>("textures/lose_screen.png"));

			panel->SetTransparency(0.0f);
		}


		GuiBatcher::SetDefaultTexture(ResourceManager::CreateAsset<Texture2D>("textures/ui-sprite.png"));
		GuiBatcher::SetDefaultBorderRadius(8);

		// Save the asset manifest for all the resources we just loaded
		ResourceManager::SaveManifest("scene-manifest.json");
		// Save the scene to a JSON file
		scene->Save("scene.json");

		// Send the scene to the application
		app.LoadScene(scene);

		_active = true;
	}
}
