﻿#include "Saffron/SaffronPCH.h"
#include "Saffron/Graphics/Layer.h"

namespace Se
{

Layer::Layer(const Window::Ptr &pWindow, std::string name)
	:
	m_pWindow(pWindow),
	m_DebugName(std::move(name))
{
	m_pWindow->AddEventHandler(this);
}

}
