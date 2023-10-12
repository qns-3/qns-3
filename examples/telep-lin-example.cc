/*
  To run this example:
  NS_LOG="QuantumNetworkSimulator=info:QuantumPhyEntity=info|logic" ./ns3 run telep-lin-example
*/
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
#include <cmath>
#include <complex>

NS_LOG_COMPONENT_DEFINE ("TelepLinExample");

using namespace ns3;

// #define N (4)
/*
Last round cost:
  Evaluating tensor network of size 110 in 3.83961 secs
  Evaluating tensor network of size 112 in 2.02858 secs
Total time cost: 12 s
*/
// #define N (8)
/*
Last round cost:
  Evaluating tensor network of size 326 in 5.12221 secs
  Evaluating tensor network of size 328 in 5.92808 secs
Total time cost: 37 s
*/
#define N (12)
/*
Last round cost:
  Evaluating tensor network of size 606 in 32.0905 secs
  Evaluating tensor network of size 608 in 35.4042 secs
Total time cost: 290 s
*/

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
  // Setup the first teleportation from Owner0 to Owner1.
  //
  Ptr<QuantumChannel> qconn1 = CreateObject<QuantumChannel> ("Owner0", "Owner1");
  qconn1->SetDepolarModel (0.93, qphyent);

  Ptr<Qubit> input1 = CreateObject<Qubit> (std::vector<std::complex<double>>{
      {sqrt (5. / 7.), 0.0},
      {0.0, sqrt (2. / 7.)}});
  TelepSrcHelper srcHelper1 (qphyent, qconn1);
  srcHelper1.SetAttribute (
      "Qubits", PairValue<StringValue, StringValue> ({"Owner0_Qubit0", "Owner0_Qubit1"}));
  srcHelper1.SetAttribute ("Qubit", StringValue ("Owner1_Qubit0"));
  srcHelper1.SetAttribute ("Input", PointerValue (input1));

  ApplicationContainer srcApps1 = srcHelper1.Install (qphyent->GetNode ("Owner0"));
  Ptr<TelepSrcApp> telepSrcApp1 = srcApps1.Get (0)->GetObject<TelepSrcApp> ();

  telepSrcApp1->SetStartTime (Seconds (0.));
  telepSrcApp1->SetStopTime (Seconds (TELEP_DELAY));

  TelepDstHelper dstHelper1 (qphyent, qconn1);
  dstHelper1.SetAttribute ("Qubit", StringValue ("Owner1_Qubit0"));

  ApplicationContainer dstApps1 = dstHelper1.Install (qphyent->GetNode ("Owner1"));
  Ptr<TelepDstApp> telepDstApp1 = dstApps1.Get (0)->GetObject<TelepDstApp> ();

  telepDstApp1->SetStartTime (Seconds (0.));
  telepDstApp1->SetStopTime (Seconds (TELEP_DELAY));

  //
  // Setup the rest N - 2 teleportations from Owner1 to OwnerN-1.
  //
  for (int rank = 1; rank < N - 1; ++rank)
    {
      std::string srcOwner = "Owner" + std::to_string (rank);
      std::string dstOwner = "Owner" + std::to_string (rank + 1);

      Ptr<QuantumChannel> qconn = CreateObject<QuantumChannel> (srcOwner, dstOwner);
      qconn->SetDepolarModel (0.93, qphyent);

      TelepSrcHelper srcHelper (qphyent, qconn);
      srcHelper.SetAttribute ("Qubits", PairValue<StringValue, StringValue> (
                                            {srcOwner + "_Qubit0", srcOwner + "_Qubit1"}));
      srcHelper.SetAttribute ("Qubit", StringValue (dstOwner + "_Qubit0"));
      srcHelper.SetAttribute ("Input", PointerValue (nullptr));

      ApplicationContainer srcApps = srcHelper.Install (qphyent->GetNode (srcOwner));
      Ptr<TelepSrcApp> telepSrcApp = srcApps.Get (0)->GetObject<TelepSrcApp> ();

      telepSrcApp->SetStartTime (Seconds (TELEP_DELAY * rank));
      telepSrcApp->SetStopTime (Seconds (TELEP_DELAY * (rank + 1)));

      TelepDstHelper dstHelper (qphyent, qconn);
      dstHelper.SetAttribute ("Qubit", StringValue (dstOwner + "_Qubit0"));

      ApplicationContainer dstApps = dstHelper.Install (qphyent->GetNode (dstOwner));
      Ptr<TelepDstApp> telepDstApp = dstApps.Get (0)->GetObject<TelepDstApp> ();

      telepDstApp->SetStartTime (Seconds (TELEP_DELAY * rank));
      telepDstApp->SetStopTime (Seconds (TELEP_DELAY * (rank + 1)));
    }

  //
  // Run the simulation.
  //
  Simulator::Stop (Seconds (TELEP_DELAY * (N - 1)));
  auto start = std::chrono::high_resolution_clock::now ();
  Simulator::Run ();
  auto end = std::chrono::high_resolution_clock::now ();
  printf ("Total time cost: %ld s\n",
          std::chrono::duration_cast<std::chrono::seconds> (end - start).count ());
  Simulator::Destroy ();

  return 0;
}
