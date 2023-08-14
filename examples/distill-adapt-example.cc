#include "ns3/csma-module.h" // class CsmaHelper, NetDeviceContainer
#include "ns3/internet-module.h" // class InternetStackHelper, Ipv6AddressHelper, Ipv6InterfaceContainer

#include "ns3/quantum-basis.h"
#include "ns3/quantum-network-simulator.h" // class QuantumNetworkSimulator
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-channel.h" // class QuantumChannel
#include "ns3/quantum-net-stack-helper.h" // class QuantumNetStackHelper
#include "ns3/distribute-epr-helper.h" // class DistributeEPRhelper, DistributeEPRDstHelper
#include "ns3/distill-nested-adapt-app.h" // class DistillNestedAdaptApp
#include "ns3/distill-nested-adapt-helper.h" // class DistillNestedAdaptHelper

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DistillAdaptExample");

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
  nodes.Add (alice);

  Ptr<QuantumNode> bob = qphyent->GetNode ("Bob");
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
  qconn->SetDepolarModel (0.95, qphyent);

  DistillNestedAdaptHelper helper (qphyent, false, qconn);
  Ptr<QuantumMemory> src_qubits =
      CreateObject<QuantumMemory> (std::vector<std::string>{"A0", "A1"});
  Ptr<QuantumMemory> dst_qubits =
      CreateObject<QuantumMemory> (std::vector<std::string>{"B0", "B1"});
  helper.SetAttribute ("SrcQubits", PointerValue (src_qubits));
  helper.SetAttribute ("DstQubits", PointerValue (dst_qubits));
  helper.SetAttribute ("FlagQubit", StringValue ("Flag"));

  ApplicationContainer apps = helper.Install (alice);
  Ptr<DistillNestedAdaptApp> distillSrcApp = apps.Get (0)->GetObject<DistillNestedAdaptApp> ();
  apps.Start (Seconds (0.));
  apps.Stop (Seconds (10.));


  //
  // Run the simulation.
  //
  Simulator::Stop (Seconds (10.));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

/* OUTPUT
At time 0 Alice generates qubit(s) named
Flag

Adding qubit Flag to Alice with local idx 0
At time 0 Alice generates qubit(s) named
A0
B0

Adding qubit A0 to Alice with local idx 1
Adding qubit B0 to Alice with local idx 2
Distributing qubit named B0 of Alice to Bob
Removing qubit B0 from Alice
At time +0s Node # 0 sent "A0.B0" to 2001:1::200:ff:fe00:2
At time +0.023784s Node # 1's DistributeEPRDstProtocol received "A0.B0" from 2001:1::200:ff:fe00:1
Adding qubit B0 to Bob with local idx 0
At time 0.03 Alice generates qubit(s) named
A1
B1

Adding qubit A1 to Alice with local idx 2
Adding qubit B1 to Alice with local idx 3
Distributing qubit named B1 of Alice to Bob
Removing qubit B1 from Alice
At time +0.03s Node # 0 sent "A1.B1" to 2001:1::200:ff:fe00:2
At time +0.032576s Node # 1's DistributeEPRDstProtocol received "A1.B1" from 2001:1::200:ff:fe00:1
Adding qubit B1 to Bob with local idx 1
At time 0.06 Alice applies gate QNS_GATE_CNOT to qubits(s)
A1
A0

At time 0.06 Bob applies gate QNS_GATE_CNOT to qubits(s)
B1
B0

At time 0.06 God applies gate QNS_GATE_CNOT to qubits(s)
B1
A1

At time 0.06 tracing out qubit(s) named:
A1

At time 0.06 God applies gate QNS_GATE_PX to qubits(s)
B1

At time 0.06 Alice generates qubit(s) named
QNS_ANCILLA0

Adding qubit QNS_ANCILLA0 to Alice with local idx 3
At time 0.06 God applies gate QNS_GATE_TOFF to qubits(s)
QNS_ANCILLA0
B1
Flag

At time 0.06 tracing out qubit(s) named:
B1

At time 0.06 Alice applies gate QNS_GATE_SWAP to qubits(s)
QNS_ANCILLA0
Flag

At time 0.06 tracing out qubit(s) named:
QNS_ANCILLA0

At time 0.06 God peeks density matrix on qubit(s)
A0
B0

Density Matrix: 
[
<0.483>   0      0    0.373 
   0   <0.017>   0      0   
   0      0   <0.017>   0   
 0.373    0      0   <0.483>
]
At time 0.06 Alice measures the qubit named Flag
Picking outcome with probability distribution: [0.0644444, 0.935556]
At time 0.06 Alice applies gate QNS_EXATN20 to qubits(s)
Flag

At time +0.06s Alice finds out wins
Probability of succeeding is 0.935556
At time 0.06 God peeks density matrix on qubit(s)
A0
B0

Density Matrix: 
[
<0.499>   0      0    0.399 
   0   <  0  >   0      0   
   0      0   <  0  >   0   
 0.399    0      0   <0.499>
]
Calculating fidelity for epr pair (A0, B0)
:
Density Matrix: 
[
<0.499>   0      0    0.399 
   0   <  0  >   0      0   
   0      0   <  0  >   0   
 0.399    0      0   <0.499>
]
=> Fidelity is 0.8985
#MSG(TAL-SH::CP-TAL): Statistics on CPU:
 Number of Flops processed    :      0.27036800000000D+06
 Average GEMM GFlop/s rate    :      0.15210864297905D-01
 Number of Bytes permuted     :      0.27840000000000D+06
 Average permute GB/s rate    :      0.56768534005495D-04
 Average contract GFlop/s rate:      0.38920886886588D-04
#END_MSG
*/