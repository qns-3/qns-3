#ifndef QUANTUM_MEMORY_H
#define QUANTUM_MEMORY_H

#include "ns3/object.h"

namespace ns3 {

class QuantumPhyEntity;
class QuantumErrorModel;

class QuantumMemory : public Object
{

private:

  /** Names of the qubits in this quantum memory. */
  std::vector<std::string> m_qubits;

public:

  QuantumMemory (std::vector<std::string> qubits_);
  ~QuantumMemory ();

  QuantumMemory ();
  static TypeId GetTypeId ();

  /**
   * \brief Add a qubit to the quantum memory.
   * \param qubit Name of the qubit to be added.
  */
  void AddQubit (std::string qubit);

  /**
   * \brief Remove a qubit from the quantum memory.
   * \param qubit Name of the qubit to be removed.
   * \return True if the qubit is successfully removed.
  */
  bool RemoveQubit (std::string qubit);

  /**
   * \brief Get the size of the quantum memory.
   * \return Number of qubits in the quantum memory.
  */
  unsigned GetSize () const;

  /**
   * \brief Get the qubit at a specific position.
   * \param local The position in the quantum memory to query.
   * \return Name of the qubit at the position.
  */
  std::string GetQubit (unsigned local) const;


  /**
   * \brief Check if a qubit is in the quantum memory.
   * \param qubit Name of the qubit to be checked.
   * \return True if the qubit is in the quantum memory.
  */
  bool ContainQubit (std::string qubit) const;
};

} // namespace ns3

#endif /* QUANTUM_MEMORY_H */