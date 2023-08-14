#include "ns3/quantum-network-simulator.h" // class QuantumNetworkSimulator

#include "ns3/quantum-basis.h"
#include "ns3/quantum-operation.h" // class QuantumOperation

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("QuantumNetworkSimulator");

QuantumNetworkSimulator::QuantumNetworkSimulator (const std::vector<std::string> &owners)
    : m_dm (exatn::TensorNetwork ()),
      m_dm_id (1),
      m_qubits_all (std::vector<std::string> ()),
      m_qubits_vld (std::vector<std::string> ()),
      m_qubit2tensor (std::map<std::string, std::pair<unsigned, unsigned>> ()),
      m_qubit2tensor_dag (std::map<std::string, std::pair<unsigned, unsigned>> ()),

      m_exatn_name_count (0),
      m_exatn_tensors (std::vector<std::string> ())
{
  /* circuit */

  exatn::initialize ();

  m_dm.rename (AllocExatnName ());
}

QuantumNetworkSimulator::QuantumNetworkSimulator (const QuantumNetworkSimulator &other)
{
  m_dm = other.m_dm;
  m_dm.rename (AllocExatnName ());

  m_dm_id = other.m_dm_id;
  m_qubits_all = other.m_qubits_all;
  m_qubits_vld = other.m_qubits_vld;
  m_qubit2tensor = other.m_qubit2tensor;
  m_qubit2tensor_dag = other.m_qubit2tensor_dag;
}

QuantumNetworkSimulator::~QuantumNetworkSimulator ()
{
  exatn::finalize ();
}

QuantumNetworkSimulator::QuantumNetworkSimulator ()
    : m_dm (exatn::TensorNetwork ()),
      m_dm_id (1),
      m_qubits_all (std::vector<std::string> ()),
      m_qubits_vld (std::vector<std::string> ()),
      m_qubit2tensor (std::map<std::string, std::pair<unsigned, unsigned>> ()),
      m_qubit2tensor_dag (std::map<std::string, std::pair<unsigned, unsigned>> ())
{
}

TypeId
QuantumNetworkSimulator::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::QuantumNetworkSimulator")
                          .SetParent<Object> ()
                          .AddConstructor<QuantumNetworkSimulator> ();
  return tid;
}



/* ExaTN */

void
QuantumNetworkSimulator::PrepareQubitsPure (const std::string &name,
                                           std::vector<std::complex<double>> data)
{
  if (std::find (m_exatn_tensors.begin (), m_exatn_tensors.end (), name) != m_exatn_tensors.end ())
    {
      std::cout << "Preparing a tensor named \"" + name + "\" for some qubits' state twice. \\
                    Its okay but data ignored :)"
                << std::endl;
      return;
    }
  m_exatn_tensors.push_back (name);

  std::vector<size_t> extents (Log2 (data.size ()), 2);
  assert (exatn::createTensor (name, exatn::TensorElementType::COMPLEX64, extents));
  assert (exatn::initTensorData (name, data));
}

void
QuantumNetworkSimulator::PrepareQubitsMixed (const std::string &name,
                                             std::vector<std::complex<double>> data)
{
  if (std::find (m_exatn_tensors.begin (), m_exatn_tensors.end (), name) != m_exatn_tensors.end ())
    {
      std::cout << "Preparing a tensor named \"" + name + "\" for some qubits' state twice. \\
                    Its okay but data ignored :)"
                << std::endl;
      return;
    }
  m_exatn_tensors.push_back (name);

  std::vector<size_t> extents ((Log2 (sqrt (data.size ())) << 1), 2);
  assert (exatn::createTensor (name, exatn::TensorElementType::COMPLEX64, extents));
  assert (exatn::initTensorData (name, data));
}

void
QuantumNetworkSimulator::PrepareGate (const std::string &name,
                                      const std::vector<std::complex<double>> &data)
{
  if (std::find (m_exatn_tensors.begin (), m_exatn_tensors.end (), name) != m_exatn_tensors.end ())
    {
      return;
    }
  NS_LOG_LOGIC ("Preparing a gate named \"" << name << "\"");
  m_exatn_tensors.push_back (name);

  std::vector<size_t> extents = {};
  for (size_t i = 0; i < (Log2 (sqrt (data.size ())) << 1); ++i)
    extents.push_back (2);

  assert (exatn::createTensor (name, exatn::TensorElementType::COMPLEX64, extents));
  assert (exatn::initTensorData (name, data));
}

void
QuantumNetworkSimulator::PrepareOperation (
    const std::string &name, const std::vector<std::vector<std::complex<double>>> &data)
{
  if (std::find (m_exatn_tensors.begin (), m_exatn_tensors.end (), name) != m_exatn_tensors.end ())
    {
      return;
    }
  m_exatn_tensors.push_back (name);

  std::vector<size_t> extents = {};
  for (size_t i = 0; i < sqrt (data[0].size ()); ++i)
    extents.push_back (2);
  extents.push_back (data.size ());
  assert (exatn::createTensor (name, exatn::TensorElementType::COMPLEX64, extents));

  std::vector<std::complex<double>> data_flat = {};
  for (const std::vector<std::complex<double>> &op : data)
    {
      data_flat.insert (data_flat.end (), op.begin (), op.end ());
    }
  assert (exatn::initTensorData (name, data_flat));
}

void
QuantumNetworkSimulator::PrepareTensor (const std::string &name,
                                        const std::vector<unsigned> &extents,
                                        const std::vector<std::complex<double>> &data)
{
  if (std::find (m_exatn_tensors.begin (), m_exatn_tensors.end (), name) != m_exatn_tensors.end ())
    {
      return;
    }
  m_exatn_tensors.push_back (name);

  assert (exatn::createTensor (name, exatn::TensorElementType::COMPLEX64, extents));
  assert (exatn::initTensorData (name, data));
}



/* circuit */

bool
QuantumNetworkSimulator::GenerateQubitsPure (
    const std::string &owner,
    const std::vector<std::complex<double>> &data,
    const std::vector<std::string> &qubits
)
{
  Time moment = Simulator::Now ();
  std::string name = AllocExatnName ();
  PrepareQubitsPure (name, data);
  NS_LOG_INFO (BLUE_CODE << "At time " << moment.As (Time::S) << " " << owner
                         << " generates qubit(s) named");
  for (const std::string &qubit : qubits)
    {
      NS_LOG_INFO (qubit);
    }
  NS_LOG_INFO (END_CODE);

  unsigned delta_height = Log2 (data.size ());
  assert (delta_height == qubits.size ());

  std::vector<exatn::LegDirection> leg_dir (delta_height, exatn::LegDirection::OUTWARD);
  std::vector<exatn::LegDirection> leg_dir_dag (delta_height, exatn::LegDirection::INWARD);

  for (const std::string &qubit : qubits)
    {
      assert (m_qubit2tensor.find (qubit) == m_qubit2tensor.end ());
      assert (m_qubit2tensor_dag.find (qubit) == m_qubit2tensor_dag.end ());
    }

  // onto the left half
  m_dm.appendTensor (m_dm_id++, exatn::getTensor (name), {}, leg_dir, false);
  unsigned tensor_id = m_dm.getMaxTensorId ();
  assert (tensor_id == m_dm_id - 1);

  // onto the right half
  m_dm.appendTensor (m_dm_id++, exatn::getTensor (name), {}, leg_dir_dag, true);
  unsigned tensor_id_dag = m_dm.getMaxTensorId ();
  assert (tensor_id_dag == m_dm_id - 1);

  for (unsigned i = 0; i < delta_height; ++i)
    {
      std::string qubit = qubits[i];

      m_qubits_all.push_back (qubit);
      m_qubits_vld.push_back (qubit);

      //             qubit idx tensor id  leg idx
      m_qubit2tensor[qubit] = {tensor_id, i};
      m_qubit2tensor_dag[qubit] = {tensor_id_dag, i};
    }

  return true;
}

bool
QuantumNetworkSimulator::GenerateQubitsMixed (
    const std::string &owner,
    const std::vector<std::complex<double>> &data,
    const std::vector<std::string> &qubits
)
{
  Time moment = Simulator::Now ();
  std::string name = AllocExatnName ();
  PrepareQubitsMixed (name, data);
  
  NS_LOG_INFO (BLUE_CODE << "At time " << moment.As (Time::S) << " " << owner
                         << " generates qubit(s) named");
  for (const std::string &qubit : qubits)
    {
      NS_LOG_INFO (qubit);
    }
  NS_LOG_INFO (END_CODE);

  unsigned delta_height = Log2 (sqrt (data.size ()));
  assert (delta_height == qubits.size ());

  std::vector<exatn::LegDirection> leg_dirs (delta_height, exatn::LegDirection::INWARD);
  for (const std::string &qubit : qubits)
    {
      leg_dirs.push_back (exatn::LegDirection::OUTWARD);
    }

  m_dm.appendTensor (m_dm_id++, exatn::getTensor (name), {}, leg_dirs, false);
  unsigned tensor_id = m_dm.getMaxTensorId ();

  for (unsigned i = 0; i < delta_height; ++i)
    {
      std::string qubit = qubits[i];

      m_qubits_all.push_back (qubit);
      m_qubits_vld.push_back (qubit);

      //             qubit idx tensor id  leg idx
      m_qubit2tensor[qubit] = {tensor_id, delta_height + i};
      m_qubit2tensor_dag[qubit] = {tensor_id, i};
    }

  return true;
}

bool
QuantumNetworkSimulator::ApplyGate (
    const std::string &owner,
    const std::string &gate,
    const std::vector<std::complex<double>> &data,
    const std::vector<std::string> &qubits
)
{
  Time moment = Simulator::Now ();

  assert (CheckValid (qubits));

  if (gate2data.find (gate) != gate2data.end ())
    PrepareGate (gate, gate2data.find (gate)->second);
  else {
    assert (data.size ());
    PrepareGate (gate, data);
  }

  NS_LOG_INFO (BLUE_CODE << "At time " << moment.As (Time::S) << " " << owner << " applies gate "
                         << gate << " to qubits(s)");
  for (const std::string &qubit : qubits)
    {
      NS_LOG_INFO (qubit);
    }
  NS_LOG_INFO (END_CODE);

  // using qubit2tensor
  std::vector<unsigned> old_tensor = {};
  std::vector<unsigned> old_leg = {};
  for (const std::string &qubit : qubits)
    {
      old_tensor.push_back (m_qubit2tensor[qubit].first);
      old_leg.push_back (m_qubit2tensor[qubit].second);
    }
  std::vector<unsigned> old_tensor_dag = {};
  std::vector<unsigned> old_leg_dag = {};
  for (const std::string &qubit : qubits)
    {
      old_tensor_dag.push_back (m_qubit2tensor_dag[qubit].first);
      old_leg_dag.push_back (m_qubit2tensor_dag[qubit].second);
    }
  std::vector<unsigned> new_leg = {};
  for (unsigned i = 0; i < qubits.size (); ++i)
    {
      new_leg.push_back (i);
    }

  // onto the left half
  std::vector<std::pair<unsigned, unsigned>> pairing = {};
  for (unsigned i = 0; i < old_tensor.size (); ++i)
    {
      pairing.push_back (
          {m_dm.getTensorConn (old_tensor[i])->getTensorLeg (old_leg[i]).getDimensionId (),
           new_leg[i]});
    }
  std::vector<exatn::LegDirection> leg_dir = {};
  for (const std::string &qubit : qubits)
    leg_dir.push_back (exatn::LegDirection::INWARD);
  for (const std::string &qubit : qubits)
    leg_dir.push_back (exatn::LegDirection::OUTWARD);
  m_dm.appendTensor (m_dm_id++, exatn::getTensor (gate), pairing, leg_dir, false);

  unsigned tensor_id = m_dm.getMaxTensorId ();
  assert (tensor_id == m_dm_id - 1);

  // updating qubit2tensor
  for (unsigned i = 0; i < qubits.size (); ++i)
    {
      //             qubit idx     tensor id  leg idx
      m_qubit2tensor[qubits[i]] = {tensor_id, qubits.size () + i};
    }

  // onto the right half
  std::vector<std::pair<unsigned, unsigned>> pairing_dag = {};
  for (unsigned i = 0; i < old_tensor_dag.size (); ++i)
    {
      pairing_dag.push_back (
          {m_dm.getTensorConn (old_tensor_dag[i])->getTensorLeg (old_leg_dag[i]).getDimensionId (),
           new_leg[i]});
    }
  std::vector<exatn::LegDirection> leg_dir_dag = {};
  for (const std::string &qubit : qubits)
    leg_dir_dag.push_back (exatn::LegDirection::OUTWARD);
  for (const std::string &qubit : qubits)
    leg_dir_dag.push_back (exatn::LegDirection::INWARD);

  m_dm.appendTensor (m_dm_id++, exatn::getTensor (gate), pairing_dag, leg_dir_dag, true);
  unsigned tensor_id_dag = m_dm.getMaxTensorId ();
  assert (tensor_id_dag == m_dm_id - 1);

  for (unsigned i = 0; i < qubits.size (); ++i)
    {
      m_qubit2tensor_dag[qubits[i]] = {tensor_id_dag, qubits.size () + i};
    }

  return true;
}

bool
QuantumNetworkSimulator::ApplyOperation (
    const QuantumOperation &quantumOperation,
    const std::vector<std::string> &qubits
)
{
  Time moment = Simulator::Now ();
  if (qubits.size () != 1)
    {
      NS_LOG_INFO (RED_CODE << "Applying a multi-qubit quantum operation" << END_CODE);
    }

  assert (CheckValid (qubits));

  std::string name = AllocExatnName ();
  PrepareOperation (name, quantumOperation.getOprs ());

  NS_LOG_LOGIC ("At time " << moment.As (Time::S) << " applying operation to qubits(s)");
  for (const auto &qubit : qubits)
    {
      NS_LOG_LOGIC (qubit);
    }
  NS_LOG_LOGIC (END_CODE);

  // using qubit2tensor
  std::vector<unsigned> tensor_id = {};
  std::vector<unsigned> leg_idx = {};
  for (const std::string &qubit : qubits)
    {
      tensor_id.push_back (m_qubit2tensor[qubit].first);
      leg_idx.push_back (m_qubit2tensor[qubit].second);
    }
  std::vector<unsigned> tensor_id_dag = {};
  std::vector<unsigned> leg_idx_dag = {};
  for (const std::string &qubit : qubits)
    {
      tensor_id_dag.push_back (m_qubit2tensor_dag[qubit].first);
      leg_idx_dag.push_back (m_qubit2tensor_dag[qubit].second);
    }

  // onto the left half
  std::vector<std::pair<unsigned, unsigned>> pairing = {};
  std::vector<exatn::LegDirection> leg_dir = {};
  for (unsigned i = 0; i < tensor_id.size (); ++i)
    {
      pairing.push_back (
          {m_dm.getTensorConn (tensor_id[i])->getTensorLeg (leg_idx[i]).getDimensionId (),
           (i << 1)});
      leg_dir.push_back (exatn::LegDirection::INWARD);
      leg_dir.push_back (exatn::LegDirection::OUTWARD);
    }
  leg_dir.push_back (exatn::LegDirection::OUTWARD);

  m_dm.appendTensor (m_dm_id++, exatn::getTensor (name), pairing, leg_dir, false);

  // updating qubit2tensor
  for (unsigned i = 0; i < qubits.size (); ++i)
    {
      m_qubit2tensor[qubits[i]] = {m_dm.getMaxTensorId (), (i << 1) + 1};
    }

  // onto the right half
  std::vector<std::pair<unsigned, unsigned>> pairing_dag = {
      {m_dm.getTensorConn (m_dm_id - 1)->getTensorLeg ((qubits.size () << 1)).getDimensionId (),
       (qubits.size () << 1)}};
  std::vector<exatn::LegDirection> leg_dir_dag = {};
  for (unsigned i = 0; i < tensor_id_dag.size (); ++i)
    {
      pairing_dag.push_back (
          {m_dm.getTensorConn (tensor_id_dag[i])->getTensorLeg (leg_idx_dag[i]).getDimensionId (),
           (i << 1)});
      leg_dir_dag.push_back (exatn::LegDirection::OUTWARD);
      leg_dir_dag.push_back (exatn::LegDirection::INWARD);
    }
  leg_dir_dag.push_back (exatn::LegDirection::INWARD);

  m_dm.appendTensor (m_dm_id++, exatn::getTensor (name), pairing_dag, leg_dir_dag, true);

  for (unsigned i = 0; i < qubits.size (); ++i)
    {
      m_qubit2tensor_dag[qubits[i]] = {m_dm.getMaxTensorId (), (i << 1) + 1};
    }

  return true;
}

bool
QuantumNetworkSimulator::ApplyControledOperation (
    const std::string &orig_owner,
    const std::string &orig_gate,
    const std::string &gate,
    const std::vector<std::complex<double>> &data, const std::vector<std::string> &control_qubits,
    const std::vector<std::string> &target_qubits)
{
  std::vector<std::string> operated_qubits = target_qubits;
  operated_qubits.insert (operated_qubits.end (), control_qubits.begin (), control_qubits.end ());
  ApplyGate ("God", gate, data, operated_qubits);

  return true;
}

std::pair<unsigned, std::vector<double>>
QuantumNetworkSimulator::Measure (
    const std::string &owner, 
    const std::vector<std::string> &qubits
)
{
  Time moment = Simulator::Now ();
  assert (qubits.size () == 1);

  std::string qubit = qubits[0];
  NS_LOG_INFO (BLUE_CODE << "At time " << moment.As (Time::S) << " " << owner
                         << " measures the qubit named " << qubits[0] << END_CODE);

  // using qubit2tensor
  std::vector<unsigned> tensor_id = {};
  std::vector<unsigned> leg_idx = {};
  for (const std::string &qubit : qubits)
    {
      tensor_id.push_back (m_qubit2tensor[qubit].first);
      leg_idx.push_back (m_qubit2tensor[qubit].second);
    }
  std::vector<unsigned> tensor_id_dag = {};
  std::vector<unsigned> leg_idx_dag = {};
  for (const std::string &qubit : qubits)
    {
      tensor_id_dag.push_back (m_qubit2tensor_dag[qubit].first);
      leg_idx_dag.push_back (m_qubit2tensor_dag[qubit].second);
    }

  std::vector<unsigned> other_tensor_id = {};
  std::vector<unsigned> other_leg_idx = {};
  std::vector<unsigned> other_tensor_id_dag = {};
  std::vector<unsigned> other_leg_idx_dag = {};
  for (const std::string &q : m_qubits_all)
    {
      if (!CheckValid ({q}))
        {
          continue;
        }
      if (q != qubit)
        {
          other_tensor_id.push_back (m_qubit2tensor[q].first);
          other_leg_idx.push_back (m_qubit2tensor[q].second);
          other_tensor_id_dag.push_back (m_qubit2tensor_dag[q].first);
          other_leg_idx_dag.push_back (m_qubit2tensor_dag[q].second);
        }
    }

  // pick outcome
  std::vector<double> prob_dist = {};
  unsigned outcome = 0;

  exatn::TensorNetwork circuit_meas;
  std::string gate = QNS_GATE_PREFIX + "M0";
  PrepareGate (gate, meas_0);

  circuit_meas = m_dm;
  unsigned id = m_dm_id;
  circuit_meas.rename (AllocExatnName ());
  circuit_meas.appendTensor (
      id++, exatn::getTensor (gate),
      {{circuit_meas.getTensorConn (tensor_id[0])->getTensorLeg (leg_idx[0]).getDimensionId (), 0}},
      {exatn::LegDirection::INWARD, exatn::LegDirection::OUTWARD}, false);
  circuit_meas.appendTensor (
      id++, exatn::getTensor (gate),
      {{circuit_meas.getTensorConn (tensor_id_dag[0])
            ->getTensorLeg (leg_idx_dag[0])
            .getDimensionId (),
        0},
       {circuit_meas.getTensorConn (id - 1)->getTensorLeg (1).getDimensionId (), 1}},
      {exatn::LegDirection::OUTWARD, exatn::LegDirection::INWARD}, true);

  PrepareGate (QNS_GATE_PREFIX + "I", pauli_I);
  for (unsigned i = 0; i < other_tensor_id.size (); ++i)
    {
      circuit_meas.appendTensor (id++, exatn::getTensor (QNS_GATE_PREFIX + "I"),
                                 {{circuit_meas.getTensorConn (other_tensor_id[i])
                                       ->getTensorLeg (other_leg_idx[i])
                                       .getDimensionId (),
                                   0},
                                  {circuit_meas.getTensorConn (other_tensor_id_dag[i])
                                       ->getTensorLeg (other_leg_idx_dag[i])
                                       .getDimensionId (),
                                   1}},
                                 {exatn::LegDirection::INWARD, exatn::LegDirection::OUTWARD},
                                 false);
    }
  double prob = 0.0;
  Evaluate (&circuit_meas);
  auto talsh_tensor = exatn::getLocalTensor (circuit_meas.getTensor (0)->getName ());
  assert (talsh_tensor->getVolume () == 1);
  const std::complex<double> *body_ptr;
  if (talsh_tensor->getDataAccessHostConst (&body_ptr))
    {
      assert (abs ((*body_ptr).imag ()) < EPS);
      prob = (*body_ptr).real ();
    }

  // set the probability distribution
  prob_dist.push_back (prob);
  prob_dist.push_back (1.0 - prob);

  // pick the outcome according to the probability distribution
  std::complex<double> outcome_dirty = PickOutcome (prob);
  outcome = (fabs (outcome_dirty.real () - 1.0) < EPS);

  // update circuit
  std::string name;
  std::vector<std::complex<double>> data;
  if (outcome)
    {
      data = (1 / sqrt (1 - prob)) * meas_1;
    }
  else
    {
      data = (1 / sqrt (prob)) * meas_0;
    }

  ApplyGate (owner, AllocExatnName (), data, qubits);

  return {outcome, prob_dist};
}

std::vector<std::complex<double>>
QuantumNetworkSimulator::PeekDM (
    const std::string &owner,
    const std::vector<std::string> &qubits,
    std::vector<std::complex<double>> &dm
)
{
  NS_LOG_LOGIC ("DM has now the number of tensors: " << m_dm.getNumTensors ());
  Time moment = Simulator::Now ();
  NS_LOG_INFO (BLUE_CODE << "At time " << moment.As (Time::S) << " " << owner
                         << " peeks density matrix on qubit(s)");
  for (const std::string &qubit : qubits)
    {
      NS_LOG_INFO (qubit);
    }
  NS_LOG_INFO (END_CODE);

  // using qubit2tensor
  std::vector<unsigned> other_tensor_id = {};
  std::vector<unsigned> other_leg_idx = {};
  std::vector<unsigned> other_tensor_id_dag = {};
  std::vector<unsigned> other_leg_idx_dag = {};
  for (const std::string &q : m_qubits_all)
    {
      if (!CheckValid ({q}))
        {
          continue;
        }
      if (std::find (qubits.begin (), qubits.end (), q) == qubits.end ())
        {
          other_tensor_id.push_back (m_qubit2tensor[q].first);
          other_leg_idx.push_back (m_qubit2tensor[q].second);
          other_tensor_id_dag.push_back (m_qubit2tensor_dag[q].first);
          other_leg_idx_dag.push_back (m_qubit2tensor_dag[q].second);
        }
    }

  // copy out the circuit
  exatn::TensorNetwork circuit_peek = m_dm;
  circuit_peek.rename (AllocExatnName ());
  unsigned id = m_dm_id;

  // partial trace
  PrepareGate (QNS_GATE_PREFIX + "I", pauli_I);
  for (unsigned i = 0; i < other_tensor_id.size (); ++i)
    {
      circuit_peek.appendTensor (id++, exatn::getTensor (QNS_GATE_PREFIX + "I"),
                                 {{circuit_peek.getTensorConn (other_tensor_id[i])
                                       ->getTensorLeg (other_leg_idx[i])
                                       .getDimensionId (),
                                   0},
                                  {circuit_peek.getTensorConn (other_tensor_id_dag[i])
                                       ->getTensorLeg (other_leg_idx_dag[i])
                                       .getDimensionId (),
                                   1}},
                                 {exatn::LegDirection::INWARD, exatn::LegDirection::OUTWARD},
                                 false);
    }

  // reorder the output legs
  std::vector<unsigned> order = {};
  for (unsigned i = 0; i < qubits.size (); ++i)
    {
      order.push_back (circuit_peek.getTensorConn (m_qubit2tensor[qubits[i]].first)
                           ->getTensorLeg (m_qubit2tensor[qubits[i]].second)
                           .getDimensionId ());
    }
  for (unsigned i = 0; i < qubits.size (); ++i)
    {
      order.push_back (circuit_peek.getTensorConn (m_qubit2tensor_dag[qubits[i]].first)
                           ->getTensorLeg (m_qubit2tensor_dag[qubits[i]].second)
                           .getDimensionId ());
    }
  circuit_peek.reorderOutputModes (order);


  Evaluate (&circuit_peek);

  // access data
  assert (circuit_peek.getTensor (0));
  auto talsh_tensor = exatn::getLocalTensor (circuit_peek.getTensor (0)->getName ());
  NS_LOG_INFO (LIGHT_YELLOW_CODE << "Density Matrix: " << END_CODE);
  assert (talsh_tensor);
  const std::complex<double> *body_ptr;
  if (talsh_tensor->getDataAccessHostConst (&body_ptr))
    {
      printf ("[\n");
      assert (std::sqrt (talsh_tensor->getVolume ()) == std::pow (2, qubits.size ()));
      if (qubits.size () < 5)
        {
          unsigned dim = std::sqrt (talsh_tensor->getVolume ());
          for (unsigned i = 0; i < talsh_tensor->getVolume (); ++i)
            {              
              if (i % dim == i / dim) // diagonal
                {
                  std::cout << "<" << body_ptr[i] << ">";
                }
              else
                {
                  std::cout << " " << body_ptr[i] << " ";
                }
              if ((i + 1) % dim == 0)
                {
                  printf ("\n");
                }
              dm.push_back (body_ptr[i]);
            }
        }
      else
        { // too long for readability
          printf ("...");
          for (unsigned i = 0; i < talsh_tensor->getVolume (); ++i)
            {
              dm.push_back (body_ptr[i]);
            }
        }
      printf ("]\n");
    }

  return dm;
}

bool
QuantumNetworkSimulator::PartialTrace (
    const std::vector<std::string> &qubits,
    std::vector<std::complex<double>> &dm
)
{
  Time moment = Simulator::Now ();
  NS_LOG_INFO (BLUE_CODE << "At time " << moment.As (Time::S) << " tracing out qubit(s) named:");
  for (const std::string &qubit : qubits)
    {
      NS_LOG_INFO (qubit);
    }
  NS_LOG_INFO (END_CODE);

  assert (CheckValid (qubits));

  // using qubit2tensor
  std::vector<unsigned> tensor_id = {};
  std::vector<unsigned> leg_idx = {};
  for (const std::string &qubit : qubits)
    {
      tensor_id.push_back (m_qubit2tensor[qubit].first);
      leg_idx.push_back (m_qubit2tensor[qubit].second);
    }
  std::vector<unsigned> tensor_id_dag = {};
  std::vector<unsigned> leg_idx_dag = {};
  for (const std::string &qubit : qubits)
    {
      tensor_id_dag.push_back (m_qubit2tensor_dag[qubit].first);
      leg_idx_dag.push_back (m_qubit2tensor_dag[qubit].second);
    }

  // partial trace
  PrepareGate (QNS_GATE_PREFIX + "I", pauli_I);
  for (unsigned i = 0; i < tensor_id.size (); ++i)
    {
      m_dm.appendTensor (
          m_dm_id++, exatn::getTensor (QNS_GATE_PREFIX + "I"),
          {{m_dm.getTensorConn (tensor_id[i])->getTensorLeg (leg_idx[i]).getDimensionId (), 0},
           {m_dm.getTensorConn (tensor_id_dag[i])->getTensorLeg (leg_idx_dag[i]).getDimensionId (),
            1}},
          {exatn::LegDirection::INWARD, exatn::LegDirection::OUTWARD}, false);
    }

  NS_LOG_LOGIC ("(qubit(s) named");
  for (const std::string &qubit : qubits)
    {
      NS_LOG_LOGIC (qubit);
      assert (CheckValid ({qubit}));
      m_qubits_vld.erase (std::find (m_qubits_vld.begin (), m_qubits_vld.end (), qubit));
    }
  NS_LOG_LOGIC ("traced out)" << END_CODE);

  return true;
}

std::vector<std::complex<double>>
QuantumNetworkSimulator::Contract ()
{
  NS_LOG_INFO (BLUE_CODE << "Contracting the tensor network" << END_CODE);

  // evaluate the tensor network to get the density matrix
  Evaluate (&m_dm);
  std::vector<std::complex<double>> dm;
  auto talsh_tensor = exatn::getLocalTensor (m_dm.getTensor (0)->getName ());
  const std::complex<double> *body_ptr;
  assert (talsh_tensor);
  if (talsh_tensor->getDataAccessHostConst (&body_ptr))
    {
      for (unsigned i = 0; i < talsh_tensor->getVolume (); ++i)
        {
          dm.push_back (body_ptr[i]);
        }
    }

  // construct a tensor of the density matrix
  std::string contracted_name = AllocExatnName ();
  std::vector<unsigned> extents (m_dm.getRank (), 2);
  PrepareTensor (contracted_name, extents, dm);

  // update qubit2tensor
  assert (m_dm.getRank () == m_qubits_vld.size () * 2);
  for (unsigned i = 0; i < m_qubits_vld.size (); ++i)
    {
      m_qubit2tensor[m_qubits_vld[i]] = {1,
        m_dm.getTensorConn (m_qubit2tensor[m_qubits_vld[i]].first)
            ->getTensorLeg (m_qubit2tensor[m_qubits_vld[i]].second)
            .getDimensionId ()};
      m_qubit2tensor_dag[m_qubits_vld[i]] = {1,
        m_dm.getTensorConn (m_qubit2tensor_dag[m_qubits_vld[i]].first)
            ->getTensorLeg (m_qubit2tensor_dag[m_qubits_vld[i]].second)
            .getDimensionId ()};
    }
  
  // reset the tensor network
  std::vector<exatn::LegDirection> leg_dirs (m_dm.getRank (), exatn::LegDirection::UNDIRECT);
  for (unsigned i = 0; i < m_qubits_vld.size (); ++i)
    {
      leg_dirs[m_qubit2tensor[m_qubits_vld[i]].second] = exatn::LegDirection::OUTWARD;
      leg_dirs[m_qubit2tensor_dag[m_qubits_vld[i]].second] = exatn::LegDirection::INWARD;
    }
  for (unsigned i = 0; i < m_dm.getRank (); ++i)
    {
      assert (leg_dirs[i] != exatn::LegDirection::UNDIRECT);
    }

  m_dm = exatn::TensorNetwork ();
  m_dm.rename (AllocExatnName ());
  m_dm_id = 1;
  m_dm.appendTensor (m_dm_id++, exatn::getTensor (contracted_name), {}, leg_dirs, false);

  return dm;
}


void
QuantumNetworkSimulator::Evaluate (exatn::TensorNetwork *circuit)
{
  // exatn::resetContrSeqOptimizer ("dummy");
  // exatn::resetContrSeqOptimizer ("heuro");
  exatn::resetContrSeqOptimizer ("greed");
  // exatn::resetContrSeqOptimizer ("metis");
  // exatn::resetContrSeqOptimizer ("cutnn");

  auto start_time = std::chrono::high_resolution_clock::now ();
  circuit->collapseIsometries ();
  exatn::evaluateSync (*circuit);
  auto end_time = std::chrono::high_resolution_clock::now ();
  auto duration = std::chrono::duration_cast<std::chrono::seconds> (end_time - start_time);
  NS_LOG_INFO (PURPLE_CODE << "Evaluating tensor network of size " << circuit->getNumTensors ()
                           << " with time cost " << duration.count () << " s" << END_CODE);

}

double
QuantumNetworkSimulator::CalculateFidelity (
  const std::pair<std::string, std::string> &epr, double &fidel
)
{
  NS_LOG_INFO (CYAN_CODE << "Calculating fidelity for epr pair (" << epr.first << ", " << epr.second << ")" << END_CODE);

  // state vector of bell
  PrepareTensor (QNS_PREFIX + "BellSV", {2, 2}, q_bell);

  // the epr density matrix rho with noise
  std::vector<std::string> qubits = {epr.first, epr.second};
  std::vector<unsigned> other_tensor_id = {};
  std::vector<unsigned> other_leg_idx = {};
  std::vector<unsigned> other_tensor_id_dag = {};
  std::vector<unsigned> other_leg_idx_dag = {};
  for (const std::string &q : m_qubits_all)
    {
      if (!CheckValid ({q}))
        {
          continue;
        }
      if (std::find (qubits.begin (), qubits.end (), q) == qubits.end ())
        {
          other_tensor_id.push_back (m_qubit2tensor[q].first);
          other_leg_idx.push_back (m_qubit2tensor[q].second);
          other_tensor_id_dag.push_back (m_qubit2tensor_dag[q].first);
          other_leg_idx_dag.push_back (m_qubit2tensor_dag[q].second);
        }
    }

  // copy out the circuit
  exatn::TensorNetwork circuit_peek = m_dm;
  circuit_peek.rename (AllocExatnName ());
  unsigned id = m_dm_id;

  // partial trace
  PrepareGate (QNS_GATE_PREFIX + "I", pauli_I);
  for (unsigned i = 0; i < other_tensor_id.size (); ++i)
    {
      circuit_peek.appendTensor (id++, exatn::getTensor (QNS_GATE_PREFIX + "I"),
                                 {{circuit_peek.getTensorConn (other_tensor_id[i])
                                       ->getTensorLeg (other_leg_idx[i])
                                       .getDimensionId (),
                                   0},
                                  {circuit_peek.getTensorConn (other_tensor_id_dag[i])
                                       ->getTensorLeg (other_leg_idx_dag[i])
                                       .getDimensionId (),
                                   1}},
                                 {exatn::LegDirection::INWARD, exatn::LegDirection::OUTWARD},
                                 false);
    }

  // reorder the output legs
  std::vector<unsigned> order = {};
  for (unsigned i = 0; i < qubits.size (); ++i)
    {
      order.push_back (circuit_peek.getTensorConn (m_qubit2tensor[qubits[i]].first)
                           ->getTensorLeg (m_qubit2tensor[qubits[i]].second)
                           .getDimensionId ());
    }
  for (unsigned i = 0; i < qubits.size (); ++i)
    {
      order.push_back (circuit_peek.getTensorConn (m_qubit2tensor_dag[qubits[i]].first)
                           ->getTensorLeg (m_qubit2tensor_dag[qubits[i]].second)
                           .getDimensionId ());
    }
  circuit_peek.reorderOutputModes (order);

  Evaluate (&circuit_peek);

  assert (circuit_peek.getTensor (0));
  NS_LOG_INFO (YELLOW_CODE << ":");

  // access data
  assert (circuit_peek.getTensor (0));
  auto talsh_tensor = exatn::getLocalTensor (circuit_peek.getTensor (0)->getName ());
  NS_LOG_INFO (LIGHT_YELLOW_CODE << "Density Matrix: " << END_CODE);
  assert (talsh_tensor);
  const std::complex<double> *body_ptr;
  if (talsh_tensor->getDataAccessHostConst (&body_ptr))
    {
      printf ("[\n");
      assert (std::sqrt (talsh_tensor->getVolume ()) == std::pow (2, qubits.size ()));
      if (qubits.size () < 5)
        {
          unsigned dim = std::sqrt (talsh_tensor->getVolume ());
          for (unsigned i = 0; i < talsh_tensor->getVolume (); ++i)
            {              
              if (i % dim == i / dim) // diagonal
                {
                  std::cout << "<" << body_ptr[i] << ">";
                }
              else
                {
                  std::cout << " " << body_ptr[i] << " ";
                }
              if ((i + 1) % dim == 0)
                {
                  printf ("\n");
                }
            }
        }
      else
        { // too long for readability
          printf ("...");
        }
      printf ("]\n");
    }

  // calculate <bell|rho|bell>
  exatn::TensorNetwork circuit_fidel = exatn::TensorNetwork ();
  circuit_fidel.rename (AllocExatnName ());
  circuit_fidel.appendTensor (1, exatn::getTensor (QNS_PREFIX + "BellSV"), {},
    {exatn::LegDirection::OUTWARD, exatn::LegDirection::OUTWARD}, false); // |bell>
  circuit_fidel.appendTensor (2, exatn::getTensor (QNS_PREFIX + "BellSV"), {}, 
    {exatn::LegDirection::INWARD, exatn::LegDirection::INWARD}, true); // <bell|
  circuit_fidel.appendTensor (3, exatn::getTensor (circuit_peek.getTensor (0)->getName ()), 
    {
      {circuit_fidel.getTensorConn (1)->getTensorLeg (0).getDimensionId (), 0},
      {circuit_fidel.getTensorConn (1)->getTensorLeg (1).getDimensionId (), 1},
      {circuit_fidel.getTensorConn (2)->getTensorLeg (0).getDimensionId (), 2},
      {circuit_fidel.getTensorConn (2)->getTensorLeg (1).getDimensionId (), 3}
    },
    {exatn::LegDirection::INWARD, exatn::LegDirection::INWARD, exatn::LegDirection::OUTWARD, exatn::LegDirection::OUTWARD},
    false); // rho  

  Evaluate (&circuit_fidel);
  
  // access data
  assert (circuit_fidel.getTensor (0));
  talsh_tensor = exatn::getLocalTensor (circuit_fidel.getTensor (0)->getName ());
  assert (talsh_tensor);
  assert (talsh_tensor->getVolume () == 1);
  if (talsh_tensor->getDataAccessHostConst (&body_ptr))
    {
      assert (abs ((*body_ptr).imag ()) < EPS);
      fidel = (*body_ptr).real ();
    }

  NS_LOG_INFO (CYAN_CODE << "=> The fidelity is " << fidel << END_CODE);

  return fidel;
}


/* util */

bool
QuantumNetworkSimulator::CheckValid (const std::vector<std::string> &qubits) const
{
  for (const std::string &qubit : qubits)
    {
      bool valid = false;
      for (const std::string &q : m_qubits_vld)
        {
          if (strcmp (qubit.c_str (), q.c_str ()) == 0)
            {
              valid = true;
              break;
            }
        }
      if (!valid)
        {
          NS_LOG_LOGIC (GREEN_CODE << "Skipping invalid qubit named " << qubit << END_CODE);
          return false;
        }
    }
  return true;
}

std::string
QuantumNetworkSimulator::AllocExatnName ()
{
  return QNS_EXATN_PREFIX + std::to_string (m_exatn_name_count++);
}



/* debug */
void
QuantumNetworkSimulator::PrintQubitsValid () const
{
  NS_LOG_LOGIC ("Valid qubits:");
  for (const std::string &qubit : m_qubits_vld)
    {
      NS_LOG_LOGIC (qubit);
    }
  NS_LOG_LOGIC (END_CODE);
}

void
QuantumNetworkSimulator::PrintTalshTensor (std::shared_ptr<talsh::Tensor> talsh_tensor,
                                           const std::complex<double> *body_ptr)
{
  if (talsh_tensor->getDataAccessHostConst (&body_ptr))
    {
      printf ("[");
      for (size_t i = 0; i < talsh_tensor->getVolume (); ++i)
        {
          std::cout << "< " << body_ptr[i] << " >";
        }
      printf ("]\n");
    }
}

void
QuantumNetworkSimulator::PrintTalshTensorNamed (const std::string &name)
{
  exatn::getTensor (name)->printIt ();
  std::cout << "  <= shape, data=> ";
  auto talsh_tensor = exatn::getLocalTensor (name);
  assert (talsh_tensor);
  const std::complex<double> *body_ptr = nullptr;
  PrintTalshTensor (talsh_tensor, body_ptr);
}

} // namespace ns3