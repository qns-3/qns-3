// Notations
// - We use the notation "(X)" for some class X without an explicit abstraction,
//   whereas "[X]" for some explicitly encapsulated class X in our simulator.
//
// - "[X] with [Y]" indicates that class X is composed of class Y.

// Telep Network Topology
//  alice     bob
//      |       |
//      =========
//    LAN 10.1.1.0

#include "ns3/csma-module.h" // class CsmaHelper, NetDeviceContainer
#include "ns3/internet-module.h" // class InternetStackHelper, Ipv6AddressHelper, Ipv6InterfaceContainer

#include "ns3/quantum-basis.h" // class APP_DISTILL_NESTED
#include "ns3/quantum-network-simulator.h" // class QuantumNetworkSimulator
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-channel.h" // class QuantumChannel
#include "ns3/distribute-epr-helper.h" // class DistributeEPRSrcHelper, DistributeEPRDstHelper
#include "ns3/quantum-net-stack-helper.h" // class QuantumNetStackHelper
#include "ns3/telep-helper.h" // class TelepAppSrcHelper, TelepAppDstHelper

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TelepAppExample");

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
  bob->SetDephaseModel ("PX", 0.23);
  nodes.Add (bob);

  //
  // Create a classical connection.
  //
  CsmaHelper csmaHelper;
  csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("1000kbps")));
  csmaHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
  NetDeviceContainer devices = csmaHelper.Install (nodes);

  //
  // Install the classical network stack.
  //
  InternetStackHelper stack;
  stack.Install (nodes);
  Ipv6AddressHelper address;
  address.SetBase ("2001:1::", Ipv6Prefix (64));
  Ipv6InterfaceContainer interfaces = address.Assign (
      devices);

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
  // Create a quantum connection.
  // (Optional) Setup the error model.
  //
  Ptr<QuantumChannel> qconn =
      CreateObject<QuantumChannel> (std::pair<std::string, std::string>{"Alice", "Bob"});
  qconn->SetDepolarModel (0.93, qphyent);

  //
  // Quantum - Software
  // Create applications on nodes alice and bob.
  //
  Ptr<Qubit> input = CreateObject<Qubit> (std::vector<std::complex<double>>{
      {sqrt (5. / 7.), 0.0},
      {0.0, sqrt (2. / 7.)}}); // state vector of the qubit to teleport
  TelepSrcHelper srcHelper (qphyent, qconn);
  srcHelper.SetAttribute ("Qubits", PairValue<StringValue, StringValue> ({"Alice0", "Alice1"}));
  srcHelper.SetAttribute ("Qubit", StringValue ("Bob0"));
  srcHelper.SetAttribute ("Input", PointerValue (input));

  ApplicationContainer srcApp = srcHelper.Install (alice);
  srcApp.Start (Seconds (2.));
  srcApp.Stop (Seconds (20.));

  TelepDstHelper dstHelper (qphyent, qconn);
  dstHelper.SetAttribute ("Qubit", StringValue ("Bob0"));

  ApplicationContainer dstApp = dstHelper.Install (bob);
  dstApp.Start (Seconds (2.));
  dstApp.Stop (Seconds (20.));

  //
  // Run the simulator.
  // TelepAppSrc teleports a qubit to some dst node.
  // TelepAppDst receives a qubit from some src node.
  //
  Simulator::Stop (Seconds (20.));
  Simulator::Run ();
  Simulator::Destroy ();


  return 0;
}
