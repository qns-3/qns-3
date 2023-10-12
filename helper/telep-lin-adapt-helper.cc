#include "ns3/telep-lin-adapt-helper.h"
#include "ns3/telep-lin-adapt-app.h"

#include "ns3/names.h"

#include "ns3/quantum-basis.h"
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-channel.h" // class QuantumChannel

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TelepLinAdaptHelper");


TelepLinAdaptHelper::TelepLinAdaptHelper (Ptr<QuantumPhyEntity> qphyent, Ptr<QuantumChannel> conn)
{
  m_factory.SetTypeId (TelepLinAdaptApp::GetTypeId ());

  if (conn)
    {
      SetAttribute ("PeerAddress", AddressValue (conn->GetDst (qphyent)->GetAddress ()));
      SetAttribute ("PeerPort", UintegerValue (conn->GetDst (qphyent)->GetNextPort ()));
      SetAttribute ("DataSize", UintegerValue (0));
      SetAttribute ("Port", UintegerValue (conn->GetSrc (qphyent)->AllocPort ()));
    }

  SetAttribute ("QPhyEntity", PointerValue (qphyent));
  SetAttribute ("QChannel", PointerValue (conn));
}

void
TelepLinAdaptHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
TelepLinAdaptHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
TelepLinAdaptHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
TelepLinAdaptHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
TelepLinAdaptHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<TelepLinAdaptApp> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
