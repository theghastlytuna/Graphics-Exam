#include "Gameplay/Components/EnemyBehaviour.h"
#include <GLFW/glfw3.h>
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/ImGuiHelper.h"
#include "Gameplay/InputEngine.h"

void EnemyBehaviour::Awake()
{
	player = GetGameObject()->GetScene()->FindObjectByName("Player");
}

void EnemyBehaviour::RenderImGui() {
}

nlohmann::json EnemyBehaviour::ToJson() const {
	return {
	};
}

EnemyBehaviour::EnemyBehaviour() :
	IComponent(),
	speed(0.02f)
{ }

EnemyBehaviour::~EnemyBehaviour() = default;

EnemyBehaviour::Sptr EnemyBehaviour::FromJson(const nlohmann::json& blob) {
	EnemyBehaviour::Sptr result = std::make_shared<EnemyBehaviour>();
	return result;
}

void EnemyBehaviour::Update(float deltaTime) 
{
	glm::vec3 direction = glm::normalize(player->GetPosition() + glm::vec3(0.f, 0.f, 3.f) - GetGameObject()->GetPosition());

	GetGameObject()->SetPosition(GetGameObject()->GetPosition() + ((direction) * speed));
}
