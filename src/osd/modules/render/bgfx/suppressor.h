// license:BSD-3-Clause
// copyright-holders:Ryan Holtz
//============================================================
//
//  suppressor.h - Conditionally suppress a bgfx chain entry
//  from being processed.
//
//============================================================

#pragma once

#ifndef __DRAWBGFX_SUPPRESSOR__
#define __DRAWBGFX_SUPPRESSOR__

#include <bgfx/bgfx.h>
#include <rapidjson/document.h>

#include <vector>

#include "slider.h"

using namespace rapidjson;

class bgfx_slider;


//============================================================
//  base suppressor
//============================================================

class bgfx_suppressor
{
public:
	enum condition_type
	{
		CONDITION_EQUAL,
		CONDITION_NOT_EQUAL,
		CONDITION_LESS,
		CONDITION_LESS_EQUAL,
		CONDITION_GREATER,
		CONDITION_GREATER_EQUAL,

		CONDITION_COUNT
	};

	enum combine_mode {
		COMBINE_AND,
		COMBINE_OR
	};

	bgfx_suppressor(uint32_t condition, combine_mode combine);
	~bgfx_suppressor();

	// Getters
	virtual bool suppress(uint32_t screen_index) = 0;
	combine_mode combine() const { return m_combine; }

protected:
	uint32_t                    m_condition;
	combine_mode                m_combine;
};


//============================================================
//  suppressor depending on sliders
//============================================================

class bgfx_slider_suppressor : public bgfx_suppressor
{
public:
	bgfx_slider_suppressor(std::vector<bgfx_slider*> sliders, uint32_t condition, combine_mode combine, std::vector<float> values);
	~bgfx_slider_suppressor();

	// Getters
	bool suppress(uint32_t screen_index);
	
private:
	std::vector<bgfx_slider*>   m_sliders;
	std::vector<float>          m_values;
};


//============================================================
//  suppressor depending on screen types
//============================================================

class bgfx_screen_suppressor : public bgfx_suppressor
{
public:
	bgfx_screen_suppressor(running_machine& machine, uint32_t condition, combine_mode combine, bgfx_slider::screen_type screen_type);
	~bgfx_screen_suppressor();

	// Getters
	bool suppress(uint32_t screen_index);

private:
	bgfx_slider::screen_type    m_screen_type;
	running_machine&			m_machine;
};

#endif // __DRAWBGFX_SUPPRESSOR__
