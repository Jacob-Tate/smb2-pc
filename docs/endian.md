# Big-endian disc data

The decomp reads file data with native loads, as it did on the big-endian
GameCube CPU. On PC each file format is byte-swapped in place right after it is
loaded and before the game relocates its offsets into pointers. The swaps live
in `src/game/shim/endian_fix.c` (TPL, NaomiLib models, ape skeleton/motion,
stagedefs, replays, dialog and scene tables) and `src/game/shim/endian_gma.c`
(GMA/GCMF models). They are called from `#ifndef __MWERKS__` hooks in the
decomp loaders, or, for loaders that are still asm-only, from the PC loader
implementations.

General rules:

- Only fields the CPU reads as 16/32-bit values are swapped. Strings, byte
  fields and GX display lists stay as they are on disc; the display-list
  interpreter (`avdisp_shim.c`) decodes big-endian explicitly. Texel data in
  TPLs also stays raw and is decoded by the renderer.
- A swap must run exactly once per byte. Where data can be shared (GMA
  archives aliasing models, stagedef arrays that are sub-ranges of other
  arrays) the swap is idempotent: GMA models check the already-native `GCMF`
  magic; the stagedef traversal keeps a bitmap with one bit per 2-byte unit.
- Swaps follow the same traversal as the game's own offset fixup, so every
  offset is still a file offset when it is followed.

## GMA archives

All GMA loads go through the avload module, which is asm-only in the decomp
(g_load_raw_gma 0x80065984, g_load_gma 0x80065AA8, g_init_gma 0x8006A8DC,
g_init_gma_model_materials 0x8006A9C8). `endian_gma.c` provides PC versions of
these entry points, mirrored from the asm, and runs `pc_swap_gma` after the
file is loaded and before the offset->pointer fixup.

Layout (decomp `include/avdisp.h`, ref smb-decomp `src/gma.h`):

```
archive   u32 numModels; u32 modelsBaseOff;
          numModels * { u32 modelOff; u32 nameOff }   modelOff 0xFFFFFFFF = null model
          name strings (not swapped)
          GCMF models at modelsBaseOff + modelOff

GCMF      0x40 header: u32 magic "GCMF"; u32 flags; Vec center; f32 radius;
          u16 tevLayerCount, opaqueShapeCount, translucentShapeCount;
          u8 mtxCount, pad; u32 headerSize; @0x24 texObjs (runtime only);
          u8 mtxIndexes[8]
tev layer 0x20 each at +0x40: u32 flags; u16 texIndex; s8 lodBias;
          u8 maxAniso; @0x08 texObj (runtime only)
matrices  mtxCount * Mtx (12 f32) after the tev layers: STITCHING bone
          matrices, or SKIN bind matrices used by draw_poly(model, NULL)
shapes    at model + headerSize, opaque + translucent of them:
          0x60 header (struct GMAShape) + display lists (not swapped).
          dispListFlags bits 0/1 select dispListSizes[0/1]; bits 2/3 add a
          0x20 block { u8 mtxIndices[8]; u32 sizes[2] } and two more lists.
```

SKIN/EFFECTIVE (deformable) models instead have a 0x20 pre-header of u32
offsets at model + headerSize (@0x1C is overwritten with model->flags at load;
@0x08/@0x0C point at the CPU-read vertex and index data), followed by flat
0x60-stride shape headers without inline display lists. The vertex, index and
skin-source layouts are documented in `avdisp_shim.c` (deformable models).
The 16-bit (GCMF_16BIT) payload is never written by the CPU, so it stays
big-endian and the draw path decodes it explicitly.
