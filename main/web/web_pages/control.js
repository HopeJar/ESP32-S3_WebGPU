const SIM_WIDTH = 256;
const SIM_HEIGHT = 256;
const WORKGROUP_SIZE = 8;

const DEFAULT_PARAMS = {
  fade: 0.985,
  radius: 18,
  intensity: 1.2,
  swirlStrength: 0.9,
  glowSpeed: 0.25,
};

const params = { ...DEFAULT_PARAMS };
let ledColor = { r: 0, g: 0, b: 0 };

const elements = {
  canvas: document.getElementById('neon-canvas'),
  error: document.getElementById('error'),
  fade: document.getElementById('fade'),
  fadeValue: document.getElementById('fade-value'),
  radius: document.getElementById('radius'),
  radiusValue: document.getElementById('radius-value'),
  intensity: document.getElementById('intensity'),
  intensityValue: document.getElementById('intensity-value'),
  swirl: document.getElementById('swirl'),
  swirlValue: document.getElementById('swirl-value'),
  glow: document.getElementById('glow'),
  glowValue: document.getElementById('glow-value'),
  reset: document.getElementById('reset'),
  color: document.getElementById('color'),
  swatch: document.getElementById('led-swatch'),
};

function setError(message) {
  if (!elements.error) return;
  if (message) {
    elements.error.textContent = message;
    elements.error.classList.remove('hidden');
  } else {
    elements.error.textContent = '';
    elements.error.classList.add('hidden');
  }
}

function updateLabels() {
  elements.fadeValue.textContent = params.fade.toFixed(4);
  elements.radiusValue.textContent = `${Math.round(params.radius)} px`;
  elements.intensityValue.textContent = params.intensity.toFixed(2);
  elements.swirlValue.textContent = params.swirlStrength.toFixed(2);
  elements.glowValue.textContent = params.glowSpeed.toFixed(2);
}

function applyParamsToInputs() {
  elements.fade.value = params.fade;
  elements.radius.value = params.radius;
  elements.intensity.value = params.intensity;
  elements.swirl.value = params.swirlStrength;
  elements.glow.value = params.glowSpeed;
  updateLabels();
}

function rgbToHex(r, g, b) {
  return `#${[r, g, b].map((v) => v.toString(16).padStart(2, '0')).join('')}`;
}

async function refreshColor() {
  try {
    const res = await fetch('/api/color', { cache: 'no-store' });
    if (!res.ok) return;
    const c = await res.json();
    ledColor = { r: c.r, g: c.g, b: c.b };
    const hex = rgbToHex(c.r, c.g, c.b);
    elements.color.textContent = `R=${c.r} G=${c.g} B=${c.b}`;
    elements.swatch.style.background = hex;
  } catch (err) {
    // Ignore transient errors.
  }
}

function bindControls() {
  elements.fade.addEventListener('input', (e) => {
    params.fade = Number(e.target.value);
    updateLabels();
  });

  elements.radius.addEventListener('input', (e) => {
    params.radius = Number(e.target.value);
    updateLabels();
  });

  elements.intensity.addEventListener('input', (e) => {
    params.intensity = Number(e.target.value);
    updateLabels();
  });

  elements.swirl.addEventListener('input', (e) => {
    params.swirlStrength = Number(e.target.value);
    updateLabels();
  });

  elements.glow.addEventListener('input', (e) => {
    params.glowSpeed = Number(e.target.value);
    updateLabels();
  });

  elements.reset.addEventListener('click', () => {
    Object.assign(params, DEFAULT_PARAMS);
    applyParamsToInputs();
  });
}

const mouse = { x: 0, y: 0, down: false };

function setupPointer(canvas) {
  const onMove = (e) => {
    const rect = canvas.getBoundingClientRect();
    const nx = (e.clientX - rect.left) / rect.width;
    const ny = (e.clientY - rect.top) / rect.height;
    mouse.x = nx * SIM_WIDTH;
    mouse.y = ny * SIM_HEIGHT;
  };

  canvas.addEventListener('pointermove', onMove);
  canvas.addEventListener('pointerenter', () => (mouse.down = true));
  canvas.addEventListener('pointerleave', () => (mouse.down = false));
  canvas.style.touchAction = 'none';
}

async function initWebGPU(canvas) {
  if (!('gpu' in navigator)) {
    const insecureOrigin =
      location.protocol !== 'https:' &&
      location.hostname !== 'localhost' &&
      location.hostname !== '127.0.0.1';

    if (insecureOrigin) {
      throw new Error('WebGPU requires HTTPS or localhost. Open this page over https:// or use localhost during development.');
    }

    throw new Error('WebGPU is not available in this browser context.');
  }

  const adapter = await navigator.gpu.requestAdapter();
  if (!adapter) {
    throw new Error('No GPU adapter available for WebGPU.');
  }

  const device = await adapter.requestDevice();
  const context = canvas.getContext('webgpu');
  if (!context) {
    throw new Error('Unable to acquire a WebGPU canvas context.');
  }

  const format = navigator.gpu.getPreferredCanvasFormat();
  context.configure({ device, format, alphaMode: 'premultiplied' });
  return { device, context, format };
}

const RENDER_WGSL = `
@group(0) @binding(0)
var<uniform> uResolution : vec2<f32>;

@group(1) @binding(0)
var uSampler : sampler;

@group(1) @binding(1)
var uTexture : texture_2d<f32>;

@vertex
fn vs_main(@builtin(vertex_index) vertexIndex : u32) -> @builtin(position) vec4<f32> {
  var positions = array<vec2<f32>, 3>(
    vec2<f32>(-1.0, -3.0),
    vec2<f32>( 3.0,  1.0),
    vec2<f32>(-1.0,  1.0)
  );
  let pos = positions[vertexIndex];
  return vec4<f32>(pos, 0.0, 1.0);
}

@fragment
fn fs_main(@builtin(position) fragCoord : vec4<f32>) -> @location(0) vec4<f32> {
  let uv = fragCoord.xy / uResolution;
  let texColor = textureSample(uTexture, uSampler, uv);

  var c = texColor.rgb;
  let intensity = max(max(c.r, c.g), c.b);
  c = c * (1.2 + intensity * 1.6);
  c = clamp(c, vec3<f32>(0.0), vec3<f32>(1.0));
  return vec4<f32>(c, 1.0);
}
`;

const BLUR_WGSL = `
@group(0) @binding(0)
var srcTex : texture_2d<f32>;

@group(0) @binding(1)
var dstTex : texture_storage_2d<rgba16float, write>;

struct SimUniforms {
  time : f32,
  fade : f32,
  swirlStrength : f32,
  intensity : f32,
  color : vec3<f32>,
  pad0 : f32,
};

@group(0) @binding(2)
var<uniform> uSim : SimUniforms;

fn sample_tex(coord : vec2<i32>) -> vec4<f32> {
  let dims = textureDimensions(srcTex);
  let clamped = vec2<i32>(
    clamp(coord.x, 0, i32(dims.x) - 1),
    clamp(coord.y, 0, i32(dims.y) - 1)
  );
  return textureLoad(srcTex, clamped, 0);
}

@compute @workgroup_size(8, 8)
fn cs_main(@builtin(global_invocation_id) gid : vec3<u32>) {
  let dims = textureDimensions(srcTex);
  if (gid.x >= dims.x || gid.y >= dims.y) {
    return;
  }

  let coord = vec2<i32>(i32(gid.x), i32(gid.y));
  let uv = vec2<f32>(f32(gid.x), f32(gid.y)) / vec2<f32>(f32(dims.x), f32(dims.y));
  let angle = sin(uv.x * 9.0 + uSim.time * 1.4) + cos(uv.y * 11.0 - uSim.time * 1.2);
  let dir = vec2<f32>(-sin(angle), cos(angle));
  let swirlOffset = vec2<i32>(i32(dir.x * uSim.swirlStrength), i32(dir.y * uSim.swirlStrength));

  let center = sample_tex(coord + swirlOffset);
  let left   = sample_tex(coord + swirlOffset + vec2<i32>(-1,  0));
  let right  = sample_tex(coord + swirlOffset + vec2<i32>( 1,  0));
  let up     = sample_tex(coord + swirlOffset + vec2<i32>( 0, -1));
  let down   = sample_tex(coord + swirlOffset + vec2<i32>( 0,  1));

  let avg = (center * 4.0 + left + right + up + down) / 8.0;
  let faded = vec4<f32>(avg.rgb * uSim.fade, avg.a);
  textureStore(dstTex, coord, faded);
}
`;

const SPLAT_WGSL = `
struct Mouse {
  pos    : vec2<f32>,
  down   : f32,
  radius : f32,
  time   : f32,
  pad0   : f32,
  pad1   : f32,
  pad2   : f32,
};

@group(0) @binding(0)
var<uniform> uMouse : Mouse;

@group(0) @binding(1)
var dstTex : texture_storage_2d<rgba16float, write>;

@group(0) @binding(2)
var srcTex : texture_2d<f32>;

struct SimUniforms {
  time : f32,
  fade : f32,
  swirlStrength : f32,
  intensity : f32,
  color : vec3<f32>,
  pad0 : f32,
};

@group(0) @binding(3)
var<uniform> uSim : SimUniforms;

@compute @workgroup_size(8, 8)
fn cs_main(@builtin(global_invocation_id) gid : vec3<u32>) {
  let dims = textureDimensions(dstTex);
  if (gid.x >= dims.x || gid.y >= dims.y) {
    return;
  }

  let coord = vec2<f32>(f32(gid.x), f32(gid.y));
  let base = textureLoad(srcTex, vec2<i32>(i32(coord.x), i32(coord.y)), 0);
  var color = base;

  if (uMouse.down > 0.5) {
    let d = distance(coord, uMouse.pos);
    let r = uMouse.radius;
    if (d < r) {
      let falloff = 1.0 - (d / r);
      let boosted = uSim.color * falloff * (0.6 + uSim.intensity * 1.2);
      var newColor = color.rgb + boosted;
      newColor = clamp(newColor, vec3<f32>(0.0), vec3<f32>(3.0));
      color = vec4<f32>(newColor, color.a);
    }
  }

  textureStore(dstTex, vec2<i32>(i32(coord.x), i32(coord.y)), color);
}
`;

async function startSimulation() {
  try {
    const { device, context, format } = await initWebGPU(elements.canvas);

    const renderShader = device.createShaderModule({ code: RENDER_WGSL });
    const blurShader = device.createShaderModule({ code: BLUR_WGSL });
    const splatShader = device.createShaderModule({ code: SPLAT_WGSL });

    const renderPipeline = device.createRenderPipeline({
      layout: 'auto',
      vertex: { module: renderShader, entryPoint: 'vs_main' },
      fragment: { module: renderShader, entryPoint: 'fs_main', targets: [{ format }] },
      primitive: { topology: 'triangle-list' },
    });

    const blurPipeline = device.createComputePipeline({
      layout: 'auto',
      compute: { module: blurShader, entryPoint: 'cs_main' },
    });

    const splatPipeline = device.createComputePipeline({
      layout: 'auto',
      compute: { module: splatShader, entryPoint: 'cs_main' },
    });

    const resolutionBuffer = device.createBuffer({
      size: 8,
      usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
    });

    const mouseBuffer = device.createBuffer({
      size: 32,
      usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
    });

    const simUniformBuffer = device.createBuffer({
      size: 32,
      usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
    });

    const texDesc = {
      size: { width: SIM_WIDTH, height: SIM_HEIGHT, depthOrArrayLayers: 1 },
      format: 'rgba16float',
      usage: GPUTextureUsage.TEXTURE_BINDING | GPUTextureUsage.STORAGE_BINDING | GPUTextureUsage.COPY_DST,
    };

    const simTexture = device.createTexture(texDesc);
    const scratchTexture = device.createTexture(texDesc);

    const simViewSample = simTexture.createView();
    const simViewStorage = simTexture.createView();
    const scratchViewSample = scratchTexture.createView();
    const scratchViewStorage = scratchTexture.createView();

    const sampler = device.createSampler({
      magFilter: 'linear',
      minFilter: 'linear',
      addressModeU: 'clamp-to-edge',
      addressModeV: 'clamp-to-edge',
    });

    const seedData = new Uint16Array(SIM_WIDTH * SIM_HEIGHT * 4);
    for (let i = 0; i < seedData.length; i += 4) {
      seedData[i] = 0;
      seedData[i + 1] = 0;
      seedData[i + 2] = 0;
      seedData[i + 3] = 15360;
    }

    device.queue.writeTexture(
      { texture: simTexture },
      seedData,
      { offset: 0, bytesPerRow: SIM_WIDTH * 4 * 2, rowsPerImage: SIM_HEIGHT },
      { width: SIM_WIDTH, height: SIM_HEIGHT, depthOrArrayLayers: 1 }
    );
    device.queue.writeTexture(
      { texture: scratchTexture },
      seedData,
      { offset: 0, bytesPerRow: SIM_WIDTH * 4 * 2, rowsPerImage: SIM_HEIGHT },
      { width: SIM_WIDTH, height: SIM_HEIGHT, depthOrArrayLayers: 1 }
    );

    const resolutionBindGroup = device.createBindGroup({
      layout: renderPipeline.getBindGroupLayout(0),
      entries: [{ binding: 0, resource: { buffer: resolutionBuffer } }],
    });

    const renderBindGroup = device.createBindGroup({
      layout: renderPipeline.getBindGroupLayout(1),
      entries: [
        { binding: 0, resource: sampler },
        { binding: 1, resource: simViewSample },
      ],
    });

    const blurBindGroup = device.createBindGroup({
      layout: blurPipeline.getBindGroupLayout(0),
      entries: [
        { binding: 0, resource: scratchViewSample },
        { binding: 1, resource: simViewStorage },
        { binding: 2, resource: { buffer: simUniformBuffer } },
      ],
    });

    const splatBindGroup = device.createBindGroup({
      layout: splatPipeline.getBindGroupLayout(0),
      entries: [
        { binding: 0, resource: { buffer: mouseBuffer } },
        { binding: 1, resource: scratchViewStorage },
        { binding: 2, resource: simViewSample },
        { binding: 3, resource: { buffer: simUniformBuffer } },
      ],
    });

    const updateCanvasResolution = () => {
      const dpr = window.devicePixelRatio || 1;
      const width = Math.floor(elements.canvas.clientWidth * dpr);
      const height = Math.floor(elements.canvas.clientHeight * dpr);
      if (elements.canvas.width !== width || elements.canvas.height !== height) {
        elements.canvas.width = width;
        elements.canvas.height = height;
      }
      device.queue.writeBuffer(resolutionBuffer, 0, new Float32Array([width, height]));
    };

    const tick = () => {
      const t = performance.now() / 1000;
      const glowPulse = params.glowSpeed > 0 ? 0.75 + 0.25 * Math.sin(t * params.glowSpeed * 6.28318) : 1.0;
      const color = [
        (ledColor.r / 255) * glowPulse,
        (ledColor.g / 255) * glowPulse,
        (ledColor.b / 255) * glowPulse,
      ];

      device.queue.writeBuffer(
        mouseBuffer,
        0,
        new Float32Array([mouse.x, mouse.y, mouse.down ? 1 : 0, params.radius, t, 0, 0, 0])
      );

      device.queue.writeBuffer(
        simUniformBuffer,
        0,
        new Float32Array([t, params.fade, params.swirlStrength, params.intensity, color[0], color[1], color[2], 0])
      );

      const encoder = device.createCommandEncoder();
      const wx = Math.ceil(SIM_WIDTH / WORKGROUP_SIZE);
      const wy = Math.ceil(SIM_HEIGHT / WORKGROUP_SIZE);

      const compute = encoder.beginComputePass();
      compute.setPipeline(splatPipeline);
      compute.setBindGroup(0, splatBindGroup);
      compute.dispatchWorkgroups(wx, wy);
      compute.end();

      const computeBlur = encoder.beginComputePass();
      computeBlur.setPipeline(blurPipeline);
      computeBlur.setBindGroup(0, blurBindGroup);
      computeBlur.dispatchWorkgroups(wx, wy);
      computeBlur.end();

      const view = context.getCurrentTexture().createView();
      const renderPass = encoder.beginRenderPass({
        colorAttachments: [
          { view, clearValue: { r: 0, g: 0, b: 0, a: 1 }, loadOp: 'clear', storeOp: 'store' },
        ],
      });

      renderPass.setPipeline(renderPipeline);
      renderPass.setBindGroup(0, resolutionBindGroup);
      renderPass.setBindGroup(1, renderBindGroup);
      renderPass.draw(3, 1, 0, 0);
      renderPass.end();

      device.queue.submit([encoder.finish()]);
    };

    const frame = () => {
      updateCanvasResolution();
      tick();
      requestAnimationFrame(frame);
    };

    requestAnimationFrame(frame);
  } catch (err) {
    const message = err instanceof Error ? err.message : String(err);
    setError(message);
  }
}

applyParamsToInputs();
bindControls();
setupPointer(elements.canvas);

setInterval(refreshColor, 500);
refreshColor();
startSimulation();
