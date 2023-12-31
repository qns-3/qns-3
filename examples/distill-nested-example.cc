/*
  To run this example:
  NS_LOG="QuantumNetworkSimulator=info:QuantumPhyEntity=info|logic" ./ns3 run distill-nested-example

*/
#include "ns3/csma-module.h" // class CsmaHelper, NetDeviceContainer
#include "ns3/internet-module.h" // class InternetStackHelper, Ipv6AddressHelper, Ipv6InterfaceContainer

#include "ns3/quantum-basis.h"
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-channel.h" // class QuantumChannel
#include "ns3/distribute-epr-helper.h" // class DistributeEPRSrcHelper, DistributeEPRDstHelper
#include "ns3/quantum-net-stack-helper.h" // class QuantumNetStackHelper
#include "ns3/distill-nested-app.h" // class DistillNestedApp
#include "ns3/distill-nested-helper.h" // class DistillNestedHelper

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DistillNestedExample");

// N stands for the # of EPR pairs shared between Alice and Bob

// #define N (4)
/*
Last round cost:
  Evaluating tensor network of size 127
   in 4.98379 secs
  Evaluating tensor network of size 151
   in 0.819106 secs
Total time cost: 7 s
*/
#define N (8)
/*
Last round cost:
  Evaluating tensor network of size 439
   in 13.0032 secs
  Evaluating tensor network of size 479
   in 16.3508 secs
Total time cost: 83 s
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
  alice->SetTimeModel (0.13);
  nodes.Add (alice);

  Ptr<QuantumNode> bob = qphyent->GetNode ("Bob");
  bob->SetDephaseModel (QNS_GATE_PREFIX + "CNOT", 0.23);
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
  qconn->SetDepolarModel (0.93, qphyent);

  //
  // Setup the source application.
  //
  DistillNestedHelper srcHelper (qphyent, false, qconn);

  std::vector<std::string> qubits_alice = {};
  std::vector<std::string> qubits_bob = {};
  for (int i = 0; i < N; ++i)
    {
      qubits_alice.push_back ("A" + std::to_string (i));
      qubits_bob.push_back ("B" + std::to_string (i));
    }
  Ptr<QuantumMemory> src_qubits = CreateObject<QuantumMemory> (qubits_alice);
  Ptr<QuantumMemory> dst_qubits = CreateObject<QuantumMemory> (qubits_bob);

  srcHelper.SetAttribute ("SrcQubits", PointerValue (src_qubits));
  srcHelper.SetAttribute ("DstQubits", PointerValue (dst_qubits));

  ApplicationContainer srcApps = srcHelper.Install (alice);
  Ptr<DistillNestedApp> distillSrcApp = srcApps.Get (0)->GetObject<DistillNestedApp> ();
  srcApps.Start (Seconds (0.));
  srcApps.Stop (Seconds (10.));


  //
  // Setup the destination application.
  //
  DistillNestedHelper dstHelper (qphyent, true, qconn);
  dstHelper.SetAttribute ("SrcQubits", PointerValue (nullptr));
  dstHelper.SetAttribute ("DstQubits", PointerValue (dst_qubits));
  ApplicationContainer dstApps = dstHelper.Install (bob);
  Ptr<DistillNestedApp> distillDstApp = dstApps.Get (0)->GetObject<DistillNestedApp> ();
  dstApps.Start (Seconds (0.));
  dstApps.Stop (Seconds (10.));

  //
  // Record the quantum connection for the many rounds of distillations.
  //
  qphyent->AddConn2Apps (qconn, APP_DISTILL_NESTED, std::make_pair (distillSrcApp, distillDstApp));

  //
  // Run the simulation.
  //
  Simulator::Stop (Seconds(10.));
  auto start = std::chrono::high_resolution_clock::now ();
  Simulator::Run ();
  auto end = std::chrono::high_resolution_clock::now ();
  printf ("Total time cost: %ld s\n",
          std::chrono::duration_cast<std::chrono::seconds> (end - start).count ());
  Simulator::Destroy ();

  return 0;
}
