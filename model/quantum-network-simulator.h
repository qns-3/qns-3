#ifndef QUANTUM_NETWORK_SIMULATOR_H
#define QUANTUM_NETWORK_SIMULATOR_H

#include "ns3/object.h"

#include "ns3/quantum-basis.h"

namespace ns3 {

class QuantumOperation;

class QuantumNetworkSimulator : public Object
{

  friend class QuantumPhyEntity;

private:
  
/* circuit */
  
  /** Tensor network for the density matrix. */
  exatn::numerics::TensorNetwork m_dm;
  
  /** Next tensor id in the tensor network */
  unsigned m_dm_id;

  /** All generated qubits. */
  std::vector<std::string> m_qubits_all;

  /** Valid qubits that are not traced out. */
  std::vector<std::string> m_qubits_vld;

  /** Map from qubit name to its tensor id in the "ket" half
   * of the tensor network, and the leg id in the tensor. */
  std::map<std::string, std::pair<unsigned, unsigned>> m_qubit2tensor;

  /** Map from qubit name to its tensor id in the "bra" half
   * of the tensor network, and the leg id in the tensor. */
  std::map<std::string, std::pair<unsigned, unsigned>> m_qubit2tensor_dag;


/* util */
  
  /** Count of ExaTN tensor names allocated automatically. */
  unsigned m_exatn_name_count;

  /** All created ExaTN tensors. */
  std::vector<std::string> m_exatn_tensors = {};

public:
  QuantumNetworkSimulator (const std::vector<std::string> &owners);

  QuantumNetworkSimulator (const QuantumNetworkSimulator &qnetsim);

  ~QuantumNetworkSimulator ();

  QuantumNetworkSimulator ();
  static TypeId GetTypeId (void);

/* tensor */

  /**
   * \brief Create and initialize an ExaTN tensor for n qubits.
   * \param name Name of the tensor.
   * \param data State vector of the qubits.
   * 
   * \note The data size must be 2^n.
   * 
  */
  void PrepareQubitsPure (const std::string &name, std::vector<std::complex<double>> data);

  /**
   * \brief Create and initialize an ExaTN tensor for n qubits.
   * \param name Name of the tensor.
   * \param data Density matrix of the qubits.
   * 
   * \note The data size must be 2^n * 2^n.
   * 
  */
  void PrepareQubitsMixed (const std::string &name, std::vector<std::complex<double>> data);

  /**
   * \brief Create and initialize an ExaTN tensor for a n-qubit gate.
   * \param name Name of the tensor.
   * \param data Data of the gate.
   * 
   * \note The data size must be 2^n * 2^n.
  */
  void PrepareGate (const std::string &name,
                    const std::vector<std::complex<double>> &data);

  /**
   * \brief Create and initialize an ExaTN tensor for a n-qubit operation.
   * \param name Name of the tensor.
   * \param data Data of the operation.
   * 
   * \note The data size must be a multiple of 2^n * 2^n.
  */
  void PrepareOperation (const std::string &name,
                         const std::vector<std::vector<std::complex<double>>> &data);

  /**
   * \brief Create and initialize an ExaTN tensor.
   * \param name Name of the tensor.
   * \param extents Extents of the tensor.
   * \param data Data of the tensor.
  */
  void PrepareTensor (const std::string &name, const std::vector<unsigned> &extents,
                      const std::vector<std::complex<double>> &data);



/* circuit */

  /**
   * \brief Generate n qubits.
   * \param owner Owner generating the qubits.
   * \param data State vector of the qubits.
   * \param qubits Unique names of the qubits to generate.
   * \return True if the qubits are generated successfully.
   * 
   * \note The data size must be 2^n.
   * \note The qubits size must be n.
   * 
  */
  bool GenerateQubitsPure (const std::string &owner, const std::vector<std::complex<double>> &data,
                       const std::vector<std::string> &qubits
  );

  /**
   * \brief Generate n qubits.
   * \param owner Owner generating the qubits.
   * \param data Density matrix of the qubits.
   * \param qubits Unique names of the qubits to generate.
   * \return True if the qubits are generated successfully.
   * 
   * \note The qubits size must be n.
   * 
  */
  bool GenerateQubitsMixed (const std::string &owner, const std::vector<std::complex<double>> &data,
                       const std::vector<std::string> &qubits
  );

  /**
   * \brief Apply a gate to n qubits.
   * \param owner Owner applying the gate.
   * \param gate Name of the gate.
   * \param data Data of the gate.
   * \param qubits Names of the qubits to be applied on.
   * \return True if the gate is applied successfully.
   * 
   * \note If the gate is not a reserved one (defined in QBasis), the data size must be 2^n * 2^n.
   * \note The qubits size must be n.
   * 
  */
  bool ApplyGate (const std::string &owner, const std::string &gate, 
                  const std::vector<std::complex<double>> &data, const std::vector<std::string> &qubits
  );

  /**
   * \brief Apply an operation to n qubits.
   * \param owner Owner applying the operation.
   * \param quantumOperation Quantum operation to be applied.
   * \param qubits Names of the qubits to be applied on.
   * \return True if the operation is applied successfully.
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
      const std::string&gate,
      const std::vector<std::complex<double>> &data, const std::vector<std::string> &control_qubits,
      const std::vector<std::string> &target_qubits);

  /**
   * \brief Measure n qubits.
   * \param owner Owner measuring the qubits. 
   * \param qubits Names of the qubits to be measured.
   * \return Measurement outcome and the probability distribution.
  */ 
  std::pair<unsigned, std::vector<double>>
  Measure (const std::string &owner, const std::vector<std::string> &qubits);

  /**
   * \brief Peek n qubits.
   * \param owner Owner peeking the qubits.
   * \param qubits Names of the qubits to be peeked.
   * \param dm Vector to store the density matrix of the qubits.
   * \return The composite density matrix of the qubits.
  */
  std::vector<std::complex<double>>
  PeekDM (const std::string &owner,
          const std::vector<std::string> &qubits,
          std::vector<std::complex<double>> &dm
  );

  /**
   * \brief Trace out n qubits by simply connecting the two ends of their wires.
   * \param qubits Names of the qubits to be traced out.
   * \param dm Vector to store the density matrix of the remainder.
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

  double CalculateFidelity (const std::pair<std::string, std::string> &epr, double &fidel);

/* util */

  void Evaluate (exatn::TensorNetwork *circuit);

  /**
   * \brief Check if the qubits are valid (not traced out).
   * \param qubits Names of the qubits to be checked.
   * \return True if the qubits are all valid.
  */
  bool CheckValid (const std::vector<std::string> &qubits) const;

  /**
   * \brief Allocate a new ExaTN tensor name.
   * \return The new ExaTN tensor name.
  */
  std::string AllocExatnName ();


/* debug */

  void PrintQubitsValid () const;

  void PrintTalshTensor (std::shared_ptr<talsh::Tensor> talsh_tensor,
                         const std::complex<double> *body_ptr);

  void PrintTalshTensorNamed (const std::string &name);  
};

} // namespace ns3

#endif /* QUANTUM_NETWORK_SIMULATOR_H */