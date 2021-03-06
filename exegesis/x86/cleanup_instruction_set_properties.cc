// Copyright 2016 Google Inc.
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

#include "exegesis/x86/cleanup_instruction_set_properties.h"

#include <string>
#include <unordered_map>

#include "exegesis/base/cleanup_instruction_set.h"
#include "glog/logging.h"
#include "util/gtl/map_util.h"

namespace exegesis {
namespace x86 {
namespace {

using ::exegesis::util::OkStatus;
using ::exegesis::util::Status;

const std::unordered_map<std::string, std::string>& GetMissingCpuFlags() {
  static const std::unordered_map<std::string, std::string>* const
      kMissingFlags = new std::unordered_map<std::string, std::string>(
          {{"CLFLUSH", "CLFSH"},
           {"CLFLUSHOPT", "CLFLUSHOPT"},
           {"MOVBE", "MOVBE"}});
  return *kMissingFlags;
}

}  // namespace

Status AddMissingCpuFlags(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  for (auto& instruction : *instruction_set->mutable_instructions()) {
    const std::string* const feature_name = FindOrNull(
        GetMissingCpuFlags(), instruction.vendor_syntax().mnemonic());
    if (feature_name) {
      // Be warned if they fix it someday. If this triggers, just remove the
      // rule.
      CHECK_NE(*feature_name, instruction.feature_name())
          << instruction.vendor_syntax().mnemonic();
      instruction.set_feature_name(*feature_name);
    }
  }
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(AddMissingCpuFlags, 1000);

namespace {

// Returns the list of protection modes for priviledged instructions.
const std::unordered_map<std::string, int>& GetProtectionModesByMnemonic() {
  static const std::unordered_map<std::string, int>* const kProtectionModes =
      new std::unordered_map<std::string, int>({
          // -----------------------
          // Restricted operations.
          {"CLAC", 0},
          {"CLI", 0},
          {"CLTS", 0},
          {"HLT", 0},
          {"INVD", 0},
          {"INVPCID", 0},
          {"LGDT", 0},
          {"LIDT", 0},
          {"LLDT", 0},
          {"LMSW", 0},
          {"LTR", 0},
          {"MWAIT", 0},
          // The instruction is not marked as priviledged in its doc, but SWAPGR
          // later states that "The IA32_KERNEL_GS_BASE MSR itself is only
          // accessible using RDMSR/WRMSR instructions. Those instructions are
          // only accessible at privilege level 0."
          {"RDMSR", 0},
          {"RDPMC", 0},
          {"STAC", 0},
          {"STD", 0},  // Not 100% sure, it looks like the SDM is wrong.
          {"STI", 0},
          {"SWAPGR", 0},
          {"SWAPGS", 0},
          {"WBINVD", 0},
          {"WRMSR", 0},
          {"XRSTORS", 0},
          {"XRSTORS64", 0},
          // -----------------------
          // Input/output.
          // For now assume the worst case: IOPL == 0.
          {"IN", 0},
          {"INS", 0},
          {"INSB", 0},
          {"INSW", 0},
          {"INSD", 0},
          {"OUT", 0},
          {"OUTS", 0},
          {"OUTSB", 0},
          {"OUTSD", 0},
          {"OUTSW", 0},
          // -----------------------
          // SMM mode.
          // For now assume that everything that needs to execute in SMM mode
          // requires CPL 0.
          {"RSM", 0},
      });
  return *kProtectionModes;
}

// Returns the list of protection modes for privileged instructions that are not
// covered by GetProtectionModesByMnemonic().
const std::unordered_map<std::string, int>& GetProtectionModesByEncoding() {
  static const std::unordered_map<std::string, int>* const kProtectionModes =
      new std::unordered_map<std::string, int>({
          // MOV from/to debug register.
          {"0F 21/r", 0},
          {"0F 23 /r", 0},
          // MOV from/to control registers.
          {"0F 20/r", 0},
          {"0F 22 /r", 0},
      });
  return *kProtectionModes;
}

}  // namespace

Status AddProtectionModes(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  for (auto& instruction : *instruction_set->mutable_instructions()) {
    const int* mode = FindOrNull(GetProtectionModesByMnemonic(),
                                 instruction.vendor_syntax().mnemonic());
    if (mode == nullptr) {
      mode = FindOrNull(GetProtectionModesByEncoding(),
                        instruction.raw_encoding_specification());
    }
    // Set default protection_mode to something negative to make sure
    // instruction is not marked as protected.
    instruction.set_protection_mode(-1);
    if (mode) {
      instruction.set_protection_mode(*mode);
    }
  }
  return OkStatus();
}
REGISTER_INSTRUCTION_SET_TRANSFORM(AddProtectionModes, 1000);

}  // namespace x86
}  // namespace exegesis
