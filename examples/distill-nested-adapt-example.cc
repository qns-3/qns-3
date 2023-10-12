/*
  To run this example:
  NS_LOG="QuantumNetworkSimulator=info:QuantumPhyEntity=info:DistillNestedAdaptApp=info:QuantumBasis=info" ./ns3 run distill-nested-adapt-example
  Note that by setting DistillNestedAdaptApp=info:QuantumBasis=info,
  the simulator logs the prob of seccess, the meas outcome of the flag qubit, and the final fidelity.
*/
#include "ns3/csma-module.h" // class CsmaHelper, NetDeviceContainer
#include "ns3/internet-module.h" // class InternetStackHelper, Ipv6AddressHelper, Ipv6InterfaceContainer

#include "ns3/quantum-basis.h"
#include "ns3/quantum-network-simulator.h" // class QuantumNetworkSimulator
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-channel.h" // class QuantumChannel
#include "ns3/distribute-epr-helper.h" // class DistributeEPRhelper, DistributeEPRDstHelper
#include "ns3/quantum-net-stack-helper.h" // class QuantumNetStackHelper

#include "ns3/distill-nested-adapt-app.h" // class DistillNestedAdaptApp
#include "ns3/distill-nested-adapt-helper.h" // class DistillNestedAdaptHelper

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DistillNestedAdaptExample");

// N stands for the # of EPR pairs shared between Alice and Bob

// #define N (8)
/*
Evaluating tensor network of size 261
 in 8.22895 secs
*/
// #define N (16)
/*
Evaluating tensor network of size 549
 in 17.4536 secs
*/
// #define N (32)
/*
Evaluating tensor network of size 1125
 in 23.7538 secs
*/
// #define N (64)
/*
Evaluating tensor network of size 2277
 in 53.9866 secs
*/
#define N (128)
/*
Evaluating tensor network of size 4581
 in 108.108 secs
*/

int
main ()
{
  //
  // Create a quantum physical entity with two nodes.
  //
  std::vector<std::string> owners = {"Alice", "Bob"};
  Ptr<QuantumPhyEntity> qphyent = CreateObject<QuantumPhyEntity> (owners);

  //
  // (Optional) Set the error models.
  //
  NodeContainer nodes;
  Ptr<QuantumNode> alice = qphyent->GetNode ("Alice");
  alice->SetTimeModel (2e1); // it's huge, because CC is slow and would cause too severe a dephase loss
  nodes.Add (alice);

  Ptr<QuantumNode> bob = qphyent->GetNode ("Bob");
  bob->SetTimeModel (2e1);
  nodes.Add (bob);

  //
  // Create a classical connection.
  //
  CsmaHelper csmaHelper;
  csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("1000kbps")));
  csmaHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (1e-1)));
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
  Ptr<QuantumChannel> qconn = CreateObject<QuantumChannel> (alice, bob);
  qconn->SetDepolarModel (0.95, qphyent);

  DistillNestedAdaptHelper helper (qphyent, false, qconn);

  std::vector<std::string> qubits_alice = {};
  std::vector<std::string> qubits_bob = {};
  for (int i = 0; i < N; ++i)
    {
      qubits_alice.push_back ("A" + std::to_string (i));
      qubits_bob.push_back ("B" + std::to_string (i));
    }
  Ptr<QuantumMemory> src_qubits = CreateObject<QuantumMemory> (qubits_alice);
  Ptr<QuantumMemory> dst_qubits = CreateObject<QuantumMemory> (qubits_bob);

  helper.SetAttribute ("SrcQubits", PointerValue (src_qubits));
  helper.SetAttribute ("DstQubits", PointerValue (dst_qubits));
  helper.SetAttribute ("FlagQubit", StringValue ("Flag"));

  ApplicationContainer apps = helper.Install (alice);
  Ptr<DistillNestedAdaptApp> distillSrcApp = apps.Get (0)->GetObject<DistillNestedAdaptApp> ();
  apps.Start (Seconds (0.));
  apps.Stop (Seconds (ETERNITY));

  Simulator::Stop (Seconds (ETERNITY));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
