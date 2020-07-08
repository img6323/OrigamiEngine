﻿#include "pch.h"

#include "DX12Wrapper.h"

#include "Texture.h"
#include "RenderTexture.h"
#include "Shader.h"
#include "GraphicPipeline.h"
#include "Material.h"
#include "Shape.h"



#define OUT_OF_RANGE(container,index) (index<0||container.size()<=index)

namespace og
{
	SPtr<ITexture> DX12Wrapper::LoadTexture(const Path& path, const bool async)
	{
		auto texture = MSPtr<Texture>(m_Dev, path);

		if (!texture->IsValid())return nullptr;

		return texture;
	}

	SPtr<IRenderTexture> DX12Wrapper::CreateRenderTexture(const S32 width, const S32 height, const TextureFormat format)
	{
		auto texture = MSPtr<RenderTexture>(m_Dev, ConvertTextureFormat(format), width, height);
		if (texture->IsValid() == false)return nullptr;
		return texture;
	}



	S32 DX12Wrapper::LoadShader(const String& path, ShaderType type, String& errorDest)
	{
		auto shader = Shader::LoadFromFile(path, type, errorDest);

		if (shader->IsValid() == false)return -1;

		m_ShaderList.push_back(std::move(shader));
		return (S32)m_ShaderList.size() - 1;
	}

	S32 DX12Wrapper::CreateShader(const String& src, ShaderType type, String& errorDest)
	{
		auto shader = MSPtr<Shader>(src, type, errorDest);
		if (shader->IsValid() == false)return -1;
		m_ShaderList.push_back(shader);
		return (S32)m_ShaderList.size() - 1;
	}

	S32 DX12Wrapper::DeleteShader(const S32 id)
	{
		if (OUT_OF_RANGE(m_ShaderList, id))return -1;
		if (m_ShaderList.at(id) == nullptr)return -1;
		auto& shader = m_ShaderList.at(id);
		if (0 < shader.use_count())return -1;
		shader.reset();
		return 0;
	}



	SPtr<IGraphicPipeline> DX12Wrapper::CreateGraphicPipeline(const GraphicPipelineDesc& desc)
	{
		InnerGraphicPipelineDesc iDesc(desc);

#define CHECK_SHADER(stage) (!OUT_OF_RANGE(m_ShaderList, desc.##stage)&&m_ShaderList.at(desc.##stage)->IsValid())

		if (CHECK_SHADER(vs))iDesc.vsInstance = m_ShaderList.at(desc.vs);
		if (CHECK_SHADER(ps))iDesc.psInstance = m_ShaderList.at(desc.ps);
		if (CHECK_SHADER(gs))iDesc.gsInstance = m_ShaderList.at(desc.gs);
		if (CHECK_SHADER(hs))iDesc.hsInstance = m_ShaderList.at(desc.hs);
		if (CHECK_SHADER(ds))iDesc.dsInstance = m_ShaderList.at(desc.ds);

		auto gpipeline = MSPtr<GraphicPipeline>(m_Dev, iDesc);
		if (gpipeline->IsValid() == false)return nullptr;

		return gpipeline;
	}


	const HashMap<String, ShaderVariableDesc>& DX12Wrapper::GetShaderParamList(const S32 graphicPipelineID)
	{
		static const HashMap<String, ShaderVariableDesc> emptyMap;
		if (OUT_OF_RANGE(m_PipelineList, graphicPipelineID))return emptyMap;

		auto& pipeline = m_PipelineList[graphicPipelineID];
		if (!pipeline)return emptyMap;

		return pipeline->GetShaderParamList();
	}


	SPtr<IMaterial> DX12Wrapper::CreateMaterial(const S32 id, const S32 mask)
	{
		if (OUT_OF_RANGE(m_PipelineList, id))return nullptr;

		// パイプラインが削除されていたら終了
		if (!m_PipelineList.at(id))return nullptr;

		// シェーダーパラメータの作成
		auto material = MUPtr<Material>(m_Dev, m_PipelineList.at(id), mask);
		if (material->IsValid() == false)return nullptr;

		return material;
	}

	S32 DX12Wrapper::SetMaterial(SPtr<IMaterial> material)
	{
		if (CheckArgs(!!material))return -1;

		auto ptr = dynamic_cast<Material*>(material.get());
		ptr->SetMaterial(m_Dev, m_CmdList);
		return 0;
	}



	SPtr<IShape> DX12Wrapper::CreateShape(const U32 stribeSize)
	{
		if (stribeSize <= 0)return nullptr;
		auto shape = MSPtr<Shape>(m_Dev, stribeSize);
		return shape;
	}


	//S32 DX12Wrapper::DrawShape(SPtr<IShape> shape)
	//{
	//	if (CheckArgs(!!shape))return -1;
	//	auto ptr = dynamic_cast<Shape*>(shape.get());
	//	return ptr->Draw(m_Dev, m_CmdList);
	//}

	// ここからプライベート関数
	DXGI_FORMAT DX12Wrapper::ConvertTextureFormat(const TextureFormat format)const
	{
		switch (format)
		{
		case TextureFormat::RGBA32:		return DXGI_FORMAT_R32G32B32A32_FLOAT;//
		case TextureFormat::RGBA16:		return DXGI_FORMAT_R16G16B16A16_UNORM;
		case TextureFormat::RGBA8:		return DXGI_FORMAT_R8G8B8A8_UNORM;
		case TextureFormat::RG8:		return DXGI_FORMAT_R8G8_SNORM;
		case TextureFormat::R32:		return DXGI_FORMAT_R32_FLOAT;//
		case TextureFormat::R16:		return DXGI_FORMAT_R16_UNORM;
		case TextureFormat::R8:			return DXGI_FORMAT_R8_UNORM;
		case TextureFormat::Depth:		return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case TextureFormat::RGB565:		return DXGI_FORMAT_B5G6R5_UNORM;
		case TextureFormat::RGBA5551:	return DXGI_FORMAT_B5G5R5A1_UNORM;
		}
		return DXGI_FORMAT_UNKNOWN;
	}
}