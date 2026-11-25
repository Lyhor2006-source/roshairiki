
# -*- coding: utf-8 -*-
import BigWorld
import os
try:
    unicode
except NameError:
    unicode = str
_PB_PATH = _RYOMENX_DIR + '/esp.txt'
_PB_STAT = _RYOMENX_DIR + '/esp_stat.txt'
class _PBNotBattle(Exception):
    pass
def _pb_is_player_type(tn):
    if not tn:
        return False
    bad = ('Space', 'Hall', 'Lobby', 'Account', 'Login', 'Select', 'Vehicle', 'Plane', 'Drone', 'Door', 'Box', 'Item', 'Drop')
    if any(b in tn for b in bad):
        return False
    return any(g in tn for g in ('BattleGround', 'Avatar', 'Athlete', 'Robot', 'Bot', 'Player', 'Hero'))
_PB_ACTION_TS = 0.0
_PB_DIAG_TS = 0.0
_PB_EMPTY = False
def _pb_health_value(entity, fields):
    import math
    for field in fields:
        try:
            value = getattr(entity, field, None)
            if value is None:
                continue
            value = value() if callable(value) else value
            if isinstance(value, bool):
                continue
            value = float(value)
            if not math.isnan(value) and not math.isinf(value) and value >= 0:
                return value
        except Exception:
            pass
    return None

def _pb_health(entity):
    import math
    hp = _pb_health_value(entity, ('hp', 'HP', 'health', 'curHp', 'getHp', 'getHP', 'getHealth'))
    maximum = _pb_health_value(entity, ('maxHp', 'maxHP', 'maxHealth', 'hpMax', 'getMaxHp', 'getMaxHP', 'getMaxHealth'))
    if hp is None:
        return -1, 100
    if maximum is None or maximum <= 0:
        maximum = 100.0
    if maximum <= 1.0 and hp <= 1.0:
        hp *= 100.0
        maximum *= 100.0
    return int(math.ceil(min(hp, maximum))), max(1, int(math.ceil(maximum)))

def _pb_publish(path, text):
    import os
    pending = path + '.new'
    with open(pending, 'w') as output:
        output.write(text)
    os.rename(pending, path)

def _pb_empty():
    global _PB_EMPTY
    if not _PB_EMPTY:
        _pb_publish(_PB_PATH, '0')
        _PB_EMPTY = True

_PB_GEN = globals().get('_PB_GEN', 0) + 1
def _pb_wrap(f, conv):
    def w2s(v):
        r = f(conv(v))
        if r is None:
            return None
        z = getattr(r, 'z', 1.0)
        if z is not None and float(z) <= 0:
            return None
        try:
            return float(r[0]), float(r[1])
        except Exception:
            return float(r.x), float(r.y)
    return w2s
def _pb_space():
    try:
        sp = BigWorld.player().space
        if sp is not None:
            return sp
    except Exception:
        pass
    for k in list(BigWorld.entities.keys()):
        e = BigWorld.entities[k]
        if 'Space' in type(e).__name__:
            return e
    return None
def _pb_w2s_get():
    me = BigWorld.player()
    test = me.position
    tries = []
    sp = _pb_space()
    if sp is not None:
        for cn in ['camera', '_camera', 'cam']:
            c = getattr(sp, cn, None)
            if c is None:
                continue
            for ec in [getattr(c, 'engine_camera', None), c]:
                if ec is None:
                    continue
                for fn in ['GetScreenPointFromWorldPoint', 'world_to_screen', 'WorldToScreenPoint']:
                    f = getattr(ec, fn, None)
                    if f is not None and callable(f):
                        tries.append((2, f, lambda v: v))
    try:
        import world, math3d
        c = world.get_active_scene().active_camera
        tries.append((3, c.world_to_screen, lambda v: math3d.vector(v.x, v.y, v.z)))
        tries.append((4, c.world_to_screen, lambda v: v))
    except Exception:
        pass
    for mode, f, conv in tries:
        try:
            w = _pb_wrap(f, conv)
            w(test)
            return mode, w
        except Exception:
            pass
    return 0, None
def _pb_feet(e):
    try:
        v = e.model.position
        if v is not None:
            return v
    except Exception:
        pass
    return e.position
def _pb_veh_pos(e):
    for an in ['vehicle', 'curVehicle', 'driveVehicle', 'ridingVehicle', 'carrier']:
        try:
            v = getattr(e, an, None)
            if v is not None and hasattr(v, 'position'):
                return v.position
        except Exception:
            pass
    for an in ['vehicleID', 'vehicleId', 'curVehicleID', 'driveVehicleId', 'rideVehicleId']:
        try:
            vid = getattr(e, an, None)
            if vid:
                ve = BigWorld.entities.get(vid)
                if ve is not None:
                    return ve.position
        except Exception:
            pass
    return None
def _pb_feet_s(e, w2s):
    for get in (lambda: e.model.position, lambda: e.position, lambda: _pb_veh_pos(e)):
        try:
            v = get()
            if v is None:
                continue
            r = w2s(v)
            if r is not None:
                return r
        except Exception:
            pass
    return None
def _pb_head(e):
    for b in ['biped Head', 'Bip001 Head', 'head', 'Head']:
        try:
            v = e.model.GetBoneWorldPosition(b)
            if v is not None:
                return v
        except Exception:
            pass
    return None
def _pb_head_pos(e, y_off=1.0):
    for get in (lambda: _pb_head(e), lambda: _pb_boneworld(e, 'head')):
        try:
            v = get()
            if v is not None:
                try:
                    import math3d
                    return math3d.vector(v.x, v.y + y_off, v.z)
                except Exception:
                    pass
                try:
                    v.y += y_off
                    return v
                except Exception:
                    pass
                return v
        except Exception:
            pass
    return None
_PB_CFG = _RYOMENX_DIR + '/aim_cfg.txt'
_PB_AC = {'t': 0, 'c': None}
def _pb_aimcfg():
    _PB_AC['t'] += 1
    if _PB_AC['c'] is None or _PB_AC['t'] % 15 == 0:
        try:
            _PB_AC['c'] = [float(x) for x in open(_PB_CFG).read().split()]
        except Exception:
            pass
    return _PB_AC['c']
_PB_TRG = {'aim': 0, 'selected': 0, 'rotated': 0}
def _pb_flag(objects, names):
    for obj in objects:
        if obj is None:
            continue
        for name in names:
            try:
                v = getattr(obj, name, False)
                if callable(v):
                    v = v()
                if bool(v):
                    return True
            except Exception:
                pass
    return False
def _pb_aim_screen(pt, W, H, metrics):
    x, y = float(pt[0]), float(pt[1])
    if abs(x) <= 2.0 and abs(y) <= 2.0:
        return x * W, y * H
    if len(metrics) >= 6:
        fbw, fbh, vpx, vpy, vpw, vph = metrics[:6]
        if fbw > 0.0 and fbh > 0.0:
            vp_top = fbh - (vpy + vph)
            return (vpx + x) * W / fbw, (vp_top + y) * H / fbh
    return x, y
def _pb_aim(me, cands, allc=None):
    import math
    _PB_TRG['aim'] = 0; _PB_TRG['selected'] = 0; _PB_TRG['rotated'] = 0
    c = _pb_aimcfg()
    if not c or len(c) < 11 or c[0] < 0.5:
        return
    en, mode, fov, smooth, power, vischk, ignkn, bone, W, H, maxd = c[:11]
    metrics = c[16:22] if len(c) >= 22 else []
    cc = getattr(me, 'camCtrlComp', None)
    if cc is None:
        return
    scoped = _pb_flag((cc, me), ('_gunSightOpened', 'isGunSightOpened', 'gunSightOpened', 'isScoped'))
    firing = _pb_flag((me, cc), ('isShooting', 'fireBtnPressed', 'isFiring', 'isAttacking', '_isFiring'))
    m = int(mode)
    if m == 1 and not scoped:
        return
    if m == 2 and (not firing or scoped):
        return
    if m >= 3 and not (scoped and firing):
        return

    cx = W * 0.5; cy = H * 0.5
    fr = 0.0 if int(bone) == 0 else (0.30 if int(bone) == 1 else 0.55)
    best = None; bd = fov
    try:
        cam = _pb_cam_pos(me)
    except Exception:
        cam = me.position

    for (e, fs, hs, kn, vis, p, dist) in cands:
        if _pb_enemy(me, e) != 1:
            continue
        if ignkn >= 0.5 and kn:
            continue
        if vischk >= 0.5 and not vis:
            continue
        if dist / 10.0 > maxd:
            continue
        fx, fy = _pb_aim_screen(fs, W, H, metrics)
        hx, hy = _pb_aim_screen(hs, W, H, metrics)
        ax = hx; ay = hy + fr * (fy - hy)
        d = ((ax - cx) ** 2 + (ay - cy) ** 2) ** 0.5
        if d < bd:
            bd = d; best = (e, p)
    if best is None:
        return
    e, p = best
    _PB_TRG['selected'] = 1
    try:
        _PB_TRG['aim'] = _pb_vis(me, e, cam)
    except Exception:
        _PB_TRG['aim'] = 1

    key = ['head', 'chest', 'pelvis'][min(2, max(0, int(bone)))]
    target = _pb_boneworld(e, key)
    if target is not None:
        tx, ty, tz = target.x, target.y, target.z
    else:
        try:
            center = e.getModelCenter()
            top = p.y + 2.0 * (center.y - p.y)
            tx, tz = center.x, center.z
            ty = top - fr * (top - p.y)
        except Exception:
            tx, ty, tz = p.x, p.y + (17.5 * (1.0 - fr)), p.z
    dx = tx - cam.x; dy = ty - cam.y; dz = tz - cam.z
    length = (dx * dx + dy * dy + dz * dz) ** 0.5
    if length < 0.001:
        return
    try:
        vd = me.getViewDirection()
        cur_yaw = math.atan2(vd.x, vd.z)
        cur_pitch = math.asin(max(-1.0, min(1.0, vd.y)))
    except Exception:
        return
    want_yaw = math.atan2(dx, dz)
    want_pitch = math.asin(max(-1.0, min(1.0, dy / length)))
    yaw_error = want_yaw - cur_yaw
    pitch_error = want_pitch - cur_pitch
    while yaw_error > math.pi: yaw_error -= 2.0 * math.pi
    while yaw_error < -math.pi: yaw_error += 2.0 * math.pi
    while pitch_error > math.pi: pitch_error -= 2.0 * math.pi
    while pitch_error < -math.pi: pitch_error += 2.0 * math.pi
    if math.isnan(yaw_error) or math.isnan(pitch_error) or math.isinf(yaw_error) or math.isinf(pitch_error):
        return

    raw_p = float(power) if power is not None else 100.0
    if raw_p > 1.0:
        assist_ratio = max(10.0, min(100.0, raw_p)) / 100.0
    else:
        assist_ratio = max(0.10, min(1.0, raw_p))
    factor_x = abs(float(getattr(cc, '_rotate_factor_x', 0.2) or 0.2))
    factor_y = abs(float(getattr(cc, '_rotate_factor_y', factor_x) or factor_x))
    factor_x = max(0.001, factor_x); factor_y = max(0.001, factor_y)

    # Pure percentage-based aim assist: exactly assist_ratio toward target
    ux = (math.degrees(yaw_error) / factor_x) * assist_ratio
    uy = (-math.degrees(pitch_error) / factor_y) * assist_ratio

    ux = max(-45.0 / factor_x, min(45.0 / factor_x, ux))
    uy = max(-45.0 / factor_y, min(45.0 / factor_y, uy))
    if abs(ux) + abs(uy) < 0.005:
        return

    calls = [lambda: cc.onRotateCameraFromUser((ux, uy)),
             lambda: cc.onRotateCameraFromUser((ux, uy), 0),
             lambda: cc.onRotateCameraFromUser(ux, uy)]
    try:
        import math3d
        calls.append(lambda: cc.onRotateCameraFromUser(math3d.vector2(ux, uy)))
    except Exception:
        pass
    for call in calls:
        try:
            call(); _PB_TRG['rotated'] = 1; break
        except Exception:
            pass

_PB_AF = {'on': False, 'owner': None}
def _pb_fire_objects(me):
    result = []; seen = set()
    def add(obj):
        if obj is None or id(obj) in seen:
            return
        seen.add(id(obj)); result.append(obj)
    add(me)
    for name in ('weaponComp', 'weaponLogicComp', 'shootWeaponComp', 'weaponMgr',
                 'weaponManager', 'gunCtrlComp', 'shootComp', 'cell', 'base'):
        try: add(getattr(me, name, None))
        except Exception: pass
    for owner in list(result):
        for name in ('curWeapon', 'currentWeapon', 'activeWeapon', 'holdingWeapon',
                     'weapon', 'getCurWeapon', 'getCurrentWeapon'):
            try:
                value = getattr(owner, name, None)
                if callable(value): value = value()
                add(value)
            except Exception:
                pass
    return result
def _pb_fire_invoke(objects, starting):
    if starting:
        names = ('startFire', 'StartFire', 'startFireSimple', 'StartFireSimple',
                 'startShooting', 'beginFire', 'onFireButtonDown', 'onFireKeyDown',
                 'pressFire', 'requestStartFire')
        signatures = ((), (0,), (True,))
    else:
        names = ('stopFire', 'StopFire', 'stopShooting', 'endFire',
                 'onFireButtonUp', 'onFireKeyUp', 'releaseFire', 'requestStopFire')
        signatures = ((), (0,), (False,))
    for obj in objects:
        for name in names:
            try:
                fn = getattr(obj, name, None)
                if fn is None or not callable(fn):
                    continue
                for args in signatures:
                    try:
                        fn(*args)
                        return obj
                    except Exception:
                        pass
            except Exception:
                pass
    return None
def _pb_autofire(me, cands):
    c = _pb_aimcfg()
    if not c or len(c) < 11 or c[0] < 0.5:
        return
    enabled = bool(len(c) >= 24 and c[22] >= 0.5)
    max_range = max(5.0, min(500.0, c[23] if c and len(c) >= 24 else 80.0))
    should_fire = False
    if enabled and cands:
        fov, W, H = c[2], c[8], c[9]
        metrics = c[16:22] if len(c) >= 22 else []
        cx, cy = W * 0.5, H * 0.5
        for e, fs, hs, kn, vis, p, dist in cands:
            if _pb_enemy(me, e) != 1:
                continue
            if kn or not vis or dist / 10.0 > max_range:
                continue
            hx, hy = _pb_aim_screen(hs, W, H, metrics)
            fx, fy = _pb_aim_screen(fs, W, H, metrics)
            import math
            if any(math.isnan(v) or math.isinf(v) for v in (hx, hy, fx, fy, dist)):
                continue
            height = max(12.0, abs(fy - hy))
            half_width = height * 0.275
            body_x = (hx + fx) * 0.5
            nearest_x = max(body_x - half_width, min(cx, body_x + half_width))
            nearest_y = max(min(hy, fy), min(cy, max(hy, fy)))
            if (nearest_x - cx) ** 2 + (nearest_y - cy) ** 2 <= max(0.0, fov) ** 2:
                should_fire = True
                break
    objects = _pb_fire_objects(me)
    if should_fire:
        if not _PB_AF['on']:
            cc = getattr(me, 'camCtrlComp', None)
            if not _pb_flag((me, cc), ('isShooting', 'fireBtnPressed', 'isFiring', 'isAttacking', '_isFiring')):
                owner = _pb_fire_invoke(objects, True)
                if owner is not None:
                    _PB_AF['on'] = True; _PB_AF['owner'] = owner
    elif _PB_AF['on']:
        ordered = [_PB_AF['owner']] + [obj for obj in objects if obj is not _PB_AF['owner']]
        _pb_fire_invoke(ordered, False)
        _PB_AF['on'] = False; _PB_AF['owner'] = None

_PB_BONES = [('head', 'biped Head'), ('neck', 'biped Neck'), ('chest', 'biped Spine2'),
    ('spine', 'biped Spine'), ('pelvis', 'biped Pelvis'),
    ('limbs_l_shoulder', 'biped L UpperArm'), ('limbs_r_shoulder', 'biped R UpperArm'),
    ('limbs_l_elbow', 'biped L Forearm'), ('limbs_r_elbow', 'biped R Forearm'),
    ('limbs_l_hand', 'biped L Hand'), ('limbs_r_hand', 'biped R Hand'),
    ('limbs_l_hip', 'biped L Thigh'), ('limbs_r_hip', 'biped R Thigh'),
    ('limbs_l_knee', 'biped L Calf'), ('limbs_r_knee', 'biped R Calf'),
    ('limbs_l_foot', 'biped L Foot'), ('limbs_r_foot', 'biped R Foot')]
_PB_BONE_NAME = dict(_PB_BONES)
_PB_SKEL = ['head', 'neck', 'chest', 'pelvis',
    'limbs_l_shoulder', 'limbs_l_elbow', 'limbs_l_hand',
    'limbs_r_shoulder', 'limbs_r_elbow', 'limbs_r_hand',
    'limbs_l_hip', 'limbs_l_knee', 'limbs_l_foot',
    'limbs_r_hip', 'limbs_r_knee', 'limbs_r_foot']
_PB_BS = {'s': None, 'fail': 0}
_PB_BMAP = {}
_PB_PARTPAT = {'head': [' head '], 'neck': [' neck '],
    'chest': [' spine2 ', ' spine 2 ', ' spine1 ', ' spine 1 ', ' chest '], 'spine': [' spine '],
    'pelvis': [' pelvis ', ' hips '], 'shoulder': [' upperarm ', ' upper arm ', ' uparm ', ' shoulder '],
    'elbow': [' forearm ', ' lowerarm ', ' fore arm ', ' elbow '], 'hand': [' hand ', ' wrist '],
    'hip': [' thigh ', ' upleg ', ' upper leg ', ' hip '],
    'knee': [' calf ', ' lowerleg ', ' lower leg ', ' shin ', ' knee '], 'foot': [' foot ', ' ankle ']}
def _pb_norm(n):
    return ' ' + str(n).lower().replace('_', ' ').replace('-', ' ').replace('.', ' ') + ' '
def _pb_model(e):
    try:
        m = e._mesh.underlyingModel
        if m is not None:
            return m
    except Exception:
        pass
    try:
        return e.appear.mesh.underlyingModel
    except Exception:
        return None
def _pb_bonemap(e, m):
    k = getattr(e, 'id', 0)
    try:
        token = (id(m), int(m.get_bone_count()))
    except Exception:
        token = (id(m), 0)
    c = _PB_BMAP.get(k)
    if c is not None and c[0] == token:
        return c[1]
    names = []
    try:
        for i in range(int(m.get_bone_count())):
            names.append((i, _pb_norm(m.get_bone_name(i))))
    except Exception:
        pass
    mp = {}
    for key, bn in _PB_BONES:
        exact = _pb_norm(bn)
        exact_hit = [i for i, n in names if n == exact]
        if exact_hit:
            mp[key] = exact_hit[0]
            continue
        if key.startswith('limbs_'):
            sd = key[6]; part = key[8:]
            for i, n in names:
                if 'nub' in n or 'twist' in n or 'end' in n:
                    continue
                if not any(pt in n for pt in _PB_PARTPAT[part]):
                    continue
                if (' %s ' % sd) in n or (sd == 'l' and ' left ' in n) or (sd == 'r' and ' right ' in n):
                    mp[key] = i
                    break
        else:
            for pt in _PB_PARTPAT[key]:
                hit = [i for i, n in names if pt in n and 'nub' not in n and 'end' not in n]
                if hit:
                    mp[key] = hit[0]
                    break
    if len(_PB_BMAP) > 400:
        _PB_BMAP.clear()
    _PB_BMAP[k] = (token, mp)
    return mp
def _pb_bw_try(m, b, s):
    import world
    if s == 0:
        return m.get_bone_matrix(b, world.SPACE_TYPE_WORLD).translation
    if s == 1:
        return m.get_bone_matrix(b, 1).translation
    if s == 2:
        return m.get_bone_matrix(b, 2).translation
    return None
def _pb_boneworld(e, key):
    primary = _PB_BONE_NAME.get(key)
    if primary is not None:
        for bone_name in (primary, primary.replace('biped', 'Bip001'), primary.replace('biped ', '')):
            try:
                v = e.model.GetBoneWorldPosition(bone_name)
                if v is not None:
                    return v
            except Exception:
                pass
    s = _PB_BS['s']
    m = _pb_model(e)
    if m is None:
        return None
    mp = _pb_bonemap(e, m)
    b = mp.get(key)
    if b is None:
        return None
    if s is None:
        hb = mp.get('head')
        try:
            c = e.getModelCenter()
        except Exception:
            return None
        if hb is None:
            return None
        for i in range(3):
            try:
                v = _pb_bw_try(m, hb, i)
                d = ((v.x - c.x) ** 2 + (v.y - c.y) ** 2 + (v.z - c.z) ** 2) ** 0.5
                if d < 30.0 and v.y > c.y:
                    _PB_BS['s'] = i; s = i
                    break
            except Exception:
                pass
        if s is None:
            _PB_BS['fail'] += 1
            return None
    try:
        return _pb_bw_try(m, b, s)
    except Exception:
        _PB_BS['s'] = None
        return None
def _pb_joint_screen(e, key, w2s):
    if w2s is None:
        return None
    try:
        import math
        v = _pb_boneworld(e, key)
        if v is None:
            return None
        xyz = (float(v.x), float(v.y), float(v.z))
        if any(math.isnan(q) or math.isinf(q) or abs(q) > 100000000.0 for q in xyz):
            return None
        try:
            root = e.position
            d2 = (v.x - root.x) ** 2 + (v.y - root.y) ** 2 + (v.z - root.z) ** 2
            if d2 > 1000000.0:
                return None
        except Exception:
            pass
        s = w2s(v)
        if s is None:
            return None
        sx, sy = float(s[0]), float(s[1])
        if math.isnan(sx) or math.isnan(sy) or math.isinf(sx) or math.isinf(sy):
            return None
        if abs(sx) > 100000000.0 or abs(sy) > 100000000.0:
            return None
        return sx, sy
    except Exception:
        return None
_PB_NRS = {'cls': None, 'try': 0, 'nr': False, 'ns': False, 'sil': False, 'idq': None}
_PB_SIL = {'t': None, 'ts': 0.0, 'cam': None, 'n': 0, 'tv': 0, 'dist': 0.0}
def _pb_idq(q):
    c = _PB_NRS['idq']
    if c is not None:
        return c
    Q = type(q)
    for mk in (lambda: Q.identity, lambda: Q(0.0, 0.0, 0.0, 1.0), lambda: Q()):
        try:
            v = mk()
            if abs(v.x) < 1e-6 and abs(v.y) < 1e-6 and abs(v.z) < 1e-6:
                _PB_NRS['idq'] = v
                return v
        except Exception:
            pass
    return None
def _pb_wq(orig, flag):
    def w(self, *a, **k):
        r = orig(self, *a, **k)
        if _PB_NRS[flag]:
            try:
                i = _pb_idq(r)
                if i is not None:
                    return i
            except Exception:
                pass
        return r
    return w
def _pb_wv(orig, flag, val):
    def w(self, *a, **k):
        if _PB_NRS[flag]:
            return val
        return orig(self, *a, **k)
    return w
def _pb_isvec(v):
    return hasattr(v, 'x') and hasattr(v, 'y') and hasattr(v, 'z') and not hasattr(v, 'w')
def _pb_redir(v, t, org):
    if _pb_isvec(v):
        L = (v.x * v.x + v.y * v.y + v.z * v.z) ** 0.5
        dx = t[0] - org[0]; dy = t[1] - org[1]; dz = t[2] - org[2]
        n = (dx * dx + dy * dy + dz * dz) ** 0.5
        if n < 1e-3:
            return v
        if 0.9 < L < 1.1:
            return type(v)(dx / n, dy / n, dz / n)
        ox = v.x - org[0]; oy = v.y - org[1]; oz = v.z - org[2]
        d = (ox * ox + oy * oy + oz * oz) ** 0.5
        if d > 50.0:
            target_dist = max(d, n + 60.0)
            return type(v)(org[0] + dx / n * target_dist, org[1] + dy / n * target_dist, org[2] + dz / n * target_dist)
        return v
    if isinstance(v, tuple):
        return tuple([_pb_redir(x, t, org) for x in v])
    if isinstance(v, list):
        return [_pb_redir(x, t, org) for x in v]
    return v
def _pb_origin(it, cam):
    items = it if isinstance(it, (tuple, list)) else [it]
    for x in items:
        if _pb_isvec(x):
            L = (x.x * x.x + x.y * x.y + x.z * x.z) ** 0.5
            if not (0.9 < L < 1.1) and cam is not None:
                if abs(x.x - cam[0]) + abs(x.y - cam[1]) + abs(x.z - cam[2]) < 60.0:
                    return (x.x, x.y, x.z)
    return cam
def _pb_silgen(g, t, cam):
    for it in g:
        try:
            org = _pb_origin(it, cam)
            if org is None:
                yield it
            else:
                _PB_SIL['n'] += 1
                yield _pb_redir(it, t, org)
        except Exception:
            yield it
def _pb_wray(orig):
    def w(self, *a, **k):
        g = orig(self, *a, **k)
        try:
            import time
            if not _PB_NRS['sil']:
                return g
            t = _PB_SIL['t']
            if t is None or time.time() - _PB_SIL['ts'] > 0.3:
                return g
            return _pb_silgen(g, t, _PB_SIL['cam'])
        except Exception:
            return g
    return w
def _pb_silent(me, cands, allc):
    import time
    if not _PB_NRS['sil']:
        _PB_SIL['t'] = None
        _PB_SIL['tv'] = 0
        return
    c = _pb_aimcfg()
    if not c or len(c) < 14 or c[0] < 0.5:
        return
    fov, vischk, ignkn, bone, W, H, maxd = c[2], c[5], c[6], c[7], c[8], c[9], c[11]
    s360 = len(c) >= 15 and c[14] >= 0.5
    cx = W * 0.5; cy = H * 0.5
    fr = 0.0 if int(bone) == 0 else (0.30 if int(bone) == 1 else 0.55)
    best = None; bd = fov
    if s360:
        bd2 = 1e18
        for (e2, kn2, vis2, p2, dist2) in allc:
            if _pb_enemy(me, e2) != 1:
                continue
            if vischk >= 0.5 and not vis2:
                continue
            if ignkn >= 0.5 and kn2:
                continue
            if dist2 / 10.0 > maxd:
                continue
            if dist2 < bd2:
                bd2 = dist2; best = (e2, p2, dist2)
    for (e, fs, hs, kn, vis, p, dist) in ([] if s360 else cands):
        if _pb_enemy(me, e) != 1:
            continue
        if ignkn >= 0.5 and kn:
            continue
        if vischk >= 0.5 and not vis:
            continue
        if dist / 10.0 > maxd:
            continue
        metrics = c[16:22] if len(c) >= 22 else []
        fx, fy = _pb_aim_screen(fs, W, H, metrics)
        hx, hy = _pb_aim_screen(hs, W, H, metrics)
        ax = hx; ay = hy + fr * (fy - hy)
        d = ((ax - cx) ** 2 + (ay - cy) ** 2) ** 0.5
        if d < bd:
            bd = d; best = (e, p, dist)
    if best is None:
        _PB_SIL['t'] = None
        _PB_SIL['tv'] = 0
        return
    e, p, edist = best
    try:
        _PB_SIL['tv'] = _pb_vis(me, e, me.getViewPos())
    except Exception:
        _PB_SIL['tv'] = 0
    if int(bone) == 0:
        hp = _pb_head_pos(e, 1.0)
        if hp is not None:
            t = (hp.x, hp.y, hp.z)
        else:
            mc = None
            try:
                mc = e.getModelCenter()
            except Exception:
                pass
            if mc is not None:
                top = p.y + 2.0 * (mc.y - p.y)
                t = (mc.x, top - 0.05 * (top - p.y), mc.z)
            else:
                t = (p.x, p.y + 17.2, p.z)
    else:
        bw = _pb_boneworld(e, ['head', 'chest', 'pelvis'][min(2, max(0, int(bone)))])
        if bw is not None:
            t = (bw.x, bw.y, bw.z)
        else:
            mc = None
            try:
                mc = e.getModelCenter()
            except Exception:
                pass
            if mc is not None:
                top = p.y + 2.0 * (mc.y - p.y)
                t = (mc.x, top - fr * (top - p.y), mc.z)
            else:
                hy = 11.0 if int(bone) == 1 else 7.5
                t = (p.x, p.y + hy, p.z)
    try:
        v = me.getViewPos()
        _PB_SIL['cam'] = (v.x, v.y, v.z)
    except Exception:
        _PB_SIL['cam'] = None
    _PB_SIL['t'] = t
    _PB_SIL['ts'] = time.time()
    _PB_SIL['dist'] = edist
def _pb_nrs_install():
    import gc
    cls = None
    for o in gc.get_objects():
        try:
            t = type(o)
            if t.__name__ == 'MechanicalEvaluator' and not isinstance(o, type):
                cls = t
                break
        except Exception:
            pass
    if cls is None:
        return
    ray = getattr(cls, 'getRayTrajectory', None)
    if not callable(ray):
        return
    if not getattr(ray, '_pb_silent_wrapped', False):
        try:
            wrapped = _pb_wray(ray)
            wrapped._pb_silent_wrapped = True
            setattr(cls, 'getRayTrajectory', wrapped)
        except Exception:
            return
    _PB_NRS['cls'] = cls
def _pb_nrs_tick():
    c = _pb_aimcfg()
    if not c or len(c) < 14:
        _PB_NRS['nr'] = False
        _PB_NRS['ns'] = False
        _PB_NRS['sil'] = False
        _PB_SIL['t'] = None
        _PB_SIL['tv'] = 0
        return
    _PB_NRS['nr'] = False
    _PB_NRS['ns'] = False
    _PB_NRS['sil'] = (len(c) >= 14 and c[13] >= 0.5) or (len(c) >= 15 and c[14] >= 0.5)
    if (_PB_NRS['nr'] or _PB_NRS['ns'] or _PB_NRS['sil']) and _PB_NRS['cls'] is None:
        _PB_NRS['try'] += 1
        if _PB_NRS['try'] % 300 == 1:
            _pb_nrs_install()
_PB_VIS = {}
def _pb_cam_pos(me):
    try:
        import world
        ac = world.get_active_scene().active_camera
        if ac is not None:
            p = getattr(ac, 'position', None) or getattr(ac, 'world_position', None)
            if p is not None and hasattr(p, 'x'): return p
    except Exception: pass
    try:
        sp = _pb_space()
        if sp is not None:
            for cn in ('camera', '_camera', 'cam'):
                c = getattr(sp, cn, None)
                if c is None: continue
                for obj in (getattr(c, 'engine_camera', None), c):
                    if obj is None: continue
                    p = getattr(obj, 'position', None) or getattr(obj, 'world_position', None)
                    if p is not None and hasattr(p, 'x'): return p
    except Exception: pass
    try: return me.getViewPos()
    except Exception: return me.position
def _pb_vis(me, e, cam):
    import time, sys
    k = getattr(e, 'id', 0)
    now = time.time()
    c = _PB_VIS.get(k)
    if c is not None and now - c[0] < 0.2:
        return c[1]
    v = 0
    try:
        origins = []
        if cam is not None and hasattr(cam, 'x'): origins.append(cam)
        try:
            vp = me.getViewPos()
            if vp is not None and hasattr(vp, 'x') and vp not in origins: origins.append(vp)
        except Exception: pass
        if not origins: origins.append(me.position)
        h_aim = _pb_head_pos(e, 1.0)
        h_raw = _pb_head(e)
        if h_raw is None: h_raw = _pb_boneworld(e, 'head')
        if h_raw is None:
            try:
                mc = e.getModelCenter()
                h_raw = type(me.position)(mc.x, mc.y + 16.0, mc.z)
            except Exception: h_raw = None
        if h_raw is None:
            try:
                p_ent = getattr(e, 'position', None)
                if p_ent is not None:
                    h_raw = type(me.position)(p_ent.x, p_ent.y + 16.5, p_ent.z)
            except Exception: pass
        if h_raw is None and h_aim is None: raise Exception('no head pos')
        targets = []
        if h_aim is not None: targets.append(h_aim)
        try:
            import math3d
            base = h_aim if h_aim is not None else h_raw
            targets.append(math3d.vector(base.x, base.y + 0.6, base.z))
        except Exception: pass
        if h_raw is not None and h_raw not in targets: targets.append(h_raw)
        su = sys.modules.get('shoot_utils')
        if su is not None and hasattr(su, 'isCanBeShot'):
            for org in origins:
                for tgt in targets:
                    try:
                        if su.isCanBeShot(org, tgt): v = 1; break
                    except Exception: pass
                if v == 1: break
        sp = _pb_space()
        if v == 0 and sp is not None and hasattr(sp, 'RawRaycast'):
            target_id = getattr(e, 'id', None)
            for org in origins:
                for tgt in targets:
                    try:
                        rc = sp.RawRaycast(org, 150.0, 19, with_trigger=False, to_pos=tgt)
                        if rc is None: v = 1; break
                        flags = getattr(rc, 'Flags', 0)
                        body = getattr(rc, 'Body', None)
                        if flags == 7 and body is None: v = 1; break
                        if body is not None:
                            oid = getattr(body, 'ownerid', None) or getattr(body, 'entity_id', None)
                            if oid == target_id: v = 1; break
                    except Exception: pass
                if v == 1: break
        if v == 0:
            try:
                if me.canSee(e): v = 1
            except Exception: pass
        if v == 0:
            try:
                if e.canSee(me): v = 1
            except Exception: pass
    except Exception: pass
    if len(_PB_VIS) > 300: _PB_VIS.clear()
    _PB_VIS[k] = (now, v)
    return v
def _pb_team_id(entity):
    ids = set()
    for record in (getattr(entity, 'campType', None), entity):
        if record is None:
            continue
        for name in ('teamID', 'teamId', 'team_id'):
            try:
                value = record.get(name) if isinstance(record, dict) else getattr(record, name, None)
                if callable(value):
                    value = value()
                if value is not None and int(value) > 0:
                    ids.add(int(value))
            except Exception:
                pass
    return next(iter(ids)) if len(ids) == 1 else None

def _pb_squad_member(me, entity):
    try:
        entity_id = int(entity.id)
        if entity_id > 0 and entity_id in getattr(me, 'fixedBattleTeamMateIds', ()):
            return True
    except Exception:
        entity_id = None
    try:
        roster = me.validBattleTeammates
    except Exception:
        roster = ()
    names = set()
    for field in ('nickname', 'playerName'):
        try:
            name = getattr(entity, field, None)
            if isinstance(name, (str, unicode)) and name:
                names.add(name)
        except Exception:
            pass
    try:
        for member in roster:
            try:
                member_id = member.get('id') if isinstance(member, dict) else getattr(member, 'id', None)
                if entity_id is not None and member_id is not None and int(member_id) == entity_id:
                    return True
            except Exception:
                pass
            for field in ('nickname', 'playerName'):
                try:
                    name = member.get(field) if isinstance(member, dict) else getattr(member, field, None)
                    if name and name in names:
                        return True
                except Exception:
                    pass
    except Exception:
        pass
    return False

def _pb_enemy(me, entity):
    if me is None or entity is None:
        return -1
    if entity is me:
        return 0
    try:
        if me.id == entity.id:
            return 0
    except Exception:
        pass
    if _pb_squad_member(me, entity):
        return 0
    my_team, other_team = _pb_team_id(me), _pb_team_id(entity)
    if my_team is not None and other_team == my_team:
        return 0
    method_enemy = None
    for owner, other in ((me, entity), (entity, me)):
        try:
            method = getattr(owner, 'IsSameTeam', None)
            if callable(method):
                same = method(other)
                if same is True or same == 1:
                    return 0
                if same is False or same == 0:
                    method_enemy = 1
        except Exception:
            pass
    if my_team is not None and other_team is not None:
        return 1
    return method_enemy if method_enemy is not None else -1

def _pb_weapon_kind(entity):
    kinds = (
        (6, ('grenade', 'molotov', 'throwable')),
        (5, ('pistol', 'glock', 'desert eagle')),
        (4, ('shotgun', 'm870', 'saiga', 'aa12')),
        (3, ('sniper', 'awm', 'barrett', 'svd', 'qbu')),
        (2, ('smg', 'mp5', 'mp7', 'vector', 'p90', 'pp19', 'uzi')),
        (1, ('rifle', 'ak47', 'akm', 'm4a1', 'm416', 'scar', 'an94', 'aug')))
    for obj in _pb_fire_objects(entity):
        for field in ('weaponName', 'gunName', 'itemName', 'name'):
            if field == 'name' and obj is entity:
                continue
            try:
                name = getattr(obj, field, None)
                if callable(name):
                    name = name()
                if not isinstance(name, (str, unicode)):
                    continue
                name = name.lower()
                for kind, fragments in kinds:
                    if any(fragment in name for fragment in fragments):
                        return kind
            except Exception:
                pass
    return 0

_PB_ORIG_FOV = {}
_PB_LAST_IPAD_STATE = {'on': None, 'fov': 0.0}
def _pb_apply_ipad_fov(c):
    if not c or len(c) < 29:
        return
    ipad_on = c[27] >= 0.5
    ipad_deg = float(c[28]) if c[28] >= 40.0 else 105.0
    global _PB_LAST_IPAD_STATE, _PB_ORIG_FOV

    state_changed = (_PB_LAST_IPAD_STATE['on'] != ipad_on or abs(_PB_LAST_IPAD_STATE['fov'] - ipad_deg) > 0.5)

    try:
        import world
        scene = world.get_active_scene() if hasattr(world, 'get_active_scene') else None
        ac = getattr(scene, 'active_camera', None) if scene else None
        if ac is not None:
            if 'active_cam' not in _PB_ORIG_FOV:
                curr = getattr(ac, 'fov', 55.0)
                _PB_ORIG_FOV['active_cam'] = curr if (curr >= 40.0 and curr <= 85.0) else 55.0

            if ipad_on:
                curr = getattr(ac, 'fov', 55.0)
                if state_changed or (curr >= 45.0 and abs(curr - ipad_deg) > 1.0):
                    ac.fov = ipad_deg
            else:
                if state_changed:
                    ac.fov = _PB_ORIG_FOV.get('active_cam', 55.0)
    except Exception:
        pass

    try:
        import BigWorld
        cam = BigWorld.camera() if hasattr(BigWorld, 'camera') else None
        if cam is not None and hasattr(cam, 'fov'):
            if 'bw_cam' not in _PB_ORIG_FOV:
                curr = getattr(cam, 'fov', 55.0)
                _PB_ORIG_FOV['bw_cam'] = curr
            if ipad_on:
                curr = getattr(cam, 'fov', 55.0)
                if curr < 3.5:
                    import math
                    rad = math.radians(ipad_deg)
                    if state_changed or abs(curr - rad) > 0.05:
                        cam.fov = rad
                else:
                    if state_changed or abs(curr - ipad_deg) > 1.0:
                        cam.fov = ipad_deg
            else:
                if state_changed:
                    cam.fov = _PB_ORIG_FOV.get('bw_cam', 55.0)
    except Exception:
        pass

    _PB_LAST_IPAD_STATE['on'] = ipad_on
    _PB_LAST_IPAD_STATE['fov'] = ipad_deg

_PB_MW_STATE = {'grass': None, 'fog': None, 'wall': None, 'cam': None, 'ug_base_y': None, 'speed_lx': None, 'speed_lz': None, 'wh_lx': None, 'wh_lz': None}

def _pb_movement_world(me, c):
    if not c or len(c) < 39 or me is None:
        return
    sp_on = c[29] >= 0.5
    sp_val = float(c[30]) if c[30] >= 1.0 else 2.0
    wh_on = c[31] >= 0.5
    nf_on = c[32] >= 0.5
    fly_on = c[33] >= 0.5
    ug_on = c[34] >= 0.5
    ng_on = c[35] >= 0.5
    para_on = c[36] >= 0.5
    cam_on = c[37] >= 0.5
    fog_on = c[38] >= 0.5

    if ng_on:
        try:
            import world
            sc = world.get_active_scene() if hasattr(world, 'get_active_scene') else None
            if sc is not None:
                if hasattr(sc, 'vegetation_enable'): sc.vegetation_enable = False
                if hasattr(sc, 'set_vegetation_visible_range'): sc.set_vegetation_visible_range(0.0)
                if hasattr(sc, 'set_EnableDetail'): sc.set_EnableDetail(False)
                for obj in (sc, getattr(sc, 'get_landscape', lambda: None)(), getattr(sc, 'landscape', None), getattr(sc, 'get_terrain', lambda: None)(), getattr(sc, 'terrain', None)):
                    if obj is not None:
                        if hasattr(obj, 'set_meadow_view_range'): obj.set_meadow_view_range(0.0, 0.0, 0.0, 0.0)
                        if hasattr(obj, 'set_vegetation_visible_range'): obj.set_vegetation_visible_range(0.0)
                        if hasattr(obj, 'vegetation_enable'): obj.vegetation_enable = False
                        if hasattr(obj, 'set_EnableDetail'): obj.set_EnableDetail(False)
        except Exception:
            pass
    elif _PB_MW_STATE.get('grass') is True:
        try:
            import world
            sc = world.get_active_scene() if hasattr(world, 'get_active_scene') else None
            if sc is not None:
                if hasattr(sc, 'vegetation_enable'): sc.vegetation_enable = True
                if hasattr(sc, 'set_vegetation_visible_range'): sc.set_vegetation_visible_range(150.0)
                if hasattr(sc, 'set_EnableDetail'): sc.set_EnableDetail(True)
                for obj in (sc, getattr(sc, 'get_landscape', lambda: None)(), getattr(sc, 'landscape', None), getattr(sc, 'get_terrain', lambda: None)(), getattr(sc, 'terrain', None)):
                    if obj is not None:
                        if hasattr(obj, 'set_meadow_view_range'): obj.set_meadow_view_range(300.0, 150.0, 50.0, 50.0)
                        if hasattr(obj, 'set_vegetation_visible_range'): obj.set_vegetation_visible_range(150.0)
                        if hasattr(obj, 'vegetation_enable'): obj.vegetation_enable = True
                        if hasattr(obj, 'set_EnableDetail'): obj.set_EnableDetail(True)
        except Exception:
            pass
    _PB_MW_STATE['grass'] = ng_on

    if _PB_MW_STATE['fog'] != fog_on:
        _PB_MW_STATE['fog'] = fog_on
        try:
            import world
            sc = world.get_active_scene() if hasattr(world, 'get_active_scene') else None
            if sc is not None:
                if hasattr(sc, 'enable_fog'):
                    sc.enable_fog = not fog_on
                if hasattr(sc, 'set_fog'):
                    if fog_on:
                        try: sc.set_fog(0, 0, 0, 50000.0, 60000.0, 0.0)
                        except Exception:
                            try: sc.set_fog(False)
                            except Exception: pass
            import BigWorld
            if hasattr(BigWorld, 'enableFog'):
                try: BigWorld.enableFog(not fog_on)
                except Exception: pass
        except Exception:
            pass

    if cam_on:
        try:
            import world
            sc = world.get_active_scene() if hasattr(world, 'get_active_scene') else None
            ac = getattr(sc, 'active_camera', None) if sc else None
            if ac is not None and hasattr(ac, 'fov'):
                if abs(getattr(ac, 'fov', 55.0) - 95.0) > 1.0:
                    ac.fov = 95.0
            import BigWorld
            cam = BigWorld.camera() if hasattr(BigWorld, 'camera') else None
            if cam is not None and hasattr(cam, 'fov'):
                if getattr(cam, 'fov', 55.0) > 3.5:
                    if abs(getattr(cam, 'fov', 55.0) - 95.0) > 1.0:
                        cam.fov = 95.0
        except Exception:
            pass

    if (sp_on and sp_val > 1.05) or wh_on:
        try:
            import math
            mult = float(sp_val) if sp_on else 1.0
            base = getattr(me, 'move_speed', 6.0) or 6.0

            if sp_on and sp_val > 1.05:
                for attr in ('moveSpeedMultiplier', 'move_speed_multiplier', 'speedMultiplier', 'speed_multiplier', 'speedRatio', 'speed_ratio'):
                    if hasattr(me, attr):
                        try: setattr(me, attr, mult)
                        except Exception: pass

                for attr in ('moveSpeed', 'move_speed', 'runSpeed', 'run_speed', 'sprintSpeed', 'sprint_speed', 'curSpeed', 'cur_speed'):
                    if hasattr(me, attr):
                        try: setattr(me, attr, base * mult)
                        except Exception: pass

                for act_obj in (getattr(me, 'actor', None), getattr(me, 'character', None), getattr(me, 'avatar', None)):
                    if act_obj is not None:
                        for m_name in ('setSpeed', 'set_speed', 'setRunSpeed', 'set_run_speed', 'setMoveSpeed', 'set_move_speed', 'setSpeedRatio', 'set_speed_ratio'):
                            if hasattr(act_obj, m_name):
                                try: getattr(act_obj, m_name)(base * mult)
                                except Exception:
                                    try: getattr(act_obj, m_name)(mult)
                                    except Exception: pass

                for v_obj in (getattr(me, 'vehicle', None), getattr(me, 'curr_vehicle', None), getattr(me, 'in_vehicle', None)):
                    if v_obj is not None:
                        for v_attr in ('speedMultiplier', 'maxSpeed', 'max_speed', 'enginePower', 'power'):
                            if hasattr(v_obj, v_attr):
                                try:
                                    cur_v = getattr(v_obj, v_attr)
                                    if isinstance(cur_v, (int, float)):
                                        setattr(v_obj, v_attr, cur_v * mult if 'multiplier' not in v_attr.lower() else mult)
                                except Exception: pass

            moving = False
            for m_attr in ('isMoving', 'is_moving', 'is_run', 'is_running', 'is_walking', 'is_walk', 'isRun', 'isWalk', 'is_sprinting'):
                if hasattr(me, m_attr):
                    val = getattr(me, m_attr)
                    if callable(val):
                        try: val = val()
                        except Exception: val = False
                    if bool(val):
                        moving = True
                        break

            if not moving:
                for st_attr in ('move_state', 'moveState', 'action_state', 'actionState'):
                    if hasattr(me, st_attr):
                        val = getattr(me, st_attr)
                        if isinstance(val, int) and val > 0:
                            moving = True
                            break

            vel = getattr(me, 'velocity', None) or getattr(getattr(me, 'physics', None), 'velocity', None)
            vx = getattr(vel, 'x', 0.0) if vel is not None and hasattr(vel, 'x') else 0.0
            vz = getattr(vel, 'z', 0.0) if vel is not None and hasattr(vel, 'z') else 0.0
            v_len = math.hypot(vx, vz)
            if v_len > 0.05:
                moving = True

            curr_pos = getattr(me, 'position', None)
            delta_x = 0.0
            delta_z = 0.0
            delta_dist = 0.0
            if curr_pos is not None and hasattr(curr_pos, 'x') and hasattr(curr_pos, 'z'):
                if _PB_MW_STATE.get('speed_lx') is not None and _PB_MW_STATE.get('speed_lz') is not None:
                    delta_x = curr_pos.x - _PB_MW_STATE['speed_lx']
                    delta_z = curr_pos.z - _PB_MW_STATE['speed_lz']
                    delta_dist = math.hypot(delta_x, delta_z)
                    if 0.003 < delta_dist < 4.0:
                        moving = True

            if moving and curr_pos is not None and hasattr(curr_pos, 'x'):
                dir_x = 0.0
                dir_z = 0.0
                has_dir = False

                if delta_dist > 0.003:
                    dir_x = delta_x / delta_dist
                    dir_z = delta_z / delta_dist
                    has_dir = True
                elif v_len > 0.05:
                    dir_x = vx / v_len
                    dir_z = vz / v_len
                    has_dir = True
                elif hasattr(me, 'yaw'):
                    try:
                        yaw = float(me.yaw)
                        dir_x = math.sin(yaw)
                        dir_z = math.cos(yaw)
                        has_dir = True
                    except Exception: pass

                if not has_dir and hasattr(me, 'getViewDirection'):
                    try:
                        vd = me.getViewDirection()
                        if vd is not None and hasattr(vd, 'x') and hasattr(vd, 'z'):
                            vd_len = math.hypot(vd.x, vd.z)
                            if vd_len > 0.001:
                                dir_x = vd.x / vd_len
                                dir_z = vd.z / vd_len
                                has_dir = True
                    except Exception: pass

                if has_dir:
                    step = 0.0
                    if sp_on and sp_val > 1.05:
                        eff_mult = min(mult, 12.0)
                        if delta_dist > 0.003:
                            step = delta_dist * (eff_mult - 1.0)
                        else:
                            step = (eff_mult - 1.0) * (base * 0.020 * 0.75)
                        if wh_on and delta_dist <= 0.003:
                            step = max(step, 0.35)
                        step = min(step, 0.95)
                    elif wh_on:
                        if delta_dist <= 0.003:
                            step = 0.35

                    if step > 0.001:
                        new_x = curr_pos.x + dir_x * step
                        new_z = curr_pos.z + dir_z * step
                        try:
                            import math3d
                            me.position = math3d.vector(new_x, curr_pos.y, new_z)
                        except Exception:
                            try:
                                me.position = type(curr_pos)(new_x, curr_pos.y, new_z)
                            except Exception: pass

            if curr_pos is not None and hasattr(curr_pos, 'x'):
                _PB_MW_STATE['speed_lx'] = getattr(me.position, 'x', curr_pos.x)
                _PB_MW_STATE['speed_lz'] = getattr(me.position, 'z', curr_pos.z)
        except Exception:
            pass
    else:
        _PB_MW_STATE['speed_lx'] = None
        _PB_MW_STATE['speed_lz'] = None
        for attr in ('moveSpeedMultiplier', 'move_speed_multiplier', 'speedMultiplier', 'speed_multiplier', 'speedRatio', 'speed_ratio'):
            if hasattr(me, attr):
                try: setattr(me, attr, 1.0)
                except Exception: pass

    if _PB_MW_STATE['wall'] != wh_on:
        _PB_MW_STATE['wall'] = wh_on
        try:
            cc = getattr(me, 'camCtrlComp', None)
            if cc is not None and hasattr(cc, 'collisions_enabled'):
                cc.collisions_enabled = not wh_on
            ph = getattr(me, 'physics', None)
            if ph is not None:
                for attr in ('collideWithScene', 'collideWithWorld', 'userCollide', 'sceneCollide'):
                    if hasattr(ph, attr):
                        try: setattr(ph, attr, not wh_on)
                        except Exception: pass
            for c_name in ('colliderComp', 'collisionComp', 'sceneCollider'):
                comp = getattr(me, c_name, None)
                if comp is not None:
                    for attr in ('collideWithScene', 'collideWithWorld', 'collisions_enabled', 'enabled'):
                        if hasattr(comp, attr):
                            try: setattr(comp, attr, not wh_on)
                            except Exception: pass
        except Exception:
            pass
    if wh_on:
        try:
            import BigWorld
            ents = BigWorld.entities
            for e in list(ents.values()):
                if e is me or not hasattr(e, 'position'):
                    continue
                if not _pb_is_player_type(type(e).__name__):
                    continue
                m = getattr(e, 'model', None)
                if m is not None:
                    if hasattr(m, 'enable_smooth_outline_depth_test'):
                        m.enable_smooth_outline_depth_test(False)
                    if hasattr(m, 'setDepthTest'):
                        m.setDepthTest(False)
        except Exception:
            pass

    if nf_on:
        try:
            for attr in ('fallDamage', 'fall_damage', 'isFalling', 'is_falling'):
                if hasattr(me, attr):
                    setattr(me, attr, 0 if 'damage' in attr.lower() else False)
            ph = getattr(me, 'physics', None)
            if ph is not None:
                for attr in ('fall_velocity', 'fall_speed', 'velocity_y'):
                    if hasattr(ph, attr): setattr(ph, attr, 0.0)
        except Exception:
            pass

    if fly_on:
        try:
            import math3d
            ph = getattr(me, 'physics', None)
            if ph is not None:
                for attr in ('fall_velocity', 'fall_speed', 'velocity_y', 'gravity', 'gravity_scale'):
                    if hasattr(ph, attr):
                        try: setattr(ph, attr, 0.0 if 'gravity' not in attr.lower() else 0.05)
                        except Exception: pass
            for act in (me, getattr(me, 'actor', None), getattr(me, 'character', None)):
                if act is not None:
                    for attr in ('jumpSpeed', 'jump_speed', 'jumpHeight', 'jump_height', 'jumpImpulse', 'jump_impulse'):
                        if hasattr(act, attr):
                            try: setattr(act, attr, 20.5)
                            except Exception: pass
                    for attr in ('fallDamage', 'fall_damage', 'isFalling', 'is_falling'):
                        if hasattr(act, attr):
                            try: setattr(act, attr, 0 if 'damage' in attr.lower() else False)
                            except Exception: pass

            vd = me.getViewDirection() if hasattr(me, 'getViewDirection') else None
            ascend = 0.0
            if vd is not None and hasattr(vd, 'y'):
                if vd.y > 0.15:
                    ascend = (vd.y - 0.10) * 0.45
                elif vd.y < -0.20:
                    ascend = (vd.y + 0.10) * 0.40

            if abs(ascend) > 0.01:
                curr_p = me.position
                new_y = max(1.0, curr_p.y + ascend)
                try:
                    me.position = math3d.vector(curr_p.x, new_y, curr_p.z)
                except Exception:
                    try: me.position = type(curr_p)(curr_p.x, new_y, curr_p.z)
                    except Exception: pass
        except Exception:
            pass

    if ug_on:
        try:
            import math3d
            if _PB_MW_STATE['ug_base_y'] is None:
                _PB_MW_STATE['ug_base_y'] = me.position.y
                me.position = math3d.vector(me.position.x, me.position.y - 3.2, me.position.z)
        except Exception:
            pass
    else:
        if _PB_MW_STATE['ug_base_y'] is not None:
            try:
                import math3d
                me.position = math3d.vector(me.position.x, me.position.y + 3.2, me.position.z)
            except Exception:
                pass
            _PB_MW_STATE['ug_base_y'] = None

    if para_on:
        try:
            import math3d
            me.position = math3d.vector(me.position.x, me.position.y - 2.8, me.position.z)
        except Exception:
            pass

    nr_on = len(c) > 39 and c[39] >= 0.5
    if nr_on:
        try:
            cc = getattr(me, 'camCtrlComp', None)
            if cc is not None:
                for attr in ('curRecoil', 'cur_recoil', 'recoilPitch', 'recoil_pitch', 'recoilYaw', 'recoil_yaw', 'recoil', 'recoil_speed', 'cameraShake', 'camera_shake', 'spread', 'curSpread', 'cur_spread'):
                    if hasattr(cc, attr):
                        try: setattr(cc, attr, 0.0)
                        except Exception: pass
            for wep in (getattr(me, 'weapon', None), getattr(me, 'cur_weapon', None)):
                if wep is not None:
                    for attr in ('recoil', 'recoil_factor', 'spread', 'recoil_pitch', 'recoil_yaw'):
                        if hasattr(wep, attr):
                            try: setattr(wep, attr, 0.0)
                            except Exception: pass
        except Exception:
            pass

def _pb_actions(me, cands, allc):
    import time
    global _PB_ACTION_TS
    now = time.time()
    if now - _PB_ACTION_TS < 0.020:
        return
    _PB_ACTION_TS = now
    try:
        _pb_apply_ipad_fov(_pb_aimcfg())
    except Exception:
        pass
    try:
        _pb_movement_world(me, _pb_aimcfg())
    except Exception:
        pass
    try:
        _pb_nrs_tick()
        _pb_silent(me, cands, allc)
    except Exception:
        pass
    try:
        _pb_aim(me, cands, allc)
    except Exception:
        pass
    try:
        _pb_autofire(me, cands)
    except Exception:
        try:
            if _PB_AF['on']:
                _pb_fire_invoke(_pb_fire_objects(me), False)
        except Exception:
            pass
        _PB_AF['on'] = False; _PB_AF['owner'] = None

def _pb_esp(g):
    if g != _PB_GEN:
        return
    delay = 0.250
    mo = 0; ne = 0; na = 0
    me6 = [0.0] * 9
    try:
        me = BigWorld.player()
        if me is None or not hasattr(me, 'position'):
            raise _PBNotBattle('not battle')
        tn0 = type(me).__name__
        if not _pb_is_player_type(tn0) and not hasattr(me, 'model'):
            raise _PBNotBattle('not battle')
        delay = 0.020
        ents = BigWorld.entities
        ne = len(ents)
        mo = 1
        mp = me.position
        me6[0], me6[1], me6[2] = mp.x, mp.y, mp.z
        try:
            v = me.getViewPos()
            me6[3], me6[4], me6[5] = v.x, v.y, v.z
        except Exception:
            pass
        try:
            d = me.getViewDirection()
            me6[6], me6[7], me6[8] = d.x, d.y, d.z
        except Exception:
            pass
        lines = []
        cands = []
        allc = []
        nb = 0
        rows = 0
        bcand = []
        live_bone_ids = set()
        mode, w2s = _pb_w2s_get()
        frame_cfg = _pb_aimcfg()
        hide_bots = bool(frame_cfg and len(frame_cfg) >= 26 and frame_cfg[25] >= 0.5)
        draw_skeleton = not frame_cfg or len(frame_cfg) < 27 or frame_cfg[26] >= 0.5
        try:
            cam = _pb_cam_pos(me)
        except Exception:
            cam = me.getViewPos() if hasattr(me, 'getViewPos') else mp
        for k in list(ents.keys()):
            e = ents[k]
            if e is me or not hasattr(e, 'position'):
                continue
            tn = type(e).__name__
            if not _pb_is_player_type(tn):
                continue
            try:
                eid = int(getattr(e, 'id', k) or k)
                is_alive = True
                try:
                    if hasattr(e, 'isAlive'):
                        v_alive = e.isAlive
                        is_alive = bool(v_alive() if callable(v_alive) else v_alive)
                    elif hasattr(e, 'hp'):
                        current_hp, unused_max_hp = _pb_health(e)
                        is_alive = current_hp != 0
                except Exception:
                    is_alive = True
                if not is_alive:
                    _PB_BMAP.pop(eid, None)
                    continue
                live_bone_ids.add(eid)
                na += 1
                p = e.position
                vid = getattr(e, 'vehicleEntID', 0) or 0
                if vid:
                    try:
                        p = ents[vid].position
                    except Exception:
                        pass
                try:
                    vp = e.getViewPos()
                    vx, vy, vz = vp.x, vp.y, vp.z
                except Exception:
                    vx, vy, vz = p.x, p.y + 18.0, p.z
                enemy = _pb_enemy(me, e)
                if enemy == 0 and _pb_squad_member(me, e):
                    _PB_BMAP.pop(eid, None)
                    continue
                vis = _pb_vis(me, e, cam)
                hp, mhp = _pb_health(e)
                dx = p.x - mp.x; dy = p.y - mp.y; dz = p.z - mp.z
                dist = (dx * dx + dy * dy + dz * dz) ** 0.5
                try:
                    kn = 1 if (callable(getattr(e, 'isWounded', None)) and e.isWounded()) else 0
                except Exception:
                    kn = 0
                bot = 0
                nick = getattr(e, 'nickname', '') or getattr(e, 'playerName', '') or ''
                if '[BOT]' in nick or 'bot' in nick.lower():
                    bot = 1
                else:
                    try:
                        if hasattr(me, 'robotInAoI') and me.robotInAoI and e.id in me.robotInAoI:
                            bot = 1
                    except Exception:
                        pass
                    try:
                        if getattr(e, 'isRobot', False):
                            bot = 1
                    except Exception:
                        pass
                try:
                    team = _pb_team_id(e) or 0
                except Exception:
                    team = 0
                sh = int(getattr(e, 'shieldHp', 0) or 0)
                msh = int(getattr(e, 'maxShieldHp', 0) or 0)
                fs = None; hs = None
                if mode >= 1:
                    hp_w = _pb_head_pos(e, 1.0)
                    if hp_w is None:
                        hp_w = _pb_head(e)
                        if hp_w is None:
                            hp_w = _pb_boneworld(e, 'head')
                    if hp_w is not None:
                        try:
                            import math
                            offset2 = (hp_w.x-p.x)**2 + (hp_w.y-p.y)**2 + (hp_w.z-p.z)**2
                            if math.isnan(offset2) or math.isinf(offset2) or offset2 > 1000000.0:
                                hp_w = None
                        except Exception:
                            hp_w = None
                    if hp_w is None:
                        try:
                            import math3d
                            hp_w = math3d.vector(p.x, p.y + 17.5, p.z)
                        except Exception:
                            try:
                                hp_w = type(p)(p.x, p.y + 17.5, p.z)
                            except Exception:
                                pass
                    if hp_w is not None:
                        try:
                            hs = w2s(hp_w)
                        except Exception:
                            hs = None
                    try:
                        fs = w2s(p)
                    except Exception:
                        fs = None
                    if fs is not None and hs is None:
                        try:
                            cs = w2s(e.getModelCenter())
                            hs = (cs[0], fs[1] - 2.0 * (fs[1] - cs[1]))
                        except Exception:
                            hs = (fs[0], fs[1] - 40.0)
                    elif hs is not None and fs is None:
                        fs = (hs[0], hs[1] + 40.0)
                bone_values = []
                skeleton_visible = mode >= 1 and draw_skeleton and fs is not None and hs is not None
                if skeleton_visible and frame_cfg and len(frame_cfg) >= 10:
                    feet_screen = _pb_aim_screen(fs, frame_cfg[8], frame_cfg[9], frame_cfg[16:22])
                    head_screen = _pb_aim_screen(hs, frame_cfg[8], frame_cfg[9], frame_cfg[16:22])
                    skeleton_visible = abs(feet_screen[1] - head_screen[1]) >= 4.0
                if skeleton_visible:
                    for bone_key in _PB_SKEL:
                        joint = _pb_joint_screen(e, bone_key, w2s)
                        if joint is None:
                            bone_values.append((0, 0.0, 0.0))
                        else:
                            bone_values.append((1, joint[0], joint[1]))
                else:
                    bone_values = [(0, 0.0, 0.0) for bone_key in _PB_SKEL]
                on = 1 if (fs is not None or hs is not None) else 0
                if fs is None and hs is None:
                    fs = (0.0, 0.0); hs = (0.0, 0.0)
                elif fs is None:
                    fs = (hs[0], hs[1] + 40.0)
                elif hs is None:
                    hs = (fs[0], fs[1] - 40.0)
                nm = nick.strip().replace(' ', '_').replace('\t', '_').replace('\r', '').replace('\n', '').replace('|', '_')[:24]
                if not nm:
                    nm = 'Bot' if bot else 'Player'
                bone_text = ';'.join(['%d %.5f %.5f' % v for v in bone_values])
                lines.append('%.4f %.4f %.4f %.4f %d %.2f %.2f %.2f %.2f %.2f %.2f %d %d %d %d %.3f %d %d %d %d %d %d %d %s|%s|%d' %
                    (fs[0], fs[1], hs[0], hs[1], on, p.x, p.y, p.z, vx, vy, vz,
                     hp, mhp, enemy, vis, dist, kn, bot, team, sh, msh, 1 if vid else 0, eid, nm, bone_text, _pb_weapon_kind(e)))
                aim_candidate = enemy != 0 and not (hide_bots and bot)
                if on and aim_candidate:
                    cands.append((e, fs, hs, kn, vis, p, dist))
                if aim_candidate:
                    allc.append((e, kn, vis, p, dist))
                rows += 1
            except Exception:
                pass
        for cached_id in list(_PB_BMAP.keys()):
            if cached_id not in live_bone_ids:
                _PB_BMAP.pop(cached_id, None)
        _pb_actions(me, cands, allc)
        bs = _PB_BS['s'] if _PB_BS['s'] is not None else -2
        trig = 1 if ((_PB_SIL['t'] is not None and _PB_SIL.get('tv') and _PB_SIL.get('dist', 9999) / 10.0 <= 200.0) or _PB_TRG['aim']) else 0
        hdr = '%d %s %d %d %d' % (len(lines) - nb, ' '.join(['%.3f' % x for x in me6]), mode, bs, trig)
        globals()['_PB_T'] = globals().get('_PB_T', 0) + 1
        _pb_publish(_PB_PATH, hdr + '\n' + '\n'.join(lines))
        global _PB_EMPTY
        _PB_EMPTY = False
        import time
        global _PB_DIAG_TS
        if time.time() - _PB_DIAG_TS >= 1.0:
            _PB_DIAG_TS = time.time()
            _pb_diag(me, ents)
    except _PBNotBattle:
        try:
            _pb_empty()
        except Exception:
            pass
    except Exception:
        try:
            import traceback, time
            now_err = time.time()
            if now_err - globals().get('_PB_LAST_ERR_TS', 0.0) >= 2.0:
                globals()['_PB_LAST_ERR_TS'] = now_err
                open(_RYOMENX_DIR + '/esp_last_err.txt', 'w').write(traceback.format_exc())
        except Exception:
            pass
    try:
        if globals().get('_PB_T', 0) % 50 == 1:
            _pb_publish(_PB_STAT, '%d %d %d %.2f %.2f %.2f' % (mo, ne, na, me6[0], me6[1], me6[2]))
    except Exception:
        pass
    try:
        BigWorld.callback(delay, lambda: _pb_esp(g))
    except Exception:
        pass
def _pb_v(o, n):
    try:
        v = getattr(o, n)
        if callable(v):
            v = v()
        return '%.1f %.1f %.1f' % (v.x, v.y, v.z)
    except Exception as ex:
        return '-'
def _pb_mv(o):
    try:
        v = o.model.position
        return '%.1f %.1f %.1f' % (v.x, v.y, v.z)
    except Exception:
        return '-'
def _pb_team_snapshot(value, depth=0):
    if value is None or isinstance(value, (bool, int, float, str, unicode)):
        return value
    if depth >= 3:
        return {'type': type(value).__name__}
    if isinstance(value, dict):
        return dict((str(k), _pb_team_snapshot(v, depth + 1))
                    for k, v in list(value.items())[:32])
    if isinstance(value, (list, tuple, set)):
        return [_pb_team_snapshot(v, depth + 1) for v in list(value)[:32]]
    result = {'type': type(value).__name__}
    for name in dir(value):
        if name.startswith('__'):
            continue
        if name not in ('id', 'nickname', 'playerName') and not any(
                token in name.lower() for token in ('team', 'squad', 'party', 'camp')):
            continue
        try:
            field = getattr(value, name)
            result[name] = '<callable>' if callable(field) else _pb_team_snapshot(field, depth + 1)
        except Exception:
            result[name] = '<unavailable>'
    return result

def _pb_team_diag(me, ents):
    import json
    import time
    now = time.time()
    if now - globals().get('_PB_TEAM_DIAG_TS', 0) < 5.0:
        return
    globals()['_PB_TEAM_DIAG_TS'] = now
    report = {'build': '3.6-squad-filter', 'local': _pb_team_snapshot(me), 'players': []}
    for entity in list(ents.values()):
        if entity is me or not hasattr(entity, 'position'):
            continue
        if not _pb_is_player_type(type(entity).__name__):
            continue
        row = {'fields': _pb_team_snapshot(entity), 'classification': _pb_enemy(me, entity)}
        for label, owner, other in (('local_api', me, entity), ('remote_api', entity, me)):
            try:
                row[label] = _pb_team_snapshot(owner.IsSameTeam(other))
            except Exception as ex:
                row[label] = {'error': str(ex)[:160]}
        report['players'].append(row)
        if len(report['players']) >= 64:
            break
    data = json.dumps(report, ensure_ascii=True, indent=2)
    for path in (_RYOMENX_DIR + '/RyomenX-team-diagnostics.json',):
        try:
            with open(path, 'w') as output:
                output.write(data)
        except Exception:
            pass

def _pb_diag(me, ents):
    try:
        _pb_team_diag(me, ents)
    except Exception:
        pass
    try:
        out = ['ESP DIAG (compare which position is real)']
        for tag, e in [('ME', me)] + [('E', ents[k]) for k in list(ents.keys()) if ents[k] is not me and _pb_is_player_type(type(ents[k]).__name__)][:6]:
            out.append('%s %s' % (tag, (getattr(e, 'nickname', '') or getattr(e, 'playerName', '') or '')[:16]))
            out.append('  position    ' + _pb_v(e, 'position'))
            out.append('  model.pos   ' + _pb_mv(e))
            out.append('  modelCenter ' + _pb_v(e, 'getModelCenter'))
            out.append('  viewPos     ' + _pb_v(e, 'getViewPos'))
        out.append('ME viewDir ' + _pb_v(me, 'getViewDirection'))
        open(_RYOMENX_DIR + '/ros_esp_diag.txt', 'w').write('\n'.join(out))
    except Exception as ex:
        try:
            open(_RYOMENX_DIR + '/ros_esp_diag.txt', 'w').write('diag err ' + str(ex))
        except Exception:
            pass
_pb_empty()
BigWorld.callback(0.250, lambda g=_PB_GEN: _pb_esp(g))

