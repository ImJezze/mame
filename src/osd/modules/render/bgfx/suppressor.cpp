// license:BSD-3-Clause
// copyright-holders:Ryan Holtz
//============================================================
//
//  suppressor.cpp - Conditionally suppress a bgfx chain entry
//  from being processed.
//
//============================================================

#include "emu.h"
#include "screen.h"
#include "suppressor.h"

#include "sliderreader.h"


//============================================================
//  base suppressor
//============================================================

bgfx_suppressor::bgfx_suppressor(uint32_t condition, combine_mode combine)
	: m_condition(condition)
	, m_combine(combine)
{
}

bgfx_suppressor::~bgfx_suppressor()
{
}


//============================================================
//  suppressor depending on sliders
//============================================================

bgfx_slider_suppressor::bgfx_slider_suppressor(std::vector<bgfx_slider*> sliders, uint32_t condition, combine_mode combine, void* value)
	: bgfx_suppressor(condition, combine)
	, m_sliders(sliders)
	, m_value(nullptr)
{
	uint32_t size = sliders[0]->size();
	m_value = new uint8_t[size];
	memcpy(m_value, value, size);
}

bgfx_slider_suppressor::~bgfx_slider_suppressor()
{
	delete[] m_value;
}

bool bgfx_slider_suppressor::suppress(uint32_t screen_index)
{
	int32_t count = 1;
	if (m_sliders[0]->type() == bgfx_slider::slider_type::SLIDER_VEC2)
	{
		count = 2;
	}
	else if (m_sliders[0]->type() == bgfx_slider::slider_type::SLIDER_COLOR)
	{
		count = 3;
	}

	float current_values[3];
	for (int32_t index = 0; index < count; index++)
	{
		current_values[index] = m_sliders[index]->value();
	}

	switch (m_condition)
	{
		case CONDITION_NOTEQUAL:
			return memcmp(m_value, current_values, m_sliders[0]->size()) != 0;
		case CONDITION_EQUAL:
			return memcmp(m_value, current_values, m_sliders[0]->size()) == 0;
		default:
			return false;
	}
}


//============================================================
//  suppressor depending on screen types
//============================================================

bgfx_screen_suppressor::bgfx_screen_suppressor(running_machine& machine, uint32_t condition, combine_mode combine, const Value& value)
	: bgfx_suppressor(condition, combine)
	, m_machine(machine)
{
	m_screen_type = bgfx_slider::screen_type(slider_reader::read_screen_type_from_value(value));
}

bgfx_screen_suppressor::~bgfx_screen_suppressor()
{
}

bool bgfx_screen_suppressor::suppress(uint32_t screen_index)
{
	screen_type_enum screen_type = SCREEN_TYPE_INVALID;
	const screen_device *screen_device = screen_device_iterator(m_machine.root_device()).byindex(screen_index);
	if (screen_device != nullptr)
	{
		screen_type = screen_device->screen_type();
	}

	switch (m_condition)
	{
		case CONDITION_NOTEQUAL:
			return !bgfx_slider::screen_type_equals(m_screen_type, screen_type);
		case CONDITION_EQUAL:
			return bgfx_slider::screen_type_equals(m_screen_type, screen_type);
		default:
			return false;
	}
}