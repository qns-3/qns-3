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
  csmaHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
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
  Ptr<QuantumMemory> src_qubits = CreateObject<QuantumMemory> (
      std::vector<std::string>{"A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7"});
  Ptr<QuantumMemory> dst_qubits = CreateObject<QuantumMemory> (
      std::vector<std::string>{"B0", "B1", "B2", "B3", "B4", "B5", "B6", "B7"});
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
  Simulator::Stop (Seconds (10.));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
