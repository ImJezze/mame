// license:BSD-3-Clause
// copyright-holders:Ryan Holtz
//============================================================
//
//  suppressorreader.cpp - Reads pass-skipping conditions
//
//============================================================

#include "emu.h"
#include <string>

#include "suppressorreader.h"

#include "suppressor.h"
#include "slider.h"
#include "sliderreader.h"

const suppressor_reader::string_to_enum suppressor_reader::CONDITION_NAMES[suppressor_reader::CONDITION_COUNT] = {
	{ "equal",        bgfx_suppressor::condition_type::CONDITION_EQUAL },
	{ "notequal",     bgfx_suppressor::condition_type::CONDITION_NOT_EQUAL },
	{ "less",         bgfx_suppressor::condition_type::CONDITION_LESS },
	{ "lessequal",    bgfx_suppressor::condition_type::CONDITION_LESS_EQUAL },
	{ "greater",      bgfx_suppressor::condition_type::CONDITION_GREATER },
	{ "greaterequal", bgfx_suppressor::condition_type::CONDITION_GREATER_EQUAL }
};

const suppressor_reader::string_to_enum suppressor_reader::COMBINE_NAMES[suppressor_reader::COMBINE_COUNT] = {
	{ "and", bgfx_suppressor::combine_mode::COMBINE_AND },
	{ "or",  bgfx_suppressor::combine_mode::COMBINE_OR }
};

bgfx_suppressor* suppressor_reader::read_from_value(const Value& value, std::string prefix, chain_manager& chains, std::map<std::string, bgfx_slider*>& sliders)
{
	if (!validate_parameters(value, prefix))
	{
		return nullptr;
	}

	uint32_t condition = uint32_t(get_enum_from_value(value, "condition", bgfx_suppressor::condition_type::CONDITION_EQUAL, CONDITION_NAMES, CONDITION_COUNT));
	bgfx_suppressor::combine_mode mode = bgfx_suppressor::combine_mode(get_enum_from_value(value, "combine", bgfx_suppressor::combine_mode::COMBINE_OR, COMBINE_NAMES, COMBINE_COUNT));

	std::string type = value["type"].GetString();
	if (type == "slider")
	{
		std::string name = value["name"].GetString();
		std::vector<bgfx_slider*> check_sliders;
		check_sliders.push_back(sliders[name + "0"]);

		int slider_count;
		switch (check_sliders[0]->type())
		{
		case bgfx_slider::slider_type::SLIDER_VEC2:
			slider_count = 2;
			break;
		case bgfx_slider::slider_type::SLIDER_COLOR:
			slider_count = 3;
			break;
		default:
			slider_count = 1;
			break;
		}

		std::vector<float> values;
		if (slider_count > 1)
		{
			get_values(value, prefix, "value", values, slider_count);
			if (!READER_CHECK(slider_count == value["value"].GetArray().Size(), (prefix + "Expected " + std::to_string(slider_count) + " values, got " + std::to_string(value["value"].GetArray().Size()) + "\n").c_str())) return nullptr;
			for (int index = 1; index < slider_count; index++)
			{
				check_sliders.push_back(sliders[name + std::to_string(index)]);
			}
		}
		else
		{
			values.push_back(value["value"].GetFloat());
		}

		return new bgfx_slider_suppressor(check_sliders, condition, mode, values);
	}
	else if (type == "screen")
	{
		bgfx_slider::screen_type screen_type = bgfx_slider::screen_type(slider_reader::read_screen_type_from_value(value));
		return new bgfx_screen_suppressor(chains.machine(), condition, mode, screen_type);
	}

	return nullptr;
}

bool suppressor_reader::validate_parameters(const Value& value, std::string prefix)
{
	if (!READER_CHECK(value.HasMember("type"), (prefix + "Must have string value 'type'\n").c_str())) return false;
	if (!READER_CHECK(value["type"].IsString(), (prefix + "Value 'type' must be a string\n").c_str())) return false;
	if (!READER_CHECK(std::string(value["type"].GetString()) == "screen" || value.HasMember("name"), (prefix + "Must have string value 'name'\n").c_str())) return false;
	if (!READER_CHECK(std::string(value["type"].GetString()) == "screen" || value["name"].IsString(), (prefix + "Value 'name' must be a string\n").c_str())) return false;
	if (!READER_CHECK(value.HasMember("value"), (prefix + "Must have numeric or array value 'value'\n").c_str())) return false;
	if (!READER_CHECK(value["value"].IsNumber() || value["value"].IsArray(), (prefix + "Value 'value' must be a number or an array the size of the corresponding slider type\n").c_str())) return false;
	return true;
}

bool suppressor_reader::get_values(const Value& value, std::string prefix, std::string name, std::vector<float>& values, const int count)
{
	const char* name_str = name.c_str();
	const Value& value_array = value[name_str];
	for (uint32_t i = 0; i < value_array.Size() && i < count; i++)
	{
		if (!READER_CHECK(value_array[i].IsNumber(), (prefix + "Value 'value[" + std::to_string(i) + "]' must be a number\n").c_str())) return false;
		values.push_back(value_array[i].GetFloat());
	}
	return true;
}
