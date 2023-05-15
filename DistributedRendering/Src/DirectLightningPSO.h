#pragma once

#include <dx12Lib/Device.h>
#include <DirectXMath.h>
#include "LightTypes.h"
#include <dx12Lib/Material.h>
#include <dx12Lib/CommandList.h>

namespace DR {
	class DirectLightingPSO
	{
	public:
		DirectLightingPSO() {}
		DirectLightingPSO(std::shared_ptr<dx12lib::Device> device, std::wstring vsPath, std::wstring psPath);

		void SetDirectionalLight(std::vector<DirectionalLight> lights);
		
		DirectX::XMMATRIX GetModelMatrix() const { return m_AlignedMVP->Model; }
		void SetModelMatrix(DirectX::CXMMATRIX modelMatrix)
		{
			m_AlignedMVP->Model = modelMatrix;
			m_MatricesChanged = true;
		}

		DirectX::XMMATRIX GetViewMatrix() const { return m_AlignedMVP->View; }
		void SetViewMatrix(DirectX::CXMMATRIX viewMatrix)
		{
			m_AlignedMVP->View = viewMatrix;
			m_MatricesChanged = true;
		}

		DirectX::XMMATRIX GetProjectionMatrix() const { return m_AlignedMVP->Projection; }
		void SetProjectionMatrix(DirectX::CXMMATRIX projectionMatrix)
		{
			m_AlignedMVP->Projection = projectionMatrix;
			m_MatricesChanged = true;
		}

		void SetMaterial(std::shared_ptr<dx12lib::Material>& material)
		{
			m_Material = material;
			m_MaterialChanged = true;
		}

		void Apply(dx12lib::CommandList& commandList);

		inline void BindTexture(dx12lib::CommandList& commandList, uint32_t offset, const std::shared_ptr<dx12lib::Texture>& texture)
		{
			if (texture)
			{
				commandList.SetShaderResourceView(4, offset, texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			}
			else
			{
				commandList.SetShaderResourceView(4, offset, m_DefaultSRV, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			}
		}

		~DirectLightingPSO();

	private:
		struct alignas(16) MVP
		{
			DirectX::XMMATRIX Model;
			DirectX::XMMATRIX View;
			DirectX::XMMATRIX Projection;
		};

		struct alignas(16) Matrices
		{
			DirectX::XMMATRIX Model;
			DirectX::XMMATRIX ModelView;
			DirectX::XMMATRIX InverseTransposeModelView;
			DirectX::XMMATRIX ModelViewProjection;
		};

		MVP* m_AlignedMVP;

		std::shared_ptr<dx12lib::RootSignature> m_RootSignature;
		std::shared_ptr<dx12lib::PipelineStateObject> m_PSO;
		std::shared_ptr<dx12lib::ShaderResourceView> m_DefaultSRV;

		std::shared_ptr<dx12lib::Device> m_Device;

		std::vector<DirectionalLight> m_DirectLight;
		std::shared_ptr<dx12lib::Material> m_Material;

		bool m_LightChanged = true;
		bool m_MatricesChanged = true;
		bool m_MaterialChanged = true;

		enum RootParameters
		{
			// Vertex shader parameter
			MatricesCB,  // ConstantBuffer<Matrices> MatCB : register(b0);

			// Pixel shader parameters
			MaterialCB,         // ConstantBuffer<Material> MaterialCB : register( b0, space1 );
			LightPropertiesCB,  // ConstantBuffer<LightProperties> LightPropertiesCB : register( b1 );

			PointLights,        // StructuredBuffer<PointLight> PointLights : register( t0 );
			SpotLights,         // StructuredBuffer<SpotLight> SpotLights : register( t1 );
			DirectionalLights,  // StructuredBuffer<DirectionalLight> DirectionalLights : register( t2 )

			Textures,  // Texture2D AmbientTexture       : register( t3 );
			// Texture2D EmissiveTexture : register( t4 );
			// Texture2D DiffuseTexture : register( t5 );
			// Texture2D SpecularTexture : register( t6 );
			// Texture2D SpecularPowerTexture : register( t7 );
			// Texture2D NormalTexture : register( t8 );
			// Texture2D BumpTexture : register( t9 );
			// Texture2D OpacityTexture : register( t10 );
			NumRootParameters
		};

		struct LightProperties
		{
			uint32_t NumPointLights;
			uint32_t NumSpotLights;
			uint32_t NumDirectionalLights;
		};
	};
}

