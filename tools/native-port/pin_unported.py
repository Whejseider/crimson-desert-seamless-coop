# Co-op+ native port: give the build checks two constants. kPeTimestamp = 1.0.0.3549 (ported code) and
# kPeTimestampUnported = 2976. Every session2 module not listed in PORTED compares against kPeTimestampUnported, so
# on 3549 it stays off by its own build check until its addresses are ported. Idempotent.
#
#   python tools/native-port/pin_unported.py            (re-run after adding a module to PORTED)
import glob, os, re

ROOT = 'C:/Users/asher/CrimsonDesert-SeamlessCoop/upstream/cdmp-native/src/mod/'
PORTED = {'offsets_s2.h', 'server.cpp', 'guest.cpp', 'user2_funnels.cpp'}  # session2 files whose addresses are ported to 3549
# Files with their own local kPeTimestamp (still 2976): pinned by definition, left as they are.
OWN_CONSTANT = {'scene_level_share.cpp', 'scene_level_share_logic.h'}


def edit(path, fn):
    s = open(path, encoding='utf-8', newline='').read()
    t = fn(s)
    if t != s:
        open(path, 'w', encoding='utf-8', newline='').write(t)
        return True
    return False


def game_offsets(s):
    if 'kPeTimestampUnported' in s:
        return s
    s = s.replace('inline constexpr uint32_t kPeTimestamp = 0x6AB28F00;',
                  'inline constexpr uint32_t kPeTimestamp = 0x6AEB7D59;')
    return s.replace(
        'inline constexpr uint32_t kPeTimestamp = 0x6AEB7D59;',
        'inline constexpr uint32_t kPeTimestamp = 0x6AEB7D59;  // Co-op+ port: 1.0.0.3549 (was 0x6AB28F00, 2976)\n'
        '// Co-op+ port: modules whose addresses are not ported yet compare against this (build 2976), so on 3549\n'
        '// they stay off by their own build check (docs/research/native-port-3549.json).\n'
        'inline constexpr uint32_t kPeTimestampUnported = 0x6AB28F00;', 1)


def s2_offsets(s):
    if 'kPeTimestampUnported' in s:
        return s
    return s.replace('inline constexpr uint32_t kPeTimestamp = 0x6AB28F00;',
                     'inline constexpr uint32_t kPeTimestamp = 0x6AEB7D59;  // Co-op+ port: 1.0.0.3549 (was 0x6AB28F00)\n'
                     'inline constexpr uint32_t kPeTimestampUnported = 0x6AB28F00;  // modules not ported yet')


def pin(s):
    return re.sub(r'::kPeTimestamp\b', '::kPeTimestampUnported', s)


def unpin(s):
    return re.sub(r'::kPeTimestampUnported\b', '::kPeTimestamp', s)


edit(ROOT + 'game/offsets.h', game_offsets)
edit(ROOT + 'session2/offsets_s2.h', s2_offsets)
for f in sorted(glob.glob(ROOT + 'session2/*.cpp') + glob.glob(ROOT + 'session2/*.h')):
    name = os.path.basename(f)
    if name == 'offsets_s2.h' or name in OWN_CONSTANT:
        if name in OWN_CONSTANT: edit(f, unpin)
        continue
    if edit(f, unpin if name in PORTED else pin):
        print(('ported: ' if name in PORTED else 'pinned to 2976: ') + name)
