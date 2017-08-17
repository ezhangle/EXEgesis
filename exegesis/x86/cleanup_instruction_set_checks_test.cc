// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "exegesis/x86/cleanup_instruction_set_checks.h"

#include "exegesis/testing/test_util.h"
#include "exegesis/util/proto_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "util/task/status.h"

namespace exegesis {
namespace x86 {
namespace {

using ::exegesis::testing::EqualsProto;
using ::exegesis::util::Status;
using ::exegesis::util::error::INVALID_ARGUMENT;
using ::testing::HasSubstr;

TEST(CheckOpcodeFormatTest, ValidOpcodes) {
  constexpr char kInstructionSetProto[] = R"(
      instructions {
        vendor_syntax {
          mnemonic: "ADC"
          operands {
            name: "AL"
          }
          operands {
            name: "imm8"
          }
        }
        raw_encoding_specification: "14 ib"
        x86_encoding_specification {
          opcode: 0x14
          legacy_prefixes {
          }
          immediate_value_bytes: 1
        }
      }
      instructions {
        vendor_syntax {
          mnemonic: 'VFMSUB231PS'
          operands {
            name: 'xmm0'
          }
          operands {
            name: 'xmm1'
          }
          operands {
            name: 'm128'
          }
        }
        raw_encoding_specification: 'VEX.DDS.128.66.0F38.W0 BA /r'
        x86_encoding_specification {
          opcode: 997562
          modrm_usage: FULL_MODRM
          vex_prefix {
            prefix_type: VEX_PREFIX
            vex_operand_usage: VEX_OPERAND_IS_SECOND_SOURCE_REGISTER
            vector_size: VEX_VECTOR_SIZE_128_BIT
            mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
            map_select: MAP_SELECT_0F38
            vex_w_usage: VEX_W_IS_ZERO
          }
        }
      }
      instructions {
        vendor_syntax {
          mnemonic: 'PMOVSXBW'
          operands {
            name: 'xmm1'
          }
          operands {
            name: 'xmm2'
          }
        }
        raw_encoding_specification: '66 0F 38 20 /r'
        x86_encoding_specification {
          opcode: 997408
          modrm_usage: FULL_MODRM
          legacy_prefixes {
            has_mandatory_operand_size_override_prefix: true
          }
        }
      }
      instructions {
        vendor_syntax {
          mnemonic: "KSHIFTLB"
          operands {
            name: "k1"
          }
          operands {
            name: "k2"
          }
          operands {
            name: "imm8"
          }
        }
        raw_encoding_specification: "VEX.L0.66.0F3A.W0 32 /r ib"
        x86_encoding_specification {
          opcode: 0x0f3a32
          modrm_usage: FULL_MODRM
          vex_prefix {
            prefix_type: VEX_PREFIX
            vector_size: VEX_VECTOR_SIZE_BIT_IS_ZERO
            mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
            map_select: MAP_SELECT_0F3A
            vex_w_usage: VEX_W_IS_ZERO
          }
          immediate_value_bytes: 1
        }
      })";
  InstructionSetProto instruction_set =
      ParseProtoFromStringOrDie<InstructionSetProto>(kInstructionSetProto);
  EXPECT_OK(CheckOpcodeFormat(&instruction_set));
  EXPECT_THAT(instruction_set, EqualsProto(kInstructionSetProto));
}

TEST(CheckOpcodeFormatTest, InvalidOpcodes) {
  static constexpr struct {
    const char* instruction_set;
    const char* expected_error_message;
  } kTestCases[] =  //
      {{R"(instructions {
             vendor_syntax {
               mnemonic: 'FSUB'
               operands {
                 name: 'ST(i)'
               }
             }
             raw_encoding_specification: 'D8 E0+i'
             x86_encoding_specification {
               opcode: 55520
               operand_in_opcode: FP_STACK_REGISTER_IN_OPCODE
               legacy_prefixes {
               }
             }
           })",
        "Invalid opcode upper bytes: d800"},
       {R"(instructions {
             vendor_syntax {
               mnemonic: "XTEST"
             }
             raw_encoding_specification: "NP 0F 01 D6"
             x86_encoding_specification {
               opcode: 983510
               legacy_prefixes {
               }
             }
           })",
        "Invalid opcode upper bytes: f0100"}};
  for (const auto& test_case : kTestCases) {
    InstructionSetProto instruction_set =
        ParseProtoFromStringOrDie<InstructionSetProto>(
            test_case.instruction_set);
    const Status status = CheckOpcodeFormat(&instruction_set);
    EXPECT_EQ(status.error_code(), INVALID_ARGUMENT);
    EXPECT_THAT(status.error_message(),
                HasSubstr(test_case.expected_error_message));
  }
}

}  // namespace
}  // namespace x86
}  // namespace exegesis