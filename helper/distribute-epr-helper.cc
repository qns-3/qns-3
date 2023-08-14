#include "ns3/distribute-epr-helper.h"
#include "ns3/distribute-epr-protocol.h"

#include "ns3/names.h"

#include "ns3/quantum-basis.h"
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-channel.h" // class QuantumChannel

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DistributeEPRHelper");

DistributeEPRSrcHelper::DistributeEPRSrcHelper (Ptr<QuantumPhyEntity> qphyent,
                                                Ptr<QuantumChannel> conn)
{
  m_factory.SetTypeId (DistributeEPRSrcProtocol::GetTypeId ());

  SetAttribute ("PeerAddress", AddressValue (conn->GetDst (qphyent)->GetAddress ()));
  SetAttribute ("PeerPort", UintegerValue (conn->GetDst (qphyent)->GetNextPort ()));
  SetAttribute ("DataSize", UintegerValue (0));
  SetAttribute ("QPhyEntity", PointerValue (qphyent));
  SetAttribute ("QChannel", PointerValue (conn));
}

DistributeEPRSrcHelper::~DistributeEPRSrcHelper ()
{
}

void
DistributeEPRSrcHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
DistributeEPRSrcHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
DistributeEPRSrcHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
DistributeEPRSrcHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
DistributeEPRSrcHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<DistributeEPRSrcProtocol> ();
  node->AddApplication (app);

  return app;
}

DistributeEPRDstHelper::DistributeEPRDstHelper (Ptr<QuantumPhyEntity> qphyent,
                                                Ptr<QuantumChannel> conn)
{
  m_factory.SetTypeId (DistributeEPRDstProtocol::GetTypeId ());

  SetAttribute ("Port", UintegerValue (conn->GetDst (qphyent)->AllocPort ()));
  SetAttribute ("QPhyEntity", PointerValue (qphyent));
  SetAttribute ("QChannel", PointerValue (conn));
}

DistributeEPRDstHelper::~DistributeEPRDstHelper ()
{
}

void
DistributeEPRDstHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
DistributeEPRDstHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
DistributeEPRDstHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
DistributeEPRDstHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
DistributeEPRDstHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<DistributeEPRDstProtocol> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
