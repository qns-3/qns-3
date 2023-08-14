#ifndef QUBIT_H
#define QUBIT_H

#include "ns3/object.h"

#include "ns3/quantum-basis.h"

#include <complex>

namespace ns3 {

/**
 * \brief Qubit in state vector representation.
 * 
 * \note This class is only used for attribute passing when setting up quantum applications.
*/
class Qubit : public Object
{

private:

  /** State vector of the qubit. */
  std::vector<std::complex<double>> m_state_vector;

public:

  Qubit (const std::vector<std::complex<double>> &state_vector_);
  ~Qubit ();

  Qubit ();
  static TypeId GetTypeId ();

  std::vector<std::complex<double>> GetStateVector () const;
};

} // namespace ns3

#endif /* QUBIT_H */