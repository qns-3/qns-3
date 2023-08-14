#include "ns3/csma-module.h" // class CsmaHelper, NetDeviceContainer
#include "ns3/internet-module.h" // class InternetStackHelper, Ipv6AddressHelper, Ipv6InterfaceContainer

#include "ns3/quantum-basis.h" // class APP_DISTILL_NESTED
#include "ns3/quantum-network-simulator.h" // class QuantumNetworkSimulator
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-channel.h" // class QuantumChannel
#include "ns3/distribute-epr-helper.h" // class DistributeEPRSrcHelper, DistributeEPRDstHelper
#include "ns3/quantum-net-stack-helper.h" // class QuantumNetStackHelper
#include "ns3/telep-adapt-helper.h" // class TelepAdaptHelper
#include "ns3/telep-adapt-app.h" // class TelepAdaptApp

#include <iostream>
#include <cmath>
#include <complex>

NS_LOG_COMPONENT_DEFINE ("TelepLinAdaptExample");

using namespace ns3;

// #define N (5)
#define N (8) // 1 s, 198 tensors
// #define N (16) // 12 s, 422 tensors
// #define N (32) // 133 s, 870 tensors

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
  csmaHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (0.1)));
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
  // Setup the first teleporatation from Owner0 to Owner1.
  //
  Ptr<QuantumChannel> qconn1 = CreateObject<QuantumChannel> ("Owner0", "Owner1");
  qconn1->SetDepolarModel (0.93, qphyent);

  Ptr<Qubit> input1 = CreateObject<Qubit> (std::vector<std::complex<double>>{
      {sqrt (5. / 7.), 0.0},
      {0.0, sqrt (2. / 7.)}});
  TelepAdaptHelper helper1 (qphyent, qconn1);
  helper1.SetAttribute ("Qubits",
                        PairValue<StringValue, StringValue> ({"Owner0_Qubit0", "Owner0_Qubit1"}));
  helper1.SetAttribute ("Qubit", StringValue ("Owner1_Qubit0"));
  helper1.SetAttribute ("Input", PointerValue (input1));

  ApplicationContainer srcApp1 = helper1.Install (qphyent->GetNode ("Owner1"));
  srcApp1.Start (Seconds (0.));
  srcApp1.Stop (Seconds (CLASSICAL_DELAY));

  //
  // Setup the rest N - 2 teleportations from Owner1 to OwnerN-1.
  //
  for (int rank = 1; rank < N - 1; ++rank)
    {
      std::string srcOwner = "Owner" + std::to_string (rank);
      std::string dstOwner = "Owner" + std::to_string (rank + 1);

      Ptr<QuantumChannel> qconn =
          CreateObject<QuantumChannel> (std::pair<std::string, std::string>{srcOwner, dstOwner});
      qconn->SetDepolarModel (0.93, qphyent);

      TelepAdaptHelper helper (qphyent, qconn);
      helper.SetAttribute ("Qubits", PairValue<StringValue, StringValue> (
                                         {srcOwner + "_Qubit0", srcOwner + "_Qubit1"}));
      helper.SetAttribute ("Qubit", StringValue (dstOwner + "_Qubit0"));
      helper.SetAttribute ("Input", PointerValue (nullptr));
      helper.SetAttribute ("LastOwner", StringValue ("Owner" + std::to_string (N - 1)));

      ApplicationContainer srcApp = helper.Install (qphyent->GetNode (srcOwner));
      srcApp.Start (Seconds (CLASSICAL_DELAY * rank));
      srcApp.Stop (Seconds (CLASSICAL_DELAY * (rank + 1)));
    }

  //
  // Run the simulation.
  //
  Simulator::Stop (Seconds (CLASSICAL_DELAY * N));
  auto start = std::chrono::high_resolution_clock::now ();
  Simulator::Run ();
  auto end = std::chrono::high_resolution_clock::now ();
  printf ("Time taken: %ld ms\n",
          std::chrono::duration_cast<std::chrono::milliseconds> (end - start).count ());
  Simulator::Destroy ();

  return 0;
}
