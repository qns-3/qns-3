#include "ns3/quantum-error-model.h"

#include "ns3/quantum-basis.h"
#include "ns3/quantum-phy-entity.h" // class QuantumPhyEntity
#include "ns3/quantum-operation.h" // QuantumOperation

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("QuantumErrorModel");

NS_OBJECT_ENSURE_REGISTERED (QuantumErrorModel);

/* abstract model */

QuantumErrorModel::QuantumErrorModel (bool time_dependent_)
    : m_time_dep (time_dependent_), m_type_desc ("QuantumErrorModel")
{
}

QuantumErrorModel::QuantumErrorModel () : m_time_dep (false), m_type_desc ("")
{
}

void
QuantumErrorModel::DoDispose (void)
{
  Object::DoDispose ();
}

TypeId
QuantumErrorModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::QuantumErrorModel").SetParent<Object> ();
  return tid;
}

std::string
QuantumErrorModel::GetTypeDesc () const
{
  return m_type_desc;
}

void
QuantumErrorModel::ApplyErrorModel (Ptr<QuantumPhyEntity> qphyent,
                                    const std::vector<std::string> &qubits,
                                    const Time &moment) const
{
  NS_LOG_ERROR (RED_CODE << "Error: reaching QuantumErrorModel::ApplyErrorModel" << END_CODE);
  assert (false);
}




/* time relevant model */

TimeModel::TimeModel (double rate_) : QuantumErrorModel (true), m_rate (rate_)
{
  m_type_desc = "TimeModel with rate " + std::to_string (m_rate);
}

TimeModel::TimeModel () : QuantumErrorModel (), m_rate (0)
{
}

void
TimeModel::DoDispose (void)
{
  QuantumErrorModel::DoDispose ();
}

TypeId
TimeModel::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::TimeModel").SetParent<QuantumErrorModel> ().AddConstructor<TimeModel> ();
  return tid;
}

void
TimeModel::ApplyErrorModel (Ptr<QuantumPhyEntity> qphyent, const std::vector<std::string> &qubits,
                            const Time &moment) const
{
  for (const std::string &qubit : qubits)
    {
      Time duration = moment - qphyent->m_qubit2time[qubit];
      if (duration.GetDouble () < 0)
        {
          NS_LOG_WARN ("duration < 0, skip");
          continue;
        }
      if (abs(duration.GetDouble () - 0) < EPS)
        {
          continue;
        }
      double prob_time = (1 - exp (-duration.GetSeconds () / m_rate)) / 2;
      QuantumOperation time = {{"I", "PZ"}, {pauli_I, pauli_Z}, 
      {1 - prob_time, prob_time}};

      NS_LOG_LOGIC ("At time " << moment.As (Time::S) << " qubit named " << qubit
                               << " applied time relevant error with prob " << prob_time
                               << " (duration = " << duration.As (Time::S)
                               << ", m_rate = " << m_rate << ")");

      qphyent->ApplyOperation (time, qubits);
    }
}




/* dephasing model */

DephaseModel::DephaseModel (double rate_)
    : QuantumErrorModel (false), // time independent
      m_rate (rate_)
{
  m_type_desc = "DephaseModel with rate " + std::to_string (m_rate );

}

DephaseModel::DephaseModel () : QuantumErrorModel (), m_rate (0)
{
}

void
DephaseModel::DoDispose (void)
{
  QuantumErrorModel::DoDispose ();
}

TypeId
DephaseModel::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::DephaseModel").SetParent<QuantumErrorModel> ().AddConstructor<DephaseModel> ();
  return tid;
}

void
DephaseModel::ApplyErrorModel (Ptr<QuantumPhyEntity> qphyent,
                               const std::vector<std::string> &qubits, const Time &moment) const
{
  for (const std::string &qubit : qubits)
    {
      Time duration = Seconds (GATE_DURATION);

      double prob_dephase = (1 - exp (- duration.GetSeconds () / m_rate)) / 2;
      QuantumOperation dephase = {
          {"I", "PZ"}, {pauli_I, pauli_Z}, 
          {1 - prob_dephase, prob_dephase}};

      NS_LOG_LOGIC ("At time " << moment.As (Time::S) << " qubit named " << qubit
                               << " applied dephasing error with prob = " << prob_dephase
                               << " (m_rate = " << m_rate << ")");
      qphyent->ApplyOperation (dephase, qubits);
    }
}





/* depolarizing model */

DepolarModel::DepolarModel (double fidel_)
    : QuantumErrorModel (false), // time independent
      m_fidel (fidel_)
{

  m_type_desc = "DepolarModel with fidel " + std::to_string (m_fidel);
}

DepolarModel::DepolarModel () : QuantumErrorModel (), m_fidel (0)
{
}

void
DepolarModel::DoDispose (void)
{
  QuantumErrorModel::DoDispose ();
}

TypeId
DepolarModel::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::DepolarModel").SetParent<QuantumErrorModel> ().AddConstructor<DepolarModel> ();
  return tid;
}

void
DepolarModel::ApplyErrorModel (Ptr<QuantumPhyEntity> qphyent,
                               const std::vector<std::string> &qubits, const Time &moment) const
{
  assert (qubits.size () == 1); // the second qubit in a epr pair

  QuantumOperation depolar = {
      {"I", "PX", "PY", "PZ"},
      {pauli_I, pauli_X, pauli_Y, pauli_Z},
      {m_fidel, (1 - m_fidel) / 3., (1 - m_fidel) / 3., (1 - m_fidel) / 3.}};

  NS_LOG_LOGIC ("At time +0s qubit named " << qubits[0] << " applied depolar error with fidel "
                                           << m_fidel);

  qphyent->ApplyOperation (depolar, qubits);
}

double 
DepolarModel::GetFidelity () const
{
  return m_fidel;
}

} // namespace ns3