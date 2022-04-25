#pragma once
#include "IComponent.h"
#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/GameObject.h"

class EnemyBehaviour : public Gameplay::IComponent {
public:
	typedef std::shared_ptr<EnemyBehaviour> Sptr;

	std::weak_ptr<Gameplay::IComponent> Panel;

	EnemyBehaviour();
	virtual ~EnemyBehaviour();

	virtual void Awake() override;
	virtual void Update(float deltaTime) override;

public:
	virtual void RenderImGui() override;
	MAKE_TYPENAME(EnemyBehaviour);
	virtual nlohmann::json ToJson() const override;
	static EnemyBehaviour::Sptr FromJson(const nlohmann::json& blob);

protected:
	float speed;
	Gameplay::GameObject::Sptr player;
};