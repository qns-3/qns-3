#include "ns3/quantum-operation.h"

namespace ns3 {
QuantumOperation::QuantumOperation (const std::vector<std::string> &name_,
                                    const std::vector<std::vector<std::complex<double>>> &opr_,
                                    const std::vector<double> &prob_)
    : m_name (name_), m_opr ({}), m_prob (prob_), m_size (opr_.size ())
{
  for (unsigned i = 0; i < m_size; ++i)
    {
      m_opr.push_back (sqrt (prob_[i]) * opr_[i]);
    }
}
const std::string &
QuantumOperation::getName (unsigned idx) const
{
  return m_name[idx];
}

const std::vector<std::complex<double>> &
QuantumOperation::getOpr (unsigned idx) const
{
  return m_opr[idx];
}
const std::vector<std::vector<std::complex<double>>> &
QuantumOperation::getOprs () const
{
  return m_opr;
}

const double &
QuantumOperation::getProb (unsigned idx) const
{
  return m_prob[idx];
}

const unsigned &
QuantumOperation::getSize () const
{
  return m_size;
}
} // namespace ns3