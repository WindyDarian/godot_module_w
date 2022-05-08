// Copyright (c) 2019 Windy Darian. MIT License.
// Custom script library for things.

#include "register_types.h"

#include "w.h"

static _W* s_w = nullptr;

void initialize_w_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) 
	{
		return;
	}

	ClassDB::register_class<_W>();
	s_w = memnew(_W);
    Engine::get_singleton()->add_singleton(Engine::Singleton("W", _W::get_singleton()));
}

void uninitialize_w_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) 
	{
		return;
	}

	if (s_w)
	{
		memdelete(s_w);
		s_w = nullptr;
	}
}
