/* 
  To run this example:
  NS_LOG="QuantumNetworkSimulator=info:QuantumPhyEntity=info|logic" ./ns3 run ent-swap-adapt-local-example
*/

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
#include "ns3/ent-swap-adapt-local-helper.h" // class EntSwapAdaptLocalLocalHelper
#include "ns3/ent-swap-adapt-local-app.h" // class EntSwapAdaptLocalLocalApp

#include <iostream>
#include <cmath>
#include <complex>

NS_LOG_COMPONENT_DEFINE ("EntSwapAdaptLocalExample");

using namespace ns3;


/* 
  With proper control flow adaptation,
  the time cost of contraction scales linearly with N, the number of owners:
*/

// #define N (8)
/*
Evaluating tensor network of size 71 in 2.44344 secs
*/
// #define N (16)
/*
Evaluating tensor network of size 159 in 5.48751 secs
*/
// #define N (32)
/*
Evaluating tensor network of size 335 in 11.2599 secs
*/
#define N (64)
/*
Evaluating tensor network of size 687 in 23.3415 secs
*/
// #define N (128)
/*
Evaluating tensor network of size 1391 in 39.0065 secs
*/
// #define N (1024)
/*
Evaluating tensor network of size 11247 in 333.711 secs
*/

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

  // set the PX and PZ gate error of Owner N - 1 
  Ptr<QuantumNode> last_node = qphyent->GetNode ("Owner" + std::to_string (N - 1));
  last_node->SetDephaseModel (QNS_GATE_PREFIX + "PX", 1.2);
  last_node->SetDephaseModel (QNS_GATE_PREFIX + "PZ", 1.2);

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
  std::vector<std::string> former_qubits_vec = {""};
  std::vector<std::string> latter_qubits_vec = {"Owner0_Qubit1"};
  for (int rank = 1; rank < N - 1; ++rank) {
    former_qubits_vec.push_back ("Owner" + std::to_string (rank) + "_Qubit0");
    latter_qubits_vec.push_back ("Owner" + std::to_string (rank) + "_Qubit1");
  }
  former_qubits_vec.push_back ("Owner" + std::to_string (N - 1) + "_Qubit0");
  latter_qubits_vec.push_back ("");

  Ptr<QuantumMemory> former_qubits = CreateObject<QuantumMemory> (former_qubits_vec);
  Ptr<QuantumMemory> latter_qubits = CreateObject<QuantumMemory> (latter_qubits_vec);

  std::string lastOwner = "Owner" + std::to_string (N - 1);
  EntSwapAdaptLocalHelper dstHelper (qphyent);
  dstHelper.SetAttribute ("QubitsFormer", PointerValue (former_qubits));
  dstHelper.SetAttribute ("QubitsLatter", PointerValue (latter_qubits));

  ApplicationContainer dstApps = dstHelper.Install (qphyent->GetNode (lastOwner));
  Ptr<EntSwapAdaptLocalApp> dstApp = dstApps.Get (0)->GetObject<EntSwapAdaptLocalApp> ();

  dstApp->SetStartTime (Seconds (SETUP_DELAY + N * DIST_EPR_DELAY));
  dstApp->SetStopTime (Seconds (SETUP_DELAY + (N + 1) * DIST_EPR_DELAY));

  //
  // Run the simulation.
  //
  Simulator::Stop (Seconds (SETUP_DELAY + (N + 1) * DIST_EPR_DELAY));
  auto start = std::chrono::high_resolution_clock::now ();
  Simulator::Run ();
  auto end = std::chrono::high_resolution_clock::now ();
  printf ("Total time cost: %ld s\n",
          std::chrono::duration_cast<std::chrono::seconds> (end - start).count ());
  Simulator::Destroy ();

  return 0;
}

