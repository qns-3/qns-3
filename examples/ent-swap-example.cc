#include "ns3/csma-module.h" // class CsmaHelper, NetDeviceContainer
#include "ns3/internet-module.h" // class InternetStackHelper, Ipv6AddressHelper, Ipv6InterfaceContainer

#include "ns3/quantum-basis.h"
#include "ns3/quantum-network-simulator.h" // class QuantumNetworkSimulator
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-channel.h" // class QuantumChannel
#include "ns3/distribute-epr-helper.h" // class DistributeEPRSrcHelper, DistributeEPRDstHelper
#include "ns3/distribute-epr-protocol.h" // class DistributeEPRSrcProtocol
#include "ns3/quantum-net-stack-helper.h" // class QuantumNetStackHelper
#include "ns3/ent-swap-helper.h" // class EntSwapHelper
#include "ns3/ent-swap-app.h" // class EntSwapApp

#include <iostream>
#include <cmath>
#include <complex>

NS_LOG_COMPONENT_DEFINE ("EntSwapExample");

using namespace ns3;

// #define N (4) // 3 s, 62 tensors
#define N (8) // 12 s, 206 tensors
// #define N (16) // 821 s, 686 tensors (the last contraction takes 46 s)

int
main ()
{
  //
  // Create a quantum physical entity with N nodes.
  //
  std::vector<std::string> owners = {};
  for (int i = 0; i < N; ++i)
    {
      owners.push_back ("Owner" + std::to_string (i));
    }
  Ptr<QuantumPhyEntity> qphyent = CreateObject<QuantumPhyEntity> (owners);

  NodeContainer nodes;
  for (int i = 0; i < N; ++i)
    {
      Ptr<QuantumNode> node = qphyent->GetNode ("Owner" + std::to_string (i));
      nodes.Add (node);
    }

  //
  // Create a classical connection.
  //
  CsmaHelper csmaHelper;
  csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("1000kbps")));
  csmaHelper.SetChannelAttribute ("Delay",
                                  TimeValue (MilliSeconds (CLASSICAL_DELAY)));
  NetDeviceContainer devices = csmaHelper.Install (nodes); 

  InternetStackHelper stack;
  stack.Install (nodes);
  Ipv6AddressHelper address;
  address.SetBase ("2001:1::", Ipv6Prefix (64));
  Ipv6InterfaceContainer interfaces = address.Assign (devices); 

  unsigned rank = 0;
  for (const std::string &owner : owners)
    {
      qphyent->SetOwnerAddress (owner, interfaces.GetAddress (rank, 1));
      qphyent->SetOwnerRank (owner, rank);
      ++rank;
    }

  //
  // Install the quantum network stack.
  //
  QuantumNetStackHelper qstack;
  qstack.Install (nodes);

  //
  // Install the quantum applications.
  //

  //
  // Every node but the last one generates and shares a EPR pair with the next
  //
  for (int rank = 0; rank < N - 1; ++rank) {
    std::string srcOwner = "Owner" + std::to_string (rank);
    std::string dstOwner = "Owner" + std::to_string (rank + 1);
    Ptr<QuantumChannel> qconn = CreateObject<QuantumChannel> (srcOwner, dstOwner);
    // generate and distribute EPR
    Ptr<DistributeEPRSrcProtocol> dist_epr_src_app =
        qphyent->GetConn2Apps (qconn, APP_DIST_EPR).first->GetObject<DistributeEPRSrcProtocol> ();
    Simulator::Schedule (Seconds(CLASSICAL_DELAY), &DistributeEPRSrcProtocol::GenerateAndDistributeEPR,
                         dist_epr_src_app, std::pair<std::string, std::string>{
                          "Owner" + std::to_string (rank) + "_QubitEntToOwner"+ std::to_string (rank + 1),
                          "Owner" + std::to_string (rank + 1) + "_QubitEntFromOwner"+ std::to_string (rank)});
  }


  //
  // Setup the N - 2 entanglement swappings
  //
  std::string dstOwner = "Owner" + std::to_string (N - 1);
  for (int rank = 1; rank < N - 1; ++rank)
    {
      std::string srcOwner = "Owner" + std::to_string (rank);

      Ptr<QuantumChannel> qconn = CreateObject<QuantumChannel> (srcOwner, dstOwner);

      EntSwapSrcHelper srcHelper (qphyent, qconn);
      srcHelper.SetAttribute ("Qubits", PairValue<StringValue, StringValue> ({
                              srcOwner + "_QubitEntFromOwner" + std::to_string (rank - 1),
                              srcOwner + "_QubitEntToOwner" + std::to_string (rank + 1)
                              }));

      ApplicationContainer srcApps = srcHelper.Install (qphyent->GetNode (srcOwner));
      Ptr<EntSwapSrcApp> telepSrcApp = srcApps.Get (0)->GetObject<EntSwapSrcApp> ();

      telepSrcApp->SetStartTime (Seconds (TELEP_DELAY * rank));
      telepSrcApp->SetStopTime (Seconds (TELEP_DELAY * (rank + 1)));
    }

  EntSwapDstHelper dstHelper (qphyent, qphyent->GetNode (dstOwner));
  dstHelper.SetAttribute ("Qubit", StringValue (dstOwner + "_QubitEntFromOwner" + std::to_string (N - 2)));
  dstHelper.SetAttribute ("Count", UintegerValue (N - 2));

  ApplicationContainer dstApps = dstHelper.Install (qphyent->GetNode (dstOwner));
  Ptr<EntSwapDstApp> dstApp = dstApps.Get (0)->GetObject<EntSwapDstApp> ();

  dstApp->SetStartTime (Seconds (CLASSICAL_DELAY));
  dstApp->SetStopTime (Seconds (TELEP_DELAY * (N - 1)));

  //
  // Run the simulation.
  //
  Simulator::Stop (Seconds (TELEP_DELAY * (N - 1)));
  auto start = std::chrono::high_resolution_clock::now ();
  Simulator::Run ();
  auto end = std::chrono::high_resolution_clock::now ();
  printf ("Time taken: %ld s\n",
          std::chrono::duration_cast<std::chrono::seconds> (end - start).count ());
  Simulator::Destroy ();

  return 0;
}

/* 
  NS_LOG="QuantumBasis=info:QuantumNetworkSimulator=info:EntSwapAdaptApp=logic:DistributeEPRProtocol=info:QuantumNode=info" ./ns3 run ent-swap-example
*/