$input v_color0, v_texcoord0, v_texcoord1

// license:BSD-3-Clause
// copyright-holders:Ryan Holtz,ImJezze
//-----------------------------------------------------------------------------
// Color Convolution Effect
//-----------------------------------------------------------------------------

#include "common.sh"

// Autos
uniform vec4 u_swap_xy;
uniform vec4 u_screen_dims;
uniform vec4 u_screen_count;
uniform vec4 u_target_dims;
uniform vec4 u_target_scale;
uniform vec4 u_quad_dims;

// User-supplied
uniform vec4 u_time_ratio; // Frame time of the vector (not set)
uniform vec4 u_time_scale; // How much frame time affects the vector's fade (not set)
uniform vec4 u_length_ratio; // Size at which fade is maximum
uniform vec4 u_length_scale; // How much length affects the vector's fade
uniform vec4 u_beam_smooth;

// www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
float roundBox(vec2 p, vec2 b, float r)
{
	return length(max(abs(p) - b + r, 0.0f)) - r;
}

float GetBoundsFactor(vec2 coord, vec2 bounds, float radiusAmount, float smoothAmount)
{
	// reduce smooth amount down to radius amount
	smoothAmount = min(smoothAmount, radiusAmount);

	float range = min(bounds.x, bounds.y);
	float amountMinimum = 1.0f / range;
	float radius = range * max(radiusAmount, amountMinimum);
	float smoothness = 1.0f / (range * max(smoothAmount, amountMinimum * 2.0f));

	// compute box
	float box = roundBox(bounds * (coord * 2.0f), bounds, radius);

	// apply smooth
	box *= smoothness;
	box += 1.0f - pow(smoothness * 0.5f, 0.5f);

	float border = smoothstep(1.0f, 0.0f, box);

	return saturate(border);
}

void main()
{
	vec2 lineSize = v_texcoord1.xy / max(u_target_dims.x, u_target_dims.y); // normalize

	float lineLength = lineSize.x;
	float lineLengthRatio = u_length_ratio.x;
	float lineLengthScale = u_length_scale.x;

	float timeModulate = mix(1.0f, u_time_ratio.x, u_time_scale.x);
	float lengthModulate = 1.0f - clamp(lineLength / lineLengthRatio, 0.0f, 1.0f);
	float timeLengthModulate = mix(1.0f, timeModulate * lengthModulate, u_length_scale.x);

	vec4 outColor = vec4(timeLengthModulate, timeLengthModulate, timeLengthModulate, 1.0f);
	outColor *= v_color0;

	float RoundCornerFactor = GetBoundsFactor(v_texcoord0.xy - 0.5f, v_texcoord1.xy, 1.0f, u_beam_smooth.x);
	outColor.rgb *= RoundCornerFactor;

	gl_FragColor = outColor;
}
