# The latest own-character position (x z) in a copy's packet log (its C>S MoveActorReq rows, actor class 0xA).
import sys, os
sys.argv = [sys.argv[0], '0', sys.argv[1], '0', '0']
here = os.path.dirname(os.path.abspath(__file__))
src = open(os.path.join(here, 'walk_to.py'), encoding='utf-8').read().split('def wrap(a):')[0]
exec(src)
p = fix()
print('%.2f %.2f' % p if p else '')
