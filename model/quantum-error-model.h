#ifndef QUANTUM_ERROR_MODEL_H
#define QUANTUM_ERROR_MODEL_H

#include "ns3/object.h"
#include "ns3/quantum-basis.h"

namespace ns3 {

class QuantumPhyEntity;


class QuantumErrorModel : public Object
{

protected:
  
  /** Whether the error model is time-dependent. */
  bool m_time_dep;

  /** Type description of the error model. */
  std::string m_type_desc;

public:

  QuantumErrorModel (bool time_dependent_);

  QuantumErrorModel ();
  void DoDispose (void) override;
  static TypeId GetTypeId (void);

  std::string GetTypeDesc () const;

  /** 
   * \brief Apply the error model to some qubits in some quantum circuit.
   * \param qphyent Quantum physical entity encapsulating the quantum circuit.
   * \param qubits Names of the qubits to be applied the error model.
   * \param moment Time when the error model is applied.
  */
  virtual void ApplyErrorModel (Ptr<QuantumPhyEntity> qphyent, const std::vector<std::string> &qubits,
                                const Time &moment) const = 0;
};



class DephaseModel : public QuantumErrorModel
{

private:

  /** Dephase rate. */
  const double m_rate;

public:

  DephaseModel (double rate_);

  DephaseModel ();
  void DoDispose (void) override;
  static TypeId GetTypeId (void);

  /**
   * \brief Apply the dephasing model to some qubits in some quantum circuit.
   * \param qphyent Quantum physical entity encapsulating the quantum circuit.
   * \param qubits Names of the qubits to be applied the dephasing model.
   * \param moment Time when the dephasing model is applied.
  */
  void ApplyErrorModel (Ptr<QuantumPhyEntity> qphyent, const std::vector<std::string> &qubits,
                        const Time &moment) const override;
};

/**
 * \brief Default dephasing model with rate 0.001.
 * \note If user does not specify a dephasing model, this model will be used.
*/
const DephaseModel default_dephase_model = DephaseModel (1.);



class DepolarModel : public QuantumErrorModel
{

private:
  
  /** Outcoming fidelity of the EPR pairs distributed through a channel. */
  const double m_fidel;

public:

  DepolarModel (double fidel_);

  DepolarModel ();
  void DoDispose (void) override;
  static TypeId GetTypeId (void);

  /**
   * \brief Apply the depolarizing model to a latter qubit in a EPR pair in a quantum circuit.
   * \param qphyent Quantum physical entity encapsulating the quantum circuit.
   * \param qubits Names of the qubit to be applied the depolarizing model.
   * \param moment Time when the depolarizing model is applied.
   * 
   * \note The qubits size should be 1.
  */
  void ApplyErrorModel (Ptr<QuantumPhyEntity> qphyent, const std::vector<std::string> &qubits,
                        const Time &moment) const override;
  double GetFidelity () const;
};

/** 
 * \brief Default depolarizing model with outcoming fidelity 0.95.
 * \note If user does not specify a depolarizing model, this model will be used.
*/
const DepolarModel default_depolar_model = DepolarModel (0.95);



class TimeModel : public QuantumErrorModel
{

private:

  /** Dephase rate. */
  const double m_rate;

public:

  TimeModel (double rate_);

  TimeModel ();
  void DoDispose (void) override;
  static TypeId GetTypeId (void);

  /**
   * \brief Apply the time relevant dephasing model to some qubits in some quantum circuit.
   * \param qphyent Quantum physical entity encapsulating the quantum circuit.
   * \param qubits Names of the qubits to be applied the time relevant dephasing model.
   * \param moment Time when the model is applied.
  */
  void ApplyErrorModel (Ptr<QuantumPhyEntity> qphyent, const std::vector<std::string> &qubits,
                        const Time &moment) const override;
};

/**
 * \brief Default time relevant dephasing model.
 * \note If user does not specify a time relevant dephasing model, this model will be used.
*/
const TimeModel default_time_model = TimeModel (1.);

} // namespace ns3

#endif /* QUANTUM_ERROR_MODEL_H */