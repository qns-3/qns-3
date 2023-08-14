#include "ns3/distill-nested-helper.h"
#include "ns3/distill-nested-app.h"

#include "ns3/names.h"

#include "ns3/quantum-basis.h"
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-channel.h" // class QuantumChannel

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DistillNestedHelper");

DistillNestedHelper::DistillNestedHelper (Ptr<QuantumPhyEntity> qphyent, bool checker,
                                          Ptr<QuantumChannel> conn)
{
  NS_LOG_LOGIC ("Creating DistillNestedHelper for ");

  m_factory.SetTypeId (DistillNestedApp::GetTypeId ());

  Ptr<QuantumNode> pnode = nullptr;
  if (!checker)
    { // Alice
      pnode = conn->GetSrc (qphyent);
      SetAttribute ("PNode", PointerValue (pnode));
      SetAttribute ("PeerAddress", AddressValue (conn->GetDst (qphyent)->GetAddress ()));
      SetAttribute ("PeerPort", UintegerValue (conn->GetDst (qphyent)->GetNextPort ()));
    }
  else
    { // Bob
      pnode = conn->GetDst (qphyent);
      SetAttribute ("PNode", PointerValue (pnode));
      SetAttribute ("PeerAddress", AddressValue (conn->GetSrc (qphyent)->GetAddress ()));
      SetAttribute ("PeerPort", UintegerValue (conn->GetSrc (qphyent)->GetNextPort () - 1));
    }

  NS_LOG_LOGIC ("PNode: " << pnode);

  SetAttribute ("Port", UintegerValue (pnode->AllocPort ()));
  SetAttribute ("DataSize", UintegerValue (0));

  SetAttribute ("QPhyEntity", PointerValue (qphyent));
  SetAttribute ("Checker", BooleanValue (checker));
  SetAttribute ("QChannel", PointerValue (conn));
}

void
DistillNestedHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
DistillNestedHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
DistillNestedHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
DistillNestedHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
DistillNestedHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<DistillNestedApp> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
