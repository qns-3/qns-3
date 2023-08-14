#ifndef QUANTUM_PHY_ENTITY_H
#define QUANTUM_PHY_ENTITY_H

#include "ns3/quantum-basis.h"
#include "ns3/quantum-network-simulator.h" // class QuantumNetworkSimulator
#include "ns3/quantum-channel.h" // class QuantumChannel

#include <exatn.hpp> // exatn::numerics::TensorNetwork

namespace ns3 {

class Address;

class QuantumNetworkSimulator;
class QuantumOperation;
class QuantumNode;
class QuantumErrorModel;

class Application;
class TelepSrcApp;
class TelepDstApp;
class DistillApp;
class DistributeEPRSrcProtocol;
class DistributeEPRDstProtocol;

class QuantumPhyEntity : public Object
{
  friend class QuantumNode;
  friend class TelepSrcApp;
  friend class TelepDstApp;
  friend class DistillApp;
  friend class DistributeEPRSrcProtocol;
  friend class DistributeEPRDstProtocol;

  friend class QuantumErrorModel;
  friend class DephaseModel;
  friend class TimeModel;
  friend class DepolarModel;

public:
  QuantumPhyEntity (const std::vector<std::string> &owners); // names of the owners

  ~QuantumPhyEntity ();

  QuantumPhyEntity ();
  void DoDispose (void);
  static TypeId GetTypeId (void);


/* circuit */

  /**
   * \brief Generate n qubits.
   * 
   * \internal Access control. Call QuantumNetworkSimulator. Set errors.
   * 
   * \param owner Owner generating the qubits.
   * \param data State vector of the qubits.
   * \param qubits Names of the qubits.
   * \return True if the qubits are generated successfully.
   * 
   * \note The data size must be 2^n.
   * \note The qubits size must be n.
  */
  bool GenerateQubitsPure (const std::string &owner,
                       const std::vector<std::complex<double>> &data,
                       const std::vector<std::string> &qubits 
  );

  /**
   * \brief Generate n qubits.
   * 
   * \internal Access control. Call QuantumNetworkSimulator. Set errors.
   * 
   * \param owner Owner generating the qubits.
   * \param data Density matrix of the qubits.
   * \param qubits Names of the qubits.
   * \return True if the qubits are generated successfully.
   * 
   * \note The data size must be 2^n * 2^n.
   * \note The qubits size must be n.
  */
  bool GenerateQubitsMixed (const std::string &owner,
                       const std::vector<std::complex<double>> &data,
                       const std::vector<std::string> &qubits 
  );
  /**
   * \brief Apply a gate to the qubits.
   * 
   * \internal Access control. Update errors. Call QuantumNetworkSimulator.
   * 
   * \param owner Owner applying the gate.
   * \param gate Name of the gate.
   * \param data Data of the gate.
   * \param qubits Names of the qubits.
   * \return True if the gate is applied successfully.
   * 
   * \note If the gate is not a reserved one (defined in QBasis), the data size must be 2^n * 2^n.
   * \note The qubits size must be n.
  */
  bool ApplyGate (const std::string &owner,
                  const std::string &gate,
                  const std::vector<std::complex<double>> &data,
                  const std::vector<std::string> &qubits
  );

  /**
   * \brief Apply a gate to the qubits.
   * 
   * \internal Call QuantumNetworkSimulator.
   * 
   * \param owner Owner applying the gate.
   * \param gate Name of the gate.
   * \param data Data of the gate.
   * \param qubits Names of the qubits.
   * \return True if the gate is applied successfully.
  */
  bool
  ApplyOperation (const QuantumOperation &quantumOperation,
                  const std::vector<std::string> &qubits
  );

  /**
   * \brief Apply a controlled operation to n qubits (control qubits and target qubits).
   * 
   * Apply a controlled operation to control qubits and target qubits,
   * instead of applying some gate (the original gate) to the target qubits
   * according to the measurement outcomes on the control qubits.
   * 
   * \internal Call QuantumNetworkSimulator. Update errors.
   * 
   * \param owner Owner applying the operation.
   * \param orig_owner Owner of the original gate.
   * \param orig_gate Name of the original gate.
   * \param gate Name of the controlled gate.
   * \param data Data of the controlled gate.
   * \param control_qubits Names of the control qubits.
   * \param target_qubits Names of the target qubits.
   * \return True if the operation is applied successfully.
   * 
   * \note If the gate is not a reserved one (defined in QBasis), the data size must be 2^n * 2^n.
  */
  bool ApplyControledOperation (
      const std::string &orig_owner,
      const std::string &orig_gate,
      const std::string &gate,
      const std::vector<std::complex<double>> &data, const std::vector<std::string> &control_qubits,
      const std::vector<std::string> &target_qubits);

  /**
   * \brief Measure n qubits.
   * 
   * \internal Access control. Update errors. Call QuantumNetworkSimulator.
   * 
   * \param owner Owner measuring the qubits. 
   * \param qubits Names of the qubits to be measured.
   * \return Measurement outcome and the probability distribution.
  */ 
  std::pair<unsigned, std::vector<double>>
  Measure (const std::string &owner, const std::vector<std::string> &qubits);


  /**
   * \brief Peek n qubits.
   * 
   * \internal Access control. Call QuantumNetworkSimulator.
   * 
   * \param owner Owner peeking the qubits
   * \param qubits Names of the qubits to be peeked.
   * \param dm Vector to store the density matrix of the qubits.
   * \return Density matrix of the qubits.
  */
  std::vector<std::complex<double>>
  PeekDM (const std::string &owner,
          const std::vector<std::string> &qubits,
          std::vector<std::complex<double>> &dm
  );

  /**
   * \brief Partial trace n qubits by simply connecting the two ends of their wires
   * 
   * \internal Update errors. Call QuantumNetworkSimulator.
   * 
   * \param qubits Names of the qubits to be traced out.
   * \param dm Vector to store the density matrix of the qubits.
   * \return True if the wires are connected successfully.
  */
  bool
  PartialTrace (const std::vector<std::string> &qubits,
                std::vector<std::complex<double>> &dm
  );

  /**
   * \brief Contract the tensor network to a single tensor.
   * \return The density matrix of the tensor.
  */
  std::vector<std::complex<double>> Contract ();
  
  /**
   * \brief Get a pointer to a quantum node by its owner name.
   * \return A pointer to a quantum node.
  */
  Ptr<QuantumNode> GetNode (const std::string &owner);

  /**
   * \brief Set the error model of a qubit.
   * \param pmodel Pointer to the error model.
   * \param qubit Name of the qubit.
  */
  void SetErrorModel (Ptr<QuantumErrorModel> pmodel, const std::string &qubit);

  /**
   * \brief Set a dephasing model to a gate of some owner.
   * \param owner Owner to apply the gate.
   * \param gate Name of the gate.
   * \param rate Dephasing rate.
  */
  void SetDephaseModel (const std::string &owner, // node applying the gate
                        const std::string &gate,
                        double rate // dephasing rate of the gate of the node
  );

  /**
   * \brief Apply a dephasing model for a n-qubit gate.
   * \param owner Owner applying the gate.
   * \param gate Name of the gate.
   * \param qubits Names of the qubits to which the gate is applied.
   * \param moment Time of the appliance.
  */
  void ApplyErrorModel (const std::string &owner, const std::string &gate,
                        const std::vector<std::string> &qubits,
                        const Time &moment = Simulator::Now ());

  /**
   * \brief Set a depolarizing model to a connection.
   * \param conn The two owners distributing some EPR pair.
   * \param fidel Fidelity of the distributed EPR pair.
  */ 
  void SetDepolarModel (
      std::pair<std::string, std::string> conn,
      double fidel
  );
  /**
   * \brief Apply a depolarizing model for some EPR distribution.
   * \param conn The two owners distributing the EPR pair.
   * \param epr The two qubits of the EPR pair.
  */
  void ApplyErrorModel (std::pair<std::string, std::string> conn,
                        const std::pair<std::string, std::string> &epr);

  /**
   * \brief Set a time model to a qubit.
   * \param owner Owner generating the qubit.
   * \param rate Dephasing rate of the time model.
  */
  void SetTimeModel (const std::string &owner,
                     double rate
  );
  /**
   * \brief Apply a time model for n qubits.
   * \param qubits Names of the qubits.
   * \param moment Time of the appliance.
  */
  void ApplyErrorModel (const std::vector<std::string> &qubits,
                        const Time &moment = Simulator::Now ());


/* util */

  /**
   * \brief Check if the qubits are valid (not traced out).
   * 
   * \internal Call QuantumNetworkSimulator.
   * 
   * \param qubits Names of the qubits to be checked.
   * \return True if the qubits are all valid.
  */
  bool CheckValid (const std::vector<std::string> &qubits) const;

  /**
   * \brief Check if the qubits are ownned by the owner.
   * \param qubits Names of the qubits to be checked.
   * \return True if the qubits are all in the owner's quantum memory.
  */
  bool CheckOwned (const std::string &owner, const std::vector<std::string> &qubits) const;



/* network */

  /**
   * \brief Generate a EPR pair.
   * 
   * \internal Call GenerateQubit ().
   * 
   * \param qconn The source node generates the EPR pair to distribute it to the destination.
   * \param epr The two qubits of the EPR pair.
  */
  void GenerateEPR (Ptr<QuantumChannel> qconn, const std::pair<std::string, std::string> &epr);

  double CalculateFidelity (const std::pair<std::string, std::string> &epr, double &fidel);

/* util */

  /**
   * \brief Record the pointers to quantum applications of a connection.
   * \param qconn The connection.
   * \param protocol The protocol name of the connection.
   * \param apps The pointers to the quantum applications.
  */
  void AddConn2Apps (Ptr<QuantumChannel> qconn, const std::string &protocol,
                     const std::pair<Ptr<Application>, Ptr<Application>> &apps);

  /**
   * \brief Get the pointers to quantum applications of a connection.
   * \param qconn The connection.
   * \param protocol The protocol name of the connection.
   * \return The pointers to the quantum applications.  
  */ 
  std::pair<Ptr<Application>, Ptr<Application>> GetConn2Apps (Ptr<QuantumChannel> qconn,
                                                              const std::string &protocol);

  /**
   * \brief Set the network layer address of an owner.
   * \param owner Name of the owner.
   * \param address Network layer address of the owner.
  */
  void SetOwnerAddress (const std::string &owner, const Address &address);

  /**
   * \brief Set the rank of an owner.
   * \param owner Name of the owner.
   * \param rank Rank of the owner in some certain subnet.
  */
  void SetOwnerRank (const std::string &owner, const unsigned &rank);



/* debug */
  
  void PrintConn2Apps () const;

private:

/* circuit */

  /** Instance of a quantum network simulator. */
  QuantumNetworkSimulator m_qnetsim;
  


/* network */

  /** Map from owner name to the pointer to its quantum node. */
  std::map<std::string, Ptr<QuantumNode>> m_owner2pnode;

  /** Map from quantum channel and a quantum application name
   * to the pointers to the applications. */
  std::map<QuantumChannel, std::map<std::string, std::pair<Ptr<Application>, Ptr<Application>>>>
      m_conn2apps;

  /** Map from qubit name to the last time it was operated. */
  std::map<std::string, Time> m_qubit2time; 


  /** Map from pointer to a quantum node (i.e. a owner)
   * to the pointer to the time relevant error model of the qubits generated by the owner. */
  std::map<Ptr<QuantumNode>, Ptr<QuantumErrorModel>> m_node2model;

  /** Map from qubit name to the pointer to its time relevant error model. */
  std::map<std::string, Ptr<QuantumErrorModel>> m_qubit2model;

  /** Map from pointer to a quantum node (i.e. a owner) and some gate name
   * to the pointer to the dephasing error model of the gate. */
  std::map<Ptr<QuantumNode>, std::map<std::string, Ptr<QuantumErrorModel>>>
      m_gate2model;

  /** Map from a quantum connection to its depolarizing error model. */
  std::map<std::pair<std::string, std::string>, Ptr<QuantumErrorModel>>
      m_conn2model;
};

} // namespace ns3

#endif /* QUANTUM_PHY_ENTITY_H */