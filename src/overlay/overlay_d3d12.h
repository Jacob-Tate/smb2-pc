// Internal: D3D12 bridge between overlay.cpp and the ImGui DX12 backend.
#ifndef SMB2_OVERLAY_D3D12_H
#define SMB2_OVERLAY_D3D12_H
#include <stdbool.h>
struct ImDrawData;
extern "C" {
bool overlay_d3d12_init(void* id3d12_device, void* id3d12_queue, int frames_in_flight);
void overlay_d3d12_shutdown(void);
void overlay_d3d12_new_frame(void);
void overlay_d3d12_render(ImDrawData* dd, void* id3d12_graphics_command_list);
void overlay_d3d12_invalidate(void);
}
#endif
