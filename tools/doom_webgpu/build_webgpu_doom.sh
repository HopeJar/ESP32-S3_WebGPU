#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DOOM_SRC_DIR="$ROOT_DIR/third_party/doomgeneric/doomgeneric"
PORT_SRC="$ROOT_DIR/tools/doom_webgpu/doomgeneric_webgpu.c"
OUT_DIR="$ROOT_DIR/main/web/web_pages/doom"
PARTITION_SIZE_BYTES=$((0x5f0000))
SPIFFS_HEADROOM_BYTES=$((512 * 1024))
SAFE_ASSET_BUDGET_BYTES=$((PARTITION_SIZE_BYTES - SPIFFS_HEADROOM_BYTES))
RUNTIME_OVERHEAD_BYTES=$((512 * 1024))

format_bytes() {
  local value="$1"
  awk -v bytes="$value" 'BEGIN { printf "%.2f MiB", bytes / 1048576 }'
}

if ! command -v emcc >/dev/null 2>&1; then
  echo "emcc was not found. Install/activate Emscripten in WSL first." >&2
  exit 1
fi

if [[ ! -d "$DOOM_SRC_DIR" ]]; then
  echo "doomgeneric source directory not found: $DOOM_SRC_DIR" >&2
  exit 1
fi

mkdir -p "$OUT_DIR"

if [[ -z "${1-}" || -z "${2-}" ]]; then
  echo "Usage: $0 --embed-iwad /full/path/to/your_iwad.wad" >&2
  echo "Any valid IWAD works here, including doom1.wad, doom2.wad, or freedoom1.wad." >&2
  exit 1
fi

case "$1" in
  --embed-iwad|--embed-wad)
    ;;
  *)
    echo "Usage: $0 --embed-iwad /full/path/to/your_iwad.wad" >&2
    echo "Any valid IWAD works here, including doom1.wad, doom2.wad, or freedoom1.wad." >&2
    exit 1
    ;;
esac

EMBED_IWAD="$2"

SOURCES=(
  dummy.c
  am_map.c
  doomdef.c
  doomstat.c
  dstrings.c
  d_event.c
  d_items.c
  d_iwad.c
  d_loop.c
  d_main.c
  d_mode.c
  d_net.c
  f_finale.c
  f_wipe.c
  g_game.c
  hu_lib.c
  hu_stuff.c
  info.c
  i_cdmus.c
  i_endoom.c
  i_joystick.c
  i_scale.c
  i_sound.c
  i_system.c
  i_timer.c
  i_input.c
  i_video.c
  i_sdlmusic.c
  i_sdlsound.c
  mus2mid.c
  memio.c
  m_argv.c
  m_bbox.c
  m_cheat.c
  m_config.c
  m_controls.c
  m_fixed.c
  m_menu.c
  m_misc.c
  m_random.c
  p_ceilng.c
  p_doors.c
  p_enemy.c
  p_floor.c
  p_inter.c
  p_lights.c
  p_map.c
  p_maputl.c
  p_mobj.c
  p_plats.c
  p_pspr.c
  p_saveg.c
  p_setup.c
  p_sight.c
  p_spec.c
  p_switch.c
  p_telept.c
  p_tick.c
  p_user.c
  r_bsp.c
  r_data.c
  r_draw.c
  r_main.c
  r_plane.c
  r_segs.c
  r_sky.c
  r_things.c
  sha1.c
  sounds.c
  statdump.c
  st_lib.c
  st_stuff.c
  s_sound.c
  tables.c
  v_video.c
  wi_stuff.c
  w_checksum.c
  w_file.c
  w_main.c
  w_wad.c
  z_zone.c
  w_file_stdc.c
  doomgeneric.c
)

SOURCE_PATHS=()
for source in "${SOURCES[@]}"; do
  SOURCE_PATHS+=("$DOOM_SRC_DIR/$source")
done

EMCC_FLAGS=(
  -O2
  -I"$DOOM_SRC_DIR"
  -DDOOMGENERIC_RESX=640
  -DDOOMGENERIC_RESY=400
  -DFEATURE_SOUND
  -sALLOW_MEMORY_GROWTH=1
  -sNO_EXIT_RUNTIME=1
  -sFORCE_FILESYSTEM=1
  -sENVIRONMENT=web
  -sUSE_SDL=2
  -sUSE_SDL_MIXER=2
  -s
  'SDL2_MIXER_FORMATS=["mid"]'
  '-sEXPORTED_FUNCTIONS=["_DGW_Boot","_DGW_Tick","_DGW_GetScreenBuffer","_DGW_GetScreenWidth","_DGW_GetScreenHeight","_DGW_PushKeyEvent"]'
  '-sEXPORTED_RUNTIME_METHODS=["FS"]'
)

if [[ ! -f "$EMBED_IWAD" ]]; then
  echo "IWAD file not found: $EMBED_IWAD" >&2
  exit 1
fi

IWAD_SIZE_BYTES=$(stat -c %s "$EMBED_IWAD")
ESTIMATED_TOTAL_BYTES=$((IWAD_SIZE_BYTES + RUNTIME_OVERHEAD_BYTES))

if (( ESTIMATED_TOTAL_BYTES > SAFE_ASSET_BUDGET_BYTES )); then
  echo "IWAD is too large for the current internal flash asset budget." >&2
  echo "  IWAD: $EMBED_IWAD ($(format_bytes "$IWAD_SIZE_BYTES"))" >&2
  echo "  Safe web asset budget: $(format_bytes "$SAFE_ASSET_BUDGET_BYTES")" >&2
  echo "Use a smaller IWAD such as shareware doom1.wad, or move Doom assets off internal flash." >&2
  exit 1
fi

EMCC_FLAGS+=(--preload-file "$EMBED_IWAD@/game.wad")

echo "Building browser Doom bundle into $OUT_DIR"
emcc "${EMCC_FLAGS[@]}" \
  "${SOURCE_PATHS[@]}" \
  "$PORT_SRC" \
  -o "$OUT_DIR/doomgeneric.js"

rm -f "$OUT_DIR/doomgeneric.html"

ACTUAL_TOTAL_BYTES=$(find "$OUT_DIR" -maxdepth 1 -type f ! -name ".gitkeep" -printf "%s\n" | awk "{sum += \$1} END {print sum + 0}")
if (( ACTUAL_TOTAL_BYTES > SAFE_ASSET_BUDGET_BYTES )); then
  echo "Generated Doom asset bundle is too large for the current SPIFFS budget." >&2
  echo "  Generated assets: $(format_bytes "$ACTUAL_TOTAL_BYTES")" >&2
  echo "  Safe web asset budget: $(format_bytes "$SAFE_ASSET_BUDGET_BYTES")" >&2
  echo "Use a smaller IWAD such as shareware doom1.wad, or move Doom assets off internal flash." >&2
  exit 1
fi

echo "Done. Generated assets: $(format_bytes "$ACTUAL_TOTAL_BYTES")"
