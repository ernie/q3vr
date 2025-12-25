# renderervk, from Quake3e

## Vulkan renderer

Based on [Quake-III-Arena-Kenny-Edition](https://github.com/kennyalive/Quake-III-Arena-Kenny-Edition) with many additions:

* high-quality per-pixel dynamic lighting
* very fast flares (**\r_flares 1**)
* anisotropic filtering (**\r_ext_texture_filter_anisotropic**)
* greatly reduced API overhead (call/dispatch ratio)
* flexible vertex buffer memory management to allow loading huge maps
* multiple command buffers to reduce processing bottlenecks
* [reversed depth buffer](https://developer.nvidia.com/content/depth-precision-visualized) to eliminate z-fighting on big maps
* merged lightmaps (atlases)
* multitexturing optimizations
* static world surfaces cached in VBO (**\r_vbo 1**)
* useful debug markers for tools like [RenderDoc](https://renderdoc.org/)
* fixed framebuffer corruption on some Intel iGPUs
* offscreen rendering, enabled with **\r_fbo 1**, all following requires it enabled:
* `screenMap` texture rendering - to create realistic environment reflections
* multisample anti-aliasing (**\r_ext_multisample**)
* supersample anti-aliasing (**\r_ext_supersample**)
* per-window gamma-correction which is important for screen-capture tools like OBS
* you can minimize game window any time during **\video**|**\video-pipe** recording
* high dynamic range render targets (**\r_hdr 1**) to avoid color banding
* bloom post-processing effect
* arbitrary resolution rendering
* greyscale mode

In general, not counting offscreen rendering features you might expect from 10% to 200%+ FPS increase comparing to KE's original version

Highly recommended to use on modern systems

## q3vr-specific changes

* Multiview
* Support for VR HUD, overlay, and virtual screen rendering compatible with existing renderergl2 API patterns
* Shader compilation via cmake