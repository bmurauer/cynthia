"""Thin ngspice wrapper for env.cir. Requires ngspice and numpy."""
import os, re, subprocess, tempfile
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
BASE = open(os.path.join(HERE, 'env.cir')).read()
DEFAULTS = dict(SAT=9.6, VP=11.5, VOS=0, XA=0.5, XR=0.5, SHAPE=0, MODE=0,
                CVA=0, CVR=0, TG=1, TP=100)
PROBES = ['v(ct)', 'v(out)', 'v(s)', 'v(q)', 'i(v.xota.vabc)', 'v(gin)', 'v(g)']
NAMES = ['ct', 'out', 's', 'q', 'iabc', 'gate', 'g']


def sim(tstop, step, **params):
    """Transient run; returns dict of numpy arrays (t, ct, out, s, q, iabc)."""
    p = dict(DEFAULTS, **params)
    txt = re.sub(r'^\.param .*$', '.param ' + ' '.join(f'{k}={v}' for k, v in p.items()),
                 BASE, count=1, flags=re.M)
    with tempfile.TemporaryDirectory() as tmp:
        dat = os.path.join(tmp, 'out.dat')
        cir = os.path.join(tmp, 'run.cir')
        txt = txt.replace('.include core.lib', f'.include {os.path.join(HERE, "core.lib")}')
        txt += f"""
.control
set wr_singlescale
tran {step} {tstop} 0 {step}
wrdata {dat} {' '.join(PROBES)}
.endc
.end
"""
        open(cir, 'w').write(txt)
        r = subprocess.run(['ngspice', '-b', cir], capture_output=True, text=True)
        if not os.path.exists(dat):
            raise RuntimeError(r.stdout + r.stderr)
        d = np.loadtxt(dat)
    out = dict(t=d[:, 0])
    out.update({n: d[:, i + 1] for i, n in enumerate(NAMES)})
    return out


def cross(t, y, lvl, rising=True, after=0):
    """Time of the first crossing of lvl after t=after (linear interpolation)."""
    m = t >= after
    t, y = t[m], y[m]
    if rising:
        idx = np.where((y[:-1] < lvl) & (y[1:] >= lvl))[0]
    else:
        idx = np.where((y[:-1] > lvl) & (y[1:] <= lvl))[0]
    if len(idx) == 0:
        return None
    i = idx[0]
    return t[i] + (lvl - y[i]) * (t[i + 1] - t[i]) / (y[i + 1] - y[i])
