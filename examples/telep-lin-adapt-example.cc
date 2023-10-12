/*
  To run this example:
  NS_LOG="QuantumNetworkSimulator=info:QuantumPhyEntity=info|logic:TelepLinAdaptApp=logic" ./ns3 run telep-lin-adapt-example
  Note that by adding "TelepLinAdaptApp=logic", the simulator logs the details of the whole process of chained teleportation.
  Also note that to simulate a longer chain, the allocation of addresses and ports needs to be modified.
*/
#include "ns3/csma-module.h" // class CsmaHelper, NetDeviceContainer
#include "ns3/internet-module.h" // class InternetStackHelper, Ipv6AddressHelper, Ipv6InterfaceContainer

#include "ns3/quantum-basis.h" // class APP_DISTILL_NESTED
#include "ns3/quantum-network-simulator.h" // class QuantumNetworkSimulator
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-channel.h" // class QuantumChannel
#include "ns3/distribute-epr-helper.h" // class DistributeEPRSrcHelper, DistributeEPRDstHelper
#include "ns3/quantum-net-stack-helper.h" // class QuantumNetStackHelper
#include "ns3/telep-lin-adapt-helper.h" // class TelepLinAdaptHelper
#include "ns3/telep-lin-adapt-app.h" // class TelepLinAdaptApp

#include <iostream>
#include <cmath>
#include <complex>

NS_LOG_COMPONENT_DEFINE ("TelepLinAdaptExample");

using namespace ns3;

// #define N (8)
/*
Evaluating tensor network of size 202
 in 6.33792 secs
/*
// #define N (16)
/*
Evaluating tensor network of size 426
 in 12.8941 secs
*/
// #define N (32)
/*
Evaluating tensor network of size 874
 in 26.6684 secs
*/
// #define N (64)
/*
Evaluating tensor network of size 1770
 in 54.291 secs
*/
#define N (72)
/*
Evaluating tensor network of size 1994
 in 61.025 secs
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

  // 
  // Setup the first teleporatation from Owner0 to Owner1.
  //
  Ptr<QuantumChannel> qconn0 = CreateObject<QuantumChannel> ("Owner0", "Owner1");
  qconn0->SetDepolarModel (0.95, qphyent);

  Ptr<Qubit> input = CreateObject<Qubit> (std::vector<std::complex<double>>{
      {sqrt (5. / 7.), 0.0},
      {0.0, sqrt (2. / 7.)}},
      "Owner0_Qubit0");
  TelepLinAdaptHelper helper0 (qphyent, qconn0);
  helper0.SetAttribute ("EPR",
                        PairValue<StringValue, StringValue> ({"Owner0_Qubit1", "Owner1_Qubit0"}));
  helper0.SetAttribute ("Input", PointerValue (input));

  ApplicationContainer app0 = helper0.Install (qphyent->GetNode ("Owner0"));
  app0.Start (Seconds (0.));
  app0.Stop (Seconds(ETERNITY));

  //
  // Setup the intermediate N - 2 teleportations from Owner1 to OwnerN-2.
  //
  for (int rank = 1; rank < N - 1; ++rank)
    {
      std::string srcOwner = "Owner" + std::to_string (rank);
      std::string dstOwner = "Owner" + std::to_string (rank + 1);

      Ptr<QuantumChannel> qconn =
          CreateObject<QuantumChannel> (std::pair<std::string, std::string>{srcOwner, dstOwner});
      qconn->SetDepolarModel (0.98, qphyent);

      TelepLinAdaptHelper helper (qphyent, qconn);
      helper.SetAttribute ("EPR", PairValue<StringValue, StringValue> (
                            {srcOwner + "_Qubit1", dstOwner + "_Qubit0"}
                          ));
      helper.SetAttribute ("Input", PointerValue (nullptr));

      ApplicationContainer app = helper.Install (qphyent->GetNode (srcOwner));
      app.Start (Seconds (0.));
      app.Stop (Seconds(ETERNITY));
    }

  //
  // Setup the teleportation application for OwnerN-1.
  //
  Ptr<QuantumChannel> qconnN = nullptr;
  TelepLinAdaptHelper helperN (qphyent, qconnN);
  helperN.SetAttribute ("EPR", PairValue<StringValue, StringValue> ({"", ""}));
  helperN.SetAttribute ("Input", PointerValue (nullptr));

  ApplicationContainer appN = helperN.Install (qphyent->GetNode ("Owner" + std::to_string (N - 1)));
  appN.Start (Seconds (0.));
  appN.Stop (Seconds(ETERNITY));

  //
  // Run the simulation.
  //
  Simulator::Stop (Seconds (CLASSICAL_DELAY * N));
  auto start = std::chrono::high_resolution_clock::now ();
  Simulator::Run ();
  auto end = std::chrono::high_resolution_clock::now ();
  printf ("Total time cost: %ld s\n",
          std::chrono::duration_cast<std::chrono::seconds> (end - start).count ());
  Simulator::Destroy ();

  return 0;
}
