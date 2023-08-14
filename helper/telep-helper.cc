#include "ns3/telep-helper.h"
#include "ns3/telep-app.h"

#include "ns3/names.h"

#include "ns3/quantum-basis.h"
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-channel.h" // class QuantumChannel

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TelepHelper");

TelepSrcHelper::TelepSrcHelper (Ptr<QuantumPhyEntity> qphyent, Ptr<QuantumChannel> conn)
{
  m_factory.SetTypeId (TelepSrcApp::GetTypeId ());

  SetAttribute ("PeerAddress", AddressValue (conn->GetDst (qphyent)->GetAddress ()));
  SetAttribute ("PeerPort", UintegerValue (conn->GetDst (qphyent)->GetNextPort ()));
  SetAttribute ("DataSize", UintegerValue (0));

  SetAttribute ("QPhyEntity", PointerValue (qphyent));
  SetAttribute ("QChannel", PointerValue (conn));
}

void
TelepSrcHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
TelepSrcHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
TelepSrcHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
TelepSrcHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
TelepSrcHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<TelepSrcApp> ();
  node->AddApplication (app);

  return app;
}

TelepDstHelper::TelepDstHelper (Ptr<QuantumPhyEntity> qphyent, Ptr<QuantumChannel> conn)
{
  m_factory.SetTypeId (TelepDstApp::GetTypeId ());

  SetAttribute ("Port", UintegerValue (conn->GetDst (qphyent)->AllocPort ()));

  SetAttribute ("QPhyEntity", PointerValue (qphyent));
  SetAttribute ("Pnode", PointerValue (conn->GetDst (qphyent)));
}

void
TelepDstHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
TelepDstHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
TelepDstHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
TelepDstHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
TelepDstHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<TelepDstApp> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
