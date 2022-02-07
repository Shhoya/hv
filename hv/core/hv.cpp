#include "hv.h"
#include "vcpu.h"

#include "../util/mm.h"

namespace hv {

static hypervisor* global_hypervisor = nullptr;

// virtualize the current system
bool start() {
  // hypervisor is already running -_-
  if (global_hypervisor)
    return false;

  global_hypervisor = alloc<hypervisor>();
  if (!global_hypervisor)
    return false;

  memset(global_hypervisor, 0, sizeof(hypervisor));

  global_hypervisor->vcpu_count = KeQueryActiveProcessorCount(nullptr);

  // size of the vcpu array
  auto const arr_size = sizeof(vcpu) * global_hypervisor->vcpu_count;

  // allocate an array of vcpus
  global_hypervisor->vcpus = static_cast<vcpu*>(
    alloc_aligned(arr_size, alignof(vcpu)));

  if (!global_hypervisor->vcpus) {
    free(global_hypervisor);
    return false;
  }

  memset(global_hypervisor->vcpus, 0, arr_size);

  // we need to be running at an irql below DISPATCH_LEVEL so
  // that KeSetSystemAffinityThreadEx takes effect immediately
  NT_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

  // virtualize every cpu
  for (unsigned long i = 0; i < global_hypervisor->vcpu_count; ++i) {
    vcpu& cpu = global_hypervisor->vcpus[i];

    // restrict execution to the specified cpu
    auto const orig_affinity = KeSetSystemAffinityThreadEx(1ull << i);

    if (!cpu.virtualize()) {
      // TODO: handle this bruh -_-
      KeRevertToUserAffinityThreadEx(orig_affinity);
      return false;
    }

    KeRevertToUserAffinityThreadEx(orig_affinity);
  }

  return true;
}

// get a pointer to the global hypervisor
hypervisor* ghv() {
  return global_hypervisor;
}

} // namespace hv

