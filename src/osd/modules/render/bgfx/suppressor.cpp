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

bgfx_slider_suppressor::bgfx_slider_suppressor(std::vector<bgfx_slider*> sliders, uint32_t condition, combine_mode combine, std::vector<float> values)
	: bgfx_suppressor(condition, combine)
	, m_sliders(sliders)
	, m_values(values)
{
}

bgfx_slider_suppressor::~bgfx_slider_suppressor()
{
}

bool bgfx_slider_suppressor::suppress(uint32_t screen_index)
{
	int32_t value_count;
	switch (m_sliders[0]->type())
	{
	case bgfx_slider::slider_type::SLIDER_VEC2:
		value_count = 2;
		break;
	case bgfx_slider::slider_type::SLIDER_COLOR:
		value_count = 3;
		break;
	default:
		value_count = 1;
		break;
	}

	for (int32_t i = 0; i < value_count; i++)
	{
		// check if any value does not meet the condition
		switch (m_condition)
		{
		case CONDITION_NOT_EQUAL:
			return !(std::abs(m_sliders[i]->value() - m_values[i]) < 0.00001f);
		case CONDITION_EQUAL:
			return !(std::abs(m_sliders[i]->value() - m_values[i]) > 0.00001f);
		case CONDITION_LESS:
			return !(m_sliders[i]->value() >= m_values[i]);
		case CONDITION_LESS_EQUAL:
			return !(m_sliders[i]->value() > m_values[i]);
		case CONDITION_GREATER:
			return !(m_sliders[i]->value() <= m_values[i]);
		case CONDITION_GREATER_EQUAL:
			return !(m_sliders[i]->value() < m_values[i]);
		default:
			return false;
		}
	}

	return true; // all values meet the condition
}


//============================================================
//  suppressor depending on screen types
//============================================================

bgfx_screen_suppressor::bgfx_screen_suppressor(running_machine& machine, uint32_t condition, combine_mode combine, bgfx_slider::screen_type screen_type)
	: bgfx_suppressor(condition, combine)
	, m_machine(machine)
	, m_screen_type(screen_type)
{
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
		case CONDITION_NOT_EQUAL:
			return !bgfx_slider::screen_type_equals(m_screen_type, screen_type);
		case CONDITION_EQUAL:
			return bgfx_slider::screen_type_equals(m_screen_type, screen_type);
		default:
			return false;
	}
}