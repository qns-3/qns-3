# Qns-3: An Efficient Simulator for Noisy Quantum Networks with Control Flow Adaption

Qns-3 is a quantum simulator designed based on ns-3 with suitable APIs respecting ns-3’s convention, and adopts the control flow adaptation method as a fundamental technique. 

Qns-3 is developed by the research group leaded by Prof. Zhenfeng Ji, from Department of Computer Science and Technology, Tsinghua University. The author list is:
- Huiping Lin (Tsinghua University)
- Ruixuan Deng (Tsinghua University)
- Chris Z. Yao (Tsinghua University)
- Zhengfeng Ji (Tsinghua University)
- Mingsheng Ying (Tsinghua University & Institute of Software, Chinese Academy of Sciences)

Qns-3 utilizes a discrete-event simulation method to model quantum networks. It leverages a tensor network structure to efficiently represent and manipulate all quantum information involved within the network.
This repository is a module of [ns-3](https://www.nsnam.org) (a very famous discrete-event network simulator for internet systems), and uses [ExaTN](https://github.com/ORNL-QCI/exatn) (a software library for expressing, manipulating and processing arbitrary tensor network) as the basic engine. 

## Research Problem

In quantum networks, many protocols can be modeled in the LOCC (local operations and classical communication) scheme.
In these protocols, classical information received by different parties influences future quantum operations, leading to the required construction of a complex backend to track quantum states if done straightforwardly.
For example, the tensor network method is a powerful technique for classical simulation of quantum information, but as the protocols become more intricate and the number of qubits increases, the contraction complexity of the tensor networks often grows exponentially.
The need to simulate the noise behavior in quantum networks marks as a further challenge for tensor network methods as noises add many new contractions in the tensor network representation.
Additionally, quantum measurements, which bring randomness in the simulation of quantum network protocols, are another source of complications.
To obtain an average simulation result when verifying a protocol, one may need to employ a Monte Carlo method and run the simulation a large number of times.

## Main Technique
The CFA method implemented in qns-3 performs a control flow analysis to determine the necessary information required for future rounds of the LOCC protocols.
By compressing and adapting the classical control flow in a way that aligns with the capabilities of tensor networks, we can efficiently handle the complexities of many quantum network protocols.
This careful adaptation ensures that the control flow information integrates seamlessly into the tensor network simulation framework.
Instead of faithfully simulating all quantum measurements and reveals one possible branch, this method provides a mechanism that is able to integrate all possible results in a single round. 

For a more detailed theory of our CFA method, please refer to our paper published in: .

## Network Model

The quantum network model incorporated in the qns-3 simulator involves two general classes of components: quantum nodes and quantum channels. Quantum nodes are entities that manage all equipments capable of processing qubits locally. Quantum channels, such as optical fibers, refer to all communication channels that can transmit quantum information.

### Quantum Node
Quantum nodes are similar to the end hosts in classical networks, which are
connected to other quantum nodes by quantum channels.
Each quantum node is equipped with network hardwares and software stacks. For the classical network stack, we reuse the ns-3 internet stacks and can support any classical network topology. The model of quantum network protocol stack is still under exploration today. In qns-3's current implementation and futural development, we follow a quantum analogue of the classical OSI 5-layer model.

### Quantum Channel
A quantum channel connects two quantum nodes and supports the transmission of
qubits between the two nodes.
The physical material of quantum channels may be polarization-maintaining
optical fibers.
Quantum channels also have the ability to transmit classical information by
default.
Quantum channels exhibit inherent noises, potentially leading to qubit errors
due to distinct physical attributes associated with each channel.
Hence, specific noise parameters are attributed to each quantum channel.