#include "ns3/csma-module.h" // class CsmaHelper, NetDeviceContainer
#include "ns3/internet-module.h" // class InternetStackHelper, Ipv6AddressHelper, Ipv6InterfaceContainer

#include "ns3/quantum-basis.h"
#include "ns3/quantum-network-simulator.h" // class QuantumNetworkSimulator
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-channel.h" // class QuantumChannel
#include "ns3/distribute-epr-helper.h" // class DistributeEPRSrcHelper, DistributeEPRDstHelper
#include "ns3/quantum-net-stack-helper.h" // class QuantumNetStackHelper
#include "ns3/distill-app.h" // class DistillApp
#include "ns3/distill-helper.h" // class DistillHelper

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DistillAppRepeatExample");

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
  // Setup the first distillation.
  //
  DistillHelper srcHelper (qphyent, false, qconn);
  srcHelper.SetAttribute ("EPRGoal", PairValue<StringValue, StringValue> ({"Alice0", "Bob0"}));
  srcHelper.SetAttribute ("EPRMeas", PairValue<StringValue, StringValue> ({"Alice1", "Bob1"}));

  ApplicationContainer srcApps = srcHelper.Install (alice);
  Ptr<DistillApp> distillSrcApp = srcApps.Get (0)->GetObject<DistillApp> ();
  srcApps.Start (Seconds (2.));
  srcApps.Stop (Seconds (10.));

  DistillHelper dstHelper (qphyent, true, qconn);
  dstHelper.SetAttribute ("Qubits", PairValue<StringValue, StringValue> ({"Bob0", "Bob1"}));

  ApplicationContainer dstApps = dstHelper.Install (bob);
  Ptr<DistillApp> distillDstApp = dstApps.Get (0)->GetObject<DistillApp> ();
  dstApps.Start (Seconds (2.));
  dstApps.Stop (Seconds (10.));

  Simulator::Stop (Seconds (10.));
  Simulator::Run ();

  //
  // Setup the second distillation.
  //
  distillSrcApp->SetEPRGoal ({"Alice2", "Bob2"});
  distillSrcApp->SetEPRMeas ({"Alice3", "Bob3"});
  distillSrcApp->SetQubits ({"Bob2", "Bob3"});

  Simulator::Schedule (Seconds (2.), &DistillApp::Distillate, distillSrcApp);

  //
  // Run the simulation.
  //
  Simulator::Stop (Seconds (10.));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
