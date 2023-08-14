#ifndef QUANTUM_ERROR_H
#define QUANTUM_ERROR_H

#include "ns3/quantum-basis.h"

namespace ns3 {
class QuantumOperation
{

private:

  /** Names of the unitaries composing this quantum operation. */
  std::vector<std::string> m_name;

  /** Unitaries composing this quantum operation. */
  std::vector<std::vector<std::complex<double>>> m_opr;

  /**
   * \brief Probabilities to perform each unitary. 
   * 
   * \note All probabilities should sum up to 1.
  */
  std::vector<double> m_prob;

  /** Number of unitaries in this quantum operation. */
  unsigned m_size;

public:
  /**
   * \brief Construct a quantum operation.
   * \param name_ Names of the unitaries composing this quantum operation.
   * \param opr_ Unitaries composing this quantum operation.
   * \param prob_ Probabilities to perform each unitary.
   * 
   * \note All probabilities should sum up to 1.
   * \note The size of name_, opr_ and prob_ should be the same.
  */
  QuantumOperation (const std::vector<std::string> &name_,
                    const std::vector<std::vector<std::complex<double>>> &opr_,
                    const std::vector<double> &prob_);

  const std::string &getName (unsigned idx) const;
  
  const std::vector<std::complex<double>> &getOpr (unsigned idx) const;
  
  const std::vector<std::vector<std::complex<double>>> &getOprs () const;
  
  const double &getProb (unsigned idx) const;
  
  const unsigned &getSize () const;
};

/** Default depolarizing quantum operation. */
const QuantumOperation depol = {{"I", "PX", "PY", "PZ"},
                                {pauli_I, pauli_X, pauli_Y, pauli_Z},
                                {0.8, 0.2 / 3, 0.2 / 3, 0.2 / 3}};

} // namespace ns3

#endif /* QUANTUM_ERROR_H */