#pragma once

#include "barretenberg/vm/avm/trace/addressing_mode.hpp"
#include "barretenberg/vm/avm/trace/alu_trace.hpp"
#include "barretenberg/vm/avm/trace/binary_trace.hpp"
#include "barretenberg/vm/avm/trace/bytecode_trace.hpp"
#include "barretenberg/vm/avm/trace/common.hpp"
#include "barretenberg/vm/avm/trace/execution_hints.hpp"
#include "barretenberg/vm/avm/trace/gadgets/conversion_trace.hpp"
#include "barretenberg/vm/avm/trace/gadgets/ecc.hpp"
#include "barretenberg/vm/avm/trace/gadgets/keccak.hpp"
#include "barretenberg/vm/avm/trace/gadgets/poseidon2.hpp"
#include "barretenberg/vm/avm/trace/gadgets/range_check.hpp"
#include "barretenberg/vm/avm/trace/gadgets/sha256.hpp"
#include "barretenberg/vm/avm/trace/gadgets/slice_trace.hpp"
#include "barretenberg/vm/avm/trace/gas_trace.hpp"
#include "barretenberg/vm/avm/trace/kernel_trace.hpp"
#include "barretenberg/vm/avm/trace/mem_trace.hpp"
#include "barretenberg/vm/avm/trace/opcode.hpp"
#include "barretenberg/vm/constants.hpp"

namespace bb::avm_trace {

using Row = bb::AvmFullRow<bb::fr>;

// This is the internal context that we keep along the lifecycle of bytecode execution
// to iteratively build the whole trace. This is effectively performing witness generation.
// At the end of circuit building, mainTrace can be moved to AvmCircuitBuilder by calling
// AvmCircuitBuilder::set_trace(rows).
class AvmTraceBuilder {

  public:
    AvmTraceBuilder(VmPublicInputs public_inputs = {},
                    ExecutionHints execution_hints = {},
                    uint32_t side_effect_counter = 0,
                    std::vector<FF> calldata = {});

    uint32_t get_pc() const { return pc; }
    uint32_t get_l2_gas_left() const { return gas_trace_builder.get_l2_gas_left(); }
    uint32_t get_da_gas_left() const { return gas_trace_builder.get_da_gas_left(); }

    // Compute - Arithmetic
    void op_add(uint8_t indirect, uint32_t a_offset, uint32_t b_offset, uint32_t dst_offset);
    void op_sub(uint8_t indirect, uint32_t a_offset, uint32_t b_offset, uint32_t dst_offset);
    void op_mul(uint8_t indirect, uint32_t a_offset, uint32_t b_offset, uint32_t dst_offset);
    void op_div(uint8_t indirect, uint32_t a_offset, uint32_t b_offset, uint32_t dst_offset);
    void op_fdiv(uint8_t indirect, uint32_t a_offset, uint32_t b_offset, uint32_t dst_offset);

    // Compute - Comparators
    void op_eq(uint8_t indirect, uint32_t a_offset, uint32_t b_offset, uint32_t dst_offset);
    void op_lt(uint8_t indirect, uint32_t a_offset, uint32_t b_offset, uint32_t dst_offset);
    void op_lte(uint8_t indirect, uint32_t a_offset, uint32_t b_offset, uint32_t dst_offset);

    // Compute - Bitwise
    void op_and(uint8_t indirect, uint32_t a_offset, uint32_t b_offset, uint32_t dst_offset);
    void op_or(uint8_t indirect, uint32_t a_offset, uint32_t b_offset, uint32_t dst_offset);
    void op_xor(uint8_t indirect, uint32_t a_offset, uint32_t b_offset, uint32_t dst_offset);
    void op_not(uint8_t indirect, uint32_t a_offset, uint32_t dst_offset);
    void op_shl(uint8_t indirect, uint32_t a_offset, uint32_t b_offset, uint32_t dst_offset);
    void op_shr(uint8_t indirect, uint32_t a_offset, uint32_t b_offset, uint32_t dst_offset);

    // Compute - Type Conversions
    void op_cast(uint8_t indirect, uint32_t a_offset, uint32_t dst_offset, AvmMemoryTag dst_tag);

    // Execution Environment
    void op_get_env_var(uint8_t indirect, uint8_t env_var, uint32_t dst_offset);
    void op_address(uint8_t indirect, uint32_t dst_offset);
    void op_sender(uint8_t indirect, uint32_t dst_offset);
    void op_function_selector(uint8_t indirect, uint32_t dst_offset);
    void op_transaction_fee(uint8_t indirect, uint32_t dst_offset);
    void op_is_static_call(uint8_t indirect, uint32_t dst_offset);

    // Execution Environment - Globals
    void op_chain_id(uint8_t indirect, uint32_t dst_offset);
    void op_version(uint8_t indirect, uint32_t dst_offset);
    void op_block_number(uint8_t indirect, uint32_t dst_offset);
    void op_timestamp(uint8_t indirect, uint32_t dst_offset);
    void op_fee_per_l2_gas(uint8_t indirect, uint32_t dst_offset);
    void op_fee_per_da_gas(uint8_t indirect, uint32_t dst_offset);

    // Execution Environment - Calldata
    void op_calldata_copy(uint8_t indirect, uint32_t cd_offset_address, uint32_t copy_size_offset, uint32_t dst_offset);

    // Machine State - Gas
    void op_l2gasleft(uint8_t indirect, uint32_t dst_offset);
    void op_dagasleft(uint8_t indirect, uint32_t dst_offset);

    // Machine State - Internal Control Flow
    // TODO(8945): skip_gas boolean is temporary and should be removed once all fake rows are removed
    void op_jump(uint32_t jmp_dest, bool skip_gas = false);
    void op_jumpi(uint8_t indirect, uint32_t jmp_dest, uint32_t cond_offset);
    // TODO(md): this program counter MUST be an operand to the OPCODE.
    void op_internal_call(uint32_t jmp_dest);
    void op_internal_return();

    // Machine State - Memory
    // TODO(8945): skip_gas boolean is temporary and should be removed once all fake rows are removed
    void op_set(uint8_t indirect, FF val, uint32_t dst_offset, AvmMemoryTag in_tag, bool skip_gas = false);
    void op_mov(uint8_t indirect, uint32_t src_offset, uint32_t dst_offset);

    // World State
    void op_sload(uint8_t indirect, uint32_t slot_offset, uint32_t size, uint32_t dest_offset);
    void op_sstore(uint8_t indirect, uint32_t src_offset, uint32_t size, uint32_t slot_offset);
    void op_note_hash_exists(uint8_t indirect,
                             uint32_t note_hash_offset,
                             uint32_t leaf_index_offset,
                             uint32_t dest_offset);
    void op_emit_note_hash(uint8_t indirect, uint32_t note_hash_offset);
    void op_nullifier_exists(uint8_t indirect,
                             uint32_t nullifier_offset,
                             uint32_t address_offset,
                             uint32_t dest_offset);
    void op_emit_nullifier(uint8_t indirect, uint32_t nullifier_offset);
    void op_l1_to_l2_msg_exists(uint8_t indirect,
                                uint32_t log_offset,
                                uint32_t leaf_index_offset,
                                uint32_t dest_offset);
    void op_get_contract_instance(
        uint8_t indirect, uint8_t member_enum, uint16_t address_offset, uint16_t dst_offset, uint16_t exists_offset);

    // Accrued Substate
    void op_emit_unencrypted_log(uint8_t indirect, uint32_t log_offset, uint32_t log_size_offset);
    void op_emit_l2_to_l1_msg(uint8_t indirect, uint32_t recipient_offset, uint32_t content_offset);

    // Control Flow - Contract Calls
    void op_call(uint16_t indirect,
                 uint32_t gas_offset,
                 uint32_t addr_offset,
                 uint32_t args_offset,
                 uint32_t args_size,
                 uint32_t ret_offset,
                 uint32_t ret_size,
                 uint32_t success_offset,
                 uint32_t function_selector_offset);
    void op_static_call(uint16_t indirect,
                        uint32_t gas_offset,
                        uint32_t addr_offset,
                        uint32_t args_offset,
                        uint32_t args_size,
                        uint32_t ret_offset,
                        uint32_t ret_size,
                        uint32_t success_offset,
                        uint32_t function_selector_offset);
    std::vector<FF> op_return(uint8_t indirect, uint32_t ret_offset, uint32_t ret_size);
    // REVERT Opcode (that just call return under the hood for now)
    std::vector<FF> op_revert(uint8_t indirect, uint32_t ret_offset, uint32_t ret_size_offset);

    // Gadgets
    void op_poseidon2_permutation(uint8_t indirect, uint32_t input_offset, uint32_t output_offset);
    void op_ec_add(uint16_t indirect,
                   uint32_t lhs_x_offset,
                   uint32_t lhs_y_offset,
                   uint32_t lhs_is_inf_offset,
                   uint32_t rhs_x_offset,
                   uint32_t rhs_y_offset,
                   uint32_t rhs_is_inf_offset,
                   uint32_t output_offset);
    void op_variable_msm(uint8_t indirect,
                         uint32_t points_offset,
                         uint32_t scalars_offset,
                         uint32_t output_offset,
                         uint32_t point_length_offset);
    // Conversions
    void op_to_radix_be(uint8_t indirect,
                        uint32_t src_offset,
                        uint32_t dst_offset,
                        uint32_t radix_offset,
                        uint32_t num_limbs,
                        uint8_t output_bits);

    // Future Gadgets -- pending changes in noir
    void op_sha256_compression(uint8_t indirect, uint32_t output_offset, uint32_t state_offset, uint32_t inputs_offset);
    void op_keccakf1600(uint8_t indirect, uint32_t output_offset, uint32_t input_offset);

    std::vector<Row> finalize();
    void reset();

    // These are used for testing only.
    AvmTraceBuilder& set_range_check_required(bool required)
    {
        range_check_required = required;
        return *this;
    }
    AvmTraceBuilder& set_full_precomputed_tables(bool required)
    {
        full_precomputed_tables = required;
        return *this;
    }

    struct MemOp {
        bool is_indirect;
        uint32_t indirect_address;
        uint32_t direct_address;
        AvmMemoryTag tag;
        bool tag_match;
        FF val;
    };

  private:
    std::vector<Row> main_trace;

    std::vector<FF> calldata;
    std::vector<FF> returndata;
    // Side effect counter will increment when any state writing values are encountered.
    uint32_t side_effect_counter = 0;
    uint32_t external_call_counter = 0; // Incremented both by OpCode::CALL and OpCode::STATICCALL
    ExecutionHints execution_hints;

    // These exist due to testing only.
    bool range_check_required = true;
    bool full_precomputed_tables = true;

    AvmMemTraceBuilder mem_trace_builder;
    AvmAluTraceBuilder alu_trace_builder;
    AvmBinaryTraceBuilder bin_trace_builder;
    AvmKernelTraceBuilder kernel_trace_builder;
    AvmGasTraceBuilder gas_trace_builder;
    AvmConversionTraceBuilder conversion_trace_builder;
    AvmSha256TraceBuilder sha256_trace_builder;
    AvmPoseidon2TraceBuilder poseidon2_trace_builder;
    AvmKeccakTraceBuilder keccak_trace_builder;
    AvmEccTraceBuilder ecc_trace_builder;
    AvmSliceTraceBuilder slice_trace_builder;
    AvmRangeCheckBuilder range_check_builder;
    AvmBytecodeTraceBuilder bytecode_trace_builder;

    Row create_kernel_lookup_opcode(uint8_t indirect, uint32_t dst_offset, FF value, AvmMemoryTag w_tag);

    Row create_kernel_output_opcode(uint8_t indirect, uint32_t clk, uint32_t data_offset);

    Row create_kernel_output_opcode_with_metadata(uint8_t indirect,
                                                  uint32_t clk,
                                                  uint32_t data_offset,
                                                  AvmMemoryTag data_r_tag,
                                                  uint32_t metadata_offset,
                                                  AvmMemoryTag metadata_r_tag);

    Row create_kernel_output_opcode_with_set_metadata_output_from_hint(
        uint8_t indirect, uint32_t clk, uint32_t data_offset, uint32_t address_offset, uint32_t metadata_offset);

    Row create_kernel_output_opcode_for_leaf_index(uint32_t clk,
                                                   uint32_t data_offset,
                                                   uint32_t leaf_index,
                                                   uint32_t metadata_offset);

    Row create_kernel_output_opcode_with_set_value_from_hint(uint8_t indirect,
                                                             uint32_t clk,
                                                             uint32_t data_offset,
                                                             uint32_t metadata_offset);

    void constrain_external_call(OpCode opcode,
                                 uint16_t indirect,
                                 uint32_t gas_offset,
                                 uint32_t addr_offset,
                                 uint32_t args_offset,
                                 uint32_t args_size_offset,
                                 uint32_t ret_offset,
                                 uint32_t ret_size,
                                 uint32_t success_offset,
                                 [[maybe_unused]] uint32_t function_selector_offset);

    void execute_gasleft(EnvironmentVariable var, uint8_t indirect, uint32_t dst_offset);

    void finalise_mem_trace_lookup_counts();

    uint32_t pc = 0;
    uint32_t internal_return_ptr =
        0; // After a nested call, it should be initialized with MAX_SIZE_INTERNAL_STACK * call_ptr
    uint8_t call_ptr = 0;

    MemOp constrained_read_from_memory(uint8_t space_id,
                                       uint32_t clk,
                                       AddressWithMode addr,
                                       AvmMemoryTag read_tag,
                                       AvmMemoryTag write_tag,
                                       IntermRegister reg,
                                       AvmMemTraceBuilder::MemOpOwner mem_op_owner = AvmMemTraceBuilder::MAIN);
    MemOp constrained_write_to_memory(uint8_t space_id,
                                      uint32_t clk,
                                      AddressWithMode addr,
                                      FF const& value,
                                      AvmMemoryTag read_tag,
                                      AvmMemoryTag write_tag,
                                      IntermRegister reg,
                                      AvmMemTraceBuilder::MemOpOwner mem_op_owner = AvmMemTraceBuilder::MAIN);

    // TODO: remove these once everything is constrained.
    AvmMemoryTag unconstrained_get_memory_tag(AddressWithMode addr);
    FF unconstrained_read_from_memory(AddressWithMode addr);
    template <typename T> void read_slice_from_memory(AddressWithMode addr, size_t slice_len, std::vector<T>& slice);
    void write_to_memory(AddressWithMode addr, FF val, AvmMemoryTag w_tag);
    template <typename T> void write_slice_to_memory(AddressWithMode addr, AvmMemoryTag w_tag, const T& slice);
};

} // namespace bb::avm_trace
