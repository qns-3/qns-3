#include "ns3/csma-module.h" // class CsmaHelper, NetDeviceContainer
#include "ns3/internet-module.h" // class InternetStackHelper, Ipv6AddressHelper, Ipv6InterfaceContainer

#include "ns3/quantum-basis.h"
#include "ns3/quantum-network-simulator.h" // class QuantumNetworkSimulator
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-node.h" // class QuantumNode
#include "ns3/quantum-channel.h" // class QuantumChannel
#include "ns3/distribute-epr-helper.h" // class DistributeEPRhelper, DistributeEPRDstHelper
#include "ns3/quantum-net-stack-helper.h" // class QuantumNetStackHelper

#include "ns3/distill-nested-adapt-app.h" // class DistillNestedAdaptApp
#include "ns3/distill-nested-adapt-helper.h" // class DistillNestedAdaptHelper

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DistillNestedAdaptExample");

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
  alice->SetTimeModel (2e1); // it's huge, because CC is slow and would cause too severe a dephase loss
  nodes.Add (alice);

  Ptr<QuantumNode> bob = qphyent->GetNode ("Bob");
  bob->SetTimeModel (2e1);
  nodes.Add (bob);

  //
  // Create a classical connection.
  //
  CsmaHelper csmaHelper;
  csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("1000kbps")));
  csmaHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (1)));
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
  Ptr<QuantumMemory> src_qubits = CreateObject<QuantumMemory> (std::vector<std::string>{
      // "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8", "A9", "A10", "A11", "A12", "A13", "A14", "A15"
      "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7"
      // "A0", "A1"
  });
  Ptr<QuantumMemory> dst_qubits = CreateObject<QuantumMemory> (std::vector<std::string>{
      // "B0", "B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8", "B9", "B10", "B11", "B12", "B13", "B14", "B15"
      "B0", "B1", "B2", "B3", "B4", "B5", "B6", "B7"
      // "B0", "B1"
  });
  helper.SetAttribute ("SrcQubits", PointerValue (src_qubits));
  helper.SetAttribute ("DstQubits", PointerValue (dst_qubits));
  helper.SetAttribute ("FlagQubit", StringValue ("Flag"));

  ApplicationContainer apps = helper.Install (alice);
  Ptr<DistillNestedAdaptApp> distillSrcApp = apps.Get (0)->GetObject<DistillNestedAdaptApp> ();
  apps.Start (Seconds (0.));
  apps.Stop (Seconds (10.));

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
At time +0.017019s Node # 1's DistributeEPRDstProtocol received "A0.B0" from 2001:1::200:ff:fe00:1
Adding qubit B0 to Bob with local idx 0
At time 0.03 Alice generates qubit(s) named
A1
B1

Adding qubit A1 to Alice with local idx 2
Adding qubit B1 to Alice with local idx 3
Distributing qubit named B1 of Alice to Bob
Removing qubit B1 from Alice
At time +0.03s Node # 0 sent "A1.B1" to 2001:1::200:ff:fe00:2
At time +0.031576s Node # 1's DistributeEPRDstProtocol received "A1.B1" from 2001:1::200:ff:fe00:1
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

At time 0.06 Alice generates qubit(s) named
A2
B2

Adding qubit A2 to Alice with local idx 4
Adding qubit B2 to Alice with local idx 5
Distributing qubit named B2 of Alice to Bob
Removing qubit B2 from Alice
At time +0.06s Node # 0 sent "A2.B2" to 2001:1::200:ff:fe00:2
At time +0.061576s Node # 1's DistributeEPRDstProtocol received "A2.B2" from 2001:1::200:ff:fe00:1
Adding qubit B2 to Bob with local idx 2
At time 0.09 Alice generates qubit(s) named
A3
B3

Adding qubit A3 to Alice with local idx 5
Adding qubit B3 to Alice with local idx 6
Distributing qubit named B3 of Alice to Bob
Removing qubit B3 from Alice
At time +0.09s Node # 0 sent "A3.B3" to 2001:1::200:ff:fe00:2
At time +0.091576s Node # 1's DistributeEPRDstProtocol received "A3.B3" from 2001:1::200:ff:fe00:1
Adding qubit B3 to Bob with local idx 3
At time 0.12 Alice applies gate QNS_GATE_CNOT to qubits(s)
A3
A2

At time 0.12 Bob applies gate QNS_GATE_CNOT to qubits(s)
B3
B2

At time 0.12 God applies gate QNS_GATE_CNOT to qubits(s)
B3
A3

At time 0.12 tracing out qubit(s) named:
A3

At time 0.12 God applies gate QNS_GATE_PX to qubits(s)
B3

At time 0.12 Alice generates qubit(s) named
QNS_ANCILLA1

Adding qubit QNS_ANCILLA1 to Alice with local idx 6
At time 0.12 God applies gate QNS_GATE_TOFF to qubits(s)
QNS_ANCILLA1
B3
Flag

At time 0.12 tracing out qubit(s) named:
B3

At time 0.12 Alice applies gate QNS_GATE_SWAP to qubits(s)
QNS_ANCILLA1
Flag

At time 0.12 tracing out qubit(s) named:
QNS_ANCILLA1

At time 0.12 Alice applies gate QNS_GATE_CNOT to qubits(s)
A2
A0

At time 0.12 Bob applies gate QNS_GATE_CNOT to qubits(s)
B2
B0

At time 0.12 God applies gate QNS_GATE_CNOT to qubits(s)
B2
A2

At time 0.12 tracing out qubit(s) named:
A2

At time 0.12 God applies gate QNS_GATE_PX to qubits(s)
B2

At time 0.12 Alice generates qubit(s) named
QNS_ANCILLA2

Adding qubit QNS_ANCILLA2 to Alice with local idx 7
At time 0.12 God applies gate QNS_GATE_TOFF to qubits(s)
QNS_ANCILLA2
B2
Flag

At time 0.12 tracing out qubit(s) named:
B2

At time 0.12 Alice applies gate QNS_GATE_SWAP to qubits(s)
QNS_ANCILLA2
Flag

At time 0.12 tracing out qubit(s) named:
QNS_ANCILLA2

At time 0.12 Alice generates qubit(s) named
A4
B4

Adding qubit A4 to Alice with local idx 8
Adding qubit B4 to Alice with local idx 9
Distributing qubit named B4 of Alice to Bob
Removing qubit B4 from Alice
At time +0.12s Node # 0 sent "A4.B4" to 2001:1::200:ff:fe00:2
At time +0.121576s Node # 1's DistributeEPRDstProtocol received "A4.B4" from 2001:1::200:ff:fe00:1
Adding qubit B4 to Bob with local idx 4
At time 0.15 Alice generates qubit(s) named
A5
B5

Adding qubit A5 to Alice with local idx 9
Adding qubit B5 to Alice with local idx 10
Distributing qubit named B5 of Alice to Bob
Removing qubit B5 from Alice
At time +0.15s Node # 0 sent "A5.B5" to 2001:1::200:ff:fe00:2
At time +0.151576s Node # 1's DistributeEPRDstProtocol received "A5.B5" from 2001:1::200:ff:fe00:1
Adding qubit B5 to Bob with local idx 5
At time 0.18 Alice applies gate QNS_GATE_CNOT to qubits(s)
A5
A4

At time 0.18 Bob applies gate QNS_GATE_CNOT to qubits(s)
B5
B4

At time 0.18 God applies gate QNS_GATE_CNOT to qubits(s)
B5
A5

At time 0.18 tracing out qubit(s) named:
A5

At time 0.18 God applies gate QNS_GATE_PX to qubits(s)
B5

At time 0.18 Alice generates qubit(s) named
QNS_ANCILLA3

Adding qubit QNS_ANCILLA3 to Alice with local idx 10
At time 0.18 God applies gate QNS_GATE_TOFF to qubits(s)
QNS_ANCILLA3
B5
Flag

At time 0.18 tracing out qubit(s) named:
B5

At time 0.18 Alice applies gate QNS_GATE_SWAP to qubits(s)
QNS_ANCILLA3
Flag

At time 0.18 tracing out qubit(s) named:
QNS_ANCILLA3

At time 0.18 Alice generates qubit(s) named
A6
B6

Adding qubit A6 to Alice with local idx 11
Adding qubit B6 to Alice with local idx 12
Distributing qubit named B6 of Alice to Bob
Removing qubit B6 from Alice
At time +0.18s Node # 0 sent "A6.B6" to 2001:1::200:ff:fe00:2
At time +0.181576s Node # 1's DistributeEPRDstProtocol received "A6.B6" from 2001:1::200:ff:fe00:1
Adding qubit B6 to Bob with local idx 6
At time 0.21 Alice generates qubit(s) named
A7
B7

Adding qubit A7 to Alice with local idx 12
Adding qubit B7 to Alice with local idx 13
Distributing qubit named B7 of Alice to Bob
Removing qubit B7 from Alice
At time +0.21s Node # 0 sent "A7.B7" to 2001:1::200:ff:fe00:2
At time +0.211576s Node # 1's DistributeEPRDstProtocol received "A7.B7" from 2001:1::200:ff:fe00:1
Adding qubit B7 to Bob with local idx 7
At time 0.24 Alice applies gate QNS_GATE_CNOT to qubits(s)
A7
A6

At time 0.24 Bob applies gate QNS_GATE_CNOT to qubits(s)
B7
B6

At time 0.24 God applies gate QNS_GATE_CNOT to qubits(s)
B7
A7

At time 0.24 tracing out qubit(s) named:
A7

At time 0.24 God applies gate QNS_GATE_PX to qubits(s)
B7

At time 0.24 Alice generates qubit(s) named
QNS_ANCILLA4

Adding qubit QNS_ANCILLA4 to Alice with local idx 13
At time 0.24 God applies gate QNS_GATE_TOFF to qubits(s)
QNS_ANCILLA4
B7
Flag

At time 0.24 tracing out qubit(s) named:
B7

At time 0.24 Alice applies gate QNS_GATE_SWAP to qubits(s)
QNS_ANCILLA4
Flag

At time 0.24 tracing out qubit(s) named:
QNS_ANCILLA4

At time 0.24 Alice applies gate QNS_GATE_CNOT to qubits(s)
A6
A4

At time 0.24 Bob applies gate QNS_GATE_CNOT to qubits(s)
B6
B4

At time 0.24 God applies gate QNS_GATE_CNOT to qubits(s)
B6
A6

At time 0.24 tracing out qubit(s) named:
A6

At time 0.24 God applies gate QNS_GATE_PX to qubits(s)
B6

At time 0.24 Alice generates qubit(s) named
QNS_ANCILLA5

Adding qubit QNS_ANCILLA5 to Alice with local idx 14
At time 0.24 God applies gate QNS_GATE_TOFF to qubits(s)
QNS_ANCILLA5
B6
Flag

At time 0.24 tracing out qubit(s) named:
B6

At time 0.24 Alice applies gate QNS_GATE_SWAP to qubits(s)
QNS_ANCILLA5
Flag

At time 0.24 tracing out qubit(s) named:
QNS_ANCILLA5

At time 0.24 Alice applies gate QNS_GATE_CNOT to qubits(s)
A4
A0

At time 0.24 Bob applies gate QNS_GATE_CNOT to qubits(s)
B4
B0

At time 0.24 God applies gate QNS_GATE_CNOT to qubits(s)
B4
A4

At time 0.24 tracing out qubit(s) named:
A4

At time 0.24 God applies gate QNS_GATE_PX to qubits(s)
B4

At time 0.24 Alice generates qubit(s) named
QNS_ANCILLA6

Adding qubit QNS_ANCILLA6 to Alice with local idx 15
At time 0.24 God applies gate QNS_GATE_TOFF to qubits(s)
QNS_ANCILLA6
B4
Flag

At time 0.24 tracing out qubit(s) named:
B4

At time 0.24 Alice applies gate QNS_GATE_SWAP to qubits(s)
QNS_ANCILLA6
Flag

At time 0.24 tracing out qubit(s) named:
QNS_ANCILLA6

At time 0.24 God peeks density matrix on qubit(s)
A0
B0

Density Matrix: 
[
<0.483>   0      0    0.271 
   0   <0.017>   0      0   
   0      0   <0.017>   0   
 0.271    0      0   <0.483>
]
At time 0.24 Alice measures the qubit named Flag
Picking outcome with probability distribution: [0.237545, 0.762455]
At time 0.24 Alice applies gate QNS_EXATN95 to qubits(s)
Flag

At time +0.24s Alice finds out wins
Probability of succeeding is 0.762455
At time 0.24 God peeks density matrix on qubit(s)
A0
B0

Density Matrix: 
[
<0.500>   0      0    0.355 
   0   <  0  >   0      0   
   0      0   <  0  >   0   
 0.355    0      0   <0.500>
]
Calculating fidelity for epr pair (A0, B0)
:
Density Matrix: 
[
<0.500>   0      0    0.355 
   0   <  0  >   0      0   
   0      0   <  0  >   0   
 0.355    0      0   <0.500>
]
=> Fidelity is 0.855129
#MSG(TAL-SH::CP-TAL): Statistics on CPU:
 Number of Flops processed    :      0.23047072000000D+08
 Average GEMM GFlop/s rate    :      0.88609707042114D-01
 Number of Bytes permuted     :      0.12334464000000D+08
 Average permute GB/s rate    :      0.30369895818315D-02
 Average contract GFlop/s rate:      0.40863759663898D-02
#END_MSG
*/