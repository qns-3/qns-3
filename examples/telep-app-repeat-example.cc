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

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TelepAppRepeatExample");

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
  // Install the quantum applications.
  //
  Ptr<QuantumChannel> qconn = CreateObject<QuantumChannel> (alice, bob);
  qconn->SetDepolarModel (0.93, qphyent);

  //
  // Setup the first teleportation.
  //
  Ptr<Qubit> input1 = CreateObject<Qubit> (std::vector<std::complex<double>>{
      {sqrt (5. / 7.), 0.0},
      {0.0, sqrt (2. / 7.)}});
  TelepSrcHelper srcHelper (qphyent, qconn);
  srcHelper.SetAttribute ("Qubits", PairValue<StringValue, StringValue> ({"Alice0", "Alice1"}));
  srcHelper.SetAttribute ("Qubit", StringValue ("Bob0"));
  srcHelper.SetAttribute ("Input", PointerValue (input1));

  ApplicationContainer srcApps = srcHelper.Install (alice);
  Ptr<TelepSrcApp> telepSrcApp = srcApps.Get (0)->GetObject<TelepSrcApp> ();

  telepSrcApp->SetStartTime (Seconds (2.));
  telepSrcApp->SetStopTime (Seconds (10.));

  TelepDstHelper dstHelper (qphyent, qconn);
  dstHelper.SetAttribute ("Qubit", StringValue ("Bob0"));

  ApplicationContainer dstApps = dstHelper.Install (bob);
  Ptr<TelepDstApp> telepDstApp = dstApps.Get (0)->GetObject<TelepDstApp> ();

  telepDstApp->SetStartTime (Seconds (2.));
  telepDstApp->SetStopTime (Seconds (10.));

  Simulator::Stop (Seconds (10.));
  Simulator::Run ();

  //
  // Setup the second teleportation.
  //
  Ptr<Qubit> input2 = CreateObject<Qubit> (
      std::vector<std::complex<double>>{{sqrt (5. / 9.), 0.0}, {0.0, sqrt (4. / 9.)}});
  telepSrcApp->SetQubits ({"Alice2", "Alice3"});
  telepSrcApp->SetQubit ("Bob1");
  telepSrcApp->SetInput (input2);

  Simulator::Schedule (Seconds (2.), &TelepSrcApp::Teleport, telepSrcApp);
  telepDstApp->SetQubit ("Bob1");

  //
  // Run the simulation.
  //
  Simulator::Stop (Seconds (20.));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
