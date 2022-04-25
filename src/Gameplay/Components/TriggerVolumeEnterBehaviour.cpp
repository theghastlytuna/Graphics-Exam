#include "Gameplay/Components/TriggerVolumeEnterBehaviour.h"
#include "Gameplay/Components/ComponentManager.h"
#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Gameplay/Components/GUI/GuiPanel.h"

TriggerVolumeEnterBehaviour::TriggerVolumeEnterBehaviour() :
	IComponent()
{ }
TriggerVolumeEnterBehaviour::~TriggerVolumeEnterBehaviour() = default;


void TriggerVolumeEnterBehaviour::OnTriggerVolumeEntered(const std::shared_ptr<Gameplay::Physics::RigidBody>& body)
{
	if (GetGameObject()->Name == "Win Trigger" && body->GetGameObject()->Name == "Player")
	{
		GetGameObject()->GetScene()->FindObjectByName("Win Text")->Get<GuiPanel>()->SetTransparency(1.0f);
	}
	if (GetGameObject()->Name == "Enemy" && body->GetGameObject()->Name == "Player")
	{
		GetGameObject()->GetScene()->FindObjectByName("Lose Text")->Get<GuiPanel>()->SetTransparency(1.0f);
	}

	LOG_INFO("Body has entered {} trigger volume: {}", GetGameObject()->Name, body->GetGameObject()->Name);
	_playerInTrigger = true;
}

void TriggerVolumeEnterBehaviour::OnTriggerVolumeLeaving(const std::shared_ptr<Gameplay::Physics::RigidBody>& body) {
	LOG_INFO("Body has left {} trigger volume: {}", GetGameObject()->Name, body->GetGameObject()->Name);
	_playerInTrigger = false;
}

void TriggerVolumeEnterBehaviour::RenderImGui() { }

nlohmann::json TriggerVolumeEnterBehaviour::ToJson() const {
	return { };
}

TriggerVolumeEnterBehaviour::Sptr TriggerVolumeEnterBehaviour::FromJson(const nlohmann::json& blob) {
	TriggerVolumeEnterBehaviour::Sptr result = std::make_shared<TriggerVolumeEnterBehaviour>();
	return result;
}
