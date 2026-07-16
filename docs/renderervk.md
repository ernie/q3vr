# RenderVK: Vulkan Renderer for Q3VR

This document describes the Vulkan renderer (`renderervk`) for Quake 3 VR, its unique features, configuration variables, and why you might prefer it to renderergl2.

## Why Vulkan for VR?

### Performance Benefits

Vulkan provides significant advantages over OpenGL for VR rendering:

1. **Lower Driver Overhead** - Vulkan's explicit API design eliminates the CPU-side validation and state tracking that OpenGL performs implicitly. This reduces per-draw-call overhead dramatically.

2. **Multi-Command Buffer Architecture** - renderervk uses 2 command buffers (`NUM_COMMAND_BUFFERS=2`) to overlap CPU command recording with GPU execution, reducing frame time variance.

3. **Explicit Memory Management** - Direct control over GPU memory allocation allows for optimal resource placement and reduced fragmentation.

4. **Predictable Frame Timing** - Vulkan's explicit synchronization provides more consistent frame delivery.

---

## Vulkan-Specific Features

These features are specific to renderervk, whether because they are exclusive, or improved significantly from their OpenGL counterpart.

### Per-Pixel Dynamic Lighting (PMLIGHT)

The most significant visual improvement over the OpenGL renderer. PMLIGHT provides high-quality per-pixel dynamic light calculations that are both **faster and better looking** than VQ3's original approach on modern GPUs.

| Cvar | Default | Description |
|------|---------|-------------|
| `r_dlightMode` | 0 | Lighting mode selection |
| `r_dlightScale` | 0.5 | Dynamic light radius multiplier |
| `r_dlightIntensity` | 1.0 | Dynamic light brightness |
| `r_dlightSaturation` | 1.0 | Dynamic light color saturation |

**r_dlightMode values:**
- `0` - VQ3 "fake" dynamic lights (legacy compatibility)
- `1` - Per-pixel dynamic lighting (faster than mode 0 on modern hardware)
- `2` - Per-pixel lighting applied to all MD3 models as well

> **Note:** While renderergl2 also supports `r_dlightMode`, renderervk uses a Vulkan-native implementation with BSP-based surface tracking (`VK_LightingPass`) that is more efficient than renderergl2's forward rendering approach (`ForwardDlight`). The visual result is similar, but renderervk's architecture is better suited to Vulkan's explicit pipeline model.

### Reversed Depth Buffer

Automatically enabled. Uses reversed depth (near=1.0, far=0.0) to provide better depth precision across the entire view distance. This eliminates the z-fighting artifacts commonly seen on large Quake 3 maps where distant geometry would flicker.

### GPU Device Selection

| Cvar | Default | Description |
|------|---------|-------------|
| `r_device` | -1 | Physical GPU selection |

**Values:**
- `-2` - Prefer integrated GPU (power saving)
- `-1` - Prefer discrete GPU (default, best performance)
- `0+` - Explicit device index

Useful for systems with multiple GPUs, allowing you to ensure the VR headset uses the correct GPU.

### ScreenMap (Environment Reflections)

Texture-based environment capture for realistic reflective surfaces. The scene is rendered to a texture that can be sampled by shaders for real-time reflections.

### Ordered Dithering

| Cvar | Default | Description |
|------|---------|-------------|
| `r_dither` | 0 | Enable ordered dithering |

Reduces color banding artifacts in gradients, particularly noticeable in dark areas and sky transitions. Especially useful when `r_hdr 0` (8-bit rendering).

### Map-Only Greyscale

| Cvar | Default | Description |
|------|---------|-------------|
| `r_mapGreyScale` | 0 | Desaturate map textures only (-1 to 1) |

Unlike `r_greyscale` which affects the entire frame, this only desaturates world geometry while keeping player models and effects in full color.

### Sky Texture Quality

| Cvar | Default | Description |
|------|---------|-------------|
| `r_neatsky` | 0 | Disable sky texture mipping |

Keeps sky textures at full resolution regardless of distance, preventing the blurry sky appearance that can occur with mipmapping.

### Teleporter Flash Color

| Cvar | Default | Description |
|------|---------|-------------|
| `r_teleporterFlash` | 1 | Use white flash instead of black |

Changes the teleporter effect from the original black flash to white, which some players find less jarring.

### Missing Texture Replacement

| Cvar | Default | Description |
|------|---------|-------------|
| `r_defaultImage` | "" | Custom missing texture |

Specify a file path or color value to use instead of the default checkerboard for missing textures.

---

## Enhanced Post-Processing

These features have different or enhanced implementations compared to renderergl2.

### Bloom Effect

renderervk implements a 4-pass Gaussian blur bloom with configurable extraction modes.

| Cvar | Default | Description |
|------|---------|-------------|
| `r_bloom` | 0 | Enable bloom effect |
| `r_bloom_threshold` | 0.6 | Brightness threshold for extraction |
| `r_bloom_threshold_mode` | 0 | Extraction algorithm |
| `r_bloom_intensity` | 0.5 | Final bloom blend strength |
| `r_bloom_modulate` | 0 | Bloom self-modulation |

**r_bloom_threshold_mode values:**
- `0` - Per-channel extraction (each RGB channel independent)
- `1` - Average-based extraction
- `2` - **Recommended for VR:** Luma-based extraction (perceptually accurate)

**r_bloom_modulate values:**
- `0` - No modulation
- `1` - Modulate by bloom color itself
- `2` - Modulate by bloom intensity

> **Technical note:** Bloom parameters are compiled into Vulkan pipelines as specialization constants. The shaders are located in `code/renderervk/shaders/`: `bloom.frag` (extraction), `blur.frag` (4-pass Gaussian), and `blend.frag` (final composite). Changing bloom cvars requires a `vid_restart` to recreate pipelines.

### HDR Framebuffer

| Cvar | Default | Description |
|------|---------|-------------|
| `r_hdr` | 0 | Framebuffer precision mode |

**Values:**
- `-1` - 4-bit (testing only, heavy banding)
- `0` - 8-bit (default, minor banding with complex shaders)
- `1` - 16-bit (eliminates banding, may impact AMD/Intel GPUs)

Unlike renderergl2's full HDR pipeline with tone mapping and auto-exposure, renderervk's HDR is purely about framebuffer precision. For VR, `r_hdr 0` with `r_dither 1` often provides the best balance.

### Desktop Mirror HDR Output

| Cvar | Default | Description |
|------|---------|-------------|
| `r_hdrDisplay` | 0 | Enable scRGB FP16 HDR output on the desktop mirror window |
| `r_hdrPeak` | 1000 | Display peak brightness in nits (range 250–10000). Sets the highlight ceiling and feeds auto paper-white |
| `r_hdrPaperWhite` | 0 | SDR-white brightness in nits. 0 = auto (BT.2408 reference white derived from `r_hdrPeak`) |
| `r_hdrHighlight` | 1.0 | Highlight push into the overbright headroom (range 0.5–4.0). 1.0 = natural roll-off |

When `r_hdrDisplay 1`, the desktop mirror window outputs scRGB-linear FP16 (extended headroom above 1.0), requiring a Windows HDR-capable display. The headset view is unaffected — OpenXR has no HDR luminance path, so the headset always receives standard Rec.709 output from the gamma pass.

**Emissive highlights (desktop mirror)**

Additive 3D emitters (weapons, projectiles, effects) are captured during the main render pass into a separate multiview FP16 layer (`vk.emissive_image`). In `desktopmirror.frag` (`hdrMode==1`), this layer is sampled alongside the pre-overbright scene base to restore channels that were clipped to the UNORM ceiling in the eye swapchain. The result is hue-preserving highlight roll-off toward the panel peak (`r_hdrPeak` nits), so bright muzzle flashes and plasma bolts appear genuinely above paper-white on an HDR display. Active only with `r_hdrDisplay 1`; the headset view and the SDR mirror (`r_hdrDisplay 0`) are byte-identical to the pre-feature output.

### MSAA (Multisample Anti-Aliasing)

| Cvar | Default | Description |
|------|---------|-------------|
| `r_ext_multisample` | 4 | MSAA sample count |

**Values:** 0, 2, 4, or 8 samples.

### Render Scaling

Render at a different resolution than the display, then scale to output.

| Cvar | Default | Description |
|------|---------|-------------|
| `r_renderWidth` | 0 | Custom render width (0=auto) |
| `r_renderHeight` | 0 | Custom render height (0=auto) |
| `r_renderScale` | 0 | Scaling mode |

**r_renderScale values:**
- `0` - Disabled (render at native resolution)
- `1` - Nearest-neighbor stretch
- `2` - Nearest-neighbor preserve aspect ratio
- `3` - Linear (bilinear) stretch
- `4` - Linear preserve aspect ratio

For supersampling, set `r_renderWidth` and `r_renderHeight` higher than display resolution.

---

## Complete Cvar Reference

### Core Rendering

| Cvar | Default | Description |
|------|---------|-------------|
| `r_fbo` | 1 | **Always-on in Q3VR** for bloom, HDR, MSAA, greyscale, screenmap |
| `r_vbo` | 1 | Vertex buffer objects for static geometry |
| `r_mergeLightmaps` | 1 | Combine lightmaps into atlases |

### Post-Processing

| Cvar | Default | Description |
|------|---------|-------------|
| `r_hdr` | 0 | HDR precision (-1=4-bit, 0=8-bit, 1=16-bit) |
| `r_hdrDisplay` | 0 | scRGB FP16 HDR on desktop mirror (0=SDR, 1=HDR) |
| `r_bloom` | 0 | Enable bloom |
| `r_bloom_threshold` | 0.6 | Extraction threshold |
| `r_bloom_threshold_mode` | 0 | 0=channel, 1=average, 2=luma |
| `r_bloom_intensity` | 0.5 | Blend intensity |
| `r_bloom_modulate` | 0 | Modulation mode |
| `r_ext_multisample` | 4 | MSAA samples (0,2,4,8) |
| `r_greyscale` | 0 | Desaturation level (-1 to 1) |
| `r_mapGreyScale` | 0 | Map-only desaturation |
| `r_dither` | 0 | Ordered dithering |

### Render Resolution

| Cvar | Default | Description |
|------|---------|-------------|
| `r_renderWidth` | 0 | Custom width (0=auto) |
| `r_renderHeight` | 0 | Custom height (0=auto) |
| `r_renderScale` | 0 | Scaling mode (0-4) |
| `r_presentBits` | 24 | Presentation color bits (16-30) |

### Dynamic Lighting

| Cvar | Default | Description |
|------|---------|-------------|
| `r_dynamiclight` | 1 | Enable dynamic lights |
| `r_dlightMode` | 0 | 0=VQ3, 1=per-pixel, 2=all models |
| `r_dlightScale` | 0.5 | Radius scale |
| `r_dlightIntensity` | 1.0 | Brightness scale |
| `r_dlightSaturation` | 1.0 | Color saturation |
| `r_dlightBacks` | 1 | Light backfaces |

### Textures

| Cvar | Default | Description |
|------|---------|-------------|
| `r_picmip` | 0 | Texture quality reduction (0=best) |
| `r_nomip` | 0 | Apply picmip to worldspawn only |
| `r_texturebits` | 0 | Texture bit depth |
| `r_textureMode` | GL_LINEAR_MIPMAP_LINEAR | Filtering mode |
| `r_neatsky` | 0 | Disable sky mipping |
| `r_detailtextures` | 1 | Detail texture stages |
| `r_ext_texture_filter_anisotropic` | 1 | Enable anisotropic filtering |
| `r_ext_max_anisotropy` | 8 | Maximum anisotropy level |
| `r_defaultImage` | "" | Missing texture replacement |

### Lighting & Color

| Cvar | Default | Description |
|------|---------|-------------|
| `r_fullbright` | 0 | Disable lighting (debug) |
| `r_overBrightBits` | 1 | Overbright bits |
| `r_mapOverBrightBits` | 2 | Lightmap overbright bits |
| `r_intensity` | 1 | Global intensity multiplier |
| `r_gamma` | 1 | Gamma correction |
| `r_ambientScale` | 0.6 | Ambient light scale (cheat) |
| `r_directedScale` | 1.0 | Direct light scale (cheat) |

### Geometry

| Cvar | Default | Description |
|------|---------|-------------|
| `r_subdivisions` | 4 | Bezier curve tessellation (lower = smoother curves) |
| `r_lodCurveError` | 8192 | LOD error threshold |
| `r_lodbias` | -2 | LOD bias (-2 = higher quality) |
| `r_lodscale` | 5 | LOD scale multiplier |
| `r_facePlaneCull` | 1 | Backface plane culling |
| `r_marksOnTriangleMeshes` | 0 | Impact marks on MD3 models |

### Flares

| Cvar | Default | Description |
|------|---------|-------------|
| `r_flares` | 1 | Enable light flares |
| `r_flareSize` | 40 | Flare sprite size |
| `r_flareFade` | 7 | Flare fade distance |
| `r_flareCoeff` | 150 | Flare intensity coefficient |

### Rail Effects

| Cvar | Default | Description |
|------|---------|-------------|
| `r_railWidth` | 16 | Rail trail width |
| `r_railCoreWidth` | 6 | Rail core width |
| `r_railSegmentLength` | 32 | Rail segment length |

### Sky

| Cvar | Default | Description |
|------|---------|-------------|
| `r_fastsky` | 0 | Flat colored sky (faster) |
| `r_drawSun` | 1 | Draw sun shader |
| `r_showsky` | 0 | Force sky in front |

### Hardware

| Cvar | Default | Description |
|------|---------|-------------|
| `r_device` | -1 | GPU: -2=integrated, -1=discrete, 0+=index |
| `r_znear` | 4 | Near clip plane |
| `r_zproj` | 64 | Projection distance |
| `r_finish` | 0 | Force GPU sync after rendering |

### Debug

| Cvar | Default | Description |
|------|---------|-------------|
| `r_showtris` | 0 | Wireframe rendering |
| `r_shownormals` | 0 | Show surface normals |
| `r_showImages` | 0 | List loaded textures |
| `r_nocull` | 0 | Disable frustum culling |
| `r_nocurves` | 0 | Disable bezier curves |
| `r_drawworld` | 1 | Draw world geometry |
| `r_drawentities` | 1 | Draw entities |
| `r_lightmap` | 0 | Show lightmaps only |
| `r_novis` | 0 | Disable PVS culling |
| `r_noportals` | 0 | Disable portal rendering |
| `r_lockpvs` | 0 | Lock PVS position |
| `r_speeds` | 0 | Performance statistics |
| `r_clear` | 0 | Clear framebuffer each frame |

### Miscellaneous

| Cvar | Default | Description |
|------|---------|-------------|
| `r_teleporterFlash` | 1 | White teleporter flash |

---

## Recommended VR Settings

For a high quality starting point, add these to your `autoexec.cfg`:

```cfg
// ==============================================
// Q3VR renderervk Recommended Settings
// ==============================================

// Per-pixel lighting calculated in fragment shader on all models
seta r_dlightMode 2

// Adds glow effect around bright objects.
seta r_bloom 1

// Reduces color banding in gradients (dark areas, sky transitions)
seta r_dither 1

// Light source flares (default 0)
seta r_flares 1

// VR-specific texture overrides
seta r_ext_max_anisotropy 16     // Default 8; sharper textures at oblique angles
seta r_textureLodBias -0.75      // Default 0; (YMMV but VR is more aggressive about mipmaps)
seta r_neatsky 1                 // Default 0; full-resolution sky textures
```

Everything else uses sensible defaults. Adjust bloom parameters (`r_bloom_threshold`, `r_bloom_intensity`, `r_bloom_threshold_mode`) to personal preference if you want to tweak the effect.
