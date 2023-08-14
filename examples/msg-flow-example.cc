#include "ns3/csma-module.h" // class CsmaHelper, NetDeviceContainer
#include "ns3/internet-module.h" // class InternetStackHelper, Ipv6AddressHelper, Ipv6InterfaceContainer

#include "ns3/quantum-basis.h"
#include "ns3/quantum-network-simulator.h" // class QuantumNetworkSimulator
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-channel.h" // class QuantumChannel
#include "ns3/distribute-epr-helper.h" // class DistributeEPRSrcHelper, DistributeEPRDstHelper
#include "ns3/quantum-net-stack-helper.h" // class QuantumNetStackHelper
#include "ns3/telep-helper.h" // class TelepAppSrcHelper, TelepAppDstHelper
#include "ns3/telep-app.h" // class TelepSrcApp, TelepDstApp

#include <iostream>
#include <cmath> // sqrt()
#include <complex>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MsgFlowExample");

int
main ()
{
  //
  // Create a quantum physical entity with three nodes.
  //
  std::vector<std::string> owners = {"Alice", "Bob", "Charlie"};
  Ptr<QuantumPhyEntity> qphyent = CreateObject<QuantumPhyEntity> (owners);

  //
  // (Optional) Set the error models.
  //
  NodeContainer nodes;
  Ptr<QuantumNode> alice = qphyent->GetNode ("Alice");
  alice->SetTimeModel (0.13);
  nodes.Add (alice);

  Ptr<QuantumNode> bob = qphyent->GetNode ("Bob");
  bob->SetDephaseModel ("PX", 0.23);
  nodes.Add (bob);

  Ptr<QuantumNode> charlie = qphyent->GetNode ("Charlie");
  nodes.Add (charlie);

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


  //
  // Setup the teleportation from Alice to Bob.
  //
  Ptr<QuantumChannel> qconn1 = CreateObject<QuantumChannel> (alice, bob);
  qconn1->SetDepolarModel (0.93, qphyent);

  Ptr<Qubit> input1 = CreateObject<Qubit> (std::vector<std::complex<double>>{
      {sqrt (5. / 7.), 0.0},
      {0.0, sqrt (2. / 7.)}});
  TelepSrcHelper srcHelper1 (qphyent, qconn1);
  srcHelper1.SetAttribute ("Qubits", PairValue<StringValue, StringValue> ({"Alice0", "Alice1"}));
  srcHelper1.SetAttribute ("Qubit", StringValue ("Bob0"));
  srcHelper1.SetAttribute ("Input", PointerValue (input1));

  ApplicationContainer srcApps1 = srcHelper1.Install (alice);
  Ptr<TelepSrcApp> telepSrcApp1 = srcApps1.Get (0)->GetObject<TelepSrcApp> ();

  telepSrcApp1->SetStartTime (Seconds (2.));
  telepSrcApp1->SetStopTime (Seconds (10.));

  TelepDstHelper dstHelper1 (qphyent, qconn1);
  dstHelper1.SetAttribute ("Qubit", StringValue ("Bob0"));

  ApplicationContainer dstApps1 = dstHelper1.Install (bob);
  Ptr<TelepDstApp> telepDstApp1 = dstApps1.Get (0)->GetObject<TelepDstApp> ();

  telepDstApp1->SetStartTime (Seconds (2.));
  telepDstApp1->SetStopTime (Seconds (10.));

  //
  // Setup the teleportation from Bob to Charlie.
  //
  Ptr<QuantumChannel> qconn2 = CreateObject<QuantumChannel> (bob, charlie);
  qconn2->SetDepolarModel (0.93, qphyent);

  Ptr<Qubit> input2 = CreateObject<Qubit> (
      std::vector<std::complex<double>>{{sqrt (1. / 4.), 0.0}, {0.0, sqrt (3. / 4.)}});
  TelepSrcHelper srcHelper2 (qphyent, qconn2);
  srcHelper2.SetAttribute ("Qubits", PairValue<StringValue, StringValue> ({"Bob1", "Bob2"}));
  srcHelper2.SetAttribute ("Qubit", StringValue ("Charlie0"));
  srcHelper2.SetAttribute ("Input", PointerValue (input2));

  ApplicationContainer srcApps2 = srcHelper2.Install (bob);
  Ptr<TelepSrcApp> telepSrcApp2 = srcApps2.Get (0)->GetObject<TelepSrcApp> ();

  telepSrcApp2->SetStartTime (Seconds (2.));
  telepSrcApp2->SetStopTime (Seconds (10.));

  TelepDstHelper dstHelper2 (qphyent, qconn2);
  dstHelper2.SetAttribute ("Qubit", StringValue ("Charlie0"));

  ApplicationContainer dstApps2 = dstHelper2.Install (charlie);
  Ptr<TelepDstApp> telepDstApp2 = dstApps2.Get (0)->GetObject<TelepDstApp> ();

  telepDstApp2->SetStartTime (Seconds (2.));
  telepDstApp2->SetStopTime (Seconds (10.));

  //
  // Run the simulation.
  //
  Simulator::Stop (Seconds (10.));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
