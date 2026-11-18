// Direct3D 12 bridge for the overlay (D2): owns the Dear ImGui DX12 backend and
// its own shader-visible SRV heap.  render_d3d12.c (plain C) only talks to the
// C API in include/overlay.h; overlay.cpp calls the functions below.
//
// Why a private heap instead of a reserved range in the renderer's ring:
// the renderer's shader-visible heap is a per-frame ring that is rewritten
// with 8-SRV tables and sized for its own use; ImGui needs one persistent
// descriptor (font atlas) that must survive every frame, and a command list
// can have only one CBV/SRV/UAV heap bound at a time.  So ImGui binds its own
// heap while it draws and the renderer re-binds its heaps afterwards
// (render_d3d12.c apply_frame_state() resets all list state).
#include "overlay_d3d12.h"

#include <d3d12.h>
#include <stdio.h>

#include "imgui.h"
#include "backends/imgui_impl_dx12.h"

namespace {

constexpr int kSrvCount = 8;

ID3D12DescriptorHeap* s_heap;
UINT                  s_inc;
bool                  s_used[kSrvCount];
bool                  s_ready;
ImGui_ImplDX12_InitInfo s_info;

void alloc_srv(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* cpu, D3D12_GPU_DESCRIPTOR_HANDLE* gpu) {
    for (int i = 0; i < kSrvCount; i++) {
        if (s_used[i]) continue;
        s_used[i] = true;
        *cpu = s_heap->GetCPUDescriptorHandleForHeapStart();
        *gpu = s_heap->GetGPUDescriptorHandleForHeapStart();
        cpu->ptr += (SIZE_T)i * s_inc;
        gpu->ptr += (UINT64)i * s_inc;
        return;
    }
    *cpu = D3D12_CPU_DESCRIPTOR_HANDLE{};
    *gpu = D3D12_GPU_DESCRIPTOR_HANDLE{};
}

void free_srv(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE) {
    SIZE_T base = s_heap->GetCPUDescriptorHandleForHeapStart().ptr;
    if (cpu.ptr < base) return;
    SIZE_T i = (cpu.ptr - base) / s_inc;
    if (i < (SIZE_T)kSrvCount) s_used[i] = false;
}

}  // namespace

extern "C" {

bool overlay_d3d12_init(void* device, void* queue, int frames_in_flight) {
    ID3D12Device* dev = (ID3D12Device*)device;
    if (!dev || !queue) return false;
    D3D12_DESCRIPTOR_HEAP_DESC hd = {};
    hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    hd.NumDescriptors = kSrvCount;
    hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(dev->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&s_heap)))) return false;
    s_inc = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    for (bool& u : s_used) u = false;

    s_info = ImGui_ImplDX12_InitInfo();
    s_info.Device = dev;
    s_info.CommandQueue = (ID3D12CommandQueue*)queue;
    s_info.NumFramesInFlight = frames_in_flight;
    s_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;   // back buffers and the dump target
    s_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
    s_info.SrvDescriptorHeap = s_heap;
    s_info.SrvDescriptorAllocFn = alloc_srv;
    s_info.SrvDescriptorFreeFn = free_srv;
    if (!ImGui_ImplDX12_Init(&s_info)) {
        s_heap->Release(); s_heap = nullptr;
        return false;
    }
    s_ready = true;
    return true;
}

void overlay_d3d12_shutdown(void) {
    if (!s_ready) return;
    ImGui_ImplDX12_Shutdown();
    if (s_heap) { s_heap->Release(); s_heap = nullptr; }
    s_ready = false;
}

void overlay_d3d12_new_frame(void) {
    if (s_ready) ImGui_ImplDX12_NewFrame();
}

void overlay_d3d12_render(ImDrawData* dd, void* cmdlist) {
    if (!s_ready || !dd || !cmdlist) return;
    // the backend does not bind its SRV heap itself; the renderer re-binds its own heaps afterwards
    ID3D12GraphicsCommandList* cl = (ID3D12GraphicsCommandList*)cmdlist;
    cl->SetDescriptorHeaps(1, &s_heap);
    ImGui_ImplDX12_RenderDrawData(dd, cl);
}

// Swap chain resize / device objects reset.  The DX12 backend has no size
// dependent resources, but the pipeline and font are rebuilt so a renderer that
// recreates its targets also gets fresh ImGui objects.  GPU must be idle.
void overlay_d3d12_invalidate(void) {
    if (!s_ready) return;
    ImGui_ImplDX12_InvalidateDeviceObjects();
    if (!ImGui_ImplDX12_CreateDeviceObjects())
        fprintf(stderr, "[overlay] D3D12 device objects recreate failed\n");
}

}  // extern "C"
