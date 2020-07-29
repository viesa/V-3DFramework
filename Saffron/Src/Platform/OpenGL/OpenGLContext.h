﻿#pragma once

#include "Saffron/Renderer/GraphicsContext.h"

struct GLFWwindow;

namespace Se
{
class OpenGLContext : public GraphicsContext
{
public:
	explicit OpenGLContext(GLFWwindow *windowHandle);

	void Init() override;
	void SwapBuffers() override;
private:
	GLFWwindow *m_WindowHandle;
};
}
