﻿#include "pch.h"

#include "GraphicPipeline.h"

#include <d3dx12.h>


#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")



#include "Shader.h"


#define OUT_OF_RANGE(container,index) (index<0||container.size()<=index)



namespace og
{
	GraphicPipeline::GraphicPipeline(ComPtr<ID3D12Device>& device, const InnerGraphicPipelineDesc& desc)
	{
		if (CheckArgs(device, desc.vsInstance != nullptr, desc.psInstance != nullptr))return;

		// 頂点レイアウトを取得
		if (ReflectInputLayout(desc.vsInstance->GetShaderBolb()) == -1)return;

		for (U32 i = 0; i < MAX_CONSTANT_BUFFER; i++)
		{
			m_ConstantBufferSizes[i] = 0;
		}
		m_TextureNum = 0;


		// 定数バッファの取得
		if (desc.vsInstance != nullptr)ReflectShader(desc.vsInstance->GetShaderBolb()); else return;
		if (desc.psInstance != nullptr)ReflectShader(desc.psInstance->GetShaderBolb()); else return;
		if (desc.gsInstance != nullptr)ReflectShader(desc.gsInstance->GetShaderBolb());
		if (desc.dsInstance != nullptr)ReflectShader(desc.dsInstance->GetShaderBolb());
		if (desc.hsInstance != nullptr)ReflectShader(desc.hsInstance->GetShaderBolb());

		// データとレジスタの結び付け用
		// レジスタ毎の定数バッファ / テクスチャ
		ArrayList<CD3DX12_DESCRIPTOR_RANGE> descriptorRanges;
		// 定数バッファ
		for (S32 i = 0; i < MAX_CONSTANT_BUFFER; i++)
		{
			if (m_ConstantBufferSizes[i] == 0)continue;

			CD3DX12_DESCRIPTOR_RANGE dr;
			dr.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, i);
			descriptorRanges.push_back(dr);
		}
		// テクスチャ
		// レジスタ番号に空きがないこと前提
		if (0 < m_TextureNum)
		{
			CD3DX12_DESCRIPTOR_RANGE dr;
			dr.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, m_TextureNum, 0);
			descriptorRanges.push_back(dr);
		}


		// SetDescriptorHeapsで割り当てる塊
		CD3DX12_ROOT_PARAMETER rootParams[MAX_CONSTANT_BUFFER + 1] = {};
		for (S32 i = 0; i < descriptorRanges.size(); i++)
		{
			rootParams[i].InitAsDescriptorTable(1, &descriptorRanges[i]);
		}


		// サンプラー
		CD3DX12_STATIC_SAMPLER_DESC samplerDescs[1] = {};
		samplerDescs[0].Init(0);


		// ルートシグネチャディスクリプタ
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.Init((UINT)descriptorRanges.size(), rootParams, 1, samplerDescs, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);


		//D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		//rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		// バイナリデータの作成
		ComPtr<ID3DBlob> rootSigBlob = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;

		auto result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);
		if (FAILED(result))
		{
			return;
		}

		// ルートシグネチャの作成
		ComPtr<ID3D12RootSignature> rootSignature;
		result = device->CreateRootSignature(
			0,
			rootSigBlob->GetBufferPointer(),
			rootSigBlob->GetBufferSize(),
			IID_PPV_ARGS(rootSignature.ReleaseAndGetAddressOf()));
		if (FAILED(result))
		{
			return;
		}


		// グラフィックパイプラインの定義
		D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};

		if (desc.vsInstance)pipelineStateDesc.VS = CD3DX12_SHADER_BYTECODE(desc.vsInstance->GetShaderBolb().Get());
		if (desc.psInstance)pipelineStateDesc.PS = CD3DX12_SHADER_BYTECODE(desc.psInstance->GetShaderBolb().Get());
		if (desc.gsInstance)pipelineStateDesc.GS = CD3DX12_SHADER_BYTECODE(desc.gsInstance->GetShaderBolb().Get());
		if (desc.hsInstance)pipelineStateDesc.HS = CD3DX12_SHADER_BYTECODE(desc.hsInstance->GetShaderBolb().Get());
		if (desc.dsInstance)pipelineStateDesc.DS = CD3DX12_SHADER_BYTECODE(desc.dsInstance->GetShaderBolb().Get());

		pipelineStateDesc.pRootSignature = rootSignature.Get();

		pipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

		pipelineStateDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

		pipelineStateDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		pipelineStateDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;					//カリングしない
		// TODO 深度バッファ
		pipelineStateDesc.DepthStencilState.DepthEnable = false;								//深度バッファを使うぞ
		pipelineStateDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;	//全て書き込み
		pipelineStateDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;			//小さい方を採用
		pipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		pipelineStateDesc.DepthStencilState.StencilEnable = false;

		pipelineStateDesc.InputLayout.pInputElementDescs = m_InputLayout.data();			//頂点レイアウト先頭アドレス
		pipelineStateDesc.InputLayout.NumElements = (UINT)m_InputLayout.size();				//頂点レイアウトサイズ

		pipelineStateDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;	//ストリップ時のカットなし
		pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;	//三角形で構成

		pipelineStateDesc.SampleDesc.Count = 1;  //サンプリングは1ピクセルにつき１
		pipelineStateDesc.SampleDesc.Quality = 0;  //クオリティは最低

		pipelineStateDesc.NumRenderTargets = desc.numRenderTargets;							//レンダ―ターゲットの数
		for (U32 i = 0; i < pipelineStateDesc.NumRenderTargets; i++)
		{
			pipelineStateDesc.RTVFormats[i] = DXGI_FORMAT_R8G8B8A8_UNORM;					//0～1に正規化されたRGBA
		}


		pipelineStateDesc.RasterizerState.FrontCounterClockwise = false;
		pipelineStateDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		pipelineStateDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		pipelineStateDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		pipelineStateDesc.RasterizerState.AntialiasedLineEnable = false;
		pipelineStateDesc.RasterizerState.ForcedSampleCount = 0;
		pipelineStateDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;


		// グラフィックパイプラインの生成
		ComPtr<ID3D12PipelineState> pipelineState;
		result = device->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(pipelineState.ReleaseAndGetAddressOf()));
		if (FAILED(result))
		{
			return;
		}


		// リソースを参照に追加
		m_PipelineState = pipelineState;
		m_RootSignature = rootSignature;
		m_VS = desc.vsInstance;
		m_PS = desc.psInstance;
		m_GS = desc.gsInstance;
		m_HS = desc.hsInstance;
		m_DS = desc.dsInstance;
	}



	S32 GraphicPipeline::GetConstantBufferSize(const U32 index)const
	{
		if (index < 0 || MAX_CONSTANT_BUFFER <= index)return -1;
		return (m_ConstantBufferSizes[index] + 0xff) & ~0xff;
	}

	S32 GraphicPipeline::GetTextureNum()const
	{
		return m_TextureNum;
	}

	ShaderVariableDesc GraphicPipeline::GetVariableData(const String& name)const
	{
		if (m_Data.count(name) == 0)
		{
			ShaderVariableDesc desc = {};
			desc.registerNum = -1;
			desc.type = ShaderParamType::UNDEFINED;
			return desc;
		}
		return m_Data.at(name);
	}


	const HashMap<String, ShaderVariableDesc>& GraphicPipeline::GetShaderParamList()const
	{
		static const HashMap<String, ShaderVariableDesc> emptyMap;
		if (!IsValid())return emptyMap;
		return m_Data;
	}


	S32 GraphicPipeline::SetGraphicPipeline(ComPtr<ID3D12GraphicsCommandList>& commandList)const
	{
		if (!IsValid())return -1;
		if (!commandList)return -1;
		commandList->SetPipelineState(m_PipelineState.Get());
		commandList->SetGraphicsRootSignature(m_RootSignature.Get());
		return 0;
	}





	S32 GraphicPipeline::ReflectShader(const ComPtr<ID3DBlob>& shaderBolb)
	{
		if (CheckArgs(shaderBolb))return -1;

		ComPtr<ID3D12ShaderReflection> reflection;
		D3DReflect(shaderBolb->GetBufferPointer(), shaderBolb->GetBufferSize(), IID_ID3D12ShaderReflection, (void**)reflection.ReleaseAndGetAddressOf());

		ReflectConstantBuffer(reflection);
		ReflectBoundingResource(reflection);

		return 0;
	}

	S32 GraphicPipeline::ReflectInputLayout(const ComPtr<ID3DBlob>& vsInstance)
	{
		if (CheckArgs(vsInstance))return -1;

		ComPtr<ID3D12ShaderReflection> reflection;
		D3DReflect(vsInstance->GetBufferPointer(), vsInstance->GetBufferSize(), IID_ID3D12ShaderReflection, (void**)reflection.ReleaseAndGetAddressOf());

		if (!reflection)return -1;

		D3D12_SHADER_DESC shaderDesc;
		reflection->GetDesc(&shaderDesc);

		m_InputLayoutNames.resize(shaderDesc.InputParameters);
		for (U32 i = 0; i < shaderDesc.InputParameters; i++)
		{
			D3D12_SIGNATURE_PARAMETER_DESC paramDesc;
			reflection->GetInputParameterDesc(i, &paramDesc);

			m_InputLayoutNames[i] = paramDesc.SemanticName;

			D3D12_INPUT_ELEMENT_DESC elementDesc;
			elementDesc.SemanticName = m_InputLayoutNames[i].c_str();
			elementDesc.SemanticIndex = paramDesc.SemanticIndex;
			elementDesc.InputSlot = 0;
			elementDesc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
			elementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			elementDesc.InstanceDataStepRate = 0;

			if (paramDesc.Mask == 1)
			{
				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
			}
			else if (paramDesc.Mask <= 3)
			{
				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
			}
			else if (paramDesc.Mask <= 7)
			{
				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
			}
			else if (paramDesc.Mask <= 15)
			{
				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			}

			if (m_InputLayoutNames[i] == "POSITION")
			{
				elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
			}

			//save element desc
			m_InputLayout.push_back(elementDesc);
		}

		return 0;
	}

	S32 GraphicPipeline::ReflectConstantBuffer(const ComPtr<ID3D12ShaderReflection>& reflection)
	{
		if (CheckArgs(reflection))return -1;

		D3D12_SHADER_DESC	shaderDesc;
		reflection->GetDesc(&shaderDesc);

		for (U32 cBufIdx = 0; cBufIdx < shaderDesc.ConstantBuffers; cBufIdx++)
		{
			auto cbufferPtr = reflection->GetConstantBufferByIndex(cBufIdx);
			if (cbufferPtr == nullptr)return -1;

			D3D12_SHADER_BUFFER_DESC bufferDesc;
			cbufferPtr->GetDesc(&bufferDesc);


			D3D12_SHADER_INPUT_BIND_DESC inputDesc;
			reflection->GetResourceBindingDescByName(bufferDesc.Name, &inputDesc);



			U32 registerNum = inputDesc.BindPoint;
			if (m_ConstantBufferSizes[registerNum] < bufferDesc.Size)
			{
				m_ConstantBufferSizes[registerNum] = bufferDesc.Size;
			}


			for (UINT varIdx = 0; varIdx < bufferDesc.Variables; varIdx++)
			{
				auto variable = cbufferPtr->GetVariableByIndex(varIdx);
				if (!variable)continue;

				auto reflectionType = variable->GetType();

				D3D12_SHADER_TYPE_DESC typeDesc;
				reflectionType->GetDesc(&typeDesc);

				D3D12_SHADER_VARIABLE_DESC variableDesc;
				variable->GetDesc(&variableDesc);


				ShaderVariableDesc vdesc;
				vdesc.offset = variableDesc.StartOffset;
				vdesc.elementCount = typeDesc.Elements;
				vdesc.type = ShaderParamType::UNDEFINED;
				vdesc.registerNum = registerNum;


				switch (typeDesc.Class)
				{
				case D3D_SVC_SCALAR:break;
				case D3D_SVC_VECTOR:break;
				case D3D_SVC_MATRIX_COLUMNS:break;
					//case D3D_SVC_OBJECT:break;
					//case D3D_SVC_STRUCT:break;
					//case D3D_SVC_INTERFACE_CLASS:break;
				}


				switch (typeDesc.Type)
				{
				case D3D_SVT_BOOL:vdesc.type = ShaderParamType::BOOL; break;
				case D3D_SVT_INT:vdesc.type = ShaderParamType::INT; break;
				case D3D_SVT_FLOAT:
					switch (typeDesc.Class)
					{
					case D3D_SVC_SCALAR:vdesc.type = ShaderParamType::FLOAT; break;
					case D3D_SVC_VECTOR:vdesc.type = ShaderParamType::FLOAT4; break;//TODO 確認が終わったらFLOAT1～FLOAT4の対応
					case D3D_SVC_MATRIX_COLUMNS: vdesc.type = ShaderParamType::MATRIX; break;
					}

					break;
				case D3D_SVT_UINT:break;
				case D3D_SVT_UINT8:break;
				case D3D_SVT_MIN8FLOAT:break;// 4
				case D3D_SVT_MIN10FLOAT:break;// 3
				case D3D_SVT_MIN16FLOAT:break;// 2
				default:
					break;
				}

				m_Data[variableDesc.Name] = vdesc;
			}
		}
		return 0;
	}


	S32 GraphicPipeline::ReflectBoundingResource(const ComPtr<ID3D12ShaderReflection>& reflection)
	{
		if (CheckArgs(reflection))return -1;

		D3D12_SHADER_DESC	shaderDesc;
		reflection->GetDesc(&shaderDesc);

		for (U32 i = 0; i < shaderDesc.BoundResources; i++)
		{
			D3D12_SHADER_INPUT_BIND_DESC desc;
			reflection->GetResourceBindingDesc(i, &desc);
			if (desc.Type == D3D_SHADER_INPUT_TYPE::D3D_SIT_TEXTURE)
			{
				ShaderVariableDesc vdesc;
				vdesc.offset = 0;
				vdesc.elementCount = 1;
				vdesc.type = ShaderParamType::TEXTURE2D;
				vdesc.registerNum = desc.BindPoint;

				m_Data[desc.Name] = vdesc;

				if (m_TextureNum < vdesc.registerNum + 1)
				{
					m_TextureNum = vdesc.registerNum + 1;
				}
			}
		}
		return 0;
	}

	S32 GraphicPipeline::ReflectOutputLayout(const ComPtr<ID3DBlob>& vsInstance)
	{
		if (CheckArgs(vsInstance))return -1;

		ComPtr<ID3D12ShaderReflection> reflection;
		D3DReflect(vsInstance->GetBufferPointer(), vsInstance->GetBufferSize(), IID_ID3D12ShaderReflection, (void**)reflection.ReleaseAndGetAddressOf());

		if (!reflection)return -1;

		D3D12_SHADER_DESC shaderDesc;
		reflection->GetDesc(&shaderDesc);

		m_InputLayoutNames.resize(shaderDesc.InputParameters);
		for (U32 i = 0; i < shaderDesc.OutputParameters; i++)
		{
			D3D12_SIGNATURE_PARAMETER_DESC paramDesc;
			reflection->GetOutputParameterDesc(i, &paramDesc);

			m_InputLayoutNames[i] = paramDesc.SemanticName;

			D3D12_INPUT_ELEMENT_DESC elementDesc;
			elementDesc.SemanticName = m_InputLayoutNames[i].c_str();
			elementDesc.SemanticIndex = paramDesc.SemanticIndex;
			elementDesc.InputSlot = 0;
			elementDesc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
			elementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			elementDesc.InstanceDataStepRate = 0;

			if (paramDesc.Mask == 1)
			{
				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
			}
			else if (paramDesc.Mask <= 3)
			{
				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
			}
			else if (paramDesc.Mask <= 7)
			{
				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
			}
			else if (paramDesc.Mask <= 15)
			{
				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			}

			//save element desc
		}

		return 0;
	}
}