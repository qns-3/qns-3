#include "ns3/quantum-net-stack-helper.h"

#include "ns3/quantum-basis.h"
#include "ns3/quantum-network-simulator.h" // class QuantumNetworkSimulator
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/distribute-epr-helper.h" // class DistributeEPRSrcHelper, DistributeEPRDstHelper


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("QuantumNetStackHelper");

QuantumNetStackHelper::QuantumNetStackHelper ()
{
}
QuantumNetStackHelper::~QuantumNetStackHelper ()
{
}

void
QuantumNetStackHelper::Install (NodeContainer c) const
{
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      for (NodeContainer::Iterator j = c.Begin (); j != c.End (); ++j)
        {
          if (i != j)
            {
              Install (DynamicCast<QuantumNode> (*i), DynamicCast<QuantumNode> (*j));
            }
        }
    }
}

void
QuantumNetStackHelper::Install (Ptr<QuantumNode> alice, Ptr<QuantumNode> bob) const
{
  Ptr<QuantumChannel> qconn = CreateObject<QuantumChannel> (alice, bob);

  NS_LOG_LOGIC ("Installing quantum network stack for " << alice->GetOwner () << " and "
                                                        << bob->GetOwner ());
  Ptr<QuantumPhyEntity> qphyent = alice->GetQuantumPhyEntity ();

  DistributeEPRSrcHelper srcHelper (qphyent, qconn);

  ApplicationContainer srcApps = srcHelper.Install (alice);
  srcApps.Start (Seconds (0.));
  srcApps.Stop (Seconds (ETERNITY));

  DistributeEPRDstHelper dstHelper (qphyent, qconn);

  ApplicationContainer dstApps = dstHelper.Install (bob);
  dstApps.Start (Seconds (0.));
  dstApps.Stop (Seconds (ETERNITY));

  qphyent->AddConn2Apps (qconn, APP_DIST_EPR, {srcApps.Get (0), dstApps.Get (0)});
}

} // namespace ns3
