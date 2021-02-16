$input v_texcoord0

/*
 * Copyright 2011-2021 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx#license-bsd-2-clause
 */

#include "shaderBase.sc"
#include "shaderLib.sc"

SAMPLERCUBE(s_texCube, 0);
uniform mat4 u_transform;

void main()
{
	vec3 dir = vec3(v_texcoord0*2.0 - 1.0, 1.0);
	dir = normalize(mul(u_transform, vec4(dir, 0.0) ).xyz);
	gl_FragColor = encodeRGBE8(textureCube(s_texCube, dir).xyz);
}