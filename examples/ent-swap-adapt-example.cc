#include "ns3/csma-module.h" // class CsmaHelper, NetDeviceContainer
#include "ns3/internet-module.h" // class InternetStackHelper, Ipv6AddressHelper, Ipv6InterfaceContainer

#include "ns3/quantum-basis.h"
#include "ns3/quantum-network-simulator.h" // class QuantumNetworkSimulator
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-memory.h" // class QuantumMemory
#include "ns3/quantum-channel.h" // class QuantumChannel
#include "ns3/distribute-epr-helper.h" // class DistributeEPRSrcHelper, DistributeEPRDstHelper
#include "ns3/distribute-epr-protocol.h" // class DistributeEPRSrcProtocol
#include "ns3/quantum-net-stack-helper.h" // class QuantumNetStackHelper
#include "ns3/ent-swap-adapt-helper.h" // class EntSwapAdaptHelper
#include "ns3/ent-swap-adapt-app.h" // class EntSwapAdaptApp

#include <iostream>
#include <cmath>
#include <complex>

NS_LOG_COMPONENT_DEFINE ("EntSwapAdaptExample");

using namespace ns3;

// #define N (8) // 0 s, 126 tensors
#define N (16) // 3 s, 270 tensors
// #define N (32) // 30 s, 558 tensors
// #define N (64) // 280 s, 1134 tensors

int
main ()
{
  //
  // Create a quantum physical entity with N nodes.
  //
  std::vector<std::string> owners = {"God"};
  for (int i = 0; i < N; ++i)
    {
      owners.push_back ("Owner" + std::to_string (i));
    }
  Ptr<QuantumPhyEntity> qphyent = CreateObject<QuantumPhyEntity> (owners);

  NodeContainer nodes;
  for (int i = 0; i < owners.size (); ++i)
    {
      Ptr<QuantumNode> node = qphyent->GetNode (owners[i]);
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
    std::string lastOwner = "Owner" + std::to_string (rank + 1);
    Ptr<QuantumChannel> qconn = CreateObject<QuantumChannel> (srcOwner, lastOwner);
    // generate and distribute EPR
    Ptr<DistributeEPRSrcProtocol> dist_epr_src_app =
        qphyent->GetConn2Apps (qconn, APP_DIST_EPR).first->GetObject<DistributeEPRSrcProtocol> ();
    Simulator::Schedule (Seconds (SETUP_DELAY), &DistributeEPRSrcProtocol::GenerateAndDistributeEPR,
                         dist_epr_src_app, std::pair<std::string, std::string>{
                          "Owner" + std::to_string (rank) + "_QubitEntToOwner"+ std::to_string (rank + 1),
                          "Owner" + std::to_string (rank + 1) + "_QubitEntFromOwner"+ std::to_string (rank)});
  }


  //
  // Setup the N - 2 entanglement swappings
  //
  std::vector<std::string> former_qubits_vec = {""};
  std::vector<std::string> latter_qubits_vec = {"Owner0_QubitEntToOwner1"};
  for (int rank = 1; rank < N - 1; ++rank) {
    former_qubits_vec.push_back ("Owner" + std::to_string (rank) + "_QubitEntFromOwner" + std::to_string (rank - 1));
    latter_qubits_vec.push_back ("Owner" + std::to_string (rank) + "_QubitEntToOwner" + std::to_string (rank + 1));
  }
  former_qubits_vec.push_back ("Owner" + std::to_string (N - 1) + "_QubitEntFromOwner" + std::to_string (N - 2));
  latter_qubits_vec.push_back ("");

  Ptr<QuantumMemory> former_qubits = CreateObject<QuantumMemory> (former_qubits_vec);
  Ptr<QuantumMemory> latter_qubits = CreateObject<QuantumMemory> (latter_qubits_vec);

  std::string lastOwner = "Owner" + std::to_string (N - 1);
  EntSwapAdaptHelper dstHelper (qphyent);
  dstHelper.SetAttribute ("QubitsFormer", PointerValue (former_qubits));
  dstHelper.SetAttribute ("QubitsLatter", PointerValue (latter_qubits));

  ApplicationContainer dstApps = dstHelper.Install (qphyent->GetNode (lastOwner));
  Ptr<EntSwapAdaptApp> dstApp = dstApps.Get (0)->GetObject<EntSwapAdaptApp> ();

  dstApp->SetStartTime (Seconds (SETUP_DELAY + N * DIST_EPR_DELAY));
  dstApp->SetStopTime (Seconds (SETUP_DELAY + (N + 1) * DIST_EPR_DELAY));

  //
  // Run the simulation.
  //
  Simulator::Stop (Seconds (SETUP_DELAY + (N + 1) * DIST_EPR_DELAY));
  auto start = std::chrono::high_resolution_clock::now ();
  Simulator::Run ();
  auto end = std::chrono::high_resolution_clock::now ();
  printf ("Time taken: %ld s\n",
          std::chrono::duration_cast<std::chrono::seconds> (end - start).count ());
  Simulator::Destroy ();

  return 0;
}

/* 
  NS_LOG="QuantumBasis=info:QuantumNetworkSimulator=info:EntSwapAdaptApp=logic:DistributeEPRProtocol=info:QuantumNode=info" ./ns3 run ent-swap-adapt-example
*/