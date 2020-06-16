#pragma once

#include "Bindable.h"

namespace ve
{
class InputLayout : public Bindable
{
public:
	InputLayout( Graphics &gfx,
				 const std::vector<D3D11_INPUT_ELEMENT_DESC> &layout,
				 ID3DBlob *pVertexShaderByteCode );
	void Bind( Graphics &gfx ) noexcept override;
protected:
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_pInputLayout;
};
}
