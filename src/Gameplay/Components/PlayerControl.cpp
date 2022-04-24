#include "Gameplay/Components/PlayerControl.h"
#include <GLFW/glfw3.h>
#define  GLM_SWIZZLE
#include <GLM/gtc/quaternion.hpp>

#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/ImGuiHelper.h"
#include "Gameplay/Components/Camera.h"

#include "Gameplay/Physics/RigidBody.h"
#include "Application/Application.h"

#include "Gameplay/InputEngine.h"
#include "Gameplay/Components/Camera.h"

PlayerControl::PlayerControl()
	: IComponent(),
	_mouseSensitivity({ 0.2f, 0.2f }),
	_moveSpeeds(glm::vec3(6.0f)),
	_shiftMultipler(2.0f),
	_currentRot(glm::vec2(0.0f)),
	_isMousePressed(false),
	_isMoving(false),
	_isSprinting(false),
	_spintVal(2.0f),
	_keyboardSensitivity({ 1.5f, 1.5f })
{ }

PlayerControl::~PlayerControl() = default;

void PlayerControl::Awake()
{
	_window = Application::Get().GetWindow();

	_camera = GetGameObject()->Get<Gameplay::Camera>();
	//IMPORTANT: This only works because the detachedCam is the only child of the player. If anything to do with children or the detached cam changes, this might break
}

void PlayerControl::Update(float deltaTime)
{
	_isMoving = false;
	_justThrew = false;

	bool moveLeft = InputEngine::GetKeyState(GLFW_KEY_A) == ButtonState::Down;
	bool moveRight = InputEngine::GetKeyState(GLFW_KEY_D) == ButtonState::Down;
	bool moveForward = InputEngine::GetKeyState(GLFW_KEY_W) == ButtonState::Down;
	bool moveBack = InputEngine::GetKeyState(GLFW_KEY_S) == ButtonState::Down;

	bool turnRight = InputEngine::GetKeyState(GLFW_KEY_RIGHT) == ButtonState::Down;
	bool turnLeft = InputEngine::GetKeyState(GLFW_KEY_LEFT) == ButtonState::Down;

	bool sprint = InputEngine::GetKeyState(GLFW_KEY_LEFT_SHIFT) == ButtonState::Down;
	bool wangin = InputEngine::GetKeyState(GLFW_KEY_KP_0) == ButtonState::Down ||
		InputEngine::GetKeyState(GLFW_KEY_RIGHT_CONTROL) == ButtonState::Down;

	/*
	if (!InputEngine::GetEnabled())
	{
		moveLeft = false;
		moveRight = false;
		moveForward = false;
		moveBack = false;
		turnRight = false;
		turnLeft = false;
		sprint = false;
	}
	*/

	_isSprinting = sprint;

	if (turnRight) _currentRot.x -= static_cast<float>(0.5f) * _keyboardSensitivity.x;
	if (turnLeft) _currentRot.x += static_cast<float>(0.5f) * _keyboardSensitivity.x;
	glm::quat rotX = glm::angleAxis(glm::radians(_currentRot.x), glm::vec3(0, 0, 1));
	glm::quat rotY = glm::angleAxis(glm::radians(90.f), glm::vec3(1, 0, 0));
	glm::quat currentRot = rotX * rotY;
	GetGameObject()->SetRotation(currentRot);

	glm::vec3 input = glm::vec3(0.0f);
	if (moveForward) {
		input.z += _moveSpeeds.x;
		_isMoving = true;
	}
	if (moveBack) {
		input.z -= _moveSpeeds.x;
		_isMoving = true;
	}
	if (moveLeft) {
		input.x += _moveSpeeds.y;
		_isMoving = true;
	}
	if (moveRight) {
		input.x -= _moveSpeeds.y;
		_isMoving = true;
	}

	input *= deltaTime;

	glm::vec3 worldMovement = glm::vec3((currentRot * glm::vec4(input, 1.0f)).x, (currentRot * glm::vec4(input, 1.0f)).y, 0.0f);

	if (worldMovement != glm::vec3(0.0f))
	{
		worldMovement = 10.0f * glm::normalize(worldMovement);

		_timeBetStep += deltaTime;
	}


	else _timeBetStep = 1.5f;
	GetGameObject()->Get<Gameplay::Physics::RigidBody>()->ApplyForce(worldMovement);

	if (GetGameObject()->GetPosition().z < -5) {
		GetGameObject()->SetPosition({ 2,-2,4 });
	}

		GetGameObject()->GetChildren()[0]->Get<Gameplay::Camera>()->SetFovDegrees(_initialFov);

}

bool PlayerControl::IsMoving()
{
	return _isMoving;
}

bool PlayerControl::IsSprinting()
{
	return _isSprinting;
}

bool PlayerControl::GetJustThrew()
{
	return _justThrew;
}

void PlayerControl::RenderImGui()
{
}

nlohmann::json PlayerControl::ToJson() const
{
	return nlohmann::json();
}

PlayerControl::Sptr PlayerControl::FromJson(const nlohmann::json& blob)
{
	return PlayerControl::Sptr();
}