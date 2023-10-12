Quantum Module Documentation
----------------------------

.. include:: replace.txt
.. highlight:: cpp

.. heading hierarchy:
   ------------- Chapter
   ************* Section (#.#)
   ============= Subsection (#.#.#)
   ############# Paragraph (no number)


Model Description
*****************

The source code for the new module lives in the directory ``contrib/quantum-basis``.

This is a module for quantum network simulations which
provides a series of typical abstractions and functionalities.
The top (i.e. application) layer is open for users
to design and track all kinds of customized quantum network protocols.


Design
======

The design goal was to make the ns-3 Quantum API simple and user friendly
in order to better facilitate user research and experimentation.

The module follows the layered architecture of the classical network stack,
and the discrete-event simulation framework of ns-3,

Some components inherit from their classical counterparts in |ns3|,
such as ``Node`` and ``Application``.
Some are only named after their classical counterparts,
such as ``QuantumErrorModel`` and ``QuantumPhyEntity``.
Others are newly designed, such as ``QuantumOperation`` and ``QuantumMemory``.

For more details, please refer to the fifth section of our paper.

Scope and Limitations
=====================

* In its current state, the quantum network stack is only equipped with L2 EPR distribution and distillation protocols, as well as L5 applications. The L3 routing protocols and L4 transport protocols are not simulated yet.

* As for now, the module is only implemented in the C++ language. Python bindings are not available yet.

References
==========

.. TODO:
.. Add academic citations here, such as if you published a paper on this
.. model, or if readers should read a particular specification or other work.

Usage
*****

.. This section is principally concerned with the usage of your model, using
.. the public API.  Focus first on most common usage patterns, then go
.. into more advanced topics.

.. Building New Module
.. ===================

.. Include this subsection only if there are special build instructions or
.. platform limitations.

Helpers
=======

To have a node run quantum applications, the easiest way would be
to use the QuantumNetStackHelper for L2 functionalities,
along with the QuantumApplicationHelper for some specific L5 protocols.

For instance:

.. sourcecode:: cpp
   
   QuantumNetStackHelper stack;
   stack.Install (nodes);

   DistillHelper dstHelper (qphyent, true, qconn);
   dstHelper.SetAttribute ("Qubits", PairValue<StringValue, StringValue> ({"Bob0", "Bob1"}));
   ApplicationContainer dstApp = dstHelper.Install (bob);


The example scripts inside ``contrib/quantum/examples/`` demonstrate the use of Quantum based nodes
in different scenarios. The helper source can be found inside ``contrib/quantum/helper/``


Attributes
==========

All quantum applications and protocols are configured through attributes.
Basically, the attributes can be divided into three categories, for:
* identification, such as ``Checker`` and ``PNode``,
* communication, such as the ``QChannel`` and ``EPR``,
* operation, such as ``QPhyEntity`` and ``Qubit``.

Output
======

.. What kind of data does the model generate?  What are the key trace
.. sources?   What kind of logging output can be enabled?

The quantum network stack generates a series of trace sources 
to track the quantum network protocols.

At the logging level ``NS_LOG_INFO``, users can track the moment and description of
the local operation events as well as the communication events.
Users can also peek into the quantum memory to check the density matrix of certain qubits
by scheduling a PeekDM() event.

At the logging level ``NS_LOG_LOGIC``, users can track more detailed information,
such as the tensor network size, the error appliance events, 
and the addresses and ports in a classical connection.

.. Advanced Usage
.. ==============

.. Go into further details (such as using the API outside of the helpers)
.. in additional sections, as needed.

Examples
========

The following examples have been written, which can be found in ``examples/``:

* distill-app-example.cc: Implementation of the quantum distillation protocol used for increasing the fidelity of EPR pairs, an integral process for any future quantum network infrastructure.
  
* distill-app-repeat-example.cc: Implementation of repeated usage of the quantum distillation protocol in order to simulate a realistic process for generating a high fidelity EPR pair.
  
* distill-adapt-example.cc: Implementation of the quantum distillation protocol with control flow adaption in order to optimise the simulation process.
  
* distill-nested-adapt-example.cc: Implementation of nested quantum distillations in order to simulate a realistic process of generating a higher fidelity EPR pair.
  
* telep-app-example.cc: Implementation of the quantum teleportation protocol used for transferring quantum information between nodes, an integral process for any future quantum network infrastructure.
  
* msg-flow-example.cc: Demonstration of several quantum teleportations among multiple nodes.
  
* telep-app-repeat-example.cc: Demonstration of repeated quantum teleportations between two nodes.
  
* telep-lin-example.cc: Demonstration of a chain of consecutive quantum teleportations, each time transferring the original qubit to a new node.
  
* telep-adapt-example.cc: Implementation of the quantum teleportation protocol with control flow adaption in order to optimise the simulation process.
  
* telep-lin-adapt-example.cc: Demonstration of a chain of consecutive quantum teleportations between two nodes, with control flow adaption.

* ent-swap-example.cc: Implementation of the chained entanglement swapping protocol used for establishing long-distance entanglement between two nodes.

* ent-swap-adapt-example.cc: Implementation of the chained entanglement swapping protocol with control flow adaption in order to optimise the simulation process.

* ent-swap-adapt-local-example.cc: Implementation of the chained entanglement swapping protocol with another possible control flow adaption in order to further optimise the simulation process, getting around long distance operations.

* ent-swap-lin-adapt-local-example.cc: Implementation of the chained entanglement swapping protocol by using the chained teleportation protocol with control flow adaption.

Troubleshooting
===============

Please check the convention described in the ``\note`` line
in the header file when calling a function.

.. Validation
.. **********

.. Describe how the model has been tested/validated.  What tests run in the
.. test suite?  How much API and code is covered by the tests?  Again,
.. references to outside published work may help here.
