#include "ns3/telep-adapt-helper.h"
#include "ns3/telep-adapt-app.h"

#include "ns3/names.h"

#include "ns3/quantum-basis.h"
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-channel.h" // class QuantumChannel

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TelepAdaptHelper");


TelepAdaptHelper::TelepAdaptHelper (Ptr<QuantumPhyEntity> qphyent, Ptr<QuantumChannel> conn)
{
  m_factory.SetTypeId (TelepAdaptApp::GetTypeId ());

  SetAttribute ("QPhyEntity", PointerValue (qphyent));
  SetAttribute ("QChannel", PointerValue (conn));
}

void
TelepAdaptHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
TelepAdaptHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
TelepAdaptHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
TelepAdaptHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
TelepAdaptHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<TelepAdaptApp> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
