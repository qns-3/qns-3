#include "ns3/ent-swap-helper.h"
#include "ns3/ent-swap-app.h"

#include "ns3/names.h"

#include "ns3/quantum-basis.h"
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-channel.h" // class QuantumChannel

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EntSwapHelper");

EntSwapSrcHelper::EntSwapSrcHelper (Ptr<QuantumPhyEntity> qphyent, Ptr<QuantumChannel> conn)
{
  m_factory.SetTypeId (EntSwapSrcApp::GetTypeId ());

  SetAttribute ("PeerAddress", AddressValue (conn->GetDst (qphyent)->GetAddress ()));
  SetAttribute ("PeerPort", UintegerValue (conn->GetDst (qphyent)->GetNextPort ()));
  SetAttribute ("DataSize", UintegerValue (0));

  SetAttribute ("QPhyEntity", PointerValue (qphyent));
  SetAttribute ("QChannel", PointerValue (conn));
}

void
EntSwapSrcHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
EntSwapSrcHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
EntSwapSrcHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
EntSwapSrcHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
EntSwapSrcHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<EntSwapSrcApp> ();
  node->AddApplication (app);

  return app;
}

EntSwapDstHelper::EntSwapDstHelper (Ptr<QuantumPhyEntity> qphyent, Ptr<QuantumNode> pnode)
{
  m_factory.SetTypeId (EntSwapDstApp::GetTypeId ());

  SetAttribute ("Port", UintegerValue (pnode->AllocPort ()));

  SetAttribute ("QPhyEntity", PointerValue (qphyent));
  SetAttribute ("Pnode", PointerValue (pnode));
}

void
EntSwapDstHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
EntSwapDstHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
EntSwapDstHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
EntSwapDstHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
EntSwapDstHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<EntSwapDstApp> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
