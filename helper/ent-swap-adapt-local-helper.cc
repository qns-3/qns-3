#include "ns3/ent-swap-adapt-local-helper.h"
#include "ns3/ent-swap-adapt-local-app.h"

#include "ns3/names.h"

#include "ns3/quantum-basis.h"
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-channel.h" // class QuantumChannel

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EntSwapAdaptLocalLocalHelper");

EntSwapAdaptLocalHelper::EntSwapAdaptLocalHelper (Ptr<QuantumPhyEntity> qphyent)
{
  m_factory.SetTypeId (EntSwapAdaptLocalApp::GetTypeId ());

  SetAttribute ("QPhyEntity", PointerValue (qphyent));
}

void
EntSwapAdaptLocalHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
EntSwapAdaptLocalHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
EntSwapAdaptLocalHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
EntSwapAdaptLocalHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
EntSwapAdaptLocalHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<EntSwapAdaptLocalApp> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
