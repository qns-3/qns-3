#include "ns3/ent-swap-adapt-helper.h"
#include "ns3/ent-swap-adapt-app.h"

#include "ns3/names.h"

#include "ns3/quantum-basis.h"
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-channel.h" // class QuantumChannel

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EntSwapAdaptHelper");

EntSwapAdaptHelper::EntSwapAdaptHelper (Ptr<QuantumPhyEntity> qphyent)
{
  m_factory.SetTypeId (EntSwapAdaptApp::GetTypeId ());

  SetAttribute ("QPhyEntity", PointerValue (qphyent));
}

void
EntSwapAdaptHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
EntSwapAdaptHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
EntSwapAdaptHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
EntSwapAdaptHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
EntSwapAdaptHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<EntSwapAdaptApp> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
