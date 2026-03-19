const DOOM_KEYS = {
  ArrowLeft: 0xac,
  ArrowUp: 0xad,
  ArrowRight: 0xae,
  ArrowDown: 0xaf,
  Control: 0xa3,
  " ": 0xa2,
  Shift: 0xb6,
  Alt: 0xb8,
  Enter: 13,
  Escape: 27,
  Tab: 9,
  Backspace: 0x7f,
  Minus: 0x2d,
  Equal: 0x3d,
  F2: 0xbc,
  F3: 0xbd,
  F4: 0xbe,
  F5: 0xbf,
  F6: 0xc0,
  F7: 0xc1,
  F8: 0xc2,
  F9: 0xc3,
  F10: 0xc4,
  F11: 0xd7,
};

const elements = {
  canvas: document.getElementById("doom-canvas"),
  status: document.getElementById("status"),
};

let runtimeReady = false;
let launched = false;
let gpuState = null;
let frameHandle = 0;

function resumeAudioIfNeeded() {
  const contexts = [
    window.Module?.SDL2?.audioContext,
    window.SDL2?.audioContext,
    window.Module?.SDL?.audioContext,
    window.SDL?.audioContext,
  ];

  for (const context of contexts) {
    if (context && typeof context.resume === "function" && context.state === "suspended") {
      void context.resume().catch(() => {});
    }
  }
}

function getHeapU8() {
  if (window.HEAPU8 instanceof Uint8Array) {
    return window.HEAPU8;
  }

  if (window.Module?.HEAPU8 instanceof Uint8Array) {
    return window.Module.HEAPU8;
  }

  if (window.wasmMemory?.buffer instanceof ArrayBuffer) {
    return new Uint8Array(window.wasmMemory.buffer);
  }

  return null;
}

function setStatus(message) {
  if (!elements.status) return;

  if (!message) {
    elements.status.textContent = "";
    elements.status.classList.add("hidden");
    return;
  }

  elements.status.textContent = message;
  elements.status.classList.remove("hidden");
}

function isSecureEnough() {
  return window.isSecureContext || location.protocol === "https:" || location.hostname === "localhost";
}

function mapBrowserKey(eventKey, eventCode) {
  if (eventKey in DOOM_KEYS) return DOOM_KEYS[eventKey];

  if (eventCode === "Minus") return DOOM_KEYS.Minus;
  if (eventCode === "Equal") return DOOM_KEYS.Equal;

  if (typeof eventKey === "string" && eventKey.length === 1) {
    return eventKey.toLowerCase().charCodeAt(0);
  }

  return null;
}

function pushKey(pressed, eventKey, eventCode = eventKey) {
  const doomKey = mapBrowserKey(eventKey, eventCode);
  if (doomKey === null || !runtimeReady || !window.Module?._DGW_PushKeyEvent) {
    return false;
  }

  resumeAudioIfNeeded();
  window.Module._DGW_PushKeyEvent(pressed ? 1 : 0, doomKey);
  return true;
}

function bindKeyboard() {
  const handler = (pressed) => (event) => {
    if (!launched) return;
    if (pressed && event.repeat) return;

    if (pushKey(pressed, event.key, event.code)) {
      event.preventDefault();
    }
  };

  window.addEventListener("keydown", handler(true), { passive: false });
  window.addEventListener("keyup", handler(false), { passive: false });
  window.addEventListener("pointerdown", () => {
    resumeAudioIfNeeded();
    elements.canvas?.focus();
  });
}

function resizeCanvas(device, context, format) {
  const pixelRatio = Math.min(window.devicePixelRatio || 1, 2);
  const rect = elements.canvas.getBoundingClientRect();
  const width = Math.max(1, Math.floor(rect.width * pixelRatio));
  const height = Math.max(1, Math.floor(rect.height * pixelRatio));

  if (elements.canvas.width === width && elements.canvas.height === height) {
    return;
  }

  elements.canvas.width = width;
  elements.canvas.height = height;
  context.configure({
    device,
    format,
    alphaMode: "opaque",
  });
}

async function ensureWebGpu(width, height) {
  if (!navigator.gpu) {
    throw new Error("WebGPU is not available in this browser. Use Chrome or Edge over HTTPS.");
  }

  const adapter = await navigator.gpu.requestAdapter();
  if (!adapter) {
    throw new Error("No WebGPU adapter was available.");
  }

  const device = await adapter.requestDevice();
  const context = elements.canvas.getContext("webgpu");
  if (!context) {
    throw new Error("The browser could not create a WebGPU canvas context.");
  }

  const format = navigator.gpu.getPreferredCanvasFormat();
  resizeCanvas(device, context, format);
  window.addEventListener("resize", () => resizeCanvas(device, context, format));

  const shaderModule = device.createShaderModule({
    code: `
      struct VertexOutput {
        @builtin(position) position : vec4<f32>,
        @location(0) uv : vec2<f32>,
      };

      @vertex
      fn vs_main(@builtin(vertex_index) vertexIndex : u32) -> VertexOutput {
        var positions = array<vec2<f32>, 6>(
          vec2<f32>(-1.0, -1.0),
          vec2<f32>( 1.0, -1.0),
          vec2<f32>(-1.0,  1.0),
          vec2<f32>(-1.0,  1.0),
          vec2<f32>( 1.0, -1.0),
          vec2<f32>( 1.0,  1.0)
        );

        var uvs = array<vec2<f32>, 6>(
          vec2<f32>(0.0, 1.0),
          vec2<f32>(1.0, 1.0),
          vec2<f32>(0.0, 0.0),
          vec2<f32>(0.0, 0.0),
          vec2<f32>(1.0, 1.0),
          vec2<f32>(1.0, 0.0)
        );

        var out : VertexOutput;
        out.position = vec4<f32>(positions[vertexIndex], 0.0, 1.0);
        out.uv = uvs[vertexIndex];
        return out;
      }

      @group(0) @binding(0) var doomTexture : texture_2d<f32>;
      @group(0) @binding(1) var doomSampler : sampler;

      @fragment
      fn fs_main(in : VertexOutput) -> @location(0) vec4<f32> {
        return textureSample(doomTexture, doomSampler, in.uv);
      }
    `,
  });

  const sampler = device.createSampler({
    magFilter: "nearest",
    minFilter: "nearest",
  });

  const texture = device.createTexture({
    size: { width, height, depthOrArrayLayers: 1 },
    format: "rgba8unorm",
    usage: GPUTextureUsage.TEXTURE_BINDING | GPUTextureUsage.COPY_DST,
  });

  const pipeline = device.createRenderPipeline({
    layout: "auto",
    vertex: {
      module: shaderModule,
      entryPoint: "vs_main",
    },
    fragment: {
      module: shaderModule,
      entryPoint: "fs_main",
      targets: [{ format }],
    },
    primitive: {
      topology: "triangle-list",
    },
  });

  const bindGroup = device.createBindGroup({
    layout: pipeline.getBindGroupLayout(0),
    entries: [
      { binding: 0, resource: texture.createView() },
      { binding: 1, resource: sampler },
    ],
  });

  return {
    device,
    context,
    pipeline,
    bindGroup,
    texture,
    width,
    height,
    rgbaFrame: new Uint8Array(width * height * 4),
    framePtr: 0,
  };
}

function presentFrame() {
  if (!gpuState || !runtimeReady) return;

  const heap = getHeapU8();
  if (!heap || !gpuState.framePtr) {
    return;
  }

  const frameBytes = gpuState.width * gpuState.height * 4;
  const frameStart = gpuState.framePtr >>> 0;
  const frameEnd = frameStart + frameBytes;
  if (frameEnd > heap.length) {
    return;
  }

  const source = heap.subarray(frameStart, frameEnd);

  for (let i = 0; i < source.length; i += 4) {
    gpuState.rgbaFrame[i] = source[i + 2];
    gpuState.rgbaFrame[i + 1] = source[i + 1];
    gpuState.rgbaFrame[i + 2] = source[i];
    gpuState.rgbaFrame[i + 3] = 255;
  }

  gpuState.device.queue.writeTexture(
    { texture: gpuState.texture },
    gpuState.rgbaFrame,
    { bytesPerRow: gpuState.width * 4 },
    { width: gpuState.width, height: gpuState.height, depthOrArrayLayers: 1 }
  );

  const encoder = gpuState.device.createCommandEncoder();
  const pass = encoder.beginRenderPass({
    colorAttachments: [
      {
        view: gpuState.context.getCurrentTexture().createView(),
        loadOp: "clear",
        clearValue: { r: 0, g: 0, b: 0, a: 1 },
        storeOp: "store",
      },
    ],
  });

  pass.setPipeline(gpuState.pipeline);
  pass.setBindGroup(0, gpuState.bindGroup);
  pass.draw(6, 1, 0, 0);
  pass.end();

  gpuState.device.queue.submit([encoder.finish()]);
}

function renderLoop() {
  if (!launched) return;

  try {
    window.Module._DGW_Tick();
    presentFrame();
    frameHandle = requestAnimationFrame(renderLoop);
  } catch (error) {
    launched = false;
    console.error(error);
    setStatus(error instanceof Error ? error.message : "Doom rendering failed.");
  }
}

function injectDoomScript() {
  return new Promise((resolve, reject) => {
    const script = document.createElement("script");
    script.src = "/doom/doomgeneric.js";
    script.async = true;
    script.onload = resolve;
    script.onerror = () => reject(new Error("Missing Doom bundle. Build the browser assets and flash the SPIFFS partition."));
    document.body.appendChild(script);
  });
}

async function loadRuntime() {
  if (runtimeReady) return;

  await new Promise((resolve, reject) => {
    window.Module = {
      noInitialRun: true,
      canvas: elements.canvas,
      locateFile(path) {
        return `/doom/${path.split("/").pop()}`;
      },
      print(text) {
        if (typeof text === "string" && text.trim()) {
          console.log(text);
        }
      },
      printErr(text) {
        console.error(text);
      },
      monitorRunDependencies(count) {
        if (count > 0) {
          setStatus(`Loading Doom assets (${count})...`);
        }
      },
      onAbort(reason) {
        reject(new Error(String(reason)));
      },
      onRuntimeInitialized() {
        runtimeReady = true;
        resolve();
      },
    };

    injectDoomScript().catch(reject);
  });
}

async function bootDoom() {
  if (!isSecureEnough()) {
    throw new Error("Open this page over HTTPS. WebGPU will not start from plain HTTP.");
  }

  setStatus("Loading Doom runtime...");
  await loadRuntime();

  const width = window.Module._DGW_GetScreenWidth();
  const height = window.Module._DGW_GetScreenHeight();
  gpuState = await ensureWebGpu(width, height);

  setStatus("Starting Doom...");
  if (!window.Module._DGW_Boot()) {
    throw new Error("Doom failed to boot. Build doomgeneric.data with an embedded WAD.");
  }

  gpuState.framePtr = window.Module._DGW_GetScreenBuffer();
  launched = true;
  elements.canvas.focus();
  setStatus("");

  cancelAnimationFrame(frameHandle);
  renderLoop();
}

async function init() {
  elements.canvas.tabIndex = 0;
  bindKeyboard();

  try {
    await bootDoom();
  } catch (error) {
    console.error(error);
    setStatus(error instanceof Error ? error.message : "Doom launch failed.");
  }
}

init();
