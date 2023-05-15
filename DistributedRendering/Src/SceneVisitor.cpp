#include "drpch.h"

#include <SceneVisitor.h>

#include "DirectLightningPSO.h"
#include <Camera.h>

#include <dx12lib/CommandList.h>
#include <dx12lib/IndexBuffer.h>
#include <dx12lib/Material.h>
#include <dx12lib/Mesh.h>
#include <dx12lib/SceneNode.h>

#include <DirectXMath.h>
#include "DirectLightningPSO.h"


using namespace DR;
using namespace dx12lib;
using namespace DirectX;

SceneVisitor::SceneVisitor( CommandList& commandList, const Camera& camera, DirectLightingPSO& pso, bool transparent )
: m_CommandList( commandList )
, m_Camera( camera )
, m_LightingPSO( pso )
, m_TransparentPass(transparent)
{}

//SceneVisitor::SceneVisitor(CommandList& commandList, const LCamera& camera, EffectPSO& pso, bool transparent)
//    : m_CommandList(commandList)
//    , m_Camera(camera)
//    , m_EffectPSO(pso)
//    , m_TransparentPass(transparent)
//{}

void SceneVisitor::Visit( dx12lib::Scene& scene )
{
    //m_EffectPSO.SetViewMatrix( m_Camera.get_ViewMatrix() );
    //m_EffectPSO.SetProjectionMatrix( m_Camera.get_ProjectionMatrix());
    
    m_LightingPSO.SetViewMatrix(m_Camera.GetView());
    m_LightingPSO.SetProjectionMatrix(m_Camera.GetProjection());
}

void SceneVisitor::Visit( dx12lib::SceneNode& sceneNode )
{
    auto model = sceneNode.GetWorldTransform();
    //m_EffectPSO.SetWorldMatrix( model);
    m_LightingPSO.SetModelMatrix( model);
}

void SceneVisitor::Visit( Mesh& mesh )
{
    auto material = mesh.GetMaterial();
    if ( material->IsTransparent() == m_TransparentPass )
    {
        //m_EffectPSO.SetMaterial( material );
        m_LightingPSO.SetMaterial( material );

        //m_EffectPSO.Apply( m_CommandList );
        m_LightingPSO.Apply( m_CommandList );
        mesh.Draw( m_CommandList );
    }
}