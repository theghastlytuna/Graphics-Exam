#pragma once
#include "Application/ApplicationLayer.h"
#include "json.hpp"

/**
 * This example layer handles creating a default test scene, which we will use 
 * as an entry point for creating a sample scene
 */
class DefaultSceneLayer final : public ApplicationLayer {
public:
	MAKE_PTRS(DefaultSceneLayer)

	DefaultSceneLayer();
	virtual ~DefaultSceneLayer();
	virtual void RepositionUI() override;

	// Inherited from ApplicationLayer
	void DefaultSceneLayer::SetActive(bool active);
	bool DefaultSceneLayer::IsActive();

	virtual void OnAppLoad(const nlohmann::json& config) override;

protected:
	void _CreateScene();
	bool _active;

};