#include "Gameplay/Components/FirstPersonCamera.h"
#include <GLFW/glfw3.h>
#define  GLM_SWIZZLE
#include <GLM/gtc/quaternion.hpp>

#include "Gameplay/GameObject.h"
#include "Gameplay/Scene.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/ImGuiHelper.h"
#include "Gameplay/GameObject.h"

#include "Gameplay/Physics/RigidBody.h"
#include "Application/Application.h"
#include "Gameplay/InputEngine.h"

FirstPersonCamera::FirstPersonCamera()
	: IComponent(),
	_mouseSensitivity({ 0.2f, 0.2f }),
	_moveSpeeds(glm::vec3(10.0f)),
	_shiftMultipler(2.0f),
	_currentRot(glm::vec2(0.0f, 180.0f)),
	_isMousePressed(false)
{}

FirstPersonCamera::~FirstPersonCamera() = default;

void FirstPersonCamera::Awake()
{
	_window = Application::Get().GetWindow();
	
}

void FirstPersonCamera::Update(float deltaTime)
{
		bool turnUp;
		bool turnDown;

		turnUp = InputEngine::GetKeyState(GLFW_KEY_UP) == ButtonState::Down;
		turnDown = InputEngine::GetKeyState(GLFW_KEY_DOWN) == ButtonState::Down;

		//Since controller joysticks are physical, they often won't be perfect, meaning at a neutral state it might still have slight movement.
		//Check to make sure that the axes aren't outputting an extremely small number, and if they are then set the input to 0.
		if (turnUp) _currentRot.y -= static_cast<float>(0.5f);
		if (turnDown) _currentRot.y += static_cast<float>(0.5f);

		//_currentRot.y = std::clamp(_currentRot.y, 90.0f, 270.0f);

		glm::quat rotX = glm::angleAxis(glm::radians(180.0f), glm::vec3(0, 0, 1));
		glm::quat rotY = glm::angleAxis(glm::radians(-_currentRot.y), glm::vec3(1, 0, 0));


		glm::quat currentRot = rotX * rotY;

		GetGameObject()->SetRotation(currentRot);
}

void FirstPersonCamera::RenderImGui()
{
	LABEL_LEFT(ImGui::DragFloat2, "Mouse Sensitivity", &_mouseSensitivity.x, 0.01f);
	LABEL_LEFT(ImGui::DragFloat3, "Move Speed       ", &_moveSpeeds.x, 0.01f, 0.01f);
	LABEL_LEFT(ImGui::DragFloat, "Shift Multiplier ", &_shiftMultipler, 0.01f, 1.0f);
}

nlohmann::json FirstPersonCamera::ToJson() const
{
	return nlohmann::json();
}

FirstPersonCamera::Sptr FirstPersonCamera::FromJson(const nlohmann::json& blob)
{
	return FirstPersonCamera::Sptr();
}
