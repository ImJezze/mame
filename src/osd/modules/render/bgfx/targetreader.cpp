// license:BSD-3-Clause
// copyright-holders:Ryan Holtz
//============================================================
//
//  targetreader.cpp - BGFX target JSON reader
//
//============================================================

#include <string>

#include "emu.h"
#include <modules/lib/osdobj_common.h>

#include "chainmanager.h"
#include "targetreader.h"
#include "target.h"

const target_reader::string_to_enum target_reader::STYLE_NAMES[target_reader::STYLE_COUNT] = {
	{ "guest",  TARGET_STYLE_GUEST },
	{ "native", TARGET_STYLE_NATIVE },
	{ "custom", TARGET_STYLE_CUSTOM }
};

const target_reader::string_to_enum target_reader::FORMAT_NAMES[target_reader::FORMAT_COUNT] = {
	{ "RGBA8",  bgfx::TextureFormat::RGBA8 },
	{ "RGBA16", bgfx::TextureFormat::RGBA16 }
};

bgfx_target* target_reader::read_from_value(const Value& value, std::string prefix, chain_manager& chains, uint32_t screen_index)
{
	if (!validate_parameters(value, prefix))
	{
		return nullptr;
	}

	std::string target_name = value["name"].GetString();
	uint32_t mode = uint32_t(get_enum_from_value(value, "mode", TARGET_STYLE_NATIVE, STYLE_NAMES, STYLE_COUNT));
	uint32_t format = uint32_t(get_enum_from_value(value, "format", bgfx::TextureFormat::RGBA8, FORMAT_NAMES, FORMAT_COUNT));
	bool bilinear = get_bool(value, "bilinear", true);
	bool double_buffer = get_bool(value, "doublebuffer", true);
	double scale = 1;
	if (value.HasMember("scale"))
	{
		scale = value["scale"].GetDouble();
	}

	uint16_t width = 0;
	uint16_t height = 0;
	switch (mode)
	{
		case TARGET_STYLE_GUEST:
			width = chains.targets().width(TARGET_STYLE_GUEST, screen_index);
			height = chains.targets().height(TARGET_STYLE_GUEST, screen_index);
			break;
		case TARGET_STYLE_NATIVE:
			width = chains.targets().width(TARGET_STYLE_NATIVE, screen_index);
			height = chains.targets().height(TARGET_STYLE_NATIVE, screen_index);
			break;
		case TARGET_STYLE_CUSTOM:
			if (!READER_CHECK(value.HasMember("width"), (prefix + "Target '" + target_name + "': Must have numeric value 'width'\n").c_str())) return nullptr;
			if (!READER_CHECK(value["width"].IsNumber(), (prefix + "Target '" + target_name + "': Value 'width' must be a number\n").c_str())) return nullptr;
			if (!READER_CHECK(value.HasMember("height"), (prefix + "Target '" + target_name + "': Must have numeric value 'height'\n").c_str())) return nullptr;
			if (!READER_CHECK(value["height"].IsNumber(), (prefix + "Target '" + target_name + "': Value 'height' must be a number\n").c_str())) return nullptr;
			width = uint16_t(value["width"].GetDouble());
			height = uint16_t(value["height"].GetDouble());
			break;
	}

	bgfx::TextureFormat::Enum target_format = static_cast<bgfx::TextureFormat::Enum>(format);

	return chains.targets().create_target(target_name, target_format, width, height, mode, double_buffer, bilinear, scale, scale, screen_index);
}

bool target_reader::validate_parameters(const Value& value, std::string prefix)
{
	if (!READER_CHECK(value.HasMember("name"), (prefix + "Must have string value 'name'\n").c_str())) return false;
	if (!READER_CHECK(value["name"].IsString(), (prefix + "Value 'name' must be a string\n").c_str())) return false;
	if (!READER_CHECK(value.HasMember("mode"), (prefix + "Must have string enum 'mode'\n").c_str())) return false;
	if (!READER_CHECK(value["mode"].IsString(), (prefix + "Value 'mode' must be a string (what screens does this apply to?)\n").c_str())) return false;
	if (!READER_CHECK(!value.HasMember("bilinear") || value["bilinear"].IsBool(), (prefix + "Value 'bilinear' must be a boolean\n").c_str())) return false;
	if (!READER_CHECK(!value.HasMember("doublebuffer") || value["doublebuffer"].IsBool(), (prefix + "Value 'doublebuffer' must be a boolean\n").c_str())) return false;
	if (!READER_CHECK(!value.HasMember("scale") || value["scale"].IsNumber() && value["scale"].GetDouble() > 0.0, (prefix + "Value 'scale' must be a positive numeric value greater than 0\n").c_str())) return false;
	if (!READER_CHECK(!value.HasMember("name") || value["format"].IsString(), (prefix + "Value 'format' must be a string\n").c_str())) return false;
	return true;
}
