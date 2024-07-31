import numpy as np
import netsquid as ns
import pydynaa as pd

from netsquid.components import ClassicalChannel, QuantumChannel, PhysicalInstruction
from netsquid.util.simtools import sim_time
from netsquid.util.datacollector import DataCollector
from netsquid.qubits import operators as ops
from netsquid.qubits import qubitapi as qapi
from netsquid.protocols.nodeprotocols import NodeProtocol, LocalProtocol
from netsquid.protocols.protocol import Signals
from netsquid.nodes.node import Node
from netsquid.nodes.network import Network
from entanglenodes import EntangleNodes
from netsquid.components.instructions import INSTR_MEASURE, INSTR_CNOT, IGate, INSTR_SWAP
from netsquid.components.component import Message, Port
from netsquid.components.qsource import QSource, SourceStatus
from netsquid.components.qprocessor import QuantumProcessor
from netsquid.components.qprogram import QuantumProgram
from netsquid.qubits import ketstates as ks
from netsquid.qubits.state_sampler import StateSampler
from netsquid.components.models.delaymodels import FixedDelayModel
from netsquid.components.models import DepolarNoiseModel, DephaseNoiseModel
from netsquid.nodes.connections import DirectConnection
from pydynaa import EventExpression

layers = 3
num_positions = 2 ** (layers + 1)

success = True
num_successes = 0
fidel_squared_sum = 0


class DistribAndDistil(NodeProtocol):
    # set basis change operators for local DEJMPS step
    _INSTR_Rx = IGate("Rx_gate", ops.create_rotation_op(np.pi / 2, (1, 0, 0)))
    _INSTR_RxC = IGate("RxC_gate", ops.create_rotation_op(np.pi / 2, (1, 0, 0), conjugate=True))

    def __init__(self, node, port, role, q_goal, q_meas, start_expression=None, msg_header="distrib_and_distil", name=None):
        if role.upper() not in ["A", "B"]:
            raise ValueError
        conj_rotation = role.upper() == "B"
        if not isinstance(port, Port):
            raise ValueError("{} is not a Port".format(port))
        name = name if name else "DistribAndDistilNode({}, {})".format(node.name, port.name)
        super().__init__(node, name=name)
        self.port = port
        self.start_expression = start_expression
        self.q_goal = q_goal
        self.q_meas = q_meas
        self._program = self._setup_dejmp_program(conj_rotation)
        self.local_qcount = 0
        self.local_meas_result = None
        self.remote_qcount = 0
        self.remote_meas_result = None
        self.header = msg_header
        self._qmem_positions = [None, None]
        self._waiting_on_second_qubit = False
        if start_expression is not None and not isinstance(start_expression, EventExpression):
            raise TypeError("Start expression should be a {}, not a {}".format(EventExpression, type(start_expression)))

    def _setup_dejmp_program(self, conj_rotation):
        INSTR_ROT = self._INSTR_Rx if not conj_rotation else self._INSTR_RxC
        prog = QuantumProgram(num_qubits=2)
        q1, q2 = prog.get_qubit_indices(2)
        prog.apply(INSTR_ROT, [q1])
        prog.apply(INSTR_ROT, [q2])
        prog.apply(INSTR_CNOT, [q1, q2])
        prog.apply(INSTR_MEASURE, q2, output_key="m", inplace=False)
        return prog

    def run(self):
        cchannel_ready = self.await_port_input(self.port)
        qmemory_ready = self.start_expression
        while True:
            self.send_signal(Signals.WAITING)
            expr = yield cchannel_ready | qmemory_ready
            self.send_signal(Signals.BUSY)
            if expr.first_term.value:
                classical_message = self.port.rx_input(header=self.header)
                if classical_message:
                    self.remote_qcount, self.remote_meas_result = classical_message.items
            elif expr.second_term.value:
                source_protocol = expr.second_term.atomic_source  # qmemory ready
                ready_signal = source_protocol.get_signal_by_event(
                    event=expr.second_term.triggered_events[0], receiver=self)
                yield from self._handle_new_qubit(ready_signal.result)  # mem_pos
            self._check_success()

    def start(self):
        # Clear any held qubits
        self._clear_qmem_positions()
        self.local_qcount = 0
        self.local_meas_result = None
        self.remote_qcount = 0
        self.remote_meas_result = None
        self._waiting_on_second_qubit = False
        return super().start()

    def _clear_qmem_positions(self):
        positions = [pos for pos in self._qmem_positions if pos is not None]
        if len(positions) > 0:
            self.node.qmemory.pop(positions=positions)
        self._qmem_positions = [None, None]

    def _handle_new_qubit(self, memory_position):
        # Process signalling of new entangled qubit
        assert not self.node.qmemory.mem_positions[memory_position].is_empty
        if self._waiting_on_second_qubit:
            self.node.qmemory.execute_instruction(INSTR_SWAP, [memory_position, self.q_meas])
            memory_position = self.q_meas
            # Perform DEJMPS
            assert not self.node.qmemory.mem_positions[self._qmem_positions[0]].is_empty
            assert memory_position != self._qmem_positions[0]
            self._qmem_positions[1] = memory_position
            self._waiting_on_second_qubit = False
            yield from self._node_do_DEJMPS()
        else:
            pop_positions = [p for p in self._qmem_positions if p is not None and p != memory_position]
            if len(pop_positions) > 0:
                self.node.qmemory.pop(positions=pop_positions)
            self.node.qmemory.execute_instruction(INSTR_SWAP, [memory_position, self.q_goal])
            memory_position = self.q_goal
            # Set new position:
            self._qmem_positions[0] = memory_position
            self._qmem_positions[1] = None
            self.local_qcount += 1
            self.local_meas_result = None
            self._waiting_on_second_qubit = True

    def _node_do_DEJMPS(self):
        pos1, pos2 = self._qmem_positions
        assert pos1 == self.q_goal
        assert pos2 == self.q_meas
        if self.node.qmemory.busy:
            yield self.await_program(self.node.qmemory)
        # We perform local DEJMPS
        yield self.node.qmemory.execute_program(self._program, qubit_mapping=[pos1, pos2])
        self.local_meas_result = self._program.output["m"][0]
        self._qmem_positions[1] = None
        self.port.tx_output(Message([self.local_qcount, self.local_meas_result],
                                    header=self.header))

    def print_qmem(self):
        print(f"Node {self.node.name} qmemory", flush=True)
        for i in range(self.node.qmemory.num_positions):
            print(f"    {i} - {self.node.qmemory.peek(i)}", flush=True)


    def _check_success(self):
        global success
        # Check if distillation succeeded by comparing local and remote results
        if (self.local_qcount == self.remote_qcount and
                self.local_meas_result is not None and
                self.remote_meas_result is not None):
            success = success and (self.local_meas_result == self.remote_meas_result)
            if self.local_meas_result == self.remote_meas_result:
                # SUCCESS
                self.send_signal(Signals.SUCCESS, self._qmem_positions[0])
            else:
                # FAILURE
                self._clear_qmem_positions()
                self.send_signal(Signals.FAIL, self.local_qcount)
            self.local_meas_result = None
            self.remote_meas_result = None
            self._qmem_positions = [None, None]

    @property
    def is_connected(self):
        if self.start_expression is None:
            return False
        if not self.check_assigned(self.port, Port):
            return False
        if not self.check_assigned(self.node, Node):
            return False
        if self.node.qmemory.num_positions < 2:
            return False
        return True

class Distil(NodeProtocol):
    # set basis change operators for local DEJMPS step
    _INSTR_Rx = IGate("Rx_gate", ops.create_rotation_op(np.pi / 2, (1, 0, 0)))
    _INSTR_RxC = IGate("RxC_gate", ops.create_rotation_op(np.pi / 2, (1, 0, 0), conjugate=True))

    def __init__(self, node, port, role, q_goal, q_meas, 
                 start_expression=None, msg_header="distil", name=None):
        if role.upper() not in ["A", "B"]:
            raise ValueError
        conj_rotation = role.upper() == "B"
        if not isinstance(port, Port):
            raise ValueError("{} is not a Port".format(port))
        name = name if name else "DistilNode({}, {})".format(node.name, port.name)
        super().__init__(node, name=name)
        self.port = port
        self.start_expression = start_expression
        self.q_goal = q_goal
        self.q_meas = q_meas
        self._program = self._setup_dejmp_program(conj_rotation)
        self.local_qcount = 2
        self.local_meas_result = None
        self.remote_qcount = 2
        self.remote_meas_result = None
        self.header = msg_header
        self._qmem_positions = [None, None]
        self._waiting_on_second_qubit = False
        if start_expression is not None and not isinstance(start_expression, EventExpression):
            raise TypeError("Start expression should be a {}, not a {}".format(EventExpression, type(start_expression)))

    def _setup_dejmp_program(self, conj_rotation):
        INSTR_ROT = self._INSTR_Rx if not conj_rotation else self._INSTR_RxC
        prog = QuantumProgram(num_qubits=2)
        q1, q2 = prog.get_qubit_indices(2)
        prog.apply(INSTR_ROT, [q1])
        prog.apply(INSTR_ROT, [q2])
        prog.apply(INSTR_CNOT, [q1, q2])
        prog.apply(INSTR_MEASURE, q2, output_key="m", inplace=False)
        return prog

    def run(self):
        cchannel_ready = self.await_port_input(self.port)
        qmemory_ready = self.start_expression  # distilled
        while True:
            self.send_signal(Signals.WAITING)
            expr = yield cchannel_ready | qmemory_ready
            self.send_signal(Signals.BUSY)
            if expr.first_term.value:
                classical_message = self.port.rx_input(header=self.header)
                if classical_message:
                    self.remote_qcount, self.remote_meas_result = classical_message.items
            elif expr.second_term.value:
                self._qmem_positions[0] = self.q_goal
                self._qmem_positions[1] = self.q_meas
                self.local_meas_result = None            
                yield from self._node_do_DEJMPS()
            self._check_success()


    def start(self):
        self.local_qcount = 0
        self.local_meas_result = None
        self.remote_qcount = 0
        self.remote_meas_result = None
        self._waiting_on_second_qubit = False
        return super().start()

    def _node_do_DEJMPS(self):
        pos1, pos2 = self._qmem_positions
        assert pos1 == self.q_goal
        assert pos2 == self.q_meas
        pos1, pos2 = self.q_goal, self.q_meas
        if self.node.qmemory.busy:
            yield self.await_program(self.node.qmemory)
        # We perform local DEJMPS
        yield self.node.qmemory.execute_program(self._program, qubit_mapping=[pos1, pos2])
        self.local_meas_result = self._program.output["m"][0]
        self._qmem_positions[1] = None
        self.port.tx_output(Message([self.local_qcount, self.local_meas_result],
                                    header=self.header))

    def print_qmem(self):
        print(f"Node {self.node.name} qmemory", flush=True)
        for i in range(self.node.qmemory.num_positions):
            print(f"    {i} - {self.node.qmemory.peek(i)}", flush=True)


    def _check_success(self):
        global success
        # Check if distillation succeeded by comparing local and remote results
        if (self.local_qcount == self.remote_qcount and
                self.local_meas_result is not None and
                self.remote_meas_result is not None):
            success = success and self.local_meas_result == self.remote_meas_result
            if self.local_meas_result == self.remote_meas_result:
                # SUCCESS
                self.send_signal(Signals.SUCCESS, self._qmem_positions[0])
            else:
                # FAILURE
                self.send_signal(Signals.FAIL, self.local_qcount)
            self.local_meas_result = None
            self.remote_meas_result = None
            self._qmem_positions = [None, None]

    @property
    def is_connected(self):
        if self.start_expression is None:
            return False
        if not self.check_assigned(self.port, Port):
            return False
        if not self.check_assigned(self.node, Node):
            return False
        if self.node.qmemory.num_positions < 2:
            return False
        return True

class DistilNestedExample(LocalProtocol):
    def __init__(self, node_a, node_b, num_runs=1):
        super().__init__(nodes={"A": node_a, "B": node_b}, name="Distillation nested example")
        self.num_runs = num_runs
        # Initialise sub-protocols
        self.distil_count = 0
        blocks = 2 ** (layers - 1)
        self.add_subprotocol(EntangleNodes(node=node_a, role="source", input_mem_pos=num_positions - 2,
                                        num_pairs=2, name="entangle_A_" + str(self.distil_count)))
        self.add_subprotocol(EntangleNodes(node=node_b, role="receiver", input_mem_pos=num_positions - 2, 
                                        num_pairs=2, name="entangle_B_" + str(self.distil_count)))
        self.add_subprotocol(DistribAndDistil(
                                node_a, node_a.get_conn_port(node_b.ID), role="A",
                                q_goal=0, q_meas=1,
                                name="distil_A_" + str(self.distil_count)
                            ))
        self.add_subprotocol(DistribAndDistil(
                                node_b, node_b.get_conn_port(node_a.ID), role="B",
                                q_goal=0, q_meas=1,
                                name="distil_B_" + str(self.distil_count)
                            ))
        # Set start expressions
        self.subprotocols["distil_A_" + str(self.distil_count)].start_expression = (
            self.subprotocols["distil_A_" + str(self.distil_count)].await_signal(
                self.subprotocols["entangle_A_" + str(self.distil_count)], Signals.SUCCESS
            )
        )  # distil_A_0 awaits entangle_A_0
        self.subprotocols["distil_B_" + str(self.distil_count)].start_expression = (
            self.subprotocols["distil_B_" + str(self.distil_count)].await_signal(
                self.subprotocols["entangle_B_" + str(self.distil_count)], Signals.SUCCESS
            )
        )  # distil_B_0 awaits entangle_B_0
        self.distil_count += 1

        for block in range(1, blocks):
            self.add_subprotocol(EntangleNodes(node=node_a, role="source", input_mem_pos=num_positions - 2,
                                 num_pairs=2, name="entangle_A_" + str(self.distil_count)))
            self.add_subprotocol(EntangleNodes(node=node_b, role="receiver", input_mem_pos=num_positions - 2, 
                                 num_pairs=2, name="entangle_B_" + str(self.distil_count)))
            self.add_subprotocol(DistribAndDistil(
                                 node_a, node_a.get_conn_port(node_b.ID), role="A",
                                 q_goal=2*block, q_meas=2*block + 1,
                                 msg_header="distrib_and_distil_" + str(self.distil_count),
                                 name="distil_A_" + str(self.distil_count)
                                ))
            self.add_subprotocol(DistribAndDistil(
                                 node_b, node_b.get_conn_port(node_a.ID), role="B",
                                 q_goal=2*block, q_meas=2*block + 1,
                                 msg_header="distrib_and_distil_" + str(self.distil_count),
                                 name="distil_B_" + str(self.distil_count)
                                ))
            # Set start expressions
            self.subprotocols["entangle_A_" + str(self.distil_count)].start_expression = (
                self.subprotocols["entangle_A_" + str(self.distil_count)].await_signal(
                    self.subprotocols["distil_A_" + str(self.distil_count - 1)], Signals.SUCCESS
                )
            )  # entangle_A_1 awaits distil_A_0
            self.subprotocols["entangle_B_" + str(self.distil_count)].start_expression = (
                self.subprotocols["entangle_B_" + str(self.distil_count)].await_signal(
                    self.subprotocols["distil_B_" + str(self.distil_count - 1)], Signals.SUCCESS
                )
            )  # entangle_B_1 awaits distil_B_0
            self.subprotocols["distil_A_" + str(self.distil_count)].start_expression = (
                self.subprotocols["distil_A_" + str(self.distil_count)].await_signal(
                    self.subprotocols["entangle_A_" + str(self.distil_count)], Signals.SUCCESS
                )  # distil_A_1 awaits entangle_A_1
            )
            self.subprotocols["distil_B_" + str(self.distil_count)].start_expression = (
                self.subprotocols["distil_B_" + str(self.distil_count)].await_signal(
                    self.subprotocols["entangle_B_" + str(self.distil_count)], Signals.SUCCESS
                )  # distil_B_1 awaits entangle_B_1
            )
            self.distil_count += 1

        # layer = 1, ..., layers - 1
        for layer in range(1, layers):
            blocks = 2 ** (layers - 1 - layer)
            for block in range(blocks):
                span = 2 ** layer
                dist = 2 * span 
                q_goal = block * dist
                q_meas = q_goal + span
                self.add_subprotocol(Distil(node_a, node_a.get_conn_port(node_b.ID), role="A",
                                            q_goal=q_goal, q_meas=q_meas,
                                            msg_header="distil_" + str(self.distil_count),
                                            name="distil_A_" + str(self.distil_count)))
                self.add_subprotocol(Distil(node_b, node_b.get_conn_port(node_a.ID), role="B",
                                            q_goal=q_goal, q_meas=q_meas,
                                            msg_header="distil_" + str(self.distil_count),
                                            name="distil_B_" + str(self.distil_count)))
                # Set start expressions
                self.subprotocols["distil_A_" + str(self.distil_count)].start_expression = (
                    self.subprotocols["distil_A_" + str(self.distil_count)].await_signal(
                        self.subprotocols["distil_A_" + str(self.distil_count - 1)], Signals.SUCCESS
                    )  # distil_A_2 awaits distil_A_1
                )
                self.subprotocols["distil_B_" + str(self.distil_count)].start_expression = (
                    self.subprotocols["distil_B_" + str(self.distil_count)].await_signal(
                        self.subprotocols["distil_B_" + str(self.distil_count - 1)], Signals.SUCCESS
                    )  # distil_B_2 awaits distil_B_1
                )
                self.distil_count += 1

    def run(self):
        global success
        self.start_subprotocols()
        for i in range(self.num_runs):
            start_time = sim_time()
            self.send_signal(Signals.WAITING)
            yield (self.await_signal(self.subprotocols["distil_A_" + str(self.distil_count - 1)], Signals.SUCCESS) &
                   self.await_signal(self.subprotocols["distil_B_" + str(self.distil_count - 1)], Signals.SUCCESS))
            signal_A = self.subprotocols["distil_A_" + str(self.distil_count - 1)].get_signal_result(Signals.SUCCESS,
                                                                       self)
            signal_B = self.subprotocols["distil_B_" + str(self.distil_count - 1)].get_signal_result(Signals.SUCCESS,
                                                                       self)
            result = {
                "pos_A": signal_A,
                "pos_B": signal_B,
                "time": sim_time() - start_time,
            }
            if success:
                self.send_signal(Signals.SUCCESS, result)
            else:
                self.send_signal(Signals.FAIL, result)
            self.distil_count = 0
            success = True


def example_network_setup(node_name_a, node_name_b, source_delay=1e5,
                          source_fidelity_sq=1.0, node_distance=20):
    network = Network("distil_network")

    node_a, node_b = network.add_nodes(["node_" + node_name_a, "node_" + node_name_b])
    physical_instructions = [
        PhysicalInstruction(INSTR_CNOT, duration=1e5, parallel=True),
        PhysicalInstruction(INSTR_SWAP, duration=1e5, parallel=True),
        PhysicalInstruction(INSTR_MEASURE, duration=1e5, parallel=True),
    ]
    node_a.add_subcomponent(QuantumProcessor(
            "QuantumMemory_" + node_name_a, num_positions=num_positions, fallback_to_nonphysical=True,
            memory_noise_models=DephaseNoiseModel(dephase_rate=20, time_independent=False),
            physical_instructions=physical_instructions
        )
    )
    
    state_sampler = StateSampler(
        [ks.b01, ks.s00],
        probabilities=[source_fidelity_sq, 1 - source_fidelity_sq])
    node_a.add_subcomponent(QSource(
        "QSource_" + node_name_a, state_sampler=state_sampler,
        models={"emission_delay_model": FixedDelayModel(delay=source_delay)},
        num_ports=2, status=SourceStatus.EXTERNAL))
    node_b.add_subcomponent(QuantumProcessor(
            "QuantumMemory_" + node_name_b, num_positions=num_positions, fallback_to_nonphysical=True,
            memory_noise_models=DephaseNoiseModel(dephase_rate=20, time_independent=False),
            physical_instructions=physical_instructions
        )
    )
    conn_cchannel = DirectConnection(
        "CChannelConn_" + node_name_a + node_name_b,
        ClassicalChannel("CChannel_" + node_name_a + "->" + node_name_b, length=node_distance,
                         models={"delay_model": FixedDelayModel(delay=1e5)}),
        ClassicalChannel("CChannel_" + node_name_b + "->" + node_name_a, length=node_distance,
                         models={"delay_model": FixedDelayModel(delay=1e5)}))
    network.add_connection(node_a, node_b, connection=conn_cchannel)
    qchannel = QuantumChannel("QChannel_" + node_name_a + "->" + node_name_b, length=node_distance,
                              models={"quantum_noise_model": DepolarNoiseModel(depolar_rate=0.05, time_independent=True),
                                      "delay_model": FixedDelayModel(delay=1e5)})
    port_name_a, port_name_b = network.add_connection(
        node_a, node_b, channel_to=qchannel, label="quantum")
    # Link Alice ports:
    node_a.subcomponents["QSource_" + node_name_a].ports["qout1"].forward_output(
        node_a.ports[port_name_a])  # send the second qubit to Bob
    node_a.subcomponents["QSource_" + node_name_a].ports["qout0"].connect(
        node_a.qmemory.ports["qin" + str(num_positions - 2)])  # pass the first qubit to the memory
    # Link Bob ports:
    node_b.ports[port_name_b].forward_input(node_b.qmemory.ports["qin" + str(num_positions - 2)])  # save the qubit in the memory
    return network


def example_sim_setup(node_a, node_b, num_runs):
    dist_example = DistilNestedExample(node_a, node_b, num_runs=num_runs)

    def record_run(evexpr):
        # Callback that collects data each run
        protocol = evexpr.triggered_events[-1].source
        result = protocol.get_signal_result(Signals.SUCCESS)
        # Record fidelity
        q_A, = node_a.qmemory.pop(positions=[result["pos_A"]])
        q_B, = node_b.qmemory.pop(positions=[result["pos_B"]])
        f2 = qapi.fidelity([q_A, q_B], ks.b01, squared=True)
        return {"F2": f2, "time": result["time"]}

    dc = DataCollector(record_run, include_time_stamp=False,
                       include_entity_name=False)
    dc.collect_on(pd.EventExpression(source=dist_example,
                                     event_type=Signals.SUCCESS.value))
    return dist_example, dc


def run_simulation(layers_, num_iters=200):
    global layers, num_positions, success, num_successes, fidel_squared_sum
    layers = layers_    
    num_positions = 2 ** (layers + 1)
    node_name_a = "A"
    node_name_b = "B"
    for _ in range(num_iters):
        network = example_network_setup(node_name_a=node_name_a, node_name_b=node_name_b)
        dist_example, dc = example_sim_setup(network.get_node("node_" + node_name_a),
                                             network.get_node("node_" + node_name_b))
        dist_example.start()
        ns.sim_run()
        if success:
            fidel_squared = dc.dataframe["F2"].mean()
            fidel_squared_sum += fidel_squared
            num_successes += 1
        success = True

    num_successes = 0
    fidel_squared_sum = 0