#include "drpch.h"
#include "DirectLightningPSO.h"
#include <dx12Lib/VertexTypes.h>
#include <dx12Lib/RootSignature.h>

using namespace DR;
using namespace dx12lib;
using namespace Microsoft::WRL;

DirectLightingPSO::DirectLightingPSO(std::shared_ptr<Device> device, std::wstring vsPath, std::wstring psPath) : m_Device(device)
{

	m_AlignedMVP = (MVP*)_aligned_malloc(sizeof(MVP), 16);

	ComPtr<ID3DBlob> vsBlob;
	ThrowIfFailed(D3DReadFileToBlob(vsPath.c_str(), &vsBlob));

	ComPtr<ID3DBlob> psBlob;
	ThrowIfFailed(D3DReadFileToBlob(psPath.c_str(), &psBlob));

	D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 8, 3);
	
	CD3DX12_ROOT_PARAMETER1 rootParameters[5];
	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameters[1].InitAsConstantBufferView(0, 1, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[2].InitAsConstants(sizeof(LightProperties) / 4, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[3].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[4].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

	//CD3DX12_ROOT_PARAMETER1 rootParameters[RootParameters::NumRootParameters];
	//rootParameters[RootParameters::MatricesCB].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
	//rootParameters[RootParameters::MaterialCB].InitAsConstantBufferView(0, 1, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
	//rootParameters[RootParameters::LightPropertiesCB].InitAsConstants(sizeof(LightProperties) / 4, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	//rootParameters[RootParameters::PointLights].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
	//rootParameters[RootParameters::SpotLights].InitAsShaderResourceView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
	//rootParameters[RootParameters::DirectionalLights].InitAsShaderResourceView(2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
	//rootParameters[RootParameters::Textures].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_STATIC_SAMPLER_DESC aniSampler(0, D3D12_FILTER_ANISOTROPIC);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc;
	rootSigDesc.Init_1_1(5, rootParameters, 1, &aniSampler, rootSigFlags);
	
	m_RootSignature = m_Device->CreateRootSignature(rootSigDesc.Desc_1_1);

	DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;
	
	DXGI_SAMPLE_DESC sampleDesc = m_Device->GetMultisampleQualityLevels(backBufferFormat);

	D3D12_RT_FORMAT_ARRAY rtvFormat = {};
	rtvFormat.NumRenderTargets = 1;
	rtvFormat.RTFormats[0] = backBufferFormat;
	
	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE        pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_VS                    VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS                    PS;
		CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER            RasterizerState;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT          InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY    PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT  DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
		CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC           SampleDesc;
	} pipelineStateStream;

	CD3DX12_RASTERIZER_DESC rasterizerState(D3D12_DEFAULT);

	pipelineStateStream.pRootSignature = m_RootSignature->GetD3D12RootSignature().Get();
	pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
	pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());
	pipelineStateStream.RasterizerState = rasterizerState;
	pipelineStateStream.InputLayout = VertexPositionNormalTangentBitangentTexture::InputLayout;
	pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.DSVFormat = depthBufferFormat;
	pipelineStateStream.RTVFormats = rtvFormat;
	pipelineStateStream.SampleDesc = sampleDesc;

	m_PSO = m_Device->CreatePipelineStateObject(pipelineStateStream);

	D3D12_SHADER_RESOURCE_VIEW_DESC defaultSRV;
	defaultSRV.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	defaultSRV.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	defaultSRV.Texture2D.MostDetailedMip = 0;
	defaultSRV.Texture2D.MipLevels = 1;
	defaultSRV.Texture2D.PlaneSlice = 0;
	defaultSRV.Texture2D.ResourceMinLODClamp = 0;
	defaultSRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	m_DefaultSRV = device->CreateShaderResourceView(nullptr, &defaultSRV);
}

void DR::DirectLightingPSO::SetDirectionalLight(std::vector<DirectionalLight> lights)
{
	m_DirectLight = lights;
	m_LightChanged = true;
}

void DR::DirectLightingPSO::Apply(dx12lib::CommandList& commandList)
{
	commandList.SetPipelineState(m_PSO);
	commandList.SetGraphicsRootSignature(m_RootSignature);

	if (m_MatricesChanged)
	{
		Matrices matrices;
		matrices.Model = m_AlignedMVP->Model;
		matrices.ModelView = m_AlignedMVP->Model * m_AlignedMVP->View;
		matrices.ModelViewProjection = matrices.ModelView * m_AlignedMVP->Projection;
		matrices.InverseTransposeModelView = DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(nullptr, matrices.ModelView));

		commandList.SetGraphicsDynamicConstantBuffer(0, matrices);

		m_MatricesChanged = false;
	}

	if (m_MaterialChanged)
	{
		if (m_Material)
		{
			const auto& materialProps = m_Material->GetMaterialProperties();

			commandList.SetGraphicsDynamicConstantBuffer(1, materialProps);
			
			BindTexture(commandList, 0, m_Material->GetTexture(Material::TextureType::Ambient));
			BindTexture(commandList, 1, m_Material->GetTexture(Material::TextureType::Emissive));
			BindTexture(commandList, 2, m_Material->GetTexture(Material::TextureType::Diffuse));
			BindTexture(commandList, 3, m_Material->GetTexture(Material::TextureType::Specular));
			BindTexture(commandList, 4, m_Material->GetTexture(Material::TextureType::SpecularPower));
			BindTexture(commandList, 5, m_Material->GetTexture(Material::TextureType::Normal));
			BindTexture(commandList, 6, m_Material->GetTexture(Material::TextureType::Bump));
			BindTexture(commandList, 7, m_Material->GetTexture(Material::TextureType::Opacity));
		}
		m_MaterialChanged = false;
	}

	if (m_LightChanged)
	{
		commandList.SetGraphicsDynamicStructuredBuffer(3, m_DirectLight);
		
		LightProperties lightProps;
		lightProps.NumDirectionalLights = 1;
		lightProps.NumPointLights = 0;
		lightProps.NumSpotLights = 0;
		

		commandList.SetGraphics32BitConstants(2, lightProps);
		m_LightChanged = false;
	}
}

DirectLightingPSO::~DirectLightingPSO()
{
	_aligned_free(m_AlignedMVP);
}